/* tree.c : tree-like filesystem, built on DAG filesystem
 *
 * ====================================================================
 * Copyright (c) 2000 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 * ====================================================================
 */


/* The job of this layer is to take a filesystem with lots of node
   sharing going on --- the real DAG filesystem as it appears in the
   database --- and make it look and act like an ordinary tree
   filesystem, with no sharing.

   We do just-in-time cloning: you can walk from some unfinished
   transaction's root down into directories and files shared with
   committed revisions; as soon as you try to change something, the
   appropriate nodes get cloned (and parent directory entries updated)
   invisibly, behind your back.  Any other references you have to
   nodes that have been cloned by other changes, even made by other
   processes, are automatically updated to point to the right clones.  */


#include <stdlib.h>
#include <string.h>
#include "svn_error.h"
#include "svn_fs.h"
#include "skel.h"
#include "id.h"
#include "fs.h"
#include "err.h"
#include "trail.h"
#include "txn-table.h"
#include "txn.h"
#include "dag.h"
#include "tree.h"



/* The root structure.  */

typedef enum root_kind_t {
  unspecified_root = 0,
  revision_root,
  transaction_root
} root_kind_t;

struct svn_fs_root_t
{

  /* What filesystem does this root belong to?  */
  svn_fs_t *fs;

  /* All data belonging to this root is allocated in this pool.
     Destroying this pool will correctly free all resources the root
     holds.  */
  apr_pool_t *pool;

  /* What kind of root is this?  */
  root_kind_t kind;

  /* For transaction roots (i.e., KIND == transaction_root), the name of
     that transaction, allocated in POOL.  */
  const char *txn;

  /* For revision roots (i.e., KIND == revision_root), the number of
     that revision.  */
  svn_revnum_t rev;

  /* For revision roots, this is a dag node for the revision's root
     directory.  For transaction roots, we open the root directory
     afresh every time, since the root may have been cloned, or
     the transaction may have disappeared altogether.  */
  dag_node_t *root_dir;
};



/* Creating root objects.  */


/* Construct a new root object in FS, allocated from POOL.  */
static svn_fs_root_t *
make_root (svn_fs_t *fs,
           apr_pool_t *pool)
{
  /* We create a subpool for each root object to allow us to implement
     svn_fs_close_root.  */
  apr_pool_t *subpool = svn_pool_create (pool);
  svn_fs_root_t *root = apr_pcalloc (subpool, sizeof (*root));

  root->fs = fs;
  root->pool = pool;

  return root;
}


/* Construct a root object referring to the root of REVISION in FS,
   whose root directory is ROOT_DIR.  Create the new root in POOL.  */
static svn_fs_root_t *
make_revision_root (svn_fs_t *fs,
                    svn_revnum_t rev,
                    dag_node_t *root_dir,
                    apr_pool_t *pool)
{
  svn_fs_root_t *root = make_root (fs, pool);
  root->kind = revision_root;
  root->rev = rev;
  root->root_dir = root_dir;

  return root;
}


/* Construct a root object referring to the root of the transaction
   named TXN in FS.  Create the new root in POOL.  */
static svn_fs_root_t *
make_txn_root (svn_fs_t *fs,
               const char *txn,
               apr_pool_t *pool)
{
  svn_fs_root_t *root = make_root (fs, pool);
  root->kind = transaction_root;
  root->txn = apr_pstrdup (root->pool, txn);

  return root;
}



/* Constructing nice error messages for roots.  */

/* Return a detailed `file not found' error message for PATH in ROOT.  */
static svn_error_t *
not_found (svn_fs_root_t *root, const char *path)
{
  svn_fs_t *fs = root->fs;

  if (root->kind == transaction_root)
    return
      svn_error_createf
      (SVN_ERR_FS_NOT_FOUND, 0, 0, fs->pool,
       "file not found: filesystem `%s', transaction `%s', path `%s'",
       fs->env_path, root->txn, path);
  else if (root->kind == revision_root)
    return
      svn_error_createf
      (SVN_ERR_FS_NOT_FOUND, 0, 0, fs->pool,
       "file not found: filesystem `%s', revision `%ld', path `%s'",
       fs->env_path, root->rev, path);
  else
    abort ();
}


/* Return a detailed `file already exists' message for PATH in ROOT.  */
static svn_error_t *
already_exists (svn_fs_root_t *root, const char *path)
{
  svn_fs_t *fs = root->fs;

  if (root->kind == transaction_root)
    return
      svn_error_createf
      (SVN_ERR_FS_ALREADY_EXISTS, 0, 0, fs->pool,
       "file already exists: filesystem `%s', transaction `%s', path `%s'",
       fs->env_path, root->txn, path);
  else if (root->kind == revision_root)
    return
      svn_error_createf
      (SVN_ERR_FS_ALREADY_EXISTS, 0, 0, fs->pool,
       "file already exists: filesystem `%s', revision `%ld', path `%s'",
       fs->env_path, root->rev, path);
  else
    abort ();
}




/* Getting dag nodes for roots.  */


/* Set *NODE_P to a freshly opened dag node referring to the root
   directory of ROOT, as part of TRAIL.  */
static svn_error_t *
root_node (dag_node_t **node_p,
           svn_fs_root_t *root,
           trail_t *trail)
{
  if (root->kind == revision_root)
    {
      /* It's a revision root, so we already have its root directory
         opened.  */
      *node_p = svn_fs__dag_dup (root->root_dir, trail);
      return SVN_NO_ERROR;
    }
  else if (root->kind == transaction_root)
    {
      /* It's a transaction root.  Open a fresh copy.  */
      return svn_fs__dag_txn_root (node_p, root->fs, root->txn, trail);
    }
  else
    abort ();
}


/* Set *NODE_P to a mutable root directory for ROOT, cloning if
   necessary, as part of TRAIL.  ROOT must be a transaction root.  Use
   ERROR_PATH in error messages.  */
static svn_error_t *
mutable_root_node (dag_node_t **node_p,
                   svn_fs_root_t *root,
                   const char *error_path,
                   trail_t *trail)
{
  if (root->kind == transaction_root)
    return svn_fs__dag_clone_root (node_p, root->fs, root->txn, trail);
  else
    /* If it's not a transaction root, we can't change its contents.  */
    return svn_fs__err_not_mutable (root->fs, root->rev, error_path);
}



/* Simple root operations.  */

void
svn_fs_close_root (svn_fs_root_t *root)
{
  apr_pool_destroy (root->pool);
}


svn_fs_t *
svn_fs_root_fs (svn_fs_root_t *root)
{
  return root->fs;
}


int
svn_fs_is_txn_root (svn_fs_root_t *root)
{
  return root->kind == transaction_root;
}


int
svn_fs_is_revision_root (svn_fs_root_t *root)
{
  return root->kind == revision_root;
}


const char *
svn_fs_txn_root_name (svn_fs_root_t *root,
                      apr_pool_t *pool)
{
  if (root->kind == transaction_root)
    return apr_pstrdup (pool, root->txn);
  else
    return 0;
}


svn_revnum_t
svn_fs_revision_root_revision (svn_fs_root_t *root)
{
  if (root->kind == revision_root)
    return root->rev;
  else
    return -1;
}



/* Traversing directory paths.  */


/* A linked list representing the path from a node up to a root
   directory.  We use this for cloning, and for operations that need
   to deal with both a node and its parent directory.  For example, a
   `delete' operation needs to know that the node actually exists, but
   also needs to change the parent directory.  */
typedef struct parent_path_t
{
  
  /* A node along the path.  This could be the final node, one of its
     parents, or the root.  Every parent path ends with an element for
     the root directory.  */
  dag_node_t *node;

  /* The name NODE has in its parent directory.  This is zero for the
     root directory, which (obviously) has no name in its parent.  */
  char *entry;

  /* The parent of NODE, or zero if NODE is the root directory.  */
  struct parent_path_t *parent;
  
} parent_path_t;


/* Allocate a new parent_path_t node from POOL, referring to NODE,
   ENTRY, and PARENT.  */
static parent_path_t *
make_parent_path (dag_node_t *node,
                  char *entry,
                  parent_path_t *parent,
                  apr_pool_t *pool)
{
  parent_path_t *parent_path = apr_pcalloc (pool, sizeof (*parent_path));

  parent_path->node = node;
  parent_path->entry = entry;
  parent_path->parent = parent;

  return parent_path;
}


/* Return a null-terminated copy of the first component of PATH,
   allocated in POOL.  If path is empty, or consists entirely of
   slashes, return the empty string.

   If the component is followed by one or more slashes, we set *NEXT_P
   to point after the slashes.  If the component ends PATH, we set
   *NEXT_P to zero.  This means:
   - If *NEXT_P is zero, then the component ends the PATH, and there
     are no trailing slashes in the path.
   - If *NEXT_P points at PATH's terminating null character, then
     the component returned was the last, and PATH ends with one or more
     slash characters.
   - Otherwise, *NEXT_P points to the beginning of the next component
     of PATH.  You can pass this value to next_entry_name to extract
     the next component.  */

static char *
next_entry_name (const char **next_p,
                 const char *path,
                 apr_pool_t *pool)
{
  const char *end;

  /* Find the end of the current component.  */
  end = strchr (path, '/');

  if (! end)
    {
      /* The path contains only one component, with no trailing
         slashes.  */
      *next_p = 0;
      return apr_pstrdup (pool, path);
    }
  else
    {
      /* There's a slash after the first component.  Skip over an arbitrary
         number of slashes to find the next one.  */
      const char *next = end;
      while (*next == '/')
        next++;
      *next_p = next;
      return apr_pstrndup (pool, path, end - path);
    }
}


/* Flags for open_path.  */
typedef enum open_path_flags_t {

  /* The last component of the PATH need not exist.  (All parent
     directories must exist, as usual.)  If the last component doesn't
     exist, simply leave the `node' member of the bottom parent_path
     component zero.  */
  open_path_last_optional = 1,

} open_path_flags_t;


/* Open the node identified by PATH in ROOT, as part of TRAIL.  Set
   *PARENT_PATH_P to a path from the node up to ROOT, allocated in
   TRAIL->pool.  The resulting *PARENT_PATH_P value is guaranteed to
   contain at least one element, for the root directory.

   If FLAGS & open_path_last_optional is zero, return an error if the
   node PATH refers to does not exist.  If it is non-zero, require all
   the parent directories to exist as normal, but if the final path
   component doesn't exist, simply return a path whose bottom `node'
   member is zero.  This option is useful for callers that create new
   nodes --- we find the parent directory for them, and tell them
   whether the entry exists already.  */
static svn_error_t *
open_path (parent_path_t **parent_path_p,
           svn_fs_root_t *root,
           const char *path,
           int flags,
           trail_t *trail)
{
  svn_fs_t *fs = root->fs;
  apr_pool_t *pool = trail->pool;

  /* The directory we're currently looking at.  */
  dag_node_t *here;

  /* The path from HERE up to the root.  */
  parent_path_t *parent_path;
  
  /* The portion of PATH we haven't traversed yet.  */
  const char *rest = path;

  SVN_ERR (root_node (&here, root, trail));
  parent_path = make_parent_path (here, 0, 0, pool);

  /* Whenever we are at the top of this loop:
     - HERE is our current directory,
     - REST is the path we're going to find in HERE, and 
     - PARENT_PATH includes HERE and all its parents.  */
  for (;;)
    {
      const char *next;
      char *entry;
      dag_node_t *child;

      /* Parse out the next entry from the path.  */
      entry = next_entry_name (&next, rest, pool);

      if (*entry == '\0')
        /* Given the behavior of next_entry_name, this happens when
           the path either starts or ends with a slash.  In either
           case, we stay put: the current directory stays the same,
           and we add nothing to the parent path.  */
        child = here;
      else
        {
          /* If we found a directory entry, follow it.  */
          svn_error_t *svn_err = svn_fs__dag_open (&child, here, entry, trail);

          /* "file not found" requires special handling.  */
          if (svn_err && svn_err->apr_err == SVN_ERR_FS_NOT_FOUND)
            {
              /* If this was the last path component, and the caller
                 said it was optional, then don't return an error;
                 just put a zero node pointer in the path.  */
              if ((flags & open_path_last_optional)
                  && (! next || *next == '\0'))
                {
                  parent_path = make_parent_path (0, entry, parent_path, pool);
                  break;
                }
              else
                /* Build a better error message than svn_fs__dag_open
                   can provide, giving the root and full path name.  */
                return not_found (root, path);
            }

          /* Other errors we return normally.  */
          SVN_ERR (svn_err);

          parent_path = make_parent_path (child, entry, parent_path, pool);
        }
      
      /* Are we finished traversing the path?  */
      if (! next)
        break;

      /* The path isn't finished yet; we'd better be in a directory.  */
      if (! svn_fs__dag_is_directory (child))
        return svn_fs__err_not_directory (fs, path);

      rest = next;
      here = child;
    }

  *parent_path_p = parent_path;
  return SVN_NO_ERROR;
}


/* Make the node referred to by PARENT_PATH mutable, if it isn't
   already, as part of TRAIL.  ROOT must be the root from which
   PARENT_PATH descends.  Clone any parent directories as needed.
   Adjust the dag nodes in PARENT_PATH to refer to the clones.  Use
   ERROR_PATH in error messages.  */
static svn_error_t *
make_path_mutable (svn_fs_root_t *root,
                   parent_path_t *parent_path,
                   const char *error_path,
                   trail_t *trail)
{
  dag_node_t *clone;

  /* Is the node mutable already?  */
  if (svn_fs__dag_is_mutable (parent_path->node))
    return SVN_NO_ERROR;

  /* Are we trying to clone the root, or somebody's child node?  */
  if (parent_path->parent)
    {
      /* We're trying to clone somebody's child. 
         Make sure our parent is mutable.  */
      SVN_ERR (make_path_mutable (root, parent_path->parent, error_path,
                                  trail));
      
      /* Now make this node mutable.  */
      SVN_ERR (svn_fs__dag_clone_child (&clone,
                                        parent_path->parent->node,
                                        parent_path->entry,
                                        trail));
    }
  else
    /* We're trying to clone the root directory.  */
    SVN_ERR (mutable_root_node (&clone, root, error_path, trail));

  /* Update the PARENT_PATH link to refer to the clone.  */
  svn_fs__dag_close (parent_path->node);
  parent_path->node = clone;
  return SVN_NO_ERROR;
}



/* Generic node operations.  */


struct node_prop_args
{
  svn_string_t **value_p;
  svn_fs_root_t *root;
  const char *path;
  svn_string_t *propname;
};


static svn_error_t *
txn_body_node_prop (void *baton,
                    trail_t *trail)
{
  struct node_prop_args *args = baton;
  parent_path_t *parent_path;
  skel_t *proplist, *prop;

  SVN_ERR (open_path (&parent_path, args->root, args->path, 0, trail));
  SVN_ERR (svn_fs__dag_get_proplist (&proplist, parent_path->node, trail));
  
  /* Search the proplist for a property with the right name.  */
  for (prop = proplist->children; prop; prop = prop->next->next)
    {
      skel_t *name = prop;
      skel_t *value = prop->next;

      if (name->len == args->propname->len
          && ! memcmp (name->data, args->propname->data, name->len))
        {
          *(args->value_p) = svn_string_ncreate (value->data, value->len,
                                                 trail->pool);
          return SVN_NO_ERROR;
        }
    }

  *(args->value_p) = 0;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_node_prop (svn_string_t **value_p,
                  svn_fs_root_t *root,
                  const char *path,
                  svn_string_t *propname,
                  apr_pool_t *pool)
{
  struct node_prop_args args;
  svn_string_t *value;

  args.value_p  = &value;
  args.root     = root;
  args.path     = path;
  args.propname = propname;
  SVN_ERR (svn_fs__retry_txn (root->fs, txn_body_node_prop, &args, pool));

  *value_p = value;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_node_proplist (apr_hash_t **table_p,
                      svn_fs_root_t *root,
                      const char *path,
                      apr_pool_t *pool)
{
  abort ();
}


struct change_node_prop_args {
  svn_fs_root_t *root;
  const char *path;
  svn_string_t *name;
  svn_string_t *value;
};


static svn_error_t *
txn_body_change_node_prop (void *baton,
                           trail_t *trail)
{
  struct change_node_prop_args *args = baton;
  parent_path_t *parent_path;
  skel_t *proplist, *prop;

  SVN_ERR (open_path (&parent_path, args->root, args->path, 0, trail));
  SVN_ERR (make_path_mutable (args->root, parent_path, args->path, trail));
  SVN_ERR (svn_fs__dag_get_proplist (&proplist, parent_path->node, trail));
  
  /* Delete the skel, either replacing or adding the given property.  */
  for (prop = proplist->children; prop; prop = prop->next->next)
    {
      skel_t *name = prop;
      skel_t *value = prop->next;

      /* We've found an existing entry for this property. 
         Replace the value.  */
      if (name->len == args->name->len
          && ! memcmp (name->data, args->name->data, name->len))
        {
          value->data = args->value->data;
          value->len = args->value->len;

          break;
        }
    }

  /* This property doesn't appear in the property list; add it to the
     beginning.  */
  if (! prop)
    {
      svn_fs__prepend (svn_fs__mem_atom (args->value->data,
                                         args->value->len,
                                         trail->pool),
                       proplist);
      svn_fs__prepend (svn_fs__mem_atom (args->name->data,
                                         args->name->len,
                                         trail->pool),
                       proplist);
    }

  SVN_ERR (svn_fs__dag_set_proplist (parent_path->node, proplist, trail));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_change_node_prop (svn_fs_root_t *root,
                         const char *path,
                         svn_string_t *name,
                         svn_string_t *value,
                         apr_pool_t *pool)
{
  struct change_node_prop_args args;

  args.root  = root;
  args.path  = path;
  args.name  = name;
  args.value = value;
  SVN_ERR (svn_fs__retry_txn (root->fs, txn_body_change_node_prop, &args,
                              pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_merge (const char **conflict_p,
              svn_fs_root_t *source_root,
              const char *source_path,
              svn_fs_root_t *target_root,
              const char *target_path,
              svn_fs_root_t *ancestor_root,
              const char *ancestor_path,
              apr_pool_t *pool)
{
  abort ();
}



/* Directories.  */


svn_error_t *
svn_fs_dir_entries (apr_hash_t **table_p,
                    svn_fs_root_t *root,
                    const char *path,
                    apr_pool_t *pool)
{
  abort ();
}


svn_error_t *
svn_fs_make_dir (svn_fs_root_t *root,
                 const char *path,
                 apr_pool_t *pool)
{
  abort ();
}
                              

struct delete_args
{
  svn_fs_root_t *root;
  const char *path;
};


static svn_error_t *
txn_body_delete (void *baton,
                 trail_t *trail)
{
  struct delete_args *args = baton;
  svn_fs_root_t *root = args->root;
  const char *path = args->path;
  parent_path_t *parent_path;

  SVN_ERR (open_path (&parent_path, root, path, 0, trail));

  /* We can't remove the root of the filesystem.  */
  if (! parent_path->parent)
    return svn_error_create (SVN_ERR_FS_ROOT_DIR, 0, NULL, trail->pool,
                             "the root directory cannot be deleted");

  /* Make the parent directory mutable.  */
  SVN_ERR (make_path_mutable (root, parent_path->parent, path, trail));

  /* We have a (semi-)bug here: we'll happily delete non-empty directories,
     if they're shared with the base revision.  */
  SVN_ERR (svn_fs__dag_delete (parent_path->parent->node,
                               parent_path->entry,
                               trail));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_delete (svn_fs_root_t *root,
               const char *path,
               apr_pool_t *pool)
{
  struct delete_args args;

  args.root = root;
  args.path = path;
  return svn_fs__retry_txn (root->fs, txn_body_delete, &args, pool);
}


svn_error_t *
svn_fs_delete_tree (svn_fs_root_t *root,
                    const char *path,
                    apr_pool_t *pool)
{
  abort ();
}


svn_error_t *
svn_fs_rename (svn_fs_root_t *root,
               const char *from,
               const char *to,
               apr_pool_t *pool)
{
  abort ();
}


svn_error_t *
svn_fs_copy (svn_fs_root_t *from_root,
             const char *from_path,
             svn_fs_root_t *to_root,
             const char *to_path,
             apr_pool_t *pool)
{
  abort ();
}



/* Files.  */


struct make_file_args
{
  svn_fs_root_t *root;
  const char *path;
};


static svn_error_t *
txn_body_make_file (void *baton,
                    trail_t *trail)
{
  struct make_file_args *args = baton;
  svn_fs_root_t *root = args->root;
  const char *path = args->path;
  parent_path_t *parent_path;
  dag_node_t *child;
  
  SVN_ERR (open_path (&parent_path, root, path, open_path_last_optional,
                      trail));

  /* If there's already a file by that name, complain.
     This also catches the case of trying to make a file named `/'.  */
  if (parent_path->node)
    return already_exists (root, path);

  /* Create the file.  */
  SVN_ERR (make_path_mutable (root, parent_path->parent, path, trail));
  SVN_ERR (svn_fs__dag_make_file (&child,
                                  parent_path->parent->node, 
                                  parent_path->entry,
                                  trail));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_make_file (svn_fs_root_t *root,
                  const char *path,
                  apr_pool_t *pool)
{
  struct make_file_args args;

  args.root = root;
  args.path = path;
  return svn_fs__retry_txn (root->fs, txn_body_make_file, &args, pool);
}



/* Creating transaction and revision root nodes.  */


struct txn_root_args
{
  svn_fs_root_t **root_p;
  svn_fs_txn_t *txn;
};


static svn_error_t *
txn_body_txn_root (void *baton,
                   trail_t *trail)
{
  struct txn_root_args *args = baton;
  svn_fs_root_t **root_p = args->root_p;
  svn_fs_txn_t *txn = args->txn;
  svn_fs_t *fs = svn_fs_txn_fs (txn);
  const char *svn_txn_id = svn_fs__txn_id (txn);
  svn_fs_id_t *root_id, *base_root_id;
  svn_fs_root_t *root;

  /* Verify that the transaction actually exists.  */
  SVN_ERR (svn_fs__get_txn (&root_id, &base_root_id, fs, svn_txn_id, trail));

  root = make_txn_root (fs, svn_txn_id, trail->pool);

  *root_p = root;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_txn_root (svn_fs_root_t **root_p,
                 svn_fs_txn_t *txn,
                 apr_pool_t *pool)
{
  svn_fs_root_t *root;
  struct txn_root_args args;

  args.root_p = &root;
  args.txn    = txn;
  SVN_ERR (svn_fs__retry_txn (svn_fs_txn_fs (txn), txn_body_txn_root,
                              &args, pool));

  *root_p = root;
  return SVN_NO_ERROR;
}


struct revision_root_args
{
  svn_fs_root_t **root_p;
  svn_fs_t *fs;
  svn_revnum_t rev;
};


static svn_error_t *
txn_body_revision_root (void *baton,
                        trail_t *trail)
{
  struct revision_root_args *args = baton;
  dag_node_t *root_dir;
  svn_fs_root_t *root;

  SVN_ERR (svn_fs__dag_revision_root (&root_dir, args->fs, args->rev, trail));
  root = make_revision_root (args->fs, args->rev, root_dir, trail->pool);

  *args->root_p = root;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_revision_root (svn_fs_root_t **root_p,
                      svn_fs_t *fs,
                      svn_revnum_t rev,
                      apr_pool_t *pool)
{
  struct revision_root_args args;
  svn_fs_root_t *root;

  args.root_p = &root;
  args.fs     = fs;
  args.rev    = rev;
  SVN_ERR (svn_fs__retry_txn (fs, txn_body_revision_root, &args, pool));

  *root_p = root;
  return SVN_NO_ERROR;
}




/* 
 * local variables:
 * eval: (load-file "../svn-dev.el")
 * end:
 */

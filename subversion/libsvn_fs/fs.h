/*
 * fs.h : interface to Subversion filesystem, private to libsvn_fs
 *
 * ================================================================
 * Copyright (c) 2000 Collab.Net.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * 3. The end-user documentation included with the redistribution, if
 * any, must include the following acknowlegement: "This product includes
 * software developed by Collab.Net (http://www.Collab.Net/)."
 * Alternately, this acknowlegement may appear in the software itself, if
 * and wherever such third-party acknowlegements normally appear.
 * 
 * 4. The hosted project names must not be used to endorse or promote
 * products derived from this software without prior written
 * permission. For written permission, please contact info@collab.net.
 * 
 * 5. Products derived from this software may not use the "Tigris" name
 * nor may "Tigris" appear in their names without prior written
 * permission of Collab.Net.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL COLLAB.NET OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by many
 * individuals on behalf of Collab.Net.
 */

/* ==================================================================== */



#ifndef SVN_LIBSVN_FS_FS_H
#define SVN_LIBSVN_FS_FS_H

#include "db.h"			/* Berkeley DB interface */
#include "apr_pools.h"
#include "svn_fs.h"

/* There are many different ways to implement the Subversion
   filesystem interface.  You could implement it directly using
   ordinary POSIX filesystem operations; you could build it using an
   SQL server as a back end; you could build it on RCS; and so on.

   This implementation of the Subversion filesystem interface is built
   on top of Berkeley DB (http://www.sleepycat.com).  Berkeley DB
   supports transactions and recoverability, making it well-suited for
   Subversion.

   In a Subversion filesystem, a `node' corresponds roughly to an
   `inode' in a Unix filesystem:
   - A node is either a file or a directory.
   - A node's contents change over time.
   - When you change a node's contents, it's still the same node; it's
     just been changed.  So a node's identity isn't bound to a specific
     set of contents.
   - If you rename a node, it's still the same node, just under a
     different name.  So a node's identity isn't bound to a particular
     filename.

   A `node version' refers to a node's contents at a specific point in
   time.  Changing a node's contents always creates a new version of
   that node.  Once created, a node version's contents never change.

   When we create a node, its initial contents are the initial version
   of the node.  As users make changes to the node over time, we
   create new versions of that same node.  When a user commits a
   change that deletes a file from the filesystem, we don't delete the
   node, or any version of it --- those stick around to allow us to
   recreate prior versions of the filesystem.  Instead, we just remove
   the reference to the node from the directory.

   Within the database, we refer to nodes and node versions using
   strings of numbers separated by periods that look a lot like RCS
   revision numbers.  

     node_id ::= number | node_version_id "." number
     node_version_id ::= node_id "." number

   So: 
   - "100" is a node id.
   - "100.10" is a node version id, referring to version 10 of node 100.
   - "100.10.3" is a node id, referring to the third branch based on
     version 10 of node 100.
   - "100.10.3.13" is a node version id, referring to version 13 of
     of the third branch from version 10 of node 100.
   And so on.

   A directory entry identifies the file or subdirectory it refers to
   using a node version number.  Changes far down in a filesystem
   hierarchy requires all their parents to be updated to hold the new
   node version ID.  This makes it easy to find changes in large
   trees.

   Note that the numbers specifying a particular version of a node is
   the number of the global filesystem version when that node version
   was created.  So 100.13 was created in filesystem version 13.  This
   means that 100.13.10.2 is meaningless --- the last number implies
   that it was created in filesystem version 2, but the root implies
   that it's a branch of the version of node 10 created in filesystem
   verison 13.  The version numbers in node ID's and node version ID's
   must increase from left to right.

   Identifying nodes and node versions this way makes it easy to
   discover whether and how two nodes are related.  Simply by looking
   at the node version id's, we can tell that the difference from
   100.10.3.11 to 100.12 is the difference from 100.10.3.11 to 100.10,
   plus the difference from 100.10 to 100.12.  */


/* A Subversion filesystem.  */
struct svn_fs {

  /* A pool for allocations for this filesystem.  */
  apr_pool_t *pool;

  /* The filename of the Berkeley DB environment, for use in error
     messages.  */
  char *env_path;

  /* A Berkeley DB environment for all the filesystem's databases.
     This establishes the scope of the filesystem's transactions.  */
  DB_ENV *env;

  /* A btree mapping version numbers onto root directories and
     property lists.  See versions.c for the details.  */
  DB *versions;

  /* A btree mapping node id's onto full texts.  If an entry is
     missing here, there must exist a full text for some later
     revision, and we will need to reconstruct the version we want by
     composing deltas.  */
  DB *full_texts;

  /* A btree database mapping node id's onto text deltas.  */
  DB *deltas;

  /* A callback function for printing warning messages, and a baton to
     pass through to it.  */
  svn_fs_warning_callback_t *warning;
  void *warning_baton;

  /* A kludge for handling errors noticed by APR pool cleanup functions.

     The APR pool cleanup functions can only return an apr_status_t
     value, not a full svn_error_t value.  This makes it difficult to
     propagate errors detected by fs_cleanup to someone who can handle
     them.

     If FS->cleanup_error is non-zero, it points to a location where
     fs_cleanup should store a pointer to an svn_error_t object, if it
     generates one.  Normally, it's zero, but if the cleanup is
     invoked by code prepared to deal with an svn_error_t object in
     some helpful way, it can create its own svn_error_t *, set it to
     zero, set cleanup_error to point to it, free the pool (thus
     invoking the cleanup), and then check its svn_error_t to see if
     anything went wrong.

     Of course, if multiple errors occur, this will only report one of
     them, but it's better than nothing.  In the case of a cascade,
     the first error message is probably the most helpful, so
     fs_cleanup won't overwrite a pointer to an existing svn_error_t
     if it finds one.  */
  svn_error_t **cleanup_error;
};



/* Typed allocation macros.  These don't really belong here.  */

/* Allocate space for a value of type T from the pool P, and return a
   typed pointer.  */
#define NEW(P, T) ((T *) apr_palloc ((P), sizeof (T)))

/* Allocate space for an array of N values of type T from pool P, and
   return a typed pointer.  */
#define NEWARRAY(P, T, N) ((T *) apr_palloc ((P), sizeof (T) * (N)))

#endif /* SVN_LIBSVN_FS_FS_H */

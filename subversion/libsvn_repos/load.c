/* load.c --- parsing a 'dumpfile'-formatted stream.
 *
 * ====================================================================
 * Copyright (c) 2000-2002 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 */


#include "svn_pools.h"
#include "svn_error.h"
#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_string.h"
#include "svn_hash.h"
#include "svn_path.h"


/*----------------------------------------------------------------------*/

/** The parser and related helper funcs **/


/* Allocate a new hash *HEADERS in POOL, and read a series of
   RFC822-style headers from STREAM.  Duplicate each header's name and
   value into POOL and store in hash as a const char * ==> const char *.

   The headers are assumed to be terminated by a blank line ("\n\n").
 */
static svn_error_t *
read_header_block (svn_stream_t *stream,
                   apr_hash_t **headers,
                   apr_pool_t *pool)
{
  char c, last_c = 0;
  apr_size_t numbytes, old_i = 0, i = 0;
  const char *name, *value;
  svn_stringbuf_t *header_str = svn_stringbuf_create ("", pool);
  *headers = apr_hash_make (pool);  

  /* Suck the whole block of headers into a stringbuf.  We read only
     one character at a time, because we're carefully looking for a
     blank line to signal the end of the header block. */
  while (1)
    {
      numbytes = 1;
      SVN_ERR (svn_stream_read (stream, &c, &numbytes));

      if ((c == '\n') && (last_c == '\n'))
        break;

      /* Note: we're using a stringbuf here, even though we know the
         header block won't ever contain anything but simple ASCII.
         That's because the appendbytes() function is more efficient
         than trying to grow a regular C string one-byte-at-a-time:
         appendbytes() keeps doubling the stringbuf's storage space. */
      svn_stringbuf_appendbytes (header_str, &c, 1);
      last_c = c;      
    }

  /* Parse the stringbuf into a hash.  Walk over the stringbuf's bytes
     until we see a NULL somewhere, which signifies that this
     particular stringbuf's data has run out. */
  while (1)
    {
      /* Find the next colon.  Bam, we have the header's name. */
      while (header_str->data[i] != ':')
        {
          if (header_str->data[i] == '\0')
            goto malformed_error;
          
          i++;
        }
      name = apr_pstrmemdup (pool, header_str->data + old_i, (i - old_i));
      
      /* Find the next newline.  Bam, we have the header's value. */
      i++;
      old_i = i;
      while (header_str->data[i] != '\n')
        {
          if (header_str->data[i] == '\0')
            goto malformed_error;
          
          i++;
        }
      value = apr_pstrmemdup (pool, header_str->data + old_i, (i - old_i));
      
      apr_hash_set (*headers, name, APR_HASH_KEY_STRING, value);

      /* 'i' now points to a newline.  If the next character is a NULL,
         then we're done.  This is the only legitimate place for a
         NULL.  If not NULL, there must be another header to read, so
         we loop.  */
      i++;
      if (header_str->data[i] == '\0')
        break;
    }

  return SVN_NO_ERROR;

 malformed_error:
  return svn_error_create (SVN_ERR_MALFORMED_STREAM_DATA, 0, NULL, pool,
                           "Found malformed header block in dumpfile stream.");
}


/* The Main Parser */
svn_error_t *
svn_repos_parse_dumpstream (svn_stream_t *stream,
                            const svn_repos_parser_fns_t *parse_fns,
                            void *parse_baton,
                            apr_pool_t *pool)
{
  apr_hash_t *headers;

  /* ### verify that we support the dumpfile format version number. */

  /* ### outline:

  while (stream):
  {
    read group of headers into a hash

    if hash contains revision-number,
       possibly close_revision() on the old revision baton
       new_revision_record()
    else if hash contains node-path,
       new_node_record()
     
    if hash contains content-length,
       read n bytes of content
       parse content:
            call set_*_property() if needed
            call set_fulltext() if a node

    if in a node,
        close_node()
   }
   if in a revision,
     close_revision()
 */

  /* shut up compiler warnings about unused functions. */
  SVN_ERR (read_header_block (stream, &headers, pool));
  

  return SVN_NO_ERROR;
}


/*----------------------------------------------------------------------*/

/** vtable for doing commits to a fs **/

/* ### right now, this vtable will do nothing but stupid printf's */

struct parse_baton
{
  svn_fs_t *fs;
  
};

struct revision_baton
{
  svn_revnum_t rev;

  svn_fs_txn_t *txn;

  struct parse_baton *pb;
  apr_pool_t *pool;
};

struct node_baton
{
  const char *path;
  enum svn_node_kind kind;
  enum svn_node_action action;

  struct revision_baton *rb;
  apr_pool_t *pool;
};


static struct node_baton *
make_node_baton (apr_hash_t *headers,
                 struct revision_baton *rb,
                 apr_pool_t *pool)
{
  struct node_baton *nb = apr_pcalloc (pool, sizeof(*nb));

  nb->rb = rb;
  nb->pool = pool;

  /* ### parse the headers into a node_baton struct */

  return NULL;
}

static struct revision_baton *
make_revision_baton (apr_hash_t *headers,
                     struct parse_baton *pb,
                     apr_pool_t *pool)
{
  struct revision_baton *rb = apr_pcalloc (pool, sizeof(*rb));
  
  rb->pb = pb;
  rb->pool = pool;

  /* ### parse the headers into a revision_baton struct */

  return NULL;
}


static svn_error_t *
new_revision_record (void **revision_baton,
                     apr_hash_t *headers,
                     void *parse_baton,
                     apr_pool_t *pool)
{
  struct parse_baton *pb = parse_baton;
  *revision_baton = make_revision_baton (headers, pb, pool);

  printf ("Got a new revision record.\n");

  /* ### Create a new *revision_baton->txn, using pb->fs. */

  return SVN_NO_ERROR;
}


static svn_error_t *
new_node_record (void **node_baton,
                 apr_hash_t *headers,
                 void *revision_baton,
                 apr_pool_t *pool)
{
  struct revision_baton *rb = revision_baton;
  printf ("Got a new node record.\n");

  *node_baton = make_node_baton (headers, rb, pool);
  return SVN_NO_ERROR;
}


static svn_error_t *
set_revision_property (void *baton,
                       const char *name,
                       const svn_string_t *value)
{
  printf("Got a revision prop.\n");

  return SVN_NO_ERROR;
}

static svn_error_t *
set_node_property (void *baton,
                   const char *name,
                   const svn_string_t *value)
{
  printf("Got a node prop.\n");

  return SVN_NO_ERROR;
}


static svn_error_t *
set_fulltext (svn_stream_t **stream,
              void *node_baton)
{
  printf ("Not interested in fulltext.\n");
  *stream = NULL;
  return SVN_NO_ERROR;
}


static svn_error_t *
close_node (void *baton)
{
  printf ("End of node\n");
  return SVN_NO_ERROR;
}


static svn_error_t *
close_revision (void *baton)
{
  /* ### someday commit a txn here. */

  printf ("End of revision\n");
  return SVN_NO_ERROR;
}



static svn_error_t *
get_parser (const svn_repos_parser_fns_t **parser_callbacks,
            void **parse_baton,
            svn_repos_t *repos,
            apr_pool_t *pool)
{
  svn_repos_parser_fns_t *parser = apr_pcalloc (pool, sizeof(*parser));
  struct parse_baton *pb = apr_pcalloc (pool, sizeof(*pb));

  parser->new_revision_record = new_revision_record;
  parser->new_node_record = new_node_record;
  parser->set_revision_property = set_revision_property;
  parser->set_node_property = set_node_property;
  parser->set_fulltext = set_fulltext;
  parser->close_node = close_node;
  parser->close_revision = close_revision;

  pb->fs = svn_repos_fs (repos);

  *parser_callbacks = parser;
  *parse_baton = pb;
  return SVN_NO_ERROR;
}


/*----------------------------------------------------------------------*/

/** The main loader routine. **/


svn_error_t *
svn_repos_load_fs (svn_repos_t *repos,
                   svn_stream_t *stream,
                   apr_pool_t *pool)
{
  const svn_repos_parser_fns_t *parser;
  void *parse_baton;
  
  /* This is really simple. */  

  SVN_ERR (get_parser (&parser, &parse_baton, repos, pool));

  SVN_ERR (svn_repos_parse_dumpstream (stream, parser, parse_baton, pool));

  return SVN_NO_ERROR;
}



/* 
 * local variables:
 * eval: (load-file "../../tools/dev/svn-dev.el")
 * end:
 */

/**
 * @copyright
 * ====================================================================
 * Copyright (c) 2000-2007 CollabNet.  All rights reserved.
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
 * @endcopyright
 *
 * @file svn_client.h
 * @brief Public interface for libsvn_client.
 */



/*** Includes ***/

/* 
 * Requires:  The working copy library and repository access library.
 * Provides:  Broad wrappers around working copy library functionality.
 * Used By:   Client programs.
 */

#ifndef SVN_CLIENT_H
#define SVN_CLIENT_H

#include <apr_tables.h>

#include "svn_types.h"
#include "svn_wc.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_opt.h"
#include "svn_version.h"
#include "svn_ra.h"
#include "svn_diff.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* ### TODO:  Multiple Targets

    - Up for debate:  an update on multiple targets is *not* atomic.
    Right now, svn_client_update only takes one path.  What's
    debatable is whether this should ever change.  On the one hand,
    it's kind of losing to have the client application loop over
    targets and call svn_client_update() on each one;  each call to
    update initializes a whole new repository session (network
    overhead, etc.)  On the other hand, it's a very simple
    implementation, and allows for the possibility that different
    targets may come from different repositories.  */


/**
 * Get libsvn_client version information.
 *
 * @since New in 1.1.
 */
const svn_version_t *svn_client_version(void);

/** Client supporting functions
 *
 * @defgroup clnt_support Client supporting subsystem
 *
 * @{
 */


/*** Authentication stuff ***/

/**  The new authentication system allows the RA layer to "pull"
 *   information as needed from libsvn_client.
 *
 *   @deprecated Replaced by the svn_auth_* functions.
 *   @see auth_fns
 *
 *   @defgroup auth_fns_depr (deprecated) AuthZ client subsystem
 *
 *   @{
 */

/** Create and return @a *provider, an authentication provider of type
 * svn_auth_cred_simple_t that gets information by prompting the user
 * with @a prompt_func and @a prompt_baton.  Allocate @a *provider in
 * @a pool.
 *
 * If both @c SVN_AUTH_PARAM_DEFAULT_USERNAME and
 * @c SVN_AUTH_PARAM_DEFAULT_PASSWORD are defined as runtime
 * parameters in the @c auth_baton, then @a *provider will return the
 * default arguments when svn_auth_first_credentials() is called.  If
 * svn_auth_first_credentials() fails, then @a *provider will
 * re-prompt @a retry_limit times (via svn_auth_next_credentials()).
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_simple_prompt_provider
  (svn_auth_provider_object_t **provider,
   svn_auth_simple_prompt_func_t prompt_func,
   void *prompt_baton,
   int retry_limit,
   apr_pool_t *pool);


/** Create and return @a *provider, an authentication provider of type @c
 * svn_auth_cred_username_t that gets information by prompting the
 * user with @a prompt_func and @a prompt_baton.  Allocate @a *provider
 * in @a pool.
 *
 * If @c SVN_AUTH_PARAM_DEFAULT_USERNAME is defined as a runtime
 * parameter in the @c auth_baton, then @a *provider will return the
 * default argument when svn_auth_first_credentials() is called.  If
 * svn_auth_first_credentials() fails, then @a *provider will
 * re-prompt @a retry_limit times (via svn_auth_next_credentials()).
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_username_prompt_provider
  (svn_auth_provider_object_t **provider,
   svn_auth_username_prompt_func_t prompt_func,
   void *prompt_baton,
   int retry_limit,
   apr_pool_t *pool);


/** Create and return @a *provider, an authentication provider of type @c
 * svn_auth_cred_simple_t that gets/sets information from the user's
 * ~/.subversion configuration directory.  Allocate @a *provider in
 * @a pool.
 *  
 * If a default username or password is available, @a *provider will
 * honor them as well, and return them when
 * svn_auth_first_credentials() is called.  (see @c
 * SVN_AUTH_PARAM_DEFAULT_USERNAME and @c
 * SVN_AUTH_PARAM_DEFAULT_PASSWORD). 
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_simple_provider(svn_auth_provider_object_t **provider,
                                    apr_pool_t *pool);


#if (defined(WIN32) && !defined(__MINGW32__)) || defined(DOXYGEN)
/**
 * Create and return @a *provider, an authentication provider of type @c
 * svn_auth_cred_simple_t that gets/sets information from the user's
 * ~/.subversion configuration directory.  Allocate @a *provider in
 * @a pool.
 *
 * This is like svn_client_get_simple_provider(), except that, when
 * running on Window 2000 or newer (or any other Windows version that
 * includes the CryptoAPI), the provider encrypts the password before
 * storing it to disk. On earlier versions of Windows, the provider
 * does nothing.
 *
 * @since New in 1.2.
 * @note This function is only available on Windows.
 *
 * @note An administrative password reset may invalidate the account's
 * secret key. This function will detect that situation and behave as
 * if the password were not cached at all.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_windows_simple_provider
  (svn_auth_provider_object_t **provider,
   apr_pool_t *pool);
#endif /* WIN32 || DOXYGEN */

/** Create and return @a *provider, an authentication provider of type @c
 * svn_auth_cred_username_t that gets/sets information from a user's
 * ~/.subversion configuration directory.  Allocate @a *provider in
 * @a pool.
 *
 * If a default username is available, @a *provider will honor it,
 * and return it when svn_auth_first_credentials() is called.  (see
 * @c SVN_AUTH_PARAM_DEFAULT_USERNAME). 
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_username_provider(svn_auth_provider_object_t **provider,
                                      apr_pool_t *pool);


/** Create and return @a *provider, an authentication provider of type @c
 * svn_auth_cred_ssl_server_trust_t, allocated in @a pool.
 *
 * @a *provider retrieves its credentials from the configuration
 * mechanism.  The returned credential is used to override SSL
 * security on an error.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_ssl_server_trust_file_provider
  (svn_auth_provider_object_t **provider,
   apr_pool_t *pool);


/** Create and return @a *provider, an authentication provider of type @c
 * svn_auth_cred_ssl_client_cert_t, allocated in @a pool.
 *
 * @a *provider retrieves its credentials from the configuration
 * mechanism.  The returned credential is used to load the appropriate
 * client certificate for authentication when requested by a server.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_ssl_client_cert_file_provider
  (svn_auth_provider_object_t **provider,
   apr_pool_t *pool);


/** Create and return @a *provider, an authentication provider of type @c
 * svn_auth_cred_ssl_client_cert_pw_t, allocated in @a pool.
 *
 * @a *provider retrieves its credentials from the configuration
 * mechanism.  The returned credential is used when a loaded client
 * certificate is protected by a passphrase.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_ssl_client_cert_pw_file_provider
  (svn_auth_provider_object_t **provider,
   apr_pool_t *pool);


/** Create and return @a *provider, an authentication provider of type @c
 * svn_auth_cred_ssl_server_trust_t, allocated in @a pool.  
 *
 * @a *provider retrieves its credentials by using the @a prompt_func
 * and @a prompt_baton.  The returned credential is used to override
 * SSL security on an error.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_ssl_server_trust_prompt_provider
  (svn_auth_provider_object_t **provider,
   svn_auth_ssl_server_trust_prompt_func_t prompt_func,
   void *prompt_baton,
   apr_pool_t *pool);


/** Create and return @a *provider, an authentication provider of type @c
 * svn_auth_cred_ssl_client_cert_t, allocated in @a pool.
 *
 * @a *provider retrieves its credentials by using the @a prompt_func
 * and @a prompt_baton.  The returned credential is used to load the
 * appropriate client certificate for authentication when requested by
 * a server.  The prompt will be retried @a retry_limit times.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_ssl_client_cert_prompt_provider
  (svn_auth_provider_object_t **provider,
   svn_auth_ssl_client_cert_prompt_func_t prompt_func,
   void *prompt_baton,
   int retry_limit,
   apr_pool_t *pool);


/** Create and return @a *provider, an authentication provider of type @c
 * svn_auth_cred_ssl_client_cert_pw_t, allocated in @a pool.
 *
 * @a *provider retrieves its credentials by using the @a prompt_func
 * and @a prompt_baton.  The returned credential is used when a loaded
 * client certificate is protected by a passphrase.  The prompt will
 * be retried @a retry_limit times.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
void svn_client_get_ssl_client_cert_pw_prompt_provider
  (svn_auth_provider_object_t **provider,
   svn_auth_ssl_client_cert_pw_prompt_func_t prompt_func,
   void *prompt_baton,
   int retry_limit,
   apr_pool_t *pool);

/** @} */

/**
 * Commit operations
 *
 * @defgroup clnt_commit Client commit subsystem
 *
 * @{
 */

/** This is a structure which stores a filename and a hash of property
 * names and values.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
typedef struct svn_client_proplist_item_t
{
  /** The name of the node on which these properties are set. */
  svn_stringbuf_t *node_name;  

  /** A hash of (const char *) property names, and (svn_string_t *) property
   * values. */
  apr_hash_t *prop_hash;

} svn_client_proplist_item_t;

/**
 * The callback invoked by svn_client_proplist3().  Each invocation
 * describes the property specified by @a item.  Use @a pool for all
 * temporary allocation.
 */
typedef svn_error_t *(*svn_proplist_receiver_t)
  (void *baton,
   svn_stringbuf_t *path,
   apr_hash_t *prop_hash,
   apr_pool_t *pool);

/** 
 * Return a duplicate of @a item, allocated in @a pool. No part of the new
 * structure will be shared with @a item.
 *
 * @since New in 1.3.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
svn_client_proplist_item_t *
svn_client_proplist_item_dup(const svn_client_proplist_item_t *item,
                             apr_pool_t *pool);

/** Information about commits passed back to client from this module.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 */
typedef struct svn_client_commit_info_t
{
  /** just-committed revision. */
  svn_revnum_t revision;

  /** server-side date of the commit. */
  const char *date;

  /** author of the commit. */
  const char *author;

} svn_client_commit_info_t;


/**
 * @name Commit state flags
 * @brief State flags for use with the @c svn_client_commit_item3_t structure
 * (see the note about the namespace for that structure, which also
 * applies to these flags).
 * @{
 */
#define SVN_CLIENT_COMMIT_ITEM_ADD         0x01
#define SVN_CLIENT_COMMIT_ITEM_DELETE      0x02
#define SVN_CLIENT_COMMIT_ITEM_TEXT_MODS   0x04
#define SVN_CLIENT_COMMIT_ITEM_PROP_MODS   0x08
#define SVN_CLIENT_COMMIT_ITEM_IS_COPY     0x10
/** @since New in 1.2. */
#define SVN_CLIENT_COMMIT_ITEM_LOCK_TOKEN  0x20
/** @} */

/** The commit candidate structure.  In order to avoid backwards
 * compatibility problems clients should use
 * svn_client_commit_item_create() to allocate and intialize this
 * structure instead of doing so themselves.
 *
 * @since New in 1.5.
 */
typedef struct svn_client_commit_item3_t
{
  /** absolute working-copy path of item */
  const char *path;

  /** node kind (dir, file) */
  svn_node_kind_t kind;

  /** commit URL for this item */
  const char *url;

  /** revision of textbase */
  svn_revnum_t revision;

  /** copyfrom-url or NULL if not a copied item */
  const char *copyfrom_url;
  
  /** copyfrom-rev, valid when copyfrom_url != NULL */
  svn_revnum_t copyfrom_rev;
  
  /** state flags */
  apr_byte_t state_flags;

  /** An array of @c svn_prop_t *'s, which are incoming changes from
   * the repository to WC properties.  These changes are applied
   * post-commit.
   *
   * When adding to this array, allocate the @c svn_prop_t and its
   * contents in @c incoming_prop_changes->pool, so that it has the
   * same lifetime as this data structure.
   *
   * See http://subversion.tigris.org/issues/show_bug.cgi?id=806 for a
   * description of what would happen if the post-commit process
   * didn't group these changes together with all other changes to the
   * item.
   */
  apr_array_header_t *incoming_prop_changes;

  /** An array of @c svn_prop_t *'s, which are outgoing changes to
   * make to properties in the repository.  These extra property
   * changes are declared pre-commit, and applied to the repository as
   * part of a commit.
   *
   * When adding to this array, allocate the @c svn_prop_t and its
   * contents in @c outgoing_prop_changes->pool, so that it has the
   * same lifetime as this data structure.
   */
  apr_array_header_t *outgoing_prop_changes;
} svn_client_commit_item3_t;

/** The commit candidate structure.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
typedef struct svn_client_commit_item2_t
{
  /** absolute working-copy path of item */
  const char *path;

  /** node kind (dir, file) */
  svn_node_kind_t kind;

  /** commit URL for this item */
  const char *url;

  /** revision of textbase */
  svn_revnum_t revision;

  /** copyfrom-url or NULL if not a copied item */
  const char *copyfrom_url;
  
  /** copyfrom-rev, valid when copyfrom_url != NULL */
  svn_revnum_t copyfrom_rev;
  
  /** state flags */
  apr_byte_t state_flags;

  /** Analogous to the @c svn_client_commit_item3_t.incoming_prop_changes
   * field.
   */
  apr_array_header_t *wcprop_changes;
} svn_client_commit_item2_t;

/** The commit candidate structure.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 */
typedef struct svn_client_commit_item_t
{
  /** absolute working-copy path of item */
  const char *path;

  /** node kind (dir, file) */
  svn_node_kind_t kind;

  /** commit URL for this item */
  const char *url;

  /** revision (copyfrom-rev if _IS_COPY) */
  svn_revnum_t revision;

  /** copyfrom-url */
  const char *copyfrom_url;

  /** state flags */
  apr_byte_t state_flags;

  /** Analogous to the @c svn_client_commit_item3_t.incoming_prop_changes
   * field.
   */
  apr_array_header_t *wcprop_changes;

} svn_client_commit_item_t;

/** Initialize a commit item.
 * Set @a *item to a commit item object, allocated in @a pool.
 *
 * In order to avoid backwards compatibility problems, this function
 * is used to intialize and allocate the @c svn_client_commit_item3_t
 * structure rather than doing so explicitly, as the size of this
 * structure may change in the future.
 * 
 * The current implementation never returns error, but callers should
 * still check for error, for compatibility with future versions.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_commit_item_create(const svn_client_commit_item3_t **item,
                              apr_pool_t *pool);

/** 
 * Return a duplicate of @a item, allocated in @a pool. No part of the new
 * structure will be shared with @a item.
 *
 * @since New in 1.5.
 */
svn_client_commit_item3_t *
svn_client_commit_item3_dup(const svn_client_commit_item3_t *item,
                            apr_pool_t *pool);

/** 
 * Return a duplicate of @a item, allocated in @a pool. No part of the new
 * structure will be shared with @a item.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
svn_client_commit_item2_t *
svn_client_commit_item2_dup(const svn_client_commit_item2_t *item,
                            apr_pool_t *pool);

/** Callback type used by commit-y operations to get a commit log message
 * from the caller.
 *  
 * Set @a *log_msg to the log message for the commit, allocated in @a 
 * pool, or @c NULL if wish to abort the commit process.  Set @a *tmp_file 
 * to the path of any temporary file which might be holding that log 
 * message, or @c NULL if no such file exists (though, if @a *log_msg is 
 * @c NULL, this value is undefined).  The log message MUST be a UTF8 
 * string with LF line separators.
 *
 * @a commit_items is a read-only array of @c svn_client_commit_item3_t
 * structures, which may be fully or only partially filled-in,
 * depending on the type of commit operation.
 *
 * @a baton is provided along with the callback for use by the handler.
 *
 * All allocations should be performed in @a pool.
 *
 * @since New in 1.5.
 */
typedef svn_error_t *(*svn_client_get_commit_log3_t)
  (const char **log_msg,
   const char **tmp_file,
   const apr_array_header_t *commit_items,
   void *baton,
   apr_pool_t *pool);

/** Callback type used by commit-y operations to get a commit log message
 * from the caller.
 *  
 * Set @a *log_msg to the log message for the commit, allocated in @a 
 * pool, or @c NULL if wish to abort the commit process.  Set @a *tmp_file 
 * to the path of any temporary file which might be holding that log 
 * message, or @c NULL if no such file exists (though, if @a *log_msg is 
 * @c NULL, this value is undefined).  The log message MUST be a UTF8 
 * string with LF line separators.
 *
 * @a commit_items is a read-only array of @c svn_client_commit_item2_t
 * structures, which may be fully or only partially filled-in,
 * depending on the type of commit operation.
 *
 * @a baton is provided along with the callback for use by the handler.
 *
 * All allocations should be performed in @a pool.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
typedef svn_error_t *(*svn_client_get_commit_log2_t)
  (const char **log_msg,
   const char **tmp_file,
   const apr_array_header_t *commit_items,
   void *baton,
   apr_pool_t *pool);

/** Callback type used by commit-y operations to get a commit log message
 * from the caller.
 *
 * Set @a *log_msg to the log message for the commit, allocated in @a
 * pool, or @c NULL if wish to abort the commit process.  Set @a *tmp_file
 * to the path of any temporary file which might be holding that log
 * message, or @c NULL if no such file exists (though, if @a *log_msg is
 * @c NULL, this value is undefined).  The log message MUST be a UTF8
 * string with LF line separators.
 *
 * @a commit_items is a read-only array of @c svn_client_commit_item_t
 * structures, which may be fully or only partially filled-in,
 * depending on the type of commit operation.
 *
 * @a baton is provided along with the callback for use by the handler.
 *
 * All allocations should be performed in @a pool.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 */
typedef svn_error_t *(*svn_client_get_commit_log_t)
  (const char **log_msg,
   const char **tmp_file,
   apr_array_header_t *commit_items,
   void *baton,
   apr_pool_t *pool);

/** @} */

/**
 * Client blame
 *
 * @defgroup clnt_blame Client blame functionality
 *
 * @{
 */

/** Callback type used by svn_client_blame() to notify the caller
 * that line @a line_no of the blamed file was last changed in
 * @a revision by @a author on @a date, and that the contents were
 * @a line.
 *  
 * All allocations should be performed in @a pool.
 *
 * @note If there is no blame information for this line, @a revision will be
 * invalid and @a author and @a date will be NULL.
 *
 * @note New in 1.4 is that the line is defined to contain only the line
 * content (and no [partial] EOLs; which was undefined in older versions).
 * Using this callback with svn_client_blame() or svn_client_blame2()
 * will still give you the old behaviour.
 */
typedef svn_error_t *(*svn_client_blame_receiver_t)
  (void *baton,
   apr_int64_t line_no,
   svn_revnum_t revision,
   const char *author,
   const char *date,
   const char *line,
   apr_pool_t *pool);


/** @} */

/**
 * Client diff
 *
 * @defgroup clnt_diff Client diff functionality
 *
 * @{
 */
/** The difference type in an svn_diff_summarize_t structure.
 *
 * @since New in 1.4.
 */
typedef enum svn_client_diff_summarize_kind_t
{
  /** An item with no text modifications */
  svn_client_diff_summarize_kind_normal,

  /** An added item */
  svn_client_diff_summarize_kind_added,

  /** An item with text modifications */
  svn_client_diff_summarize_kind_modified,

  /** A deleted item */
  svn_client_diff_summarize_kind_deleted
} svn_client_diff_summarize_kind_t;


/** A struct that describes the diff of an item. Passed to
 * @c svn_diff_summarize_func_t.
 *
 * @note Fields may be added to the end of this structure in future
 * versions.  Therefore, users shouldn't allocate structures of this
 * type, to preserve binary compatibility.
 *
 * @since New in 1.4.
 */
typedef struct svn_client_diff_summarize_t
{
  /** Path relative to the target. */
  const char *path;

  /** Change kind */
  svn_client_diff_summarize_kind_t summarize_kind;

  /** Properties changed? */
  svn_boolean_t prop_changed;

  /** File or dir */
  svn_node_kind_t node_kind;
} svn_client_diff_summarize_t;  

/**
 * Return a duplicate of @a diff, allocated in @a pool. No part of the new
 * structure will be shared with @a diff.
 *
 * @since New in 1.4.
 */
svn_client_diff_summarize_t *
svn_client_diff_summarize_dup(const svn_client_diff_summarize_t *diff,
                              apr_pool_t *pool);


/** A callback used in svn_client_diff_summarize2() and
 * svn_client_diff_summarize_peg2() for reporting a @a diff summary.
 *
 * All allocations should be performed in @a pool.
 *
 * @a baton is a closure object; it should be provided by the implementation,
 * and passed by the caller.
 *
 * @since New in 1.4.
 */
typedef svn_error_t *(*svn_client_diff_summarize_func_t)
  (const svn_client_diff_summarize_t *diff,
   void *baton,
   apr_pool_t *pool);


/** A callback used in svn_client_merge3() for resolving merge
 * conflicts to content or properties during the application of a tree
 * delta.
 *
 * All allocations should be performed in @a pool.
 *
 * @a baton is a closure object; it should be provided by the implementation,
 * and passed by the caller.
 *
 * @since New in 1.5.
 */
typedef svn_error_t *(*svn_client_conflict_resolver_func_t)
  (const char *path,
   void *baton,
   apr_pool_t *pool);
 

/** @} */

/**
 * Client context
 *
 * @defgroup clnt_ctx Client context management
 *
 * @{
 */

/** A client context structure, which holds client specific callbacks, 
 * batons, serves as a cache for configuration options, and other various 
 * and sundry things.  In order to avoid backwards compatibility problems 
 * clients should use svn_client_create_context() to allocate and 
 * intialize this structure instead of doing so themselves.
 */
typedef struct svn_client_ctx_t
{
  /** main authentication baton. */
  svn_auth_baton_t *auth_baton;

  /** notification callback function.
   * This will be called by notify_func2() by default.
   * @deprecated Provided for backward compatibility with the 1.1 API. */
  svn_wc_notify_func_t notify_func;

  /** notification callback baton for notify_func()
   * @deprecated Provided for backward compatibility with the 1.1 API. */
  void *notify_baton;

  /** Log message callback function.  NULL means that Subversion
    * should try not attempt to fetch a log message.
    * @deprecated Provided for backward compatibility with the 1.2 API. */
  svn_client_get_commit_log_t log_msg_func;

  /** log message callback baton
    * @deprecated Provided for backward compatibility with the 1.2 API. */
  void *log_msg_baton;

  /** a hash mapping of <tt>const char *</tt> configuration file names to
   * @c svn_config_t *'s. For example, the '~/.subversion/config' file's
   * contents should have the key "config".  May be left unset (or set to
   * NULL) to use the built-in default settings and not use any configuration.
   */
  apr_hash_t *config;

  /** a callback to be used to see if the client wishes to cancel the running 
   * operation. */
  svn_cancel_func_t cancel_func;

  /** a baton to pass to the cancellation callback. */
  void *cancel_baton;

  /** notification function, defaulting to a function that forwards
   * to notify_func().
   * @since New in 1.2. */
  svn_wc_notify_func2_t notify_func2;

  /** notification baton for notify_func2().
   * @since New in 1.2. */
  void *notify_baton2;

  /** Log message callback function. NULL means that Subversion
   *   should try log_msg_func.
   * @since New in 1.3. */
  svn_client_get_commit_log2_t log_msg_func2;

  /** callback baton for log_msg_func2
   * @since New in 1.3. */
  void *log_msg_baton2;

  /** Log message callback function. NULL means that Subversion
   *   should try @c log_msg_func2, then @c log_msg_func.
   * @since New in 1.5. */
  svn_client_get_commit_log3_t log_msg_func3;

  /** The callback baton for @c log_msg_func3.
   * @since New in 1.5. */
  void *log_msg_baton3;

  /** Notification callback for network progress information.
   * May be NULL if not used.
   * @since New in 1.3. */
  svn_ra_progress_notify_func_t progress_func;

  /** Callback baton for progress_func.
   * @since New in 1.3. */
  void *progress_baton;

  /** MIME types map.
   * @since New in 1.5. */
  apr_hash_t *mimetypes_map;
  
  /** Table holding the extra revision properties to be set.  This table
   * cannot contain any standard Subversion properties.
   * @since New in 1.5. */
  apr_hash_t *revprop_table;

  /* @since New in 1.5. */
  svn_client_conflict_resolver_func_t conflict_resolver_func;

} svn_client_ctx_t;

/** @} end group: Client context management */

/**
 * @name Authentication information file names
 *
 * Names of files that contain authentication information.
 *
 * These filenames are decided by libsvn_client, since this library
 * implements all the auth-protocols;  libsvn_wc does nothing but
 * blindly store and retrieve these files from protected areas.
 *
 * @defgroup clnt_auth_filenames Client authentication file names
 * @{
 */
#define SVN_CLIENT_AUTH_USERNAME            "username"
#define SVN_CLIENT_AUTH_PASSWORD            "password"
/** @} group end: Authentication information file names */

/** @} */

/**
 * Client context
 *
 * @defgroup clnt_ctx Client context management
 *
 * @{
 */

/** Initialize a client context.
 * Set @a *ctx to a client context object, allocated in @a pool, that
 * represents a particular instance of an svn client.
 *
 * In order to avoid backwards compatibility problems, clients must 
 * use this function to intialize and allocate the 
 * @c svn_client_ctx_t structure rather than doing so themselves, as 
 * the size of this structure may change in the future. 
 * 
 * The current implementation never returns error, but callers should
 * still check for error, for compatibility with future versions.
 */ 
svn_error_t *
svn_client_create_context(svn_client_ctx_t **ctx,
                          apr_pool_t *pool);

/** @} end group: Client context */

/**
 * Client working copy management functions
 *
 * @defgroup clnt_wc Client working copy management
 *
 * @{
 */

/**
 * @defgroup clnt_wc_checkout Checkout
 *
 * @{
 */


/**
 * Checkout a working copy of @a URL at @a revision, looked up at @a
 * peg_revision, using @a path as the root directory of the newly
 * checked out working copy, and authenticating with the
 * authentication baton cached in @a ctx.  If @a result_rev is not @c
 * NULL, set @a *result_rev to the value of the revision actually
 * checked out from the repository.
 *
 * If @a peg_revision->kind is @c svn_opt_revision_unspecified, then it
 * defaults to @c svn_opt_revision_head.
 *
 * @a revision must be of kind @c svn_opt_revision_number,
 * @c svn_opt_revision_head, or @c svn_opt_revision_date.  If
 * @a revision does not meet these requirements, return the error
 * @c SVN_ERR_CLIENT_BAD_REVISION.
 *
 * If @a depth is @c svn_depth_infinity, check out fully recursively.
 * Else if it is @c svn_depth_immediates, check out @a URL and its
 * immediate entries (subdirectories will be present, but will be at
 * depth @c svn_depth_empty themselves); else @c svn_depth_files,
 * check out @a URL and its file entries, but no subdirectories; else
 * if @c svn_depth_empty, check out @a URL as an empty directory at
 * that depth, with no entries present.
 *
 * If @a depth is @c svn_depth_unknown, then behave as if for
 * @c svn_depth_infinity, except in the case of resuming a previous
 * checkout of @a path (i.e., updating), in which case use the depth
 * of the existing working copy.
 *
 * If @a ignore_externals is set, don't process externals definitions
 * as part of this operation.
 *
 * If @a ctx->notify_func2 is non-null, invoke @a ctx->notify_func2 with
 * @a ctx->notify_baton2 as the checkout progresses.
 *
 * If @a allow_unver_obstructions is true then the checkout tolerates
 * existing unversioned items that obstruct added paths from @a URL.  Only
 * obstructions of the same type (file or dir) as the added item are
 * tolerated.  The text of obstructing files is left as-is, effectively
 * treating it as a user modification after the checkout.  Working
 * properties of obstructing items are set equal to the base properties.
 * If @a allow_unver_obstructions is false then the checkout will abort
 * if there are any unversioned obstructing items.
 *
 * If @a URL refers to a file rather than a directory, return the
 * error @c SVN_ERR_UNSUPPORTED_FEATURE.  If @a URL does not exist,
 * return the error @c SVN_ERR_RA_ILLEGAL_URL.
 *
 * Use @a pool for any temporary allocation.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_checkout3(svn_revnum_t *result_rev,
                     const char *URL,
                     const char *path,
                     const svn_opt_revision_t *peg_revision,
                     const svn_opt_revision_t *revision,
                     svn_depth_t depth,
                     svn_boolean_t ignore_externals,
                     svn_boolean_t allow_unver_obstructions,
                     svn_client_ctx_t *ctx,
                     apr_pool_t *pool);


/**
 * Similar to svn_client_checkout3() but with @a allow_unver_obstructions
 * always set to false, and @a depth set according to @a recurse: if
 * @a recurse is true, @a depth is @c svn_depth_infinity, if @a recurse
 * is false, @a depth is @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
svn_error_t *
svn_client_checkout2(svn_revnum_t *result_rev,
                     const char *URL,
                     const char *path,
                     const svn_opt_revision_t *peg_revision,
                     const svn_opt_revision_t *revision,
                     svn_boolean_t recurse,
                     svn_boolean_t ignore_externals,
                     svn_client_ctx_t *ctx,
                     apr_pool_t *pool);


/**
 * Similar to svn_client_checkout3(), but with @a allow_unver_obstructions
 * always set to false, @a peg_revision always set to
 * @c svn_opt_revision_unspecified, @a ignore_externals always set to
 * false, and @a depth set according to @a recurse: if @a recurse is
 * true, @a depth is @c svn_depth_infinity, if @a recurse is false,
 * @a depth is @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_checkout(svn_revnum_t *result_rev,
                    const char *URL,
                    const char *path,
                    const svn_opt_revision_t *revision,
                    svn_boolean_t recurse,
                    svn_client_ctx_t *ctx,
                    apr_pool_t *pool);
/** @} */

/**
 * @defgroup Update
 *
 * @{
 *
 */

/**
 * Update working trees @a paths to @a revision, authenticating with the
 * authentication baton cached in @a ctx.  @a paths is an array of const
 * char * paths to be updated.  Unversioned paths that are direct children
 * of a versioned path will cause an update that attempts to add that path,
 * other unversioned paths are skipped.  If @a result_revs is not
 * @c NULL an array of svn_revnum_t will be returned with each element set
 * to the revision to which @a revision was resolved.
 *
 * @a revision must be of kind @c svn_opt_revision_number,
 * @c svn_opt_revision_head, or @c svn_opt_revision_date.  If @a 
 * revision does not meet these requirements, return the error
 * @c SVN_ERR_CLIENT_BAD_REVISION.
 *
 * The paths in @a paths can be from multiple working copies from multiple
 * repositories, but even if they all come from the same repository there
 * is no guarantee that revision represented by @c svn_opt_revision_head
 * will remain the same as each path is updated.
 *
 * If @a ignore_externals is set, don't process externals definitions
 * as part of this operation.
 *
 * If @a depth is @c svn_depth_infinity, update fully recursively.
 * Else if it is @c svn_depth_immediates or @c svn_depth_files, update
 * each target and its file entries, but not its subdirectories.  Else
 * if @c svn_depth_empty, update exactly each target, nonrecursively
 * (essentially, update the target's properties).
 *
 * If @a depth is @c svn_depth_unknown, take the working depth from
 * @a paths and then behave as described above.
 *
 * ### TODO(sd): In the @c svn_depth_immediates case, shouldn't we update
 * ### the presence/absence of subdirs, even though we don't update
 * ### inside the subdirs themselves?
 *
 * If @a allow_unver_obstructions is true then the update tolerates
 * existing unversioned items that obstruct added paths from @a URL.  Only
 * obstructions of the same type (file or dir) as the added item are
 * tolerated.  The text of obstructing files is left as-is, effectively
 * treating it as a user modification after the update.  Working
 * properties of obstructing items are set equal to the base properties.
 * If @a allow_unver_obstructions is false then the update will abort
 * if there are any unversioned obstructing items.
 *
 * If @a ctx->notify_func2 is non-null, invoke @a ctx->notify_func2 with
 * @a ctx->notify_baton2 for each item handled by the update, and also for
 * files restored from text-base.  If @a ctx->cancel_func is non-null, invoke
 * it passing @a ctx->cancel_baton at various places during the update.
 *
 * Use @a pool for any temporary allocation.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_update3(apr_array_header_t **result_revs,
                   const apr_array_header_t *paths,
                   const svn_opt_revision_t *revision,
                   svn_depth_t depth,
                   svn_boolean_t ignore_externals,
                   svn_boolean_t allow_unver_obstructions,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

/**
 * Similar to svn_client_update3() but with @a allow_unver_obstructions
 * always set to false, and @a depth set according to @a recurse:
 * if @a recurse is true, set @a depth to @c svn_depth_infinity, if
 * @a recurse is false, set @a depth to @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
svn_error_t *
svn_client_update2(apr_array_header_t **result_revs,
                   const apr_array_header_t *paths,
                   const svn_opt_revision_t *revision,
                   svn_boolean_t recurse,
                   svn_boolean_t ignore_externals,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

/**
 * Similar to svn_client_update3() except @a allow_unver_obstructions
 * is always set to false, accepts only a single target in @a path,
 * returns a single revision if @a result_rev is not NULL,
 * @a ignore_externals is always set to false, and @a depth set
 * according to @a recurse: if @a recurse is true, set @a depth to @c
 * svn_depth_infinity, if @a recurse is false, set @a depth to
 * @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_update(svn_revnum_t *result_rev,
                  const char *path,
                  const svn_opt_revision_t *revision,
                  svn_boolean_t recurse,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);
/** @} */

/**
 * @defgroup Switch
 *
 * @{
 */

/** Switch working tree @a path to @a url at @a revision,
 * authenticating with the authentication baton cached in @a ctx.  If
 * @a result_rev is not @c NULL, set @a *result_rev to the value of
 * the revision to which the working copy was actually switched.
 *
 * Summary of purpose: this is normally used to switch a working
 * directory over to another line of development, such as a branch or
 * a tag.  Switching an existing working directory is more efficient
 * than checking out @a url from scratch.
 *
 * @a revision must be of kind @c svn_opt_revision_number,
 * @c svn_opt_revision_head, or @c svn_opt_revision_date; otherwise,
 * return @c SVN_ERR_CLIENT_BAD_REVISION.
 *
 * If @a depth is @c svn_depth_infinity, switch fully recursively.
 * Else if it is @c svn_depth_immediates, switch @a path and its file
 * children (if any), and switch subdirectories but do not update
 * them.  Else if @c svn_depth_files, switch just file children,
 * ignoring subdirectories completely.  Else if @c svn_depth_empty,
 * switch just @a path and touch nothing underneath it.
 *
 * ### TODO(sd): But, I think the svn_depth_immediates behavior is not
 * ### actually implemented yet.
 *
 * If @a allow_unver_obstructions is true then the switch tolerates
 * existing unversioned items that obstruct added paths from @a URL.  Only
 * obstructions of the same type (file or dir) as the added item are
 * tolerated.  The text of obstructing files is left as-is, effectively
 * treating it as a user modification after the switch.  Working
 * properties of obstructing items are set equal to the base properties.
 * If @a allow_unver_obstructions is false then the switch will abort
 * if there are any unversioned obstructing items.
 *
 * If @a ctx->notify_func2 is non-null, invoke it with @a ctx->notify_baton2
 * on paths affected by the switch.  Also invoke it for files may be restored
 * from the text-base because they were removed from the working copy.
 *
 * Use @a pool for any temporary allocation.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_switch2(svn_revnum_t *result_rev,
                   const char *path,
                   const char *url,
                   const svn_opt_revision_t *revision,
                   svn_depth_t depth,
                   svn_boolean_t allow_unver_obstructions,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);


/**
 * Similar to svn_client_switch2() but with @a allow_unver_obstructions
 * always set to false, and @a depth set according to @a recurse: if
 * @a recurse is true, set @a depth to @c svn_depth_infinity, if @a
 * recurse is false, set @a depth to @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
svn_error_t *
svn_client_switch(svn_revnum_t *result_rev,
                  const char *path,
                  const char *url,
                  const svn_opt_revision_t *revision,
                  svn_boolean_t recurse,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);

/** @} */

/**
 * @defgroup Add
 *
 * @{
 */

/**
 * Schedule a working copy @a path for addition to the repository.
 *
 * ### TODO(sd): For consistency, this should take svn_depth_t depth
 * ### instead of svn_boolean_t recursive.  However, it is not
 * ### important for the sparse-directories work, so leaving it
 * ### for now.
 * 
 * @a path's parent must be under revision control already, but @a 
 * path is not.  If @a recursive is set, then assuming @a path is a 
 * directory, all of its contents will be scheduled for addition as 
 * well.
 *
 * If @a force is not set and @a path is already under version
 * control, return the error @c SVN_ERR_ENTRY_EXISTS.  If @a force is
 * set, do not error on already-versioned items.  When used on a
 * directory in conjunction with the @a recursive flag, this has the
 * effect of scheduling for addition unversioned files and directories
 * scattered deep within a versioned tree.
 *
 * If @a ctx->notify_func2 is non-null, then for each added item, call
 * @a ctx->notify_func2 with @a ctx->notify_baton2 and the path of the 
 * added item.
 *
 * If @a no_ignore is false, don't add files or directories that match
 * ignore patterns.
 *
 * @par Important:
 * This is a *scheduling* operation.  No changes will
 * happen to the repository until a commit occurs.  This scheduling
 * can be removed with svn_client_revert().
 *
 * @since New in 1.3.
 */
svn_error_t *
svn_client_add3(const char *path,
                svn_boolean_t recursive,
                svn_boolean_t force,
                svn_boolean_t no_ignore,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool);

/**
 * Similar to svn_client_add3(), but with @a no_ignore always set to
 * false. 
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 */
svn_error_t *
svn_client_add2(const char *path,
                svn_boolean_t recursive,
                svn_boolean_t force,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool);

/**
 * Similar to svn_client_add3(), but with @a no_ignore always set to
 * false, and @a force always set to false.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
svn_error_t *
svn_client_add(const char *path,
               svn_boolean_t recursive,
               svn_client_ctx_t *ctx,
               apr_pool_t *pool);

/** @} */

/**
 * @defgroup Mkdir
 *
 * @{
 */

/** Create a directory, either in a repository or a working copy.
 *
 * If @a paths contains URLs, use the authentication baton in @a ctx
 * and @a message to immediately attempt to commit the creation of the
 * directories in @a paths in the repository.  If the commit succeeds,
 * allocate (in @a pool) and populate @a *commit_info_p.
 *
 * Else, create the directories on disk, and attempt to schedule them
 * for addition (using svn_client_add(), whose docstring you should
 * read).
 *
 * @a ctx->log_msg_func3/@a ctx->log_msg_baton3 are a callback/baton
 * combo that this function can use to query for a commit log message
 * when one is needed.
 *
 * If @a ctx->notify_func2 is non-null, when the directory has been created
 * (successfully) in the working copy, call @a ctx->notify_func2 with
 * @a ctx->notify_baton2 and the path of the new directory.  Note that this is
 * only called for items added to the working copy.
 *
 * @since New in 1.3.
 */
svn_error_t *
svn_client_mkdir2(svn_commit_info_t **commit_info_p,
                  const apr_array_header_t *paths,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);


/**
 * Same as svn_client_mkdir2(), but takes the @c svn_client_commit_info_t
 * type for @a commit_info_p.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 */
svn_error_t *
svn_client_mkdir(svn_client_commit_info_t **commit_info_p,
                 const apr_array_header_t *paths,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool);

/** @} */

/**
 * @defgroup Delete
 *
 * @{
 */

/** Delete items from a repository or working copy.
 *
 * If the paths in @a paths are URLs, use the authentication baton in
 * @a ctx and @a ctx->log_msg_func3/@a ctx->log_msg_baton3 to
 * immediately attempt to commit a deletion of the URLs from the
 * repository.  If the commit succeeds, allocate (in @a pool) and
 * populate @a *commit_info_p.  Every path must belong to the same
 * repository.
 *
 * Else, schedule the working copy paths in @a paths for removal from
 * the repository.  Each path's parent must be under revision control.
 * This is just a *scheduling* operation.  No changes will happen to
 * the repository until a commit occurs.  This scheduling can be
 * removed with svn_client_revert(). If a path is a file it is
 * immediately removed from the working copy. If the path is a
 * directory it will remain in the working copy but all the files, and
 * all unversioned items, it contains will be removed. If @a force is
 * not set then this operation will fail if any path contains locally
 * modified and/or unversioned items. If @a force is set such items
 * will be deleted.
 *
 * If the paths are working copy paths and @a keep_local is true then
 * the paths will not be removed from the working copy, only scheduled
 * for removal from the repository.  Once the scheduled deletion is
 * committed, they will appear as unversioned paths in the working copy.
 *
 * @a ctx->log_msg_func3/@a ctx->log_msg_baton3 are a callback/baton
 * combo that this function can use to query for a commit log message
 * when one is needed.
 *
 * If @a ctx->notify_func2 is non-null, then for each item deleted, call
 * @a ctx->notify_func2 with @a ctx->notify_baton2 and the path of the deleted
 * item.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_delete3(svn_commit_info_t **commit_info_p,
                   const apr_array_header_t *paths,
                   svn_boolean_t force,
                   svn_boolean_t keep_local,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

/**
 * Similar to svn_client_delete3(), but with @a keep_local always set
 * to false.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
svn_error_t *
svn_client_delete2(svn_commit_info_t **commit_info_p,
                   const apr_array_header_t *paths,
                   svn_boolean_t force,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

/**
 * Similar to svn_client_delete2(), but takes the @c svn_client_commit_info_t
 * type for @a commit_info_p.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 */
svn_error_t *
svn_client_delete(svn_client_commit_info_t **commit_info_p,
                  const apr_array_header_t *paths,
                  svn_boolean_t force,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);


/** @} */

/**
 * @defgroup Import
 *
 * @{
 */

/** Import file or directory @a path into repository directory @a url at
 * head, authenticating with the authentication baton cached in @a ctx, 
 * and using @a ctx->log_msg_func3/@a ctx->log_msg_baton3 to get a log message 
 * for the (implied) commit.  Set @a *commit_info_p to the results of the 
 * commit, allocated in @a pool.  If some components of @a url do not exist
 * then create parent directories as necessary.
 *
 * If @a path is a directory, the contents of that directory are
 * imported directly into the directory identified by @a url.  Note that the
 * directory @a path itself is not imported -- that is, the basename of 
 * @a path is not part of the import.
 *
 * If @a path is a file, then the dirname of @a url is the directory
 * receiving the import.  The basename of @a url is the filename in the
 * repository.  In this case if @a url already exists, return error.
 *
 * If @a ctx->notify_func2 is non-null, then call @a ctx->notify_func2 with 
 * @a ctx->notify_baton2 as the import progresses, with any of the following 
 * actions: @c svn_wc_notify_commit_added,
 * @c svn_wc_notify_commit_postfix_txdelta.
 *
 * Use @a pool for any temporary allocation.  
 * 
 * @a ctx->log_msg_func3/@a ctx->log_msg_baton3 are a callback/baton
 * combo that this function can use to query for a commit log message
 * when one is needed.
 *
 * ### TODO(sd): For consistency, this should probably take svn_depth_t
 * ### depth instead of svn_boolean_t nonrecursive.  But it's not
 * ### needed for the sparse-directories work right now, so leaving it
 * ### alone.
 *
 * Use @a nonrecursive to indicate that imported directories should not
 * recurse into any subdirectories they may have.
 *
 * If @a no_ignore is false, don't add files or directories that match
 * ignore patterns.
 *
 * ### kff todo: This import is similar to cvs import, in that it does
 * not change the source tree into a working copy.  However, this
 * behavior confuses most people, and I think eventually svn _should_
 * turn the tree into a working copy, or at least should offer the
 * option. However, doing so is a bit involved, and we don't need it
 * right now.
 *
 * @since New in 1.3.
 */
svn_error_t *svn_client_import2(svn_commit_info_t **commit_info_p,
                                const char *path,
                                const char *url,
                                svn_boolean_t nonrecursive,
                                svn_boolean_t no_ignore,
                                svn_client_ctx_t *ctx,
                                apr_pool_t *pool);

/**
 * Similar to svn_client_import2(), but with @a no_ignore always set
 * to false and using the @c svn_client_commit_info_t type for
 * @a commit_info_p.
 * 
 * @deprecated Provided for backward compatibility with the 1.2 API.
 */
svn_error_t *svn_client_import(svn_client_commit_info_t **commit_info_p,
                               const char *path,
                               const char *url,
                               svn_boolean_t nonrecursive,
                               svn_client_ctx_t *ctx,
                               apr_pool_t *pool);

/** @} */

/**
 * @defgroup Commit
 *
 * @{
 */

/**
 * Commit files or directories into repository, authenticating with
 * the authentication baton cached in @a ctx, and using 
 * @a ctx->log_msg_func3/@a ctx->log_msg_baton3 to obtain the log message. 
 * Set @a *commit_info_p to the results of the commit, allocated in @a pool.
 *
 * @a targets is an array of <tt>const char *</tt> paths to commit.  They 
 * need not be canonicalized nor condensed; this function will take care of
 * that.  If @a targets has zero elements, then do nothing and return
 * immediately without error.
 *
 * If @a ctx->notify_func2 is non-null, then call @a ctx->notify_func2 with 
 * @a ctx->notify_baton2 as the commit progresses, with any of the following 
 * actions: @c svn_wc_notify_commit_modified, @c svn_wc_notify_commit_added,
 * @c svn_wc_notify_commit_deleted, @c svn_wc_notify_commit_replaced,
 * @c svn_wc_notify_commit_postfix_txdelta.
 *
 * ### TODO(sd): For consistency, this should probably take svn_depth_t
 * ### depth instead of svn_boolean_t recurse.  But it's not needed
 * ### for the sparse-directories work right now, so leaving it alone
 * ### for now, although note that this is a 1.5 API so revving it
 * ### would be fairly painless.
 *
 * If @a recurse is false, subdirectories of directories in @a targets
 * will be ignored.
 *
 * Unlock paths in the repository, unless @a keep_locks is true.
 *
 * If @a changelist_name is non-NULL, then use it as a restrictive filter
 * on items that are committed;  that is, don't commit anything unless
 * it's a member of changelist @a changelist_name.  After the commit
 * completes successfully, remove changelist associations from the
 * targets, unless @a keep_changelist is set.
 *
 * Use @a pool for any temporary allocations.
 *
 * If no error is returned and @a (*commit_info_p)->revision is set to
 * @c SVN_INVALID_REVNUM, then the commit was a no-op; nothing needed to
 * be committed.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_commit4(svn_commit_info_t **commit_info_p,
                   const apr_array_header_t *targets,
                   svn_boolean_t recurse,
                   svn_boolean_t keep_locks,
                   svn_boolean_t keep_changelist,
                   const char *changelist_name,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

/**
 * Similar to svn_client_commit4(), but always with NULL for
 * @a changelist_name and false for @a keep_changelist.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 *
 * @since New in 1.3.
 */
svn_error_t *
svn_client_commit3(svn_commit_info_t **commit_info_p,
                   const apr_array_header_t *targets,
                   svn_boolean_t recurse,
                   svn_boolean_t keep_locks,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

/**
 * Similar to svn_client_commit4(), but always with NULL for
 * @a changelist_name and false for @a keep_changelist, and
 * uses the @c svn_client_commit_info_t type for @a commit_info_p.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_client_commit2(svn_client_commit_info_t **commit_info_p,
                   const apr_array_header_t *targets,
                   svn_boolean_t recurse,
                   svn_boolean_t keep_locks,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

/**
 * Similar to svn_client_commit4(), but always with NULL for
 * @a changelist_name, false for @a keep_changelist, true for
 * @a keep_locks, a @a nonrecursive argument instead of @a recurse,
 * and using the @c svn_client_commit_info_t type for
 * @a commit_info_p.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_commit(svn_client_commit_info_t **commit_info_p,
                  const apr_array_header_t *targets,
                  svn_boolean_t nonrecursive,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);

/** @} */

/**
 * @defgroup Status
 *
 * @{
 */

/**
 * Given @a path to a working copy directory (or single file), call
 * @a status_func/status_baton with a set of @c svn_wc_status_t *
 * structures which describe the status of @a path, and its children
 * (recursing according to @a depth).
 *
 *    - If @a get_all is set, retrieve all entries; otherwise,
 *      retrieve only "interesting" entries (local mods and/or
 *      out-of-date).
 *
 *    - If @a update is set, contact the repository and augment the
 *      status structures with information about out-of-dateness (with
 *      respect to @a revision).  Also, if @a result_rev is not @c NULL,
 *      set @a *result_rev to the actual revision against which the
 *      working copy was compared (@a *result_rev is not meaningful unless
 *      @a update is set).
 *
 * If @a ignore_externals is not set, then recurse into externals
 * definitions (if any exist) after handling the main target.  This
 * calls the client notification function (in @a ctx) with the @c
 * svn_wc_notify_status_external action before handling each externals
 * definition, and with @c svn_wc_notify_status_completed
 * after each.
 * 
 * @since New in 1.5.
 */
svn_error_t *
svn_client_status3(svn_revnum_t *result_rev,
                   const char *path,
                   const svn_opt_revision_t *revision,
                   svn_wc_status_func2_t status_func,
                   void *status_baton,
                   svn_depth_t depth,
                   svn_boolean_t get_all,
                   svn_boolean_t update,
                   svn_boolean_t no_ignore,
                   svn_boolean_t ignore_externals,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

/**
 * Like svn_client_status3(), except with @a recurse instead of @a depth.
 * If @a recurse is true, behave as if for @c svn_depth_infinity; else
 * if @a recurse is false, behave as if for @c svn_depth_files.
 *
 * @since New in 1.2.
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
svn_error_t *
svn_client_status2(svn_revnum_t *result_rev,
                   const char *path,
                   const svn_opt_revision_t *revision,
                   svn_wc_status_func2_t status_func,
                   void *status_baton,
                   svn_boolean_t recurse,
                   svn_boolean_t get_all,
                   svn_boolean_t update,
                   svn_boolean_t no_ignore,
                   svn_boolean_t ignore_externals,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);


/**
 * Similar to svn_client_status2(), but with @a ignore_externals
 * always set to false, taking the @c svn_wc_status_func_t type
 * instead of the @c svn_wc_status_func2_t type for @a status_func,
 * and requiring @a *revision to be non-const even though it is
 * treated as constant.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_status(svn_revnum_t *result_rev,
                  const char *path,
                  svn_opt_revision_t *revision,
                  svn_wc_status_func_t status_func,
                  void *status_baton,
                  svn_boolean_t recurse,
                  svn_boolean_t get_all,
                  svn_boolean_t update,
                  svn_boolean_t no_ignore,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);

/** @} */

/**
 * @defgroup Log
 *
 * @{
 */

/** 
 * Invoke @a receiver with @a receiver_baton on each log message from @a 
 * start to @a end in turn, inclusive (but never invoke @a receiver on a 
 * given log message more than once).
 *
 * @a targets contains either a URL followed by zero or more relative
 * paths, or a list of working copy paths, as <tt>const char *</tt>,
 * for which log messages are desired.  The repository info is
 * determined by taking the common prefix of the target entries' URLs.
 * @a receiver is invoked only on messages whose revisions involved a
 * change to some path in @a targets.  @a peg_revision indicates in
 * which revision @a targets are valid.  If @a peg_revision is @c
 * svn_opt_revision_unspecified, it defaults to @c
 * svn_opt_revision_head for URLs or @c svn_opt_revision_working for
 * WC paths.
 *
 * If @a limit is non-zero only invoke @a receiver on the first @a limit
 * logs.
 *
 * If @a discover_changed_paths is set, then the `@a changed_paths' argument
 * to @a receiver will be passed on each invocation.
 *
 * If @a strict_node_history is set, copy history (if any exists) will
 * not be traversed while harvesting revision logs for each target.
 *
 * If @a start->kind or @a end->kind is @c svn_opt_revision_unspecified,
 * return the error @c SVN_ERR_CLIENT_BAD_REVISION.
 *
 * Use @a pool for any temporary allocation.
 *
 * @par Important:
 * A special case for the revision range HEAD:1, which was present
 * in svn_client_log(), has been removed from svn_client_log2().  Instead, it
 * is expected that callers will specify the range HEAD:0, to avoid a 
 * SVN_ERR_FS_NO_SUCH_REVISION error when invoked against an empty repository
 * (i.e. one not containing a revision 1).
 *
 * If @a ctx->notify_func2 is non-null, then call @a ctx->notify_func2/baton2
 * with a 'skip' signal on any unversioned targets.
 *
 * @since New in 1.4.
 */
svn_error_t *
svn_client_log3(const apr_array_header_t *targets,
                const svn_opt_revision_t *peg_revision,
                const svn_opt_revision_t *start,
                const svn_opt_revision_t *end,
                int limit,
                svn_boolean_t discover_changed_paths,
                svn_boolean_t strict_node_history,
                svn_log_message_receiver_t receiver,
                void *receiver_baton,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool);


/**
 * Similar to svn_client_log3(), but with the @c kind field of
 * @a peg_revision set to @c svn_opt_revision_unspecified.
 *
 * @deprecated Provided for compatibility with the 1.3 API.
 * @since New in 1.2.
 */
svn_error_t *
svn_client_log2(const apr_array_header_t *targets,
                const svn_opt_revision_t *start,
                const svn_opt_revision_t *end,
                int limit,
                svn_boolean_t discover_changed_paths,
                svn_boolean_t strict_node_history,
                svn_log_message_receiver_t receiver,
                void *receiver_baton,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool);


/**
 * Similar to svn_client_log3(), but with the @c kind field of
 * @a peg_revision set to @c svn_opt_revision_unspecified,
 * @a limit set to 0, and the following special case:
 *
 * Special case for repositories at revision 0:
 *
 * If @a start->kind is @c svn_opt_revision_head, and @a end->kind is
 * @c svn_opt_revision_number && @a end->number is @c 1, then handle an
 * empty (no revisions) repository specially: instead of erroring
 * because requested revision 1 when the highest revision is 0, just
 * invoke @a receiver on revision 0, passing @c NULL for changed paths and
 * empty strings for the author and date.  This is because that
 * particular combination of @a start and @a end usually indicates the
 * common case of log invocation -- the user wants to see all log
 * messages from youngest to oldest, where the oldest commit is
 * revision 1.  That works fine, except when there are no commits in
 * the repository, hence this special case.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
svn_error_t *
svn_client_log(const apr_array_header_t *targets,
               const svn_opt_revision_t *start,
               const svn_opt_revision_t *end,
               svn_boolean_t discover_changed_paths,
               svn_boolean_t strict_node_history,
               svn_log_message_receiver_t receiver,
               void *receiver_baton,
               svn_client_ctx_t *ctx,
               apr_pool_t *pool);

/** @} */

/**
 * @defgroup Blame
 *
 * @{
 */

/**
 * Invoke @a receiver with @a receiver_baton on each line-blame item
 * associated with revision @a end of @a path_or_url, using @a start
 * as the default source of all blame.  @a peg_revision indicates in
 * which revision @a path_or_url is valid.  If @a peg_revision->kind
 * is @c svn_opt_revision_unspecified, then it defaults to @c
 * svn_opt_revision_head for URLs or @c svn_opt_revision_working for
 * WC targets.
 *
 * If @a start->kind or @a end->kind is @c svn_opt_revision_unspecified,
 * return the error @c SVN_ERR_CLIENT_BAD_REVISION.  If either are @c
 * svn_opt_revision_working, return the error @c
 * SVN_ERR_UNSUPPORTED_FEATURE.  If any of the revisions of @a
 * path_or_url have a binary mime-type, return the error @c
 * SVN_ERR_CLIENT_IS_BINARY_FILE, unless @a ignore_mime_type is true,
 * in which case blame information will be generated regardless of the
 * MIME types of the revisions.
 *
 * Use @a diff_options to determine how to compare different revisions of the
 * target.
 *
 * Use @a pool for any temporary allocation.
 *
 * @since New in 1.4.
 */
svn_error_t *
svn_client_blame3(const char *path_or_url,
                  const svn_opt_revision_t *peg_revision,
                  const svn_opt_revision_t *start,
                  const svn_opt_revision_t *end,
                  const svn_diff_file_options_t *diff_options,
                  svn_boolean_t ignore_mime_type,
                  svn_client_blame_receiver_t receiver,
                  void *receiver_baton,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);

/**
 * Similar to svn_client_blame3(), but with @a diff_options set to
 * default options as returned by svn_diff_file_options_parse() and
 * @a ignore_mime_type set to false.
 *
 * @deprecated Provided for backwards compatibility with the 1.3 API.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_client_blame2(const char *path_or_url,
                  const svn_opt_revision_t *peg_revision,
                  const svn_opt_revision_t *start,
                  const svn_opt_revision_t *end,
                  svn_client_blame_receiver_t receiver,
                  void *receiver_baton,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);

/**
 * Similar to svn_client_blame3(), but with @a diff_options set to
 * default options as returned by svn_diff_file_options_parse(),
 * @a ignore_mime_type set to false, and @a peg_revision always
 * the same as @a end.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_blame(const char *path_or_url,
                 const svn_opt_revision_t *start,
                 const svn_opt_revision_t *end,
                 svn_client_blame_receiver_t receiver,
                 void *receiver_baton,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool);

/** @} */

/**
 * @defgroup Diff
 *
 * @{
 */

/**
 * Produce diff output which describes the delta between
 * @a path1/@a revision1 and @a path2/@a revision2.  Print the output 
 * of the diff to @a outfile, and any errors to @a errfile.  @a path1 
 * and @a path2 can be either working-copy paths or URLs.
 *
 * If either @a revision1 or @a revision2 has an `unspecified' or
 * unrecognized `kind', return @c SVN_ERR_CLIENT_BAD_REVISION.
 *
 * @a path1 and @a path2 must both represent the same node kind -- that 
 * is, if @a path1 is a directory, @a path2 must also be, and if @a path1 
 * is a file, @a path2 must also be.
 *
 * If @a depth is @c svn_depth_infinity, diff fully recursively.
 * Else if it is @c svn_depth_immediates or @c svn_depth_files, diff
 * the named paths and their file children (if any), but no
 * subdirectories.  Else if @c svn_depth_empty, diff just the named
 * paths but nothing underneath them.
 * 
 * ### TODO(sd): If svn_depth_immediates, would it perhaps be more correct
 * ### to diff file children and subdirs' properties?
 *
 * Use @a ignore_ancestry to control whether or not items being
 * diffed will be checked for relatedness first.  Unrelated items
 * are typically transmitted to the editor as a deletion of one thing
 * and the addition of another, but if this flag is true, unrelated
 * items will be diffed as if they were related.
 *
 * If @a no_diff_deleted is true, then no diff output will be
 * generated on deleted files.
 *
 * Generated headers are encoded using @a header_encoding.
 *
 * Diff output will not be generated for binary files, unless @a
 * ignore_content_type is true, in which case diffs will be shown
 * regardless of the content types.
 * 
 * @a diff_options (an array of <tt>const char *</tt>) is used to pass 
 * additional command line options to the diff processes invoked to compare
 * files.
 *
 * The authentication baton cached in @a ctx is used to communicate with 
 * the repository.
 *
 * @note @a header_encoding doesn't affect headers generated by external
 * diff programs.
 *
 * @since New in 1.5.
 */
svn_error_t *svn_client_diff4(const apr_array_header_t *diff_options,
                              const char *path1,
                              const svn_opt_revision_t *revision1,
                              const char *path2,
                              const svn_opt_revision_t *revision2,
                              svn_depth_t depth,
                              svn_boolean_t ignore_ancestry,
                              svn_boolean_t no_diff_deleted,
                              svn_boolean_t ignore_content_type,
                              const char *header_encoding,
                              apr_file_t *outfile,
                              apr_file_t *errfile,
                              svn_client_ctx_t *ctx,
                              apr_pool_t *pool);


/**
 * Similar to svn_client_diff4(), but with @a depth set according to
 * @a recurse: if @a recurse is true, set @a depth to @c
 * svn_depth_infinity, if @a recurse is false, set @a depth to @c
 * svn_depth_empty.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 *
 * @since New in 1.3.
 */
svn_error_t *svn_client_diff3(const apr_array_header_t *diff_options,
                              const char *path1,
                              const svn_opt_revision_t *revision1,
                              const char *path2,
                              const svn_opt_revision_t *revision2,
                              svn_boolean_t recurse,
                              svn_boolean_t ignore_ancestry,
                              svn_boolean_t no_diff_deleted,
                              svn_boolean_t ignore_content_type,
                              const char *header_encoding,
                              apr_file_t *outfile,
                              apr_file_t *errfile,
                              svn_client_ctx_t *ctx,
                              apr_pool_t *pool);


/**
 * Similar to svn_client_diff4(), but with @a header_encoding set to
 * @c APR_LOCALE_CHARSET, and @a depth set according to @a recurse:
 * if @a recurse is true, set @a depth to @c svn_depth_infinity, if
 * @a recurse is false, set @a depth to @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 *
 * @since New in 1.2.
 */
svn_error_t *svn_client_diff2(const apr_array_header_t *diff_options,
                              const char *path1,
                              const svn_opt_revision_t *revision1,
                              const char *path2,
                              const svn_opt_revision_t *revision2,
                              svn_boolean_t recurse,
                              svn_boolean_t ignore_ancestry,
                              svn_boolean_t no_diff_deleted,
                              svn_boolean_t ignore_content_type,
                              apr_file_t *outfile,
                              apr_file_t *errfile,
                              svn_client_ctx_t *ctx,
                              apr_pool_t *pool);

/**
 * Similar to svn_client_diff4(), but with @a header_encoding set to
 * @c APR_LOCALE_CHARSET, @a ignore_content_type always set to false,
 * and @a depth set according to @a recurse: if @a recurse is true,
 * set @a depth to @c svn_depth_infinity, if @a recurse is false, set
 * @a depth to @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
svn_error_t *svn_client_diff(const apr_array_header_t *diff_options,
                             const char *path1,
                             const svn_opt_revision_t *revision1,
                             const char *path2,
                             const svn_opt_revision_t *revision2,
                             svn_boolean_t recurse,
                             svn_boolean_t ignore_ancestry,
                             svn_boolean_t no_diff_deleted,
                             apr_file_t *outfile,
                             apr_file_t *errfile,
                             svn_client_ctx_t *ctx,
                             apr_pool_t *pool);

/**
 * Produce diff output which describes the delta between the
 * filesystem object @a path in peg revision @a peg_revision, as it
 * changed between @a start_revision and @a end_revision.  @a path can
 * be either a working-copy path or URL.
 *
 * If @a peg_revision is @c svn_opt_revision_unspecified, behave
 * identically to svn_client_diff4(), using @a path for both of that
 * function's @a path1 and @a path2 argments.
 *
 * All other options are handled identically to svn_client_diff4().
 *
 * @since New in 1.5.
 */
svn_error_t *svn_client_diff_peg4(const apr_array_header_t *diff_options,
                                  const char *path,
                                  const svn_opt_revision_t *peg_revision,
                                  const svn_opt_revision_t *start_revision,
                                  const svn_opt_revision_t *end_revision,
                                  svn_depth_t depth,
                                  svn_boolean_t ignore_ancestry,
                                  svn_boolean_t no_diff_deleted,
                                  svn_boolean_t ignore_content_type,
                                  const char *header_encoding,
                                  apr_file_t *outfile,
                                  apr_file_t *errfile,
                                  svn_client_ctx_t *ctx,
                                  apr_pool_t *pool);

/**
 * Similar to svn_client_diff_peg4(), but with @a depth set according
 * to @a recurse: if @a recurse is true, set @a depth to
 * @c svn_depth_infinity, if @a recurse is false, set @a depth to
 * @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 *
 * @since New in 1.3.
 */
svn_error_t *svn_client_diff_peg3(const apr_array_header_t *diff_options,
                                  const char *path,
                                  const svn_opt_revision_t *peg_revision,
                                  const svn_opt_revision_t *start_revision,
                                  const svn_opt_revision_t *end_revision,
                                  svn_boolean_t recurse,
                                  svn_boolean_t ignore_ancestry,
                                  svn_boolean_t no_diff_deleted,
                                  svn_boolean_t ignore_content_type,
                                  const char *header_encoding,
                                  apr_file_t *outfile,
                                  apr_file_t *errfile,
                                  svn_client_ctx_t *ctx,
                                  apr_pool_t *pool);

/**
 * Similar to svn_client_diff_peg4(), but with @a header_encoding set
 * to @c APR_LOCALE_CHARSET, and with @a depth set according to
 * @a recurse: if @a recurse is true, set @a depth to
 * @c svn_depth_infinity, if @a recurse is false, set @a depth to
 * @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 *
 * @since New in 1.2.
 */
svn_error_t *svn_client_diff_peg2(const apr_array_header_t *diff_options,
                                  const char *path,
                                  const svn_opt_revision_t *peg_revision,
                                  const svn_opt_revision_t *start_revision,
                                  const svn_opt_revision_t *end_revision,
                                  svn_boolean_t recurse,
                                  svn_boolean_t ignore_ancestry,
                                  svn_boolean_t no_diff_deleted,
                                  svn_boolean_t ignore_content_type,
                                  apr_file_t *outfile,
                                  apr_file_t *errfile,
                                  svn_client_ctx_t *ctx,
                                  apr_pool_t *pool);

/**
 * Similar to svn_client_diff_peg4(), but with @a header_encoding set to
 * @c APR_LOCALE_CHARSET, @a ignore_content_type always set to false,
 * and with @a depth set according to @a recurse: if @a recurse is
 * true, set @a depth to @c svn_depth_infinity, if @a recurse is
 * false, set @a depth to @c svn_depth_files.
 *
 * @since New in 1.1.
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *svn_client_diff_peg(const apr_array_header_t *diff_options,
                                 const char *path,
                                 const svn_opt_revision_t *peg_revision,
                                 const svn_opt_revision_t *start_revision,
                                 const svn_opt_revision_t *end_revision,
                                 svn_boolean_t recurse,
                                 svn_boolean_t ignore_ancestry,
                                 svn_boolean_t no_diff_deleted,
                                 apr_file_t *outfile,
                                 apr_file_t *errfile,
                                 svn_client_ctx_t *ctx,
                                 apr_pool_t *pool);

/**
 * Produce a diff summary which lists the changed items between
 * @a path1/@a revision1 and @a path2/@a revision2 without creating text
 * deltas. @a path1 and @a path2 can be either working-copy paths or URLs.
 *
 * The function may report false positives if @a ignore_ancestry is false,
 * since a file might have been modified between two revisions, but still
 * have the same contents.
 *
 * Calls @a summarize_func with @a summarize_baton for each difference
 * with a @c svn_client_diff_summarize_t structure describing the difference.
 *
 * See svn_client_diff4() for a description of the other parameters.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_diff_summarize2(const char *path1,
                           const svn_opt_revision_t *revision1,
                           const char *path2,
                           const svn_opt_revision_t *revision2,
                           svn_depth_t depth,
                           svn_boolean_t ignore_ancestry,
                           svn_client_diff_summarize_func_t summarize_func,
                           void *summarize_baton,
                           svn_client_ctx_t *ctx,
                           apr_pool_t *pool);

/**
 * Similar to svn_client_diff_summarize2(), but with @a depth set
 * according to @a recurse: if @a recurse is true, set @a depth to
 * @c svn_depth_infinity, if @a recurse is false, set @a depth to
 * @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 *
 * @since New in 1.4.
 */
svn_error_t *
svn_client_diff_summarize(const char *path1,
                          const svn_opt_revision_t *revision1,
                          const char *path2,
                          const svn_opt_revision_t *revision2,
                          svn_boolean_t recurse,
                          svn_boolean_t ignore_ancestry,
                          svn_client_diff_summarize_func_t summarize_func,
                          void *summarize_baton,
                          svn_client_ctx_t *ctx,
                          apr_pool_t *pool);

/**
 * Produce a diff summary which lists the changed items between the
 * filesystem object @a path in peg revision @a peg_revision, as it
 * changed between @a start_revision and @a end_revision. @a path can
 * be either a working-copy path or URL.
 *
 * If @a peg_revision is @c svn_opt_revision_unspecified, behave
 * identically to svn_client_diff_summarize2(), using @a path for both
 * of that function's @a path1 and @a path2 argments.
 *
 * The function may report false positives if @a ignore_ancestry is false,
 * as described in the documentation for svn_client_diff_summarize2().
 *
 * Call @a summarize_func with @a summarize_baton for each difference
 * with a @c svn_client_diff_summarize_t structure describing the difference.
 *
 * See svn_client_diff_peg4() for a description of the other parameters.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_diff_summarize_peg2(const char *path,
                               const svn_opt_revision_t *peg_revision,
                               const svn_opt_revision_t *start_revision,
                               const svn_opt_revision_t *end_revision,
                               svn_depth_t depth,
                               svn_boolean_t ignore_ancestry,
                               svn_client_diff_summarize_func_t summarize_func,
                               void *summarize_baton,
                               svn_client_ctx_t *ctx,
                               apr_pool_t *pool);

/**
 * Similar to svn_client_diff_summarize_peg2(), but with @a depth set
 * according to @a recurse: if @a recurse is true, set @a depth to
 * @c svn_depth_infinity, if @a recurse is false, set @a depth to
 * @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 *
 * @since New in 1.4.
 */
svn_error_t *
svn_client_diff_summarize_peg(const char *path,
                              const svn_opt_revision_t *peg_revision,
                              const svn_opt_revision_t *start_revision,
                              const svn_opt_revision_t *end_revision,
                              svn_boolean_t recurse,
                              svn_boolean_t ignore_ancestry,
                              svn_client_diff_summarize_func_t summarize_func,
                              void *summarize_baton,
                              svn_client_ctx_t *ctx,
                              apr_pool_t *pool);

/** @} */

/**
 * @defgroup Merge
 *
 * @{
 */

/** Merge changes from @a source1/@a revision1 to @a source2/@a revision2 into 
 * the working-copy path @a target_wcpath.
 *
 * @a source1 and @a source2 are either URLs that refer to entries in the 
 * repository, or paths to entries in the working copy.
 *
 * By "merging", we mean:  apply file differences using
 * svn_wc_merge(), and schedule additions & deletions when appropriate.
 *
 * @a source1 and @a source2 must both represent the same node kind -- that 
 * is, if @a source1 is a directory, @a source2 must also be, and if @a source1 
 * is a file, @a source2 must also be.
 *
 * If either @a revision1 or @a revision2 has an `unspecified' or
 * unrecognized `kind', return @c SVN_ERR_CLIENT_BAD_REVISION.
 *
 * If @a depth is @c svn_depth_infinity and the URLs are
 * directories, apply changes fully recursively.  Else if it is
 * @c svn_depth_immediates and the URLs are directories, apply changes only
 * to files that are immediate children of @a target_wcpath and
 * to subdirectory properties.  Else if @c svn_depth_files, apply
 * changes only to immediate file children @a target_wcpath.  Else if
 * @c svn_depth_empty, apply changes only to @a target_wcpath (i.e.,
 * property changes only)
 *
 * ### TODO(sd): Improve above wording when figure out exactly what all
 * ### that means.
 *
 * If @a depth is @c svn_depth_unknown, use the depth of @a target_wcpath.
 *
 * Use @a ignore_ancestry to control whether or not items being
 * diffed will be checked for relatedness first.  Unrelated items
 * are typically transmitted to the editor as a deletion of one thing
 * and the addition of another, but if this flag is true, unrelated
 * items will be diffed as if they were related.
 *
 * If @a force is not set and the merge involves deleting locally modified or
 * unversioned items the operation will fail.  If @a force is set such items
 * will be deleted.
 *
 * @a merge_options (an array of <tt>const char *</tt>), if non-null,
 * is used to pass additional command line arguments to the merge
 * processes (internal or external).  @see
 * svn_diff_file_options_parse().
 *
 * If @a ctx->notify_func2 is non-null, then call @a ctx->notify_func2 with @a 
 * ctx->notify_baton2 once for each merged target, passing the target's local 
 * path.
 *
 * If @a record_only is true, the merge isn't actually performed, but
 * the merge info for the revisions which would've been merged is
 * recorded in the working copy (and must be subsequently committed
 * back to the repository).
 *
 * If @a dry_run is true, the merge is carried out, and full notification
 * feedback is provided, but the working copy is not modified.
 *
 * The authentication baton cached in @a ctx is used to communicate with the 
 * repository.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_merge3(const char *source1,
                  const svn_opt_revision_t *revision1,
                  const char *source2,
                  const svn_opt_revision_t *revision2,
                  const char *target_wcpath,
                  svn_depth_t depth,
                  svn_boolean_t ignore_ancestry,
                  svn_boolean_t force,
                  svn_boolean_t record_only,
                  svn_boolean_t dry_run,
                  const apr_array_header_t *merge_options,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);

/**
 * Similar to svn_client_merge3(), but with @a record_only set to @c
 * FALSE, and @a depth set according to @a recurse: if @a recurse is
 * true, set @a depth to @c svn_depth_infinity, if @a recurse is
 * false, set @a depth to @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 *
 * @since New in 1.4.
 */
svn_error_t *
svn_client_merge2(const char *source1,
                  const svn_opt_revision_t *revision1,
                  const char *source2,
                  const svn_opt_revision_t *revision2,
                  const char *target_wcpath,
                  svn_boolean_t recurse,
                  svn_boolean_t ignore_ancestry,
                  svn_boolean_t force,
                  svn_boolean_t dry_run,
                  const apr_array_header_t *merge_options,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);


/**
 * Similar to svn_client_merge3(), but with @a merge_options set to @c NULL,
 * and @a depth set according to @a recurse: if @a recurse is true,
 * set @a depth to @c svn_depth_infinity, if @a recurse is false, set
 * @a depth to @c svn_depth_files.
 *
 * @deprecated Provided for backwards compatibility with the 1.3 API.
 */
svn_error_t *
svn_client_merge(const char *source1,
                 const svn_opt_revision_t *revision1,
                 const char *source2,
                 const svn_opt_revision_t *revision2,
                 const char *target_wcpath,
                 svn_boolean_t recurse,
                 svn_boolean_t ignore_ancestry,
                 svn_boolean_t force,
                 svn_boolean_t dry_run,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool);


/**
 * Merge the changes between the filesystem object @a source in peg
 * revision @a peg_revision, as it changed between @a revision1 and @a
 * revision2.  
 *
 * All other options are handled identically to svn_client_merge3().
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_merge_peg3(const char *source,
                      const svn_opt_revision_t *revision1,
                      const svn_opt_revision_t *revision2,
                      const svn_opt_revision_t *peg_revision,
                      const char *target_wcpath,
                      svn_depth_t depth,
                      svn_boolean_t ignore_ancestry,
                      svn_boolean_t force,
                      svn_boolean_t record_only,
                      svn_boolean_t dry_run,
                      const apr_array_header_t *merge_options,
                      svn_client_ctx_t *ctx,
                      apr_pool_t *pool);

/**
 * Similar to svn_client_merge_peg3(), but with @a record_only set to
 * @c FALSE, and @a depth set according to @a recurse: if @a recurse
 * is true, set @a depth to @c svn_depth_infinity, if @a recurse is
 * false, set @a depth to @c svn_depth_files.
 *
 * @deprecated Provided for backwards compatibility with the 1.3 API.
 *
 * @since New in 1.4.
 */
svn_error_t *
svn_client_merge_peg2(const char *source,
                      const svn_opt_revision_t *revision1,
                      const svn_opt_revision_t *revision2,
                      const svn_opt_revision_t *peg_revision,
                      const char *target_wcpath,
                      svn_boolean_t recurse,
                      svn_boolean_t ignore_ancestry,
                      svn_boolean_t force,
                      svn_boolean_t dry_run,
                      const apr_array_header_t *merge_options,
                      svn_client_ctx_t *ctx,
                      apr_pool_t *pool);

/**
 * Similar to svn_client_merge_peg3(), but with @a merge_options set to
 * @c NULL, and @a depth set according to @a recurse: if @a recurse is
 * true, set @a depth to @c svn_depth_infinity, if @a recurse is false,
 * set @a depth to @c svn_depth_files.
 *
 * @deprecated Provided for backwards compatibility with the 1.3 API.
 *
 * @since New in 1.1.
 */
svn_error_t *
svn_client_merge_peg(const char *source,
                     const svn_opt_revision_t *revision1,
                     const svn_opt_revision_t *revision2,
                     const svn_opt_revision_t *peg_revision,
                     const char *target_wcpath,
                     svn_boolean_t recurse,
                     svn_boolean_t ignore_ancestry,
                     svn_boolean_t force,
                     svn_boolean_t dry_run,
                     svn_client_ctx_t *ctx,
                     apr_pool_t *pool);

/** @} */

/**
 * @defgroup Cleanup
 *
 * @{
 */

/** Recursively cleanup a working copy directory @a dir, finishing any
 * incomplete operations, removing lockfiles, etc.
 *
 * If @a ctx->cancel_func is non-null, invoke it with @a
 * ctx->cancel_baton at various points during the operation.  If it
 * returns an error (typically SVN_ERR_CANCELLED), return that error
 * immediately.
 */
svn_error_t *
svn_client_cleanup(const char *dir,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);


/** @} */

/**
 * @defgroup Relocate
 *
 * @{
 */

/**
 * Modify a working copy directory @a dir, changing any
 * repository URLs that begin with @a from to begin with @a to instead,
 * recursing into subdirectories if @a recurse is true.
 *
 * @param dir Working copy directory
 * @param from Original URL
 * @param to New URL
 * @param recurse Whether to recurse
 * @param ctx svn_client_ctx_t
 * @param pool The pool from which to perform memory allocations
 *
 * ### TODO(sd): I don't see any reason to change this recurse parameter
 * ### to a depth, but making a note to re-check this logic later.
 */
svn_error_t *
svn_client_relocate(const char *dir,
                    const char *from,
                    const char *to,
                    svn_boolean_t recurse,
                    svn_client_ctx_t *ctx,
                    apr_pool_t *pool);


/** @} */

/**
 * @defgroup Revert
 *
 * @{
 */

/** Restore the pristine version of a working copy @a paths,
 * effectively undoing any local mods.  For each path in @a paths, if
 * it is a directory, and @a recursive is true, this will be a
 * recursive operation.
 *
 * ### TODO(sd): I don't see any reason to change this recurse parameter
 * ### to a depth, but making a note to re-check this logic later.
 *
 * If @a ctx->notify_func2 is non-null, then for each item reverted,
 * call @a ctx->notify_func2 with @a ctx->notify_baton2 and the path of
 * the reverted item.
 *
 * If an item specified for reversion is not under version control,
 * then do not error, just invoke @a ctx->notify_func2 with @a
 * ctx->notify_baton2, using notification code @c svn_wc_notify_skip.
 */
svn_error_t *
svn_client_revert(const apr_array_header_t *paths,
                  svn_boolean_t recursive,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);


/** @} */

/**
 * @defgroup Resolved
 *
 * @{
 */

/** Remove the 'conflicted' state on a working copy @a path.  This will
 * not semantically resolve conflicts;  it just allows @a path to be
 * committed in the future.  The implementation details are opaque.
 * If @a recursive is set, recurse below @a path, looking for conflicts 
 * to resolve.
 *
 * ### TODO(sd): I don't see any reason to change this recurse parameter
 * ### to a depth, but making a note to re-check this logic later.
 *
 * If @a path is not in a state of conflict to begin with, do nothing.
 * If @a path's conflict state is removed and @a ctx->notify_func2 is non-null,
 * call @a ctx->notify_func2 with @a ctx->notify_baton2 and @a path.
 */
svn_error_t *
svn_client_resolved(const char *path,
                    svn_boolean_t recursive,
                    svn_client_ctx_t *ctx,
                    apr_pool_t *pool);


/** @} */

/**
 * @defgroup Copy
 *
 * @{
 */

/**
 * A structure which describes the source of a copy operation--its path, 
 * revision, and peg revision.
 *
 * @since New in 1.5.
 */
typedef struct svn_client_copy_source_t
{
    /** The source path or URL. */
    const char *path;

    /** The source operational revision. */
    const svn_opt_revision_t *revision;

    /** The source peg revision. */
    const svn_opt_revision_t *peg_revision;
} svn_client_copy_source_t;

/** Copy each @a src in @a sources to @a dst_path.
 *
 * If multiple @a sources are given, @a dst_path must be a directory,
 * and @a sources will be copied as children of @a dst_path.
 *
 * @a sources must be an array of elements of type
 * <tt>svn_client_copy_item_t *</tt>.
 *
 * Each @a src in @a sources must be files or directories under version control,
 * or URLs of a versioned item in the repository.  If @a sources has multiple
 * items, the @a src members must be all repository URLs or all working copy
 * paths.
 *
 * The parent of @a dst_path must already exist.
 *
 * If @a sources has only one item, attempt to copy it to @a dst_path.  If
 * @a copy_as_child is TRUE and @a dst_path already exists, attempt to copy the
 * item as a child of @a dst_path.  If @a copy_as_child is FALSE and
 * @a dst_path already exists, fail with @c SVN_ERR_ENTRY_EXISTS if @a dst_path
 * is a working copy path and @c SVN_ERR_FS_ALREADY_EXISTS if @a dst_path is a
 * URL.
 *
 * If @a sources has multiple items, and @a copy_as_child is TRUE, all
 * @a sources are copied as children of @a dst_path.  If any child of
 * @a dst_path already exists with the same name any item in @a sources,
 * fail with @c SVN_ERR_ENTRY_EXISTS if @a dst_path is a working copy path and
 * @c SVN_ERR_FS_ALREADY_EXISTS if @a dst_path is a URL.
 *
 * If @a src_paths has multiple items, and @a copy_as_child is FALSE, fail
 * with @c SVN_ERR_CLIENT_MULTIPLE_SOURCES_DISALLOWED.
 *
 * If @a dst_path is a URL, use the authentication baton 
 * in @a ctx and @a ctx->log_msg_func3/@a ctx->log_msg_baton3 to immediately 
 * attempt to commit the copy action in the repository.  If the commit 
 * succeeds, allocate (in @a pool) and populate @a *commit_info_p.  If
 * @a dst_path is not a URL, and the copy succeeds, set @a
 * *commit_info_p to @c NULL.
 *
 * If @a dst_path is not a URL, then this is just a variant of 
 * svn_client_add(), where the @a sources are scheduled for addition
 * as copies.  No changes will happen to the repository until a commit occurs.
 * This scheduling can be removed with svn_client_revert().
 *
 * @a ctx->log_msg_func3/@a ctx->log_msg_baton3 are a callback/baton combo
 * that this function can use to query for a commit log message when one is
 * needed.
 *
 * If @a ctx->notify_func2 is non-null, invoke it with @a ctx->notify_baton2
 * for each item added at the new location, passing the new, relative path of
 * the added item.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_copy4(svn_commit_info_t **commit_info_p,
                 apr_array_header_t *sources,
                 const char *dst_path,
                 svn_boolean_t copy_as_child,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool);

/**
 * Similar to svn_client_copy4(), with only one @a src_path and
 * @a copy_as_child set to FALSE.  Also, use @a src_revision as both 
 * the operational and peg revision.
 *
 * @since New in 1.4.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
svn_error_t *
svn_client_copy3(svn_commit_info_t **commit_info_p,
                 const char *src_path,
                 const svn_opt_revision_t *src_revision,
                 const char *dst_path,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool);


/**
 * Similar to svn_client_copy3(), with the difference that if @a dst_path
 * already exists and is a directory, copy the item into that directory,
 * keeping its name (the last component of @a src_path).
 *
 * @since New in 1.3.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
svn_error_t *
svn_client_copy2(svn_commit_info_t **commit_info_p,
                 const char *src_path,
                 const svn_opt_revision_t *src_revision,
                 const char *dst_path,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool);


/** 
 * Similar to svn_client_copy3(), but with the difference that if
 * @a dst_path already exists and is a directory, copy the item into
 * that directory, keeping its name (the last component of @a src_path);
 * and using the @c svn_client_commit_info_t type for @a commit_info_p.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 */
svn_error_t *
svn_client_copy(svn_client_commit_info_t **commit_info_p,
                const char *src_path,
                const svn_opt_revision_t *src_revision,
                const char *dst_path,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool);


/** @} */

/**
 * @defgroup Move
 *
 * @{
 */

/**
 * Move @a src_paths to @a dst_path.
 *
 * @a src_paths must be files or directories under version control, or
 * URLs of versioned items in the repository.  All @a src_paths must be of
 * the same type.  If multiple @a src_paths are given, @a dst_path must be
 * a directory and @a src_paths will be moved as children of @a dst_path.
 *
 * If @a src_paths are repository URLs:
 *
 *   - @a dst_path must also be a repository URL.
 *
 *   - The authentication baton in @a ctx and @a ctx->log_msg_func/@a 
 *     ctx->log_msg_baton are used to commit the move.
 *
 *   - The move operation will be immediately committed.  If the
 *     commit succeeds, allocate (in @a pool) and populate @a *commit_info_p.
 *
 * If @a src_paths are working copy paths:
 *
 *   - @a dst_path must also be a working copy path.
 *
 *   - @a ctx->log_msg_func3 and @a ctx->log_msg_baton3 are ignored.
 *
 *   - This is a scheduling operation.  No changes will happen to the
 *     repository until a commit occurs.  This scheduling can be removed
 *     with svn_client_revert().  If one of @a src_paths is a file it is
 *     removed from the working copy immediately.  If one of @a src_path
 *     is a directory it will remain in the working copy but all the files,
 *     and unversioned items, it contains will be removed.
 *
 *   - If one of @a src_paths contains locally modified and/or unversioned
 *     items and @a force is not set, the move will fail. If @a force is set
 *     such items will be removed.
 *
 *   - If the move succeeds, set @a *commit_info_p to @c NULL.
 *
 * The parent of @a dst_path must already exist.
 *
 * If @a src_paths has only one item, attempt to move it to @a dst_path.  If
 * @a move_as_child is TRUE and @a dst_path already exists, attempt to move the
 * item as a child of @a dst_path.  If @a move_as_child is FALSE and
 * @a dst_path already exists, fail with @c SVN_ERR_ENTRY_EXISTS if @a dst_path
 * is a working copy path and @c SVN_ERR_FS_ALREADY_EXISTS if @a dst_path is a
 * URL.
 *
 * If @a src_paths has multiple items, and @a move_as_child is TRUE, all
 * @a src_paths are moved as children of @a dst_path.  If any child of
 * @a dst_path already exists with the same name any item in @a src_paths,
 * fail with @c SVN_ERR_ENTRY_EXISTS if @a dst_path is a working copy path and
 * @c SVN_ERR_FS_ALREADY_EXISTS if @a dst_path is a URL.
 *
 * If @a src_paths has multiple items, and @a move_as_child is FALSE, fail
 * with @c SVN_ERR_CLIENT_MULTIPLE_SOURCES_DISALLOWED.
 *
 * @a ctx->log_msg_func3/@a ctx->log_msg_baton3 are a callback/baton combo that
 * this function can use to query for a commit log message when one is needed.
 *
 * If @a ctx->notify_func2 is non-null, then for each item moved, call
 * @a ctx->notify_func2 with the @a ctx->notify_baton2 twice, once to indicate 
 * the deletion of the moved thing, and once to indicate the addition of
 * the new location of the thing.
 *
 * ### Is this really true?  What about svn_wc_notify_commit_replaced()? ###
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_move5(svn_commit_info_t **commit_info_p,
                 apr_array_header_t *src_paths,
                 const char *dst_path,
                 svn_boolean_t force,
                 svn_boolean_t move_as_child,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool);

/**
 * Similar to svn_client_move5(), with only one @a src_path and
 * @a move_as_child set to FALSE.
 *
 * @since New in 1.4.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */ 
svn_error_t *
svn_client_move4(svn_commit_info_t **commit_info_p,
                 const char *src_path,
                 const char *dst_path,
                 svn_boolean_t force,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool);

/**
 * Similar to svn_client_move4(), with the difference that if @a dst_path
 * already exists and is a directory, move the item into that directory,
 * keeping its name (the last component of @a src_path).
 *
 * @since New in 1.3.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
svn_error_t *
svn_client_move3(svn_commit_info_t **commit_info_p,
                 const char *src_path,
                 const char *dst_path,
                 svn_boolean_t force,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool);

/** 
 * Similar to svn_client_move4(), with the difference that if
 * @a dst_path already exists and is a directory, move the item
 * into that directory, keeping its name (the last component of
 * @a src_path); and with the @c svn_client_commit_info_t type for
 * @a commit_info_p.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_client_move2(svn_client_commit_info_t **commit_info_p,
                 const char *src_path,
                 const char *dst_path,
                 svn_boolean_t force,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool);

/**
 * Similar to svn_client_move4(), with the difference that if
 * @a dst_path already exists and is a directory, move the item
 * into that directory, keeping its name (the last component of
 * @a src_path); and with the @c svn_client_commit_info_t type for
 * @a commit_info_p; and with an extra argument @a src_revision.
 * The extra argument has no effect, but must be of kind
 * @c svn_opt_revision_unspecified or @c svn_opt_revision_head,
 * otherwise error @c SVN_ERR_UNSUPPORTED_FEATURE is returned.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */ 
svn_error_t *
svn_client_move(svn_client_commit_info_t **commit_info_p,
                const char *src_path,
                const svn_opt_revision_t *src_revision,
                const char *dst_path,
                svn_boolean_t force,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool);

/** @} */


/** Properties
 *
 * Note that certain svn-controlled properties must always have their
 * values set and stored in UTF8 with LF line endings.  When
 * retrieving these properties, callers must convert the values back
 * to native locale and native line-endings before displaying them to
 * the user.  For help with this task, see
 * svn_prop_needs_translation(), svn_subst_translate_string(),  and
 * svn_subst_detranslate_string().
 *
 * @defgroup svn_client_prop_funcs Property functions
 * @{
 */


/**
 * Set @a propname to @a propval on @a target.  If @a recurse is true, 
 * then @a propname will be set on recursively on @a target and all 
 * children.  If @a recurse is false, and @a target is a directory, @a 
 * propname will be set on _only_ @a target.
 *
 * ### TODO(sd): I don't see any reason to change this recurse parameter
 * ### to a depth right now; it's not exactly part of the
 * ### sparse-directories feature, although it's related.
 * 
 * A @a propval of @c NULL will delete the property.
 *
 * The @a target may only be an URL if @a base_revision_for_url is not
 * @c SVN_INVALID_REVNUM; in this case, the property will only be set
 * if it has not changed since revision @a base_revision_for_url.  @a
 * base_revision_for_url must be @c SVN_INVALID_REVNUM if @a target is
 * not an URL.  @a recurse is not supported on URLs.  The
 * authentication baton in @a ctx and @a ctx->log_msg_func3/@a
 * ctx->log_msg_baton3 will be used to immediately attempt to commit
 * the property change in the repository.  If the commit succeeds,
 * allocate (in @a pool) and populate @a *commit_info_p.
 *
 * If @a propname is an svn-controlled property (i.e. prefixed with
 * @c SVN_PROP_PREFIX), then the caller is responsible for ensuring that
 * the value is UTF8-encoded and uses LF line-endings.
 *
 * If @a skip_checks is true, do no validity checking.  But if @a
 * skip_checks is false, and @a propname is not a valid property for @a
 * target, return an error, either @c SVN_ERR_ILLEGAL_TARGET (if the
 * property is not appropriate for @a target), or @c
 * SVN_ERR_BAD_MIME_TYPE (if @a propname is "svn:mime-type", but @a
 * propval is not a valid mime-type).
 *
 * If @a ctx->cancel_func is non-null, invoke it passing @a
 * ctx->cancel_baton at various places during the operation.
 *
 * Use @a pool for all memory allocation.
 * 
 * @since New in 1.5.
 */
svn_error_t *
svn_client_propset3(svn_commit_info_t **commit_info_p,
                    const char *propname,
                    const svn_string_t *propval,
                    const char *target,
                    svn_boolean_t recurse,
                    svn_boolean_t skip_checks,
                    svn_revnum_t base_revision_for_url,
                    svn_client_ctx_t *ctx,
                    apr_pool_t *pool);

/**
 * Like svn_client_propset2(), but with @a base_revision_for_url
 * always @c SVN_INVALID_REVNUM, and @a commit_info_p always NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
svn_error_t *
svn_client_propset2(const char *propname,
                    const svn_string_t *propval,
                    const char *target,
                    svn_boolean_t recurse,
                    svn_boolean_t skip_checks,
                    svn_client_ctx_t *ctx,
                    apr_pool_t *pool);

/** 
 * Like svn_client_propset2(), but with @a skip_checks always false and a
 * newly created @a ctx.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_propset(const char *propname,
                   const svn_string_t *propval,
                   const char *target,
                   svn_boolean_t recurse,
                   apr_pool_t *pool);

/** Set @a propname to @a propval on revision @a revision in the repository
 * represented by @a URL.  Use the authentication baton in @a ctx for 
 * authentication, and @a pool for all memory allocation.  Return the actual 
 * rev affected in @a *set_rev.  A @a propval of @c NULL will delete the 
 * property.
 *
 * If @a force is true, allow newlines in the author property.
 *
 * If @a propname is an svn-controlled property (i.e. prefixed with
 * @c SVN_PROP_PREFIX), then the caller is responsible for ensuring that
 * the value UTF8-encoded and uses LF line-endings.
 *
 * Note that unlike its cousin svn_client_propset2(), this routine
 * doesn't affect the working copy at all;  it's a pure network
 * operation that changes an *unversioned* property attached to a
 * revision.  This can be used to tweak log messages, dates, authors,
 * and the like.  Be careful:  it's a lossy operation.
 *
 * Also note that unless the administrator creates a
 * pre-revprop-change hook in the repository, this feature will fail.
 */
svn_error_t *
svn_client_revprop_set(const char *propname,
                       const svn_string_t *propval,
                       const char *URL,
                       const svn_opt_revision_t *revision,
                       svn_revnum_t *set_rev,
                       svn_boolean_t force,
                       svn_client_ctx_t *ctx,
                       apr_pool_t *pool);
                        
/**
 * Set @a *props to a hash table whose keys are `<tt>char *</tt>' paths,
 * prefixed by @a target (a working copy path or a URL), of items on
 * which property @a propname is set, and whose values are `@c svn_string_t
 * *' representing the property value for @a propname at that path.
 *
 * Allocate @a *props, its keys, and its values in @a pool.
 *           
 * Don't store any path, not even @a target, if it does not have a
 * property named @a propname.
 *
 * If @a revision->kind is @c svn_opt_revision_unspecified, then: get
 * properties from the working copy if @a target is a working copy
 * path, or from the repository head if @a target is a URL.  Else get
 * the properties as of @a revision.  The actual node revision
 * selected is determined by the path as it exists in @a peg_revision.
 * If @a peg_revision->kind is @c svn_opt_revision_unspecified, then
 * it defaults to @c svn_opt_revision_head for URLs or @c
 * svn_opt_revision_working for WC targets.  Use the authentication
 * baton in @a ctx for authentication if contacting the repository.
 * If @a actual_revnum is not @c NULL, the actual revision number used
 * for the fetch is stored in @a *actual_revnum.
 *
 * If @a target is a file or @a recurse is false, @a *props will have
 * at most one element.
 *
 * ### TODO(sd): I don't see any reason to change this recurse parameter
 * ### to a depth right now; it's not exactly part of the
 * ### sparse-directories feature, although it's related.  Usually
 * ### you would just name the target carefully... Is there a
 * ### situation where depth support would be useful here?
 *
 * If error, don't touch @a *props, otherwise @a *props is a hash table 
 * even if empty.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_client_propget3(apr_hash_t **props,
                    const char *propname,
                    const char *target,
                    const svn_opt_revision_t *peg_revision,
                    const svn_opt_revision_t *revision,
                    svn_revnum_t *actual_revnum,
                    svn_boolean_t recurse,
                    svn_client_ctx_t *ctx,
                    apr_pool_t *pool);

/**
 * Similar to svn_client_propget3(), except that @a actual_revnum is
 * always @c NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
svn_error_t *
svn_client_propget2(apr_hash_t **props,
                    const char *propname,
                    const char *target,
                    const svn_opt_revision_t *peg_revision,
                    const svn_opt_revision_t *revision,
                    svn_boolean_t recurse,
                    svn_client_ctx_t *ctx,
                    apr_pool_t *pool);

/**
 * Similar to svn_client_propget2(), except that @a peg_revision is
 * always the same as @a revision.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_propget(apr_hash_t **props,
                   const char *propname,
                   const char *target,
                   const svn_opt_revision_t *revision,
                   svn_boolean_t recurse,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

/** Set @a *propval to the value of @a propname on revision @a revision 
 * in the repository represented by @a URL.  Use the authentication baton 
 * in @a ctx for authentication, and @a pool for all memory allocation.  
 * Return the actual rev queried in @a *set_rev.
 *
 * Note that unlike its cousin svn_client_propget(), this routine
 * doesn't affect the working copy at all; it's a pure network
 * operation that queries an *unversioned* property attached to a
 * revision.  This can query log messages, dates, authors, and the
 * like.
 */
svn_error_t *
svn_client_revprop_get(const char *propname,
                       svn_string_t **propval,
                       const char *URL,
                       const svn_opt_revision_t *revision,
                       svn_revnum_t *set_rev,
                       svn_client_ctx_t *ctx,
                       apr_pool_t *pool);

/**
 * Invoke @a receiver with @a receiver_baton to return the regular properies
 * of @a target, a URL or working copy path.  @a receiver will be called
 * for each path encountered.
 *
 * If @a revision->kind is @c svn_opt_revision_unspecified, then get
 * properties from the working copy, if @a target is a working copy
 * path, or from the repository head if @a target is a URL.  Else get
 * the properties as of @a revision.  The actual node revision
 * selected is determined by the path as it exists in @a peg_revision.
 * If @a peg_revision->kind is @c svn_opt_revision_unspecified, then it
 * defaults to @c svn_opt_revision_head for URLs or @c
 * svn_opt_revision_working for WC targets.  Use the authentication
 * baton cached in @a ctx for authentication if contacting the
 * repository.
 *
 * If @a recurse is false, or @a target is a file, @a *props will contain 
 * only a single element.  Otherwise, it will contain one element for each
 * versioned entry below (and including) @a target.
 *
 * ### TODO(sd): I don't see any reason to change this recurse parameter
 * ### to a depth right now; it's not exactly part of the
 * ### sparse-directories feature, although it's related.  Usually
 * ### you would just name the target carefully... Is there a
 * ### situation where depth support would be useful here?
 *
 * If @a target is not found, return the error @c SVN_ERR_ENTRY_NOT_FOUND.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_proplist3(const char *target,
                     const svn_opt_revision_t *peg_revision,
                     const svn_opt_revision_t *revision,
                     svn_boolean_t recurse,
                     svn_proplist_receiver_t receiver,
                     void *receiver_baton,
                     svn_client_ctx_t *ctx,
                     apr_pool_t *pool);

/**
 * Similar to svn_client_proplist3(), except the properties are returned 
 * as an array of @c svn_client_proplist_item_t * structures, instead of
 * by invoking the receiver function.
 *
 * @since New in 1.2.
 *
 * @deprecated Provided for backward compatiblility with the 1.2 API.
 */
svn_error_t *
svn_client_proplist2(apr_array_header_t **props,
                     const char *target,
                     const svn_opt_revision_t *peg_revision,
                     const svn_opt_revision_t *revision,
                     svn_boolean_t recurse,
                     svn_client_ctx_t *ctx,
                     apr_pool_t *pool);

/**
 * Similar to svn_client_proplist2(), except that @a peg_revision is
 * always the same as @a revision.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_proplist(apr_array_header_t **props,
                    const char *target,
                    const svn_opt_revision_t *revision,
                    svn_boolean_t recurse,
                    svn_client_ctx_t *ctx,
                    apr_pool_t *pool);

/** Set @a *props to a hash of the revision props attached to @a revision in
 * the repository represented by @a URL.  Use the authentication baton cached 
 * in @a ctx for authentication, and @a pool for all memory allocation.  
 * Return the actual rev queried in @a *set_rev.
 *
 * The allocated hash maps (<tt>const char *</tt>) property names to
 * (@c svn_string_t *) property values.
 *
 * Note that unlike its cousin svn_client_proplist(), this routine
 * doesn't read a working copy at all; it's a pure network operation
 * that reads *unversioned* properties attached to a revision.
 */
svn_error_t *
svn_client_revprop_list(apr_hash_t **props,
                        const char *URL,
                        const svn_opt_revision_t *revision,
                        svn_revnum_t *set_rev,
                        svn_client_ctx_t *ctx,
                        apr_pool_t *pool);
/** @} */


/**
 * @defgroup Export
 *
 * @{
 */

/**
 * Export the contents of either a subversion repository or a
 * subversion working copy into a 'clean' directory (meaning a
 * directory with no administrative directories).  If @a result_rev
 * is not @c NULL and the path being exported is a repository URL, set
 * @a *result_rev to the value of the revision actually exported (set
 * it to @c SVN_INVALID_REVNUM for local exports).
 *
 * @a from is either the path the working copy on disk, or a URL to the
 * repository you wish to export.
 *
 * @a to is the path to the directory where you wish to create the exported
 * tree.
 *
 * @a peg_revision is the revision where the path is first looked up
 * when exporting from a repository.  If @a peg_revision->kind is @c
 * svn_opt_revision_unspecified, then it defaults to @c svn_opt_revision_head
 * for URLs or @c svn_opt_revision_working for WC targets.
 *
 * @a revision is the revision that should be exported, which is only used 
 * when exporting from a repository.
 *
 * @a ctx->notify_func2 and @a ctx->notify_baton2 are the notification
 * functions and baton which are passed to svn_client_checkout() when
 * exporting from a repository.
 *
 * @a ctx is a context used for authentication in the repository case.
 *
 * @a overwrite if true will cause the export to overwrite files or directories.
 *
 * If @a ignore_externals is set, don't process externals definitions
 * as part of this operation.
 *
 * @a native_eol allows you to override the standard eol marker on the platform
 * you are running on.  Can be either "LF", "CR" or "CRLF" or NULL.  If NULL
 * will use the standard eol marker.  Any other value will cause the
 * SVN_ERR_IO_UNKNOWN_EOL error to be returned.
 *
 * If @a depth is @c svn_depth_infinity, export fully recursively.
 * Else if it is @c svn_depth_immediates, export @a from and its immediate
 * children (if any), but with subdirectories empty and at
 * @c svn_depth_empty.  Else if @c svn_depth_files, export @a from and
 * its immediate file children (if any) only.  If @a depth is @c
 * svn_depth_empty, then export exactly @a from and none of its children.
 *
 * If @a recurse is true, export recursively.  Otherwise, export
 * just the directory represented by @a from and its immediate
 * non-directory children, but none of its child directories (if any).
 * Also, if @a recurse is false, the export will behave as if
 * @a ignore_externals is true.
 *
 * All allocations are done in @a pool.
 *
 * @since New in 1.5.
 */ 
svn_error_t *
svn_client_export4(svn_revnum_t *result_rev,
                   const char *from,
                   const char *to,
                   const svn_opt_revision_t *peg_revision,
                   const svn_opt_revision_t *revision,
                   svn_boolean_t overwrite, 
                   svn_boolean_t ignore_externals,
                   svn_depth_t depth,
                   const char *native_eol,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);


/**
 * Similar to svn_client_export4(), but with @a depth set according to
 * @a recurse: if @a recurse is true, set @a depth to
 * @c svn_depth_infinity, if @a recurse is false, set @a depth to
 * @c svn_depth_files.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 *
 * @since New in 1.2.
 */ 
svn_error_t *
svn_client_export3(svn_revnum_t *result_rev,
                   const char *from,
                   const char *to,
                   const svn_opt_revision_t *peg_revision,
                   const svn_opt_revision_t *revision,
                   svn_boolean_t overwrite, 
                   svn_boolean_t ignore_externals,
                   svn_boolean_t recurse,
                   const char *native_eol,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);


/**
 * Similar to svn_client_export4(), but with @a peg_revision always
 * set to @c svn_opt_revision_unspecified, @a overwrite set to the
 * value of @a force, @a ignore_externals always false, and @a depth
 * always @c svn_depth_infinity.
 *
 * @since New in 1.1.
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_export2(svn_revnum_t *result_rev,
                   const char *from,
                   const char *to,
                   svn_opt_revision_t *revision,
                   svn_boolean_t force, 
                   const char *native_eol,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);


/**
 * Similar to svn_client_export4(), but with @a peg_revision always
 * set to @c svn_opt_revision_unspecified, @a overwrite set to the
 * value of @a force, @a ignore_externals always false, @a depth
 * always @c svn_depth_infinity, and @a native_eol always @c NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
svn_error_t *
svn_client_export(svn_revnum_t *result_rev,
                  const char *from,
                  const char *to,
                  svn_opt_revision_t *revision,
                  svn_boolean_t force, 
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);

/** @} */

/**
 * @defgroup List List / ls
 *
 * @{
 */

/** Invoked by svn_client_list() for each @a path with its @a dirent and,
 * if @a path is locked, its @a lock.  @a abs_path is the filesystem path
 * to which @a path is relative.  @a baton is the baton passed to the
 * caller.  @a pool may be used for temporary allocations.
 *
 * @since New in 1.4.
 */
typedef svn_error_t *(*svn_client_list_func_t)(void *baton,
                                               const char *path,
                                               const svn_dirent_t *dirent,
                                               const svn_lock_t *lock,
                                               const char *abs_path,
                                               apr_pool_t *pool);

/**
 * Report the directory entry, and possibly children, for @a
 * path_or_url at @a revision.  The actual node revision selected is
 * determined by the path as it exists in @a peg_revision.  If @a
 * peg_revision->kind is @c svn_opt_revision_unspecified, then it defaults
 * to @c svn_opt_revision_head for URLs or @c svn_opt_revision_working
 * for WC targets.
 *
 * Report directory entries by invoking @a list_func/@a baton with @a path
 * relative to @a path_or_url.  The dirent for @a path_or_url is reported
 * using an empty @a path.  If @a path_or_url is a directory, also report
 * its children.  If @a path_or_url is non-existent, return
 * @c SVN_ERR_FS_NOT_FOUND.
 *
 * If @a fetch_locks is true, include locks when reporting directory entries.
 *
 * Use @a pool for temporary allocations.
 *
 * Use authentication baton cached in @a ctx to authenticate against the 
 * repository.
 *
 * If @a recurse is true (and @a path_or_url is a directory) this will
 * be a recursive operation.
 *
 * ### TODO(sd): This really should take depth instead of recurse, but
 * ### one thing at a time, one thing at a time...
 *
 * @a dirent_fields controls which fields in the @c svn_dirent_t's are
 * filled in.  To have them totally filled in use @c SVN_DIRENT_ALL, 
 * otherwise simply bitwise OR together the combination of @c SVN_DIRENT_
 * fields you care about.
 *
 * @since New in 1.4.
 */
svn_error_t *
svn_client_list(const char *path_or_url,
                const svn_opt_revision_t *peg_revision,
                const svn_opt_revision_t *revision,
                svn_boolean_t recurse,
                apr_uint32_t dirent_fields,
                svn_boolean_t fetch_locks,
                svn_client_list_func_t list_func,
                void *baton,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool);

/**
 * Same as svn_client_list(), but always passes @c SVN_DIRENT_ALL for
 * the @a dirent_fields argument and returns all information in two
 * hash tables instead of invoking a callback.
 *
 * Set @a *dirents to a newly allocated hash of directory entries.
 * The @a dirents hash maps entry names (<tt>const char *</tt>) to
 * @c svn_dirent_t *'s.
 *
 * If @a locks is not @c NULL, set @a *locks to a hash table mapping
 * entry names (<tt>const char *</tt>) to @c svn_lock_t *'s.
 *
 * @since New in 1.3.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
svn_error_t *
svn_client_ls3(apr_hash_t **dirents,
               apr_hash_t **locks,
               const char *path_or_url,
               const svn_opt_revision_t *peg_revision,
               const svn_opt_revision_t *revision,
               svn_boolean_t recurse,
               svn_client_ctx_t *ctx,
               apr_pool_t *pool);

/**
 * Same as svn_client_ls3(), but without the ability to get locks.
 *
 * @since New in 1.2.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 */
svn_error_t *
svn_client_ls2(apr_hash_t **dirents,
               const char *path_or_url,
               const svn_opt_revision_t *peg_revision,
               const svn_opt_revision_t *revision,
               svn_boolean_t recurse,
               svn_client_ctx_t *ctx,
               apr_pool_t *pool);

/**
 * Similar to svn_client_ls3(), but without the ability to get locks,
 * and with @a peg_revision always the same as @a revision.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_ls(apr_hash_t **dirents,
              const char *path_or_url,
              svn_opt_revision_t *revision,
              svn_boolean_t recurse,
              svn_client_ctx_t *ctx,
              apr_pool_t *pool);


/** @} */

/**
 * @defgroup Cat
 *
 * @{
 */

/**
 * Output the content of file identified by @a path_or_url and @a
 * revision to the stream @a out.  The actual node revision selected
 * is determined by the path as it exists in @a peg_revision.  If @a
 * peg_revision->kind is @c svn_opt_revision_unspecified, then it defaults
 * to @c svn_opt_revision_head for URLs or @c svn_opt_revision_working
 * for WC targets.
 *
 * If @a path_or_url is not a local path, then if @a revision is of
 * kind @c svn_opt_revision_previous (or some other kind that requires
 * a local path), an error will be returned, because the desired
 * revision cannot be determined.
 *
 * Use the authentication baton cached in @a ctx to authenticate against the 
 * repository.
 *
 * Perform all allocations from @a pool.
 *
 * ### TODO: Add an expansion/translation flag?
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_client_cat2(svn_stream_t *out,
                const char *path_or_url,
                const svn_opt_revision_t *peg_revision,
                const svn_opt_revision_t *revision,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool);


/**
 * Similar to svn_client_cat2() except that the peg revision is always
 * the same as @a revision.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
svn_error_t *
svn_client_cat(svn_stream_t *out,
               const char *path_or_url,
               const svn_opt_revision_t *revision,
               svn_client_ctx_t *ctx,
               apr_pool_t *pool);

/** @} end group: cat */



/** Changelist commands
 *
 * @defgroup svn_client_changelist_funcs Client Changelist Functions
 * @{
 */

/** Implementation note:
 *
 *  For now, changelists are implemented by scattering the
 *  associations across multiple .svn/entries files in a working copy.
 *  However, this client API was written so that we have the option of
 *  changing the underlying implementation -- we may someday want to
 *  store changelist definitions in a centralized database.
 */

/**
 * Add each path in @a paths to changelist @a changelist.
 *
 * If a path is already a member of another changelist, then remove it
 * from the other changelist and add it to @a changelist.  (For now, a path
 * cannot belong to two changelists at once.)
 *
 * @note This metadata is purely a client-side "bookkeeping"
 * convenience, and is entirely managed by the working copy.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_add_to_changelist(const apr_array_header_t *paths,
                             const char *changelist,
                             svn_client_ctx_t *ctx,
                             apr_pool_t *pool);

/**
 * Remove each path in @a paths from changelist @a changelist.
 *
 * If a path is not already a member of @a changelist, attempt to
 * throw a notification warning that the path has been skipped.
 *
 * If @a changelist is NULL, then be more lax: for each path, remove
 * it from whatever changelist it's already a member of.
 *
 * @note This metadata is purely a client-side "bookkeeping"
 * convenience, and is entirely managed by the working copy.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_remove_from_changelist(const apr_array_header_t *paths,
                                  const char *changelist,
                                  svn_client_ctx_t *ctx,
                                  apr_pool_t *pool);

/**
 * Beginning at @a root_path, seek and discover every path which
 * belongs to @a changelist_name.  Return the list of paths in @a
 * *paths.  If no matching paths are found, return an empty array.
 *
 * If @a ctx->cancel_func is non-null, invoke it passing @a ctx->cancel_baton
 * during the recursive walk.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_get_changelist(apr_array_header_t **paths,
                          const char *changelist_name,
                          const char *root_path,
                          svn_client_ctx_t *ctx,
                          apr_pool_t *pool);

/**
 * The callback type used by @a svn_client_get_changelist_streamy.
 *
 * On each invocation, @a path is a newly discovered member of the
 * changelist, and @a baton is a private function closure.
 *
 * @since New in 1.5.
 */
typedef svn_error_t *(*svn_changelist_receiver_t)
  (void *baton,
   const char *path);

/**
 * A "streamy" version of @a svn_client_get_changelist:
 *
 * Beginning at @a root_path, seek and discover every path which
 * belongs to @a changelist_name.  Call @a callback_func (with @a
 * callback_baton) each time a changelist member is found.
 *
 * If @a ctx->cancel_func is non-null, invoke it passing @a ctx->cancel_baton
 * during the recursive walk.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_client_get_changelist_streamy(svn_changelist_receiver_t callback_func,
                                  void *callback_baton,
                                  const char *changelist_name,
                                  const char *root_path,
                                  svn_client_ctx_t *ctx,
                                  apr_pool_t *pool);

/** @} */



/** Locking commands
 *
 * @defgroup svn_client_locking_funcs Client Locking Functions
 * @{
 */

/**
 * Lock @a targets in the repository.  @a targets is an array of
 * <tt>const char *</tt> paths - either all working copy paths or URLs.  All
 * @a targets must be in the same repository.
 *
 * If a target is already locked in the repository, no lock will be
 * acquired unless @a steal_lock is true, in which case the locks are
 * stolen.  @a comment, if non-null, is an xml-escapable description
 * stored with each lock in the repository.  Each acquired lock will
 * be stored in the working copy if the targets are WC paths.
 *
 * For each target @a ctx->notify_func2/notify_baton2 will be used to indicate
 * whether it was locked.  An action of @c svn_wc_notify_state_locked
 * means that the path was locked.  If the path was not locked because
 * it was out-of-date or there was already a lock in the repository,
 * the notification function will be called with @c
 * svn_wc_notify_failed_lock, and the error passed in the notification
 * structure. 
 *
 * Use @a pool for temporary allocations.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_client_lock(const apr_array_header_t *targets,
                const char *comment,
                svn_boolean_t steal_lock,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool);

/**
 * Unlock @a targets in the repository.  @a targets is an array of
 * <tt>const char *</tt> paths - either all working copy paths or all URLs.
 * All @a targets must be in the same repository.
 *
 * If the targets are WC paths, and @a break_lock is false, the working
 * copy must contain a locks for each target.
 * If this is not the case, or the working copy lock doesn't match the
 * lock token in the repository, an error will be signaled.
 *
 * If the targets are URLs, the locks may be broken even if @a break_lock
 * is false, but only if the lock owner is the same as the
 * authenticated user.
 *
 * If @a break_lock is true, the locks will be broken in the
 * repository.  In both cases, the locks, if any, will be removed from
 * the working copy if the targets are WC paths.
 *
 * The notification functions in @a ctx will be called for each
 * target.  If the target was successfully unlocked, @c
 * svn_wc_notify_unlocked will be used.  Else, if the error is
 * directly related to unlocking the path (see @c
 * SVN_ERR_IS_UNLOCK_ERROR), @c svn_wc_notify_failed_unlock will be
 * used and the error will be passed in the notification structure.

 * Use @a pool for temporary allocations.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_client_unlock(const apr_array_header_t *targets,
                  svn_boolean_t break_lock,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool);

/** @} */

/**
 * @defgroup Info
 *
 * @{
 */

/**
 * A structure which describes various system-generated metadata about
 * a working-copy path or URL.
 *
 * @note Fields may be added to the end of this structure in future
 * versions.  Therefore, users shouldn't allocate structures of this
 * type, to preserve binary compatibility.
 *
 * @since New in 1.2.
 */
typedef struct svn_info_t
{
  /** Where the item lives in the repository. */
  const char *URL;

  /** The revision of the object.  If path_or_url is a working-copy
   * path, then this is its current working revnum.  If path_or_url
   * is a URL, then this is the repos revision that path_or_url lives in. */
  svn_revnum_t rev;

  /** The node's kind. */
  svn_node_kind_t kind;

  /** The root URL of the repository. */
  const char *repos_root_URL;
  
  /** The repository's UUID. */
  const char *repos_UUID;

  /** The last revision in which this object changed. */
  svn_revnum_t last_changed_rev;
  
  /** The date of the last_changed_rev. */
  apr_time_t last_changed_date;
  
  /** The author of the last_changed_rev. */
  const char *last_changed_author;

  /** An exclusive lock, if present.  Could be either local or remote. */
  svn_lock_t *lock;

  /** Whether or not to ignore the next 10 wc-specific fields. */
  svn_boolean_t has_wc_info;

  /**
   * @name Working-copy path fields
   * These things only apply to a working-copy path.
   * See svn_wc_entry_t for explanations.
   * @{
   */
  svn_wc_schedule_t schedule;
  const char *copyfrom_url;
  svn_revnum_t copyfrom_rev;
  apr_time_t text_time;
  apr_time_t prop_time;
  const char *checksum;
  const char *conflict_old;
  const char *conflict_new;
  const char *conflict_wrk;
  const char *prejfile;

  /* @since New in 1.5. */
  const char *changelist;
  svn_depth_t depth;
  /** @} */

} svn_info_t;


/**
 * The callback invoked by svn_client_info().  Each invocation
 * describes @a path with the information present in @a info.  Note
 * that any fields within @a info may be NULL if information is
 * unavailable.  Use @a pool for all temporary allocation.
 *
 * @since New in 1.2.
 */
typedef svn_error_t *(*svn_info_receiver_t)
  (void *baton,
   const char *path,
   const svn_info_t *info,
   apr_pool_t *pool);

/** 
 * Return a duplicate of @a info, allocated in @a pool. No part of the new
 * structure will be shared with @a info.
 *
 * @since New in 1.3.
 */
svn_info_t *
svn_info_dup(const svn_info_t *info, apr_pool_t *pool);

/**
 * Invoke @a receiver with @a receiver_baton to return information
 * about @a path_or_url in @a revision.  The information returned is
 * system-generated metadata, not the sort of "property" metadata
 * created by users.  See @c svn_info_t.
 *
 * If both revision arguments are either @c
 * svn_opt_revision_unspecified or NULL, then information will be
 * pulled solely from the working copy; no network connections will be
 * made.
 *
 * Otherwise, information will be pulled from a repository.  The
 * actual node revision selected is determined by the @a path_or_url
 * as it exists in @a peg_revision.  If @a peg_revision->kind is @c
 * svn_opt_revision_unspecified, then it defaults to @c
 * svn_opt_revision_head for URLs or @c svn_opt_revision_working for
 * WC targets.
 *
 * If @a path_or_url is not a local path, then if @a revision is of
 * kind @c svn_opt_revision_previous (or some other kind that requires
 * a local path), an error will be returned, because the desired
 * revision cannot be determined.
 *
 * Use the authentication baton cached in @a ctx to authenticate
 * against the repository.
 *
 * If @a recurse is true (and @a path_or_url is a directory) this will
 * be a recursive operation, invoking @a receiver on each child.
 *
 * ### TODO(sd): I don't see any compelling reason to switch to
 * ### depth-style instead of recurse-style control here
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_client_info(const char *path_or_url,
                const svn_opt_revision_t *peg_revision,
                const svn_opt_revision_t *revision,
                svn_info_receiver_t receiver,
                void *receiver_baton,
                svn_boolean_t recurse,
                svn_client_ctx_t *ctx,
                apr_pool_t *pool);

/** @} */

/** @} end group: Client working copy management */

/**
 *
 * @defgroup clnt_sessions Client session related functions
 *
 * @{
 *
 */


/* Converting paths to URLs. */

/** Set @a *url to the URL for @a path_or_url.
 *
 * If @a path_or_url is already a URL, set @a *url to @a path_or_url.
 *
 * If @a path_or_url is a versioned item, set @a *url to @a
 * path_or_url's entry URL.  If @a path_or_url is unversioned (has
 * no entry), set @a *url to null.
 */
svn_error_t *
svn_client_url_from_path(const char **url,
                         const char *path_or_url,
                         apr_pool_t *pool);




/* Fetching repository UUIDs. */

/** Get repository @a uuid for @a url.
 *
 * Use a @a pool to open a temporary RA session to @a url, discover the
 * repository uuid, and free the session.  Return the uuid in @a uuid,
 * allocated in @a pool.  @a ctx is required for possible repository
 * authentication.
 */
svn_error_t *
svn_client_uuid_from_url(const char **uuid,
                         const char *url,
                         svn_client_ctx_t *ctx,
                         apr_pool_t *pool);


/** Return the repository @a uuid for working-copy @a path, allocated
 * in @a pool.  Use @a adm_access to retrieve the uuid from @a path's
 * entry; if not present in the entry, then call
 * svn_client_uuid_from_url() to retrieve, using the entry's URL.  @a
 * ctx is required for possible repository authentication.
 *
 * @note The only reason this function falls back on
 * svn_client_uuid_from_url() is for compatibility purposes.  Old
 * working copies may not have uuids in the entries file.
 */
svn_error_t *
svn_client_uuid_from_path(const char **uuid,
                          const char *path,
                          svn_wc_adm_access_t *adm_access,
                          svn_client_ctx_t *ctx,
                          apr_pool_t *pool);


/* Opening RA sessions. */

/** Open an RA session rooted at @a url, and return it in @a *session.
 *
 * Use the authentication baton stored in @a ctx for authentication.
 * @a *session is allocated in @a pool.
 *
 * @since New in 1.3.
 *
 * @note This function is similar to svn_ra_open2(), but the caller avoids
 * having to providing its own callback functions.
 */
svn_error_t *
svn_client_open_ra_session(svn_ra_session_t **session,
                           const char *url,
                           svn_client_ctx_t *ctx,
                           apr_pool_t *pool);


/** @} end group: Client session related functions */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* SVN_CLIENT_H */

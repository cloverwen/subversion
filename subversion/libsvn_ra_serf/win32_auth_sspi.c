/*
 * win32_auth_sspi.c : authn implementation through SSPI
 *
 * ====================================================================
 * Copyright (c) 2007 CollabNet.  All rights reserved.
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

#ifdef WIN32

/* TODO: 
   - remove NTLM dependency so we can reuse SSPI for Kerberos later. */

/*** Includes ***/
#include <windows.h>
#include <string.h>

#include <apr_base64.h>

#include "svn_error.h"

#include "win32_auth_sspi.h"

/*** Global variables ***/
HANDLE security_dll = INVALID_HANDLE_VALUE;
INIT_SECURITY_INTERFACE InitSecurityInterface_;
static PSecurityFunctionTable sspi = NULL;
static unsigned int ntlm_maxtokensize = 0;

#define SECURITY_DLL "security.dll"

/* Loads security.dll in memory on the first call. Afterwards the 
   function table SSPI is loaded which we can use it to call SSPI's 
   public functions. */
static svn_error_t *
load_security_dll()
{
  if (security_dll != INVALID_HANDLE_VALUE)
    return SVN_NO_ERROR;

  security_dll = LoadLibrary(SECURITY_DLL);
  if (security_dll != INVALID_HANDLE_VALUE)
    {
      /* Load the function(s) */
      InitSecurityInterface_ = 
        (INIT_SECURITY_INTERFACE)GetProcAddress(security_dll, 
                                                "InitSecurityInterfaceA");
      sspi = InitSecurityInterface_();

      if (sspi)
        return SVN_NO_ERROR;
    }

  /* Initialization failed, clean up and raise error */
  if (security_dll)
    FreeLibrary(security_dll);

  return svn_error_createf
          (SVN_ERR_RA_SERF_SSPI_INITIALISATION_FAILED, NULL,
           "SSPI Initialization failed.");
}

/* Calculates the maximum token size based on the authentication protocol. */
static svn_error_t *
sspi_maxtokensize(char *auth_pkg, unsigned int *maxtokensize)
{
    SECURITY_STATUS status;
    SecPkgInfo *sec_pkg_info = NULL;

    status = sspi->QuerySecurityPackageInfo(auth_pkg, 
                                            &sec_pkg_info);
    if (status == SEC_E_OK) 
      {
        *maxtokensize = sec_pkg_info->cbMaxToken;
        sspi->FreeContextBuffer(sec_pkg_info);
      }
    else
      return svn_error_createf
        (SVN_ERR_RA_SERF_SSPI_INITIALISATION_FAILED, NULL,
         "SSPI Initialization failed.");
  return SVN_NO_ERROR;
}

svn_error_t *
init_sspi_connection(svn_ra_serf__session_t *session,
                     svn_ra_serf__connection_t *conn,
                     apr_pool_t *pool)
{
  load_security_dll();

  conn->sspi_context = (serf_sspi_context_t*)
    apr_palloc(pool, sizeof(serf_sspi_context_t));
  conn->sspi_context->ctx.dwLower = 0;
  conn->sspi_context->ctx.dwUpper = 0;
  conn->auth_header = NULL;
  conn->auth_value = NULL;

  return SVN_NO_ERROR;
}

svn_error_t *
handle_sspi_auth(svn_ra_serf__session_t *session,
                 svn_ra_serf__connection_t *conn,
                 serf_request_t *request,
                 serf_bucket_t *response,
                 char *auth_hdr,
                 char *auth_attr,
                 apr_pool_t *pool)
{
  const char *tmp;
  char *base64_token, *token = NULL, *last;
  apr_size_t tmp_len, encoded_len, token_len = 0;

  base64_token = apr_strtok(auth_attr, " ", &last);
  if (base64_token)
    {
      token_len = apr_base64_decode_len(base64_token);
      token = apr_palloc(pool, token_len);
      apr_base64_decode(token, base64_token);
    }

  SVN_ERR(sspi_get_credentials(token, token_len, &tmp, &tmp_len,
                               conn->sspi_context));

  encoded_len = apr_base64_encode_len(tmp_len);

  session->auth_value = apr_palloc(session->pool, encoded_len + 5);

  apr_cpystrn(session->auth_value, "NTLM ", 6);

  apr_base64_encode(&session->auth_value[5], tmp, tmp_len);

  session->auth_header = "Authorization";

  conn->auth_header = session->auth_header;
  conn->auth_value = session->auth_value;

  return SVN_NO_ERROR;
}

svn_error_t *
sspi_get_credentials(char *token, apr_size_t token_len, const char **buf, 
                     apr_size_t *buf_len, serf_sspi_context_t *sspi_ctx)
{
  SecBuffer in_buf, out_buf;
  SecBufferDesc in_buf_desc, out_buf_desc;
  SECURITY_STATUS status;
  DWORD ctx_attr;
  TimeStamp expires;
  CredHandle creds;
  char *target = NULL;
  CtxtHandle *ctx = &(sspi_ctx->ctx);

  if (ntlm_maxtokensize == 0)
    sspi_maxtokensize("NTLM", &ntlm_maxtokensize);
  /* Prepare inbound buffer. */
  in_buf.BufferType = SECBUFFER_TOKEN;
  in_buf.cbBuffer   = token_len;
  in_buf.pvBuffer   = token;
  in_buf_desc.cBuffers  = 1;
  in_buf_desc.ulVersion = SECBUFFER_VERSION;
  in_buf_desc.pBuffers  = &in_buf;

  /* Prepare outbound buffer. */
  out_buf.BufferType = SECBUFFER_TOKEN;
  out_buf.cbBuffer   = ntlm_maxtokensize;
  out_buf.pvBuffer   = (char*)malloc(ntlm_maxtokensize);
  out_buf_desc.cBuffers  = 1;
  out_buf_desc.ulVersion = SECBUFFER_VERSION;
  out_buf_desc.pBuffers  = &out_buf;

  /* Try to accept the server token. */
  status = sspi->AcquireCredentialsHandle(NULL, /* current user */
                                          "NTLM",
                                          SECPKG_CRED_OUTBOUND,
                                          NULL, NULL,
                                          NULL, NULL,
                                          &creds,
                                          &expires);

  if (status != SEC_E_OK)
    return svn_error_createf
            (SVN_ERR_RA_SERF_SSPI_INITIALISATION_FAILED, NULL,
             "SSPI Initialization failed.");

  status = sspi->InitializeSecurityContext(&creds,
                                           ctx != NULL && ctx->dwLower != 0 
                                             ? ctx 
                                             : NULL,
                                           target,
                                           ISC_REQ_REPLAY_DETECT |
                                           ISC_REQ_SEQUENCE_DETECT |
                                           ISC_REQ_CONFIDENTIALITY |
                                           ISC_REQ_DELEGATE,
                                           0,
                                           SECURITY_NATIVE_DREP,
                                           &in_buf_desc,
                                           0,
                                           ctx,
                                           &out_buf_desc,
                                           &ctx_attr,
                                           &expires);

  /* Finish authentication if SSPI requires so. */
  if (status == SEC_I_COMPLETE_NEEDED
      || status == SEC_I_COMPLETE_AND_CONTINUE)
    {
      if (sspi->CompleteAuthToken != NULL)
        sspi->CompleteAuthToken(ctx, &out_buf_desc);
    }

  *buf = out_buf.pvBuffer;
  *buf_len = out_buf.cbBuffer;

  switch (status)
    {
      case SEC_E_OK:
      case SEC_I_COMPLETE_NEEDED:
          break;

      case SEC_I_CONTINUE_NEEDED:
      case SEC_I_COMPLETE_AND_CONTINUE:
          break;

      default:
          return svn_error_createf(SVN_ERR_AUTHN_FAILED, NULL,
                "Authentication failed with error 0x%x.", status);
    }

  return SVN_NO_ERROR;
}

#endif /* WIN32 */

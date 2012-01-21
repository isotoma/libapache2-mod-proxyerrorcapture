/* Copyright 2012 John Carr.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * mod_proxy_404.c: A better version of ProxyErrorOverrides
 *
 * Code inspired by mod_proxy, mod_xsendfile
 *
 * Installation:
 *     apxs2 -cia mod_proxy_404.c
 */

#include "apr.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_buckets.h"
#include "apr_file_io.h"

#include "apr_hash.h"
#define APR_WANT_IOVEC
#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "httpd.h"
#include "http_log.h"
#include "http_config.h"
#include "http_log.h"
#define CORE_PRIVATE
#include "http_request.h"
#include "http_core.h"
#include "util_filter.h"
#include "http_protocol.h"

module AP_MODULE_DECLARE_DATA proxy404_module;

typedef struct {
  int enabled[600];
} proxy404_config;

static void *proxy404_create_config(apr_pool_t *p, server_rec *s) {
  proxy404_config *newcfg;
  newcfg = (proxy404_config *) apr_pcalloc(p, sizeof(proxy404_config));
  return (void *) newcfg;
}

static apr_status_t ap_proxy404_output_filter(ap_filter_t *f, apr_bucket_brigade *in) {
  request_rec *r = f->r;
  apr_status_t rv = r->status;
  apr_bucket *e;

  // If this is a 200 or a 404 then allow it to continue on its merry way
  if (r->status == HTTP_OK || r->status == HTTP_NOT_FOUND) {
    ap_remove_output_filter(f);
    return ap_pass_brigade(f->next, in);
  }

  // Otherwise it's some kind of error
  // Drop everything as we are going to replace it with an ErrorDocument
  // FIXME: We could just try doing an ap_die() straight away, it just seems polite to wait.
  while (!APR_BRIGADE_EMPTY(in)) {
    e = APR_BRIGADE_FIRST(in);
    apr_bucket_delete(e);
  }
  r->eos_sent = 0;

  // If this filter returns an error and the r->status isn't 200 then 
  // Apache thinks its a recursive error and won't show us anything.
  r->status = HTTP_OK;

  // We ignored all the things, so these can't be valid!
  apr_table_unset(r->headers_out, "Content-Length");
  apr_table_unset(r->err_headers_out, "Content-Length");
  apr_table_unset(r->headers_out, "Content-Encoding");
  apr_table_unset(r->err_headers_out, "Content-Encoding");

  ap_remove_output_filter(f);

  // There are mixed indicators about whether this is needed:
  // ap_die(status, r);

  return rv;
}

static void ap_proxy404_insert_output_filter(request_rec *r) {
  ap_add_output_filter(
    "PROXY404",
    NULL,
    r,
    r->connection);
}

static void proxy404_register_hooks(apr_pool_t *p) {
  ap_register_output_filter(
    "PROXY404",
    ap_proxy404_output_filter,
    NULL,
    AP_FTYPE_CONTENT_SET
    );

  ap_hook_insert_filter(
    ap_proxy404_insert_output_filter,
    NULL,
    NULL,
    APR_HOOK_LAST + 1
    );
}

static const char *proxy404_set_capture(cmd_parms *parms, void *mconfig, const char *code, const char *val)
{
  proxy404_config *s_cfg = ap_get_module_config(
    parms->server->module_config, &proxy404_module);

  int icode = atoi(code);
  if (icode < 100 || icode > 600)
    return "Code must be between 100 and 600";

  if (strcasecmp(val, "on") == 0)
    s_cfg->enabled[icode] = 1;
  else if (strcasecmp(val, "off") == 0)
    s_cfg->enabled[icode] = 1;
  else
    return "Must be 'On' or 'Off'";

  return NULL;
}

static const command_rec proxy404_cmds[] = {
  AP_INIT_TAKE2(
    "ProxyErrorCapture",
    proxy404_set_capture,
    NULL,
    RSRC_CONF,
    "ProxyErrorCapture <STATUS_CODE> <On|Off>."
  ),
  {NULL}
};

module AP_MODULE_DECLARE_DATA proxy404_module = {
  STANDARD20_MODULE_STUFF,
  NULL,
  NULL,
  proxy404_create_config,
  NULL,
  proxy404_cmds,
  proxy404_register_hooks
};

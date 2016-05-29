
/*
 * $Id: client_side_rewrite.c,v 1.2.2.2 2008/07/21 20:18:50 hno Exp $
 *
 * DEBUG: section 33    Client-side Routines - URL Rewriter
 * AUTHOR: Duane Wessels; Adrian Chadd
 *
 * SQUID Web Proxy Cache          http://www.squid-cache.org/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from
 *  the Internet community; see the CONTRIBUTORS file for full
 *  details.   Many organizations have provided support for Squid's
 *  development; see the SPONSORS file for full details.  Squid is
 *  Copyrighted (C) 2001 by the Regents of the University of
 *  California; see the COPYRIGHT file for full details.  Squid
 *  incorporates software developed and/or copyrighted by other
 *  sources; see the CREDITS file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

#include "squid.h"


/* Local functions */

void
clientRedirectAccessCheckDone(int answer, void *data)
{
	clientHttpRequest *http = data;
	http->acl_checklist = NULL;
	if (answer == ACCESS_ALLOWED)
	{
		debug(28,2)("redirect because %s\n", AclMatchedName);
		redirectStart(http, clientRedirectDone, http);
	}
	else
		clientRedirectDone(http, NULL);
}

void
clientRedirectStart(clientHttpRequest * http)
{
	debug(33, 5) ("clientRedirectStart: '%s'\n", http->uri);
#if CC_FRAMEWORK
	cc_call_hook_func_http_before_redirect_read(http);
	cc_call_hook_func_private_http_before_redirect_for_billing(http); 
	cc_call_hook_func_http_before_redirect(http);	

	int ret = 0;
	ret = cc_call_hook_func_private_http_before_redirect(http);
	if (ret == 1)
		return;
#endif
	if (Config.Program.url_rewrite.command == NULL)
	{
		clientRedirectDone(http, NULL);
		return;
	}
#ifdef CC_FRAMEWORK
	acl_access * acl = cc_get_acl_access_by_token("redirector_access",http);
		if ( acl != NULL)
#else
	if (Config.accessList.url_rewrite)
#endif
	{
#ifdef CC_FRAMEWORK
		http->acl_checklist = cc_clientAclCheckListCreate("redirector_access",http);
#else
		http->acl_checklist = clientAclChecklistCreate(Config.accessList.url_rewrite, http);
#endif
		aclNBCheck(http->acl_checklist, clientRedirectAccessCheckDone, http);
	}
	else
	{
		redirectStart(http, clientRedirectDone, http);
	}
}

void
clientRedirectDone(void *data, char *result)
{
	clientHttpRequest *http = data;
	request_t *new_request = NULL;
	request_t *old_request = http->request;
	const char *urlgroup = http->conn->port->urlgroup;
	debug(33, 5) ("clientRedirectDone: '%s' result=%s\n", http->uri,
				  result ? result : "NULL");
	assert(http->redirect_state == REDIRECT_PENDING);
	http->redirect_state = REDIRECT_DONE;
	if (result)
	{
		http_status status;
		if (*result == '!')
		{
			char *t;
			if ((t = strchr(result + 1, '!')) != NULL)
			{
				urlgroup = result + 1;
				*t++ = '\0';
				result = t;
			}
			else
			{
				debug(33, 1) ("clientRedirectDone: bad input: %s\n", result);
			}
		}
		status = (http_status) atoi(result);
		if (status == HTTP_MOVED_PERMANENTLY
				|| status == HTTP_MOVED_TEMPORARILY
				|| status == HTTP_SEE_OTHER
				|| status == HTTP_TEMPORARY_REDIRECT) 
		{
			char *t = result;
			if ((t = strchr(result, ':')) != NULL)
			{
				http->redirect.status = status;
				http->redirect.location = xstrdup(t + 1);
				goto redirect_parsed;
			}
			else
			{
				debug(33, 1) ("clientRedirectDone: bad input: %s\n", result);
			}
		}
#ifdef CC_FRAMEWORK
		else if(status == HTTP_BAD_REQUEST
				|| status == HTTP_FORBIDDEN 
				|| status == HTTP_UNAUTHORIZED 
				|| status == HTTP_METHOD_NOT_ALLOWED
				|| status == HTTP_PROXY_AUTHENTICATION_REQUIRED)
		{       
			http->redirect.status = status; 
			goto redirect_parsed;
		} 
#endif
		else if (strcmp(result, http->uri))
			new_request = urlParse(old_request->method, result);
	}
/* added by jiangbo.tian for store_multi_url */
#ifdef CC_FRAMEWORK
	cc_call_hook_func_private_clientRedirectDone(&new_request,result,http);
#endif
	/*added end by jiangbo.tian */
redirect_parsed:
	if (new_request)
	{
		safe_free(http->uri);
		http->uri = xstrdup(urlCanonical(new_request));
#ifdef CC_FRAMEWORK
		if(!http->log_uri)
#endif
			http->log_uri = xstrdup(urlCanonicalClean(old_request));

		new_request->http_ver = old_request->http_ver;
		httpHeaderAppend(&new_request->header, &old_request->header);
		new_request->client_addr = old_request->client_addr;
		new_request->client_port = old_request->client_port;
#if FOLLOW_X_FORWARDED_FOR
		new_request->indirect_client_addr = old_request->indirect_client_addr;
#endif /* FOLLOW_X_FORWARDED_FOR */
		new_request->my_addr = old_request->my_addr;
		new_request->my_port = old_request->my_port;
		new_request->flags = old_request->flags;
		new_request->flags.redirected = 1;

		if (old_request->auth_user_request)
		{
			new_request->auth_user_request = old_request->auth_user_request;
			authenticateAuthUserRequestLock(new_request->auth_user_request);
		}
		if (old_request->body_reader)
		{
			new_request->body_reader = old_request->body_reader;
			new_request->body_reader_data = old_request->body_reader_data;
			old_request->body_reader = NULL;
			old_request->body_reader_data = NULL;
		}
		new_request->content_length = old_request->content_length;
		if (strBuf(old_request->extacl_log))
			new_request->extacl_log = stringDup(&old_request->extacl_log);
		if (old_request->extacl_user)
			new_request->extacl_user = xstrdup(old_request->extacl_user);
		if (old_request->extacl_passwd)
			new_request->extacl_passwd = xstrdup(old_request->extacl_passwd);
#ifdef CC_FRAMEWORK
		if(old_request->store_url)
                        new_request->store_url = xstrdup(old_request->store_url);
#endif
		requestUnlink(old_request);
		http->request = requestLink(new_request);
	}
	else
	{
		/* Don't mess with urlgroup on internal request */
		if (old_request->flags.internal)
			urlgroup = NULL;
	}
	safe_free(http->request->urlgroup);		/* only paranoia. should not happen */
	if (urlgroup && *urlgroup)
		http->request->urlgroup = xstrdup(urlgroup);

#ifdef CC_FRAMEWORK
		{
			/* this pos get right request, we can handle the received request */
			int ret;
			ret = cc_call_hook_func_http_req_read(http);

			ret = cc_call_hook_func_http_req_read_second(http);

			//only for ppliveVod
			if(cc_call_hook_func_http_private_req_read_second(http) == -1)
				return;
			cc_call_hook_func_http_req_read_three(http);

		}
        if (7 != cc_call_hook_func_store_url_rewrite(http))     // for mod_store_url_rewrite only
#endif
            clientStoreURLRewriteStart(http);
}

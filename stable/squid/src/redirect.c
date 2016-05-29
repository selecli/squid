
/*
 * $Id: redirect.c,v 1.100.2.1 2008/05/04 23:23:13 hno Exp $
 *
 * DEBUG: section 85   Store URL Redirector
 * AUTHOR: Adrian Chadd; based on redirect.c by Duane Wessels
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

typedef struct
{
#ifdef CC_FRAMEWORK

	 /* 
	  * Chinacache SquidDevTeam
	  * $Id: redirect.c 1129 2007-07-10 08:11:04Z alex $
	  * handle headers
	  */
	HttpHeader headers;
#endif	

	void *data;
	char *orig_url;
	struct in_addr client_addr;
	const char *client_ident;
	const char *method_s;
	RH *handler;
} redirectStateData;

static HLPCB redirectHandleReply;
static void redirectStateFree(redirectStateData * r);
static helper *redirectors = NULL;
static OBJH redirectStats;
static int n_bypassed = 0;
CBDATA_TYPE(redirectStateData);
#ifdef CC_FRAMEWORK
static RH redirectReloadDone;
void redirectReloadStart(void *unused);
char *reloadMark = "CC_REDIRECT_RELOAD\n";
char *reloadReplyMark = "CC_RED_RELOAD_OK";
#endif

static void
redirectHandleReply(void *data, char *reply)
{
	redirectStateData *r = data;
	int valid;
	char *t;
	debug(61, 5) ("redirectHandleReply: {%s}\n", reply ? reply : "<NULL>");
	if (reply)
	{
		if ((t = strchr(reply, ' ')))
			*t = '\0';
		if (*reply == '\0')
			reply = NULL;
	}
	valid = cbdataValid(r->data);
	cbdataUnlock(r->data);
	if (valid)
		r->handler(r->data, reply);
	redirectStateFree(r);
}

static void
redirectStateFree(redirectStateData * r)
{
	safe_free(r->orig_url);
	cbdataFree(r);
}

static void
redirectStats(StoreEntry * sentry)
{
	storeAppendPrintf(sentry, "Redirector Statistics:\n");
	helperStats(sentry, redirectors);
	if (Config.onoff.redirector_bypass)
		storeAppendPrintf(sentry, "\nNumber of requests bypassed "
						  "because all redirectors were busy: %d\n", n_bypassed);
}

/**** PUBLIC FUNCTIONS ****/

void
redirectStart(clientHttpRequest * http, RH * handler, void *data)
{
	ConnStateData *conn = http->conn;
	redirectStateData *r = NULL;
	const char *fqdn;
	char *urlgroup = conn->port->urlgroup;
#ifdef CC_FRAMEWORK
	char buf[20488];
#else
	char buf[8192];
#endif
	char claddr[20];
	char myaddr[20];
	assert(http);
	assert(handler);
	debug(61, 5) ("redirectStart: '%s'\n", http->uri);
	if (Config.onoff.redirector_bypass && redirectors->stats.queue_size)
	{
		/* Skip redirector if there is one request queued */
		n_bypassed++;
		handler(data, NULL);
		return;
	}
	r = cbdataAlloc(redirectStateData);
	r->orig_url = xstrdup(http->uri);
	r->client_addr = conn->log_addr;
	r->client_ident = NULL;
	if (http->request->auth_user_request)
		r->client_ident = authenticateUserRequestUsername(http->request->auth_user_request);
	else if (http->request->extacl_user)
	{
		r->client_ident = http->request->extacl_user;
	}
	if (!r->client_ident && conn->rfc931[0])
		r->client_ident = conn->rfc931;
#if USE_SSL
	if (!r->client_ident)
		r->client_ident = sslGetUserEmail(fd_table[conn->fd].ssl);
#endif
	if (!r->client_ident)
		r->client_ident = dash_str;
	r->method_s = RequestMethods[http->request->method].str;
	r->handler = handler;
	r->data = data;
	cbdataLock(r->data);
	if ((fqdn = fqdncache_gethostbyaddr(r->client_addr, 0)) == NULL)
		fqdn = dash_str;
	xstrncpy(claddr, inet_ntoa(r->client_addr), 20);
	xstrncpy(myaddr, inet_ntoa(http->request->my_addr), 20);
#ifdef CC_FRAMEWORK	
	 /* 
	  * Chinacache SquidDevTeam
	  * $Id: redirect.c 1129 2007-07-10 08:11:04Z alex $
	  * handle headers
	  */
	char headerbuf[20488] = "";
	const request_t *req = http->request;
	r->headers = req->header;
	const HttpHeaderEntry *e;
	HttpHeaderPos pos=HttpHeaderInitPos;
	while((e=httpHeaderGetEntry(&r->headers,&pos)))
	{
		strcat(headerbuf,e->name.buf);
		strcat(headerbuf,": ");
	    strcat(headerbuf, e->value.buf);
	    strcat(headerbuf, "\t");
	}

	snprintf(buf, 20480, "%s %s/%s %s %s %s myip=%s myport=%d\t%s" ,
			 r->orig_url,
			 claddr,
			 fqdn,
			 r->client_ident[0] ? rfc1738_escape(r->client_ident) : dash_str,
			 r->method_s,
			 urlgroup ? urlgroup : "-",
			 myaddr,
			 http->request->my_port,
			 headerbuf);
	/* end */
#else	
	snprintf(buf, 8191, "%s %s/%s %s %s %s myip=%s myport=%d",
			 r->orig_url,
			 claddr,
			 fqdn,
			 r->client_ident[0] ? rfc1738_escape(r->client_ident) : dash_str,
			 r->method_s,
			 urlgroup ? urlgroup : "-",
			 myaddr,
			 http->request->my_port);
#endif	
	debug(61, 6) ("redirectStart: sending '%s' to the helper\n", buf);
	strcat(buf, "\n");
	helperSubmit(redirectors, buf, redirectHandleReply, r);
}

#ifdef CC_FRAMEWORK
int redirectShouldReload()
{
	wordlist *tmp = Config_r.Program.url_rewrite.command;
	if (NULL == tmp && NULL == redirectors)
		return 0;
	if (NULL == tmp && NULL != redirectors)
		return 1;
	if (NULL != tmp && NULL == redirectors)
		return 1;

	if (redirectors->n_active < redirectors->n_to_start)
		return 1;
	if (redirectors->n_to_start != Config_r.Program.url_rewrite.children)
		return 1;
	if (redirectors->concurrency != Config_r.Program.url_rewrite.concurrency)
		return 1;

	wordlist *tmp2 = Config.Program.url_rewrite.command;
	while (tmp && tmp2) {
		if (strcmp(tmp->key,tmp2->key)) 
			return 1;
		if (strchr(tmp->key,'/')) {
			struct stat st;
			if(stat(tmp->key,&st) != 0)
				fatal("redirect program or configfile does not exist\n");


			if (strstr(tmp->key,"redirect.conf")) {
				if(redirectors->config_size != st.st_size)
					return 1;
				if(st.st_size == 0)
					return 0;
			}

			if(st.st_mtime >redirectors->last_change)
				return 1;
		}
		tmp = tmp->next;
		tmp2 = tmp2->next;
	}

	if (tmp || tmp2) 
		return 1;
	return 0;
}

/*
 * @@Func:  Judeg whether debug level of redirect changed or not
 * @@ ret:
 * 			1: changed
 * 			0: not changed
 * 
 */
int redirectDebugLevelChanged()
{
	wordlist *newComm = Config_r.Program.url_rewrite.command;
	wordlist *oldComm = Config.Program.url_rewrite.command;

	if (newComm && oldComm){
		while (newComm && oldComm){
			newComm = newComm->next;
			oldComm = oldComm->next;
		}			
		
		if (newComm || oldComm){
			debug(61, 1) ("redirectDebugLevelChanged: redirect debug level changed!\n");
			return 1;
		}			
		
		debug(61, 1) ("redirectDebugLevelChanged: redirect debug level haven`t changed!\n");
		return 0;
	}
	
}



/*
 * @@Func:   When redirectors should reload, we check whether should restart them.
 */
int redirectShouldRestart()
{
	if(NULL != redirectors)
	{
		if ((redirectors->n_active != redirectors->n_to_start)||
				(redirectors->n_to_start != Config_r.Program.url_rewrite.children) ||
				(redirectors->concurrency != Config_r.Program.url_rewrite.concurrency) ||
				redirectDebugLevelChanged() ) {
			debug(61, 1) ("redirectShouldRestart: redirectors should restart!\n");
			return 1;
		}
	}
	return 0;
}
#endif

void
redirectInit(void)
{
	static int init = 0;
	int i;
	if (!Config.Program.url_rewrite.command)
		return;
	if (redirectors == NULL)
		redirectors = helperCreate("url_rewriter");
	
#ifdef CC_FRAMEWORK
	struct stat sb;
	wordlist *tmp = Config.Program.url_rewrite.command;
	while(tmp) {
		if(strchr(tmp->key,'/')) {
			if(stat(tmp->key,&sb) == 0) {
				redirectors->last_change=redirectors->last_change > sb.st_mtime ?redirectors->last_change : sb.st_mtime;
				if(strstr(tmp->key,"redirect.conf"))
					redirectors->config_size = sb.st_size;
			}
		}
		tmp= tmp->next;
	}
	
	
	if (do_reconfigure) {	
													//if we don`t need to reload redirect
		if ((!redirectShouldReload()) && (redirectors != NULL)) {
			redirectors->cmdline = Config.Program.url_rewrite.command;
			redirectors->n_to_start = Config.Program.url_rewrite.children;
			redirectors->concurrency = Config.Program.url_rewrite.concurrency;
			redirectors->ipc_type = IPC_STREAM;
			return;
		}	
		else									     //should reload and numbers of helper changed
		{
			if (!redirectShouldRestart()) {
				debug(61,1) ("redirectInit: Just reconfigure url_rewriters, don`t re-open helpers\n");
				return;													
			}
		}
	}
	// added by chenqi
	// to save cpu resource, do not start redirect if redirect.conf is empty
	/*
	if (redirectors->config_size == 0) {
		int i = 0; 
		access_array *aa ,*bb;
		for(i = 0; i < 1000; i++) {    
			aa = Config.accessList2[i];
			while (aa) {    
				bb = aa->next;
				aclDestroyAccessList(&aa->url_rewrite);
				aa = bb;
			}
		}

		wordlistDestroy(&Config.Program.url_rewrite.command);
		helperFree(redirectors);
		debug(61,1)("Warning: redirect.conf is empty, do not start redirect process\n");
		return;
	}
	*/
	// add end
#endif

	redirectors->cmdline = Config.Program.url_rewrite.command;
	redirectors->n_to_start = Config.Program.url_rewrite.children;
	redirectors->concurrency = Config.Program.url_rewrite.concurrency;
	redirectors->ipc_type = IPC_STREAM;
	helperOpenServers(redirectors);
	if (!init)
	{
		cachemgrRegister("url_rewriter",
						 "URL Rewriter Stats",
						 redirectStats, 0, 1);
		init = 1;
		CBDATA_INIT_TYPE(redirectStateData);
	}
}


#ifdef CC_FRAMEWORK
/*
 * @@func: According to the relply, set the flag
 */
static void
redirectReloadDone(void *data, char *reply)
{
	helper_server * srv = data;

	if (NULL == reply)
		return;
	debug(61, 3) ("redirectReloadDone: reply is [%s]\n", reply);
	if (!strncmp(reply, reloadReplyMark, strlen(reloadReplyMark))) {
		srv->flags.reload_stats = REDIRECT_RELOAD_DONE;
		eventAdd("redirectReloadStart", redirectReloadStart, NULL, 0.01, 0);	
		debug(61, 3) ( "redirectReloadDone: url_rewriter  #%d reload success!\n",srv->index+ 1);
	}
	else {
		srv->flags.reload_stats = REDIRECT_TO_RELOAD;
		debug(61, 3) ("redirectReloadDone: url_rewriter  #%d reload failed!\n", srv->index + 1);
	}

}

/*
 * @@func: submit reload task to a certain helper server process
 */
static void
helperSubmitCertain(helper_server* srv, const char *buf, HLPCB * callback, void *data)
{
	helper_request *req = memAllocate(MEM_HELPER_REQUEST);
	if (srv == NULL) {
		debug(61, 3) ("helperSubmitCertain: srv == NULL\n");
		callback(data, NULL);
		return;
	}
	req->callback = callback;
	req->data = data;
	req->buf = xstrdup(buf);
	cbdataLock(req->data);
	debug(61,3) ("helperSubmitCertain: cbdataLock req->data = %p\n", req->data);
	helperDispatch(srv, req);
	debug(61, 3) ("helperSubmitCertain: %s\n", buf);
}


/*
 * @@func: Looply add task to redirectors 
 */
void
redirectReloadStart(void *unused)
{
	static dlink_node *n = NULL;							// record the number of helper server
	helper_server *srv;

	redirectStateData *r = NULL;

	if (NULL == n)
		n = redirectors->servers.head;

	srv = n->data;	

	switch (srv->flags.reload_stats) {

		//need to reload or fullload
		case REDIRECT_TO_RELOAD:
		case REDIRECT_TO_RELOAD_FULLLOAD:

			debug(61,1) ("redirectReloadStart: reloading url_rewriter #%d,\tflag is [%d]\n", 
					srv->index +1, srv->flags.reload_stats );
			//checkup helper_server`s load
			helper* hlp = srv->parent;
			int slot = -1;
			int flag = 1;
			for (slot = 0; slot < (hlp->concurrency ? hlp->concurrency : 1); slot++) {
				if (!srv->requests[slot]) {						//There is idle server
					r = cbdataAlloc(redirectStateData);
					r->handler = redirectReloadDone;								  
					r->data = srv;
					cbdataLock(r->data);
					helperSubmitCertain(srv, reloadMark, redirectHandleReply, r );
					srv->flags.reload_stats = REDIRECT_RELOADING;
					flag = 0;
					break;
				}
			}
			if (flag) {
				srv->flags.reload_stats = REDIRECT_TO_RELOAD_FULLLOAD;
				eventAdd("redirectReloadStart", redirectReloadStart, NULL, 0.01, 0);	
			}
			break;

/*			//reloading 
		case REDIRECT_RELOADING:
			break;
*/
			//reload done
		case REDIRECT_RELOAD_DONE:
			srv->flags.reload_stats = REDIRECT_TO_RELOAD;
			n = n->next;
			if (NULL == n)
				debug(61, 1) ("redirectReloadStart: Congratulations! All the url_rewriters are reloaded!\n");
			else
				eventAdd("redirectReloadStart", redirectReloadStart, NULL, 0.01, 0);	

			break;

		default:
			debug(61, 1) ("redirectReloadStart: Something Error. Should not see this one.\n");
			break;
	}

	return;
}
#endif

void
redirectShutdown(void)
{
	if (!redirectors)
		return;
#ifdef CC_FRAMEWORK
	if (do_reconfigure) {                     	//Checkup  the occasion is reload or not 
		if (!redirectShouldReload())		
			return;			
		else {
			// If the number of url_rewriters don`t change, we just reconfigure helper server
			if (!redirectShouldRestart()) {
				debug(61, 1) ("redirectShutdown: Just reconfigure redirectors ,don`t shutdown\n");
				return;								
			}
		}
	}	
#endif
	helperShutdown(redirectors);
	if (!shutting_down)
		return;
	helperFree(redirectors);
	redirectors = NULL;
}

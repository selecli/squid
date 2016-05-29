#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

typedef struct _mod_config
{
	int hlpchildren;
	int hlpconcurrency;
	wordlist * hlpcmdline;
} mod_config;
typedef struct _url_checksignStateData
{
	void * data;
	RH * handler;
} url_checksignStateData;

static cc_module *mod = NULL; 
static helper * url_checksign = NULL; 
static MemPool * mod_config_pool = NULL; 
CBDATA_TYPE(url_checksignStateData);

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == mod_config_pool)
		mod_config_pool = memPoolCreate("mod_url_checksign config_struct", sizeof(mod_config));
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_cfg(void * data) 
{
	mod_config * cfg = data; 
	if (NULL != cfg) 
	{       
		wordlistDestroy(&cfg->hlpcmdline);
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL; 
	}       
	return 0;
}

static void parse_programline(wordlist ** line)
{
	if (*line)
		self_destruct();
	parse_wordlist(line);
}

static int func_sys_parse_param(char *cfg_line)
{
	debug(154,1)("mod_url_checksign: config is %s\n", cfg_line);
	mod_config * cfg = NULL;
	char *token = NULL;
	char cmd[256];
	wordlist * line  = NULL;

	char *p = strstr(cfg_line, "allow");
	if (!p)
		goto err;
	*p = 0;

	if (NULL == (token = strtok(cfg_line, w_space)))
		goto err;
	else if (0 != strcmp("mod_url_checksign", token))
		goto err;

	cfg = (mod_config *)mod_config_pool_alloc();

	if (NULL == (token = strtok(NULL, w_space)))
		goto err;
	else if ((cfg->hlpchildren = atoi(token)) < 0)
		goto err;

	if (NULL == (token = strtok(NULL, w_space)))
		goto err;
	else if ((cfg->hlpconcurrency = atoi(token)) < 0)
		goto err;

	//some no use
	token = strtok(NULL, w_space);
	strcat(cmd, "a ");
	strcat(cmd, token);
	token = strtok(cmd, w_space);
	//how to send data to line??
	parse_programline(&line);
	if (NULL != line)
		cfg->hlpcmdline = line;
	else
		goto err;
	debug(154, 1) ("mod_url_checksign: param hlpchildren = %d hlpconcurrency = %d  hlpcmdline = %s\n", cfg->hlpchildren, cfg->hlpconcurrency, cfg->hlpcmdline->key);
	cc_register_mod_param(mod, cfg, free_cfg);
	return 0;
err:
	if (NULL != cfg)
		free_cfg(cfg);
	debug(154, 1)("mod_url_checksign : error occured when parsing cfg line\n");
	return -1;
}

static void func_url_checksignShutdown()
{
	if (NULL != url_checksign)
	{
		helperShutdown(url_checksign);
		helperFree(url_checksign);
		url_checksign = NULL;
		debug(154, 3)("clean up previous url_checksign when reconfiguring\n");
	}
}

static void url_checksignInit(clientHttpRequest * http)
{
	mod_config *cfg = (mod_config*)cc_get_mod_param(http->conn->fd, mod);

	if (NULL == url_checksign)
	{
		url_checksign = helperCreate("url_checksign");
		url_checksign->cmdline = cfg->hlpcmdline;
		url_checksign->n_to_start = cfg->hlpchildren;
		url_checksign->concurrency = cfg->hlpconcurrency;
		url_checksign->ipc_type = IPC_STREAM;
		helperOpenServers(url_checksign);
		CBDATA_INIT_TYPE(url_checksignStateData);
		debug(154, 3)("Init url_checksign with the config params as the first request coming\n");
	}
}

static void url_checksignHandleReply(void *data, char *reply)
{
	url_checksignStateData * state = data;
	int valid;
	char *t;
	debug(154, 5) ("url_checksignHandleReply: {%s}\n", reply ? reply : "<NULL>");
	if (reply) 
	{       
		if ((t = strchr(reply, '$')))
			*t = '\0'; 
		if (*reply == '\0')
			reply = NULL; 
	}       
	valid = cbdataValid(state->data);
	cbdataUnlock(state->data);
	if (valid) 
		state->handler(state->data, reply); 
	cbdataFree(state);
	return;
}

static void modifyStoreUrl(clientHttpRequest * http)
{
	char strNewStoreUrl[4096];
	memset(strNewStoreUrl, '\0', sizeof(strNewStoreUrl));
	char * uri = http->uri;
	char * pPos1 = NULL;
	char * pPos2 = NULL;
	char * pPos3 = NULL;
	char * pPos4 = NULL;
	pPos1 = strstr(uri, "sf-server/file/");
	pPos1 += strlen("sf-server/file/");
	pPos2 = strstr(pPos1, "/");
	memcpy(strNewStoreUrl, uri, pPos2 - uri);
	pPos3 = strstr(pPos2 + 1, "/");
	if (NULL == pPos3)
	{
	}
	else
	{
		pPos4 = strstr(pPos3, "?");
		if (pPos4) {
			int len = strlen(strNewStoreUrl);
			memcpy(strNewStoreUrl + len, pPos3, pPos4-pPos3);
		} else
			strcat(strNewStoreUrl,pPos3);
	}
	if (http->request->store_url)
		xfree(http->request->store_url);
	http->request->store_url = xstrdup(strNewStoreUrl);
	int len2 = strlen(strNewStoreUrl);
	http->request->store_url[len2] = '\0';
}

static void url_checksignDone(void * data, char *reply)
{
	clientHttpRequest * http = data;
	if (NULL != strstr(reply, "200"))
	{
		modifyStoreUrl(http);
		clientCheckFollowXForwardedFor(http);
	}
	else
	{
		int nCode = 403;
		char* pPos = strstr(reply, "HTTP/1.1");
		if(!pPos)
		{
			pPos = strstr(reply, "HTTP/1.0");
		}
		if(NULL != pPos)
		{
			pPos += strlen("HTTP/1.1 ");
			nCode = atoi(pPos);
		}
		ErrorState *err = errorCon(ERR_ACCESS_DENIED, nCode, http->request);
		http->log_type = LOG_TCP_MISS;
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		errorAppendEntry(http->entry, err);
		debug(154, 3)("do not auth successfully, so forbid!\n");
	}
}

/*
static int preProcessRequest_NetEasy(clientHttpRequest* http, char* newurl, char* headerbuf)
{
	char *httpuri;
	const HttpHeaderEntry *e = NULL;
	HttpHeaderPos pos=HttpHeaderInitPos;
	const request_t *req = http->orig_request;
	httpuri = xstrdup(urlCanonical(http->orig_request));
	struct in_addr addr;
	int flag = 0;
	const char *key = NULL;

	
	if (strncmp(httpuri, "http://file.m.163.com/app/vip/",30))
	{
		debug(154, 5)("do not be a vip request to AUTH\n");
		safe_free(httpuri);
		return 4;
	}

	while((e=httpHeaderGetEntry(&req->header,&pos)))
	{
		if (e->id == HDR_COOKIE)
			if (NULL != strstr(e->value.buf, "NTES_PASSPORT")|| NULL != strstr(e->value.buf, "NTES_SESS"))
					flag = 1;
		if (e->id == HDR_HOST)
			continue;
		strcat(headerbuf,e->name.buf);
		strcat(headerbuf,":");
		strcat(headerbuf, e->value.buf);
		strcat(headerbuf, "\t");
	}
	if (flag == 0)
	{
		safe_free(httpuri);
		ErrorState *err = errorCon(ERR_ACCESS_DENIED, HTTP_FORBIDDEN, http->request);
		http->log_type = LOG_TCP_MISS;
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		errorAppendEntry(http->entry, err);
		debug(154, 3)("request is forbiddened as there is no cookie header or no NTES_PASSPORT and NTES_SESS fields in cookie header\n");
		return 3;
	}

	if (httpHeaderHas(&req->header, HDR_X_FORWARDED_FOR))
	{
		String s = httpHeaderGetList(&http->request->header, HDR_X_FORWARDED_FOR);
		if (strLen(s))
		{
			const char * p = strBuf(s);
			int l = 0;
			while (p[l] == ',' || xisspace(p[l]))
				l++;
			key = &p[l];
			while (!(p[l] == ',' || xisspace(p[l])) && p[l] != '\0')
				l++;
			strCut(s, l);
			if (inet_aton(key, &addr) == 0)	
				key = NULL;
		}
	}
	if (NULL == key)
		key = xinet_ntoa(http->conn->peer.sin_addr);

	snprintf(newurl, MAX_URL+55, "%s?url=%s&ip=%s","http://check.m.163.com/checking", httpuri, key);
	safe_free(httpuri);
	char * c = strstr(newurl, "&ip=");
	if (NULL == c || inet_aton(c+4, &addr) == 0)
	{
		ErrorState *err = errorCon(ERR_ACCESS_DENIED, HTTP_FORBIDDEN, http->request);
		http->log_type = LOG_TCP_MISS;
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		errorAppendEntry(http->entry, err);
		debug(154, 1)("the length of url is longer than MAX_URL, request is forbiddened\n");
		return 3;
	}
	return 0;
}

*/

static int preProcessRequest_Infinitus(clientHttpRequest* http, char* newurl, char* headerbuf)
{
	char *httpuri;
	const HttpHeaderEntry *e = NULL;
	HttpHeaderPos pos=HttpHeaderInitPos;
	const request_t *req = http->orig_request;
	httpuri = xstrdup(urlCanonical(http->orig_request));

	
	if (NULL == strstr(httpuri, "sf-server/file/downloadFile/") && NULL == strstr(httpuri, "sf-server/file/getFile/") && NULL == strstr(httpuri, "sf-server/file/getScormFile"))
	{
		debug(154, 5)("do not be a right url for checksign: Infinitus\n");
		safe_free(httpuri);
		return 4;
	}

	while((e=httpHeaderGetEntry(&req->header,&pos)))
	{
		strcat(headerbuf,e->name.buf);
		strcat(headerbuf,":");
		strcat(headerbuf, e->value.buf);
		strcat(headerbuf, "\t");
	}
	
	char* pPos1 = strstr(httpuri, "sf-server/file/");
	int nLen = pPos1 - httpuri + strlen("sf-server/file/");
	memcpy(newurl, httpuri, nLen);
	strcat(newurl, "checkSign/");
	strcat(newurl, httpuri + nLen);
	safe_free(httpuri);
	return 0;
}

/*
 * This func has two return values , 2 and 3.
 * 2 means the request will submit for AUTH,  processing of this 
 * request will not start until geting a AUTH result.
 * 3 means the request should not be AUTHed,and ought to process it
 * continually.
 */
static int func_urlSubmitForAuth(clientHttpRequest * http)
{
	char newurl[MAX_URL+55] = "";
	char buf[24584];
	char headerbuf[20488] = "";
	                 
	int nRet = preProcessRequest_Infinitus(http, newurl, headerbuf);
	if(0 != nRet)
		return nRet;
	url_checksignInit(http);
	url_checksignStateData * state;
	state = cbdataAlloc(url_checksignStateData);
	state->data = http;
	state->handler = url_checksignDone;
	cbdataLock(state->data);
	snprintf(buf, 24584, "%s####%s\n", newurl, headerbuf);
	helperSubmit(url_checksign, buf, url_checksignHandleReply, state);
	debug(154, 3)("auth url \"%s\" was submited\n", buf);
	return 2;
}

static int hook_cleanup(cc_module *module)
{
	debug(154, 1)("(mod_url_checksign) ->     hook_cleanup:\n");
	if (NULL != url_checksign)
	{
		helperShutdown(url_checksign);
		helperFree(url_checksign);
		url_checksign = NULL;
	}
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}

int mod_register(cc_module *module)
{
	debug(154, 1)("(mod_url_checksign) ->      mod_register:\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);

	//modified by fangfang.yang for case 4807, modified the hook point of func_url_checksignShutdown
	//module->hook_func_before_sys_init = func_url_checksignShutdown;
	cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
			func_url_checksignShutdown);
	//module->hook_func_before_sys_init = func_url_checksignShutdown;
	//cc_register_hook_handler(HPIDX_hook_func_module_reconfigure,
	//		module->slot,
	//		(void **)ADDR_OF_HOOK_FUNC(module, hook_func_module_reconfigure),
	//		func_url_checksignShutdown);

	//module->hook_func_http_repl_send_start = func_url_checksignShutdown; 
	cc_register_hook_handler(HPIDX_hook_func_http_req_parsed,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_parsed),
			func_urlSubmitForAuth);

	//mod = module;
        if(reconfigure_in_thread)
                mod = (cc_module*)cc_modules.items[module->slot];
        else
                mod = module;
	return 0;
}
#endif

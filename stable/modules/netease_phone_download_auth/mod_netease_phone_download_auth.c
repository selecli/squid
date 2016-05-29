#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

typedef struct _mod_config
{
	int hlpchildren;
	int hlpconcurrency;
	wordlist * hlpcmdline;
} mod_config;
typedef struct _neteasedownloadauthStateData
{
	void * data;
	RH * handler;
} neteasedownloadauthStateData;

static cc_module *mod = NULL; 
static helper * neteasedownloadauth = NULL; 
static MemPool * mod_config_pool = NULL; 
CBDATA_TYPE(neteasedownloadauthStateData);

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == mod_config_pool)
		mod_config_pool = memPoolCreate("mod_netease_phone_download_auth config_struct", sizeof(mod_config));
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
	debug(154,1)("mod_netease_phone_download_auth: config is %s\n", cfg_line);
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
	else if (0 != strcmp("mod_netease_phone_download_auth", token))
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

	token = strtok(NULL, w_space);
	strcat(cmd, "a ");
	strcat(cmd, token);
	token = strtok(cmd, w_space);
	parse_programline(&line);
	if (NULL != line)
		cfg->hlpcmdline = line;
	else
		goto err;
	debug(154, 1) ("mod_netease_phone_download_auth: param hlpchildren = %d hlpconcurrency = %d  hlpcmdline = %s\n", cfg->hlpchildren, cfg->hlpconcurrency, cfg->hlpcmdline->key);
	cc_register_mod_param(mod, cfg, free_cfg);
	return 0;
err:
	if (NULL != cfg)
		free_cfg(cfg);
	debug(154, 1)("mod_netease_phone_download_auth : error occured when parsing cfg line\n");
	return -1;
}

static void func_neteasedownloadauthShutdown()
{
	if (NULL != neteasedownloadauth)
	{
		helperShutdown(neteasedownloadauth);
		helperFree(neteasedownloadauth);
		neteasedownloadauth = NULL;
		debug(154, 3)("clean up previous neteasedownloadauth when reconfiguring\n");
	}
}

static void neteasedownloadauthInit(clientHttpRequest * http)
{
	mod_config *cfg = (mod_config*)cc_get_mod_param(http->conn->fd, mod);

	if (NULL == neteasedownloadauth)
	{
		neteasedownloadauth = helperCreate("neteasedownloadauth");
		neteasedownloadauth->cmdline = cfg->hlpcmdline;
		neteasedownloadauth->n_to_start = cfg->hlpchildren;
		neteasedownloadauth->concurrency = cfg->hlpconcurrency;
		neteasedownloadauth->ipc_type = IPC_STREAM;
		helperOpenServers(neteasedownloadauth);
		CBDATA_INIT_TYPE(neteasedownloadauthStateData);
		debug(154, 3)("Init neteasedownloadauth with the config params as the first request coming\n");
	}
}

static void neteasedownloadauthHandleReply(void *data, char *reply)
{
	neteasedownloadauthStateData * state = data;
	int valid;
	char *t;
	debug(154, 5) ("neteasedownloadauthHandleReply: {%s}\n", reply ? reply : "<NULL>");
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

static void neteasedownloadauthDone(void * data, char *reply)
{
	clientHttpRequest * http = data;
	if (NULL == strstr(reply, "200"))
	{
		ErrorState *err = errorCon(ERR_ACCESS_DENIED, HTTP_FORBIDDEN, http->request);
		http->log_type = LOG_TCP_MISS;
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		errorAppendEntry(http->entry, err);
		debug(154, 3)("do not auth successfully, so forbid!\n");
	}
	else
		clientCheckFollowXForwardedFor(http);
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
	char *httpuri;
	char newurl[MAX_URL+55];
	char buf[24584];
	const HttpHeaderEntry *e = NULL;
	char headerbuf[20488] = "";
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
	                 
	neteasedownloadauthInit(http);
	neteasedownloadauthStateData * state;
	state = cbdataAlloc(neteasedownloadauthStateData);
	state->data = http;
	state->handler = neteasedownloadauthDone;
	cbdataLock(state->data);
	snprintf(buf, 24584, "%s####%s\n", newurl, headerbuf);
	helperSubmit(neteasedownloadauth, buf, neteasedownloadauthHandleReply, state);
	debug(154, 3)("auth url \"%s\" was submited\n", buf);
	return 2;
}

static int hook_cleanup(cc_module *module)
{
	debug(154, 1)("(mod_netease_phone_download_auth) ->     hook_cleanup:\n");
	if (NULL != neteasedownloadauth)
	{
		helperShutdown(neteasedownloadauth);
		helperFree(neteasedownloadauth);
		neteasedownloadauth = NULL;
	}
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}

int mod_register(cc_module *module)
{
	debug(154, 1)("(mod_netease_phone_download_auth) ->      mod_register:\n");

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

	//module->hook_func_before_sys_init = func_neteasedownloadauthShutdown;
	cc_register_hook_handler(HPIDX_hook_func_module_reconfigure,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_module_reconfigure),
			func_neteasedownloadauthShutdown);

	//module->hook_func_http_repl_send_start = func_neteasedownloadauthShutdown; 
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

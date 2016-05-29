#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

typedef struct _mod_config
{
	int hlpchildren;
	int hlpconcurrency;
	wordlist * hlpcmdline;
} mod_config;

typedef struct _token_authStateData
{
	void * data;
	RH * handler;
} token_authStateData;

static cc_module *mod = NULL; 
static helper * token_auth = NULL; 
static MemPool * mod_config_pool = NULL; 
CBDATA_TYPE(token_authStateData);

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == mod_config_pool)
		mod_config_pool = memPoolCreate("mod_token_auth config_struct", sizeof(mod_config));
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
	debug(156,1)("mod_token_auth: config is %s\n", cfg_line);
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
	else if (0 != strcmp("mod_token_auth", token))
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
	debug(222, 1)("mod_token_auth: param hlpchildren = %d hlpconcurrency = %d  hlpcmdline = %s\n", cfg->hlpchildren, cfg->hlpconcurrency, cfg->hlpcmdline->key);
	cc_register_mod_param(mod, cfg, free_cfg);
	return 0;
err:
	if (NULL != cfg)
		free_cfg(cfg);
	debug(222, 1)("mod_token_auth: error occured when parsing cfg line\n");
	return -1;
}

static void func_token_auth_shutdown()
{
	if (NULL != token_auth)
	{
		helperShutdown(token_auth);
		helperFree(token_auth);
		token_auth = NULL;
		debug(222, 3)("mod_token_auth: clean up previous token_auth when reconfiguring\n");
	}
}

static void token_authInit(clientHttpRequest * http)
{
	mod_config *cfg = (mod_config*)cc_get_mod_param(http->conn->fd, mod);

	if (NULL == token_auth)
	{
		token_auth = helperCreate("token_auth");
		token_auth->cmdline = cfg->hlpcmdline;
		token_auth->n_to_start = cfg->hlpchildren;
		token_auth->concurrency = cfg->hlpconcurrency;
		token_auth->ipc_type = IPC_STREAM;
		helperOpenServers(token_auth);
		CBDATA_INIT_TYPE(token_authStateData);
		debug(222, 3)("Init token_auth with the config params as the first request coming\n");
	}
}

static void token_authHandleReply(void *data, char *reply)
{
	token_authStateData * state = data;
	int valid;
	debug(222, 5) ("token_authHandleReply: {%s}\n", reply ? reply : "<NULL>");
	if (reply) 
	{       
		if (*reply == '\0')
			reply = NULL; 
	}       
	valid = cbdataValid(state->data);
	cbdataUnlock(state->data);
	if (valid) {
		state->handler(state->data, reply); 
		debug(222, 3)("mod_token_auth: reply --> %s\n", reply);
	} else {
		debug(222, 3)("mod_token_auth: valid is NULL\n");
	}
	cbdataFree(state);
	return;
}

static void token_authDone(void * data, char *reply)
{
	debug(222, 3)("mod_token_auth: token_authDone reply --> %s\n", reply);
	char *p = NULL, *tmp = NULL;
	clientHttpRequest * http = data;
	if (NULL == (p = strstr(reply, "200")))
	{
		ErrorState *err = errorCon(ERR_ACCESS_DENIED, HTTP_UNAUTHORIZED, http->request);
		http->log_type = LOG_TCP_MISS;
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		errorAppendEntry(http->entry, err);
		debug(222, 3)("mod_token_auth: do not auth successfully, so forbid!\n");
	}
	else {
		p += 3;
		while (' ' == *p || '\t' == *p) {
			++p;
		}   
		if (NULL != (tmp = strrchr(p, '\n'))) {
			*tmp = '\0';
		}
		debug(222, 3)("mod_token_auth: p--> %s!\n", p);
		//strcpy(download_url, p);
		xfree(http->uri);
		http->uri = xstrdup(p);
		debug(222, 3)("http->uri = %s\n",http->uri);

		request_t *new_request = NULL;
		request_t *old_request = http->request;
		const char *urlgroup = http->conn->port->urlgroup;

		new_request = urlParse(old_request->method, http->uri);
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
			requestUnlink(old_request);
			http->request = requestLink(new_request);
		}
		else
		{
			/* Don't mess with urlgroup on internal request */
			if (old_request->flags.internal)
				urlgroup = NULL;
		}
		safe_free(http->request->urlgroup);     /* only paranoia. should not happen */
		//xfree(http->orig_request);
		//http->orig_request = xstrdup(p);
		clientCheckFollowXForwardedFor(http);
	}
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
	char buf[24584];
	httpuri = xstrdup(urlCanonical(http->orig_request));

	/*
HTTP_UNAUTHORIZED:
ErrorState *err = errorCon(ERR_ACCESS_DENIED, HTTP_UNAUTHORIZED, http->request);
http->log_type = LOG_TCP_MISS;
http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
errorAppendEntry(http->entry, err);
debug(222, 1)("the length of url is longer than MAX_URL, request is forbiddened\n");
return 3;
	 */
	token_authInit(http);
	token_authStateData * state;
	state = cbdataAlloc(token_authStateData);
	state->data = http;
	state->handler = token_authDone;
	cbdataLock(state->data);
	int	valid = cbdataValid(state->data);
	debug(222, 3)("valid after cbdatalock %d\n",valid);
	snprintf(buf, 24584, "%s\n", httpuri);
	helperSubmit(token_auth, buf, token_authHandleReply, state);
	valid = cbdataValid(state->data);
	debug(222, 3)("valid after helperSubmit %d\n",valid);
	debug(222, 3)("auth url \"%s\" was submited\n", buf);
	return 2;
}

static int hook_cleanup(cc_module *module)
{
	debug(222, 1)("(mod_token_auth) ->     hook_cleanup:\n");
	if (NULL != token_auth)
	{
		helperShutdown(token_auth);
		helperFree(token_auth);
		token_auth = NULL;
	}
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}

int mod_register(cc_module *module)
{
	debug(222, 1)("(mod_token_auth) ->      mod_register:\n");

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

	//module->hook_func_before_sys_init = func_token_auth_shutdown;
	cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
			func_token_auth_shutdown);

	//module->hook_func_http_repl_send_start = func_urlSubmitForAuth; 
	cc_register_hook_handler(HPIDX_hook_func_http_req_parsed,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_parsed),
			func_urlSubmitForAuth);

	mod = module;
	return 0;
}
#endif

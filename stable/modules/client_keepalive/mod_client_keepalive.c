#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;
static MemPool * client_keepalive_config_pool = NULL;

typedef struct _persistent_client_config
{
	int on_off;
	int timeout;
	int always;
	int pre_connection;
}mod_client_keepalive_config;

static void * client_ip_info_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == client_keepalive_config_pool)
	{
		client_keepalive_config_pool = memPoolCreate("mod_client_keepalive other_data mod_client_keepalive_config", sizeof(mod_client_keepalive_config));
	}
	return obj = memPoolAlloc(client_keepalive_config_pool);
}
static int free_mod_client_keepalive_config(void *data)
{
	mod_client_keepalive_config *cfg = (mod_client_keepalive_config *)data;
	if (cfg)
	{
		memPoolFree(client_keepalive_config_pool, cfg);
		cfg = NULL;
	}
	return 0;
}

static int check_if_conf_end(char *str)
{
	if(!strcasecmp(str,"allow") || !strcasecmp(str,"deny"))
		return 1;


	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	//Must be [mod_cc_fast on] or [mod_cc_fast off]
	char *t = strtok(cfg_line, w_space);
	if(strcmp(t, "mod_client_keepalive"))
	{
		debug(156,0)("mod_client_keepalive cfg_line not begin with mod_client_keepalive!\n");
		return -1;
	}

	mod_client_keepalive_config *cfg = client_ip_info_pool_alloc();
	if (NULL == (t = strtok(NULL,w_space))) goto out;

	if (!strcmp(t, "on"))
		cfg->on_off = 1;
	else if(!strcmp(t, "off"))
		cfg->on_off = 0;
	else
		goto out;
	
	if (NULL == (t = strtok(NULL,w_space))) goto out;
	if(check_if_conf_end(t) == 1)
		goto done;
	cfg->timeout = atoi(t);

	if (NULL == (t = strtok(NULL,w_space))) goto out;
	if(check_if_conf_end(t) == 1)
		goto done;
	else if(!strcmp(t,"always"))
		cfg->always = 1;
	else if(!strcmp(t,"never"))
		cfg->always = 2;
    	else 
		goto out;

done:
	cc_register_mod_param(mod, cfg, free_mod_client_keepalive_config);
	return 0;

out:
	free_mod_client_keepalive_config(cfg);
	debug(156,0)("mod_client_keepalive :parse error!\n");
	return -1;
}


/*
   *set_client_keepalive:
   *set the client's keepalive flag 
   */

static int  set_client_keepalive(clientHttpRequest* http)
{
	mod_client_keepalive_config *cfg =(mod_client_keepalive_config*) cc_get_mod_param(http->conn->fd,mod);
	if( cfg == NULL )
	{
		debug(156,0)("mod_client_keepalive :get cfg error!\n");
		return 0;
	}
	request_t *request = http->request;
	if (cfg->always == 1)
	{
		cfg->pre_connection = request->flags.proxy_keepalive; 
		request->flags.proxy_keepalive = 1;
	}
	else if (cfg->always == 2 )
	{
		cfg->pre_connection = request->flags.proxy_keepalive; 
		request->flags.proxy_keepalive = 0;
	}
	else if (!(cfg->on_off) && !request->flags.must_keepalive)
	{
		cfg->pre_connection = request->flags.proxy_keepalive; 
		request->flags.proxy_keepalive = cfg->on_off;
	}
	debug(156,2)("set_client_keepalive: the param is:cfg->always[%d]on_off[%d] cfg->pre_connection[%d]\n",cfg->always,cfg->on_off,cfg->pre_connection);

	if (cfg->timeout > 0)
		Config.Timeout.persistent_request = cfg->timeout;
		
	return 1;
}
static int  set_client_keepalive_timeout(int fd)
{
	mod_client_keepalive_config *cfg =(mod_client_keepalive_config*) cc_get_mod_param(fd,mod);
	if( cfg == NULL )
	{
		debug(156,0)("mod_client_keepalive :in set_client_keepalive_timeout get cfg error!\n");
		return  Config.Timeout.persistent_request;
	}
	int ret;
	debug(156,2)("set_client_keepalive: the param is:timeout[%d][0-not set]\n",cfg->timeout);
	if (cfg->timeout > 0)
		ret = cfg->timeout;
	else 
		ret = Config.Timeout.persistent_request;

	return ret;
}

static int func_http_repl_send_start(clientHttpRequest *http)
{
	assert(http);
	HttpReply *rep = http->reply;
	assert(rep);
	HttpHeader *hdr = &rep->header;
	request_t *request = http->request;
	assert(request);

	mod_client_keepalive_config *cfg =(mod_client_keepalive_config*) cc_get_mod_param(http->conn->fd,mod);
	if( cfg == NULL )
	{
		debug(156,0)("mod_client_keepalive: func_http_repl_send_start get cfg error!\n");
		return 0;
	}
	debug(156,2)("set_client_keepalive:old flags.proxy_keepalive [%d]\n",cfg->pre_connection);
	if (cfg->always == 1 || cfg->always == 2)
	{
		httpHeaderDelById(hdr, HDR_CONNECTION);

/*
		if (!cfg->pre_connection)
			httpHeaderPutStr(hdr, HDR_CONNECTION, "close");
		else if ((request->http_ver.major == 1 && request->http_ver.minor == 0) || !http->conn->port->http11)
		{
			httpHeaderPutStr(hdr, HDR_CONNECTION, "keep-alive");
			if (!(http->flags.accel || http->flags.transparent))
				httpHeaderPutStr(hdr, HDR_PROXY_CONNECTION, "keep-alive");
		}

*/	
		request_t *request = http->request;
		
		const HttpHeader *req_hdr = &request->header;

		debug(156, 3) ("clientSetKeepaliveFlag: method = %s\n",
				RequestMethods[request->method].str);
		{
			http_version_t http_ver;
			if (http->conn->port->http11)
				http_ver = request->http_ver;
			else    
				httpBuildVersion(&http_ver, 1, 0);  /* we are HTTP/1.0, no matter what the client requests... */
		
			debug(156, 3) ("clientSetKeepaliveFlag: http_ver = %d.%d\n",
				request->http_ver.major, request->http_ver.minor);

			if (httpMsgIsPersistent(http_ver, req_hdr))
				httpHeaderPutStr(hdr, HDR_CONNECTION, "keep-alive");
			else
				httpHeaderPutStr(hdr, HDR_CONNECTION, "close");

		}
	}

	return 0;
}

static int hook_cleanup(cc_module *module)
{
	debug(156, 1)("(mod_client_keepalive) ->     hook_cleanup:\n");
	if(NULL != client_keepalive_config_pool)
		memPoolDestroy(client_keepalive_config_pool);
	return 0;
}

// module init 
int mod_register(cc_module *module)
{
	debug(156, 1)("(mod_client_keepalive) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	
//	module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
			func_sys_parse_param);
	//module->hook_func_private_set_client_keepalive = set_client_keepalive;
		
	cc_register_hook_handler(HPIDX_hook_func_private_set_client_keepalive ,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_client_keepalive), 
			set_client_keepalive);

	cc_register_hook_handler(HPIDX_hook_func_private_client_keepalive_set_timeout ,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_keepalive_set_timeout), 
			set_client_keepalive_timeout);

	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			func_http_repl_send_start);

	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);
	return 0;
}

#endif


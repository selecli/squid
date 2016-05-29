#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;
static MemPool * mod_config_pool = NULL;

struct mod_conf_param {
	int http_version; //0 means http 1.0 1 means http 1.1
};

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_http_protocol_modify config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data)
		memPoolFree(mod_config_pool, data);
	return 0;
}

typedef struct _request_param
{
	int http_version; //1 1.1 0 1.0
} request_param;

// confige line example:
// mod_http_protocol_modify http11/http10 allow/deny acl
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data=NULL;
	char* token = NULL;
	int http_version = 0;

	//mod_http_protocl_modify
	if ((token = strtok(cfg_line, w_space)) != NULL) {
		if(strcmp(token, "mod_http_protocol_modify"))
			goto err;	
	}
	else {
		goto err;	
	}

	//http_version
	if (NULL == (token = strtok(NULL, w_space)))
		goto err;
	if (!(strcmp(token, "allow") && strcmp(token, "deny")))
		goto err;
	else {

		if(!strcmp(token, "http11"))
			http_version = 1;		
		else if(!strcmp(token, "http10"))
			http_version = 0;	
		else
			goto err;
	}

	data = mod_config_pool_alloc();
	data->http_version = http_version;
	cc_register_mod_param(mod, data, free_callback);
	return 0;		

err:
	debug(144, 1)("(mod_http_protocol_modify) ->  parse line error\n");
	free_callback(data);
	return -1;
}

static int hookFuncHPMFixClientVersion(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
    if(http->reply && param)
    {   
        http->reply->sline.version.minor = param->http_version;
        debug(144, 5)("mod_http_protocol_modify Reply Version %d %s\n",param->http_version, http->uri);
    }   
    return 0;

}


/* module init */
int mod_register(cc_module *module)
{
	debug(144, 1)("(mod_http_protocol_modify) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
            func_sys_parse_param);

    cc_register_hook_handler(HPIDX_hook_func_private_process_send_header,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_process_send_header),
            hookFuncHPMFixClientVersion);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif

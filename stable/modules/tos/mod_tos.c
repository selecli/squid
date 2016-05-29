#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
//这是本模块在主程序的索引信息的结构指针
static cc_module* mod = NULL;

//这是和acl对应的mod para结构的挂载值：即框架可以为匹配对应acl的fd挂载一个对应的para
struct mod_conf_param {
	int tos_client;	
	int tos_server;
};
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_tos config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data)
	{
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return 0;
}


// confige line example:
// mod_tos client_tos_value server_tos_value allow/deny acl
// 如果配置为0, 即说明此
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data=NULL;
	char* token = NULL;
//mod_tos
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_tos"))
			goto err;	
	}
	else
	{
		debug(95, 3)("(mod_tos) ->  parse line error\n");
		goto err;	
	}

//client_tos_value
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(95, 3)("(mod_tos) ->  parse line data does not existed\n");
		goto err;
	}
	if (!(strcmp(token, "allow") && strcmp(token, "deny")))
	{
		debug(95, 3)("(mod_tos) ->  missing server side tos setting\n");
	}
	
	data = mod_config_pool_alloc();
	data->tos_client = atoi(token);
	
//server_tos_value
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(95, 3)("(mod_tos) ->  parse line data does not existed\n");
		goto err;
	}
	if (!(strcmp(token, "allow") && strcmp(token, "deny")))
	{
		debug(95, 3)("(mod_tos) ->  missing server side tos setting\n");
	}
	data->tos_server = atoi(token);
	
	if(0 > data->tos_client || 0 > data->tos_server)
	{
		debug(95, 2)("(mod_tos) ->  data set error\n");
		goto err;
	}

	cc_register_mod_param(mod, data, free_callback);

	return 0;		
err:
	if(data)
		xfree(data);
	return -1;
}

//set client tos
int func_http_req_read(clientHttpRequest *http)
{
	int newoptval = 0;
	int ret = 0;	
	
	int fd = http->conn->fd;

	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);

	//if client value == 0, return. else  set it
	if(0 == param->tos_client)
	{
		return 0;
	}
	else
	{
		newoptval = param->tos_client << 5;

		ret = setsockopt(fd, IPPROTO_IP, IP_TOS, (char *)&newoptval, sizeof(int));
		if (ret != 0)
		{
			debug(95, 1)("(mod_tos) ->  setsockopt failed\n");
			return -1;
		}
		return 0;	
	}
}


//set server tos
int func_http_req_send(HttpStateData *httpState)
{
	int newoptval;
	int ret;	
	int fd = httpState->fd;

	//here fd_table[fd].ccmod_params must not null, 
	//else ,the framework failed to move data from origin fd to this
	
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);

	//if client value == 0, return. else  set it
	
	if(0 == param->tos_server)
	{
		return 0;
	}
	else
	{
		newoptval = param->tos_server << 5;

		ret = setsockopt(fd, IPPROTO_IP, IP_TOS, (char *)&newoptval, sizeof(int));
		if (ret !=0)
		{
			debug(95, 1)("(mod_tos) ->  setsockopt failed\n");
			return -1;
		}
		return 0;	
	}
}

static int hook_cleanup(cc_module *module)
{                       
	debug(95, 1)("(mod_tos) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;       
}  

/* module init */
int mod_register(cc_module *module)
{
	debug(95, 1)("(mod_tos) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	
	//module->hook_func_http_req_read = func_http_req_read; //modify client side func
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			func_http_req_read);
	//module->hook_func_http_req_send = func_http_req_send; //modify server side func
	cc_register_hook_handler(HPIDX_hook_func_http_req_send,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send),
			func_http_req_send);

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
	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif

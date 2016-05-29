#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
//这是本模块在主程序的索引信息的结构指针
static cc_module* mod = NULL;

//这是和acl对应的mod para结构的挂载值：即框架可以为匹配对应acl的fd挂载一个对应的para
struct mod_conf_param {
	String header_name;	
};
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_ms_client_ip config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data)
	{
	 	if(strLen(data->header_name))
			stringClean(&data->header_name);
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return 0;
}


// confige line example:
// mod_ms_client_ip header_name allow/deny acl
// dont support func : on
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data = mod_config_pool_alloc();
	char* token = NULL;
//mod_ms_client_ip
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_ms_client_ip"))
			goto err;	
	}
	else
	{
		debug(108, 1)("(mod_ms_client_ip) ->  parse line error\n");
		goto err;	
	}
//header_name
	if ((token = strtok(NULL, w_space)) != NULL)
	{
		stringInit(&data->header_name, token);
	}
	else
	{
		debug(108, 1)("(mod_ms_client_ip) ->  parse line error\n");
		goto err;	
	}

	cc_register_mod_param(mod, data, free_callback);

	return 0;		
err:
	if(data)
	{
		if(strLen(data->header_name))
			stringClean(&data->header_name);
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return -1;
}

#ifdef CC_MULTI_CORE	
static void get_clientip(char *buf, request_t *request)
{
	if(opt_squid_id > 0)
	{
		//multisquid case
		String xff = httpHeaderGetList(&request->header, HDR_X_FORWARDED_FOR);
		if(!strLen(xff))
		{
			strcpy(buf, inet_ntoa(request->client_addr));
		}
		else
		{
			strncpy(buf, strBuf(xff), strLen(xff));
		}
		stringClean(&xff);
	}
	else
	{
		strcpy(buf, inet_ntoa(request->client_addr));
	}
}
#endif
/*
 *This line is to modify the header
 */
static int mod_ms_client_ip(HttpStateData* data)
{
	assert(data);
	request_t* request = data->request;
	int fd = data->fd;
#ifdef CC_MULTI_CORE	
	char buf[16];
#else
	char* buf;
#endif
	int i, len;

	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	assert(param);

	debug(108, 3)("param->header_name=%s\n", strBuf(param->header_name));
	HttpHeaderEntry e;

#ifdef CC_MULTI_CORE	
	get_clientip(buf, request);
#else
	buf = inet_ntoa(request->client_addr);
#endif

	e.name = stringDup(&param->header_name);

	stringInit(&e.value, buf);
	len=strLen(e.name);
	i=httpHeaderIdByNameDef(strBuf(e.name), len);
	if(-1 == i)
		e.id = HDR_OTHER;
	else    
		e.id = i;

	httpHeaderAddEntry(&request->header, httpHeaderEntryClone(&e));

	stringClean(&e.name);
	stringClean(&e.value);
	return 0;
}


static int hook_cleanup(cc_module *module)
{                       
	debug(108, 1)("(mod_ms_client_ip) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}  


/* module init */
int mod_register(cc_module *module)
{
	debug(108, 1)("(mod_ms_client_ip) ->  init: init module\n");

	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	strcpy(module->version, "7.0.R.16488.i686");
	
	//module->hook_func_http_req_send = mod_ms_client_ip; //modify server side func
	cc_register_hook_handler(HPIDX_hook_func_http_req_send,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send),
			mod_ms_client_ip);
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
	return 0;
}

#endif

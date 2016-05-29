#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
//这是本模块在主程序的索引信息的结构指针
static cc_module* mod = NULL;

//这是和acl对应的mod para结构的挂载值：即框架可以为匹配对应acl的fd挂载一个对应的para
struct mod_conf_param {
	String orig_name;	
	String new_name;
};
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_modify_s2o_header_name config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data)
	{
		if(strLen(data->orig_name))
			stringClean(&data->orig_name);
		if(strLen(data->new_name))
			stringClean(&data->new_name);
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return 0;
}


// confige line example:
// mod_modify_s2o_header_name orig_header_name new_header_name allow/deny acl
// dont support func : on
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data = mod_config_pool_alloc();
	char* token = NULL;
//mod_modify_s2o_header_name
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_modify_s2o_header_name"))
			goto err;	
	}
	else
	{
		debug(107, 1)("(mod_modify_s2o_header_name) ->  parse line error\n");
		goto err;	
	}
//orig_name
	if ((token = strtok(NULL, w_space)) != NULL)
	{
		stringInit(&data->orig_name, token);
	}
	else
	{
		debug(107, 1)("(mod_modify_s2o_header_name) ->  parse line error\n");
		goto err;	
	}
//new_name
	if ((token = strtok(NULL, w_space)) != NULL)
	{
		stringInit(&data->new_name, token);
	}
	else
	{
		debug(107, 1)("(mod_modify_s2o_header_name) ->  parse line error\n");
		goto err;	
	}
//allow or deny
	if ((token = strtok(NULL, w_space)) != NULL)
	{
		if(!(!strcmp(token, "allow")||!strcmp(token, "deny")||!strcmp(token, "on")||!strcmp(token, "off")))
			goto err;
	}
	else
	{
		debug(107, 1)("(mod_modify_s2o_header_name) ->  parse line error\n");
		goto err;	
	}


	cc_register_mod_param(mod, data, free_callback);

	return 0;		
err:
	if(data)
	{
		if(strLen(data->orig_name))
			stringClean(&data->orig_name);
		if(strLen(data->new_name))
			stringClean(&data->new_name);
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return -1;
}

	

/*
 *This line is to modify the header
 */
static int mod_modify_s2o_header(HttpStateData* data, HttpHeader* hdr)
{
	assert(data);
	int fd = data->fd;
	int i, len;

	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	assert(param);

	debug(107, 3)("param->orig_name=%s, param->new_name=%s\n", strBuf(param->orig_name), strBuf(param->new_name));
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *myheader;
	HttpHeaderEntry e;

	while ((myheader = httpHeaderGetEntry(hdr, &pos)))
	{
		debug(107, 3)("myheader=%s, param->new_name=%s\n", strBuf(myheader->name), strBuf(param->new_name));
		if (strCaseCmp(myheader->name, strBuf(param->orig_name)) == 0)
		{
			debug(107, 3)("%s is myheader->value,%s is param->orig_name\n",strBuf(myheader->value), strBuf(param->orig_name));

			if(strLen(myheader->value) >= 4095)
			{
				debug(107, 3)("A too long header value!!\n");
				return -1;
			}

			stringInit(&e.name, strBuf(param->new_name));
			stringInit(&e.value, myheader->value.buf);
			len=strlen(strBuf(e.name));
			i=httpHeaderIdByNameDef(strBuf(e.name), len);
			if(-1 == i)
				e.id = HDR_OTHER;
			else    
				e.id = i;
			httpHeaderDelByName(hdr, strBuf(param->orig_name));
			httpHeaderAddEntry(hdr, httpHeaderEntryClone(&e));
			//httpHeaderDelAt(&request->header, pos);
			//httpHeaderRefreshMask(&request->header);
			//httpHeaderInsertEntry(&request->header, httpHeaderEntryClone(&e), pos);
			stringClean(&e.name);
			stringClean(&e.value);			
			break;
		}
	}
	return 0;

}

static int hook_cleanup(cc_module *module)
{                       
	debug(107, 1)("(mod_modify_s2o_header_name) ->     hook_cleanup:\n");
	if(mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;       
}       
/* module init */
int mod_register(cc_module *module)
{
	debug(107, 1)("(mod_modify_s2o_header_name) ->  init: init module\n");

	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	strcpy(module->version, "7.0.R.16488.i686");
		
	//module->hook_func_http_req_send_exact = mod_modify_s2o_header; //modify server side func
	cc_register_hook_handler(HPIDX_hook_func_http_req_send_exact,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send_exact),
			mod_modify_s2o_header);

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

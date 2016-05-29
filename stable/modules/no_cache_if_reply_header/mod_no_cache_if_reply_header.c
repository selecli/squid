#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
//这是本模块在主程序的索引信息的结构指针
static cc_module* mod = NULL;

//这是和acl对应的mod para结构的挂载值：即框架可以为匹配对应acl的fd挂载一个对应的para
struct mod_conf_param {
	HttpHeaderEntry e;
};
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_no_cache_if_reply_header config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data)
	{
		if(strLen(data->e.name))
			stringClean(&data->e.name);
		if(strLen(data->e.value))
			stringClean(&data->e.value);
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return 0;
}


// confige line example:
// mod_no_cache_if_reply_header name A value B allow/deny acl[on]
// 符合此acl的并且在源站回复的有header A的, 并且value中包含串B的.不予缓存
// B可以不写
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data = NULL;
	char* token = NULL;
	int i, len;


	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_no_cache_if_reply_header"))
			goto err;	
	}
	else
	{
		debug(103, 3)("(mod_no_cache_if_reply_header) ->  parse line error\n");
		goto err;	
	}	

	data = mod_config_pool_alloc();
	HttpHeaderEntry* e = &data->e;

	while (NULL != (token = strtok(NULL, w_space)))
	{
		if(!strcmp(token, "name"))
		{
			if(NULL != (token = strtok(NULL, w_space)))
			{
				stringInit(&e->name, token);
			}
			else
				debug(103, 1)("(mod_no_cache_if_reply_header) ->  parse line error\n");
		}
		else if(!strcmp(token, "value"))
		{
			if(NULL != (token = strtok(NULL, w_space)))
			{
				stringInit(&e->value, token);
			}
			else
				debug(103, 1)("(mod_no_cache_if_reply_header) ->  parse line error\n");		
		}
		else if(!strcmp(token, "allow") || !strcmp(token, "deny") || !strcmp(token, "on"))	
		{
			break;
		}
		else
			debug(103, 1)("(mod_no_cache_if_reply_header) -> parse line eeror\n");

	}

	len = strLen(e->name);
	i = httpHeaderIdByNameDef(strBuf(e->name), len); 	

	if(-1 == i)
		e->id = HDR_OTHER;
	else
		e->id = i;


	debug(103, 3)("(mod_no_cache_if_reply_header) -> e->id(%d) == HDR_OTHER? %d\n", e->id, (e->id == HDR_OTHER)?1:0);
	cc_register_mod_param(mod, data, free_callback);

	return 0;		
err:
	free_callback(data);
	return -1;
}



/*
 *Make private
 */
static int makePrivate(int fd, HttpStateData *httpState)
{

	debug(103, 3)("(mod_no_cache_if_reply_header) ->  makePrivate beginning:\n");

	Array result; 
	arrayInit(&result);
	int i, ret = 0, find = 0;
	int header_value_exsit = 0;
	int reply_include_header = 0;
	String tmp = StringNull;;
	HttpHeaderEntry *e = NULL;
	cc_mod_param *mod_param = NULL;
	HttpHeaderEntry *myheader = NULL;
	struct mod_conf_param *param = NULL;
	HttpHeaderPos pos = HttpHeaderInitPos;
	//here get all domain's parameters for matching
	Array *mod_params = cc_get_mod_param_array(fd, mod);
	HttpReply* reply = httpState->entry->mem_obj->reply;

	for (i = 0; i < mod_params->count; i++)
	{
		mod_param = mod_params->items[i];
		param = mod_param->param;
		e = &param->e;

		debug(103, 3)("(mod_no_cache_if_reply_header) ->  matching mod_params[%d]: e->name: %s, e->value: %s, e->id: %d\n", i, strBuf(e->name), strBuf(e->value), e->id);

		if(e->id == HDR_OTHER)
		{
			while((myheader = httpHeaderGetEntry(&reply->header, &pos)))
			{
				if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name,strBuf(e->name)) == 0)             
				{
					reply_include_header = 1;
					if(strBuf(myheader->value))
					{
						header_value_exsit = 1;
						arrayAppend(&result, &(myheader->value));
					}
					find = 1;
					break;
				}	
			}
		}
		else
		{
			tmp = httpHeaderGetStrOrList(&reply->header, e->id);
			if(strBuf(tmp))
			{
				header_value_exsit = 1;
				reply_include_header = 1;
				arrayAppend(&result,&(tmp));
				find = 1;
			}
		}
		if (1 == find)
			break;
	}
	if (1 == find)
	{
		if(header_value_exsit == 1 && strBuf(e->value))
		{	
			for(i = 0; i < result.count; i++)
			{
				if(!strCaseCmp(*((String*)(result.items[i])),strBuf(e->value)))
				{
					ret = 5;
					break;
				}
			}
		} 	
		else if (1 == reply_include_header && !strBuf(e->value))
		{
			ret = 5;//set for hook
		}
		debug(103, 3)("(mod_no_cache_if_reply_header) ->  matched header entry:\n");
		debug(103, 3)("(mod_no_cache_if_reply_header) ->  e->name: %s, e->value: %s, e->id: %d; return %d\n", strBuf(e->name), strBuf(e->value), e->id, ret);
		for(i = 0; i < result.count; i++)
		{
			debug(103,3)("(mod_no_cache_if_reply_header)-> result = %s\n",strBuf(*((String*)(result.items[i]))));
		}
	}
	else
	{
		debug(103, 3)("(mod_no_cache_if_reply_header) -> no matched header entry\n");
	}
	arrayClean(&result);
	stringClean(&tmp);

	return ret;
}

static int hook_cleanup(cc_module *module)
{
	debug(103, 1)("(mod_no_cache_if_reply_header) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(103, 1)("(mod_no_cache_if_reply_header) ->  init: init module\n");

	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	strcpy(module->version, "7.0.R.16488.i686");
	//mod->hook_func_http_repl_cachable_check = makePrivate; 
	cc_register_hook_handler(HPIDX_hook_func_http_repl_cachable_check,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_cachable_check),
			makePrivate);
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

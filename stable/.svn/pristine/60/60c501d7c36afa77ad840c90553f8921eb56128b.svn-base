#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;

struct mod_conf_param {
	int http_code[49];
	int count;
};
static MemPool * mod_config_pool = NULL;

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

/*parse config line*/
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data = NULL;
	char* token = NULL;
	int flag = 0; 
	int temp;
	
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_no_cache_if_http_code"))
			goto err;	
	}
	else
	{
		debug(190, 3)("(mod_no_cache_if_http_code) ->  parse line error\n");
		goto err;	
	}	

	if(NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_no_cache_if_http_code config_struct", sizeof(struct mod_conf_param));
	}
	data = memPoolAlloc(mod_config_pool);
	data->count = 0;
	while (NULL != (token = strtok(NULL, w_space)))
	{
		if(0 != strcmp(token, "allow") || 0 != strcmp(token, "deny"))	
		{
			if(49 == data->count)
			{
				debug(190, 3)("mod_no_cache_if_http_code : there are 49 values of squid`s http_code totally.\n");
				flag = 1;
				break;
			}
			if((temp = atoi(token)) >= 0)
			{
				data->http_code[data->count++] = temp; 
			}
			else
			{
				flag = 1;
				debug(190, 3)("mod_no_cache_if_http_code : there is something wrong during parsing the http code in config line\n");	
				break;
			}
		}
		else
		{
			flag = 0;
			break;
		}
	}

	if(1 == flag)
		goto err;
	cc_register_mod_param(mod, data, free_callback);
	return 0;		
err:
	free_callback(data);
	return -1;
}

/*
 *Make private
 *Make http reply uncachable by http code
 */
static int makePrivate(int fd, HttpStateData *httpState)
{
	HttpReply *rep = httpState->entry->mem_obj->reply;
	int ret = 0;
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	int i;

	for(i=0; i<param->count; i++)
	{
		if(param->http_code[i] == rep->sline.status)
		{
			ret = 5;
			break;
		}	
	}
	debug(190, 3)("mod_nocache_if_http_code : Reply will be %s\n",ret ? "uncachable." : "cachable.");
	return ret;
}

static int hook_cleanup(cc_module *module)
{
	debug(190, 1)("(mod_no_cache_if_http_code) ->     hook_cleanup:\n");
	memPoolDestroy(mod_config_pool);
	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(190, 1)("(mod_no_cache_if_http_code) ->  init: init module\n");

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

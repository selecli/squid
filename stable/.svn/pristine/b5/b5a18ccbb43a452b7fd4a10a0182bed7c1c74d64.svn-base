#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_storedir_max_load config_data", sizeof(int));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_mod_config(void *data)
{
	int * cfg = data;
	if(NULL != cfg)
	{
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	return 0;
}
////////////////////// hook functions ////////////////

/**
  * return 0 if parse ok, -1 if error
  */
static int func_sys_parse_param(char *cfg_line)
{
	//Must be [mod_cc_fast on] or [mod_cc_fast off]
	assert(cfg_line);
	int max_load;
	char *t = strtok(cfg_line, w_space);
	if(strcmp(t, "mod_storedir_max_load"))
	{
		debug(114,0)("mod_storedir_max_load cfg_line not begin with mod_storedir_max_load!\n");
		return -1;
	}
	t = strtok(NULL, w_space);
	max_load = atoi(t);
	if(max_load < 1000)
	{
		debug(114, 0)("mod_storedir_max_load Do not setmod_storedir_max_load < 1000 !");
		return -1;
	}
	else
	{
		debug(114, 1)("mod_storedir_max_load max_load = %d\n", max_load);
	}
	
	t = strtok(NULL, w_space);
	if(!strcmp(t, "on"))
	{
		int *p_max_load = mod_config_pool_alloc();
		*p_max_load = max_load;
		cc_register_mod_param(mod, p_max_load, free_mod_config);
		return 0;
	}
	else if(!strcmp(t, "off"))
	{
		return 0;
	}
	debug(114,0)("mod_storedir_max_load cfg_line not end with on or off!\n");
	return -1;
}

int storedir_set_max_load(int max_load)
{
	int* p_max_load = cc_get_global_mod_param(mod);
	int result = *p_max_load > max_load? *p_max_load : max_load;
	debug(114, 3)("storedir_set_max_load returning %d\n", result);
	return result;
}

static int hook_cleanup(cc_module *module)
{                       
	debug(114, 1)("(mod_storedir_max_load) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;       
}     
// module init 
int mod_register(cc_module *module)
{
	debug(114, 1)("(mod_storedir_max_load) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	//module->hook_func_storedir_set_max_load = storedir_set_max_load;
	cc_register_hook_handler(HPIDX_hook_func_storedir_set_max_load,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_storedir_set_max_load),
			storedir_set_max_load);

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

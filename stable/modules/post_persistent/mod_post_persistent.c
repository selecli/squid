#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

/*
typedef struct{
	char on_or_off;
}post_persistent_config;
*/

/**
  * return 0 if parse ok, -1 if error

static int func_sys_parse_param(char *cfg_line)
{
	assert(cfg_line);
	char *t = strtok(cfg_line, w_space);
	if(strcmp(t, "mod_post_persistent"))
	{
		debug(194,0)("mod_post_persistent cfg_line not begin with mod_post_persistent!\n");
		return -1;
	}

	return 0;
}
  */

int post_use_persistent_connection(FwdState *fwdState)
{
	if(fwdState->request->method == METHOD_POST)
	{
		debug(194,8)("mod_post_persistent reuse the fd:%d!\n",fwdState->server_fd);
		return 1;
	}
	return 0;
}

int mod_register(cc_module *module)
{
	debug(194, 1)("(mod_post_persistent) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_parse_param = func_sys_parse_param;
    /*
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
			func_sys_parse_param);
    */

	cc_register_hook_handler(HPIDX_hook_func_private_post_use_persistent_connection,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_post_use_persistent_connection), 
			post_use_persistent_connection);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif


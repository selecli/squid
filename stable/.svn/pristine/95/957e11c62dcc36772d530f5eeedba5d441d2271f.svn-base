#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

static int func_can_find_vary()
{
	return 0;
}

// module init 
int mod_register(cc_module *module)
{
	debug(103, 1)("(mod_disable_vary) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_can_find_vary = func_can_find_vary;
	cc_register_hook_handler(HPIDX_hook_func_can_find_vary,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_can_find_vary),
			func_can_find_vary);

	//mod = module;
//	mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif

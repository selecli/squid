
#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK


static cc_module* mod = NULL;

static int should_set_tproxy(FwdState* fwdState)
{

	if(httpHeaderGetByName(&fwdState->request->header,"X-Forwarded-For").buf)
		return 1;
	else
		return 0;
}

int mod_register(cc_module *module)
{
	debug(149, 1)("(mod_xtproxy) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_private_set_tproxy_ip,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_tproxy_ip), 
			should_set_tproxy);

//	mod = module;
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif

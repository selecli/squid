#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static int is_disable_sendfile(store_client *sc, int fd)
{
	sc->is_zcopyable = 0;
	return 1;
}

// module init 
int mod_register(cc_module *module)
{
	debug(132, 1)("(mod_disable_sendfile) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_client_set_zcopyable = is_disable_sendfile;
	cc_register_hook_handler(HPIDX_hook_func_client_set_zcopyable,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_set_zcopyable),
			is_disable_sendfile);

	return 0;
}

#endif

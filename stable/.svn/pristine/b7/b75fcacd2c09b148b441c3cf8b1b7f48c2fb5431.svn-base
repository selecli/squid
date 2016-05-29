#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

void func_client_handle_ims_reply(clientHttpRequest *http)
{
	if(http->request->flags.nocache_hack)
	{
		storeRelease(http->old_entry);
	}
}

// module init 
int mod_register(cc_module *module)
{
	debug(138, 1)("(mod_ctrl_f5_refresh) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	module->hook_func_client_handle_ims_reply = func_client_handle_ims_reply;

	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif

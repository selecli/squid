#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

static int func_private_clientCacheHit_etag_mismatch(clientHttpRequest* http)
{
	debug(137,3)("mod_trend take function\n");
	http->log_type = LOG_TCP_REFRESH_MISS;
	clientProcessExpired(http);
	return 1;
}

int mod_register(cc_module *module)
{
	debug(137,5)("mod_trend_if_none_match-->mod_register: enter\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_private_clientCacheHit_etag_mismatch,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_clientCacheHit_etag_mismatch),
			func_private_clientCacheHit_etag_mismatch);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif

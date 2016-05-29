#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

static int hook_func_http_req_process(clientHttpRequest * http)
{
	char serverip[16];
	request_t *r = http->request;
	strcpy(serverip, inet_ntoa(http->conn->me.sin_addr));

	http->entry = clientCreateStoreEntry(http, r->method, null_request_flags);
	storeReleaseRequest(http->entry);
	storeBuffer(http->entry);
	HttpReply *rep = httpReplyCreate();
	httpReplySetHeaders(rep, HTTP_OK, NULL, "text/plain",
			strlen(serverip), 0, squid_curtime);
	MemBuf *mb = &rep->body.mb;
	memBufInit(mb,16,16);
	memBufAppend(mb, serverip, strlen(serverip));
	char gang0 [1] = "";
	memBufAppend(mb, gang0, 1);
	httpReplySwapOut(rep, http->entry);

	storeComplete(http->entry);
	return 1; 
}

#ifdef CC_MULTI_CORE
static int func_sys_init()
{
	int do_not_use_mod_show_ip_in_fc6 = (opt_squid_id <= 0);
	assert(do_not_use_mod_show_ip_in_fc6);
	return 0;
}
#endif

// module init 
int mod_register(cc_module *module)
{
	debug(131, 1)("(mod_show_ip) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_http_req_process = hook_func_http_req_process;
	cc_register_hook_handler(HPIDX_hook_func_http_req_process,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
			hook_func_http_req_process);
#ifdef CC_MULTI_CORE
	//module->hook_func_sys_init = func_sys_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			func_sys_init);
#endif
//	mod = module;
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
//	mod = (cc_module*)cc_modules.items[module->slot];
	return 0;
}

#endif

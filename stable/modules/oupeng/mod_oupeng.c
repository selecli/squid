#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;

static int func_http_repl_send_start(clientHttpRequest *http)
{
	assert(http);
    if (0 == http->request->flags.redirected)
        return -1;

    HttpHeader *hdr = &http->reply->header;
    HttpHeaderEntry e;
    stringInit(&e.name,"Content-Disposition");
    stringInit(&e.value,"attachment; filename=\"");

    char *filename = strrchr(http->uri, '/');
    if (NULL == filename)
        return -1;
    filename++;
    stringAppend(&e.value, filename, strlen(filename));
    stringAppend(&e.value, "\"", 1);

    e.id = HDR_CONTENT_DISPOSITION;
    httpHeaderDelById(hdr, e.id);  
    httpHeaderAddEntry(hdr, httpHeaderEntryClone(&e));
    stringClean(&e.name);
    stringClean(&e.value);
	return 0;
}


/* module init */
int mod_register(cc_module *module)
{
	debug(98, 1)("(mod_oupeng) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			func_http_repl_send_start);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 1;
}
#endif

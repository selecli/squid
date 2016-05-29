#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;

static int func_http_repl_send_start(clientHttpRequest *http)
{
    assert(http);
    if (0 == http->request->flags.redirected)
        return -1; 

    char *tmp = strstr(http->uri, "&filename=");
    if (NULL == tmp)
        return -1; 

    char *filename = tmp + strlen("&filename=");
    if ('\0' == *filename)
        return -1; 

    char filename_buf[512];
    if (strstr(http->uri, "RequestDownload"))
        strcpy(filename_buf, "attachment; filename=\"");
    else if (strstr(http->uri, "RequestThumbnail"))
        strcpy(filename_buf, "inline; filename=\"");
    else
        return -1;

    if (NULL != (tmp = strchr(filename, '&')))
        strncat(filename_buf, filename, tmp-filename);
    else
        strcat(filename_buf, filename);
    strcat(filename_buf, "\"");

    // add header 
    HttpHeader *hdr = &http->reply->header;
    HttpHeaderEntry e;
    stringInit(&e.name,"Content-Disposition");
    stringInit(&e.value,filename_buf);
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
    debug(98, 1)("(mod_ninety_nine) ->  init: init module\n");
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

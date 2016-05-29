#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

/*
static int func_http_repl_read_start(int fd, HttpStateData *data)
{
    debug(142, 1) ("GOT HTTP REPLY HDR:\n---------\n%s\n----------\n",
            data->reply_hdr.buf);
    return 0;

}
*/

static int verify_lastmodified(StoreEntry* entry,request_t *request)
{
	if(entry == NULL){
		return 0;
	}

    time_t mod_time = entry->lastmod;
    if (mod_time < 0)
        return 0;

    aclCheck_t ch; 
    memset(&ch, 0, sizeof(ch));
    ch.request = request;
    aclChecklistCacheInit(&ch);

    char *host = request->host;
    mod_domain_access *mda = cc_get_domain_access(host, mod);
    if (mda == NULL)
        mda = cc_get_domain_access("common",mod);

    acl_access *A = mda->acl_rules;
    int pos = 0, answer = 0;  
    while (A) 
    {    
        answer = aclMatchAclList(A->acl_list, &ch);
        if (answer > 0 && A->allow == ACCESS_ALLOWED)
        {    
            if (mod_time < request->ims)
            {
                debug(142, 3) ("(mod_lastmodified_verify)--> YES: entry old than client\n");
                return 1;
            }
        }   
        pos++;
        A = A->next;
    }   

    aclCheckCleanup(&ch);
	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
    debug(142, 1)("(mod_lastmodified_verify) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

    cc_register_hook_handler(HPIDX_hook_func_process_modified_since,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_process_modified_since),
            verify_lastmodified);

    //module->hook_func_http_repl_read_start = func_http_repl_read_start;
    /*
    cc_register_hook_handler(HPIDX_hook_func_http_repl_read_start,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_start),
            func_http_repl_read_start);
    */

    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else
        mod = module;

	return 0;
}

#endif

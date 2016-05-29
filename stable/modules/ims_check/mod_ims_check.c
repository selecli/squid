#include "cc_framework_api.h"
      
#ifdef CC_FRAMEWORK

#define DEBUG_NO 161

static cc_module *mod=NULL;

int func_nocache_stale(clientHttpRequest *http, int stale)
{
    HttpReply *rep = storeEntryReply(http->entry);
    if (rep && 200 == rep->sline.status)
    {
        http->request->flags.nocache = 0;
        stale = 1;
    }
    return stale;
}

 int mod_register(cc_module *module)
   {
       debug(DEBUG_NO,1)("mod_ims_check -> init module\n)");
       strcpy(module->version, "7.0.R.16488.i686");
   
       cc_register_hook_handler(HPIDX_hook_func_http_client_cache_hit,
               module->slot,
               (void **)ADDR_OF_HOOK_FUNC(module,hook_func_http_client_cache_hit),
               func_nocache_stale);
       // reconfigure
       if(reconfigure_in_thread)
           mod = (cc_module*)cc_modules.items[module->slot];
       else
           mod = module;
       return 0; 
   } 


#undef DEBUG_NO 
#endif

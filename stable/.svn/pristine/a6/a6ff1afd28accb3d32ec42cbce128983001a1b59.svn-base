#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

int myStoreClientCopyAddin(StoreEntry *e, store_client *sc)
{
	if ( !EBIT_TEST(e->flags, ENTRY_CACHABLE) || EBIT_TEST(e->flags, RELEASE_REQUEST) ) 
        return 0;

	if (e->store_status == STORE_PENDING) 
        storeSwapOut(e);

	if (SWAPOUT_NONE != e->swap_status && e->mem_obj->swapout.sio != NULL && e->mem_obj->swapout.sio->offset > 0)
	{
		sc->type = STORE_DISK_CLIENT;
		storeSetMemStatus(e, NOT_IN_MEMORY);
	}

	return 0;
}

int myStoreClientCopy3Addin(StoreEntry *e, store_client *sc)
{
	if (sc->copy_offset < e->mem_obj->inmem_lo || sc->copy_offset >= e->mem_obj->inmem_hi)
		sc->type = STORE_DISK_CLIENT;
	return 0;
}

squid_off_t getHighIfFast(const StoreEntry *e, int *shouldReturn)
{
	if (!EBIT_TEST(e->flags, ENTRY_CACHABLE))
	{
		*shouldReturn = 0;
		return 0;
	}
	if (EBIT_TEST(e->flags, RELEASE_REQUEST))
	{
		*shouldReturn = 0;
		return 0;
	}

	*shouldReturn = 1;
	return e->mem_obj->inmem_hi + 1;
}

// module init 
int mod_register(cc_module *module)
{
	debug(104, 1)("(mod_cc_fast) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	
	//module->hook_func_store_client_copy = myStoreClientCopyAddin;
	cc_register_hook_handler(HPIDX_hook_func_store_client_copy,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_store_client_copy), 
			myStoreClientCopyAddin);

	//module->hook_func_store_client_copy3 = myStoreClientCopy3Addin;
	cc_register_hook_handler(HPIDX_hook_func_store_client_copy3,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_store_client_copy3), 
			myStoreClientCopy3Addin);

	//module->hook_func_private_storeLowestMemReaderOffset = getHighIfFast;	
	
	cc_register_hook_handler(HPIDX_hook_func_private_storeLowestMemReaderOffset,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_storeLowestMemReaderOffset), 
			getHighIfFast);
		
	if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
	else
        mod = module;
	return 0;
}

#endif

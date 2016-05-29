#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;

typedef int ISDEFER(void *data);
typedef struct _is_defer_func is_defer_func;
struct _is_defer_func{
	ISDEFER * isDeferFunc;
	void * isdefer_data; //isDeferFunc`s param
	is_defer_func * next; 
};      
 static MemPool * is_defer_func_pool = NULL;
/* these two lists are used to register defers func for funcs httpAcceptDefer and clientReadDefer respectively*/
is_defer_func * is_defer_accept_req_funcs_list = NULL;
is_defer_func * is_defer_read_req_funcs_list = NULL; 

static void *  
is_defer_func_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == is_defer_func_pool)
	{       
		is_defer_func_pool = memPoolCreate("mod_helper_defer other_data link_node", sizeof(is_defer_func));
	}       
	return obj = memPoolAlloc(is_defer_func_pool);  
}

//an isdefer func quoted by httpAcceptDefer 
static int
isDeferAcceptRequestForHelper(void * data)
{
	static time_t last_warn = 0;
	helper * hlp = data;

	if (NULL == hlp)
	{        
		return 0;
	}       
	else if (hlp->stats.queue_size <= 2*hlp->n_running)
	{        
		return 0;
	}       
	if (last_warn + 15 < squid_curtime)
	{       
		debug(147, 1) ("WARNING! Helper's queue will overflow and accepting requests is defered (%d on %d)\n", hlp->stats.queue_size, hlp->n_running);
		last_warn = squid_curtime;
	}       

	return 1;
}

//an isdefer func quoted by clientReadDefer 
static int
isDeferReadRequestForHelper(void * data)
{
	helper * hlp = data;
	if (NULL != hlp)
	{       
		static time_t last_warn = 0;
		helper_server * srv = GetFirstAvailable(hlp);
		if (NULL == srv)
		{
			if (hlp->last_queue_warn != 0
			&& squid_curtime - hlp->last_queue_warn >= 30 )
			{
				if (hlp->stats.queue_size >= 2*hlp->n_running)
				{
					if (last_warn + 15 < squid_curtime)
					{
						debug(147, 1) ("WARNING! Helper's queue will overflow and reading requests is defered (%d on %d)\n", hlp->stats.queue_size, hlp->n_running);
						last_warn = squid_curtime;
					}
					return 1;
				}
			}
		}
	}
	return 0;
}

//register isdefer funcs to isdefer func list of httpAcceptDefer and clientReadDefer 
void 
register_deferfunc_for_helper(helper * hlp)
{
	is_defer_func * is_defer_func_node_accept = is_defer_accept_req_funcs_list;	
	is_defer_func * is_defer_func_node_read = is_defer_read_req_funcs_list;	

	if (NULL == is_defer_accept_req_funcs_list)
	{
		is_defer_accept_req_funcs_list = (is_defer_func*)is_defer_func_pool_alloc();
	 	is_defer_accept_req_funcs_list->isDeferFunc = isDeferAcceptRequestForHelper;	
	 	is_defer_accept_req_funcs_list->isdefer_data = hlp;	
		is_defer_accept_req_funcs_list->next = NULL;
		debug(147, 1) ("By helper defer : Isdefer_func \"isDeferAcceptRequestForHelper\" is registered to the isdefer_func list of httpAcceptDefer\n");	
	}
	else
	{
		while (NULL != is_defer_func_node_accept)
		{
			if (NULL == is_defer_func_node_accept->next)
			{
				is_defer_func * new_node = (is_defer_func *)is_defer_func_pool_alloc();
				new_node->isDeferFunc = isDeferAcceptRequestForHelper;
				new_node->isdefer_data = hlp;
				new_node->next = NULL;
				is_defer_func_node_accept->next = new_node;
				debug(147, 1) ("By helper defer : Isdefer_func \"isDeferAcceptRequestForHelper\" is registered to the isdefer_func list of httpAcceptDefer\n");	
				break;
			}
			else
			{
				is_defer_func_node_accept = is_defer_func_node_accept->next;
			}
		}
	}

	if (NULL == is_defer_read_req_funcs_list)
	{
		is_defer_read_req_funcs_list = (is_defer_func *)is_defer_func_pool_alloc();
	 	is_defer_read_req_funcs_list->isDeferFunc = isDeferReadRequestForHelper;	
	 	is_defer_read_req_funcs_list->isdefer_data = hlp;
		is_defer_read_req_funcs_list->next = NULL;
		debug(147, 1) ("By helper defer : Isdefer_func \"isDeferReadRequestForHelper\" is registered to the isdefer_func list of clientReadDefer\n");	
	}
	else
	{
		while (NULL != is_defer_func_node_read)
		{
			if (NULL == is_defer_func_node_read->next)
			{
				is_defer_func * new_node = (is_defer_func *)is_defer_func_pool_alloc();
				new_node->isDeferFunc = isDeferReadRequestForHelper;
				new_node->isdefer_data = hlp;
				new_node->next = NULL;
				is_defer_func_node_read->next = new_node;
				debug(147, 1) ("By helper defer : Isdefer_func \"isDeferReadRequestForHelper\" is registered to the isdefer_func list of clientReadDefer\n");	
				break;
			}
			else
			{
				is_defer_func_node_read = is_defer_func_node_read->next;
			}
		}
	}
}

//Traverse the isdefer_func list and execute the isder_func registered and decide to defer or not 
static int
getIsdeferAcceptRequest()
{
	is_defer_func * is_defer_func_node = is_defer_accept_req_funcs_list;
	int res = 0;
	while (is_defer_func_node)
	{
		if (0 == (res=is_defer_func_node->isDeferFunc(is_defer_func_node->isdefer_data)))
		{       
			is_defer_func_node = is_defer_func_node->next;
			continue;
		}       
		else    
			break;  
	}       
	return res;
}

//Traverse the isdefer_func list and execute the isder_func registered and decide to defer or not 
static int
getIsdeferReadRequest()
{
	is_defer_func * is_defer_func_node = is_defer_read_req_funcs_list;
	int res = 0;
	while (is_defer_func_node)
	{
		if (0 == (res=is_defer_func_node->isDeferFunc(is_defer_func_node->isdefer_data)))
		{
			is_defer_func_node = is_defer_func_node->next;
			continue;
		}
		else
			break;
	}
	return res;
}

//remove registered isdefer funcs from isdefer func list of httpAcceptDefer and clientReadDefer 
static void
cleanup_registered_deferfunc_when_reconfig(int redirect_should_reload)
{
	is_defer_func * is_defer_func_node_accept = is_defer_accept_req_funcs_list;
	is_defer_func * is_defer_func_node_read = is_defer_read_req_funcs_list;
	is_defer_func * prev_node_accept = is_defer_accept_req_funcs_list;
	is_defer_func * prev_node_read = is_defer_read_req_funcs_list;
	is_defer_func * temp_node = NULL;

	while (NULL != is_defer_func_node_accept)
	{
		if (is_defer_func_node_accept->isDeferFunc == isDeferAcceptRequestForHelper)
		{
			helper *hlp = is_defer_func_node_accept->isdefer_data;
			if (hlp && !strcmp(hlp->id_name, "url_rewriter") && 0 == redirect_should_reload)
			{
				debug(147, 1) ("By helper defer : should not remove redirectors` Isdefer_func from the isdefer_func list of httpAcceptDefer because they do not actually reload this time\n");
				prev_node_accept = is_defer_func_node_accept;
				is_defer_func_node_accept = is_defer_func_node_accept->next;
				continue;
			}

			if (is_defer_func_node_accept == is_defer_accept_req_funcs_list)
			{
				is_defer_func_node_accept = is_defer_func_node_accept->next;
				memPoolFree(is_defer_func_pool, is_defer_accept_req_funcs_list);
				is_defer_accept_req_funcs_list = is_defer_func_node_accept;
				prev_node_accept = is_defer_accept_req_funcs_list;
			}
			else
			{
				prev_node_accept->next = is_defer_func_node_accept->next;
				temp_node = is_defer_func_node_accept->next;
				memPoolFree(is_defer_func_pool, is_defer_func_node_accept);
				is_defer_func_node_accept = temp_node;
			}
			debug(147, 1) ("By helper defer : an Isdefer_func is removed from the isdefer_func list of httpAcceptDefer\n");
		}
		else
		{
			prev_node_accept = is_defer_func_node_accept;
			is_defer_func_node_accept = is_defer_func_node_accept->next;
		}
	}

	while (NULL != is_defer_func_node_read)
	{
		if (is_defer_func_node_read->isDeferFunc == isDeferReadRequestForHelper)
		{
			helper *hlp = is_defer_func_node_read->isdefer_data;
			if (hlp && !strcmp(hlp->id_name, "url_rewriter") && 0 == redirect_should_reload)
			{
				debug(147, 1) ("By helper defer : do not remove redirectors` Isdefer_func from the isdefer_func list of ClientReadDefer for they are actually not reload this time\n");
				prev_node_read = is_defer_func_node_read;
				is_defer_func_node_read = is_defer_func_node_read->next;
				continue;
			}

			if (is_defer_func_node_read == is_defer_read_req_funcs_list)
			{
				is_defer_func_node_read = is_defer_func_node_read->next;
				memPoolFree(is_defer_func_pool, is_defer_read_req_funcs_list);
				is_defer_read_req_funcs_list = is_defer_func_node_read;
				prev_node_read = is_defer_func_node_read;
			}
			else
			{
				prev_node_read->next = is_defer_func_node_read->next;
				temp_node = is_defer_func_node_read->next;
				memPoolFree(is_defer_func_pool, is_defer_func_node_read);
				is_defer_func_node_read = temp_node;
			}
			debug(147, 1) ("By helper defer : an Isdefer_func is removed from the isdefer_func list of clientReadDefer\n");
		}
		else
		{
			prev_node_read = is_defer_func_node_read;
			is_defer_func_node_read = is_defer_func_node_read->next;
		}
	}
}

static int
cleanup_registered_deferfunc_when_shutdown(cc_module * module)
{
	debug(147, 1)("(mod_helper_defer) ->     hook_cleanup:\n");
	is_defer_func * temp_node = NULL;

	while(NULL != is_defer_accept_req_funcs_list)
	{
		temp_node = is_defer_accept_req_funcs_list->next;
		memPoolFree(is_defer_func_pool, is_defer_accept_req_funcs_list);
		is_defer_accept_req_funcs_list = temp_node;
	}

	while(NULL != is_defer_read_req_funcs_list)
	{
		temp_node = is_defer_read_req_funcs_list->next;
		memPoolFree(is_defer_func_pool, is_defer_read_req_funcs_list);
		is_defer_read_req_funcs_list = temp_node;
	}
	return 0;
}

// module init 
int mod_register(cc_module *module)
{
	debug(147, 1)("(mod_helper_defer) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");
//register this mod to the hook point in helperOpenServer,module->hook_func_private_register_deferfunc_for_helper = register_deferfunc_for_helper
	cc_register_hook_handler(HPIDX_hook_func_private_register_deferfunc_for_helper,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_register_deferfunc_for_helper),
			register_deferfunc_for_helper);
//module->hook_func_sys_before_parse_param  = cleanup_registered_deferfunc_for_helper
	cc_register_hook_handler(HPIDX_hook_func_module_reconfigure,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_module_reconfigure),
			cleanup_registered_deferfunc_when_reconfig);
//module->hook_func_is_defer_accept_req  = getIsdeferAcceptRequest
	cc_register_hook_handler(HPIDX_hook_func_is_defer_accept_req,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_is_defer_accept_req),
			getIsdeferAcceptRequest);
//module->hook_func_is_defer_read_req  = getIsdeferReadRequest
	cc_register_hook_handler(HPIDX_hook_func_is_defer_read_req,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_is_defer_read_req),
			getIsdeferReadRequest);
//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanup_registered_deferfunc_when_shutdown);

	//mod = module; 
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif

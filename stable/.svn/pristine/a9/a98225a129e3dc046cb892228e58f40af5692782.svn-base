#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

typedef enum{
	add_modify,
	only_modify
} expires_header_op_type;

typedef struct{
	int should_del;
} client_304_expires_private_data;

/*
static int free_private_data(void * data)
{
	safe_free(data);
	return 0;
}
*/
static void httpHeaderChangeValue(HttpHeaderEntry *header, const char *value, HttpHeader * headers)
{	
	int oldLen = strLen(header->value);
	stringClean(&header->value);
	stringInit(&header->value, value);
	int newLen = strLen(header->value);
	headers->len -= oldLen;
	headers->len += newLen;
}

static void client_process_hit_start(clientHttpRequest* http)
{
	expires_header_op_type *param = cc_get_mod_param(http->conn->fd, mod);

	StoreEntry *e = http->entry;
	if(e->timestamp > -1)
	{       
		HttpHeaderEntry *oldHdrDate = httpHeaderFindEntry(&http->entry->mem_obj->reply->header, HDR_DATE);
		if(!oldHdrDate)
		{
			HttpHeaderEntry *newHdrDate = httpHeaderEntryCreate(HDR_DATE, NULL, mkrfc1123(e->timestamp));
			httpHeaderAddEntry(&http->entry->mem_obj->reply->header, newHdrDate);
		}
		else
		{
			httpHeaderChangeValue(oldHdrDate, mkrfc1123(e->timestamp), &http->entry->mem_obj->reply->header);
		}

	}
       	
	int should_del = 0;
	HttpHeaderEntry *oldHdrExpires = httpHeaderFindEntry(&http->entry->mem_obj->reply->header, HDR_EXPIRES);
	if( (!oldHdrExpires) && (*param == add_modify) && (e->expires > -1) )
	{
		should_del = 1;
		HttpHeaderEntry *hdrExpires = httpHeaderEntryCreate(HDR_EXPIRES, NULL, mkrfc1123(e->expires));
		httpHeaderAddEntry(&http->entry->mem_obj->reply->header, hdrExpires);
	}
	else if(oldHdrExpires && e->expires > -1)
	{
		should_del = 0;
		httpHeaderChangeValue(oldHdrExpires, mkrfc1123(e->expires), &http->entry->mem_obj->reply->header);
	}
	/*
	client_304_expires_private_data *data = xcalloc(1, sizeof(client_304_expires_private_data));
	data->should_del = should_del;
	cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, data, free_private_data, mod);
*/
}

static void del_expires(clientHttpRequest* http)
{	
	client_304_expires_private_data *data = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if(data && data->should_del)
	{
		debug(150,3)("should_del = 1 and deleted expires\n");
		httpHeaderDelById(&http->entry->mem_obj->reply->header, HDR_EXPIRES);
	}
	else
	{
		debug(150,3)("not deleted expires because data=%p \n", data);
	}

}

static void client_process_hit_after_304_send(clientHttpRequest* http)
{
	del_expires(http);
}

static void client_process_hit_end(clientHttpRequest* http)
{
	del_expires(http);
}

static void refresh_staleness(StoreEntry *entry)
{
//	assert(entry->timestamp < 0);
	assert(entry->expires < 0);
}

static int refresh_check_pre_return(time_t age, time_t delta, request_t *request, StoreEntry *entry, time_t *cc_age)
{
	time_t check_time = squid_curtime;
	*cc_age = (delta > 0) ? age - delta : age;

	if((!request || !request->flags.nocache_hack) && entry->expires > -1)
	{       
		if(request && request->cache_control && 
				request->cache_control->max_age > 0 &&  
				age > request->cache_control->max_age)
		{       
			debug(150,2)("XIAOSI_STALE: STALE_EXCEEDS_REQUEST_MAX_AGE_VALUE\n");
			return STALE_EXCEEDS_REQUEST_MAX_AGE_VALUE;
		}       
		else if (entry->expires > check_time)
		{       
			debug(150, 2) ("XIAOSI_FRESH: expires %d >= check_time %d \n",
					(int) entry->expires, (int) check_time);
			return FRESH_EXPIRES;
		}       
		else    
		{       
			debug(150, 2) ("XIAOSI_STALE: expires %d < check_time %d \n",
					(int) entry->expires, (int) check_time);

		}       
	}       
	else    
	{       
		debug(150,2)("XIAOSI_NOTIN\n");
	}
	return -1;	
}

static void refresh_check_set_expires(StoreEntry *entry, time_t cc_age, const refresh_t *R)
{
	if(entry->expires < 0 && entry->timestamp >= 0)
	{
		entry->expires = entry->timestamp + R->min;
		debug(150,2)("XIAOSI changed entry->expire time=%ld+%ld\n",entry->timestamp,
				R->min);
	}
	else
	{
		debug(150,2)("XIAOSI not changed entry->expire time because entry->expires=%ld and entry->timestamp=%ld\n", entry->expires, entry->timestamp);
	}
}

static int free_mod_param(void *private_data)
{
	safe_free(private_data);
	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	expires_header_op_type *type = xcalloc(1, sizeof(expires_header_op_type));
	char *token = NULL;
	
	token = strtok(cfg_line, w_space);
	assert(!strcmp(token, "mod_client_304_expires"));
	
	token = strtok(NULL, w_space);
	if(!strcmp(token, "only_modify"))
	{
		*type = only_modify;
	}
	else
	{
		*type = add_modify;
	}

	cc_register_mod_param(mod, type, free_mod_param);
	return 0;
}

// module init 
int mod_register(cc_module *module)
{
	debug(150, 1)("(mod_client_304_expires) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_client_process_hit_start = client_process_hit_start;
	cc_register_hook_handler(HPIDX_hook_func_client_process_hit_start,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_process_hit_start),
			client_process_hit_start);
	//module->hook_func_client_process_hit_after_304_send = client_process_hit_after_304_send;
	cc_register_hook_handler(HPIDX_hook_func_client_process_hit_after_304_send,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_process_hit_after_304_send),
			client_process_hit_after_304_send);
	//module->hook_func_client_process_hit_end = client_process_hit_end;
	cc_register_hook_handler(HPIDX_hook_func_client_process_hit_end,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_process_hit_end),
			client_process_hit_end);
	//module->hook_func_refresh_staleness = refresh_staleness;
	cc_register_hook_handler(HPIDX_hook_func_refresh_staleness,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_refresh_staleness),
			refresh_staleness);
	//module->hook_func_refresh_check_pre_return = refresh_check_pre_return;
	cc_register_hook_handler(HPIDX_hook_func_refresh_check_pre_return,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_refresh_check_pre_return),
			refresh_check_pre_return);
	//module->hook_func_refresh_check_set_expires = refresh_check_set_expires;
	cc_register_hook_handler(HPIDX_hook_func_refresh_check_set_expires,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module,hook_func_refresh_check_set_expires),
			refresh_check_set_expires);
	        //module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
	//mod = module;
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	return 0;
}

#endif

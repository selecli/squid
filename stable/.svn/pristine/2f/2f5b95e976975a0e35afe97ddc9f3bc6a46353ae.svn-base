#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

typedef struct _mod_conf_param 
{
	HttpHeaderEntry e;
	char location[256];
}mod_conf_param;

static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_prevent_tamper_for_msn config_struct", sizeof(mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data)
	{
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return 0;
}

static int hook_cleanup(cc_module *module)
{
	debug(211, 1)("(mod_prevent_tamper_for_msn) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
	{
		memPoolDestroy(mod_config_pool);
	}
	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	mod_conf_param *cfg = NULL;
	char* token = NULL;
	int i=0;
	
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_prevent_tamper_for_msn"))
		{
			goto err;	
		}
	}
	else
	{
		debug(211, 3)("(mod_prevent_tamper_for_msn) ->  parse line error\n");
		goto err;	
	}	

	cfg = mod_config_pool_alloc();
	
	HttpHeaderEntry* e = &cfg->e;
	
	if(NULL != (token = strtok(NULL, w_space)))
	{
		stringInit(&e->name, token);
	}
	else
	{
		debug(211, 1)("(mod_prevent_tamper_for_msn) ->  parse header error\n");
	}

	if(NULL != (token = strtok(NULL, w_space)))
	{
		stringInit(&e->value, token);
	}
	else
	{
		debug(211, 1)("(mod_prevent_tamper_for_msn) ->  parse value error\n");		
	}

	if(NULL != (token = strtok(NULL, w_space)))
	{
		strcpy(cfg->location, token);
	}
	else
	{
		debug(211, 1)("(mod_prevent_tamper_for_msn) ->  parse location error\n");		
	}

	i = httpHeaderIdByNameDef(strBuf(e->name), strLen(e->name)); 	

	if(-1 == i)
	{
		e->id = HDR_OTHER;
	}
	else
	{
		e->id = i;
	}

	debug(211, 3)("(mod_prevent_tamper_for_msn) -> header %s\n", strBuf(cfg->e.name));
	debug(211, 3)("(mod_prevent_tamper_for_msn) -> value %s\n", strBuf(cfg->e.value));
	debug(211, 3)("(mod_prevent_tamper_for_msn) -> location %s\n", cfg->location);

	cc_register_mod_param(mod, cfg, free_callback);

	return 0;
	
err:
	free_callback(cfg);
	return -1;
}

static HttpHeaderEntry* httpHeaderFindEntry2(const HttpHeader * hdr, http_hdr_type id)
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *e;         
	assert(hdr);
	//assert_eid(id);         

	/* check mask first */
	if (!CBIT_TEST(hdr->mask, id))
		return NULL;    
	/* looks like we must have it, do linear search */
	while ((e = httpHeaderGetEntry(hdr, &pos))) {
		if (e->id == id)    
			return e;       
	}
	/* hm.. we thought it was there, but it was not found */
	assert(0);            
	return NULL;        /* not reached */
}

static int check_reply_header(HttpHeaderEntry* e, HttpHeader *reply_header)
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *reply_e;

	if(e->id == HDR_OTHER)
	{
		while((reply_e = httpHeaderGetEntry(reply_header, &pos)))
		{
			if (reply_e && reply_e->id == HDR_OTHER && 
				strCaseCmp(reply_e->name,strBuf(e->name)) == 0)
			{
				if(strcasecmp(strBuf(e->value),"NULL")==0)
				{
					return 1;
				}
				else
				{
					if(strCaseCmp(reply_e->value,strBuf(e->value)) == 0)
					{
						return 1;
					}
					else
					{
						return 0;
					}
				}
			}
		}

	}
	else
	{
		reply_e = httpHeaderFindEntry2(reply_header, e->id);
		if(reply_e && strCaseCmp(reply_e->name,strBuf(e->name)) == 0)
		{
			if(strcasecmp(strBuf(e->value),"NULL")==0)
			{
				return 1;
			}
			else
			{
				if(strCaseCmp(reply_e->value,strBuf(e->value)) == 0)
				{
					return 1;
				}
				else
				{
					return 0;
				}
			}
		}
	}

	return 0;
}

static int func_http_repl_read_start(int fd, HttpStateData *data)
{
	debug(211, 3)("(mod_prevent_tamper_for_msn) ->  makePrivate\n");
	
	HttpReply* reply = data->entry->mem_obj->reply;
	mod_conf_param *cfg = (mod_conf_param *)cc_get_mod_param(fd, mod);

	if(check_reply_header(&cfg->e, &reply->header)==0)
	{
		EBIT_SET(data->entry->flags, RELEASE_REQUEST);
	}
	
	return 0;
}

static int func_check_and_response_302(clientHttpRequest *http, HttpReply * rep)
{
	if(EBIT_TEST(http->entry->flags, RELEASE_REQUEST))
	{
		mod_conf_param *cfg = (mod_conf_param *)cc_get_mod_param(http->conn->fd, mod);
		
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		storeReleaseRequest(http->entry);
		storeBuffer(http->entry);
		HttpReply *rep = httpReplyCreate();
		httpReplySetHeaders(rep, HTTP_MOVED_TEMPORARILY, NULL, "text/plain", 0, 0, squid_curtime);
		httpHeaderAddEntry(&rep->header, httpHeaderEntryCreate(HDR_LOCATION, "Location", cfg->location));
		httpReplySwapOut(rep, http->entry);
		storeComplete(http->entry);

		return 1;
	}
	return 0;
}

static int func_check_and_response_old_entry(clientHttpRequest *http, http_status status)
{
	mod_conf_param *cfg = (mod_conf_param *)cc_get_mod_param(http->conn->fd, mod);
	
	if(check_reply_header(&cfg->e, &http->reply->header)==0 || status != HTTP_OK)
	{
		return 1;
	}

	return 0;
}

int mod_register(cc_module *module)
{
	debug(211, 1)("(mod_prevent_tamper_for_msn) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);

	cc_register_hook_handler(HPIDX_hook_func_http_repl_read_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_start),
			func_http_repl_read_start);

    cc_register_hook_handler(HPIDX_hook_func_send_header_clone_before_reply,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_send_header_clone_before_reply),
            func_check_and_response_302);

	cc_register_hook_handler(HPIDX_hook_func_get_content_orig_down,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_get_content_orig_down),
			func_check_and_response_old_entry);

	if(reconfigure_in_thread)
	{
		mod = (cc_module*)cc_modules.items[module->slot];
	}
	else
	{
		mod = module;
	}
	
	return 0;
}

#endif

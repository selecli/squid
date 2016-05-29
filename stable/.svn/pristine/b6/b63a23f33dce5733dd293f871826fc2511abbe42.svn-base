#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;

typedef struct merge_header_param {
	String headerName;
} merge_header_param;
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_merge_header config_struct", sizeof(merge_header_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int isEndofCfgline(char * s)
{
	return !strcmp(s, "allow") || 
		!strcmp(s, "deny") || 
		!strcmp(s, "on") ||
		!strcmp(s, "off");
}

static int merge_header_param_free_callback(void *data)
{
	merge_header_param *d = data;
	if(NULL != d)
	{
		stringClean(&d->headerName);
		memPoolFree(mod_config_pool, d);
		d = NULL;
	}

	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	assert(cfg_line);
	char *token = NULL;
	token = strtok(cfg_line, w_space);
	if(!token || strcmp(token, "mod_merge_header"))
	{
		debug(109,0)("mod_merge_header module name error!\n");
		return -1;
	}
	token = strtok(NULL, w_space);
//	char * name = NULL;
	if(!token)
	{
		debug(109,0)("mod_merge_header missing header name!\n");
		return -1;
	}
	else if(isEndofCfgline(token))
	{
		debug(109,0)("mod_merge_header missing header name!\n");
		return -1;
	}
//	else
//	{
//		name = xstrdup(token);
	//}
	merge_header_param *param = mod_config_pool_alloc();
	stringInit(&param->headerName,token);
	cc_register_mod_param(mod, param, merge_header_param_free_callback);

	return 0;
}

static int func_http_req_parsed(clientHttpRequest *http)
{
	assert(http);
	request_t *request = http->request;
	int fd = http->conn->fd;
	Array * modparam = cc_get_mod_param_array(fd, mod);
	int paramcount = cc_get_mod_param_count(fd, mod);
	merge_header_param *param = NULL;
	int i;
	for(i=0; i<paramcount; i++){
		//get the name to merge
		assert(modparam->items[i]);
		param = (merge_header_param *)((cc_mod_param*)modparam->items[i])->param;
		assert(param);
		const char *name = strBuf(param->headerName);
		assert(name);
		
		//merge by name
		String mergedValue;
	       	stringInit(&mergedValue, "");
		HttpHeaderPos pos = HttpHeaderInitPos;
		HttpHeaderEntry *curHE;
		while ((curHE = httpHeaderGetEntry(&request->header, &pos)))
		{
			if(!strcmp(strBuf(curHE->name), name))
			{
				//merge value
				stringAppend(&mergedValue, strBuf(curHE->value), strLen(curHE->value));
			}
		}

		//only if header is existing
		if(strLen(mergedValue))
		{
			httpHeaderDelByName(&request->header,name);
			int headerid = httpHeaderIdByNameDef(name, strLen(param->headerName));
			if(headerid == HDR_UNKNOWN)
			{
				headerid = HDR_OTHER;
			}
			HttpHeaderEntry *newe = httpHeaderEntryCreate(headerid, name, strBuf(mergedValue));

			httpHeaderAddEntry(&request->header, newe);
		}
		
		stringClean(&mergedValue);
	}
	return 0;
}

static int hook_cleanup(cc_module *module)
{
	debug(109, 1)("(mod_merge_header) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}
/* module init */
int mod_register(cc_module *module)
{
	debug(109, 1)("(mod_merge_header) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_http_req_parsed = func_http_req_parsed;
	cc_register_hook_handler(HPIDX_hook_func_http_req_parsed,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_parsed),
			func_http_req_parsed);
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);
	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 1;
}
#endif

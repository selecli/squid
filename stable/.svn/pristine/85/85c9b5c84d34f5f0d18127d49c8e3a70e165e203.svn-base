#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

#define MAX_BROKEN_SERVER_TYPES 256

typedef struct
{
	String vary_headers; //For example, accept-encoding or accept-encoding, accept-language
	regex_t *server_reg;
	int is_all;
} vary_enhance_config;
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_vary_enhance config_struct", sizeof(vary_enhance_config));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_vary_enhance_config(void * data)
{
	assert(data);
	vary_enhance_config * config = data;
	stringClean(&config->vary_headers);
	if(!config->is_all){
		regfree(config->server_reg);
	}
	memPoolFree(mod_config_pool, config);	
	config = NULL;
	return 0;
}
static HttpHeaderEntry* httpHeaderFindEntry2(const HttpHeader * hdr, http_hdr_type id)
{       
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *e;
	assert(hdr);
	if (!CBIT_TEST(hdr->mask, id))
		return NULL;
	/* looks like we must have it, do linear search */
	while ((e = httpHeaderGetEntry(hdr, &pos))) 
	{
		if (e->id == id)
			return e;       
	}
	/* hm.. we thought it was there, but it was not found */
	assert(0);
	return NULL;        /* not reached */
}
static int is_servertype_match(const HttpHeader *header, vary_enhance_config *config){
	HttpHeaderEntry * entry = httpHeaderFindEntry2(header,HDR_SERVER);
	//Some replies has not Server header such as 304
	if(!entry)
	{
		return 0;
	}
	if(config->is_all)
	{
		return 1;
	}
	const char* server;
	
	String value = entry->value;
	server = strBuf(value);
	if (regexec(config->server_reg, server, 0, 0, 0) == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
	
}
/////////////////////// hook functions ////////////////

/**
  * parse config line to offline_t
  * return 0 if parse ok, -1 if error
  */
static int func_sys_parse_param(char *cfg_line)
{
	assert(cfg_line);
	vary_enhance_config *config = mod_config_pool_alloc();
	config->is_all = 0;
	char *token = NULL;
	//Read mod name
	token = strtok(cfg_line,w_space);
	if(token == NULL || strcmp(token,"mod_vary_enhance"))
	{
		return -1;
	}
	//Read key word vary_headers
	token = strtok(NULL,w_space);
	if(token == NULL || strcmp(token,"vary_headers"))
	{
		debug(100,0)("mod_vary_enhance parse cfgline error: Missing key word vary_headers!\n");
		return -1;
	}
	//Read between ()
	if(!(token = strtok(NULL,"()")))
	{
		debug(100,0)("mod_vary_enhance parse cfgline error: Missing value of vary_header,which must be in the brackets ( )!\n");
		return -1;
	}
	stringInit(&config->vary_headers, token);
	//Read server_types
	token = strtok(NULL,w_space);
	if(token == NULL || strcmp(token,"server_types"))
	{
		debug(100,0)("mod_vary_enhance parse cfgline error: Missing key word server_types!\n");
		return -1;
	}
		
	//Read content of server_types
	token = strtok(NULL,w_space);
	if(!strcmp(token,"all"))
	{
		config->is_all = 1;
	}
	else
	{
		config->is_all = 0;
		int flags = REG_EXTENDED | REG_NOSUB;
		config->server_reg = xcalloc(1,sizeof(regex_t));
		int errcode;
		if ((errcode = regcomp(config->server_reg, token, flags)) != 0)
		{
			char errbuf[256];
			regerror(errcode, config->server_reg, errbuf, sizeof errbuf);
			debug(100, 0) ("mod_vary_enhance: Invalid regular expression '%s': %s\n",token, errbuf);
			return -1;
		}
	}
	

	cc_register_mod_param(mod,config,free_vary_enhance_config);
	return 0;
}


static int is_CacheNegatively(HttpReply *reply)
{
	switch(reply->sline.status)
	{
		case HTTP_NO_CONTENT:
		case HTTP_USE_PROXY:
		case HTTP_BAD_REQUEST:
		case HTTP_FORBIDDEN:
		case HTTP_NOT_FOUND:
		case HTTP_METHOD_NOT_ALLOWED:
		case HTTP_REQUEST_URI_TOO_LONG:
		case HTTP_INTERNAL_SERVER_ERROR:
		case HTTP_NOT_IMPLEMENTED:
		case HTTP_BAD_GATEWAY:
		case HTTP_SERVICE_UNAVAILABLE:
		case HTTP_GATEWAY_TIMEOUT:
			return 1;
		default:
			return 0;
	}
}


/**
 * Add vary header at direction 2 if has no vary and server_type matches
 */
static int func_http_repl_read_start(int fd, HttpStateData *data)
{

	HttpReply * reply = data->entry->mem_obj->reply;
	int hasVary = (httpHeaderFindEntry2(&reply->header,HDR_VARY) != NULL);
	int isNegatively = is_CacheNegatively(reply);

	/**
	if(hasVary && isNegatively){
		httpHeaderDelByName(&reply->header,"Vary");
		return 0;
	}
	*/
	if (isNegatively)
	{
		return 0;
	}
	if(hasVary)
	{
		return 0;
	}
	
	vary_enhance_config *config = (vary_enhance_config *)cc_get_mod_param(fd, mod);
	if(is_servertype_match(&reply->header,config))
	{
		HttpHeaderEntry e;
		stringInit(&e.name, "Vary");
		e.value = stringDup(&config->vary_headers);
		e.id = HDR_VARY;
		httpHeaderAddEntry(&data->entry->mem_obj->reply->header, httpHeaderEntryClone(&e));
		stringClean(&e.name);
		stringClean(&e.value);
		debug(100,2)("server_type matched\n");
	}
	else
	{
		debug(100,2)("server_type not matched\n");
	}

	return 0;
}

/**
 * Add vary into varyEvaluateMatch check
 */
int func_http_client_cache_hit_vary(clientHttpRequest *http)
{
	if(httpHeaderFindEntry2(&http->entry->mem_obj->reply->header,HDR_VARY))
	{
		return 0;
	}
	request_t *request = http->request;
	const char *vary = request->vary_headers;
	vary_enhance_config *config = (vary_enhance_config *)cc_get_mod_param(http->conn->fd, mod);
	if(vary)
	{//Only take place in second attempt
		if(is_servertype_match(&http->entry->mem_obj->reply->header,config))
		{
			HttpHeaderEntry e;
			stringInit(&e.name, "Vary");
			e.value = stringDup(&config->vary_headers);
			e.id = HDR_VARY;
			httpHeaderAddEntry(&http->entry->mem_obj->reply->header, httpHeaderEntryClone(&e));
			stringClean(&e.name);
			stringClean(&e.value);
		}
	}
	return 0;
}

static int hook_cleanup(cc_module *module)
{
	debug(100, 1)("(mod_vary_enhance) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}
// module init 
int mod_register(cc_module *module)
{
	debug(100, 1)("(mod_vary_enhance) ->  init: init module\n");

	strcpy(module->version, "5.5.6030.i686");
	
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	//module->hook_func_http_repl_read_start = func_http_repl_read_start;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_read_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_start),
			func_http_repl_read_start);

	//module->hook_func_http_client_cache_hit_vary = func_http_client_cache_hit_vary;
	cc_register_hook_handler(HPIDX_hook_func_http_client_cache_hit_vary,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_client_cache_hit_vary),
			func_http_client_cache_hit_vary);

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
	return 0;
}

#endif

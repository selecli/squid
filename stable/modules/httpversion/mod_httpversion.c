#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
#define HEADERS_LENTH_IN_ALL 4096
#define white_space " \t"

static char* removeTablesAndBlank (char* string) 
{
	assert(string);
	char* buf = string; 

	while( *buf == ' ' || *buf == '\t' ||  *buf == '\r')
	{
		buf++;  
	}
	return buf;
}

static cc_module* mod = NULL;

/*
 *use array not list to store the headers wanna to del
 */
struct mod_conf_param
{
	String  hdr_chain;
};
static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_httpversion config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

//callback: free the data
static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	assert(data);
	if(strLen(data->hdr_chain) != 0)
		stringClean(&data->hdr_chain);
	memPoolFree(mod_config_pool, data);
	data = NULL;
	return 0;
}

// confige line example:mod_httpversion header{1,..n} allow acl
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data = NULL;

	char* pos_dev = NULL;
	char* loopchar = cfg_line;
	data = mod_config_pool_alloc();

	//remove acl part
	while(*loopchar != '\0')
		loopchar++;

	while(*loopchar != ' ' && *loopchar != '\t')
		loopchar--;
	
	if( !strcmp((loopchar+1), "on") )
		*loopchar = '\0';
	else
	{
		do      
		{       
			loopchar--;
		}       
		while(*loopchar != ' ' && *loopchar != '\t');

		if( !strncmp((loopchar+1), "allow", strlen("allow")) || !strncmp((loopchar+1), "deny", strlen("deny")))
			*loopchar = '\0';
		else
			goto out;
	}
	       	

//devide the cfg line	
	pos_dev = strstr(cfg_line, " ");
	if(pos_dev)
	{
		*pos_dev = '\0';
		pos_dev++;
		pos_dev = removeTablesAndBlank(pos_dev);
	}
	else
	{
		goto out;
	}
	
	if(strncmp(cfg_line, "mod_httpversion", 15))
		goto out;
		
	
	stringInit(&data->hdr_chain, pos_dev);
	
	if(strLen(data->hdr_chain) != 0)
	{
		cc_register_mod_param(mod, data, free_callback);
		return 0;
	}
	else
		goto out;

		
out:
	if (strLen(data->hdr_chain) != 0)
		stringClean(&data->hdr_chain);
	if (data)
	{
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return -1;		
}


/*
 *func: remove a header
 */
static void RemoveHdr(char* hdr_name, request_t* request)
{
	assert(request);

	httpHeaderDelByName(&request->header,hdr_name);
}

/*
 *main loop of func
 */
static int func_http_req_read(clientHttpRequest *http)
{
	assert(http);
	assert(http->request);

	int fd = http->conn->fd;
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	char* token = NULL ;
	
	assert(param);
	//main func part
	assert(strLen(param->hdr_chain));

	if(0 == http->http_ver.minor)
	{
		if ((token = strtok(param->hdr_chain.buf, w_space)) != NULL)
		{
			do
			{
				RemoveHdr(token, http->request);
			}while(NULL != (token = strtok(NULL, w_space)));
		}
		else    
		{
			//can't be here.
			assert(0);
		}
	}

	return 0;

}

//added for destroying the mempool of config_struct
static int hook_cleanup(cc_module *module)
{
	debug(100, 1)("(mod_httpversion) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(100, 1)("(mod_httpversion) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
	//module->hook_func_http_before_redirect = func_http_req_read;
	cc_register_hook_handler(HPIDX_hook_func_http_before_redirect,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
			func_http_req_read);
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

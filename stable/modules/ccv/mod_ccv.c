#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK


static cc_module* mod = NULL;
struct ccv_config
{
	char ccv1[20];
	char ccv2[20];
    int on;
};

static MemPool * mod_config_pool = NULL;
static void * mod_config_pool_alloc(void)
{
    void * obj = NULL;
    if (NULL == mod_config_pool)
    {   
        mod_config_pool = memPoolCreate("mod_ccv config_struct", sizeof(struct ccv_config));
    }   
    return obj = memPoolAlloc(mod_config_pool);
}

// free modules parameter
static int free_callback(void* param)
{
    struct ccv_config* data = (struct ccv_config*) param;
    if(data)
    {    
        memPoolFree(mod_config_pool, data);
        data = NULL; 
    }    
    return 0;
}



//static struct ccv_config *cfg;
/**
  * return 0 if parse ok, -1 if error
  */
static int func_sys_parse_param(char *cfg_line)
{
	//Must be [mod_cc_fast on] or [mod_cc_fast off]
	assert(cfg_line);
	char *t = strtok(cfg_line, w_space);
	if(strcmp(t, "mod_ccv"))
	{
		debug(144,0)("mod_ccv cfg_line not begin with mod_ccv!\n");
		return -1;
	}
	
	struct ccv_config *cfg = mod_config_pool_alloc();
	//memset(cfg,0,sizeof(struct ccv_config));
	t = strtok(NULL, w_space);
	if(t == NULL)
	{
        free_callback(cfg);
		return -1;
	}
	
	strncpy(cfg->ccv1, t, strlen(t));
	t = strtok(NULL,w_space);
	if(t == NULL)
	{
        free_callback(cfg);
		return -1;
	}
	strncpy(cfg->ccv2,t,strlen(t));

	cc_register_mod_param(mod, cfg, free_callback);
	return 0;
}

static int set_ccv(clientHttpRequest *http)
{

#define TCP_CONGESTION      13  /* Congestion control algorithm */
	int fd = http->conn->fd;
	struct ccv_config* cfg = cc_get_mod_param(fd,mod);
	debug(144,2)("set-ccv: enter  ccv1 is: %s, ccv2 is: %s\n",cfg->ccv1, cfg->ccv2);
	int ret = -1;

	char* ccv1 = xcalloc(1,20);
	char* ccv2 = xcalloc(1,20);
	snprintf(ccv1,20,"ccv%s",cfg->ccv1);
	snprintf(ccv2,20,"ccv%s",cfg->ccv2);
	
	if(http->request->flags.proxy_keepalive == 1)
	{
		
		if(cfg->ccv1[0]>'0' && cfg->ccv1[0] <='9')
		{
			if((ret = setsockopt(fd,IPPROTO_TCP,TCP_CONGESTION,ccv1,strlen(ccv1)))==-1)
			{

				debug(144,2)("set-ccv: set ccv1 error\n");

			}
		}
		else
		{
				debug(144,2)("set_ccv: config ccv1 error\n");
		}
	}
	else
	{
		if(cfg->ccv2[0]>'0' && cfg->ccv2[0] <='9')
		{
			if((ret = setsockopt(fd,IPPROTO_TCP,TCP_CONGESTION,ccv2,strlen(ccv2)))==-1)
			{
				debug(144,2)("set_ccv: set ccv2 error\n");
			}
		}
		else
		{
				debug(144,2)("set_ccv: config ccv2 error\n");
		}

	}
	char* cc_name=xcalloc(1,20);	
	socklen_t len=20;
	getsockopt(fd,IPPROTO_TCP,TCP_CONGESTION,cc_name,&len);
	debug(144,2)("cc_name is: %s\n",cc_name);

	safe_free(cc_name);
	safe_free(ccv1);
	safe_free(ccv2);
	return 0;
}

int mod_register(cc_module *module)
{
	debug(144, 1)("(mod_ccv) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
			func_sys_parse_param);
	//module->hook_func_http_req_parsed = set_ccv;

	cc_register_hook_handler(HPIDX_hook_func_http_req_parsed,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_parsed), 
			set_ccv);

	//mod = module;
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	return 0;
}

#endif


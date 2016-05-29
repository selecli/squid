#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;
struct persist_request_config {
	char* sublayer_ips;
	int tcp_keepalive_idle;
	int tcp_keepalive_interval;
	int tcp_keepalive_timeout;
};
static MemPool * mod_config_pool = NULL;
static void * mod_config_pool_alloc(void)
{
    void * obj = NULL;
    if (NULL == mod_config_pool)
    {   
        mod_config_pool = memPoolCreate("mod_persist_request_timeout config_struct", sizeof(struct persist_request_config));
    }   
    return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
    struct persist_request_config* data = (struct persist_request_config*) param;
    if(data)
    {   
        if(data->sublayer_ips)
            safe_free(data->sublayer_ips);
        memPoolFree(mod_config_pool, data);
        data = NULL;
    }
    return 0;
}

/**
  * return 0 if parse ok, -1 if error
  */

static int func_sys_parse_param(char *cfg_line)
{
	//Must be [mod_cc_fast on] or [mod_cc_fast off]
	char *t = strtok(cfg_line, w_space);
	if(strcmp(t, "mod_persist_request_timeout"))
	{
		debug(153,0)("mod_persist_request_timeout cfg_line not begin with mod_persist_request_timeout!\n");
		return -1;
	}
	
    struct persist_request_config *pr_cfg = NULL;
    pr_cfg = mod_config_pool_alloc();
    memset(pr_cfg, 0, sizeof(struct persist_request_config));

	if((t=strtok(NULL,w_space)) ==NULL)   //parse sublayer_ips
	{
		goto error;	
	}

	if(!strcmp(t,"sublayer_ips"))	
	{
		if(!(t=strtok(NULL,w_space)))		
		{
			return -1;	
		}else{
			pr_cfg->sublayer_ips= xstrdup(t);
//			fprintf(stderr,"the sublayer_ips is %s\n",t);
		}
	}else{
		goto error;	
	}



	if((t=strtok(NULL,w_space)) ==NULL)  //parse tcp_keepalive parameter!!
	{
		goto error;	
	}

	if(strcmp(t,"tcp_keepalive"))
	{
		goto error;	
		
	}

	if(!(t=strtok(NULL,w_space)))		
	{
		goto error;	
	}else{

//		fprintf(stderr,"the tcp_keepalive is %s\n",t);
		pr_cfg->tcp_keepalive_idle = atoi(t);
		t = strchr(t, ',');
		if (t)
		{
			t++;
			pr_cfg->tcp_keepalive_interval = atoi(t);
			t = strchr(t, ',');
		}
		if (t)
		{
			t++;
			pr_cfg->tcp_keepalive_timeout= atoi(t);
		}

	}
    cc_register_mod_param(mod, pr_cfg, free_callback);
	return 0;

error:
    free_callback(pr_cfg);
	debug(153, 1)("(mod_persist_request_timeout) -> parse parameter failed!!!\n");
	return -1;
}


static void set_sublayer_tcp_keepalive(int fd)
{
    struct persist_request_config *pr_cfg = (struct persist_request_config *)cc_get_global_mod_param(mod);
    if (NULL == pr_cfg)
        return;

	fde *F=&fd_table[fd];
	if(strstr(pr_cfg->sublayer_ips,F->ipaddr) && pr_cfg && fd >0 )	
	{
		debug(153, 4)("(mod_persist_request_timeout) ->  set_sublayer_tcp_keepalive successfully\n");
		commSetTcpKeepalive(fd, pr_cfg->tcp_keepalive_idle, pr_cfg->tcp_keepalive_interval, pr_cfg->tcp_keepalive_timeout);
	}
}

static void set_sublayer_persist_request_timeout(int fd)
{
    struct persist_request_config *pr_cfg = (struct persist_request_config *)cc_get_global_mod_param(mod);
    if (NULL == pr_cfg)
        return;
	fde *F=&fd_table[fd];
//	debug(153, 4)("(mod_persist_request_timeout) ->  set_sublayer_persist_request_timeout %d successfully\n",pr_cfg->tcp_keepalive_timeout);
	if(strstr(pr_cfg->sublayer_ips,F->ipaddr) && pr_cfg && fd >0)	
	{
		debug(153, 4)("(mod_persist_request_timeout) ->  set_sublayer_persist_request_timeout %d successfully\n",pr_cfg->tcp_keepalive_timeout);
		commSetTimeout(fd,pr_cfg->tcp_keepalive_timeout,NULL,NULL);
	}
}

static void set_sublayer_persist_connection_timeout(int fd)
{
    struct persist_request_config *pr_cfg = (struct persist_request_config *)cc_get_mod_param(fd, mod);
    if (NULL == pr_cfg)
        return;
	//fde *F=&fd_table[fd];
	debug(153, 4)("(mod_persist_request_timeout) ->  set_sublayer_persist_connection_timeout %d successfully\n",pr_cfg->tcp_keepalive_timeout);
	commSetTimeout(fd,pr_cfg->tcp_keepalive_timeout,NULL,NULL);
}


static int cleanup()
{
    if(NULL != mod_config_pool)
        memPoolDestroy(mod_config_pool);
	return 0;
}


int mod_register(cc_module *module)
{
    debug(153, 1)("(mod_persist_request_timeout) ->  init: init module\n");

    strcpy(module->version, "7.0.R.16488.i686");

    //module->hook_func_sys_parse_param = func_sys_parse_param;
    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
            func_sys_parse_param);

    cc_register_hook_handler(HPIDX_hook_func_private_set_sublayer_tcp_keepalive,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_sublayer_tcp_keepalive), 
            set_sublayer_tcp_keepalive);

    cc_register_hook_handler(HPIDX_hook_func_private_set_sublayer_persist_connection_timeout,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_sublayer_persist_connection_timeout), 
            set_sublayer_persist_connection_timeout);

    cc_register_hook_handler(HPIDX_hook_func_private_set_sublayer_persist_request_timeout,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_sublayer_persist_request_timeout), 
            set_sublayer_persist_request_timeout);

    cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module,hook_func_sys_cleanup),
            cleanup);

    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else
        mod = module;
    return 0;
}

#endif


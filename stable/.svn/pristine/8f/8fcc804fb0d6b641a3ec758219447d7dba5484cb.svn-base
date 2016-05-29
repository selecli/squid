#include "cc_framework_api.h"
#include <fnmatch.h>
#include <ctype.h>
#include <string.h>
#include "squid.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

uint32_t fd_flag[SQUID_MAXFD];

uint32_t reconf = 0;

typedef enum 
{
	ACT_UNSET,
	ACT_DROP,
	ACT_REDIRECT
} action_t;


//mod_param
typedef struct _mod_config
{
	int limit_num;
	int current_num;
	action_t action;
} mod_config;

static MemPool * mod_config_pool = NULL; 
/* callback: free the data */
static int free_mod_config(void *data)
{
	mod_config *cfg = (mod_config *)data;
	if (cfg)
	{
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	return 0;
}

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_connection_limit config_struct", sizeof(mod_config));
	}
	return obj = memPoolAlloc(mod_config_pool);	
}

/////////////////////// hook functions ////////////////
static int func_sys_parse_param(char *cfg_line)
{
	reconf++;
	
	debug(102, 1)("(connection_limit) ->  parse_param [%s]\n", cfg_line);
	char *token = NULL;
	mod_config *cfg = NULL;
	char *p = strstr(cfg_line, "allow");
	if (!p)
		goto out;
	*p = 0;

	if ((token = strtok(cfg_line, w_space)) == NULL)
		goto out;
	else if(strcmp(token,"mod_connection_limit"))
		goto out;
	cfg = mod_config_pool_alloc();
	
	while ((token = strtok(NULL, w_space)))
	{
		if (0 == cfg->limit_num)
			cfg->limit_num = atoi(token);
		if (0 == strcmp(token, "drop"))		
			cfg->action = ACT_DROP;
	}

	if (ACT_UNSET == cfg->action)
		goto out;

	cc_register_mod_param(mod, cfg, free_mod_config);

	return 0;

out:
	if (cfg)
	{
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	debug(102, 1)("(mod_connection_limit) ->  parse err, cfgline=%s\n", cfg_line);
	return -1;
}


//#define HTTP_CONNECTION_LARGE 605
#define HTTP_CONNECTION_LARGE 555

static int func_http_req_process(clientHttpRequest *http)
{
	mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	debug(102, 3)("(mod_connection_limit) -> limit_num=%d currnt_num=%d  url=%s\n", cfg->limit_num, cfg->current_num, strBuf(http->request->urlpath));

	/* if connection is large then err */	
	if (cfg->current_num >= cfg->limit_num)
	{
		ErrorState *err = errorCon(ERR_CONNECT_FAIL, HTTP_CONNECTION_LARGE, http->orig_request);
		http->log_type = LOG_TCP_MISS;
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		errorAppendEntry(http->entry, err);
		return RET_RETURN;	
	}
	
	mod_config *private_cfg = cc_get_mod_private_data(CONN_STATE_DATA, http->conn, mod);
	if (!private_cfg)
	{
		//assert(fd_flag[http->conn->fd] == 0);
		if(fd_flag[http->conn->fd] == 0)
		{
			cfg->current_num++;
		}
		fd_flag[http->conn->fd] = reconf;
		cc_register_mod_private_data(CONN_STATE_DATA, http->conn, cfg, NULL, mod);	
	}

	
	return 0;
}


static int func_conn_state_free(ConnStateData *connState)
{
	debug(102, 3)("(connection_limit) ->  connection free\n");
	mod_config *private_cfg = cc_get_mod_private_data(CONN_STATE_DATA, connState, mod);
	if (private_cfg) {
		if( fd_flag[connState->fd] == reconf )
			private_cfg->current_num--;
		fd_flag[connState->fd] = 0;
	}
	return 0;	
}

static int func_sys_cleanup()
{
	debug(102, 1)("(connection_limit) ->  cleanup moudle\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}
int mod_register(cc_module *module)
{
	debug(102, 1)("(connection_limit) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_sys_cleanup = cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			func_sys_cleanup);
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
			func_sys_parse_param);

	//module->hook_func_http_req_process = func_http_req_process;
	cc_register_hook_handler(HPIDX_hook_func_http_req_process,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
			func_http_req_process);

	//module->hook_func_conn_state_free = func_conn_state_free;
	cc_register_hook_handler(HPIDX_hook_func_conn_state_free,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_conn_state_free),
			func_conn_state_free);

	memset(fd_flag, 0, sizeof(fd_flag));
	
	//mod = module;
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
//	mod = (cc_module*)cc_modules.items[module->slot];
	return 0;
}

#endif

#include "cc_framework_api.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef CC_FRAMEWORK

static cc_module *mod = NULL;
static helper *media_prefetchers = NULL;
static int helper_proc_num = 60;
typedef struct _mod_config
{
	int is_start_helper;
}mod_config;

static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_pps config_sturct", sizeof(mod_config));	
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_cfg(void *data)
{
	struct mod_config * cfg = (struct mod_config*)data;
	if(NULL != cfg)
	{
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	debug(188, 1)("(mod_pps) ->     hook_func_sys_parse_param:----:)\n");

        if(strstr(cfg_line, "allow") == NULL )
                return 0;
	
	debug(188, 3)("(mod_pps) ->     hook_func_sys_parse_param:----: set is_start_helper now)\n");
	/*case3872:use mempool replace static variable*/
	mod_config *cfg = mod_config_pool_alloc();
	cfg->is_start_helper = 1;
	cc_register_mod_param(mod, cfg, free_cfg);
	return 0;
}

static void parse_programline(wordlist ** line)
{
        if (*line)      
                self_destruct();
        parse_wordlist(line);
}

static void hook_before_sys_init()
{
	debug(188, 1)("(mod_pps) ->     hook_before_sys_init:----:)\n");
	debug(188, 3)("mod_pps: helper process number  =%d \n",helper_proc_num);

        if (media_prefetchers)
        {
                helperShutdown(media_prefetchers);
                helperFree(media_prefetchers);
                media_prefetchers = NULL;
        }

        media_prefetchers = helperCreate("miss_prefetcher");
        wordlist *pwl = NULL;
        char cfg_line2[128];
        strcpy(cfg_line2,"a /usr/local/squid/bin/miss_prefetch.pl");
        strtok(cfg_line2, w_space);
        parse_programline(&pwl);

	media_prefetchers->cmdline = pwl;
        media_prefetchers->n_to_start = helper_proc_num;
        media_prefetchers->concurrency = 0;
        media_prefetchers->ipc_type = IPC_STREAM;
        helperOpenServers(media_prefetchers);

	return;
}

static int hook_init()
{
        debug(188, 1)("(mod_pps) ->     hook_init:----:)\n");

//	if(media_prefetchers == NULL)
//		hook_before_sys_init();
        return 0;
}

static int hook_cleanup(cc_module *module)
{
        debug(188, 1)("(mod_pps) ->     hook_cleanup:\n");
        return 0;
}

static void mediaPrefetchHandleReplyForMissStart(void *data, char *reply)
{                       
//        media_prefetch_state_data* state = data;

        if(strcmp(reply, "OK"))
        {               
                debug(188,3)("mod_pps can not prefetch media! reply=[%s] \n",reply);
        }               
        else{           
                debug(188,3)("mod_pps: quering whole object is success reply=[%s] \n",reply);
        }
                
  //      cbdataFree(state);
}

static int httpHeaderCheckExistByName(HttpHeader *h, char *name)
{
        HttpHeaderPos pos = HttpHeaderInitPos;
        HttpHeaderEntry *curHE;
        while ((curHE = httpHeaderGetEntry(h, &pos)))
        {
                if(!strcmp(strBuf(curHE->name), name))
                {
                        debug(188,3)("Internal X-CC-Prefetch request!\n");
                        return 1;
                }
        }

        debug(188,3)("Not internal X-CC-Prefetch request!\n");
        return 0;
}

static int mod_pps_http_req_process(clientHttpRequest *http)
{
        debug(188, 3)("(mod_pps) ->     mod_pps_http_req_process:----:)\n");

	fde *F = NULL;
	int fd = -1;
	int local_port = -1;
	char *state= NULL;
        static char buf[4096];
	static char tmp_buf[32];
        StoreEntry *e = NULL;
	int has_header = httpHeaderCheckExistByName(&http->request->header, "X-CC-Prefetch");

	mod_config *cfg = (mod_config *)cc_get_mod_param(http->conn->fd, mod);
	if (!cfg) {
		debug(188, 1) ("mod_pps -> mod_pps_http_req_process cc_get_mod_param failed!\n");
		return 0;
	}
	if( http->log_type == LOG_TCP_MISS && cfg->is_start_helper && !has_header) 
	{
		debug(188,3)("mod_pps: request miss_start object because http->log_type=%d\n", http->log_type);
/*
		if(http->request->flags.range == 1)
		{
			debug(188,3)("mod_pps: request has range and goto helper process\n");
			goto r_helper;
		}
*/

		if(http->request->flags.cachable || http->request->flags.range == 1)
			e = storeGetPublicByRequest(http->request);
		else
			return 0;

                if(e == NULL){//query whole object
			
			debug(188, 2) ("mod_pps: storeEntry is NULL, query whole obj now!\n");

                        snprintf(buf, 4096, "%s", http->uri);
			
			fd = http->conn->fd;
			F = &fd_table[fd];

			if(!F->flags.open)
				debug(188, 2)("mod_pps: FD is not open!\n");
			
			local_port = F->local_port;

			debug(188, 3) ("mod_pps: http->uri: %s\n", buf);
			debug(188, 3) ("mod_pps: http->port: %d\n", local_port);

			memset(tmp_buf, 0, sizeof(tmp_buf));

			snprintf(tmp_buf, 32, "@@@%d", local_port);
			strcat(buf, tmp_buf);
			strcat(buf, "\n");

			debug(188, 3) ("mod_pps: helper buf: %s\n", buf);

                        helperSubmit(media_prefetchers, buf, mediaPrefetchHandleReplyForMissStart, state);
                }

		if(e == NULL|| e->store_status != STORE_OK){

                        debug(188, 2) ("mod_pps: storeEntry pending and redirect to upper layer to query start data %s\n",http->uri);

                        http->request->flags.cachable = 0;//no cache
                        safe_free(http->request->store_url);
                        //modify_request(http,http->uri);

                        http->entry = NULL;
                        http->log_type = LOG_TCP_MISS;

                }
	}

	return 0;
}

int mod_register(cc_module *module)
{
        debug(188, 1)("(mod_pps) ->     mod_register:\n");

        strcpy(module->version, "7.0.R.16488.i686");

        //module->hook_func_sys_init = hook_init;
        cc_register_hook_handler(HPIDX_hook_func_sys_init,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
                        hook_init);
        //module->hook_func_sys_parse_param = func_sys_parse_param;
        cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
                        func_sys_parse_param);

	//module->hook_func_before_sys_init = hook_before_sys_init;
        cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
                        hook_before_sys_init);

	//module->hook_func_http_req_process = mod_f4v_http_req_process;
        cc_register_hook_handler(HPIDX_hook_func_http_req_process,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
                        mod_pps_http_req_process);

	//module->hook_func_sys_cleanup = hook_cleanup;
        cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
                        hook_cleanup);

	mod = module;

        return 0;
}

#endif

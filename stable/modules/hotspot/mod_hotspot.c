#include "cc_framework_api.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef CC_FRAMEWORK

#define HOTSPOT_LOG_PATH "/data/hotspot/hot_spot.log"
#define HOTSPOT_RESULT_PATH "/data/hotspot/hot_spot.result"

#define MAX_DOMAIN_NUM 100
#define MAX_DOMAIN_LEN 1024
#define MAX_URL_LEN 4096

static char *name = "CC-PRIVATE";
static char *value = "0";

static cc_module *mod = NULL;

static FILE* fp = NULL;
static FILE* fp2 = NULL;

static helper * hotspot_checks = NULL;
static int helper_proc_num = 30;

static FILE* init_log_handle();

/*
char cc_domain[MAX_DOMAIN_LEN];

struct config_domain
{
char cc_domain[MAX_DOMAIN_NUM][MAX_DOMAIN_LEN];
}	
 */
typedef struct _mod_config{
	int cfg_helper_proc_num;  
}mod_config;

static MemPool * mod_config_pool = NULL; 

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_hotspot config_struct", sizeof(mod_config));
	}
	return obj = memPoolAlloc(mod_config_pool);
}
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


typedef struct _fde_data
{       
	char ori_url[MAX_URL_LEN];
	int state;	// 0:ori_state; 1:hot request; -1:not hot;
} fde_data;

typedef void CHRP(clientHttpRequest *);

typedef struct _hotspot_data
{
	clientHttpRequest * http;
	CHRP *callback;
} hotspot_data;

CBDATA_TYPE(hotspot_data);

static void parse_programline(wordlist ** line)
{
        if (*line)
                self_destruct();
        parse_wordlist(line);
}

static int func_sys_parse_param(char *cfg_line)
{
	debug(185,9)("mod_hotspot cfg_line: %s\n", cfg_line);
	char *pos = NULL;

	if((pos = strstr(cfg_line, "n=")) == NULL)
		return 0;

	if(isdigit(cfg_line[pos - cfg_line + 2]))
		helper_proc_num = atoi(cfg_line + (pos - cfg_line + 2));
	mod_config *cfg =mod_config_pool_alloc(); 
	cfg->cfg_helper_proc_num = helper_proc_num;
	cc_register_mod_param(mod, cfg, free_mod_config);
	return 0;
}


static void hook_before_sys_init()
{
	if(!init_log_handle())
	{
		debug(185,3)("(mod_hotspot) --> cc_mod_hotspot hook_before_sys_init failed\n");
		abort();
	}

        debug(185,3)("mod_hotspot: helper process number  =%d \n",helper_proc_num);

        if (hotspot_checks)
        {
                helperShutdown(hotspot_checks);
                helperFree(hotspot_checks);
                hotspot_checks = NULL;
        }

        hotspot_checks = helperCreate("hotspot_checker");
        wordlist *pwl = NULL;
        char cfg_line2[128];
        strcpy(cfg_line2,"a /usr/local/squid/bin/hotspot_check.pl");
        strtok(cfg_line2, w_space);
        parse_programline(&pwl);

        hotspot_checks->cmdline = pwl;
        hotspot_checks->n_to_start = helper_proc_num;

	hotspot_checks->concurrency = 0;
        hotspot_checks->ipc_type = IPC_STREAM;
        helperOpenServers(hotspot_checks);

}

static int free_fde_data(void *data){
        fde_data* fd_data = data;
        if(fd_data != NULL){
                safe_free(fd_data);
        }
        return 0;
}

static FILE* init_log_handle()
{
	struct stat sb;
	if (stat("/data/hotspot", &sb) != 0){
		mkdir("/data/hotspot", S_IRWXU);
	}

	if(fp)
	{
		fflush(fp);
		fclose(fp);
	}
	
	fp = NULL;

	fp = fopen(HOTSPOT_LOG_PATH, "a+");

	fp2 = (fopen(HOTSPOT_RESULT_PATH, "a+"));
	fclose(fp2);

	return fp;
}

/*
static int httpHeaderCheckExistByName(HttpHeader *h, char *name)
{
        HttpHeaderPos pos = HttpHeaderInitPos;
        HttpHeaderEntry *curHE;
        while ((curHE = httpHeaderGetEntry(h, &pos)))
        {
                if(!strcmp(strBuf(curHE->name), name))
                {
                        return 1;
                }
        }

        return 0;
}
*/

// whether the request come from local hotspot process
/*
static int is_local_request(clientHttpRequest * http)
{
	return httpHeaderCheckExistByName(&http->request->header, "X-CC-HOTSPOT-Prefetch");	
}
*/

// add private header to avoid conflict with redirect_location module
static int add_private_header(HttpReply *rep)
{
	HttpHeaderEntry e;
	int len;

	stringInit(&e.name, name);
        len = strlen(name);
        stringInit(&e.value, value);

	e.id = HDR_OTHER;

	httpHeaderAddEntry(&rep->header, httpHeaderEntryClone(&e));

	stringClean(&e.name);
        stringClean(&e.value);

	
	return 0;
}

// whether hit
static int is_hit(clientHttpRequest * http)
{
	StoreEntry *e = NULL;

	e = storeGetPublicByRequest(http->request);

	if(e == NULL)
		return 0;
	else
	{
		if(e->store_status != STORE_OK)
			return 0;
		else
			return 1;
	}
}

static int add_url_to_log(char* url, clientHttpRequest * http)
{
	char buf[4096];
	memset(buf, 0, sizeof(buf));

	snprintf(buf, sizeof(buf), "%ld %s %s\n", squid_curtime, url, (http->request->store_url != NULL)?http->request->store_url:http->request->canonical);

	if(!fp)
		fp = fopen(HOTSPOT_LOG_PATH, "a+");

	fwrite(buf, 1, strlen(buf), fp);
        fflush(fp);

	return 0;
}

static int reply_request_with_302(char* ori_url, clientHttpRequest *http)
{
	char *pos = NULL;
	static char location_url[4096];

	strcpy(location_url, http->uri);
	if((pos = strstr(location_url, "@@@")) != NULL)
		*pos = '\0';

	http->redirect.status = 302;
	http->redirect.location = xstrdup(location_url);

	HttpReply *rep = httpReplyCreate();
	http->entry = clientCreateStoreEntry(http, http->request->method, http->request->flags);
#if LOG_TCP_REDIRECTS
	http->log_type = LOG_TCP_REDIRECT;
#endif
	storeReleaseRequest(http->entry);
	httpRedirectReply(rep, http->redirect.status, http->redirect.location);

	//add private header
	add_private_header(rep);
	//

	httpReplySwapOut(rep, http->entry);
	storeComplete(http->entry);

	return	0;
}

static int add_ori_url_to_private_data(clientHttpRequest *http)
{
	debug(185, 8)("mod_hotspot--> after client_access_check_done, client request: %s\n", http->uri);

	fde_data* fd_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &(http->conn->fd), mod);
	if(fd_data == NULL){
		fd_data = xmalloc(sizeof(fde_data));
		memset(fd_data,0,sizeof(fde_data));
		cc_register_mod_private_data(FDE_PRIVATE_DATA, &(http->conn->fd), fd_data, free_fde_data, mod);
	}

	strcpy(fd_data->ori_url, http->uri);
	fd_data->state = 0;

	return 0;
}

static void hotspotHandleReply(void *data, char *reply)
{               
        hotspot_data* hotdata = data;

	fde_data* fd_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &(hotdata->http->conn->fd), mod);

        if(fd_data == NULL)
        {
                debug(185, 5)("mod_hotspot--> get fd private data failed\n");
		return;
        }

	if(reply == NULL)
	{

                debug(185,1)("mod_hotspot reply is null \n");
                fd_data->state = -1;

        }
        else{
                if(strcmp(reply, "OK"))
                {
                        debug(185,6)("mod_hotspot request:[%s] is not a hot request: reply=[%s] \n", hotdata->http->uri, reply);
                        fd_data->state = -1;
                }
                else
		{
                        debug(185,6)("mod_hotspot request:[%s] is a hot request: reply=[%s] \n", hotdata->http->uri, reply);
                        fd_data->state = 1;
                }
        }

        int valid = cbdataValid(hotdata->http);
        cbdataUnlock(hotdata->http);
        cbdataUnlock(hotdata);
        if(valid)
        {
                hotdata->callback(hotdata->http);
        }
        cbdataFree(hotdata);
}

static int hook_http_req_process(clientHttpRequest * http)
{
	debug(185, 5)("mod_hotspot--> http->uri: %s\n", http->uri);

	fde_data* fd_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &(http->conn->fd), mod);

	if(fd_data == NULL)
	{
		debug(185, 5)("mod_hotspot--> get fd private data failed\n");
		return 0;
	}
	else
		debug(185, 5)("mod_hotspot--> ori_http_request: %s\n", fd_data->ori_url);

	//not hot requset ???
	if(fd_data->state == -1)
	{
		debug(185, 3)("mod_hotspot--> in hook_http_req_process: %s is not a hot request\n", fd_data->ori_url);

		if(reply_request_with_302(NULL, http) == 0)
		{
			debug(185, 3)("reply_request_with_302 success.\n");
			return 1;
		}
		
	}
	else if(fd_data->state == 1)
	{
		debug(185, 3)("mod_hotspot--> in hook_http_req_process: %s is a hot request\n", fd_data->ori_url);
		return 0;
	}

	//below process is first in the function

	static char url[4096];
	static char ori_url[4096];

	memset(url, 0, sizeof(url));
	memset(ori_url, 0, sizeof(ori_url));

	//url is ori url
	strcpy(url, http->uri);
	strcpy(ori_url, fd_data->ori_url);
	
	if(http->request->method != METHOD_GET)
	{
		debug(185, 3)("mod_hotspot--> request: %s method is not GET.\n", http->uri);
		return 0;
	}

	/*
	if(is_local_request(http))
	{
		debug(185, 3)("mod_hotspot--> request: %s is from local.\n", http->uri);
		return 0;
	}
	*/

	if(is_hit(http))
	{
		debug(185, 3)("mod_hotspot--> request with store_url: [%s] is HIT from FC.\n", (http->request->store_url != NULL)?http->request->store_url:http->request->canonical);

		return 0;
	}
	else
	{
		
		debug(185, 5)("mod_hotspot--> request with store_url: [%s] is MISS from FC.\n", (http->request->store_url != NULL)?http->request->store_url:http->request->canonical);

	}

	add_url_to_log(ori_url, http);

	//check if the request whether hot
	CBDATA_INIT_TYPE(hotspot_data);
	hotspot_data *state = cbdataAlloc(hotspot_data);
	state->http = http;
	state->callback = clientProcessRequest;

	cbdataLock(state);
	cbdataLock(state->http);

	static char buf[4096];
	snprintf(buf, 4096, "%s\n", (http->request->store_url != NULL)?http->request->store_url:http->request->canonical);

	helperSubmit(hotspot_checks, buf, hotspotHandleReply, state);

	return 1;
}

static int hook_init(cc_module *module)
{
	debug(185,3)("(mod_hotspot) --> cc_mod_hotspot hook_init\n");

	/* hotspot init */
/*

	if(!init_log_handle())
	{
		debug(185,3)("(mod_hotspot) --> cc_mod_hotspot hook_init failed\n");
		abort();
	}
*/

	return 1;
}

static int cleanup(cc_module *module)
{
        debug(185,3)("(mod_refresh) --> cc_mod_hotspot cleanup\n");

	if(fp)
	{
		fflush(fp);
		fclose(fp);
	}
        return 1;
}

/*
static int hook_http_read_reply(int fd,HttpStateData *data)
{
	debug(185,3)("(mod_hotspot) --> cc_mod_hotspot hook_http_read_reply\n");
	StoreEntry *entry = data->entry;
	const request_t *request = data->request;

	if(flag){
		const char *key = storeKeyText(entry->hash.key);
		const char *url = storeUrl(entry);
		debug(91,3) ("rf_add_url -- key : %s,url : %s\n",key,url);
		rf_add_url(key,url);
	}
	return 1;
}
*/

/* module init */
int mod_register(cc_module *module)
{
	debug(185,3)("(mod_hotspot) --> cc_mod_hotspot init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	/* necessary functions*/
	//module->hook_func_sys_cleanup = cleanup;

	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanup);

	//module->hook_func_sys_parse_param = func_sys_parse_param;
        cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
                        func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
			hook_before_sys_init);

	//module->hook_func_sys_init = hook_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			hook_init);

	cc_register_hook_handler(HPIDX_hook_func_client_access_check_done,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_access_check_done),
                        add_ori_url_to_private_data);

	//module->hook_func_private_http_req_process(clientHttpRequest *http) = hook_http_req_process;
        cc_register_hook_handler(HPIDX_hook_func_private_before_http_req_process,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_before_http_req_process),
                        hook_http_req_process);


	/*
	//module->hook_func_http_repl_read_start = hook_http_read_reply;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_read_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_start),
			hook_http_read_reply);
	*/
	
	mod = module;

	return 1;
}

#endif

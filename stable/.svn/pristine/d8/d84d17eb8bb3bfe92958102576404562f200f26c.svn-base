#include "mod_billing_cal.h"

#define BILLLINGD_PIDFILE "/var/run/cc_billingd.pid"
#define BILLINGD_PATH "/usr/local/squid/bin/billingd"
static pid_t get_running_pid(void);
static int start_billingd(void);
//static void reload_billingd(void);
static cc_module *mod = NULL;
extern int inited;

typedef struct _req_param
{
    char billing_host[256];
}request_param;
struct mod_conf_param *dir_config = NULL;
static MemPool *mod_config_pool = NULL;

// allocate modules paramteter
static void *mod_config_pool_alloc(void)
{
    void * obj = NULL;
    if (NULL == mod_config_pool)
    {   
        mod_config_pool = memPoolCreate("mod_billing config_struct", sizeof(struct mod_conf_param));
    }   
    return obj = memPoolAlloc(mod_config_pool);
}

// free modules parameter
static int free_callback(void* param)
{
    struct mod_conf_param* data = (struct mod_conf_param*) param;
    if(data)
    {          
        if (data->channel)
            wordlistDestroy(&data->channel);
        memPoolFree(mod_config_pool, data);
        data = NULL; 
    }          
    return 0;
}


// mod_billing filter black_list_path on
static int func_sys_parse_param(char *cfg_line)
{
    if (inited == true) // reload
    {
        debug(92,3)("mod_billing: reparse billing cfg when reload [%s]\n", cfg_line); 
        // reload_billingd();      /* Reconfigure billingd if FC was reloaded */
    }
    else
        debug(92, 3)("mod_billing: the config line is [%s]\n", cfg_line);
    struct mod_conf_param *data = mod_config_pool_alloc();
    memset(data,0,sizeof(struct mod_conf_param));
    dir_config = data;

    char *token = NULL;
    if ( NULL == (token = strtok(cfg_line, w_space)) || strcasecmp(token, "mod_billing") )
        goto end;

    if ( NULL != (token = strtok(NULL, w_space)) )
        strcpy(data->type, token);
    else
        goto end;

    if (!strcmp(data->type,"on"))
    {
        goto fin;
    }
    else if (!strcmp(data->type,"filter"))
    {
        if ( NULL != (token = strtok(NULL, w_space)) )
        {
            strcpy(data->black_list_path, token);
            parse_list();
        }
        else
            goto end;
    }
    else if (!strcmp(data->type,"before_url_rewrite"))
    {
        if (NULL == (token = strtok(NULL, w_space)) || strcasecmp(token, "on"))
            goto end;
    }
    else        // to be extended...
        goto end;

fin:
    cc_register_mod_param(mod, data, free_callback);
    debug(92,3)("[mod_billing %s] parse success\n", data->type);
    return 0;

end:
    debug(92,1)("[mod_billing %s] parse fail, take default value [mod_billing on]\n",data->type);
    free_callback(data);
    dir_config = mod_config_pool_alloc();
    strcpy(dir_config->type,"on");
    cc_register_mod_param(mod, dir_config, free_callback);
    return 0;
}


//回源站bind
static int hook_req_send(HttpStateData* httpState)
{
	assert(httpState);
	assert(httpState->request);

	debug(92, 3)("(mod_billing) -> hook_req_send: start hook_req_send\n");

	int fd = httpState->fd;
	request_t *req = httpState->request;

	http_hdr_type id;
	HttpHeaderEntry *findname = NULL;

	int islocal = 0;
	if (strcmp("127.0.0.1", inet_ntoa(req->client_addr)) == 0)
	{
		String xff = httpHeaderGetList(&req->header, HDR_X_FORWARDED_FOR);
		if(!strLen(xff) || strStr(xff, "127.0.0.1"))
		{
			billing_bind(req->host, fd, LOCAL_SOURCE);
			islocal = 1;
		}
		else
		{
			islocal = 0;
		}
		stringClean(&xff);
	}
	else
	{
		islocal = 0;
	}
	
	if(!islocal)
	{
		id = httpHeaderIdByNameDef("User-Agent", strlen("User-Agent"));
		findname = httpHeaderFindEntry(&(req->header), id);

	   	if (findname && strstr(findname->value.buf, "powered-by-chinacache"))
		{
			billing_bind(req->host, fd, FC_SOURCE);
		}
		else
		{
			billing_bind(req->host, fd, REMOTE_SOURCE);
		}
	}

	debug(92, 3)("(mod_billing) -> hook_req_send: end hook_req_send\n");

	return 0;
}


//客户端bind
static int hook_req_read(clientHttpRequest *http)
{
	assert(http);
	assert(http->conn);
//	assert(http->uri);
	if(NULL == http->uri)
	{
		debug(92,0)("(mod_billing) -> hook_req_read: http->url =%s\n",http->uri);
		return 0;
	}

	debug(92, 3)("(mod_billing) ->	hook_req_read: start uri = %s\n", http->uri);

	char billing_host[256] = {0};
	ConnStateData *conn = http->conn;
    url2host(billing_host, http->uri);

    /* billing host */
    if (!strcmp(dir_config->type,"before_url_rewrite"))
    {
        request_param *req_param;
        if ( NULL != (req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod)))
        {
            strcpy(billing_host,req_param->billing_host);
            debug(92,3)("(mod_billing) -> hook_req_read: use billing host before url rewrite[%s]\n",billing_host);
        }
    }
    /* billing host */

	http_hdr_type id;
	HttpHeaderEntry *findname = NULL;

	int islocal = 0;
	if (strcmp("127.0.0.1", inet_ntoa(conn->peer.sin_addr)) == 0)
	{
		String xff = httpHeaderGetList(&http->request->header, HDR_X_FORWARDED_FOR);
		if(!strLen(xff) || strStr(xff, "127.0.0.1"))

		{
			billing_bind(billing_host, conn->fd, LOCAL_CLIENT);
			islocal = 1;
		}
		else
		{
			islocal = 0;
		}
		stringClean(&xff);
	}
	else
	{
		islocal = 0;
	}
	
	if(!islocal)
	{
		id = httpHeaderIdByNameDef("User-Agent", strlen("User-Agent"));
		findname = httpHeaderFindEntry(&http->request->header, id);

		if (findname && strstr(findname->value.buf, "powered-by-chinacache"))
		{
			billing_bind(billing_host, conn->fd, FC_CLIENT);
		}
	   	else
		{
			billing_bind(billing_host, conn->fd, REMOTE_CLIENT);
		}
	}

	return 0;
}


//从服务器端读完成，unbind
static int hook_repl_read(int fd, HttpStateData *data)
{
	billing_unbind(fd);
	return 0;
}


//向客户端写完成，unbind
static int hook_repl_send_end(clientHttpRequest *http)
{
	assert(http);
	assert(http->conn);
//FIXME:
//目前在unbind中处理了httprequest是否是异常的
//将来要提到外面来做
	billing_unbind(http->conn->fd);
	return 0;
}


static int hook_fd_close(int fd)
{
	assert(fd >= 0);
	debug(92, 3)("(mod_billing) ->	hook_fd_close: start hook_fd_close\n");
	billing_close(fd);
	return 0;
}


static int hook_fd_bytes(int fd, int len, unsigned int type)
{
	if (FD_READ == type)
	{
		assert(fd >= 0);
		debug(92, 4)("(mod_billing) ->	hook_fd_read: start hook_fd_read\n");
		if( len <= 0 )
			return 0;
		billing_flux_read(fd, len);
	}
	else if (FD_WRITE == type)
	{
		assert(fd >= 0);
		debug(92, 4)("(mod_billing) ->	hook_fd_write: start hook_fd_write\n");
		if( len <= 0 )
			return 0;
		billing_flux_write(fd, len);
	}
	return 0;
}


static void billing_sync_event(void *val)
{
	billing_sync(true);
	eventAdd("mod_billing", billing_sync_event, NULL, 6, 1);
}

static pid_t get_running_pid(void)
{

	static pid_t last_pid = -1;
	char pid[128];
	if( last_pid == -1 ) {
		FILE *fd;
		if( (fd = fopen(BILLLINGD_PIDFILE, "r")) == NULL )
		{
			debug(91, 1)("mod_billing: open [/var/run/cc_billingd.pid] error!\n");
			return last_pid;
		}
		fgets(pid, sizeof(pid), fd);
		fclose(fd);

		last_pid = atoi(pid);
		if( last_pid < 0 ) {
			debug(91, 1)("mod_billing: atoi(pid) = %s from pidfile is wrong!\n", pid);
			last_pid = -1;
			return -1;
		}
	}

	assert(last_pid != -1);

	char buff[1024];
	FILE *procfd;
	snprintf(buff, sizeof(buff), "/proc/%d/status", last_pid);

	if( (procfd = fopen(buff,"r")) == NULL ) {
		last_pid = -1;
		debug(91, 1)("mod_billing: open [%s] error!\n", buff);
		return last_pid;
	}

	fgets(buff, sizeof(buff), procfd);
	fclose(procfd);
	if( strstr(buff, "billingd") == NULL ) {
		debug(91, 1)("mod_billing: No billingd in [%s] line! The file is (/proc/%d/status)\n", buff, last_pid);
		last_pid = -1;
		return last_pid;
	}

	return last_pid;
}

#define BILLLINGD_LOCKFILE "/var/run/billingd.lock"
static void check_billingd()
{
	eventAdd("check_billingd", check_billingd, NULL, 60, 0);

	/* case 3862 ,pidfile lock, for multi-squid running */
	int lock_file;
	lock_file = open(BILLLINGD_LOCKFILE, O_RDWR|O_CREAT|O_EXCL,0400);
	if (lock_file < 0) {
		if (EEXIST == errno)
			debug(91,1)("mod_billing: There is another squid is calling check_billingd(), so stop here\n");
		else
			debug(91,1)("mod_billing: open failed ==> %s\n",strerror(errno));
		return;
	}
	// debug(91,1)("mod_billing: got lock, %d is lunching billingd soon...\n", getpid());

	pid_t pid = get_running_pid();

	if (pid < 0)
	{
		debug(91,0)("(mod_billing) --> Billingd stopped! Now starting it ...\n");

		int ret = start_billingd();
		if (ret != 0)
		{
			debug(91,0)("(mod_billing) --> ERROR : start billingd error!!!\n");
		}
	}

	/* case 3862 ,pidfile lock, for multi-squid running */
	close(lock_file);
	unlink(BILLLINGD_LOCKFILE);
	// debug(91,1)("mod_billing: %d release lock...\n", getpid());
}

/*static void reload_billingd()
{
    pid_t pid = get_running_pid();
    if (pid > 1){
        kill(pid, SIGHUP); 
    }
}*/

static int start_billingd(void)
{
	enter_suid();

	int cid = fork();
	if (cid == 0)   //child
	{
		int ret = execl(BILLINGD_PATH, "billingd","-D",(char *)0);;
		if (ret < 0)
		{
			debug(92,0)("execl error : %s\n",xstrerror());
		}

		exit(-1);
	}
	else if (cid < 0)
	{
		debug(92,0) ("fork error!!\n");
		leave_suid();
		return -1;
	}

	leave_suid();

	debug(92,1)("mod_billing: running billingd, whose [pid = %d]\n",cid+1);
	return 0;
}

static int hook_init()
{
	debug(92, 1)("(mod_billing) ->	hook_init: init module\n");

	debug(92,1)("Billingd stopped! Now starting it ...\n");

	billing_init("127.0.0.1", 8877);
	eventAdd("check_billingd", check_billingd, NULL, 60, 0);
	eventAdd("mod_billing", billing_sync_event, NULL, 6, 1);
	return 0;
}

static int hook_cleanup(cc_module *module)
{
    billing_destroy();
    if (NULL !=  mod_config_pool)
    {
        memPoolDestroy(mod_config_pool);
        mod_config_pool = NULL;
    }
    return 0;
}


/* billing host */
static MemPool * request_param_pool = NULL;
static void * request_param_pool_alloc(void)
{
    void * obj = NULL;
    if (NULL == request_param_pool)
    {
        request_param_pool = memPoolCreate("mod_billing private_data request_param", sizeof(request_param));
    }
    return obj = memPoolAlloc(request_param_pool);
}
static int free_request_param(void* data)
{
    request_param *req_param = (request_param *) data;
    if(req_param != NULL){
        memPoolFree(request_param_pool, req_param);
        req_param = NULL;
    }   
    return 0;
}

static int hook_req_read_before(clientHttpRequest *http)
{
    if (strcmp(dir_config->type,"before_url_rewrite"))
        return 0;
    debug(92, 3)("(mod_billing) -> hook_req_send_before: hook_req_rend_beforebilling_host\n");
    request_param *req_param = request_param_pool_alloc();
    url2host(req_param->billing_host, http->uri);
    cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, req_param, free_request_param, mod);
    debug(92, 3)("(mod_billing) -> hook_req_send_before: end hook_req_rend_before, billing_host[%s]\n",req_param->billing_host);
    return 0;
}
/* billing host */

/* module init */
int mod_register(cc_module *module)
{
	debug(92, 1)("(mod_billing) ->	init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");
	
	//module->hook_func_sys_init = hook_init;
    cc_register_hook_handler(HPIDX_hook_func_sys_init,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init), 
            hook_init);
    //module->hook_func_sys_parse_param = func_sys_parse_param
    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
            func_sys_parse_param);
    //module->hook_func_sys_cleanup = hook_cleanup;
    cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);
	//module->hook_func_http_req_send = hook_req_send;
	cc_register_hook_handler(HPIDX_hook_func_http_req_send,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send),
			hook_req_send);
	//module->hook_func_http_req_read = hook_req_read;
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			hook_req_read);
	//module->hook_func_http_repl_send_end = hook_repl_send_end;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_end,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_end),
			hook_repl_send_end);
	//module->hook_func_http_repl_read_end = hook_repl_read;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_read_end,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_end),
			hook_repl_read);
	//module->hook_func_fd_close = hook_fd_close;
	cc_register_hook_handler(HPIDX_hook_func_fd_close,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fd_close),
			hook_fd_close);
	//module->hook_func_fd_bytes = hook_fd_bytes;
	cc_register_hook_handler(HPIDX_hook_func_fd_bytes,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fd_bytes),
			hook_fd_bytes);
    //module->hook_func_http_req_read = hook_req_read;
    cc_register_hook_handler(HPIDX_hook_func_private_http_before_redirect_for_billing,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_before_redirect_for_billing),
            hook_req_read_before);
   // reconfigure
    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else
        mod = module;

	return 0;
}




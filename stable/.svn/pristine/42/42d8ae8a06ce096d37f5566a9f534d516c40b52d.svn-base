#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;

struct time_stat
{
	time_t cur_time;
	char times[1024];
};
static MemPool * time_stat_pool = NULL;

static void * time_stat_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == time_stat_pool)
	{
		time_stat_pool = memPoolCreate("mod_time_stat private_data time_stat", sizeof(struct time_stat));
	}
	return obj = memPoolAlloc(time_stat_pool);
}

static int free_private_data_func(void * data)
{
	struct time_stat * stat = data;
	if(NULL != stat)
	{
		memPoolFree(time_stat_pool, stat);
		stat = NULL;
	}
	return 0;
}
static int add_time_tag(int fd, char *hook_name)
{	
	struct time_stat *stat;
	char tag[64] = {0};
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	
	stat = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	snprintf(tag, 64, "%s-%ld : ", hook_name, (cur_time.tv_sec*1000000+cur_time.tv_usec - stat->cur_time));
	debug(99, 2)("(mod_time_stat) -> add_time_tag: tags=%s\n", tag);
	stat->cur_time = cur_time.tv_sec*1000000+cur_time.tv_usec;
	strcat(stat->times, tag);
	return 1;	
}


static int func_fd_close(int fd)
{
	struct time_stat *stat;
	char tag[64] = {0};
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);

	stat = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	if (stat)
	{
		snprintf(tag, 64, "FD_CLOSE-%ld : ", cur_time.tv_sec*1000000+cur_time.tv_usec-stat->cur_time);
		strcat(stat->times, tag);	
		debug(99, 3)("(mod_time_stat) -> func_fd_close: %d time %s\n", fd, stat->times);
	}
	return 1;	
}

//accept
static int func_private_http_accept(int fd)
{
	struct time_stat *stat;
	int ret;
	char *tag = "HTTPACCEPT-0 : ";
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);

	stat =time_stat_pool_alloc();
	stat->cur_time = cur_time.tv_sec*1000000+cur_time.tv_usec;
	strncpy(stat->times, tag, strlen(tag));
	//ret = cc_register_mod_private_data(FDE_PRIVATE_DATA, &fd, stat, NULL, mod);
	ret = cc_register_mod_private_data(FDE_PRIVATE_DATA, &fd, stat, free_private_data_func, mod);
	debug(99, 1)("(mod_time_stat) -> func_private_http_accept: fd=%d time=%ld tag=%s\n", fd, stat->cur_time, stat->times);
	return 1;	
}


//read req from client
static int func_http_req_read(clientHttpRequest *http)
{
	add_time_tag(http->conn->fd, "REQ_READ");
	return 1;	
}


//send req to orignal server
static int func_http_req_send(HttpStateData *data)
{
	add_time_tag(data->fd, "REQ_SEND");
	return 1;	
}


//start read reply from orignal server
static int func_http_repl_read_start(int fd, HttpStateData *data)
{
	add_time_tag(fd, "REPL_READ_START");
	return 1;	
}


//end read reply form orignal server
static int func_http_repl_read_end(int fd, HttpStateData *data)
{
	add_time_tag(fd, "REPL_READ_END");
	return 1;	
}


//start send reply to client
static int func_http_repl_send_start(clientHttpRequest *http)
{
	add_time_tag(http->conn->fd, "REPL_SEND_START");
	return 1;	
}


//send end reply to client
static int func_http_repl_send_end(clientHttpRequest *http)
{
	add_time_tag(http->conn->fd, "REPL_SEND_END");
	return 1;	
}

static int hook_cleanup(cc_module *module)
{                       
	debug(94, 1)("(mod_time_stat) ->     hook_cleanup:\n");
	if(NULL != time_stat_pool)
	{
		memPoolDestroy(time_stat_pool);
	}
	return 0;       
}       

/* module init */
int mod_register(cc_module *module)
{
	debug(94, 1)("(mod_time_stat) ->  init: init module\n");

	strcpy(module->version, "5.5.6030.i686");
	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	//mod->hook_func_private_http_accept = func_private_http_accept; 
	cc_register_hook_handler(HPIDX_hook_func_private_http_accept,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_accept),
			func_private_http_accept);

	//mod->hook_func_http_req_read = func_http_req_read;
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			func_http_req_read);

	//mod->hook_func_http_req_send = func_http_req_send;
	cc_register_hook_handler(HPIDX_hook_func_http_req_send,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send),
			func_http_req_send);

	//mod->hook_func_http_repl_read_start = func_http_repl_read_start;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_read_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_start),
			func_http_repl_read_start);

	//mod->hook_func_http_repl_read_end = func_http_repl_read_end;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_read_end,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_end),
			func_http_repl_read_end);

	//mod->hook_func_http_repl_send_start = func_http_repl_send_start;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			func_http_repl_send_start);

	//mod->hook_func_http_repl_send_end = func_http_repl_send_end;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_end,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_end),
			func_http_repl_send_end);

	//mod->hook_func_fd_close = func_fd_close;
	cc_register_hook_handler(HPIDX_hook_func_fd_close,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fd_close),
			func_fd_close);
	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);
	return 0;
}
#endif

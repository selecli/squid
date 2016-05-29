#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
#define DEBUG_LOG_PATH "/data/proclog/log/squid/debug.log"

typedef struct _debuglog_http_private_data{
	struct timeval firstbyte;
	struct timeval acltime;
	unsigned long long start_serial;
	unsigned long long acl_serial;
	squid_off_t head_bytes;
	squid_off_t body_bytes;
	struct {
		unsigned int not_complete:1;  
	}flags;

	NOCACHE_REASON ncr;
} debuglog_http_private_data;

typedef struct _debuglog_memobject_private_data{
	NOCACHE_REASON ncr;
} debuglog_memobject_private_data;

struct mod_conf_param {
	int time;
};

static MemPool * mod_config_pool = NULL;

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data)
	{
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return 0;
}


static cc_module* mod = NULL;
static Logfile* debuglog = NULL;
static MemPool * http_private_data_pool;
static MemPool * memobject_private_data_pool;
static int cur_epoll_load;
static int client_reads;
static int client_writes;
static int server_reads;
static int server_writes;
static int accepts;

static int cur_dir_load;
static int filemap_time;
//static int cur_event_load;

static debuglog_http_private_data* makeSureGetHttpPrivateData(clientHttpRequest *http);
static debuglog_memobject_private_data * makeSureGetMemObjectPrivateData(MemObject *mem);

static int func_sys_init()
{
	debuglog = logfileOpen(DEBUG_LOG_PATH, 0, 1);
	return 0;
}
	
static int cleanup(cc_module *mod)
{
	logfileClose(debuglog);
	debuglog = NULL;
	return 0;
}
static int http_req_debug_log(clientHttpRequest *http)
{
	if(!http->entry || !http->entry->mem_obj)
	{
		return 0;
	}
	
	debuglog_http_private_data *http_private_data = makeSureGetHttpPrivateData(http);
	debuglog_memobject_private_data *memobject_private_data = makeSureGetMemObjectPrivateData(http->entry->mem_obj);

	debug(201, 3)("printing from mem_obj %p\n", memobject_private_data);
	
	int firstbyte = tvSubMsec(http->start, http_private_data->firstbyte);
	int acltime = tvSubMsec(http->start, http_private_data->acltime);
	int response = http->al.cache.msec;
	const char * nocache_reason;
	squid_off_t head_bytes = http_private_data->head_bytes;
	squid_off_t body_bytes = http_private_data->body_bytes;
	debug(201, 3)(" %" PRINTF_OFF_T "- %" PRINTF_OFF_T "\n", head_bytes,body_bytes);
	if (NCR_UNKNOWN != http_private_data->ncr)
	{
		nocache_reason = NOCACHE_REASON_STR[http_private_data->ncr];
		 
	}
	else
		nocache_reason = NOCACHE_REASON_STR[memobject_private_data->ncr];
	
	logfileLineStart(debuglog);
	//timestamp hit_or_miss/http_status url acltime firstbyte response | nocache_reason | cur_epoll_load = client_reads+client_writes+server_reads+server_writes+accepts cur_dir_load | acl_matches acl_adds acl_deletes | filemap_time
	logfilePrintf(debuglog, "%" PRINTF_OFF_T ".%" PRINTF_OFF_T " %s/%d %s %d %d %d | %s | %d=%d+%d+%d+%d+%d %d | %d %d %d | %d %" PRINTF_OFF_T "-%" PRINTF_OFF_T "\n",
			(squid_off_t)current_time.tv_sec,		//timestamp_sec
			(squid_off_t)current_time.tv_usec/1000,		//timestamp_msec
			log_tags[http->al.cache.code],			//hit_or_miss
			http->al.http.code,				//http_status
			rfc1738_escape_unescaped(http->al.url),		//url
			acltime,					//acltime
			firstbyte,					//firstbyte
			response,					//response
			nocache_reason,					//nocache_reason
			cur_epoll_load,					//cur_epoll_load
			client_reads,					//client_reads
			client_writes,					//client_writes
			server_reads,					//server_reads
			server_writes,					//server_writes
			accepts,					//accepts
			cur_dir_load,					//cur_dir_load
			http->request->cc_acl_matches,			//acl_matches
			http->request->cc_acl_adds,			//acl_adds
			http->request->cc_acl_deletes,			//acl_deletes
			filemap_time,
			head_bytes,
			body_bytes);
	
	logfileLineEnd(debuglog);

	filemap_time = 0;
	return 0;
}

static int free_debuglog_http_private_data(void *data)
{
	debuglog_http_private_data *private_data = data;
	memPoolFree(http_private_data_pool, private_data);
	return 0;
}

static int free_debuglog_memobject_private_data(void *data)
{
	debuglog_memobject_private_data *private_data = data;
	memPoolFree(memobject_private_data_pool, private_data);
	return 0;
}

static debuglog_http_private_data* makeSureGetHttpPrivateData(clientHttpRequest *http)
{
	if(!http_private_data_pool)
	{
		http_private_data_pool = memPoolCreate("mod_debug_log private_data http_private_data", 
				sizeof(debuglog_http_private_data));
	}
	debuglog_http_private_data *private_data = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if(!private_data)
	{
		private_data = memPoolAlloc(http_private_data_pool);
		cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, private_data, 
				free_debuglog_http_private_data, mod);
	}

	return private_data;
	
}

static  struct mod_conf_param* makeSureGetConfPrivateData(int fd)
{

	struct mod_conf_param *private_data = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	if(!private_data)
	{
		private_data = memPoolAlloc(mod_config_pool);
		cc_register_mod_param(mod, private_data, free_callback);
	}

	return private_data;
	
}


static debuglog_memobject_private_data* makeSureGetMemObjectPrivateData(MemObject *mem)
{
	if(!memobject_private_data_pool)
	{
		memobject_private_data_pool = memPoolCreate(
				"mod_debug_log private_data memobject_private_data", 
				sizeof(debuglog_memobject_private_data));
	}
	debuglog_memobject_private_data *private_data = cc_get_mod_private_data(MEMOBJECT_PRIVATE_DATA, mem, mod);
	if(!private_data)
	{
		private_data = memPoolAlloc(memobject_private_data_pool);
		cc_register_mod_private_data(MEMOBJECT_PRIVATE_DATA, mem, private_data, 
				free_debuglog_memobject_private_data, mod);
	}

	return private_data;
	
}

int http_repl_send_start(clientHttpRequest *http)
{
	debuglog_http_private_data *private_data = makeSureGetHttpPrivateData(http);
	private_data->firstbyte = current_time;
	return 0;
}
int http_before_redirect(clientHttpRequest *http)
{
	debuglog_http_private_data *private_data = makeSureGetHttpPrivateData(http);

	struct timeval acltime;
#if GETTIMEOFDAY_NO_TZP
	gettimeofday(&acltime);
#else
	gettimeofday(&acltime, NULL);
#endif
	private_data->acltime = acltime;

	private_data->acl_serial = comm_select_serial;
	return 0;
}

void http_created(clientHttpRequest *http)
{
	debug(201,3)("http_created\n");
	debuglog_http_private_data *private_data = makeSureGetHttpPrivateData(http);
	private_data->firstbyte = current_time;
	private_data->firstbyte.tv_usec -= 1000;
	private_data->start_serial = comm_select_serial;
}

void httpreq_no_cache_reason(clientHttpRequest *http, NOCACHE_REASON ncr)
{
	
	debuglog_http_private_data *private_data = makeSureGetHttpPrivateData(http);
	if (NCR_UNKNOWN != private_data->ncr)
		return;
	private_data->ncr = ncr;
}

void httprep_no_cache_reason(MemObject *mem, NOCACHE_REASON ncr)
{
	debuglog_memobject_private_data *private_data = makeSureGetMemObjectPrivateData(mem);
	private_data->ncr = ncr;
}

void store_no_cache_reason(StoreEntry *e, NOCACHE_REASON ncr)
{
	if(!e->mem_obj)
	{
		debug(201,3)("store_no_cache_reason not recorded %s\n", 
			NOCACHE_REASON_STR[ncr]);
		return;
	}
	
	MemObject *mem = e->mem_obj;
	debuglog_memobject_private_data *private_data = makeSureGetMemObjectPrivateData(mem);
	
	if(private_data->ncr != NCR_NONE && private_data->ncr != NCR_UNKNOWN)
	{
		return;
	}

	debug(201,3)("store_no_cache_reason recorded %s to %p\n",
			NOCACHE_REASON_STR[ncr], private_data);

	private_data->ncr = ncr;
}

void set_epoll_load(int num, struct epoll_event *events)
{
	cur_epoll_load = num;
	client_reads = 0;
	client_writes = 0;
	server_reads = 0;
	server_writes = 0;
	accepts = 0;
	
	int i;
	for( i = 0; i < num ; i ++)
	{
		int is_client = (fd_table[events[i].data.fd].cctype == 0);
		int is_read = events[i].events & EPOLLIN;
		int is_write = events[i].events & EPOLLOUT;
		int is_accept = 0;
		if(!is_read && !is_write)
		{
			is_accept = 1;
		}
		
		if(is_client && is_read)
		{
			client_reads ++;
		}
		else if(is_client && is_write)
		{
			client_writes ++;
		}
		else if(!is_client && is_read)
		{
			server_reads ++;
		}
		else if(!is_client && is_write)
		{
			server_writes ++;
		}
		else
		{
			accepts ++;
		}
	}
}

void clear_dir_load()
{
	cur_dir_load = 0;
}

void add_dir_load()
{
	cur_dir_load ++;
}

void set_filemap_time(int count)
{
	filemap_time = count;
}

/*parse config line*/
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data = NULL;
	char* token = NULL;

	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_debug_log"))
			goto err;	
	}
	else
	{
		debug(201, 3)("(mod_debug_log) ->  parse line error\n");
		goto err;	
	}	

	if(NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_debug_log config_struct", sizeof(struct mod_conf_param));
	}

	data = memPoolAlloc(mod_config_pool);

	while (NULL != (token = strtok(NULL, w_space)))
	{
		if(!strcmp(token, "cross"))
		{
			if(NULL != (token = strtok(NULL, w_space)))
			{
				data->time = atoi(token);
				debug(201, 1)("(mod_debug_log) -> data->time[%d]\n",data->time);
			}
			else
			{
				debug(201, 1)("(mod_debug_log) ->  parse line error\n");
				goto err;
			}
		}
		else if(!strcmp(token, "allow") || !strcmp(token, "deny") || !strcmp(token, "on"))	
		{
			break;
		}
		else
		{
			debug(201, 1)("(mod_debug_log) -> parse line eeror\n");
			goto err;
		}

	}
	
	cc_register_mod_param(mod, data, free_callback);
	return 0;		
err:
	free_callback(data);
	return -1;
}

//send end reply to client
static void func_http_done_precondition(clientHttpRequest *http,int *precondition)
{
	debuglog_http_private_data *private_data = makeSureGetHttpPrivateData(http);
	int cross = tvSubMsec(http->start,current_time);
	HttpReply *rep = http->reply; 
	struct mod_conf_param* tmp = makeSureGetConfPrivateData(http->conn->fd);
	debug(201,5)("mod_debug_log cross[%d],tmp->time[%d] fd[%d]\n",cross,tmp->time,http->conn->fd);
	if(tmp->time * 1000 > cross )
	{
		private_data->body_bytes = http->out.offset - rep->hdr_sz;
		private_data->head_bytes = rep->hdr_sz; 
		private_data->flags.not_complete = 1;
	}
}

static int hook_func_http_repl_send_end(clientHttpRequest *http)
{
	HttpReply *rep = http->reply; 
	debuglog_http_private_data *private_data = makeSureGetHttpPrivateData(http);
	if(private_data->flags.not_complete == 0)
	{
		private_data->body_bytes = http->out.offset - rep->hdr_sz;
		private_data->head_bytes = rep->hdr_sz; 
	}
	return 1;
}

// module init 
int mod_register(cc_module *module)
{
	debug(201, 1)("(mod_debug_log) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_sys_init = func_sys_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			func_sys_init);
	//module->hook_func_sys_cleanup = cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanup);
	//module->hook_func_http_req_debug_log = http_req_debug_log;
	cc_register_hook_handler(HPIDX_hook_func_http_req_debug_log,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_debug_log),
			http_req_debug_log);
	//module->hook_func_http_before_redirect = http_before_redirect;
	cc_register_hook_handler(HPIDX_hook_func_http_before_redirect,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
			http_before_redirect);
	//module->hook_func_repl_send_start = http_repl_send_start;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			http_repl_send_start);
	//module->hook_func_http_create = http_create;
	cc_register_hook_handler(HPIDX_hook_func_http_created,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_created),
			http_created);
	//module->hook_func_http_no_cache_reason = httprep_no_cache_reason;
	cc_register_hook_handler(HPIDX_hook_func_httprep_no_cache_reason,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_httprep_no_cache_reason),
			httprep_no_cache_reason);

	//module->hook_func_http_no_cache_reason = httpreq_no_cache_reason;
	cc_register_hook_handler(HPIDX_hook_func_httpreq_no_cache_reason,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_httpreq_no_cache_reason),
			httpreq_no_cache_reason);

	//module->hook_func_store_no_cache_reason = store_no_cache_reason;
	cc_register_hook_handler(HPIDX_hook_func_store_no_cache_reason,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_store_no_cache_reason),
			store_no_cache_reason);

	//module->hook_func_set_epoll_load = set_epoll_load;
	cc_register_hook_handler(HPIDX_hook_func_set_epoll_load,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_set_epoll_load),
			set_epoll_load);
	
	//module->hook_func_clear_dir_load = clear_dir_load;
	cc_register_hook_handler(HPIDX_hook_func_clear_dir_load,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_clear_dir_load),
			clear_dir_load);
	//module->hook_func_add_dir_load = add_dir_load;
	cc_register_hook_handler(HPIDX_hook_func_add_dir_load,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_add_dir_load),
			add_dir_load);
	//module->hook_func_set_filemap_time = set_filemap_time;
	cc_register_hook_handler(HPIDX_hook_func_set_filemap_time,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_set_filemap_time),
			set_filemap_time);
	cc_register_hook_handler(HPIDX_hook_func_transfer_done_precondition,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_transfer_done_precondition),
			func_http_done_precondition);
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_end,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_end),
			hook_func_http_repl_send_end);
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

//	mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif

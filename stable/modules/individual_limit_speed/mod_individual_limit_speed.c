#include "cc_framework_api.h"
#include "mod_individual_limit_speed.h"

#define CC_DEFER_FD_MAX SQUID_MAXFD

static cc_module* mod = NULL;
static int limit_by_host = 0;
static long g_iLimitSpeedTimer = 0;
static struct cc_defer_fd_st g_stDeferWriteFd[CC_DEFER_FD_MAX];

static MemPool * mod_config_pool;
static MemPool * individual_speed_control_pool;
static MemPool * limit_speed_private_data_pool;
static MemPool * speed_control_pool;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool) {
		mod_config_pool = memPoolCreate("mod_limit_speed cfg_struct", sizeof(mod_config));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_mod_config(void *data)
{
	mod_config* cfg = (mod_config*)data;
	if (cfg) {
		if(cfg->limit_speed_individual) {
			memPoolFree(individual_speed_control_pool, cfg->limit_speed_individual);
			cfg->limit_speed_individual = NULL;
		}
		if (cfg->host_list){
			wordlistDestroy(&cfg->host_list);
			cfg->host_list = NULL;	
		}
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	return 0;
}

static void * individual_speed_control_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == individual_speed_control_pool) {
		individual_speed_control_pool = memPoolCreate("mod_limit_speed cfg_struct individual_speed_ctl", sizeof(individual_speed_control));
	}
	return obj = memPoolAlloc(individual_speed_control_pool);
}

static void * limit_speed_private_data_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == limit_speed_private_data_pool) {
		limit_speed_private_data_pool = memPoolCreate("mod_limit_speed private_data limit_speed_private_data", sizeof(limit_speed_private_data));
	}
	return obj = memPoolAlloc(limit_speed_private_data_pool);
}

static void * speed_control_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == speed_control_pool) {
		speed_control_pool = memPoolCreate("mod_limit_speed private_data speed_control", sizeof(speed_control));
	}
	return obj = memPoolAlloc(speed_control_pool);
}

static int free_limit_speed_control(void *data)
{

	limit_speed_private_data* private_data = (limit_speed_private_data*) data;
	if(private_data !=NULL) {
		speed_control* sc = private_data->sc;
		if(sc!=NULL) {
			memPoolFree(speed_control_pool, sc);
			sc = NULL;
		}
		memPoolFree(limit_speed_private_data_pool, private_data);
		private_data = NULL;
	}
	return 0;
}

/* case 4210 */
static int list_exist(wordlist *list, struct hash_entry *entry)
{
	wordlist *item = list;
	while (item) {
		if (!strcmp(item->key, entry->host))
			return 1;
		item = item->next;
	}
	return 0;
}

static void parse_list(char *path, wordlist *list)
{
	char config_input_line[256];
	int channel_num = 0;
	FILE *fp = NULL;
	char *ptr;

	if (NULL == (fp = fopen(path, "r")) ) {   
		debug(113,1)("mod_individual_limit_speed: Cannot open file %s\n", path);
		return;
	}   

	while (fgets(config_input_line, BUFSIZ, fp) != NULL) {   
		// ignore black line
		if (NULL == (ptr = strtok(config_input_line, w_space)))
			continue;
		wordlistAdd(&list, ptr);
		channel_num++;
	}   

	debug(113,1)("mod_individual_limit_speed: read %d channel from file %s\n", channel_num, path);

	// for reload
	// Update HashTable
	if (0 == limit_by_host) {
		limit_hashtable_init();
		limit_by_host = 1;
	}

	struct hash_entry **entry_array;
	entry_array = xmalloc(sizeof(struct hash_entry*) * hash_table_entry_count);
	limit_get_all_entry(entry_array);
	int i;
	for (i = 0 ; i < hash_table_entry_count ; i++) {
		if (!list_exist(list, entry_array[i]))
			limit_hashtable_delete(entry_array[i]);
	}

	wordlist *item = list;
	while (item) {
		if (!limit_hashtable_get(item->key))
			limit_hashtable_create(item->key);
		item = item->next;
	}

	fclose(fp);
	fp = NULL;
	return;
}

static int func_sys_parse_param(char* cfgline)
{
	char *token = NULL;
	if (NULL == (token = strtok(cfgline,w_space))) {
		debug(113,0)("mod_individual_limit_speed: func_sys_parse_param error");
		return -1;
	} 

	if (strcmp(token,"mod_individual_limit_speed")) {
		debug(113,0)("mod_individual_limit_speed: func_sys_parse_param error,mod_name\n");
		return -1;
	}

	mod_config* cfg = mod_config_pool_alloc();
	memset(cfg, 0, sizeof(mod_config));
	cfg->limit_speed_individual = individual_speed_control_pool_alloc();
	memset(cfg->limit_speed_individual, 0, sizeof(individual_speed_control));

	if (NULL==(token = strtok(NULL,w_space))) {
	  	debug(113,0)("mod_individual_limit_speed: func_sys_parse_param error in init_bytes\n");
		free_mod_config(cfg);
		return -1;
	} else {
		cfg->limit_speed_individual->init_bytes = atoi(token);
	}

	if (NULL==(token = strtok(NULL,w_space))) {
		debug(113,0)("mod_individual_limit_speed: func_sys_parse_param error in restore_bps\n");
		free_mod_config(cfg);
		return -1;
	} else {
		int i;
		i = cfg->limit_speed_individual->restore_bps = atoi(token);	
		if (i<=0) {
			debug(113,0)("mod_individual_limit_speed: func_sys_parse_param error, restore_bps value is not available\n");
			free_mod_config(cfg);
			return -1;
		}
	}

	if(NULL==(token = strtok(NULL,w_space))) {
		debug(113,0)("mod_individual_limit_speed: func_sys_parse_param error in max_bytes\n");
		free_mod_config(cfg);
		return -1;
	} else {
		int i = cfg->limit_speed_individual->max_bytes = atoi(token);
		if (i < cfg->limit_speed_individual->restore_bps) {
			debug(113,1)("mod_individual_limit_speed: the max_bytes value is not fit,max_bytes<restore_bps, now we set max_bytes = restore_bps\n");
			cfg->limit_speed_individual->max_bytes = cfg->limit_speed_individual->restore_bps;
		}
	}

	/* case 4210 */
	if (NULL != (token = strtok(NULL,w_space))) {
		if (strcmp(token, "allow") && strcmp(token, "deny"))
			parse_list(token, cfg->host_list);
	}

	cc_register_mod_param(mod,cfg,free_mod_config);	
	return 0;
}

static void disable_mod(int fd)
{
	debug(113,3)("mod_individual_limit_speed: disable_mod, fd=%d\n", fd);
	fd_table[fd].cc_run_state[mod->slot]=0;
	return;
}

/* clear fd's data for limit_speed */
static void cc_clear_defer_fd(int fd)
{
	debug(113,3)("mod_individual_limit_speed: cc_clear_defer_fd, fd=%d\n", fd);
	memset(&g_stDeferWriteFd[fd], 0, sizeof(struct cc_defer_fd_st));
	/*
	g_stDeferWriteFd[fd].fd = 0;   
	if(g_stDeferWriteFd[fd].data!=NULL) {   
		g_stDeferWriteFd[fd].data = NULL;
	}   
	*/
	return;
}

/* set fd's data for limit_speed */
static void cc_set_defer_fd(int fd, void * data, PF *resume_handler)
{
	debug(113,3)("mod_individual_limit_speed: cc_set_defer_fd, fd=%d\n", fd);
	if (g_stDeferWriteFd[fd].is_client) {
		g_stDeferWriteFd[fd].fd = fd;  
		g_stDeferWriteFd[fd].data = data;
		g_stDeferWriteFd[fd].resume_handler = resume_handler;
	}
	return;
}

static void resumeDeferWriteFd(void* unused)
{
	g_iLimitSpeedTimer++;
	if (g_iLimitSpeedTimer < squid_curtime) {
		eventAdd("resumeDeferWriteFd", resumeDeferWriteFd, NULL, 0.9, 0);
	} else if(g_iLimitSpeedTimer > squid_curtime) {
		eventAdd("resumeDeferWriteFd", resumeDeferWriteFd, NULL, g_iLimitSpeedTimer-squid_curtime, 0);
		return;
	} else {
		eventAdd("resumeDeferWriteFd", resumeDeferWriteFd, NULL, 1.1-(double)current_time.tv_usec/1000000.0, 0);
	}

	int i;
	int fd;
	for (i=0; i<CC_DEFER_FD_MAX; i++) {
		fd = g_stDeferWriteFd[i].fd;
		if (fd == 0 || g_stDeferWriteFd[fd].is_client == 0)
			continue;

		if (g_stDeferWriteFd[fd].is_client 
				&& fd_table[fd].cc_run_state
				&& fd_table[fd].cc_run_state[mod->slot] > 0 )
		{
			debug(113,3)("mod_individual_limit_speed: the fd have limit speed data is: %d\n",fd);
			limit_speed_private_data* private_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd,mod);
			if (private_data==NULL) {
				debug(113,1)("mod_individual_limit_speed--->>>resumeDeferWriteFd: the private_data is NULL \n");
				disable_mod(fd);
				cc_clear_defer_fd(fd);
				continue;
			}

			if (private_data->client_write_defer) {
				private_data->client_write_defer = 0;
				debug(113,3)("mod_individual_limit_speed: fd_table[fd].write_handler=%p\n",fd_table[fd].write_handler);
				commSetSelect(fd, COMM_SELECT_WRITE, g_stDeferWriteFd[fd].resume_handler, g_stDeferWriteFd[fd].data, 0);
			}
		}
	}
}


/* 负责模块相关数据的初始化工作 */
static int func_sys_init()
{
	debug(113,1)("mod_individual_limit_speed: func_sys_init called\n");
	int i=0;
	for (i=0; i<CC_DEFER_FD_MAX; i++) 
		cc_clear_defer_fd(i);

	/* 该函数会在系统初始化时被调用，然后在事件队列中一秒种执行一次 */
	eventAdd("resumeDeferWriteFd", resumeDeferWriteFd, NULL, 1, 0);	
	g_iLimitSpeedTimer = squid_curtime;
	return 0;
}


/* decrease the last_bytes of the limit_speed struct owned by  a fde */
// fd_bytes
static int limit_speed_fd_bytes(int fd, int len, unsigned int type)
{
	if (FD_WRITE == type) {
		if (g_stDeferWriteFd[fd].is_client ==0) {
			return -1;
		}
		limit_speed_private_data* private_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd,mod);
		if (private_data==NULL) {
			disable_mod(fd);
			debug(113,1)("mod_individual_limit_speed: limit_speed_fd_bytes-> the private_data is NULL \n");
			return 0;
		}
		speed_control* sc = private_data->sc;
		if (sc->restore_bps > 0) {
			sc->last_bytes -= len;
		}
	}
	return 0;
}


// clientRedirectDone
static int setSpeedLimit(clientHttpRequest* http)
{
	int fd = http->conn->fd;
	debug(113,3)("mod_individual_limit_speed: setSpeedLimit entered fd = [%d]\n", fd);


	if (fd_table[fd].cc_run_state[mod->slot]>0) {
		mod_config *cfg = (mod_config*)cc_get_mod_param(http->conn->fd,mod);
		assert(cfg);

		if (cfg) {
			// initialize fde private data
			individual_speed_control *individual = cfg->limit_speed_individual;
			limit_speed_private_data* p_data = limit_speed_private_data_pool_alloc();
			if (p_data->sc==NULL) {
				p_data->sc = speed_control_pool_alloc();
			}
			p_data->client_write_defer = 0;
			speed_control *sc = (speed_control*)(p_data->sc);
			sc->init_bytes = individual->init_bytes;
			sc->restore_bps = individual->restore_bps;
			sc->max_bytes = individual->max_bytes;
			sc->last_bytes = individual->init_bytes;
			sc->now = squid_curtime;

			// initialize fd_table[fd]
//modified by fangfang.yang for case 4581
//			assert(g_stDeferWriteFd[fd].is_client == 0);
			g_stDeferWriteFd[fd].is_client = 1; //set the is_client bit, which cleared by fd_close

			// bind fd to host
			assert(g_stDeferWriteFd[fd].host_node == NULL);
			struct hash_entry *entry;
			if (NULL != (entry = limit_hashtable_get(http->request->host))) {
				g_stDeferWriteFd[fd].host_node = entry;
				entry->conn++;
				debug(113,3)("mod_individual_limit_speed: host [%s] connections=%d\n", http->request->host,entry->conn);
				// Recalculate 
				int N = entry->conn;
				if (N > 1) {
					if (sc->init_bytes > 0) sc->init_bytes /= N;
					if (sc->restore_bps > 0) sc->restore_bps /= N;
					if (sc->max_bytes > 0) sc->max_bytes /= N;
					if (sc->last_bytes > 0) sc->last_bytes /= N;
				}
			}
			cc_register_mod_private_data(FDE_PRIVATE_DATA, &fd, p_data,free_limit_speed_control , mod);
		}
	}
	return 0;
}


// conn means the number of fd under host
static int modifySpeedControl(speed_control *sc, int fd)
{
	debug(113,5)("mod_individual_limit_speed: modifySpeedControl called\n");
	assert(sc->restore_bps > 0);
	if (sc->now < squid_curtime) {
		int ret = squid_curtime - sc->now;
		if (ret > 10) {
			ret = 10;
		}
		sc->last_bytes += ret * sc->restore_bps;
		if (sc->last_bytes > sc->max_bytes) {
			sc->last_bytes = sc->max_bytes;
		}
	}

	sc->now = squid_curtime;
	return 0;
}

/* get the bytes need to send to client, and saved as nleft */
// commHandleWrite
static int getMayWriteBytes(int fd, void* data,int *nleft, PF *resume_handler)
{
	debug(113,3)("mod_individual_limit_speed: getMayWriteBytes fd=%d nleft=%d, handler=%p\n", fd,*nleft,resume_handler);
	if (g_stDeferWriteFd[fd].is_client == 0) {
		debug(113,4)("mod_individual_limit_speed: do not limit speed to %d\n",fd);
		return *nleft;
	}

	limit_speed_private_data* private_data = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd,mod);
	if (NULL == private_data) {
		cc_clear_defer_fd(fd);
		disable_mod(fd);
		debug(113,1)("mod_individual_limit_speed: the private_data is NULL\n");
		return *nleft;
	} 

	assert(!private_data->client_write_defer);
	int len = 0;
	speed_control *sc = (speed_control*)private_data->sc;
	if (sc) {
		if (sc->restore_bps > 0) {
			if (sc->last_bytes <= 0 ) {
				debug(113,5)("mod_individual_limit_speed: limit speed to %d\n", fd);
				if (sc->now != squid_curtime) 
					modifySpeedControl(sc, fd);
				cc_set_defer_fd(fd,data,resume_handler);
				private_data->client_write_defer = 1;
				*nleft = 0;
				return 0;
			}
			len = sc->last_bytes;
		} else {
			debug(113,4)("mod_individual_limit_speed: do not limit speed to %d\n",fd);
		}
	} else {
		cc_clear_defer_fd(fd);
		disable_mod(fd);
		debug(113,1)("mod_individual_limit_speed: the speed_control_data is NULL,and the nleft is: %d\n", *nleft);
		return *nleft;
	}

	if (len < *nleft) 
		*nleft = len;

	return len;
}


/* fd_close */
/*
static int func_fd_close(int fd)
{
	cc_clear_defer_fd(fd);
	return 0;
}
*/

// httpRequestFree
static int http_req_free(clientHttpRequest *http)
{
	debug(113,5)("mod_individual_limit_speed: http_req_free called\n");
	int fd = http->conn->fd;
	// unbind
	struct hash_entry *entry = g_stDeferWriteFd[fd].host_node;
	if (entry) {
		entry->conn--;
		debug(113,5)("mod_individual_limit_speed: unbind [%d] for %s\n", fd, entry->host);
	}
	cc_clear_defer_fd(fd);
	return 0;
}

static int func_sys_cleanup(cc_module* mod)
{
	eventDelete(resumeDeferWriteFd, NULL);
	g_iLimitSpeedTimer = 0;
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);	
	if(NULL != individual_speed_control_pool)
		memPoolDestroy(individual_speed_control_pool);	
	if(NULL != limit_speed_private_data_pool)
		memPoolDestroy(limit_speed_private_data_pool);
	if(NULL != speed_control_pool)
		memPoolDestroy(speed_control_pool);	

	if (limit_by_host)
		limit_hashtable_dest();
	return 0;
}


int mod_register(cc_module *module)
{
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			func_sys_init);

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			setSpeedLimit);

	cc_register_hook_handler(HPIDX_hook_func_fd_bytes,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fd_bytes),
			limit_speed_fd_bytes);

	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			func_sys_cleanup);

	cc_register_hook_handler(HPIDX_hook_func_private_individual_limit_speed,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_individual_limit_speed),
			getMayWriteBytes);

	/*
	cc_register_hook_handler(HPIDX_hook_func_fd_close,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fd_close),
			func_fd_close);
			*/

	cc_register_hook_handler(HPIDX_hook_func_http_req_free,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_free),
			http_req_free);

	if (reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}



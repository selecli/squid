#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

#define MAX_CONFIG_LINE_NUM  20 
static cc_module* mod = NULL;
typedef struct _mod_config
{
	int line_num;
	int num;
	int sec;
	int deny_span;
	int lru_length; 
} mod_config;

typedef struct _FreqLruPolicy FreqLruPolicy;
struct _FreqLruPolicy 
{
	dlink_list list;
	int count;
};

typedef struct _FreqLruNode FreqLruNode;
struct _FreqLruNode 
{
	/* Note: the dlink_node MUST be the first member of the FreqLruNode
	 * structure. This member is later pointer typecasted to FreqLruNode *.
	 */
	dlink_node node;
};

typedef struct _ClientipAppendedInfo ClientipAppendedInfo;
struct _ClientipAppendedInfo
{
	int should_deny;
	int should_delete;
	int req_num;
	int new_add;
	time_t start_timestamp;
	time_t warning_timestamp;
};

typedef struct _ClientIpInfo ClientIpInfo;
struct _ClientIpInfo
{
	hash_link hash;             /* must be first */
	struct in_addr addr;
	void ** append_data;
	LruPolicyNode lru_repl;
	int new_add;
};

static MemPool * client_ip_info_pool = NULL;
static MemPool * freq_lru_node_pool = NULL;
static MemPool * mod_config_pool = NULL;
static MemPool * clientipinfo_appendedarray_pool = NULL;
static MemPool * clientipinfo_appendedmeta_pool = NULL;
static hash_table *client_ip_table = NULL;
FreqLruPolicy  lru_policy; 

static ClientIpInfo * lruClientipAdd(struct in_addr addr);
static void ClientipInfoAppendeddataInit(ClientIpInfo * c, int n);
static void ClientipInfoUpdate(clientHttpRequest *http, ClientIpInfo *c);
static void freq_lru_remove(FreqLruPolicy * policy, ClientIpInfo * c, LruPolicyNode * node);
/* callback: free the configdata */
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

/*free the lru list when shutting down squid*/
static void free_lru(void)
{
	dlink_node *m;

	m = lru_policy.list.head;
	while(m)
	{
		dlink_node *tmp = m->next;	
		memPoolFree(freq_lru_node_pool, (FreqLruNode *)m);
		m = tmp;
	}
}

static void client_ip_info_free(void *data)
{
	int j;  
	ClientIpInfo *c = data;
	for (j=0; j<mod->mod_params.count; j++)
	{       
		memPoolFree(clientipinfo_appendedmeta_pool, (ClientipAppendedInfo *)c->append_data[j]);  
	}       
	memPoolFree(clientipinfo_appendedarray_pool, c->append_data);	
	safe_free(c->hash.key);
	memPoolFree(client_ip_info_pool, c);
}

/*func to destroy hash table when shuting down*/
static void hash_table_destroy(void)
{
	if (client_ip_table)
	{
			hashFreeItems(client_ip_table, client_ip_info_free);
			hashFreeMemory(client_ip_table);
			client_ip_table = NULL;
	}
}

/*hook free_func for hash_table and appeded data when reconfiguring  squid*/
static void free_when_reconfigure()
{
	FreqLruPolicy * policy = &lru_policy;
	dlink_node * m = policy->list.head;
	ClientIpInfo * tmp = NULL;
	while(m)
	{
		tmp = m->data;
		m = m->next;
		freq_lru_remove(policy, tmp, &tmp->lru_repl);	
		hash_remove_link(client_ip_table, &tmp->hash);
		int j;
		for (j=0; j<MAX_CONFIG_LINE_NUM; j++)
		{
			if (NULL != (ClientipAppendedInfo *)tmp->append_data[j])
			{
				memPoolFree(clientipinfo_appendedmeta_pool, (ClientipAppendedInfo *)tmp->append_data[j]);	
				tmp->append_data[j] = NULL;
			}
		}
		memPoolFree(clientipinfo_appendedarray_pool, tmp->append_data);
		tmp->append_data = NULL;
		safe_free(tmp->hash.key);
		memPoolFree(client_ip_info_pool, tmp);
		tmp = NULL;
	}
}

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if(NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_access_freq_limit_for_singleIP config_struct", sizeof(mod_config));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static void * freq_lru_node_pool_alloc(void)
{
	void * obj = NULL;
	if(NULL == freq_lru_node_pool)
	{
		freq_lru_node_pool = memPoolCreate("mod_access_freq_limit_for_singleIP other_data FreqLruNode", sizeof(FreqLruNode));
	}
	return obj = memPoolAlloc(freq_lru_node_pool);
}

static void * clientipinfo_appendedarray_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == clientipinfo_appendedarray_pool)
	{
		clientipinfo_appendedarray_pool = memPoolCreate("mod_access_freq_limit_for_singleIP other_data clientipinfo_appendedarray", MAX_CONFIG_LINE_NUM*sizeof(void *));
	}
	return obj = memPoolAlloc(clientipinfo_appendedarray_pool);
}

static void * clientipinfo_appendedmeta_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == clientipinfo_appendedmeta_pool)
	{
		clientipinfo_appendedmeta_pool = memPoolCreate("mod_access_freq_limit_for_singleIP other_data ClientipAppendedInfo", sizeof(ClientipAppendedInfo));
	}
	return obj = memPoolAlloc(clientipinfo_appendedmeta_pool);
}

static void * client_ip_info_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == client_ip_info_pool)
	{
		client_ip_info_pool = memPoolCreate("mod_access_freq_limit_for_singleIP other_data ClientIpInfo", sizeof(ClientIpInfo));
	}
	return obj = memPoolAlloc(client_ip_info_pool);
}

/* hook functions for parsing config line*/
static int func_sys_parse_param(char *cfg_line)
{
	debug(151, 1)("mod_access_freq_limit_for_singleIP:  parse_param [%s]\n", cfg_line);
	char *token = NULL;
	mod_config *cfg = NULL;
	char *p = strstr(cfg_line, "allow");
	if (!p)
		goto out;
	*p = 0;

	if ((token = strtok(cfg_line, w_space)) == NULL)
		goto out;
	else if (strcmp(token,"mod_access_freq_limit_for_singleIP"))
		goto out;
	
	cfg = mod_config_pool_alloc();
	
	while ((token = strtok(NULL, w_space)))
	{
		if (0 == cfg->line_num)
			cfg->line_num = atoi(token);
		else if (0 == cfg->num)
			cfg->num = atoi(token);
		else if (0 == cfg->sec)		
			cfg->sec = atoi(token);
		else if (0 == cfg->deny_span)
			cfg->deny_span = atoi(token);
		else
			cfg->lru_length = atoi(token);
	}
        if (cfg->num<=0 || cfg->sec<=0 || cfg->deny_span<=0 || cfg->lru_length<=0)
		goto out;

	debug (151,3)("mod_access_freq_limit_for_singleIP parsing result: parmas num[%d]  sec[%d]  span[%d]  len[%d]\n", cfg->num, cfg->sec, cfg->deny_span, cfg->lru_length);
	cc_register_mod_param(mod, cfg, free_mod_config);
	return 0;

out:
	if (cfg)
	{
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	debug(151, 1)("mod_access_freq_limit_for_singleIP: parse err, cfgline=%s\n", cfg_line);
	return -1;
}

/*hook func for checking the module config params again*/
static void func_after_parse_param()
{
	int cfg_line_num = mod->mod_params.count;	
	int i;
	int hashtable_sz = 0;

	for(i=0; i<cfg_line_num; i++)
	{
		if(((mod_config*)((cc_mod_param *)mod->mod_params.items[i])->param)->line_num != i+1
		  || ((mod_config*)((cc_mod_param *)mod->mod_params.items[i])->param)->lru_length != ((mod_config*)((cc_mod_param *)mod->mod_params.items[0])->param)->lru_length)
		{
			mod->flags.enable = 0;
			break;
		}
	}
	hashtable_sz = hashPrime(((mod_config*)((cc_mod_param *)mod->mod_params.items[0])->param)->lru_length);
	if (0 != mod->flags.enable && NULL == client_ip_table)
	{
		client_ip_table = hash_create((HASHCMP *) strcmp, hashtable_sz, hash_string);
	}
	if (NULL != lru_policy.list.head)
		free_when_reconfigure();
	debug(151, 1)("mod_access_freq_limit_for_singleIP is %s and hash table size is %d\n", mod->flags.enable?"enabled":"disabled", hashtable_sz);
}

static void changeStoreUrl(clientHttpRequest *http)
{
	char url_t[BUFSIZ] = {0};

	if (http->request && http->request->store_url)
		snprintf(url_t, BUFSIZ, "%s%d", http->request->store_url, rand());
	else
		snprintf(url_t, BUFSIZ, "%s%d", http->uri, rand());
	if (0 != strlen(url_t))
	{   
		safe_free(http->request->store_url);
		http->request->store_url = xstrdup(url_t);
	}
	return;
}

static int func_req_deny_for_high_accessfreq(clientHttpRequest *http)
{
	const char *key = NULL;
	ClientIpInfo * c = NULL;
	ClientipAppendedInfo * info = NULL;
	mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);

	key = xinet_ntoa(http->conn->peer.sin_addr);
	c = (ClientIpInfo *) hash_lookup(client_ip_table, key);
	if (c == NULL)
	{
		c = lruClientipAdd(http->conn->peer.sin_addr);
		if (NULL != c)
		{
			debug(151, 3)("info for a new clientIP is added when processing request\n");
			ClientipInfoAppendeddataInit(c, mod->mod_params.count);
		}
		else
			debug_trap("mod_access_freq_limit_for_singleIP: Failed to add entry");
	}
	info = (ClientipAppendedInfo *) c->append_data[cfg->line_num-1];
	debug (151,3)("lru status before trying to deny requests:clientIP[%s] matched_configline_num[%d]  should_deny[%d]  should_delete[%d]  req_num[%d] start_timestamp[%ld] warning_timestamp[%ld]\n",
			key, cfg->line_num, info->should_deny, info->should_delete, info->req_num, info->start_timestamp, info->warning_timestamp);
	if (info->should_deny)
	{
		// add by cwg ,case 5611
		changeStoreUrl(http);
		ErrorState *err = errorCon(ERR_ACCESS_DENIED, HTTP_FORBIDDEN, http->request);
		http->log_type = LOG_TCP_MISS;
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		errorAppendEntry(http->entry, err);
		ClientipInfoUpdate(http, c);
		return 2;     
	}
	ClientipInfoUpdate(http, c);
	return 0;
}

static void freq_lru_add(FreqLruPolicy * policy, ClientIpInfo * c, LruPolicyNode * node)
{
	FreqLruPolicy *lru = policy;
	FreqLruNode *lru_node;
        node->data = lru_node = freq_lru_node_pool_alloc();
	dlinkAddTail(c, &lru_node->node, &lru->list);
	lru->count += 1;
	debug(151, 5) ("add a new clientipinfo node to the tail of lru list\n");
}

static void freq_lru_remove(FreqLruPolicy * policy, ClientIpInfo * c, LruPolicyNode * node)
{
	FreqLruPolicy *lru = policy;
	FreqLruNode *lru_node = node->data;
	if (!lru_node)
		return;
	/*
	 * It seems to be possible for an entry to exist in the hash
	 * but not be in the LRU list, so check for that case rather
	 * than suffer a NULL pointer access.
	 */
	if (NULL == lru_node->node.data)
		return;
	assert(lru_node->node.data == c);
	node->data = NULL;
	dlinkDelete(&lru_node->node, &lru->list);
	memPoolFree(freq_lru_node_pool, lru_node);
	lru->count -= 1;
}

static void freq_lru_referenced(FreqLruPolicy * policy, const ClientIpInfo * c, LruPolicyNode * node) 
{
	FreqLruPolicy *lru = policy;
	FreqLruNode *lru_node = node->data;
	if (!lru_node)
		return; 
	dlinkDelete(&lru_node->node, &lru->list);
	dlinkAddTail((void *) c, &lru_node->node, &lru->list);
}

static ClientIpInfo * lruClientipAdd(struct in_addr addr)
{
	ClientIpInfo *c;
	c = client_ip_info_pool_alloc();
	c->hash.key = xstrdup(xinet_ntoa(addr));
	c->addr = addr; 
	hash_join(client_ip_table, &c->hash);
	statCounter.client_http.clients++;
	return c;
}

/*alloc space for apended data of clientip info*/
static void ClientipInfoAppendeddataInit(ClientIpInfo * c, int n)
{
	int i;
	c->new_add = 1;
	c->append_data = clientipinfo_appendedarray_pool_alloc();
	assert(c->append_data);
	for (i=0; i<n; i++)
	{
		c->append_data[i] = (ClientipAppendedInfo *) clientipinfo_appendedmeta_pool_alloc();
	}
	debug(151, 5) ("append lru status data to clientipinfo for future use\n");	
}

/*hook func to shink clientipdb during freeing http request*/
static void func_ClientipdbScheduledGC(clientHttpRequest *http)
{
	int cleanup_removed = 0;
	int scan_count = 0;
	dlink_node * m = lru_policy.list.head;
	ClientIpInfo * tmp = NULL;
	
	mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	if(lru_policy.count < cfg->lru_length)
		return;

	debug(151, 3) ("going to collect client_ip info garbage!\n");
	int policy_count = lru_policy.count;
	if (lru_policy.count - cfg->lru_length <= (int)(0.1*cfg->lru_length))
	{
		while(m)
		{
			tmp = m->data;
			m = m->next;
			scan_count++;
			if(scan_count > (int)(0.1*cfg->lru_length) + policy_count - cfg->lru_length)
				break;
			int i;
			int flag = 1;
			for(i=0; i<mod->mod_params.count; i++)
			{
				if (((ClientipAppendedInfo *)tmp->append_data[i])->should_delete == 0 
						&& ((ClientipAppendedInfo *)tmp->append_data[i])->start_timestamp != 0)
				{
					flag = 0;
					break;	
				}	
			}
			if(1 == flag)
			{
				freq_lru_remove(&lru_policy, tmp, &tmp->lru_repl);	
				hash_remove_link(client_ip_table, &tmp->hash);
				int j;
				for (j=0; j<mod->mod_params.count; j++)
				{
					memPoolFree(clientipinfo_appendedmeta_pool, (ClientipAppendedInfo *)tmp->append_data[j]);	
				}
				memPoolFree(clientipinfo_appendedarray_pool, tmp->append_data);
				safe_free(tmp->hash.key);
				memPoolFree(client_ip_info_pool, tmp); 
				debug(151, 3) ("an clientip info structor is deleted when going through the lru list\n");
				statCounter.client_http.clients--;
				cleanup_removed++;
			}
		}
		debug(151, 3) ("go through a tenth of lru list length and remove %d lru node\n", cleanup_removed);
	}

	if(cleanup_removed < (int)(0.05*cfg->lru_length) + policy_count - cfg->lru_length)
	{
		m = lru_policy.list.head;
		int tmp_count = (int)(0.05*cfg->lru_length) + policy_count - cfg->lru_length - cleanup_removed;
		while(m)
		{
			tmp = m->data;
			m = m->next;
			freq_lru_remove(&lru_policy, tmp, &tmp->lru_repl);
			hash_remove_link(client_ip_table, &tmp->hash);
			int i;
			for (i=0; i< mod->mod_params.count; i++)
			{
				memPoolFree(clientipinfo_appendedmeta_pool, (ClientipAppendedInfo *) tmp->append_data[i]);	
			}
			memPoolFree(clientipinfo_appendedarray_pool, tmp->append_data);
			safe_free(tmp->hash.key);
			memPoolFree(client_ip_info_pool, tmp); 
			debug(151, 3) ("an clientip info structor is deleted from the head of lru\n");
			statCounter.client_http.clients--;
			tmp_count--;
			if(0 == tmp_count)
				break;
		}
		debug(151, 3) ("can not remove enough lru nodes by going through the lru list, so remove another %d older lru nodes\n", (int)(0.05*cfg->lru_length) + policy_count - cfg->lru_length - cleanup_removed);
	}
}

static void ClientipInfoUpdate(clientHttpRequest *http, ClientIpInfo *c)
{
	mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);

	if(c->new_add == 1)
	{
		freq_lru_add(&lru_policy, c, &c->lru_repl);	
		c->new_add = 0;
	}

	ClientipAppendedInfo * appended_info = (ClientipAppendedInfo *)c->append_data[cfg->line_num-1];
	if (appended_info->new_add == 0)
	{
		appended_info->new_add = 1;
		appended_info->start_timestamp = squid_curtime;
	}

	int age = squid_curtime - appended_info->start_timestamp;
	appended_info->req_num++;
	if(0 != appended_info->warning_timestamp)
	{
		if(squid_curtime - appended_info->warning_timestamp >= cfg->deny_span)
		{
			appended_info->start_timestamp = squid_curtime;
			appended_info->warning_timestamp = 0;
			appended_info->req_num = 0;
			appended_info->should_deny = 0;
		}
		freq_lru_referenced(&lru_policy, c, &c->lru_repl);
	}
	else if(appended_info->req_num<cfg->num && age <= cfg->sec)
	{
		freq_lru_referenced(&lru_policy, c, &c->lru_repl);
	}
	else if(appended_info->req_num<cfg->num && age > cfg->sec)
	{
		appended_info->should_delete = 1;
		appended_info->start_timestamp = squid_curtime;
		appended_info->warning_timestamp = 0;
		appended_info->req_num = 0;
		appended_info->should_deny = 0;

	}
	else if(age<=cfg->sec && appended_info->req_num==cfg->num)
	{
		appended_info->warning_timestamp = squid_curtime;
		appended_info->should_deny = 1;
		freq_lru_referenced(&lru_policy, c, &c->lru_repl);
		appended_info->should_delete = 0;
	}	
	else 
	{
		if((age-cfg->sec)/cfg->sec*100 < 5)
		{
			appended_info->warning_timestamp = squid_curtime;
			appended_info->should_deny = 1;
			freq_lru_referenced(&lru_policy, c, &c->lru_repl);
			appended_info->should_delete = 0;
		}		
		else
		{
			appended_info->should_delete = 1;
			appended_info->start_timestamp = squid_curtime;
			appended_info->warning_timestamp = 0;
			appended_info->req_num = 0;
			appended_info->should_deny = 0;
		}
	}

	debug (151,3)("lru status after updating :clientIP[%s] matched_configline[%d]  should_deny[%d]  should_delete[%d]  req_num[%d] start_timestamp[%ld] warning_timestamp[%ld]\n",
		xinet_ntoa(http->conn->peer.sin_addr), cfg->line_num, appended_info->should_deny, appended_info->should_delete, appended_info->req_num, appended_info->start_timestamp, appended_info->warning_timestamp);
}

static int hook_cleanup(cc_module * module)
{
	debug(151, 1)("mod_access_freq_limit_for_singleIP:  hook cleanup\n");
	hash_table_destroy();
	free_lru();
	if (NULL != freq_lru_node_pool)
		memPoolDestroy(freq_lru_node_pool);
	if (NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	if (NULL != clientipinfo_appendedarray_pool)
		memPoolDestroy(clientipinfo_appendedarray_pool);
	if (NULL != clientipinfo_appendedmeta_pool)
		memPoolDestroy(clientipinfo_appendedmeta_pool);
	return 0;
}

int mod_register(cc_module *module)
{
	debug(151, 1)("mod_access_freq_limit_for_singleIP init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
			func_sys_parse_param);

	//module->hook_func_http_req_process = hook_func_http_req_parsed;
	cc_register_hook_handler(HPIDX_hook_func_http_req_parsed,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_parsed),
			func_req_deny_for_high_accessfreq);
	
	//module->hook_func_sys_after_parse_param = func_after_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_after_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_after_parse_param),
			func_after_parse_param);

	//module->hook_func_http_req_free = func_ClientipdbScheduledGC;
	cc_register_hook_handler(HPIDX_hook_func_http_req_free,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_free),
			func_ClientipdbScheduledGC);

	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);
//	mod = module;
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;

	return 0;
}
#endif

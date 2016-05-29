#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
//这是本模块在主程序的索引信息的结构指针
static cc_module* mod = NULL;

//这是和acl对应的mod para结构的挂载值：即框架可以为匹配对应acl的fd挂载一个对应的para
struct mod_conf_param {
	int negative_ttl;	//seconds
};
static MemPool *  mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_negative_ttl config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

//callback: free the data
static int free_callback(void* param)
{
	struct mod_conf_param * data = param;
	if(NULL != data)
	{
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}

	return 0;
}

// confige line example:
// mod_negative_ttl time allow/deny acl
// 解析从头开始，拿到time即止
// 包含错误处理
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data = NULL;
	char* token = NULL;

	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_negative_ttl"))
			goto out;	
	}
	else
	{
		debug(94, 3)("(mod_negative_ttl) ->  parse line error\n");
		goto out;	
	}

	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(94, 3)("(mod_negative_ttl) ->  parse line data does not existed\n");
		goto out;
	}
	
	data = mod_config_pool_alloc();
	data->negative_ttl = atoi(token);
	debug(94, 2) ("(mod_negative_ttl) ->  netative_ttl=%d\n", data->negative_ttl);
	if(0 > data->negative_ttl)
	{
		debug(94, 2)("(mod_negative_ttl) ->  data set error\n");
		goto out;
	}

	cc_register_mod_param(mod, data, free_callback);
	return 0;
	
out:
	if (data)
	{
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return -1;		
}


//实现negative页面缓存的函数, 与原流程 http.c : storeNegativeCache() 相似
//在expires == -1时，  expires = squid_curtime + cc.negative_ttl改为从模块中拿到的数据的 negative_ttl
static void private_storeNegativeCache(StoreEntry *e, int negative_ttl)
{
	StoreEntry *oe = e->mem_obj->old_entry;
	time_t expires = e->expires;
	http_status status = e->mem_obj->reply->sline.status;
	refresh_cc cc = refreshCC(e, e->mem_obj->request);
	expires = squid_curtime + negative_ttl;
	if (status && oe && !EBIT_TEST(oe->flags, KEY_PRIVATE) && !EBIT_TEST(oe->flags, ENTRY_REVALIDATE) &&
			500 <= status && status <= 504)
	{
		HttpHdrCc *oldcc = oe->mem_obj->reply->cache_control;
		if (oldcc && EBIT_TEST(oldcc->mask, CC_STALE_IF_ERROR) && oldcc->stale_if_error >= 0)
			cc.max_stale = oldcc->stale_if_error;
		if (cc.max_stale >= 0)
		{
			time_t max_expires;
			storeTimestampsSet(oe);
			max_expires = oe->expires + cc.max_stale;
			/* Bail out if beyond the stale-if-error staleness limit */
			if (max_expires <= squid_curtime)
				goto cache_error_response;
			/* Limit expiry time to stale-if-error/max_stale */
			if (expires > max_expires)
				expires = max_expires;
		}
		/* Block the new error from getting cached */
		EBIT_CLR(e->flags, ENTRY_CACHABLE);
		/* And negatively cache the old one */
		if (oe->expires < expires)
			oe->expires = expires;
		EBIT_SET(oe->flags, REFRESH_FAILURE);
		return;
	}
cache_error_response:
	e->expires = expires;
	EBIT_SET(e->flags, ENTRY_NEGCACHED);
}

//a copy of http.c : httpCacheNegatively()
//不同之处在于抛弃公共变量中的negative_ttl 改用私有配置中的量。
static int func_private_negative_ttl(HttpStateData *httpState)
{
	StoreEntry *entry = httpState->entry;
	int fd = httpState->fd;		

	//FIXME:need a more user-friendly API
	//从fd中取slot位置 拿出acl对应的位置的para
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);

	debug(94, 3)("(mod_negative_ttl) ->  match fd=%d, negative_ttl=%d\n", fd, param->negative_ttl);
	
	private_storeNegativeCache(entry, param->negative_ttl);
	if (entry->expires <= squid_curtime)
		storeRelease(entry);
	if (EBIT_TEST(entry->flags, ENTRY_CACHABLE))
		storeSetPublicKey(entry);
	return 0;
}

static int hook_cleanup(cc_module *module)
{
	debug(94, 1)("(mod_negative_ttl) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}
/* module init */
int mod_register(cc_module *module)
{
	debug(94, 1)("(mod_negative_ttl) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_private_negative_ttl = func_private_negative_ttl;
	cc_register_hook_handler(HPIDX_hook_func_private_negative_ttl,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_negative_ttl),
			func_private_negative_ttl);
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);

	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif

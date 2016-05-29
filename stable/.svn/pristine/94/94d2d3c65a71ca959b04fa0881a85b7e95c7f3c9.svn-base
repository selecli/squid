#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
//这是本模块在主程序的索引信息的结构指针
static cc_module* mod = NULL;
typedef enum{
    CACHE,
    NO_CACHE,
    NO_CACHE_NO
}cache_type;

//这是和acl对应的mod para结构的挂载值：即框架可以为匹配对应acl的fd挂载一个对应的para
struct mod_conf_param {
	HttpHeaderEntry e;
	cache_type cacheable;
    int value;
};

static MemPool * mod_config_pool = NULL; 

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_cacheable_if_reply_header config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data)
	{
		if(strLen(data->e.name))
			stringClean(&data->e.name);
		if(strLen(data->e.value))
			stringClean(&data->e.value);
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}
	return 0;
	
}


// confige line example:
// mod_cacheable_if_reply_header cache|no-cache|no-cache-no name A [value B] allow acl[on]
static int func_sys_parse_param(char *cfg_line)
{
    struct mod_conf_param *data = NULL;
    char* token = NULL;
    int i, len;

    if (NULL == (token = strtok(cfg_line, w_space)))
        goto err;	

    data = mod_config_pool_alloc();
    memset(data,0, sizeof(struct mod_conf_param));
    HttpHeaderEntry* e = &data->e;

    if (NULL == (token = strtok(NULL, w_space)))
        goto err;
    if(!strcmp(token,"no_cache")||!strcmp(token,"no-cache"))
        data->cacheable = NO_CACHE;
    else if (!strcmp(token, "cache"))
        data->cacheable = CACHE;
    else if (!strcmp(token, "no-cache-no")||!strcmp(token,"no_cache_no"))
        data->cacheable = NO_CACHE_NO;
    else
        goto err;

    if (NULL == (token = strtok(NULL, w_space)))
        goto err;
    if(!strcmp(token, "name"))
    {
        if(NULL == (token = strtok(NULL, w_space)))
            goto err;
        stringInit(&e->name, token);
    }
    else
        goto err;

    if (NULL == (token = strtok(NULL, w_space)))
        goto err;
    if (!strcmp(token, "value"))
    {
        data->value = 1;
        if(NULL == (token = strtok(NULL, w_space)))
            goto err;
        stringInit(&e->value, token);
    }
    else if(!strcmp(token, "allow") || !strcmp(token, "deny"));
    else
        goto err;

    len = strlen(strBuf(e->name));
    i = httpHeaderIdByNameDef(strBuf(e->name), len); 	
    if(-1 == i)
        e->id = HDR_OTHER;
    else
        e->id = i;

    debug(151, 3)("(mod_cacheable_if_reply_header) -> e->id(%d) == HDR_OTHER? %d, name=%s,vale=%s\n", e->id, (e->id == HDR_OTHER)?1:0,strBuf(data->e.name),strBuf(data->e.value));

    cc_register_mod_param(mod, data, free_callback);
    return 0;		

err:
    debug(151, 1)("(mod_cacheable_if_reply_header) ->  parse line error1\n");
    free_callback(data);
    return -1;
}

static int makePublicPrivate(int fd, HttpStateData *httpState)
{
	HttpReply* reply = httpState->entry->mem_obj->reply;

	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *myheader;
	HttpHeaderEntry* e = &param->e;
	Array result; 
	arrayInit(&result);
	String tmp = StringNull;;
	int i = 0;
	int ret = 0;
	int reply_include_header = 0;
	int header_value_exsit = 0;

	if (e->id == HDR_OTHER) {
		while ((myheader = httpHeaderGetEntry(&reply->header, &pos))) {
			if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name,strBuf(e->name)) == 0){
				arrayAppend(&result, &(myheader->value));
				reply_include_header = 1;
				if (strBuf(myheader->value))
					header_value_exsit = 1;
            }
        }
    }
	else {
        tmp = httpHeaderGetStrOrList(&reply->header, e->id);
        if (strBuf(tmp)) {
            header_value_exsit = 1;
            reply_include_header = 1;
            arrayAppend(&result,&(tmp));
        }
        else {
            reply_include_header = 0; 
        }
    }

    for(i = 0; i < result.count; i++)
        debug(151,3)("(mod_cacheable_if_reply_header)-> result = %s\n",strBuf(*((String*)(result.items[i]))));
    debug(151,3)("(mod_cacheable_if_reply_header)-> header_value_exsit=%d,reply_include_header=%d\n",header_value_exsit,reply_include_header);

    switch (param->cacheable)
    {
        case CACHE:
            if (reply_include_header && param->value)
                    for (i = 0; i < result.count; i++) {
                        if (!strCaseCmp(*((String*)(result.items[i])),strBuf(e->value)))
                            ret = 4;
                    }
            else if (reply_include_header && !param->value)
                ret = 4;
            break;

        case NO_CACHE:
            if (reply_include_header && param->value)
                    for (i = 0; i < result.count; i++) {
                        if (!strCaseCmp(*((String*)(result.items[i])),strBuf(e->value)))
                            ret = 5;
                    }
            else if (reply_include_header && !param->value)
                ret = 5;
            break;

        case NO_CACHE_NO:
            if (!reply_include_header)
                ret=5;
            break;

        default: 
            ret=0;
            break;
    }

	debug(151, 3)("(mod_cacheable_if_reply_header) ->  ret=%s, e->name=%s, e->value=%s e->id=%d\n", (ret==5)?"no_cache":(ret==4)?"cache":"default", strBuf(e->name), strBuf(e->value), e->id);

	arrayClean(&result);
	stringClean(&tmp);
	return ret;
}

static int hook_cleanup(cc_module *module)
{
	debug(151, 1)("(mod_cacheable_if_reply_header) ->     hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}


/* module init */
int mod_register(cc_module *module)
{
	debug(151, 1)("(mod_cacheable_if_reply_header) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	//mod->hook_func_http_repl_cachable_check = makePublicPrivate; 
	cc_register_hook_handler(HPIDX_hook_func_http_repl_cachable_check,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_cachable_check),
			makePublicPrivate);

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

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;

	return 0;
}
#endif

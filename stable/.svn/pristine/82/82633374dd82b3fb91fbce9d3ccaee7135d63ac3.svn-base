#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;
struct mod_conf_param {
	String name;
	String value;
};

static MemPool * mod_config_pool = NULL;
static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{   
		mod_config_pool = memPoolCreate("mod_Powered-By-ChinaCache_header_details config_struct", sizeof(struct mod_conf_param));
	}   
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data)
	{   
		if(strLen(data->name))
			stringClean(&data->name);
		if(strLen(data->value))
			stringClean(&data->value);
		memPoolFree(mod_config_pool, data);
		data = NULL;
	}   
	return 0;
}

////////////////////// hook functions ////////////////

/**
 * return 0 if parse ok, -1 if error
 */
// mod_Powered-By-ChinaCache_header_details Power-By-CC 1 allow acl_name
static int func_sys_parse_param(char *cfg_line)
{
	char* token = NULL;

	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if (strcmp(token, "mod_Powered-By-ChinaCache_header_details"))
			return -1;
	}
	else
	{
		debug(103, 3)("(mod_Powered-By-ChinaCache_header_details) ->  parse line error\n");
		return -1;
	}

	struct mod_conf_param *data = mod_config_pool_alloc();
	memset(data, 0, sizeof(struct mod_conf_param));

	if (NULL == (token = strtok(NULL, w_space)))
		goto err;
	if (!strncmp("name=", token, strlen("name=")))
		stringInit(&data->name, token+strlen("name="));
	else if (!strncmp("value=", token, strlen("value=")))
		if (!strncmp("log_type", token+strlen("value="), strlen("log_type")))
			stringInit(&data->value, "log_type");

	if (NULL != (token = strtok(NULL, w_space)))
	{
		if (!strncmp("name=", token, strlen("name=")))
			stringInit(&data->name, token+strlen("name="));
		else if (!strncmp("value=", token, strlen("value=")))
			if (!strncmp("log_type", token+strlen("value="), strlen("log_type")))
				stringInit(&data->value, "log_type");
	}

	debug(103,2)("(mod_Powered-By-ChinaCache_header_details) -> name=%s, value=%s\n", strBuf(data->name),strBuf(data->value));

	if (strLen(data->name) == 0 && strLen(data->value) == 0)
		goto err;
	cc_register_mod_param(mod, data, free_callback);
	return 0;

err:
	debug(103, 1)("(mod_Powered-By-ChinaCache_header_details) -> func_sys_parse_param called error\n");
	free_callback(data);
	return -1;
}

static int func_fc_reply_header(clientHttpRequest *http, HttpHeader *hdr)
{
	char buf[65534];
    char from[8]="from";
	HttpHeaderEntry e;
	int fd = http->conn->fd;
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
    if(NULL == param)
    {
        debug(104, 1)("(mod_Powered-By-ChinaCache_header_details) param is NULL\n");   
        return 0;
    }
	
	if (strLen(param->name))
		e.name = stringDup(&param->name);
	else
		stringInit(&e.name, "Powered-By-ChinaCache");

    if (!strcasecmp(strBuf(e.name),"Webluker-Edge"))
        strcpy(from,"via");

	if (http->flags.hit && http->entry)
	{
		if (STORE_PENDING == http->entry->store_status)
			snprintf(buf, 65534, "PENDING %s %s", from,getMyHostname());
		else if (strLen(param->value) && !strcmp(strBuf(param->value), "log_type"))
			snprintf(buf, 65534, "%s %s %s", log_tags[http->log_type],from, getMyHostname());
		else 	
			snprintf(buf, 65534, "HIT %s %s", from,getMyHostname());
	}
	else
	{
		if ((strLen(param->value) && !strcmp(strBuf(param->value), "log_type")))
			snprintf(buf, 65534, "%s %s %s", log_tags[http->log_type],from, getMyHostname());
		else
			snprintf(buf, 65534, "MISS %s %s", from,getMyHostname());	
	}
	stringInit(&e.value, buf);
	e.id = HDR_OTHER;
	httpHeaderAddEntry(hdr, httpHeaderEntryClone(&e));
	stringClean(&e.name);
	stringClean(&e.value);
	return 1;
}

// module init 
int mod_register(cc_module *module)
{
	debug(104, 1)("(mod_Powered-By-ChinaCache_header_details) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
			func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_fc_reply_header,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fc_reply_header), 
			func_fc_reply_header);

    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else    
        mod = module; 

	return 0;

}

#endif

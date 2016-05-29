#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

#define MOD_NAME		"mod_header_three"
static cc_module* mod = NULL;

typedef struct _parse_param_value_st {
	char hdr[BUFSIZ];
	char w_val[BUFSIZ];
	char cdn_hdr[BUFSIZ];
}parse_param_value_t;

static MemPool * mod_config_pool = NULL;

static void * 
mod_config_pool_alloc(void)
{
    void * obj = NULL;

    if (NULL == mod_config_pool)
        mod_config_pool = memPoolCreate("mod_header_three config_struct", sizeof(parse_param_value_t));

    return obj = memPoolAlloc(mod_config_pool);
}

static int 
free_cfg(void * data)
{
    parse_param_value_t *params = data;
    if (NULL != params)
    {
        memPoolFree(mod_config_pool, params);
        params = NULL;
    }
    return 0;
}

static int 
funcSysParseParam(char *cfg_line)
{
	char *token = NULL;
	parse_param_value_t *params = NULL;

	token = strtok(cfg_line, w_space);
	if (NULL == token || strcmp(token, MOD_NAME))
	{
		debug(98, 5) ("%s:%s parsing module name failed\n", MOD_NAME, __func__);
		goto err;
	}
	params = (parse_param_value_t *)mod_config_pool_alloc();
	memset(params, '\0', sizeof(parse_param_value_t));	

	token = strtok(NULL, w_space);
	if (NULL == token)
	{
		debug(98, 5) ("%s:%s parsing modify header failed\n", MOD_NAME, __func__);
		goto err;
	}
	memcpy(params->hdr, token, strlen(token));
	
	token = strtok(NULL, w_space);
	if (NULL == token)
	{
		debug(98, 5) ("%s:%s parsing modify header value failed\n", MOD_NAME, __func__);
		goto err;
	}
	memcpy(params->w_val, token, strlen(token));

	token = strtok(NULL, w_space);
	if (NULL == token)
	{
		debug(98, 5) ("%s:%s parsing formal header filed\n", MOD_NAME, __func__);
		goto err;
	}
	memcpy(params->cdn_hdr, token, strlen(token));

	cc_register_mod_param(mod, params, free_cfg);
	return 0;
err:
    if (params != NULL)
        free_cfg(params);
    return -1;
}

static int
eatBlackString(char *s)
{
	while('\0' == s[0] || '\t' == s[0] || ' ' == s[0])
	{
		memmove(s, s + 1, strlen(s + 1));
		s[strlen(s + 1)] = '\0';
	}
	return 0;
}

static int
calculateCacheTime(char *buf, char *str)
{
	int ret = 0;
	int flag = 1;
	char *ptr = NULL;
	char *token = NULL;
	char *lasts = NULL;
	char befor[BUFSIZ] = {0};
	char after[BUFSIZ] = {0};
	char buf_bak[BUFSIZ] = {0};
	memcpy(buf_bak, buf, strlen(buf));

	do {
		if (flag)
			token = strtok_r(buf_bak, ",", &lasts);
		else
			token = strtok_r(NULL, ",", &lasts);

		if (NULL == token)
			break;

		flag = 0;
		if (NULL == (ptr = strchr(token, '=')))
			continue;
		
		if (NULL == (ptr + 1))
			continue;
		memset(befor, '\0', BUFSIZ);
		memcpy(befor, token, ptr - token);
		memset(after, '\0', BUFSIZ);
		memcpy(after, ptr + 1, strlen(ptr + 1));
		//eat black
		//eatBlackString(befor);
		eatBlackString(after);
	
		if (0 == strcmp(befor, str))
			ret = atoi(after);
	
	} while(1);
	
	return ret;
}

static int 
updateCacheTime(HttpStateData *data)
{
	int fd = data->fd;
	String s = StringNull;
	char val[BUFSIZ] = {0}; 

	if (!data || !data->entry || !data->entry->mem_obj || !data->entry->mem_obj->reply)
		return 0;

	parse_param_value_t * param = (parse_param_value_t *)cc_get_mod_param(fd, mod);

	http_hdr_type id = httpHeaderIdByNameDef(param->hdr, strlen(param->hdr));;
	s = httpHeaderGetList(&data->entry->mem_obj->reply->header, id);
	if (NULL == strBuf(s))
		return 0;
	memcpy(val, strBuf(s), strlen(strBuf(s)));

	int time_val = calculateCacheTime(val, param->w_val);
	if (0 == time_val)
		return 0;

	if (data->entry->timestamp < 0)
		data->entry->timestamp = squid_curtime;
	data->entry->expires = data->entry->timestamp + time_val;
	return 0;
}

static int
getValue(char *s)
{
	if (0 == strlen(s))
		return 0;
	char *token = NULL;
	char *lasts = NULL;

	token = strtok_r(s, "=", &lasts);
	if (NULL == token)
		return 0;
	token = strtok_r(NULL, "=", &lasts);
	if (NULL == token)
		return 0;
	return atoi(token);
}


static int 
deleteHttpHeaderValue(char *s, parse_param_value_t * param, char *d) 
{
	int ret = 0;
    char *ptr = NULL;
    char *ptr1 = NULL;
	char val[BUFSIZ] = {0};
    
    if (NULL == (ptr = strstr(s, param->w_val)))
        return 0;
    
	if ((calculateCacheTime(s, param->w_val)) == 0)
		return 0;

    memset(d, '\0', BUFSIZ);
    memcpy(d, s, ptr - s - 1); 

    if (NULL == (ptr1 = strchr(ptr, ',')))
	{
		memcpy(val, ptr, strlen(ptr));
		ret = getValue(val);
        return ret;
	}
	memcpy(val, ptr, ptr1 - ptr);
	ret = getValue(val);
	strncat(d, ptr1, strlen(ptr1));
    return ret;
}

static int
httpHeaderChangeByParam(HttpHeader *hdr, parse_param_value_t *param)
{
	int ret = 0;
	char value[BUFSIZ] = {0};

	http_hdr_type id = httpHeaderIdByNameDef(param->hdr, strlen(param->hdr));

    HttpHeaderPos pos = HttpHeaderInitPos;
    HttpHeaderEntry *hdr_entry;
    CBIT_CLR(hdr->mask, id);
	while ((hdr_entry = httpHeaderGetEntryPlus(hdr, &pos)))
	{   
		if (!strCaseCmp(hdr_entry->name, param->hdr))
		{
			if (strStr(hdr_entry->value, param->w_val))
			{
				ret = deleteHttpHeaderValue((char *)strBuf(hdr_entry->value), param, value);
				httpHeaderDelAt(hdr, hdr->pSearch[pos]);
				hdr->pSearch[pos] = -1;
				if (0 != strlen(value))
					httpHeaderAddEntry(hdr, httpHeaderEntryCreate(id, param->hdr, value));
			}
		}   
	}   
    return ret;
}

static int
httpHeaderAddCdnByParam(clientHttpRequest *http, parse_param_value_t *param)
{
	if (http->entry->expires - http->entry->timestamp > 0)
	{
		httpHeaderAddEntry(&http->reply->header, 
				httpHeaderEntryCreate(HDR_OTHER, param->cdn_hdr, 
					xitoa(http->entry->expires - http->entry->timestamp)));
	}
	return 0;
}

static int 
hookMcpSetReplyHeader(clientHttpRequest *http)
{
	if (!http || !http->reply || !http->entry)
		return 0;
	parse_param_value_t * param = (parse_param_value_t *)cc_get_mod_param(http->conn->fd, mod);
	int ret = httpHeaderChangeByParam(&http->reply->header, param);
	if (0 < ret && (ret != http->entry->expires - http->entry->timestamp))
		http->entry->expires = http->entry->timestamp + ret;
	httpHeaderAddCdnByParam(http, param);
    return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(98, 1)("(mod_header) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.x86_64");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
            funcSysParseParam);

	cc_register_hook_handler(HPIDX_hook_func_store_append_check,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_store_append_check),
            updateCacheTime);

    cc_register_hook_handler(HPIDX_hook_func_private_process_send_header,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_process_send_header),
            hookMcpSetReplyHeader);
	
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 1;
}
#endif

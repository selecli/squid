/*******************************************************************************
* Copyright (C) 2012
*
* File name: mod_url_range.c
* File ID:
* Summary:
* Description: Use to support the range info from url
* Version: V1.0
* Author: Haobo.Zhang
* Date: 2012-5-24
********************************************************************************/


#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;

struct mod_conf_param {
	char*  	strStart;	//the string before range start num
	char*	strEnd;		//the string before range end num
};

static MemPool *  mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_url_range config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* pConfig = (struct mod_conf_param*) param;
	if(pConfig->strStart)
	{
		xfree(pConfig->strStart);
	}
	if(pConfig->strEnd)
	{
		xfree(pConfig->strEnd);
	}
	memPoolFree(mod_config_pool, pConfig);
	pConfig = NULL;
	return 0;
}

// confige line example:
// mod_mod_conf_param regex_match regex_change allow/deny acl
static int func_sys_parse_param(char* cfg_line)
{
	struct mod_conf_param* reg= mod_config_pool_alloc();

	char* token = NULL;

	//mod_mod_conf_param
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{       
		if(strcmp(token, "mod_url_range"))
		{
			memPoolFree(mod_config_pool, reg);
			return -1;
		}
	}       
	else    
	{       
		debug(120, 3)("(mod_url_range) ->  parse line error\n");
		memPoolFree(mod_config_pool, reg);
		return -1;       
	}       


	if(NULL == (token=strtok(NULL, w_space)))
	{       
		debug(120, 0) ("%s line %d: %s\n", cfg_filename, config_lineno,
				config_input_line);
		debug(120, 0) ("parse_url_range: missing match string.\n");
		memPoolFree(mod_config_pool, reg);
		reg = NULL;
		return -1; 
	}

	reg->strStart = xmalloc(strlen(token) + 1);
	strcpy(reg->strStart, token);

	if(NULL == (token=strtok(NULL, w_space)))
	{
		debug(120, 0) ("%s line %d: %s\n", cfg_filename, config_lineno,
				config_input_line);
		debug(120, 0) ("parse_mod_conf_param: missing replace string.\n");
		memPoolFree(mod_config_pool, reg);
		reg = NULL;
		return -1;
	}
	reg->strEnd = xmalloc(strlen(token) + 1);
	strcpy(reg->strEnd, token);

	cc_register_mod_param(mod, reg, free_callback);
	return 0;	
}

//get the number after the param string
int get_param_num(char* strBase, char* strParam)
{
	char* token = strstr(strBase, strParam);
	if(token)
	{
		token += strlen(strParam);
		return atoi(token);
	}
	return -1;
}

//delete the range info from the url
void DeleteRangeUrl(char* strUrl, struct mod_conf_param *param)
{
	char * pRange = strstr(strUrl, param->strStart);
	if(pRange)
	{
		char * pNext = strstr(pRange, "&");
		if(pNext)
			memcpy(pRange, pNext + 1, strlen(pNext));
		else if(*(pRange - 1) == '&' || *(pRange - 1) == '?')
			*(pRange - 1) = '\0';
		else
			pRange[0] = '\0';
	}
	pRange = strstr(strUrl, param->strEnd);
	if(pRange)
	{
		char * pNext = strstr(pRange, "&");
		if(pNext)
			memcpy(pRange, pNext + 1, strlen(pNext));
		else if(*(pRange - 1) == '&' || *(pRange - 1) == '?')
			*(pRange - 1) = '\0';
		else
			pRange[0] = '\0';
	}
	
}

//take the range info from url, and modify request header
static int range_set(clientHttpRequest *http, struct mod_conf_param *param)
{
	int nRangeStart = get_param_num(http->uri, param->strStart);
	int nRangeEnd = get_param_num(http->uri, param->strEnd);

	if(-1 == nRangeStart && -1 == nRangeEnd)
	{
		return 1;
	}
	if(nRangeStart > nRangeEnd && nRangeEnd > 0)
	{
		ErrorState *err = NULL;
		err = errorCon(ERR_INVALID_URL, HTTP_RANGE_NOT_SATISFIABLE, http->request);
		err->url = xstrdup(http->uri);
		http->al.http.code = err->http_status;
		http->log_type = LOG_TCP_DENIED;
		http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
		errorAppendEntry(http->entry, err);
		debug(120, 0) ("mod_url_range: reply 416 ccrangeend(%d) < ccrangestart(%d)", nRangeEnd, nRangeStart);
		return 3;
	}
	if(-1 == nRangeStart)
		nRangeStart = 0;
	if(-1 == nRangeEnd)
		nRangeEnd = 0;
	debug(120, 0) ("mod_url_range: get ccrange request, url = %s\n", http->uri);
	DeleteRangeUrl(http->uri, param);
	DeleteRangeUrl(http->request->urlpath.buf, param);
	http->request->urlpath.len = strlen(http->request->urlpath.buf);
	DeleteRangeUrl(http->orig_request->urlpath.buf, param);
	http->orig_request->urlpath.len = strlen(http->orig_request->urlpath.buf);
	DeleteRangeUrl(http->request->canonical, param);
	DeleteRangeUrl(http->orig_request->canonical, param);
	char* strRangeValue = xmalloc(50);
	if(!nRangeEnd)
		snprintf(strRangeValue, 50, "bytes=%d-", nRangeStart);
	else
		snprintf(strRangeValue, 50, "bytes=%d-%d", nRangeStart, nRangeEnd);
	HttpHeaderEntry * pEntry = httpHeaderEntryCreate(HDR_RANGE, "Range", strRangeValue);
	xfree(strRangeValue);
	
	debug(120, 0) ("mod_url_range: range_set done, url = %s\n", http->uri);
	httpHeaderAddEntry(&(http->request->header), pEntry);

	return 1;
}

static int func_http_req_parsed(clientHttpRequest *http)
{

	int fd = http->conn->fd;

	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	
	int nRet = range_set(http, param);
	if(!nRet)
		debug(120, 0) ("mod_url_range: range_set failed.\n");
	
	return nRet;
}

//before request send to the server, check if have range header ,resume the range info to url,else make the range info to url
static int func_check_request_change(MemBuf * mb, HttpStateData * httpState)
{
	int nRangeStart = 0, nRangeEnd = 0;
	if(httpState->request->range->specs.count)
	{
		nRangeStart = ((HttpHdrRangeSpec *)(httpState->request->range->specs.items[0]))->offset;
		int nLenth = ((HttpHdrRangeSpec *)(httpState->request->range->specs.items[0]))->length;
		nRangeEnd = nRangeStart + nLenth - 1;
	}
	int fd = httpState->fd;
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	char strRange[256] = {0};
	char strURL[4096] = {0};
	char * p1 = strstr(mb->buf, " HTTP");
	char * p2 = strstr(mb->buf, "Range:");
	char * p3 = NULL;
	if(p2)
	{
		snprintf(strRange, 256, "%s%d%s%d", param->strStart, nRangeStart, param->strEnd, nRangeEnd);
		p3 = strstr(p2, "\r\n");
		char * p = strURL;
		memcpy(p, mb->buf, p1 - mb->buf);
		p += (p1 - mb->buf);
		memcpy(p, strRange, strlen(strRange));
		p += strlen(strRange);
		memcpy(p, p1, p2 - p1);
		p += (p2 - p1);
		memcpy(p, p3 + 2, strlen(p3) - 2);
	}
	else
	{
		snprintf(strRange, 256, "%s%d", param->strStart, 0);
		char * p = strURL;
		memcpy(p, mb->buf, p1 - mb->buf);
		p += (p1 - mb->buf);
		memcpy(p, strRange, strlen(strRange));
		p += strlen(strRange);
		memcpy(p, p1, strlen(p1));
	}
	memset(mb->buf, 0, mb->size);
	mb->size = 0;
	memBufAppend(mb, strURL, strlen(strURL));
	return 1;
}

static int hook_cleanup(cc_module *module)
{
	debug(120, 1)("(mod_url_range) -> hook_cleanup:\n");
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(120, 1)("(mod_mod_conf_param) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_http_before_redirect = func_http_befoee_redirect;
	cc_register_hook_handler(HPIDX_hook_func_http_req_parsed,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_parsed),
			func_http_req_parsed);
	
	cc_register_hook_handler(HPIDX_hook_func_http_check_request_change,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_check_request_change),
			func_check_request_change);

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

/*
 * Autor: xin.yao
 * Date: 2012-11-19
 * Description: for microsoft 
 * modify response header: Age, Expires, Cache-Control, Content-Disposition 
 * see case: 3820
 */

#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

typedef struct request_private_data_st {
	char *downloadname;
} pda_t;

typedef struct cfg_params_st {
	unsigned int max_age:1;
	unsigned int filename:1;
	unsigned int all:1;
} cfg_params_t;

static cc_module *mod = NULL;
static MemPool *cfg_pool = NULL;
static MemPool *pda_pool = NULL;

static void *cfgParamsPoolAlloc(void)
{
	void *obj = NULL;

	if (NULL == cfg_pool)
	{
		cfg_pool = memPoolCreate("mod_modify_s2c_header cfg_params", sizeof(cfg_params_t));
	}
	obj = memPoolAlloc(cfg_pool);

	return obj;
}

static void *requestPrivateDataPoolAlloc(void)
{
	void *obj = NULL;

	if (NULL == pda_pool)
	{
		pda_pool = memPoolCreate("mod_modify_s2c_header request_private_data", sizeof(pda_t));
	}
	obj = memPoolAlloc(pda_pool);

	return obj;
}

static int freeCfgParams(void *data)
{
	cfg_params_t *params = data;
	if (NULL != params)
	{
		memPoolFree(cfg_pool, params);		
		params = NULL;
	}
	return 0;
}

static int freeRequestPrivateData(void *data)
{
	pda_t *pda = data;

	if (NULL != pda)
	{
		if (NULL != pda->downloadname)
			safe_free(pda->downloadname);
		memPoolFree(pda_pool, pda);
		pda = NULL;
	}

	return 0;
}

HttpHeaderEntry *httpHeaderFindEntry2(const HttpHeader * hdr, http_hdr_type id)
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *e;

	/* check mask first */
	if (!CBIT_TEST(hdr->mask, id)) 
		return NULL;
	/* looks like we must have it, do linear search */
	while ((e = httpHeaderGetEntry(hdr, &pos)))
	{    
		if (e->id == id)
			return e;
	}    
	return NULL;
}

static void httpReplyHeaderByMaxage(clientHttpRequest *http)
{
	char buffer[4096];
	char max_age_buf[256];
	time_t max_age, age = 0;
	const char *pctl, *p, *pp;
	HttpReply *reply = http->reply;
	HttpHeader *hdr = &reply->header;
	HttpHeaderEntry *hdr_ctl, *hdr_age;

	hdr_ctl = httpHeaderFindEntry2(hdr, HDR_CACHE_CONTROL);
	if (NULL == hdr_ctl)
	{
		debug(223, 3)("mod_modify_s2c_header: reply headers have no 'Cache-Control'\n");
		return;
	}
	debug(223, 3)("mod_modify_s2c_header: reply header 'Cache-Control' is: %s\n", strBuf(hdr_ctl->value));
	pctl = strBuf(hdr_ctl->value);
	p = strstr(pctl, "max-age=");
	if (NULL == p)
	{
		debug(223, 3)("mod_modify_s2c_header: 'Cache-Control' have no 'max-age='\n");
		return;
	}
	hdr_age = httpHeaderFindEntry2(hdr, HDR_AGE);
	if (NULL == hdr_age)
	{
		age = 0;
		debug(223, 3)("mod_modify_s2c_header: reply headers have no 'Age', set age=0\n");	
	}
	else
	{
		age = atol(strBuf(hdr_age->value));
		debug(223, 3)("mod_modify_s2c_header: reply header 'Age' is: %s\n", strBuf(hdr_age->value));
	}
	//8: strlen("max-age=")
	max_age = atol(p + 8) - age;
	memset(buffer, 0, 4096);
	memset(max_age_buf, 0, 256);
	snprintf(max_age_buf, 256, "%ld", (long)max_age);
	memcpy(buffer, pctl, p - pctl + 8);
	strcat(buffer, max_age_buf);
	pp = strchr(p, ',');
	if (NULL != pp)
		strcat(buffer, pp);
	stringReset(&(hdr_ctl->value), buffer);
	debug(223, 3)("mod_modify_s2c_header: reply header 'Cache-Control' new value: %s\n", buffer);
	//delete header 'Age' and 'Expires'
	httpHeaderDelById(hdr, HDR_AGE);
	httpHeaderDelById(hdr, HDR_EXPIRES);
}

static void httpReplyHeaderByFilename(clientHttpRequest *http)
{
	char buffer[4096];
	HttpReply *reply = http->reply;
	HttpHeader *hdr = &reply->header;
	HttpHeaderEntry *hdr_disp, e;
	pda_t *pda = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if (NULL == pda)
	{
		debug(223, 3)("mod_modify_s2c_header: no register 'downloadname' for: %s\n", http->uri);
		return;
	}
	//add reply header: Content-Disposition
	hdr_disp = httpHeaderFindEntry2(hdr, HDR_CONTENT_DISPOSITION);
	if (NULL != hdr_disp)
	{
		debug(223, 3)("mod_modify_s2c_header: reply header 'Content-Disposition' is: %s\n", strBuf(hdr_disp->value));
		httpHeaderDelById(hdr, HDR_CONTENT_DISPOSITION);
	}
	memset(buffer, 0, 4096);
	snprintf(buffer, 4096, 
			"attachment; filename=\"%s\"",
			pda->downloadname);
	e.id = HDR_CONTENT_DISPOSITION;
	stringInit(&e.name, "Content-Disposition");
	stringInit(&e.value, buffer);
	httpHeaderAddEntry(hdr, httpHeaderEntryClone(&e));
	stringClean(&e.name);
	stringClean(&e.value);
	debug(223, 3)("mod_modify_s2c_header: reply header 'Content-Disposition' new value: %s\n", buffer);
}

static int httpReplySendStart(clientHttpRequest *http)
{
	int flag = 0;
	Array *params_array;
	cfg_params_t *params;
	int i, max_age, filename, all;

	params_array = cc_get_mod_param_array(http->conn->fd, mod);

	if (NULL == params_array || 0 == params_array->count)
	{
		debug(223, 3)("mod_modify_s2c_header: no register params\n");
		return 0;
	}
	max_age = filename = all = 0;
	for (i = 0; i < params_array->count; ++i)
	{
		params = ((cc_mod_param *)params_array->items[i])->param;
		if (params->max_age)
			max_age = 1;
		if (params->filename)
			filename = 1;
		if (params->all)
			all = 1;
		debug(223, 3)("mod_modify_s2c_header: params_array[%d] (all)%d (max_age)%d (filename)%d\n", i, params->all, params->max_age, params->filename);
	}

	if (all)
	{
		flag = 1;
		httpReplyHeaderByMaxage(http);
		httpReplyHeaderByFilename(http);
		return 0;
	}
	if (max_age)
	{
		flag = 1;
		httpReplyHeaderByMaxage(http);
	}
	if (filename)
	{
		flag = 1;
		httpReplyHeaderByFilename(http);
	}
	if (!flag)
	{
		debug(223, 3)("mod_modify_s2_header: no register params\n");
	}

	return 0;
}

static void getFileNameByDownloadName(char *ptr, pda_t **pda)
{
	//13: strlen("downloadname=");
	char *p = ptr + 13;
	char *pp = NULL;

	//if not have filename
	if ('\0' == p)
		return;
	*pda = requestPrivateDataPoolAlloc();
	(*pda)->downloadname = NULL;
	//if have other parameters delimit by '&'
	pp = strchr(p, '&');
	(*pda)->downloadname = (NULL == pp) ? xstrdup(p) : xstrndup(p, pp - p + 1);
}

static int httpBeforeRedirectRead(clientHttpRequest *http)
{
	char *p1, *p2;
	pda_t *pda = NULL;

	if (NULL == http || NULL == http->uri)
		return 0;
	debug(223, 2)("http->uri: %s\n", http->uri);
	p1 = strstr(http->uri, "?");
	//if not have any parameters
	if (NULL == p1)
		return 0;
	p2 = strstr(http->uri, "downloadname=");
	//if not have parameter name "downloadname"
	//or "downloadname" address less than "?" in request uri
	if (NULL == p2 || p2 < p1)
		return 0;
	getFileNameByDownloadName(p2, &pda);
	if (NULL == pda)
		return 0;
	if (NULL == pda->downloadname)
	{
		freeRequestPrivateData(pda);
		return 0;
	}
	//register 'dowloadname' for reply header: Content-Desposition
	cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, pda, freeRequestPrivateData, mod);

	return 1;
}

static int parseModuleOptions(cfg_params_t *params, char *token)
{
	if (!strcmp(token, "max_age"))
	{
		params->max_age = 1;
		return 1;
	}
	if (!strcmp(token, "filename"))
	{
		params->filename = 1;
		return 1;
	}
	if (!strcmp(token, "all"))
	{
		params->all = 1;
		return 1;
	}
	debug(223, 3)("mod_modify_s2c_header: no this module options %s\n", token);
	return 0;
}

static int parseParameters(char *cfg_line)
{
	char *token;
	cfg_params_t *params;

	token = strtok(cfg_line, w_space);
	if (NULL == token)
	{
		debug(223, 3)("mod_modify_s2c_header: parse module name error\n");	
		return -1;
	}
	if (strcmp(token, "mod_modify_s2c_header"))
	{
		debug(223, 3)("mod_modify_s2c_header: module name error\n");
		return -1;
	}
	token = strtok(NULL, w_space);
	if (NULL == token)
	{
		debug(223, 3)("mod_modify_s2c_header: module options error\n");
		return -1;
	}
	if (!strcmp(token, "allow") || !strcmp(token, "deny"))
	{
		debug(223, 3)("mod_modify_s2c_header: lack module options\n");
		return -1;
	}
	params = cfgParamsPoolAlloc();
	if (!parseModuleOptions(params, token))
		return -1;
	cc_register_mod_param(mod, params, freeCfgParams);

	return 0;
}

static int cleanUp(cc_module *module)
{
	if (NULL != pda_pool)
		memPoolDestroy(pda_pool);

	return 0;
}

int mod_register(cc_module *module)
{
	debug(223, 1)("mod_modify_s2c_header: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			parseParameters);

	cc_register_hook_handler(HPIDX_hook_func_http_before_redirect_read,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect_read),
			httpBeforeRedirectRead);

	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			httpReplySendStart);
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanUp);

	if (reconfigure_in_thread)
		mod = (cc_module *)cc_modules.items[module->slot];
	else
		mod = module;

	return 0;
}

#endif


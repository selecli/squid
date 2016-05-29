/*
 * This module support five ways to modify the store_url of a StoreEntry :
 * solution 1: Give store_url to an helper program on standard-out, then read it back from standard-in.
 * solution 2: Make a regular expression for store_url.
 * solution 3: Keep some specified arguments of url, ignore rest. 
 * solution 4: ignore case. 
 * solution 5: ignore question.
 *
 * Author: chenqi.
 * Date: 2012/09/07.
 * Last updated @ 2012/12/19.
 */
#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK
#define REPLACE_REGEX_SUBSTR_NUMBER 99
static regmatch_t g_pmatch[REPLACE_REGEX_SUBSTR_NUMBER+1];
typedef struct _StoreUrlRewriteStateData
{
	void * data;
	RH * handler;
} StoreUrlRewriteStateData;

CBDATA_TYPE(StoreUrlRewriteStateData);
static MemPool *mod_config_pool = NULL;

typedef enum{
	HELPER,
	ARG,
	ARG_REMOVE,
	REG,
	ignore_case,
	ignore_question,
	http_version
} rewrite_type;

static cc_module* mod = NULL;

struct mod_conf_param {
	rewrite_type type;
	// ARG
	wordlist *arg;     

	// HELPER
	int hlpchildren;
	int hlpconcurrency;
	wordlist *hlpcmdline;
	helper *storeurlors;

	// REG
	regex_t reg;
	char** replace;
};

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{   
		mod_config_pool = memPoolCreate("mod_store_url_rewrite config_struct", sizeof(struct mod_conf_param));
	}   
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if (NULL == data)
		return 0;

	if (ARG == data->type) 
		wordlistDestroy(&data->arg);
	else if (ARG_REMOVE == data->type)
		wordlistDestroy(&data->arg);
	else if (HELPER == data->type) 
		wordlistDestroy(&data->hlpcmdline);
	else if(REG == data->type)
	{
		int i;
		regfree(&data->reg);
		for(i=0;;i+=2)
		{   
			if(data->replace[i])
				xfree(data->replace[i]);
			else
				break;
			if(!data->replace[i+1])
				break;
		}
	}

	memPoolFree(mod_config_pool, data);
	data = NULL;
	return 0;
}

//========= regular expression =================//
static inline int analysisRegexMatch(const char* buffer, int incase, struct mod_conf_param* reg)
{
	int ret = 0;
	const char* str = buffer;
	while(NULL != (str=strchr(str, ')')))
	{   
		if('\\' != *(str-1))
		{   
			ret++;
		}   
		str++;
	}   
	if(ret > REPLACE_REGEX_SUBSTR_NUMBER)
	{   
		return -1; 
	}   
	int flags = REG_EXTENDED;
	if(incase)
	{   
		flags |= REG_ICASE;
	}   
	if(0 != regcomp(&reg->reg, buffer, flags))
	{   
		return -1; 
	}   
	return ret;
}

static inline int analysisRegexReplace(const char* buffer, struct mod_conf_param* reg)
{
	int ret = 0;
	char* temp_replace[REPLACE_REGEX_SUBSTR_NUMBER*2+2];
	const char* str = buffer;
	const char* str2;
	int i = 0;
	while(NULL != (str2 = strchr(str, '\'')))
	{
		temp_replace[i] = xmalloc(str2-str+1);
		memcpy(temp_replace[i], str, str2-str);
		temp_replace[i][str2-str] = 0;
		temp_replace[i+1] = (char*)atol(str2+1);
		if((long int)temp_replace[i+1] > ret)
		{
			ret = (long int)temp_replace[i+1];
		}
		if((long int)temp_replace[i+1] > 9)
		{
			str = str2 + 3;
		}
		else
		{
			str = str2 + 2;
		}
		i += 2;
	}
	if(0 != *str)
	{
		temp_replace[i] = xmalloc(strlen(str)+1);
		strcpy(temp_replace[i], str);
		i++;
	}
	temp_replace[i] = 0;
	i++;
	reg->replace = (char**)xmalloc(i*sizeof(char*));
	memcpy(reg->replace, temp_replace, i*sizeof(char*));
	return ret;
}


//========== parse =============//
static int parse_param_reg(struct mod_conf_param* data)
{
	int ret1, ret2;
	char *token = NULL;
	if (NULL == (token = strtok(NULL, w_space)))
		return -1;

	if ((ret1 = analysisRegexMatch(token, 1, data)) < 0)
		return -1; 

	if (NULL == (token = strtok(NULL, w_space)))
		return -1;

	if ((ret2 = analysisRegexReplace(token, data)) < 0)
		return -1;

	if (ret1 < ret2)
	{
		debug(150, 1)("mod_store_url_rewrite: replace string number is too large\n");
		return -1;
	}

	debug(150,1)("mod_store_url_rewrite: regular expression\n");
	return 0;
}

static int parse_param_arg(struct mod_conf_param* data)
{
	char *token = NULL;
	while (NULL != (token = strtok(NULL, w_space)))
	{
		if (!strcmp(token, "allow"))
			break;
		else if(!strcmp(token, "deny"))
			break;
		else
		{
			wordlistAdd(&data->arg, token);
			debug(150,1)("mod_store_url_rewrite: got argument [%s] form url\n",token);
		}
	}

	if (NULL == data->arg)
		return -1;
	return 0;
}

static int parse_param_hlp(struct mod_conf_param* data)
{
	char *token = NULL;
	if (NULL == (token = strtok(NULL, w_space)))
		return -1;
	data->hlpchildren = atoi(token);
	if (data->hlpchildren <= 0)
		return -1;

	if (NULL == (token = strtok(NULL, w_space)))
		return -1;
	data->hlpconcurrency = atoi(token);
	if (data->hlpconcurrency <= 0)
		return -1;

	wordlist *line  = NULL;
	while (NULL != (token = strtok(NULL, w_space))) {
		if (!strcmp(token, "allow") || !strcmp(token,"deny"))
			break;
		wordlistAdd(&line, token);
	}

	if (line) {
		data->hlpcmdline = line;
		debug(154, 1) ("mod_store_url_rewrite: param hlpchildren = %d hlpconcurrency = %d hlpcmdline = %s\n", data->hlpchildren, data->hlpconcurrency, data->hlpcmdline->key);
		return 0;
	}
	else
		return -1;
}

static int func_sys_parse_param(char* cfg_line)
{
	int ret;
	char* token = NULL;

	struct mod_conf_param* data = mod_config_pool_alloc();
	memset(data, 0, sizeof(struct mod_conf_param));

	if (NULL == (token = strtok(cfg_line, w_space)))
		goto err;
	if (strcmp(token, "mod_store_url_rewrite"))
		goto err;

	if (NULL == (token = strtok(NULL, w_space)))
		goto err;

	if (0 == strcasecmp(token, "reg"))
		data->type = REG;
	else if (0 == strcasecmp(token, "arg"))
		data->type = ARG;
	else if (0 == strcasecmp(token, "arg_remove"))
		data->type = ARG_REMOVE;
	else if (0 == strcasecmp(token, "helper"))
		data->type = HELPER;
	else if (0 == strcasecmp(token, "ignore_case"))
		data->type = ignore_case;
	else if (0 == strcasecmp(token, "ignore_question"))
		data->type = ignore_question;
	else if (0 == strcasecmp(token, "http_version"))
		data->type = http_version;
	else 
		goto err;

	switch(data->type) {
		case REG:
			ret = parse_param_reg(data);
			break;
		case ARG:
			ret = parse_param_arg(data);
			break;
		case ARG_REMOVE:
			ret = parse_param_arg(data);
			break;
		case HELPER:
			ret = parse_param_hlp(data);
			break;
		case ignore_case:       // no parameter
			ret = 0;
			break;
		case ignore_question:	// no parameter
			ret = 0;
			break;
		case http_version:      // no parameter
			ret = 0;
			break;
		default:
			goto err;
	}

	if (ret)
		goto err;

	cc_register_mod_param(mod, data, free_callback);
	debug(150,5)("mod_store_url_rewrite: %s parse successfully\n", cfg_line);
	return 0;

err:
	debug(150,1)("mod_store_url_rewrite: %s parse error\n", cfg_line);
	free_callback(data);
	return -1;
}

// HELPER MODE
static void func_StoreUrlHelperInit(struct mod_conf_param *cfg)
{
	if (NULL == cfg->storeurlors)
	{   
		cfg->storeurlors = helperCreate("mod_store_url_rewrite");
		cfg->storeurlors->cmdline = cfg->hlpcmdline;
		cfg->storeurlors->n_to_start = cfg->hlpchildren;
		cfg->storeurlors->concurrency = cfg->hlpconcurrency;
		cfg->storeurlors->ipc_type = IPC_STREAM;
		helperOpenServers(cfg->storeurlors);
		CBDATA_INIT_TYPE(StoreUrlRewriteStateData);
		debug(150, 2)("mod_store_url_rewrite: Init helpers\n");
	}   
}

static void func_StoreUrlHelperShutdown(int redirect_should_reload)
{
	debug(150,1)("mod_store_url_rewrite: func_StoreUrlHelperShutdown Start\n");
	int i=0;
	cc_mod_param *mod_param;
	struct mod_conf_param *cfg;
	for (i=0; i<mod->mod_params.count; i++)
	{
		mod_param = (cc_mod_param*) mod->mod_params.items[i];
		cfg  = (struct mod_conf_param*) mod_param->param;
		debug(150, 5)("mod_store_url_rewrite: type=%d\n",cfg->type);
		if (cfg->type == HELPER && cfg->storeurlors)
		{
			helperShutdown(cfg->storeurlors);  
			helperFree(cfg->storeurlors);
			cfg->storeurlors = NULL;
			debug(150, 2)("mod_store_url_rewrite: clean up previous storeurlors when reconfiguring\n");
		}
	}
	debug(150,1)("mod_store_url_rewrite: func_StoreUrlHelperShutdown Done!\n");
}

static void StoreUrlHandleReply(void *data, char *reply)
{
	StoreUrlRewriteStateData *state = data;
	int valid;
	char *t;
	if (reply)
	{
		if ((t = strchr(reply, ' ')))
			*t = '\0';
		if (*reply == '\0')
			reply = NULL;
	}
	debug(150, 5) ("mod_store_url_rewrite: StoreUrlHandleReply {%s}\n", reply ? reply : "<NULL>");
	valid = cbdataValid(state->data);
	cbdataUnlock(state->data);
	if (valid)
		state->handler(state->data, reply);
	cbdataFree(state);
}

static void StoreUrlRewriteDone(void * data, char *reply)
{
	clientHttpRequest *http = data;
	debug(150, 3) ("mod_store_url_rewrite: '%s' result=%s\n", http->uri, reply ? reply : "NULL");
	if (reply)
	{   
		safe_free(http->request->store_url);
		http->request->store_url = xstrdup(reply);
		debug(150, 3) ("Rewrote to %s\n", http->request->store_url);
	}   
	clientStoreURLRewriteStart(http);       // mod_store_url_rewrite takes effect before storeURLRewrite
}

static void dealwith_helper(clientHttpRequest *http, struct mod_conf_param *param)
{
	char buf[8192];
	func_StoreUrlHelperInit(param);
	StoreUrlRewriteStateData *state;
	state = cbdataAlloc(StoreUrlRewriteStateData);
	state->data = http;
	state->handler = StoreUrlRewriteDone;
	cbdataLock(state->data);
	snprintf(buf, 8192, "%s\n", http->uri);
	debug(150,5)("mod_store_url_rewrite:  sending '%s' to the helper\n", buf);
	helperSubmit(param->storeurlors, buf, StoreUrlHandleReply, state); // maybe we should add HTTP headers to buf 
} 


// ARG MODE
int IsArg(char *arg ,struct mod_conf_param *param)
{
	char key[128];
	memset(key,0,128);
	char *assign = strchr(arg,'=');
	if (NULL == assign)
		return 0;
	strncpy(key, arg, assign - arg);

	wordlist *word = param->arg;
	while (word) {
		if (!strcmp(key, word->key)) {
			return 1;
		}
		word = word->next;
	}
	return 0;
}

static void dealwith_arg(clientHttpRequest *http, struct mod_conf_param *param)
{
	char *start, *end;
	char uri[2048]; 
	strcpy(uri, http->uri);
	if (NULL == (start = strchr(uri, '?'))) 
		return;
	start++;

	while (1) {
		end = strchr(start, '&');
		if (NULL == end) {
			if (0 == IsArg(start, param))
				*start = '\0';
			break;
		}
		if (0 == IsArg(start, param))
			memmove(start, end+1,strlen(end));
		else
			start = end + 1;
	}

	end = uri+strlen(uri)-1;
	if ('?' == *end || '&' == *end)
		*end = '\0';
	safe_free(http->request->store_url);
	http->request->store_url = xstrdup(uri);
	debug(150,2)("mod_store_url_rewrite: now storeurl = [%s]\n",http->request->store_url);
}

static void dealwith_arg_remove(clientHttpRequest *http, struct mod_conf_param *param)
{
	char *start, *end;
	char uri[2048]; 
	strcpy(uri, http->uri);
	if (NULL == (start = strchr(uri, '?'))) 
		return;
	start++;

	while (1) {
		end = strchr(start, '&');
		if (NULL == end) {
			if (IsArg(start, param))
				*start = '\0';
			break;
		}
		if (IsArg(start, param))
			memmove(start, end + 1, strlen(end));
		else
			start = end + 1;
	}

	end = uri + strlen(uri) - 1;
	if ('?' == *end || '&' == *end)
		*end = '\0';
	safe_free(http->request->store_url);
	http->request->store_url = xstrdup(uri);
	debug(150,2)("mod_store_url_rewrite: now storeurl = [%s]\n",http->request->store_url);
}

// REG MODE
static void dealwith_reg(clientHttpRequest *http, struct mod_conf_param *param)
{
	int ret =0;
	ret = regexec(&param->reg, http->uri, REPLACE_REGEX_SUBSTR_NUMBER+1, g_pmatch, 0); 
	if (ret) 
	{   
		debug(150, 3)("mod_store_url_rewrite: do not match regex, uri=[%s]\n", http->uri);
		return; 
	}    

	String url = StringNull;
	stringAppend(&url, param->replace[0], strlen(param->replace[0]));
	int i = 1;
	while (0 != (ret = (long int) param->replace[i]))
	{    
		stringAppend(&url, http->uri+g_pmatch[ret].rm_so, g_pmatch[ret].rm_eo-g_pmatch[ret].rm_so);
		debug(150,3)("mod_store_url_rewrite: rm_so=%d, rm_eo=%d\n", g_pmatch[ret].rm_so, g_pmatch[ret].rm_eo);
		i++;    
		if(param->replace[i])
		{    
			stringAppend(&url, param->replace[i], strlen(param->replace[i]));
			i++;    
		}    
		else    
		{    
			break;
		}    
	}    

	safe_free(http->request->store_url);
	http->request->store_url = xstrdup(url.buf);
	debug(150,2)("mod_store_url_rewrite: now storeurl = [%s]\n",http->request->store_url);
	stringClean(&url);
}

// ignore case
static void dealwith_ignore_case(clientHttpRequest *http)
{
	char *url = xstrdup(http->uri);
	char *tmp = url;
	char *end = strstr(url, "@@@");
	while ('\0' != *tmp)
	{
		if (end && tmp == end)
			break;
		if (*tmp >= 'A' && *tmp <= 'Z') 
			*tmp += 'a' - 'A';
		tmp++;
	}

	safe_free(http->request->store_url);
	http->request->store_url = url;
	debug(150,2)("mod_store_url_rewrite: now storeurl = [%s]\n",http->request->store_url);
}

// ignore question of a url
static void dealwith_ignore_question(clientHttpRequest *http)
{
	char *question = NULL;
	if (NULL == (question = strrchr(http->uri, '?'))) {
		debug(150,2)("mod_store_url_rewrite: There is no '?' in the [%s]\n",http->uri);
		return;
	}   

	int len = question - http->uri;
	if (len <= 0)
		return;

	char *url = xstrdup(http->uri);
	question = strrchr(url, '?');
	assert(question);
	char *end = strstr(url, "@@@"); // avoid conflict with mod_header_combination
	if (end) 
		memmove(question, end, strlen(end) + 1); 
	else 
		*question = '\0';

	safe_free(http->request->store_url);
	http->request->store_url = url;
	debug(150,2)("mod_store_url_rewrite: now storeurl = [%s]\n",http->request->store_url);
}

// ==============================================================================================
//create storeurl by http version for the same uri
//added by xin.yao: 2012-11-15
static void dealwith_http_version(clientHttpRequest *http)
{
	const char *suffix = NULL;
	const char *suffix_ver10 = "http10";
	const char *suffix_ver11 = "http11";
	http_version_t *http_ver = &http->request->http_ver;

	debug(150, 2)("mod_store_url_rewrite: major: %d, minor: %d\n", http_ver->major, http_ver->minor);

	if (1 == http_ver->major && 0 == http_ver->minor)
		suffix = suffix_ver10;
	else if (1 == http_ver->major && 1 == http_ver->minor)
		suffix = suffix_ver11;
	else
		return; //other: do nothing
	String new_storeurl = StringNull;
	stringAppend(&new_storeurl, http->uri, strlen(http->uri));
	stringAppend(&new_storeurl, suffix, strlen(suffix));
	safe_free(http->request->store_url);
	http->request->store_url = xstrdup(new_storeurl.buf);
	stringClean(&new_storeurl);
	debug(150, 2)("mod_store_url_rewrite: now storeurl = [%s]\n", http->request->store_url);
}

static int modifyHttpVersionToOrigin(HttpStateData *httpState)
{
	int fd = httpState->fd;
	struct mod_conf_param *param = cc_get_mod_param(fd, mod);

	if (param->type != http_version)
		return 1;
	http_version_t *http_ver = &httpState->request->http_ver;
	if (1 == http_ver->major && 0 == http_ver->minor)
		httpState->flags.http11 = 0;
	else if (1 == http_ver->major && 1 == http_ver->minor)
		httpState->flags.http11 = 1;
	//other: do nothing
	debug(150, 2)("mod_store_url_rewrite: to origin http version: HTTP/1.%d\n", httpState->flags.http11);

	return 0;
}

static int modifyHttpVersionToClient(clientHttpRequest *http)
{
	int fd = http->conn->fd;
	struct mod_conf_param *param = cc_get_mod_param(fd, mod);

	if (param->type != http_version)
		return 1;
	http_version_t *http_ver = &http->request->http_ver;
	if (1 == http_ver->major && 0 == http_ver->minor)
		httpBuildVersion(&http->reply->sline.version, 1, 0);
	else if (1 == http_ver->major && 1 == http_ver->minor)
		httpBuildVersion(&http->reply->sline.version, 1, 1);
	//other, do nothing
	debug(150, 2)("mod_store_url_rewrite: to client http version: HTTP/%d.%d\n", http->reply->sline.version.major, http->reply->sline.version.minor);

	return 0;
}

static int func_internalStoreUrlRewriteImplement(clientHttpRequest *http)
{
	char *newuri = http->request->store_url;
	if (NULL == newuri)
		newuri = http->request->canonical;
	assert(newuri);

	int fd = http->conn->fd;
	struct mod_conf_param *param = (struct mod_conf_param *) cc_get_mod_param(fd, mod);
	switch(param->type) {
		case HELPER:
			debug(150,2)("mod_store_url_rewrite: [%s] with helper\n", newuri);
			dealwith_helper(http, param);
			return 7;
		case ARG:
			debug(150,2)("mod_store_url_rewrite: [%s] with arguments\n", newuri);
			dealwith_arg(http, param);
			return 0;
		case ARG_REMOVE:
			debug(150,2)("mod_store_url_rewrite: [%s] with arguments remove\n", newuri);
			dealwith_arg_remove(http, param);
			return 0;
		case REG:
			debug(150,2)("mod_store_url_rewrite: [%s] with regular expression\n", newuri);
			dealwith_reg(http, param);
			return 0;
		case ignore_case:
			debug(150,2)("mod_store_url_rewrite: [%s] with ignore case\n", newuri);
			dealwith_ignore_case(http);
			return 0;
		case ignore_question:
			debug(150,2)("mod_store_url_rewrite: [%s] with ignore question\n", newuri);
			dealwith_ignore_question(http);
			return 0;
		case http_version:
			debug(150, 2)("mod_store_url_rewrite: [%s] with http version", newuri);
			dealwith_http_version(http);
			return 0;
		default:
			debug(150,2)("mod_store_url_rewrite: [%s] keep stay\n", newuri);
			return 0;
	}
}

static int hook_cleanup(cc_module *module)
{
	debug(150, 1)("mod_store_url_rewrite ->     hook_cleanup:\n");
	func_StoreUrlHelperShutdown(1);
	if (mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}


int mod_register(cc_module *module)
{
	debug(150, 1)("mod_store_url_rewrite init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_http_req_read_second = func_internalStoreUrlImplement;
	cc_register_hook_handler(HPIDX_hook_func_store_url_rewrite,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_store_url_rewrite),
			func_internalStoreUrlRewriteImplement);

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

	// module->hook_func_module_reconfigure = func_StoreUrlHelperShutdown
	cc_register_hook_handler(HPIDX_hook_func_module_reconfigure,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
			func_StoreUrlHelperShutdown);

	//modify http version for requesting to origin
	//added by xin.yao: 2012-11-15
	cc_register_hook_handler(HPIDX_hook_func_http_req_send,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send),
			modifyHttpVersionToOrigin);

	//keep consistent version number with client request line
	//added by xin.yao: 2012-11-15
	cc_register_hook_handler(HPIDX_hook_func_private_process_send_header,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_process_send_header),
			modifyHttpVersionToClient);

	//mod = module;
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif

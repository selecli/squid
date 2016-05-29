#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;

struct mod_conf_param {
	int ingoreCase;
	regex_t reg;
	char** replace;
};

static MemPool *  mod_config_pool = NULL;

#define REPLACE_REGEX_SUBSTR_NUMBER 99

static regmatch_t g_pmatch[REPLACE_REGEX_SUBSTR_NUMBER+1];

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_url_rewrite config_struct", sizeof(struct mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* param)
{
	struct mod_conf_param* reg = (struct mod_conf_param*) param;
	int i;
	regfree(&reg->reg);
	for(i=0;;i+=2)
	{
		if(reg->replace[i])
		{
			xfree(reg->replace[i]);
		}
		else
		{
			break;
		}
		if(!reg->replace[i+1])
		{
			break;
		}
	}
	memPoolFree(mod_config_pool, reg);
	reg = NULL;
	return 0;
}


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



// confige line example:
// mod_mod_conf_param regex_match regex_change allow/deny acl
static int func_sys_parse_param(char* cfg_line)
{
	struct mod_conf_param* reg = mod_config_pool_alloc();

	char* token = NULL;

	//mod_mod_conf_param
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{       
		if(strcmp(token, "mod_url_rewrite"))
			return -1;       
	}       
	else    
	{       
		debug(120, 3)("(mod_url_rewrite) ->  parse line error\n");
		return -1;       
	}       


	if(NULL == (token=strtok(NULL, w_space)))
	{       
		debug(120, 0) ("%s line %d: %s\n", cfg_filename, config_lineno, config_input_line);
		debug(120, 0) ("parse_url_rewrite: missing match string.\n");
		memPoolFree(mod_config_pool, reg);
		reg = NULL;
		return -1; 
	}       

	if (!strcmp(token, "ingoreCase"))
	{
		reg->ingoreCase = 1;
		if (NULL == (token = strtok(NULL, w_space)))
		{
			debug(120, 0) ("processing ingoreCase: %s line %d: %s\n", cfg_filename, config_lineno, config_input_line);
			debug(120, 0) ("parse_url_rewrite: missing match string.\n");
			memPoolFree(mod_config_pool, reg);
			reg = NULL;
			return -1; 
		}
	}
	else 
		reg->ingoreCase = 0;

	int number = analysisRegexMatch(token, 1, reg);
	if(number < 0)
	{       
		debug(120, 0)("match string error\n");
		memPoolFree(mod_config_pool, reg);
		reg = NULL;
		return -1;
	}

	if(NULL == (token=strtok(NULL, w_space)))
	{
		debug(120, 0) ("%s line %d: %s\n", cfg_filename, config_lineno,
				config_input_line);
		debug(120, 0) ("parse_mod_conf_param: missing replace string.\n");
		regfree(&reg->reg);
		memPoolFree(mod_config_pool, reg);
		reg = NULL;
		return -1;
	}
	int number2 = analysisRegexReplace(token, reg);
	if(number2 < 0)
	{
		debug(120, 0) ("%s line %d: %s\n", cfg_filename, config_lineno,
				config_input_line);
		debug(120, 0)("replace string error\n");
		regfree(&reg->reg);
		memPoolFree(mod_config_pool, reg);
		reg = NULL;
		return -1;
	}
	if(number2 > number)
	{
		debug(120, 0) ("%s line %d: %s\n", cfg_filename, config_lineno,
				config_input_line);
		debug(120, 0)("replace string number is too large\n");
		regfree(&reg->reg);
		int i;
		for(i=0;;i+=2)
		{
			if(reg->replace[i])
			{
				xfree(reg->replace[i]);
			}
			else
			{
				break;
			}
			if(!reg->replace[i+1])
			{
				break;
			}
		}
		memPoolFree(mod_config_pool, reg);
		reg = NULL;
		return -1;
	}

	cc_register_mod_param(mod, reg, free_callback);

	return 0;	

}

static int changeCase(char *url, char *dstUrl)
{
	if (!url)
		return 0;
	char *protocol= NULL;
	char *domainEnd = NULL;
	int len = 0;
	int i;
	
	memset(dstUrl, '\0', BUFSIZ);
	memcpy(dstUrl, url, strlen(url));
	if (NULL == (protocol = strstr(dstUrl, "://")))
	{
		return 0;
	}
	if (NULL == (domainEnd = strchr(protocol + 3, '/')))
	{
		return 0;
	}
	len = strlen(domainEnd);
	for (i = 0; i < len; i++)
	{
		if(*(domainEnd + i) >= 'A' && *(domainEnd + i) <= 'Z')
			*(domainEnd + i) = *(domainEnd + i) + 32;
	}
	return 1;
}

static inline int url_replace(clientHttpRequest* http, struct mod_conf_param* param)
{
	char tmpUrl[BUFSIZ];
	int ret = 0;
	ret = regexec(&param->reg, http->uri, REPLACE_REGEX_SUBSTR_NUMBER+1, g_pmatch, 0);
	if(0 != ret) 
	{
		debug(120, 3)("url_replace: donot match regex, uri=[%s]\n", http->uri);
		return 0;
	}       
	String url = StringNull;
	stringAppend(&url, param->replace[0], strlen(param->replace[0]));
	int i = 1;
	while(0 != (ret=(long int)param->replace[i]))
	{       
		stringAppend(&url, http->uri+g_pmatch[ret].rm_so, g_pmatch[ret].rm_eo-g_pmatch[ret].rm_so);
		debug(120,3)("rm_so=%d, rm_eo=%d\n", g_pmatch[ret].rm_so, g_pmatch[ret].rm_eo);
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
	xfree(http->uri);       
	if (param->ingoreCase)
	{
		if (changeCase(url.buf, tmpUrl))
			http->uri = xstrdup(tmpUrl);
		else
			http->uri = xstrdup(url.buf);
	}
	else
		http->uri = xstrdup(url.buf);
	stringClean(&url);
	debug(120, 3)("url_replace: uri modified, uri=[%s]\n", http->uri);
	return 1;
}




static inline void modify_request(clientHttpRequest * http)
{
	debug(120, 3)("modify_request: start, uri=[%s]\n", http->uri);
	request_t* old_request = http->request;
	request_t* new_request = urlParse(old_request->method, http->uri);
	//safe_free(http->uri);

	if (new_request)
	{
		safe_free(http->uri);
		http->uri = xstrdup(urlCanonical(new_request));
		safe_free(http->log_uri);
		if(!http->log_uri)
			http->log_uri = xstrdup(urlCanonicalClean(old_request));
		new_request->http_ver = old_request->http_ver;
		httpHeaderAppend(&new_request->header, &old_request->header);
		new_request->client_addr = old_request->client_addr;
		new_request->client_port = old_request->client_port;
#if FOLLOW_X_FORWARDED_FOR
		new_request->indirect_client_addr = old_request->indirect_client_addr;
#endif /* FOLLOW_X_FORWARDED_FOR */
		new_request->my_addr = old_request->my_addr;
		new_request->my_port = old_request->my_port;
		new_request->flags = old_request->flags;
		new_request->flags.redirected = 1;
		if (old_request->auth_user_request)
		{
			new_request->auth_user_request = old_request->auth_user_request;
			authenticateAuthUserRequestLock(new_request->auth_user_request);
		}
		if (old_request->body_reader)
		{
			new_request->body_reader = old_request->body_reader;
			new_request->body_reader_data = old_request->body_reader_data;
			old_request->body_reader = NULL;
			old_request->body_reader_data = NULL;
		}
		new_request->content_length = old_request->content_length;
		if (strBuf(old_request->extacl_log))
			new_request->extacl_log = stringDup(&old_request->extacl_log);
		if (old_request->extacl_user)
			new_request->extacl_user = xstrdup(old_request->extacl_user);
		if (old_request->extacl_passwd)
			new_request->extacl_passwd = xstrdup(old_request->extacl_passwd);
		if(old_request->cc_request_private_data)
		{
			new_request->cc_request_private_data = old_request->cc_request_private_data;
			old_request->cc_request_private_data = NULL;
		}
		if(old_request->store_url)
			new_request->store_url = xstrdup(old_request->store_url);
		requestUnlink(old_request);
		http->request = requestLink(new_request);
	}
	else
	{
		debug(120,1)("mod_url_rewrite can not parse url [%s] into request\n", http->uri);

		//recover if it is an illegal uri
		safe_free(http->uri);
		http->uri = xstrdup(urlCanonical(old_request));
	}
}




int func_http_befoee_redirect(clientHttpRequest *http)
{

	int fd = http->conn->fd;

	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);

	if(url_replace(http, param))
		modify_request(http);

	return 1;
}

static int hook_cleanup(cc_module *module)
{
	debug(120, 1)("(mod_url_rewrite) -> hook_cleanup:\n");
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
	cc_register_hook_handler(HPIDX_hook_func_http_before_redirect,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
			func_http_befoee_redirect);

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

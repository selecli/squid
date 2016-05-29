#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;
extern void clientProcessRequest(clientHttpRequest *);
extern CWCB clientWriteComplete;
static MemPool * memobject_private_data_pool;

struct mod_conf_param {
	regex_t reg;
	char** replace;
};
typedef struct _redirect_pd {
    int fd;
} redirect_pd;
#define REPLACE_REGEX_SUBSTR_NUMBER 99

//add by zh for mod_hotspot
static char *name = "CC-PRIVATE";
//static char *value = "0";

static int has_private_header(HttpReply *rep)
{
	HttpHeaderEntry *myheader;
	HttpHeaderPos pos = HttpHeaderInitPos;

	while ((myheader = httpHeaderGetEntry(&rep->header, &pos)))
	{
		if (myheader->id == HDR_OTHER && strCaseCmp(myheader->name, name) == 0)
			return 1;
	}

	return 0;
}
//add end

static regmatch_t g_pmatch[REPLACE_REGEX_SUBSTR_NUMBER+1];

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
	xfree(reg);
	return 0;
}

static int free_memobject_private_data(void *data)
{
	redirect_pd *pd = data;
	memPoolFree(memobject_private_data_pool, pd);
	return 0;
}
static inline int modify_request(clientHttpRequest * http)
{
	debug(97, 3)("modify_request: start, uri=[%s]\n", http->uri);
	request_t* old_request = http->request;
	request_t* new_request = urlParse(old_request->method, http->uri);

	if (new_request)
	{
		safe_free(http->uri);
		http->uri = xstrdup(urlCanonical(new_request));
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
		{
			new_request->store_url = old_request->store_url;
			old_request->store_url = NULL;
		}
		requestUnlink(old_request);
		http->request = requestLink(new_request);
		return 1;
	}
	else
	{
		debug(153,1)("mod_redirect_location: urlparse error\n");
		return 0;

	}
}

static inline int storeurl_replace(clientHttpRequest* http, struct mod_conf_param* param)
{
	if (NULL == param)
	{
		debug(153,3)("storeurl_replace : cc_get_mod_param is NULL!!\n");
		return 0;
	}
	int ret = 0;
	ret = regexec(&param->reg, http->uri, REPLACE_REGEX_SUBSTR_NUMBER+1, g_pmatch, 0);
	if(0 != ret) 
	{
		debug(153, 3)("storeurl_replace: donot match regex, uri=[%s]\n", http->uri);
		return 0;
	}       
	String url = StringNull;
	stringAppend(&url, param->replace[0], strlen(param->replace[0]));
	int i = 1;
	while(0 != (ret=(long int)param->replace[i]))
	{       
		stringAppend(&url, http->uri+g_pmatch[ret].rm_so, g_pmatch[ret].rm_eo-g_pmatch[ret].rm_so);
		debug(153,3)("rm_so=%d, rm_eo=%d\n", g_pmatch[ret].rm_so, g_pmatch[ret].rm_eo);
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
	xfree(http->request->store_url);       
	http->request->store_url = xstrdup(url.buf);
	stringClean(&url);
	debug(153, 3)("storeurl_replace: store_url modified, storeurl=[%s]\n", http->request->store_url);
	return 1;
	
}


static int createNewRequest(char* result,clientHttpRequest *http)
{
	char *p_question = NULL;
	int fd = http->conn->fd;
	char *url = xstrdup(urlCanonical(http->request));
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
//	assert(param != NULL);
	debug(153,5)("mod_redirect_location ->http->request->store_url %s canonical =%s\n,",http->request->store_url,http->request->canonical);
	debug(153,5)("mod_redirect_locaiton ->http->request->canonical [%s][%s]\n",url,result);
	if(url == NULL)
	{
		debug(153,1)("mod_redirect_location: canonical url is NULL!!!new request canceled\n");
		return 0;
	}
	else if( !storeurl_replace(http, param))
	{
		p_question = strchr(url,'?');
		if(p_question)
		{
			*p_question = 0;
			debug(153,5)("mod_redirect_location  new_store_url [%s]\n",url);
		}
		safe_free(http->request->store_url);
		http->request->store_url = url;
	}
	else
	{
	}

	assert(result != NULL);
	safe_free(http->uri);
	http->uri = xstrdup(result);
	if ( modify_request(http) == 0 )//parse http->uri error
		return 0;
	
	/*fix case 3248: file cannot Hit*/ 
	HttpHeader *req_hdr = &http->request->header;
	request_t* request = http->request;
	if (request->method == METHOD_GET)
	{
		request->range = httpHeaderGetRange(req_hdr);
		if (request->range)
		{
			request->flags.range = 1;
		}
	}
	return 1;
}

static int client_before_reply_send_end(clientHttpRequest *http)
{
	HttpReply *rep = http->reply;
	debug(153, 3)("mod_redirect_location ->  client_before_reply_send_end %d\n,",rep->sline.status);
	debug(153, 3)("mod_redirect_location ->  client_before_reply_send_end http->uri %s\n,",http->uri);
	if(rep->sline.status == HTTP_MOVED_TEMPORARILY || rep->sline.status ==HTTP_MOVED_PERMANENTLY )
	{
		//add by zh
		if(has_private_header(rep))
		{
			debug(153, 3)("mod_redirect_location -> http has private header...\n"); 
			return 0;
		}
		//add end

		const char *location = httpHeaderGetStr(&rep->header, HDR_LOCATION);
		if(location == NULL)
		{
			debug(153,3)("mod_redirect_location-> have no location ,return\n");
			return 0;
		}

		char * new_url = xstrdup(location);

		StoreEntry* e = http->entry;
		store_client *sc = http->sc;
		storeClientUnregister(sc, e, http);
		storeRelease(e);
		storeUnlockObject(e);
		http->entry = NULL;
		http->sc = NULL;
		if(http->reply)
		{
			request_t* request = http->request;			
			if (httpReplyBodySize(request->method, rep) < 0)
			{
				if (http->conn->port->http11 && (request->http_ver.major > 1 || (request->http_ver.major == 1 && request->http_ver.minor >= 1)))
				{
					debug(153,3)("mod_redirect_location ->  destroying 'Transfer-Encoding: chunked'\n");
					request->flags.chunked_response = 0;
				}
			}
			httpReplyDestroy(http->reply);
			http->reply = NULL;
		}
		debug(153,2)("mod_redirect_location->: client_process_send_header repl->sline.status  == %d, and new_url is (%s)\n",rep->sline.status,new_url);


		if (createNewRequest(new_url,http) == 0 )
		{
			safe_free(new_url);
            ErrorState *err = errorCon(ERR_INVALID_URL, HTTP_BAD_REQUEST, http->orig_request);
            http->sc = NULL;
            http->log_type = LOG_TCP_DENIED;
            http->entry = clientCreateStoreEntry(http, http->request->method,
                    null_request_flags);
            errorAppendEntry(http->entry, err);
			return 1;
		}
		else
		{
			safe_free(new_url);
			clientProcessRequest(http);
			return 1;
		}
	}
	else
	{
		//nothing to do
	}

	return 0;
}

/*          
 * return value:
 * 0 should send header
 * 1 should not send header, and response is 200 or 206
 * 2 should not send header, and response is not 200 or 206
 */         
static int clientBeforeSendHeader(clientHttpRequest *http,MemBuf *mb)
{
	debug(153,5)("mod_redirect_location : client_before_send_header: uri == %s http->reply->sline.status =[%d]\n",http->uri,http->reply->sline.status);
	if(http->reply->sline.status == HTTP_OK || http->reply->sline.status == HTTP_PARTIAL_CONTENT)
	{	
		return 0;
	}
	else if(http->reply->sline.status == HTTP_MOVED_PERMANENTLY || http->reply->sline.status == HTTP_MOVED_TEMPORARILY)
	{   
		//add by zh
                if(has_private_header(http->reply))
                {
                        debug(153, 3)("mod_redirect_location -> http has private header...\n");
                        return 0;
                }
                //add end
        HttpReply *rep = http->reply;
		const char *location = httpHeaderGetStr(&rep->header, HDR_LOCATION);
        if (strncmp(location,"http://",strlen("http://")) != 0) 
        {
			debug(153,3)("mod_redirect_location-> location[%s] is not standard ,fc should give up follow it!\n",location);
            return 0;
        }

		clientWriteComplete(http->conn->fd, NULL,0,COMM_OK,http);
		return 2;
	}   
	else if (http->reply->sline.status >=HTTP_BAD_REQUEST || http->reply->sline.status < HTTP_OK)
	{
		return 0;
	}

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

static int func_sys_parse_param(char* cfg_line)
{
	struct mod_conf_param* reg = (struct mod_conf_param*)xcalloc(1, sizeof(struct mod_conf_param));

	char* token = NULL;

	//mod_mod_conf_param
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{       
		if(strcmp(token, "mod_redirect_location"))
			return -1;       
	}       
	else    
	{       
		debug(153, 3)("(mod_redirect_location) ->  parse line error\n");
		return -1;       
	}       


	if(NULL == (token=strtok(NULL, w_space)))
	{       
		debug(153, 0) ("%s line %d: %s\n", cfg_filename, config_lineno,
				config_input_line);
		debug(153, 0) ("parse_redirect_location: missing match string.\n");
		debug(153, 0) ("parse_redirect_location: use default.\n");
		xfree(reg);
		return -1; 
	}       

	int number = analysisRegexMatch(token, 1, reg);
	if(number < 0)
	{       
		debug(153, 0)("match string error\n");
		xfree(reg);
		return -1;
	}

	if(NULL == (token=strtok(NULL, w_space)))
	{
		debug(153, 0) ("%s line %d: %s\n", cfg_filename, config_lineno,
				config_input_line);
		debug(153, 0) ("parse_mod_conf_param: missing replace string.\n");
		regfree(&reg->reg);
		xfree(reg);
		return -1;
	}
	int number2 = analysisRegexReplace(token, reg);
	if(number2 < 0)
	{
		debug(153, 0) ("%s line %d: %s\n", cfg_filename, config_lineno,
				config_input_line);
		debug(153, 0)("replace string error\n");
		regfree(&reg->reg);
		xfree(reg);
		return -1;
	}
	if(number2 > number)
	{
		debug(153, 0) ("%s line %d: %s\n", cfg_filename, config_lineno,
				config_input_line);
		debug(153, 0)("replace string number is too large\n");
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
		xfree(reg);
		return -1;
	}

	cc_register_mod_param(mod, reg, free_callback);

	return 0;	

}
static int client_cache_hit_for_redirect_location(clientHttpRequest *http)
{
    debug(153, 3)("mod_redirect_location -> enter client_cache_hit_for_redirect_location!\n");
    return 0;
}

static void store_meta_url_for_redirect_location(MemObject *mem,int *pass)
{
    assert(mem);
    redirect_pd *pd = cc_get_mod_private_data(MEMOBJECT_PRIVATE_DATA, mem, mod);
    
    if (!pd)
        return;

    int fd = pd->fd;
    if (NULL != fd_table[fd].cc_run_state && fd_table[fd].cc_run_state[mod->slot] > 0) {
        *pass = 0;
    }
    debug(153, 3)("mod_redirect_location -> enter store_meta_url_for_redirect_location pass=%d\n",*pass);
}

static int registe_fd_to_memobj(StoreEntry* e,clientHttpRequest* http)
{
    MemObject *mem = http->entry->mem_obj;
    assert(mem);

	if(!memobject_private_data_pool)
	{
		memobject_private_data_pool = memPoolCreate(
				"mod_redirect_location private_data memobject_private_data", 
				sizeof(redirect_pd));
	}

    redirect_pd *pd = cc_get_mod_private_data(MEMOBJECT_PRIVATE_DATA, mem, mod);
    if (!pd) {
        pd = memPoolAlloc(memobject_private_data_pool);
		cc_register_mod_private_data(MEMOBJECT_PRIVATE_DATA, mem, pd,free_memobject_private_data, mod);
    }
    pd->fd = http->conn->fd;
    debug(153, 3)("mod_redirect_location -> enter registe_fd_to_memobj fd = %d\n",pd->fd);
    return 0;
}

int mod_register(cc_module *module)
{
	debug(153, 1)("mod_redirect_location ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_private_http_repl_send_end,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_repl_send_end),
			client_before_reply_send_end);

	cc_register_hook_handler(HPIDX_hook_func_private_before_send_header,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_before_send_header), 
			clientBeforeSendHeader);

	cc_register_hook_handler(HPIDX_hook_func_private_client_cache_hit_for_redirect_location,
	        module->slot, 
	        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_cache_hit_for_redirect_location),
	        client_cache_hit_for_redirect_location);

	cc_register_hook_handler(HPIDX_hook_func_private_store_meta_url_for_redirect_location,
	        module->slot, 
	        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_store_meta_url_for_redirect_location),
            store_meta_url_for_redirect_location);

    cc_register_hook_handler(HPIDX_hook_func_private_sc_unregister,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_sc_unregister),
            registe_fd_to_memobj);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif


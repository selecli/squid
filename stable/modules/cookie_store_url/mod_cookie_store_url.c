#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

struct mod_conf_param {
	char *cookie_name; 
	int  cookie_length;
	char *conf_cookie_value;
	int  cookie_value_len;
};

static int free_callback(void* param)
{
	struct mod_conf_param* data = (struct mod_conf_param*) param;
	if(data){
		safe_free(data->cookie_name);
		safe_free(data->conf_cookie_value);
		safe_free(data);
	}
	return 0;
}

static HttpHeaderEntry* httpHeaderFindEntry2(const HttpHeader * hdr, http_hdr_type id)
{
        HttpHeaderPos pos = HttpHeaderInitPos;
        HttpHeaderEntry *e;
        assert(hdr);

        /* check mask first */
        if (!CBIT_TEST(hdr->mask, id))
                return NULL;
        /* looks like we must have it, do linear search */
        while ((e = httpHeaderGetEntry(hdr, &pos))) {
                if (e->id == id)
                        return e;
        }
        /* hm.. we thought it was there, but it was not found */
        assert(0);
        return NULL;        /* not reached */
}

static const char * httpHeaderGetStr2(const HttpHeader * hdr, http_hdr_type id)
{
        HttpHeaderEntry *e;
        if ((e = httpHeaderFindEntry2(hdr, id)))
        {
                return strBuf(e->value);
        }
        return NULL;
}

// confige line example:
//mod_cookie_store_url current_city= lanxun allow acl
//mod_cookie_store_url current_city= allow acl
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data=NULL;
	char* token = NULL;

//mod_http_protocl_modify
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_cookie_store_url"))
			goto err;	
	}
	else
	{
		debug(145, 3)("(mod_cookie_store_url) ->  parse line error\n");
		goto err;	
	}

//http_version
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(145, 3)("(mod_cookie_store_url) ->  parse line data does not existed\n");
		goto err;
	}
	if (!(strcmp(token, "allow") && strcmp(token, "deny")))
	{
		debug(145, 3)("(mod_cookie_store_url) ->  missing cookie name setting\n");
		goto err;
	}
	else{
			
		data = xmalloc(sizeof(struct mod_conf_param));
		data->cookie_length = strlen(token);
		data->cookie_name   = xmalloc(data->cookie_length + 1);
		strcpy(data->cookie_name,token);
	}

//configure a specified cookie value
	if (NULL == (token = strtok(NULL, w_space)))
        {   
                debug(145, 3)("(mod_cookie_store_url) ->  parse line data does not existed\n");
                goto err;
        } else {
		if (!(strcmp(token, "allow") && strcmp(token, "deny"))) {
			data->conf_cookie_value = NULL;
		} else {
			data->cookie_value_len = strlen(token);
			data->conf_cookie_value   = xmalloc(data->cookie_value_len + 1);
			strcpy(data->conf_cookie_value, token);
		}
	} 

	cc_register_mod_param(mod, data, free_callback);

	return 0;		
err:
	//free_callback(data);
	return -1;
}

static int modify_store_url_for_cookie(clientHttpRequest *http)
{

	int fd = http->conn->fd;

	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	if(param == NULL){
		return 0;
	}

	request_t* request = http->request;
	const char* cookie = httpHeaderGetStr2(&request->header, HDR_COOKIE);

	if(cookie == NULL)
	{
		return 0;
	}
	debug(145, 5) ("mod_cookie_store_url cookie value: %s \n", cookie);

	char *str = NULL;
	char *endstr = NULL;
	char *cookie_value = NULL;
	int  cookie_len = 0;
	str = strstr(cookie,param->cookie_name);
	if(str != NULL){

		//debug(145, 5) ("mod_cookie_store_url cookie value: %s \n", str);
		endstr = strstr(str,";");
		if(endstr != NULL){
			cookie_len = endstr - str;
		}
		else{
			cookie_len = strlen(str);	
		}
		cookie_len = cookie_len - param->cookie_length;
		cookie_value = xmalloc(cookie_len + 1);
		memset(cookie_value,0,cookie_len + 1);

		strncpy(cookie_value,str+param->cookie_length,cookie_len);

		debug(145, 5) ("mod_cookie_store_url cookie value: %s \n", cookie_value);
	}

	if(cookie_value != NULL){
		safe_free(http->request->store_url);
		int store_lenth = strlen(http->uri) + cookie_len + 1;
		if (param->conf_cookie_value != NULL) {
			store_lenth += param->cookie_value_len;
		}
		http->request->store_url = xmalloc(store_lenth);
		memset(http->request->store_url,0,store_lenth);

		strcpy(http->request->store_url, http->uri);
		strcat(http->request->store_url, cookie_value);
		if (param->conf_cookie_value != NULL) {
			strcat(http->request->store_url, param->conf_cookie_value);
		}
		debug(145, 3)("mod_cookie_store_url :store_url=%s \n", http->request->store_url);
	}

	safe_free(cookie_value);
	return 0;
}


/* module init */
int mod_register(cc_module *module)
{
	debug(145, 1)("(mod_cookie_store_url) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_http_req_read_second = add_range;
	cc_register_hook_handler(HPIDX_hook_func_http_req_read_second,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read_second),
			modify_store_url_for_cookie);

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	mod = module;
	return 0;
}

#endif

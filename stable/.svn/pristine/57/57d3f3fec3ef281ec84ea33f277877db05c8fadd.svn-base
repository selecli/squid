#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;
#define SECOND_PART 2

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
        return NULL; /* not reached */
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
//mod_elong_cookie current_city= lanxun allow acl
//mod_elong_cookie current_city= allow acl
static int func_sys_parse_param(char *cfg_line)
{
	struct mod_conf_param *data=NULL;
	char* token = NULL;

	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_elong_cookie"))
			goto err;	
	}
	else
	{
		debug(202, 3)("(mod_elong_cookie) ->  parse line error\n");
		goto err;	
	}

//http_version
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(202, 3)("(mod_elong_cookie) ->  parse line data does not existed\n");
		goto err;
	}
	if (!(strcmp(token, "allow") && strcmp(token, "deny")))
	{
		debug(202, 3)("(mod_elong_cookie) ->  missing cookie name setting\n");
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
                debug(202, 3)("(mod_elong_cookie) ->  parse line data does not existed\n");
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

static int httpSetNewStoreURL(clientHttpRequest *http, const char *cookie_value)
{
	int fd = http->conn->fd;
	struct mod_conf_param *param = (struct mod_conf_param *)cc_get_mod_param(fd, mod);
	if(param == NULL){
		return 0;
	}

	int store_length = 0,cookie_len = 1 ;
	if( http->request->store_url == NULL)
       		store_length = strlen(http->uri) + cookie_len + 1;
	else
		store_length = strlen(http->request->store_url) + cookie_len + 1;

	if (param->conf_cookie_value != NULL) {
		store_length += param->cookie_value_len;
	}

	safe_free(http->request->store_url);
	http->request->store_url = xmalloc(store_length);

	memset(http->request->store_url,0,store_length);
	strcpy(http->request->store_url, http->uri);
	if (param->conf_cookie_value != NULL) {
		strcat(http->request->store_url, param->conf_cookie_value);
	}
	strcat(http->request->store_url, cookie_value);
	debug(202, 3)("mod_elong_cookie: httpSetNewStoreURL store_url=%s \n", http->request->store_url);

	return 1;
}

static int httpSetStoreURLFromReferer(clientHttpRequest *http)
{
	http->request->flags.cachable = 0; //make no cache`
	/*
	request_t* request = http->request;
	const char* referer = httpHeaderGetStr2(&request->header, HDR_REFERER);
	debug(202,5)("mod_elong_cookie: get referer %s\n",referer);
	if(referer == NULL )
	{
		httpSetNewStoreURL (http,"1");
	}
	else
	{

		const char *referer_name[] = {"qq.trip.elong.com","qq.com","ecitic.com"};
		if (strstr(referer,referer_name[0]))
			httpSetNewStoreURL (http,"2");
		else if (strstr(referer,referer_name[1]))
			httpSetNewStoreURL (http,"2");
		else if (strstr(referer,referer_name[2]))
			httpSetNewStoreURL (http,"3");
		else
			httpSetNewStoreURL (http,"1");
	}
	*/

	return 1;

}

static int my_strcasecmp(char *str , char *dest) 
{
	if(!dest || !str)
		return 1;
	else
		return strcasecmp(str,dest);
}
static int httpGetCookieUID(clientHttpRequest *http) 
{
	const char *cookie_name[] = {"HOMEUID=","HOMEURL="};
	request_t* request = http->request;
	const char* cookie = httpHeaderGetStr2(&request->header, HDR_COOKIE);
	if(cookie == NULL)
		return 0;

	char *str = NULL;
	char *endstr = NULL;
	char *cookie_uid = NULL, *cookie_url = NULL;
	int  cookie_len = 0 , i = 0;
//	int need_force_no_cache = 0;
	for(;i<2;i++)
	{
		str = strstr(cookie,cookie_name[i]);
		if(str != NULL){
			endstr = strstr(str,";");
			if(endstr != NULL){
				cookie_len = endstr - str;
			}
			else{
				cookie_len = strlen(str);	
			}
			cookie_len = cookie_len - strlen(cookie_name[i]);
			if(i == 0 )
			{
				cookie_uid= xmalloc(cookie_len + 1);
				memset(cookie_uid,0,cookie_len + 1);
				strncpy(cookie_uid,str+strlen(cookie_name[i]),cookie_len);
			}
			else if (i == 1)
			{
				cookie_url = xmalloc(cookie_len + 1);
				memset(cookie_url ,0,cookie_len + 1);
				strncpy(cookie_url ,str+strlen(cookie_name[i]),cookie_len);
			}

		}
	}
		
	debug(202, 5) ("mod_elong_cookie cookie value: %s \n", cookie_uid);
	debug(202, 5) ("mod_elong_cookie cookie value: %s \n", cookie_url);

	char *url_second = NULL;
	const char *key = "://";
       	char *pstr = NULL;
	int key_len = 3, url_sec_len = 0;
	
	str = strstr(http->uri,key);
	if(str != NULL){
		str += key_len;
		if(str == NULL)
		{
			debug(202,5)("mod_elong_cookie: url scecond part get error!\n");
			goto match;
		}
		assert(str);
		for(i =0 ; i< SECOND_PART; i++)
		{
			endstr = strchr(str,'/');
			debug(202,1)("mod_elong_cookie: url second part[%s]\n",str);
			if((endstr != NULL) && (endstr+1 != NULL ))
			{
				pstr = str;
				if(endstr != NULL){
					url_sec_len = endstr - str;
				}
				else{
					url_sec_len = strlen(str);	
				}
				str = endstr+1;  
		
			}
			else
			{
				debug(202,1)("mod_elong_cookie: url second part get error!!\n");
				goto match;
			}
		}

		url_second = xmalloc(url_sec_len + 1);
		memset(url_second ,0,url_sec_len + 1);
		strncpy(url_second ,pstr,url_sec_len);
	}
	debug(202, 9) ("mod_elong_cookie url_second: %s \n", url_second);

	if(!my_strcasecmp(url_second, cookie_uid) || !my_strcasecmp(url_second,cookie_url))
	{
		debug(202, 5) ("mod_elong_cookie make no cache cause url_second[%s] == cookie[%s]||cookie[%s] \n", url_second,cookie_uid,cookie_url);

		return 1;
	}
match:	
	safe_free(url_second);
	safe_free(cookie_uid);
	safe_free(cookie_url);

	return 0;
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
	char *end_with = NULL;
	if( param->cookie_name && param->cookie_length)
		end_with = param->cookie_name + param->cookie_length -1;

	if(strncmp(end_with,"=",1))
	{//Mode:2
		debug(202,5)("mod_elong_cookie, enter mode2\n");
		if(httpGetCookieUID(http))
			http->request->flags.cachable = 0; //make no cache`
	
		return 0;
	}
	

	if(cookie == NULL)
	{
		debug(202,5)("mod_elong_cookie find no cookie , httpSetStoreURLFromReferer \n");
		httpSetStoreURLFromReferer(http);
		return 0;
	}
	debug(202, 5) ("mod_elong_cookie cookie value: %s \n", cookie);

	char *str = NULL;
	char *endstr = NULL;
	char *cookie_value = NULL;
	int  cookie_len = 0;
//	int need_force_no_cache = 0;
	str = strstr(cookie,param->cookie_name);
	if(str != NULL){
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

		debug(202, 5) ("mod_elong_cookie cookie value: %s \n", cookie_value);
	}
	else //with cookie but without special name
	{
		http->request->flags.cachable = 0; //make no cache` /
		debug(202,5)("mod_elong_cookie: with cookie but without [%s]\n",param->cookie_name);
		return 0;
	}
	
	char *endptr;
	int base = 10;
	long cookie_num;

	if(cookie_value == NULL )
	{
		debug(202,1)("mod_elong_cookie: cookie ptr null \n");
		cookie_num = 0;
	}
	else
	{
		cookie_num = strtol(cookie_value, &endptr, base); 
		if ((errno == ERANGE && (cookie_num == LONG_MAX || cookie_num == LONG_MIN)) || (errno != 0 && cookie_num == 0)) {  
			debug(202,1)("mod_elong_cookie: cookie num error[%d]\n",errno);
			cookie_num = 0;
		}
	}
	
	debug(202,5)("mod_elong_cookie: cookie_num[%ld]\n",cookie_num);
	
	if ( cookie_num == 1 || cookie_num == 2 || cookie_num == 3 )
	{
		httpSetNewStoreURL(http, cookie_value);
		debug(202, 3)("mod_elong_cookie :store_url=%s \n", http->request->store_url);
	}
	else
	{
		httpSetStoreURLFromReferer(http);
	}

//	if(need_force_no_cache == 1)
//	{
//		debug(202,5)("mod_elong_cookie: make no cache\n");
//		http->request->flags.cachable = 0; //make no cache`
//	}

	safe_free(cookie_value);
	return 0;
}
/* module init */
int mod_register(cc_module *module)
{
	debug(202, 1)("(mod_elong_cookie) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_http_req_read_second = add_range;
//	cc_register_hook_handler(HPIDX_hook_func_http_req_read_second,
//			module->slot,
//			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read_second),
//			modify_store_url_for_cookie);
	cc_register_hook_handler(HPIDX_hook_func_before_clientProcessRequest2,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_clientProcessRequest2),
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

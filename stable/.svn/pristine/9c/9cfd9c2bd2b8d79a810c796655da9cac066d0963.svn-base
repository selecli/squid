#include "cc_framework_api.h"
#include <stdbool.h>

#ifdef CC_FRAMEWORK

static cc_module *mod = NULL;

typedef enum{
	STANDARD,
	YOUKU,
	TUDOU,
	KU6,
	QQ,
	SOHU,
	SINA
}store_type;



struct mod_config
{
	char	start[128];
	char	end[128];
	store_type website;
};

typedef enum{
	NONE,
	HIT_FLV,
} media_type;


typedef struct _request_param
{
	int flag;
	int start;
	int end;
	media_type is_real_flv;
} request_param;


static void modify_request(clientHttpRequest * http,char *new_uri);
static void drop_startend(char* start, char* end, char* url);

static int free_cfg(void *data)
{
	safe_free(data);
	return 0;
}

static int free_request_param(void* data)
{
	request_param *req_param = (request_param *) data;
	if(req_param != NULL){
		safe_free(req_param);
		req_param = NULL;
	}
	return 0;
}

static int hook_init()
{
	debug(137, 1)("(mod_f4v_byte_13_miss_wait) ->	hook_init:----:)\n");
	return 0;
}


static int hook_cleanup(cc_module *module)
{
	debug(137, 1)("(mod_f4v_byte_13_miss_wait) ->	hook_cleanup:\n");
	return 0;
}


/* 去掉url参数中的start和end，但保留其它的 */
static void drop_startend(char* start, char* end, char* url)
{
	char *s;
	char *e;

	//去掉start
	s = strstr(url, start);
	if(s)
	{
		e = strstr(s, "&");
		if( e == 0 )
			*(s-1) = 0;
		else {
			memmove(s, e+1, strlen(e+1));
			*(s+strlen(e+1)) = 0;
		}
	}

	//去掉end，如果有的话
	s = strstr(url, end);
	if( s ) {
		e = strstr(s, "&");
		if( e == 0 )
			*(s-1) = 0;
		else {
			memmove(s, e+1, strlen(e+1));
			*(s+strlen(e+1)) = 0;
		}
	}

}

//standard safeguard chain: store_url = the part of url which is before "?"
static char *store_key_url(char* url){
	
 	char * uri = xstrdup(url);
        char *tmpuri = strstr(uri,"?");
        if(tmpuri != NULL)
                *tmpuri = 0;

	debug(137, 3)("(mod_f4v_byte_13_miss_wait) -> store_url is %s \n", uri);
	return uri;
}

//youku safeguard chain: 1 remove ip 2 remove random directory 
static char *store_youku_key_url(char* url){

        char new_url[1024];
        memset(new_url,0,1024);

        char* str1 = strstr(url, "://");
        if(NULL == str1)
        {
                return NULL;
        }

        str1 += 3;

        memcpy(new_url, url, str1-url);

        char* str2 = strchr(str1, '/');


        if(NULL == str2)
        {
                return NULL;
        }

        //memcpy(new_url, url, str2-url);

        str2 += 1;
        char* str4 = strchr(str2, '/');
        if(NULL == str4)
        {
                return NULL;
        }

        strncat(new_url, str2, str4-str2);

        str4 += 1;

        char* str3 = strchr(str4, '/');
 	if(NULL == str3)
        {
                return NULL;
        }

        strcat(new_url,str3);

        debug(137, 3)("store_url is %s \n", new_url);
        return xstrdup(new_url);

}

//tudou safeguard chain: 1 remove ip 2 remove key and posky
static char *store_tudou_key_url(char* url){
        
        if((strstr(url,"key=") == NULL)||(strstr(url,"posky=")==NULL)){
                return NULL;
        }

        char *new_url = xstrdup(url);
        char *s;
        char *e;
       
        s = strstr(new_url, "://");
        s += 3;
        e = strstr(s, "/");
        if( e != NULL)
        {
             memmove(s, e+1, strlen(e+1));
             *(s+strlen(e+1)) = 0;
        }

        drop_startend("key=","posky=",new_url);

	debug(137, 3)("(mod_f4v_byte_13_miss_wait) -> store_url is %s \n", new_url);
        return new_url;
}

//tudou safeguard chain: 1 remove ip 2 remove the part after "?"
static char *store_ku6_key_url(char* url){

        /*if((strstr(url,"key=") == NULL)||(strstr(url,"tm=")==NULL)){
                return NULL;
        }*/

        char *new_url = xstrdup(url);
        char *s;
        char *e;

        s = strstr(new_url, "://");
        s += 3;
        e = strstr(s, "/");
        if( e != NULL)
        {
             memmove(s, e+1, strlen(e+1));
             *(s+strlen(e+1)) = 0;
        }

        char *quesurl = strchr(new_url,'?');
        if(quesurl != NULL){
                *quesurl = 0;
        }

        debug(137, 3)("store_url=%s \n", new_url);
        //drop_startend("tm=","key=",new_url);

        return new_url;
}

static void remove_safeguard_chain(clientHttpRequest *http,store_type type){
	
        safe_free(http->request->store_url);
	if(type == STANDARD)
	        http->request->store_url = store_key_url(http->uri);
	else if(type == YOUKU)
	        http->request->store_url = store_youku_key_url(http->uri);
	else if(type == TUDOU)
	        http->request->store_url = store_tudou_key_url(http->uri);
	else if(type == KU6)
	        http->request->store_url = store_ku6_key_url(http->uri);
	else if(type == QQ)
	        http->request->store_url = store_key_url(http->uri);
	else if(type == SOHU)
	        http->request->store_url = store_key_url(http->uri);
	else if(type == SINA)
	        http->request->store_url = store_key_url(http->uri);

}
static int add_range(clientHttpRequest *http)
{
	debug(137, 3)("(mod_f4v_byte_13_miss_wait) -> read request finished, add range: \n");


	//参数不完整，忽略该请求
	if( !http || !http->uri || !http->request ) {
		debug(137, 3)("http = NULL, return \n");
		return 0;
	}


	if(strstr(http->uri,".flv") == NULL && strstr(http->uri,".f4v") == NULL && strstr(http->uri,".hlv") == NULL && strstr(http->uri,".letv") == NULL){
		debug(137, 3)("(mod_f4v_byte_13_miss_wait) ->has not flv video file %s \n", http->uri);
		return 0;
	}

	request_param *req_param = xmalloc(sizeof(request_param));
	memset(req_param,0,sizeof(request_param));
	cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, req_param, free_request_param, mod);

	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);

	remove_safeguard_chain(http,cfg->website);

	//fix one bug which url has start but not get request
	if(http->request->method != METHOD_GET ){
		debug(137, 1)("client url is not GET request and return %s \n", http->uri);
		return 0;
	}

	char *tmp;

	//没有?，返回
	if( (tmp = strchr(http->uri, '?')) == NULL ) {
		debug(137, 3)("<%s> not find ?, return \n", http->uri);
		return 0;
	}

	//没有找到start的变量名，返回
	debug(137,4)("(mod_f4v_byte_13_miss_wait) ->: cfg->start[%s]tmp[%s]\n",cfg->start,tmp);
	if( (tmp = strstr(tmp, cfg->start)) == NULL )  {
		debug(137, 3) ("not find start return %s %s %s \n",http->uri,tmp,cfg->start);
		return 0;
	}

	tmp--;
	if( *tmp != '?' && *tmp != '&' ) {
		debug(137, 3)("<%s> has not start variable, return \n", http->uri);
		return 0;
	}
	tmp++;

	//找起始偏移量
	int start_value;
	tmp += strlen(cfg->start);
	start_value = atol(tmp);

	int end_value = 0;
	if( (tmp = strstr(tmp, cfg->end)) ) {
		tmp--;
		if( *tmp == '?' || *tmp == '&' ) {
			tmp++;
			tmp += strlen(cfg->end);
			end_value = atoi(tmp);
		}
	}


	if( start_value > 13 )
		start_value -= 13;
	else
		start_value = 0;
	char range_value[32];
	if( end_value )
		snprintf(range_value, 31, "bytes=%d-%d", start_value, end_value);
	else
		snprintf(range_value, 31, "bytes=%d-", start_value);

	HttpHeaderEntry eEntry;
	eEntry.id = HDR_RANGE;
	stringInit(&eEntry.name, "Range");
	stringInit(&eEntry.value, range_value);

	httpHeaderAddEntry(&http->request->header, httpHeaderEntryClone(&eEntry));
	stringClean(&eEntry.name);
	stringClean(&eEntry.value);


	req_param->flag = 1;
	req_param->start = start_value;
	req_param->end   = end_value;
	req_param->is_real_flv = HIT_FLV;

	debug(137, 3)("(mod_f4v_byte_13_miss_wait) -> read request finished, add range: done start=%d end=%d \n",start_value,end_value);

	debug(137, 5) ("(mod_f4v_byte_13_miss_wait) ->: paramter start= %s end= %s website = %d \n", cfg->start, cfg->end, cfg->website);

	remove_safeguard_chain(http,cfg->website);

	return 0;
}


static void add_flv_header(clientHttpRequest* http, char* buff, int size)
{
	if(strstr(http->uri,".flv") == NULL && strstr(http->uri,".f4v") == NULL && strstr(http->uri,".hlv") == NULL && strstr(http->uri,".letv") == NULL){
		debug(137, 3)("(mod_f4v_byte_13_miss_wait) ->has not flv video file %s \n", http->uri);
		return ;
	}
	request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if( req_param == NULL ) {
		return;
	}

	if(req_param->is_real_flv != HIT_FLV || req_param->flag != 1){
		return;
	}

	req_param->flag = 2;

	if( size < 13 )
		return;

	const unsigned char flv[13] = {'F', 'L', 'V', 1, 1, 0, 0, 0, 9, 0, 0, 0, 9};
	memcpy(buff, flv, 13);

	debug(137, 6)("(mod_f4v_byte_13_miss_wait) ->: copy f4v data ... \n");
}

static void modify_request(clientHttpRequest * http,char *new_uri){

	debug(137, 5)("(mod_f4v_byte_13_miss_wait) ->: modify_request, uri=[%s]\n", http->uri);
	request_t* old_request = http->request;

	request_t* new_request = urlParse(old_request->method, new_uri);

	debug(137, 5)("(mod_f4v_byte_13_miss_wait) ->: modify_request, new_uri=[%s]\n", new_uri);
	if (new_request)
	{
		safe_free(http->uri);
	//	safe_free(http->request->store_url);
		http->uri = xstrdup(urlCanonical(new_request));
		safe_free(http->log_uri);
		http->log_uri = xstrdup(urlCanonicalClean(old_request));
		new_request->http_ver = old_request->http_ver;
		httpHeaderAppend(&new_request->header, &old_request->header);
		new_request->client_addr = old_request->client_addr;
		new_request->client_port = old_request->client_port;
#if FOLLOW_X_FORWARDED_FOR
		new_request->indirect_client_addr = old_request->indirect_client_addr;
#endif // FOLLOW_X_FORWARDED_FOR 
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

		if(old_request->store_url)
			new_request->store_url = xstrdup(old_request->store_url);

		safe_free(old_request->store_url);

		if(old_request->range)
			new_request->range = httpHdrRangeDup(old_request->range);

		requestUnlink(old_request);
		http->request = requestLink(new_request);
	}
}

static int func_sys_parse_param(char *cfg_line)
{
	if( strstr(cfg_line, "allow") == NULL )
		return 0;

	if( strstr(cfg_line, "mod_f4v_byte_13_miss_wait") == NULL )
		return 0;
		
	store_type website_type = STANDARD;
	if( strstr(cfg_line, "standard") != NULL )
	{
		 website_type = STANDARD;
	}
	else if( strstr(cfg_line, "youku") != NULL )
	{
		 website_type = YOUKU;
	}
	else if( strstr(cfg_line, "tudou") != NULL )
	{
		 website_type = TUDOU;
	}
	else if( strstr(cfg_line, "ku6") != NULL )
	{
		 website_type = KU6;
	}
	else if( strstr(cfg_line, "qq") != NULL )
	{
		 website_type = QQ;
	}
	else if( strstr(cfg_line, "sohu") != NULL )
	{
		 website_type = SOHU;
	}
	else if( strstr(cfg_line, "sina") != NULL )
	{
		 website_type = SINA;
	}

	char *token = NULL;

	if ((token = strtok(cfg_line, w_space)) == NULL)
		return 0;


	struct mod_config *cfg = xmalloc(sizeof(struct mod_config));
	memset(cfg, 0, sizeof(struct mod_config));
	
	token = strtok(NULL, w_space);
	strncpy(cfg->start, token, 126);
	strcat(cfg->start, "=");

	token = strtok(NULL, w_space);
	strncpy(cfg->end, token, 126);
	strcat(cfg->end, "=");

	cfg->website = website_type;

    	debug(137, 5) ("(mod_f4v_byte_13_miss_wait) ->: paramter start= %s end= %s website= %d \n", cfg->start, cfg->end, cfg->website);
	
	cc_register_mod_param(mod, cfg, free_cfg);
	return 0;
}

static int is_disable_sendfile(store_client *sc, int fd) 
{
	sc->is_zcopyable = 0;
	return 1;
}

static int mod_f4v_http_req_process(clientHttpRequest *http){

	if(strstr(http->uri,".flv") == NULL && strstr(http->uri,".f4v") == NULL && strstr(http->uri,".hlv") == NULL && strstr(http->uri,".letv") == NULL){
		debug(137, 3)("(mod_f4v_byte_13_miss_wait) ->has not flv video file %s \n", http->uri);
		return 0;
	}

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		return 0;
	}

	if(req_param->is_real_flv != HIT_FLV){

		return 0;
	}

	debug(137,3)("(mod_f4v_byte_13_miss_wait) ->: http->uri = %s\n",http->uri);
	if(http->request->store_url)
		debug(137,3)("(mod_f4v_byte_13_miss_wait) ->: http->store_url = %s\n",http->request->store_url);
	else
		debug(137,3)("(mod_f4v_byte_13_miss_wait) ->: http->store_url is NULL\n");

	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	assert(cfg);
	char *new_uri = NULL;

	new_uri = xstrdup(http->uri);
	drop_startend(cfg->start, cfg->end, new_uri);
	modify_request(http,new_uri);

	debug(137, 3)("(mod_f4v_byte_13_miss_wait) ->: new request: %s\n", http->uri);
	if(http->request->store_url)
		debug(137,3)("(mod_f4v_byte_13_miss_wait) ->: http->store_url = %s\n",http->request->store_url);
	else
		debug(137,3)("(mod_f4v_byte_13_miss_wait) ->: http->store_url is NULL\n");

	safe_free(new_uri);

	return 0;
}

static void hook_before_sys_init()
{
	return ;

}

static int func_http_repl_send_start(clientHttpRequest *http)
{
	int flag = 0;
	static char location[10240];

	if(strstr(http->uri,".flv") == NULL && strstr(http->uri,".f4v") == NULL && strstr(http->uri,".hlv") == NULL && strstr(http->
uri,".letv") == NULL){
                debug(137, 3)("(mod_f4v_byte_13_miss_wait) ->has not flv video file %s \n", http->uri);
                return 0;
        }

        request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
        if( req_param == NULL){
                return 0;
        }

        if(req_param->is_real_flv != HIT_FLV){

                return 0;
        }

	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
        assert(cfg);

	HttpReply* reply = http->reply;

	if(reply == NULL)
	{	
		debug(137, 3)("(mod_f4v_byte_13_miss_wait) ->request %s reply is NULL, return... \n", http->uri);
		return 0;
	}

	if(reply->sline.status == HTTP_PARTIAL_CONTENT )
		reply->sline.status = HTTP_OK;

	if((reply->sline.status == 301) || (reply->sline.status == 302))
	{
		debug(137, 5)("(mod_f4v_byte_13_miss_wait) ->request %s reply is %d, ... \n", http->uri, reply->sline.status);
	}
	else
	{
                debug(137, 7)("(mod_f4v_byte_13_miss_wait) ->request %s reply is %d, no Location \n", http->uri, reply->sline.status);
		return 0;
	}

	if((req_param->start == 0) && (req_param->end == 0))
	{
                debug(137, 5)("(mod_f4v_byte_13_miss_wait) ->request %s reply is %d, not find start \n", http->uri, reply->sline.status);
		return 0;
	}

	HttpHeaderEntry *myheader;
        HttpHeaderPos pos = HttpHeaderInitPos;

	memset(location, 0, sizeof(location));

	while ((myheader = httpHeaderGetEntry(&reply->header, &pos)))
	{
		if (myheader->id == HDR_LOCATION)
		{
			flag = 1;
			strcpy(location, myheader->value.buf);
			if(strstr(location, "?") == NULL)
				strcat(location, "?");
			else
				strcat(location, "&");

			if(req_param->start)
			{
				strcat(location, cfg->start);
				snprintf(location + strlen(location), 10240 - strlen(location) - 1,"%d", req_param->start + 13);
			}
		
			if(req_param->end)
			{
				if(req_param->start)
					strcat(location, "&");	

				strcat(location, cfg->end);
				snprintf(location + strlen(location), 10240 - strlen(location) - 1, "%d", req_param->end);
			}
	
			debug(137, 5)("[%s] is myheader->value,[%s] is new value\n",myheader->value.buf, location);
			stringReset(&myheader->value, location);

			break;

		}
	}

	if(!flag)
	{
                debug(137, 2)("(mod_f4v_byte_13_miss_wait) ->request %s reply is %d, no Location \n", http->uri, reply->sline.status);
		return 0;
	}

	return 0;
}

int mod_register(cc_module *module)
{
	debug(137, 1)("(mod_f4v_byte_13_miss_wait) ->	mod_register:\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_init = hook_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			hook_init);
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	//module->hook_func_http_req_read_second = add_range;
	cc_register_hook_handler(HPIDX_hook_func_http_req_read_second,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read_second),
			add_range);

	//module->hook_func_private_clientPackMoreRanges = add_flv_header;
	cc_register_hook_handler(HPIDX_hook_func_private_clientPackMoreRanges,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_clientPackMoreRanges),
			add_flv_header);

	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);

	//module->hook_func_client_set_zcopyable = is_disable_sendfile;
	cc_register_hook_handler(HPIDX_hook_func_client_set_zcopyable,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_set_zcopyable),
			is_disable_sendfile);
	
	//module->hook_func_http_req_process = mod_f4v_http_req_process;
        cc_register_hook_handler(HPIDX_hook_func_http_req_process,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
                        mod_f4v_http_req_process);
	
	//module->hook_func_before_sys_init = hook_before_sys_init;

	cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
                        hook_before_sys_init);

	//module->hook_func_http_repl_send_start = func_http_repl_send_start;
        cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
                        func_http_repl_send_start);

	mod = module;

	return 0;
}

#endif

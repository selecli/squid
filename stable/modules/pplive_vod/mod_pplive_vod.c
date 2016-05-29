/*
* mod_pplive_vod.c
* 1) process request url, and add Range if need
* 2) http://domain/segno/rangestart/rangeend/xxx.mp4?xxxx --> miss_start 
*/

#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

#include "m_gzip.h"

static cc_module *mod = NULL;

char m_gzip_param[64];
static int gzip_level = 6;

static helper *media_prefetchers = NULL;
static int is_start_helper = 0;
static int helper_proc_num = 0;

struct mod_config
{
        char    start[128];
        char    end[128];
};      


typedef struct _range_struct
{
        size_t start;
        size_t end;
} range_struct;

typedef struct _request_param
{
	int is_range_request;
	int has_segno;
	int has_start;
	size_t content_length;
	size_t gzip_length;
	int no_gzip;

	gz_stream *gs;
	int flag;

} request_param;

typedef void CHRP(clientHttpRequest *);

typedef struct _media_prefetch_state_data{
        clientHttpRequest * http;
        CHRP *callback;
}media_prefetch_state_data;

CBDATA_TYPE(media_prefetch_state_data);

static MemPool * request_param_pool = NULL;
static MemPool * mod_config_pool = NULL;

static void * request_param_pool_alloc(void)
{
        void * obj = NULL;
        if (NULL == request_param_pool)
        {
                 request_param_pool = memPoolCreate("(mod_pplive_vod) -> private_data request_param", sizeof(request_param));
        }
        return obj = memPoolAlloc(request_param_pool);
}

static int free_request_param(void* data)
{
        request_param *req_param = (request_param *) data;
        if(req_param != NULL){
		if(req_param->gs != NULL)
		{
			destroy(req_param->gs);
			req_param->gs = NULL;
		}
                memPoolFree(request_param_pool, req_param);
                req_param = NULL;
        }
        return 0;
}

static void * mod_config_pool_alloc(void)
{
        void * obj = NULL;
        if (NULL == mod_config_pool)
        {
                mod_config_pool = memPoolCreate("(mod_pplive_vod) -> config_sturct", sizeof(struct mod_config));
        }
        return obj = memPoolAlloc(mod_config_pool);
}

static int free_cfg(void *data)
{
        struct mod_config * cfg = (struct mod_config*)data;
        if(NULL != cfg)
        {
                memPoolFree(mod_config_pool, cfg);
                cfg = NULL;
        }
        return 0;
}

static int httpHeaderCheckExistByName(HttpHeader *h, char *name);
static int need_gzip(clientHttpRequest *http);
static void drop_startend(char* url);
static void drop_domain(char* url);

static range_struct range_pair;

// return: 1, all num; 0 not
static int is_all_digit(char *str)
{
	if(str == NULL)
		return 0;

	debug(183, 6)("(mod_pplive_vod) -> check digit for [%s]\n", str);
        int i;

        for(i = 0; i < strlen(str); i++)
        {
                if((str[i] < '0') || (str[i] > '9'))
                        return 0;
        }

        return 1;
}

// http://domain/segno/rangestart/rangeend/xxx.mp4?xxxx
// return: 1, OK; 0, NO
static int get_range_pair(char *in_url)
{
        static char url[10240];
        char *tmp;
        char *tmp2;

        memset(&range_pair, 0, sizeof(range_struct));
        memset(url, 0, sizeof(url));

        strncpy(url, in_url, 10240);

        if((tmp = strstr(url, "http://")) == NULL)
        {
                debug(183, 1)("not find [http://], return ...\n");
                return 0;
        }

        if((tmp2 = strchr(tmp + strlen("http://"), '/')) == NULL)
        {
                debug(183, 1)("not find [/] after [http://], return ...\n");
                return 0;
        }

        tmp = tmp2 + 1;

	//segno ?
        if((tmp2 = strtok(tmp, "/")) != NULL)
        {
                if(!is_all_digit(tmp2))
                {
                        debug(183, 1)("in segno position is not digit, return ...\n");
                        return 0;
                }
        }
        else
        {
                debug(183, 1)("not have segno, return ...\n");
                return 0;
        }

        //rangestart ?
        if((tmp2 = strtok(NULL, "/")) != NULL)
        {
                if(!is_all_digit(tmp2))
                {
                        debug(183, 1)("in rangestart position is not digit, return ...\n");
                        return 0;
                }
                else
                {
                        range_pair.start = atoi(tmp2);
                }
        }

	else
        {
                debug(183, 1)("not have rangestart, return ...\n");
                return 0;
        }

        //rangeend ?
        if((tmp2 = strtok(NULL, "/")) != NULL)
        {
                if(!is_all_digit(tmp2))
                {
                        debug(183, 1)("in rangeend position is not digit, return ...\n");
                        return 0;
                }
                else
                {
                        range_pair.end = atoi(tmp2);
                }
        }
        else
        {
                debug(183, 1)("not have rangeend, return ...\n");
                return 0;
        }

        return 1;
}

static char * get_and_drop_segno(char* url, char* segno)
{
        char *s;
        char *e;

        static char segno_buf[32];

        s = strstr(url, segno);
        if(s == NULL)
                return NULL;

        memset(segno_buf, 0, sizeof(segno_buf));

        e = strstr(s, "&");
        if(e == NULL)
        {
                strncpy(segno_buf, s + strlen(segno) + 1, 31);
                *(s - 1) = 0;
        }
        else
        {
                memcpy(segno_buf, s + strlen(segno) + 1, e - s - strlen(segno) - 1);
                memmove(s, e + 1, strlen(e + 1));
                *(s + strlen(e + 1)) = 0;
        }

        return segno_buf;
}

static char * get_segno_url(char *url, char *segno_keyword)
{
        static char buffer[40960];
        char * segno = NULL;
        char * tmp = NULL;

        if((segno = get_and_drop_segno(url, segno_keyword)) == NULL)
                return NULL;
	else
		debug(183, 3)("(mod_pplive_vod) -> after drop_segno, we get segno: [%s], url: [%s]\n", segno, url);


        if(strstr(url, "http://") == NULL)
                return NULL;

        if((tmp = strstr(url + strlen("http://"), "/")) == NULL)
                return NULL;

	memset(buffer, 0, sizeof(buffer));

        strncpy(buffer, url, tmp - url + 1);
        strcat(buffer, segno);
        strcat(buffer, "/");
        strcat(buffer, tmp + 1);

        return buffer;
}

static char * change_url(char *url)
{
        static char buffer[40960];
        static char result[40960];
        int len, i, status, pos, change;
        char *underscodes = "_____________________________________________________";
        char ori = '%';

        if((len = strlen(url)) > sizeof(buffer) -1)
                return NULL;

	memset(buffer, 0, sizeof(buffer));
        memset(result, 0, sizeof(result));

        strcpy(buffer, url);

        for(i = 0, status = 0, pos = 0, change = 0; i < len; i++)
	{
		if(buffer[i] == '_')
                {
                        status++;
		}
		else 
		{
			if(status >= 2)
			{
				if(((buffer[i] != 0) && ((buffer[i] >= '0' && buffer[i] <= '9') || (buffer[i] >= 'a' && buffer[i] <= 'f') || (buffer[i] >= 'A' && buffer[i] <= 'F'))) && ((buffer[i + 1] != 0) && ((buffer[i + 1] >= '0' && buffer[i + 1] <= '9') || (buffer[i + 1] >= 'a' && buffer[i + 1] <= 'f') || (buffer[i + 1] >= 'A' && buffer[i + 1] <= 'F'))))
				{
					if(status > 2)
					{
						memcpy(result + pos, underscodes, status - 2);
						pos += (status - 2);

						memcpy(result + pos, &ori, 1);
						pos++;
						change = 1;

						memcpy(result + pos, buffer + i, 2);
						pos += 2;
						i++;

						status = 0;
					}
					else if(status == 2)
					{
						memcpy(result + pos, &ori, 1);
						pos++;
						change = 1;

						memcpy(result + pos, buffer + i, 2);
						pos += 2;
						i++;

						status = 0;
					}
				}	
				else
				{
					memcpy(result + pos, underscodes, status);
					pos += status;

					memcpy(result + pos, buffer + i, 1);
					pos++;

					status = 0;
				}
			}
			else
			{
				if(status > 0)
				{
					memcpy(result + pos, underscodes, status);
					pos += status;
				}

				memcpy(result + pos, buffer + i, 1);
				pos++;
				status = 0;
			}
		}
	}
				
        if(change)
                return result;
        else
        {
                debug(183, 3)("(mod_pplive_vod) -> Nothing changed ...\n");
                return NULL;
        }
}

static int hook_init()
{
        debug(183, 1)("(mod_pplive_vod) ->     hook_init:----:)\n");
        return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	char line[4096];       
	char *token = NULL;
	debug(183, 1)("(mod_pplive_vod) ->     hook_ func_sys_parse_param:cfg_line--: ) : %s\n", cfg_line);

	memset(line, 0, sizeof(line));
	strncpy(line, cfg_line, 4095);

	helper_proc_num = 0;

	memset(m_gzip_param,0,64);

	if(strstr(line, "mod_pplive_vod") == NULL)
		return 0;

	if(strstr(line, "allow") != NULL)
		is_start_helper = 1;

	if((token = strstr(line, "gzip_level=")) != NULL)
	{
			if((*(token + strlen("gzip_level=")) >= '0') && (*(token + strlen("gzip_level=")) <= '9'))
				gzip_level = *(token + strlen("gzip_level=")) - '0';
			else
				gzip_level = 6;
	
        	snprintf(m_gzip_param, 64,"%c%c%d", 'w', 'b', gzip_level);
		debug(183, 3)("(mod_pplive_vod) -> gzip_level: [%d]\n", gzip_level);
	}


        if ((token = strtok(line, w_space)) == NULL)
                return 0;

	struct mod_config *cfg = mod_config_pool_alloc();
        memset(cfg, 0, sizeof(struct mod_config));

        if((token = strtok(NULL, w_space)) == NULL)
	{
		debug(183, 1)("(mod_pplive_vod) -> cfg line , not find start\n");
		abort();
	}
	
        strncpy(cfg->start, token, 126);
	debug(183, 3)("(mod_pplive_vod) -> cfg->start: [%s]\n", cfg->start);

	if((token = strtok(NULL, w_space)) == NULL)
	{
		debug(183, 1)("(mod_pplive_vod) -> cfg line , not find end\n");
		abort();
	}
        strncpy(cfg->end, token, 126);
	debug(183, 3)("(mod_pplive_vod) -> cfg->end: [%s]\n", cfg->end);

	cc_register_mod_param(mod, cfg, free_cfg);

	if(is_all_digit(token))
	{
		helper_proc_num = atoi(token);

		debug(183, 3)("(mod_pplive_vod) -> helper_num is: %d\n", helper_proc_num);
		return 0;
	}

	while((token = strtok(NULL, w_space)) != NULL)
	{
		if(is_all_digit(token))
		{
			helper_proc_num = atoi(token);

			debug(183, 3)("(mod_pplive_vod) -> helper_num is: %d\n", helper_proc_num);
			return 0;
		}
	}

	if(helper_proc_num == 0)
		is_start_helper = 0;

        return 0;
}

static int hook_cleanup(cc_module *module)
{
        debug(183, 1)("(mod_pplive_vod) ->     hook_cleanup:\n");
	return 0;
}

static void add_range_for_request(clientHttpRequest *http, int start_value, int end_value)
{
	debug(183, 3)("(mod_pplive_vod) -> add range: [%d-%d] for %s\n", start_value, end_value, http->uri);

	char range_value[32];
	if(end_value)
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

	return;
}

// 1: OK; 0: bad
static int check_if_range_ok(range_struct *range_pair)
{
	if((range_pair->start < 0) || (range_pair->end < 0))
		return 0;

	if((range_pair->start > range_pair->end) && (range_pair->end != 0))
		return 0;

	return 1;
}

static void make_416_response(clientHttpRequest * http)
{
        char *url = http->uri;
        request_t *r = http->request;
        ErrorState *err = NULL;
        http->flags.hit = 0;
	debug(183, 2)("(mod_pplive_vod) -> 416 for '%s %s'\n",
                        RequestMethods[r->method].str, url);
	http->log_type = LOG_TCP_MISS;
        http->al.http.code = HTTP_RANGE_NOT_SATISFIABLE;
        err = errorCon(ERR_INVALID_REQ, HTTP_RANGE_NOT_SATISFIABLE, http->orig_request);
        if (http->entry)
        {
                storeClientUnregister(http->sc, http->entry, http);
                http->sc = NULL;
                storeUnlockObject(http->entry);
        }
        http->entry = clientCreateStoreEntry(http, r->method, null_request_flags);
        errorAppendEntry(http->entry, err);
}

/*
static void add_private_data_for_http(clientHttpRequest *http)
{
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if(req_param == NULL)
	{
		req_param = request_param_pool_alloc();
		cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, req_param, free_request_param, mod);
	}
	else
	{
        	debug(183, 2)("(mod_pplive_vod) -> already has request private data\n");
	}
}
*/

//only remove ?...
static void add_store_url(clientHttpRequest *http)
{
	char *url = http->uri;
	safe_free(http->request->store_url);

	char * uri = xstrdup(url);
        char *tmpuri = strstr(uri,"?");
        if(tmpuri != NULL)
                *tmpuri = 0;
                
	drop_domain(uri);

	http->request->store_url = uri;
        debug(183, 5)("(mod_pplive_vod) -> store_url is %s \n", uri);

        return;
}       

//remove /rangestart/rangeend/   and ?...
static void add_store_url_for_range(clientHttpRequest *http)
{
	static char buffer1[10240];
	static char buffer2[10240];

	char *tmp;

	safe_free(http->request->store_url);

	memset(buffer1, 0, sizeof(buffer1));
	memset(buffer2, 0, sizeof(buffer1));

	strncpy(buffer1, http->uri, 10240);

	if((tmp = strstr(buffer1, "?")) != NULL)
		*tmp = '\0';

	drop_startend(buffer1);
	drop_domain(buffer1);

	http->request->store_url = xstrdup(buffer1);
        debug(183, 5)("(mod_pplive_vod) -> store_url is %s \n", buffer1);

	return;
}

//remove ? and append @@@Accept-Encoding:gzip for whole request with compress ...
static void add_compress_store_url(clientHttpRequest *http)
{
	char buffer[10240];
	safe_free(http->request->store_url);

	strcpy(buffer, http->uri);
        char *tmpuri = strstr(buffer,"?");
        if(tmpuri != NULL)
                *tmpuri = 0;

	strcat(buffer, "@@@Accept-Encoding:gzip");
	drop_domain(buffer);
                
	http->request->store_url = xstrdup(buffer);
        debug(183, 5)("(mod_pplive_vod) -> store_url is %s \n", buffer);

        return;
}

static inline void modify_request(clientHttpRequest * http)
{
        debug(97, 3)("modify_request: start, uri=[%s]\n", http->uri);
        request_t* old_request = http->request;
        request_t* new_request = urlParse(old_request->method, http->uri);
        safe_free(http->uri);

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
                requestUnlink(old_request);
                http->request = requestLink(new_request);
        }
}

static int request_url_has_start(clientHttpRequest *http)
{
	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
        assert(cfg);

        debug(183, 6)("(mod_pplive_vod) -> search [%s] for %s\n", cfg->start, http->uri);

	if(strstr(http->uri, cfg->start) != NULL)
		return 1;
	else
		return 0;
}

static int change_request_url(clientHttpRequest *http)
{
	char *change_result, *segno_url;
        static char url_buffer[10240];

        int need_chang_request = 0;

	if( !http || !http->uri || !http->request ) {
                debug(183, 3)("(mod_pplive_vod) -> before redirect, http = NULL, return \n");
                return 0;
        }

        debug(183, 3)("(mod_pplive_vod) -> before redirect, change url start: %s\n", http->uri);

	//local range miss_start curl request ? already changed, return
        if(httpHeaderCheckExistByName(&http->request->header, "X-CC-Media-Prefetch"))
        {
                debug(183, 5)("(mod_pplive_vod) -> Prefetch request: %s\n", http->uri);
                return 0;
        }

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

        if(req_param == NULL)
        {
                req_param = request_param_pool_alloc();
		req_param->has_segno = 0;
		req_param->has_start = 0;
		req_param->is_range_request = 0;
		req_param->content_length = -1;
		req_param->gzip_length = -1;
		req_param->no_gzip = 1;
		req_param->gs = NULL;

                cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, req_param, free_request_param, mod);
        }
        else
        {
                debug(183, 2)("(mod_pplive_vod) -> already has request private data\n");
	}

	if(need_gzip(http))
		req_param->no_gzip = 0;
	else
		req_param->no_gzip = 1;

	strcpy(url_buffer, http->uri);

        if((change_result = change_url(url_buffer)) == NULL)
        {
                (void) 0;
        }
        else
        {
                debug(183, 5)("(mod_pplive_vod) -> after change url is: %s\n", change_result);
                strcpy(url_buffer, change_result);
                need_chang_request = 1;
        }

	if((segno_url =  get_segno_url(url_buffer, "segno")) == NULL)
        {
                debug(183, 5)("(mod_pplive_vod) -> has no segno keyword, return ...\n");
        }
        else
        {
                need_chang_request = 2;
                debug(183, 5)("(mod_pplive_vod) -> after process segno, url is: %s\n", segno_url);

		req_param->has_segno = 1;

		/*
		//if url has both segno and start, it must not a range request; else it has segno, so it maybe a range request
		if(request_url_has_start(http))
		{
			req_param->has_start = 1;
			req_param->no_gzip = 1;
		//	add_private_data_for_http(http);
		}
		*/
        }

	if(need_chang_request == 1)
        {
                strcpy(http->uri, url_buffer);
        }
        else if(need_chang_request == 2)
        {
                strcpy(http->uri, segno_url);
        }

        if(need_chang_request)
        {
                modify_request(http);
                debug(183, 5)("(mod_pplive_vod) -> we get new request: %s\n", http->uri);
        }

        debug(183, 3)("(mod_pplive_vod) -> before redirect, change url result: %s\n", http->uri);

	return 0;
}

//return: 0, OK; -1, ERR
static int process_url_and_add_range_or_storeurl(clientHttpRequest *http)
{
	char *segno_url;

        debug(183, 3)("(mod_pplive_vod) -> read request finished, add range: \n");

        if( !http || !http->uri || !http->request ) {
                debug(183, 3)("http = NULL, return \n");
                return 0;
        }

	segno_url = http->uri;

	//local range miss_start curl request ?
	if(httpHeaderCheckExistByName(&http->request->header, "X-CC-Media-Prefetch"))
	{
		debug(183, 5)("(mod_pplive_vod) -> Prefetch request: %s\n", http->uri);
	
		add_store_url(http);
		return 0;
	}

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

        if(req_param == NULL)
	{
		debug(183, 2)("(mod_pplive_vod) -> request: %s has no req_param ... !!!\n", http->uri);
		return 0;
	}

	/*
	if(!(req_param->has_segno) || (req_param->has_segno && req_param->has_start))
        {
		debug(183, 5)("(mod_pplive_vod) -> request has no segno, or has both segno and start keyword, so don't need add Range: %s\n", http->uri);
		add_store_url(http);
		return 0;
	}
	*/

	if(req_param->has_start)
	{
		debug(183, 5)("(mod_pplive_vod) -> request has start keyword, so don't need add Range: %s\n", http->uri);
		add_store_url(http);
		return 0;
	}

	// has no start, if also has no segno too, it must a whole request
	if(!req_param->has_segno)
	{
		debug(183, 5)("(mod_pplive_vod) -> request has no segno and no start keyword, so don't need add Range: %s\n", http->uri);
		if(req_param->no_gzip)
			add_store_url(http);
		else // we store whole compress request as a different cache, so it do not need gzip again when sent to client
		{
			add_compress_store_url(http);
			req_param->no_gzip = 1;
		}

		return 0;
	}

	// at here request must has segno= , check if a range request ...
	req_param->is_range_request = 0;

	if(get_range_pair(segno_url) == 0)
	{
		debug(183, 5)("(mod_pplive_vod) -> url has no range pair, return\n");

		if(req_param->no_gzip)
			add_store_url(http);
		else
		{
			add_compress_store_url(http);
			req_param->no_gzip = 1;
		}	

		return 0;
	}
	else
	{
		debug(183, 5)("(mod_pplive_vod) -> we get range:[%d-%d] \n", (int)range_pair.start, (int)range_pair.end);

		if(check_if_range_ok(&range_pair))
		{
			add_range_for_request(http, range_pair.start, range_pair.end);
			req_param->is_range_request = 1;

			add_store_url_for_range(http);
			return 0;
		}
		else
		{
			debug(183, 3)("(mod_pplive_vod) -> bad range, response 416 to client ... \n");
			make_416_response(http);

			return -1;
		}
	}
}	

// miss start for rangestart/rangeend
static int httpHeaderCheckExistByName(HttpHeader *h, char *name)
{
        HttpHeaderPos pos = HttpHeaderInitPos;
        HttpHeaderEntry *curHE;
        while ((curHE = httpHeaderGetEntry(h, &pos)))
        {
                if(!strcmp(strBuf(curHE->name), name))
                {
                        debug(183,3)("(mod_pplive_vod) -> Internal X-CC-Media-Prefetch request!\n");
                        return 1;
                }
        }

        debug(183,3)("(mod_pplive_vod) -> Not internal X-CC-Media-Prefetch request!\n");
        return 0;
}

static int need_gzip(clientHttpRequest *http)
{
	if(request_url_has_start(http))
		return 0;

	request_t *req = http->request;
        if(req == NULL)
                return 0;
        
	if(httpHeaderHas(&req->header, HDR_ACCEPT_ENCODING))
		return 1;
	else
		return 0;
}

static void mediaPrefetchHandleReplyForMissStart(void *data, char *reply)
{               
        media_prefetch_state_data* state = data;
                        
        if(strcmp(reply, "OK"))
        {               
                debug(183,3)("(mod_pplive_vod) -> can not prefetch media! reply=[%s] \n",reply);
        }       
        else{   
                debug(183,3)("(mod_pplive_vod) ->: quering whole object is success reply=[%s] \n",reply);
        }               
                
        cbdataFree(state);
}               

static int is_disable_sendfile(store_client *sc, int fd)
{               
        sc->is_zcopyable = 0;
        return 1;
}       

static void restore_http(clientHttpRequest *http){

        request_t *req = http->request;
        if(req == NULL){
                debug(183, 2)("(mod_pplive_vod) ->: http request is null in restore_http \n");
                return;
        }

        HttpHeaderEntry *range = NULL;
        range = httpHeaderFindEntry(&req->header, HDR_RANGE);
        if(range != NULL){

                httpHeaderDelById(&req->header, HDR_RANGE);
        }
        return;

}

// for store_url
static void drop_domain(char *url)
{
	char *tmp1;
	char *tmp2;
	static char buffer[10240];
	memset(buffer, 0, sizeof(buffer));

	strcpy(buffer, url);
	if((tmp1 = strstr(buffer, "http://")) == NULL)
		return;

	//http://domain/
	if((tmp2 = strchr(tmp1 + strlen("http://") + 1, '/')) == NULL)
		return;

	tmp2++;

	strcpy(url, "http://");
	strcat(url, tmp2);
}

// http://domain/segno/rangestart/rangeend/xxx.mp4?xxxx
static void drop_startend(char* url)
{
	char *tmp1;
	char *tmp2;
	static char buffer[10240];
	static char result[10240];
	memset(buffer, 0, sizeof(buffer));
	memset(result, 0, sizeof(result));

	strcpy(buffer, url);
	if((tmp1 = strstr(buffer, "http://")) == NULL)
		return;

	//http://domain/
	if((tmp2 = strchr(tmp1 + strlen("http://") + 1, '/')) == NULL)
		return;

	tmp2++;

	//http://domain/segno/
	if((tmp1 = strchr(tmp2, '/')) == NULL)
                return;

	tmp1++;
	*(tmp1 - 1) = '\0';

	strcpy(result, buffer);

	//http://domain/segno/rangestart/
	if((tmp2 = strchr(tmp1, '/')) == NULL)
		return;

	tmp2++;

	//http://domain/segno/rangestart/rangeend/
	if((tmp1 = strchr(tmp2, '/')) == NULL)
                return;

	strcat(result, tmp1);
	strcpy(url, result);
}

static int mod_f4v_http_req_process(clientHttpRequest *http){

	fde *F = NULL;
        int fd = -1;
        int local_port = -1;
        static char buf[4096];
        static char tmp_buf[32];

	if(http->request->store_url)
	{
        	debug(183, 3)("(mod_pplive_vod) -> uri: [%s], store_url: [%s] \n", http->uri, http->request->store_url);
	}
	else
	{
        	debug(183, 3)("(mod_pplive_vod) -> uri: [%s], store_url: [%s] \n", http->uri, "NULL");
	}

	if(is_start_helper == 0){

                debug(183,3)("(mod_pplive_vod) -> helper proc num is %d, return ...\n",  helper_proc_num);
		return 0;
	}

	int has_header = httpHeaderCheckExistByName(&http->request->header, "X-CC-Media-Prefetch");

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		return 0;
	}

	if(req_param->is_range_request == 0){
		return 0;
	}

	if( http->log_type == LOG_TCP_MISS && !has_header)
	{
		debug(183,3)("(mod_pplive_vod) ->: request miss_start object because http->log_type=%d\n", http->log_type);
		StoreEntry *e = NULL;

		if(http->request->store_url == NULL)
		{
			debug(183,3)("(mod_pplive_vod) ->: request uri: %s, but it has no store_url!\n", http->uri);
			http->request->flags.cachable = 0;//no cache
			return 0;
		}
		e = storeGetPublic(http->request->store_url,METHOD_GET);

		if(e == NULL){//query whole object

			CBDATA_INIT_TYPE(media_prefetch_state_data);
			media_prefetch_state_data *state = cbdataAlloc(media_prefetch_state_data);
			state->http = NULL;
			state->callback = NULL;

			char *new_uri = xstrdup(http->uri);
			drop_startend(new_uri);

			debug(183, 2) ("(mod_pplive_vod) ->: miss and redirect to upper layer to query whole data %s \n",new_uri);
			//static char buf[4096];
			//snprintf(buf, 4096, "%s\n", new_uri);
			//safe_free(new_uri);
			snprintf(buf, 4096, "%s", new_uri);
			safe_free(new_uri);

                        fd = http->conn->fd;
                        F = &fd_table[fd];
                        
                        if(!F->flags.open)
			{
                                debug(183, 2)("(mod_pplive_vod) ->: FD is not open!\n");
				local_port = 80;
			}
                        else
			{
                        	local_port = F->local_port;
			}
                
                        debug(183, 3) ("(mod_pplive_vod) ->: http->uri: %s\n", buf);
                        debug(183, 3) ("(mod_pplive_vod) ->: http->port: %d\n", local_port);
        
                        memset(tmp_buf, 0, sizeof(tmp_buf));

                        snprintf(tmp_buf, 32, "@@@%d", local_port);
                        strcat(buf, tmp_buf);
                        strcat(buf, "\n");

                        debug(183, 3) ("(mod_pplive_vod) ->: helper buf: %s\n", buf);
			helperSubmit(media_prefetchers, buf, mediaPrefetchHandleReplyForMissStart, state);
		}

		if(e == NULL|| e->store_status != STORE_OK){

			debug(183, 2) ("(mod_pplive_vod) ->: pending and redirect to upper layer to query start data %s\n",http->uri);
			restore_http(http);
			http->request->flags.cachable = 0;//no cache
			safe_free(http->request->store_url);

			http->entry = NULL;
			http->log_type = LOG_TCP_MISS;

		}
		else{
			debug(183, 3) ("(mod_pplive_vod) ->: flv hit request http->uri = %s\n", http->uri);
		}

		return 0;
	}
	
	return 0;
}

static void parse_programline(wordlist ** line)
{
        if (*line)
                self_destruct();
        parse_wordlist(line);
}

static void hook_before_sys_init()
{

        if(is_start_helper == 0){

                debug(183,3)("(mod_pplive_vod) ->: module cfg is_start_helper=%d and don't start helper program\n",is_start_helper);

                if (media_prefetchers)
                {
                        helperShutdown(media_prefetchers);
                        helperFree(media_prefetchers);
                        media_prefetchers = NULL;
                }

                return;
        }


        debug(183,3)("(mod_pplive_vod) ->: module cfg is_start_helper=%d and start helper program\n",is_start_helper);
        debug(183,3)("(mod_pplive_vod) ->: helper process number  =%d \n",helper_proc_num);

        if (media_prefetchers)
        {
               helperShutdown(media_prefetchers);
               helperFree(media_prefetchers);
               media_prefetchers = NULL;
        }

	media_prefetchers = helperCreate("pplive_media_prefetcher");
        wordlist *pwl = NULL;
        char cfg_line2[128];
        strcpy(cfg_line2,"a /usr/local/squid/bin/pplive_media_prefetch.pl");
        strtok(cfg_line2, w_space);
        parse_programline(&pwl);

        media_prefetchers->cmdline = pwl;
        media_prefetchers->n_to_start = helper_proc_num;
        media_prefetchers->concurrency = 0;
        media_prefetchers->ipc_type = IPC_STREAM;
        helperOpenServers(media_prefetchers);

}

// like mp4 mod_mp4_client_send_more_data function to release entry of MISS/2xx reply of IMS request  
// ( http://xxxx/segno/rangestart/rangeend/...?...)  hit, but expired, then If-Modify-Since ??? ... <---------------> MISS/2xx
static int mod_f4v_release_incomplete_entry(clientHttpRequest * http, MemBuf *mb,char *buf, ssize_t size)
{
        request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
        if( req_param == NULL){
                return 0;
        }

	if(req_param->is_range_request == 0){
                return 0;
        }
	
	StoreEntry *entry = http->entry;

        if(!entry || entry->swap_dirn == -1){
#if 1   
                //fix case 2927
                if (entry) {
                        if (entry->mem_obj->store_url) {
                                if (storeGet(storeKeyPublic(entry->mem_obj->store_url,METHOD_GET)))
                                {
                                        storeRelease(storeGet(storeKeyPublic(entry->mem_obj->store_url,METHOD_GET)));
                                        debug(183, 2)("(mod_pplive_vod) ->     release incomplete IMS reply entry\n");
                                }
                                else
                                {
                                        debug(183, 3)("(mod_pplive_vod) -> no.1\n");
                                }
                        }
                        else
                        {
                                debug(183, 3)("(mod_pplive_vod) -> no.2\n");
                        }
                }
                else
                {
                        debug(183, 3)("(mod_pplive_vod) -> no.3\n");
		}
#endif
        }
        else
        {
                debug(183, 9)("(mod_pplive_vod) -> ok\n");
        }

        return 0;
}

// if hit, get reply content-length and gzip
static int gzip_body_data(clientHttpRequest * http, MemBuf *mb, char *buf, ssize_t size) {

	if(!http)
	{
        	debug(183, 1)("(mod_pplive_vod) -> http is NULL ... \n");
		return 0;
	}

	if(!http->flags.hit)
	{
        	debug(183, 6)("(mod_pplive_vod) -> request is MISS, return ... \n");
		return 0;
	}

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	gz_stream * gs = NULL;
	int flag = -1;
	int first = 0;
	static char result_buffer[1024*64*10];
	size_t result_len = 0;
                
        if(req_param == NULL)
        {
        	debug(183, 2)("(mod_pplive_vod) -> req_param is NULL, return ... \n");
		return 0;
	}

	if(req_param->no_gzip)
	{
        	debug(183, 6)("(mod_pplive_vod) -> request: %s reply's need not gzip, return ... \n", http->uri);
		return 0;
	}

	flag = req_param->flag;

	if(flag == 0)
	{	first = 1;
		req_param->flag = 1;
	}

       	debug(183, 6)("(mod_pplive_vod) -> before gzip %s, buffer size is: %d\n", http->uri, (int)mb->size);

	gs = req_param->gs;
	req_param->gzip_length += mb->size;

	if(req_param->gzip_length == req_param->content_length)
	{
		if(first)
			flag = 3;
		else
			flag = 2;
	}

	m_gzip(gs, flag, mb->buf, mb->size, result_buffer, &result_len, 1024*64*10);

	if(result_len > 0)
	{
		memBufReset(mb);
		memBufAppend(mb, result_buffer, result_len);
	}
	else
	{
       		debug(183, 2)("(mod_pplive_vod) -> after gzip %s, buffer size is: %d\n", http->uri, (int)result_len);
		req_param->no_gzip = 1;
	}

       	debug(183, 6)("(mod_pplive_vod) -> after gzip %s, buffer size is: %d\n", http->uri, (int)mb->size);
	
	return 0;
}

static void transfer_done_precondition(clientHttpRequest *http, int *precondition)
{                       
	if(!http)
        {
                debug(183, 1)("(mod_pplive_vod) -> http is NULL ... \n");
                return;
        }

        if(!http->flags.hit)
        {
                debug(183, 6)("(mod_pplive_vod) -> request is MISS, return ... \n");
                return;
        }

        request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if(req_param == NULL)
        {
                debug(183, 2)("(mod_pplive_vod) -> req_param is NULL, return ... \n");
                return;
        }

        if(req_param->no_gzip)
        {
                debug(183, 6)("(mod_pplive_vod) -> request: %s need not gzip reply, return ... \n", http->uri);
                return;
        }

	if(req_param->content_length == req_param->gzip_length)
	{
                debug(183, 2)("(mod_pplive_vod) -> request: %s send end ... \n", http->uri);
		*precondition = 1;
		http->flags.done_copying = 1;
	}

	return;
}

// if reply HTTP_OK:200/206
static int if_reply_ok(HttpReply *reply)
{
        if(reply != NULL)
        {
                return ((reply->sline.status == HTTP_OK) || (reply->sline.status == HTTP_PARTIAL_CONTENT))?1:0;
        }
        else
                return 1;
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

static void private_repl_send_start(clientHttpRequest *http, int *ret)
{                       
	if(!http)
        {
                debug(183, 1)("(mod_pplive_vod) -> http is NULL ... \n");
                return;
        }

        if(!http->flags.hit)
        {
                debug(183, 2)("(mod_pplive_vod) -> request is MISS, return ... \n");
                return;
        }

        if(!if_reply_ok(http->reply))
        {
                debug(183, 2)("(mod_cpis_body_modify) -> httpState entry url: %s, reply status: %d is not 200/206, so we don't process it\n", storeUrl(http->entry), http->reply->sline.status);
                return ;
        }               
                        
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if(req_param == NULL)
        {
                debug(183, 2)("(mod_pplive_vod) -> req_param is NULL, return ... \n");
                return;
        }

	if(req_param->no_gzip)
        {
                debug(183, 2)("(mod_pplive_vod) -> request: %s need not gzip reply, return ... \n", http->uri);
                return;
        }

	if(req_param->content_length == -1)
        {
                if(http->reply->content_length > 0)
                {
                        req_param->content_length = http->reply->content_length;
                        req_param->gzip_length = 0;


                        //memset(m_gzip_param,0,64);
                        //snprintf(m_gzip_param, 64,"%c%c%d", 'w', 'b', gzip_level);
                        gz_stream *g_s = gz_open(m_gzip_param);
                        if(g_s == NULL)
                        {
                                req_param->no_gzip = 1;
                                debug(183, 2)("(mod_pplive_vod) -> gz_open failed ...\n");
                                return;
                        }

                        req_param->gs = g_s;
                        req_param->flag = 0;

                        debug(183, 2)("(mod_pplive_vod) -> request: %s reply content_length is %d\n", http->uri, (int)req_param->content_length);
                }
		else
                {
                        req_param->no_gzip = 1;
                        debug(183, 2)("(mod_pplive_vod) -> request: %s reply content_length is bad, return ... \n", http->uri);
                        return;
                }
        }

	const char* Content_Encoding = httpHeaderGetStr2(&http->reply->header, HDR_CONTENT_ENCODING);
        if(!Content_Encoding)
                httpHeaderPutStr(&http->reply->header, HDR_CONTENT_ENCODING, "gzip");

        const char* transfer_encoding = httpHeaderGetStr2(&http->reply->header, HDR_TRANSFER_ENCODING);
        if(!transfer_encoding)
                httpHeaderPutStr(&http->reply->header, HDR_TRANSFER_ENCODING, "Chunked");
        http->request->flags.chunked_response = 1;
        httpHeaderDelById(&http->reply->header, HDR_CONTENT_LENGTH);

        http->reply->content_length = -1;

        *ret = 0;
}

int mod_register(cc_module *module)
{
        debug(183, 1)("(mod_pplive_vod) ->     mod_register:\n");

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
        cc_register_hook_handler(HPIDX_hook_func_http_private_req_read_second,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_private_req_read_second),
                        process_url_and_add_range_or_storeurl);
	//module->hook_func_sys_cleanup = hook_cleanup;
        cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
                        hook_cleanup);

	//module->hook_func_before_sys_init = hook_before_sys_init;
        cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
                        hook_before_sys_init);

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

	//module->hook_func_private_mp4_send_more_data = mod_mp4_client_send_more_data;
        cc_register_hook_handler(HPIDX_hook_func_private_mp4_send_more_data,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_mp4_send_more_data),
                        mod_f4v_release_incomplete_entry);

	//module->hook_func_http_before_redirect = clientHandleMp4;
        cc_register_hook_handler(HPIDX_hook_func_http_before_redirect,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
                        change_request_url);

	cc_register_hook_handler(HPIDX_hook_func_private_client_send_more_data,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_send_more_data),
                        gzip_body_data);

	cc_register_hook_handler(HPIDX_hook_func_transfer_done_precondition,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_transfer_done_precondition),
                        transfer_done_precondition);

	cc_register_hook_handler(HPIDX_hook_func_private_repl_send_start,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_repl_send_start),
                        private_repl_send_start);

	if(reconfigure_in_thread)
                mod = (cc_module*)cc_modules.items[module->slot];
        else
                mod = module;
        return 0;
}

#endif

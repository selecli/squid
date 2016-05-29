/*******************************************************************************************
 *	Author: liubin@chinacache.com
 *	Date  : 2012/7/17
 *
 *	Useage:
 *		mod_merge_request keyword helper_num allow acl
 *	eg    :
 *		acl wasu_ts_ad url_regex -i ^http://wasu.com/A.ts|ad=http://wasu.com/B.ts
 *		mod_merge_request ab 60 allow wasu_ts_ad
 *	Detail:
 *		client will receive one ts file composed by A.ts and B.ts.
 *	Case  : 3301
 ********************************************************************************************/

#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

#define SUBSLEN 10
#define EBUFLEN 128             
#define BUFLEN 1024

static cc_module* mod = NULL;

typedef struct _mod_config 
{
	char keyword[128];
}mod_config;

typedef struct _request_param
{
	Array urls;
	squid_off_t ad_ts_len;
	squid_off_t normal_ts_len;
	int current_url;
	squid_off_t start_value;
	squid_off_t end_value;
	int is_send_body;
	int orig_request_range;
	int end_exist;
	struct {
		//1: (true)have range request,0: (false)haven't range request
		unsigned int send_ad_ts:1;
		unsigned int send_normal_ts:1;
	} flags;
	struct {
		squid_off_t ad_range_start;
		squid_off_t ad_range_end;
		squid_off_t normal_range_start;
		squid_off_t normal_range_end;
	} range_value;
}request_param;

typedef struct _prefetch_state_data
{
	clientHttpRequest * http;
	HLPCB *callback;
}prefetch_state_data;

CBDATA_TYPE(prefetch_state_data);

static HLPCB prefetchHandleReply;
static helper *prefetchers = NULL;
static int helper_proc_num = 60;


static void
prefetchHandleReply(void *data, char *reply)
{
	prefetch_state_data *state = data; 
        request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, state->http, mod);

        if(!req_param) {
		debug(162, 0)("mod_merge_request prefetchHandleReply cc_get_mod_private_data failed!\n");
		return;
        }
	
	if(reply) {
		char *p = NULL, *q = NULL;
		if ((p = strstr(reply, "ad_len")) == NULL || (q = strstr(reply, "normal_len")) == NULL) {
			debug(162, 0)("mod_merge_request content_len_prefetch.pl failed for :[%s]\n",reply);
			return;
		}

		req_param->ad_ts_len = atoll(p+strlen("ad_len="));
		req_param->normal_ts_len = atoll(q+strlen("normal_len="));

		debug(162, 3)("mod_merge_request prefetchHandleReply ad_ts_len=[%"PRINTF_OFF_T"] normal_ts_len=[%"PRINTF_OFF_T"]\n",req_param->ad_ts_len, req_param->normal_ts_len);
	}

        int valid = cbdataValid(state->http);
        cbdataUnlock(state->http);
        cbdataUnlock(state);
        if(valid) {
		debug(162, 9) ("mod_merge_request callback clientRedirectDone for get content-length\n");
                state->callback(state->http, NULL);//clientRedirectDone(http, NULL);
        }
        cbdataFree(state);
}

static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_merge_request config_sturct", sizeof(mod_config));	
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

static int checkStrIsDigit(char *pStr, int len)
{
        int i;  
        for (i = 0;i < len; i++)
        {       
                if (0 == isdigit(*(pStr+i)))
                {       
                        debug(162, 3)("mod_merge_request config parameter value is not digit.\n");
                        return -1;
                }       
        }       

        return 0;
}

static void clientBuildHttpRequestRange(clientHttpRequest * http)
{
	request_t *r = http->request;
	if (r->range) {
		httpHdrRangeDestroy(r->range);
	}
	r->range = NULL;
	r->range = httpHeaderGetRange(&r->header);

	if (r->range) 
		r->flags.range = 1;

	debug(162, 9)("mod_merge_request clientBuildHttpRequestRange http->reuqest->flags.range[%d],req->range[%p]\n",http->request->flags.range, http->request->range);
}

static int func_sys_parse_param(char *cfg_line)
{
	assert(cfg_line);
	debug(162, 3)("mod_merge_request config = [%s]\n",cfg_line);
	if (strstr(cfg_line, "allow") == NULL)
		return 0;
	if (strstr(cfg_line, "mod_merge_request") == NULL)
		return 0;

	char *token = NULL;
	if ((token = strtok(cfg_line, w_space)) == NULL)
		return 0;
	mod_config *cfg = mod_config_pool_alloc();
	
	token = strtok(NULL, w_space);
	strncpy(cfg->keyword, token, 126);
	strcat(cfg->keyword, "=");
	
	int proc_num = -1;
	token = strtok(NULL, w_space);
	if (checkStrIsDigit(token, strlen(token)) == 0) {
		proc_num = atoi(token);
		if (proc_num <= 0)
			debug(162, 0)("Error:mod_merge_request helper number =[%d],this number should > 0\n",proc_num);
		else
			helper_proc_num = proc_num;
	}
	debug(162, 3)("mod_merge_request config keyword = [%s],helper number =[%d]\n",cfg->keyword,proc_num);
	cc_register_mod_param(mod, cfg, free_cfg);
	
	return 0;
}

static int hook_cleanup(cc_module *module)
{
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}

static int free_private_data(void *data)
{
        request_param *req_param = data; 
        int i;
        for(i = 0;i<req_param->urls.count; i++) {       
                safe_free(req_param->urls.items[i]);
        }
        arrayClean(&req_param->urls);
        safe_free(req_param);
        return 0;
}

static void display_http_range(clientHttpRequest *http)
{
	request_t *req = http->request;	
	if(req == NULL){
                debug(162, 2)("mod_merge_request http request is null in display_http_range \n");
                return;
        }
        
	HttpHeaderEntry *range = NULL;
        range = httpHeaderFindEntry(&req->header, HDR_RANGE);
        if(range != NULL){
		debug(162, 2)("mod_merge_request display_http_range range->name.buf=[%s]\n",range->name.buf);
		debug(162, 2)("mod_merge_request display_http_range range->value.buf=[%s]\n",range->value.buf);
        }

	debug(162, 2)("mod_merge_request display_http_range http->reuqest->flags.range[%d],req->range[%p]\n",http->request->flags.range, http->request->range);
}

static void restore_http(clientHttpRequest *http)
{
	request_t *req = http->request;	
	if(req == NULL){
                debug(162, 2)("mod_merge_request restore_http http request is null in\n");
                return;
        }       
        
	debug(162, 2)("mod_merge_request before restore_http http->reuqest->flags.range[%d],req->range[%p]\n",http->request->flags.range,http->request->range);
	HttpHeaderEntry *range = NULL;
        range = httpHeaderFindEntry(&req->header, HDR_RANGE);
        if (range != NULL) {
		debug(162, 2)("mod_merge_request restore_http range->name.buf = %s\n",range->name.buf);
		debug(162, 2)("mod_merge_request restore_http range->value.buf = %s\n",range->value.buf);
        	httpHeaderDelById(&req->header, HDR_RANGE);
                debug(162, 2)("mod_merge_request restore_http del HDR_RANGE\n");
	}
	
	debug(162, 2)("mod_merge_request after restore_http http->reuqest->flags.range[%d],req->range[%p]\n",http->request->flags.range,http->request->range);
	return;
}

static void modify_request(char* result, clientHttpRequest *http)
{
	request_t *new_request = NULL;
	request_t *old_request = http->request;
	const char *urlgroup = http->conn->port->urlgroup;
	if (result)
	{
		debug(162,3)("mod_merge_request-->modify_request: the result is :\n%s\nold_request->method == %d\n",result, old_request->method);
		new_request = urlParse(old_request->method, result);
	}
	if (new_request)
	{
		safe_free(http->uri);
		http->uri = xstrdup(urlCanonical(new_request));
#ifdef CC_FRAMEWORK
		if(!http->log_uri)
#endif
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
		requestUnlink(old_request);
		http->request = requestLink(new_request);
	}
	else
	{
		/* Don't mess with urlgroup on internal request */
		if (old_request->flags.internal)
			urlgroup = NULL;
	}
	safe_free(http->request->urlgroup);		/* only paranoia. should not happen */
	if (urlgroup && *urlgroup)
		http->request->urlgroup = xstrdup(urlgroup);

}

/* follow format is validity
bytes=0-499
bytes=500-999
bytes=-500
bytes=500-
*/
static int checkRangeValidity(const char *src)
{
	assert(src);
	size_t          len;
        regex_t         re;
        regmatch_t      subs[SUBSLEN];
        char            errbuf[EBUFLEN];
        int             err;

        const char *pattern="(^([0-9])+-([0-9])+$|^-([0-9])+$|^([0-9])+-$)";
	debug(162, 4) ("mod_merge_request checkRangeValidity->range(%s)\n", src);
        debug(162, 4) ("mod_merge_request checkRangeValidity->regex Pattern: \"%s\"\n", pattern);

        err = regcomp(&re, pattern, REG_EXTENDED);

        if (err) { 
                len = regerror(err, &re, errbuf, sizeof(errbuf));
		debug(162, 1)("mod_merge_request checkRangeValidity error: regcomp: %s\n", errbuf);
		return -1;
        }

        err = regexec(&re, src, (size_t) SUBSLEN, subs, 0);

        if (err == REG_NOMATCH) {
                debug(162, 4) ("mod_merge_request checkRangeValidity  no match\n");
                regfree(&re);
                return -1;
        } else if (err) {
                len = regerror(err, &re, errbuf, sizeof(errbuf));
                debug(162, 4) ("mod_merge_request checkRangeValidity error: regexec: %s\n", errbuf);
                return -1;
        }

        debug(162, 4) ("mod_merge_request checkRangeValidity has matched ...\n\n");
        regfree(&re);
        return 0;
}

static void add_range(request_param *req_param, clientHttpRequest *http)
{
	assert(req_param);
	assert(http);
	assert(http->request);
	request_t *req = http->request;
	HttpHeaderEntry *range = NULL;
	range = httpHeaderFindEntry(&req->header, HDR_RANGE);
	if (!range) {
		req_param->orig_request_range = 0;
		debug(162, 1)("mod_merge_request add_range hasn't Range,just return,orig_request_range=[%d]\n",req_param->orig_request_range);
		return;
	}

	squid_off_t start_value = 0, end_value = 0; 

	start_value = atoll((range->value.buf) + 6); 
	
	if (start_value >=0) {
		char *end = NULL;
		end = strstr(range->value.buf+6, "-");
		if (end) {
			end_value = atoll(end+1);
			debug(162, 5) ("mod_merge_request before process abnormal add_range:start=%"PRINTF_OFF_T",end=%"PRINTF_OFF_T"\n",start_value,end_value);
			if (*(end+1) == '\0')
				req_param->end_exist = 0;//bytes=0- (should return all files)not bytes=0-0(only return 1 byte)
			else 
				req_param->end_exist = 1;

		}
	}
	debug(162, 5) ("mod_merge_request before checking add_range:start=%"PRINTF_OFF_T",end=%"PRINTF_OFF_T"\n",start_value,end_value);

	if (checkRangeValidity(range->value.buf+6) < 0) {
		debug(162, 1)("mod_merge_request checkRangeValidity failed,return all ts\n");
		start_value = 0, end_value = 0;
		return;
	}

	debug(162, 5) ("mod_merge_request after checking add_range:start = %"PRINTF_OFF_T",end = %"PRINTF_OFF_T"\n",start_value,end_value);
	//add_new
	req_param->start_value = start_value;
	req_param->end_value = end_value;

	if (start_value < 0) {
		/* FIX ME case3810: change abs to llabs(long long i) */
		if (llabs(start_value) <= req_param->normal_ts_len) {
			req_param->flags.send_ad_ts = 0;//don't send ad ts stream
			req_param->range_value.normal_range_start = start_value;
		}
		else if (llabs(start_value) > req_param->normal_ts_len) {
			if (llabs(start_value + req_param->normal_ts_len) < req_param->ad_ts_len)
				//this value is negative
				req_param->range_value.ad_range_start = start_value + req_param->normal_ts_len; 
			else
				req_param->range_value.ad_range_start = 0;
		}
	} else if (start_value >= 0) {
		if (start_value < req_param->ad_ts_len)
			req_param->range_value.ad_range_start = start_value;
		else if (start_value >= req_param->ad_ts_len) {
			//req_param->flags.send_ad_ts = 0;
			if (start_value < req_param->ad_ts_len + req_param->normal_ts_len) {
				req_param->range_value.normal_range_start = start_value - req_param->ad_ts_len;
				req_param->flags.send_ad_ts = 0;//-r 11- (11 >= ad_ts_len + normal_ts_len)
			}
			else
				req_param->range_value.normal_range_start = 0;
		}

		assert (end_value >= 0);
		if (end_value > 0 && end_value < start_value) {//deal it for: -r 10-1
			req_param->flags.send_ad_ts = 1;
			req_param->flags.send_normal_ts = 1;
			req_param->range_value.ad_range_start=0, req_param->range_value.ad_range_end=0;
			req_param->range_value.normal_range_start= 0, req_param->range_value.normal_range_end=0;
		} else if (end_value < req_param->ad_ts_len && end_value > 0) {
			req_param->flags.send_normal_ts = 0;//don't send normal ts stream
			req_param->range_value.ad_range_end = end_value;
		} else if (end_value >= req_param->ad_ts_len) {
			req_param->range_value.ad_range_end = 0;
			if (end_value >= req_param->ad_ts_len + req_param->normal_ts_len) {
				req_param->range_value.normal_range_end = 0;
			} else {
				req_param->range_value.normal_range_end = end_value - req_param->ad_ts_len;
			}
		} 
	}
	debug(162, 3) ("mod_merge_request after add_range modify req_param\n\t\t\t send_ad_ts=%d, send_normal_ts=%d \n\t\t\t ad_range_start=%"PRINTF_OFF_T", ad_range_end=%"PRINTF_OFF_T" \n\t\t\t normal_range_start=%"PRINTF_OFF_T", normal_range_end=%"PRINTF_OFF_T"\n",
			req_param->flags.send_ad_ts, req_param->flags.send_normal_ts,
			req_param->range_value.ad_range_start, req_param->range_value.ad_range_end,
			req_param->range_value.normal_range_start, req_param->range_value.normal_range_end);
}

static squid_off_t getHttpContentLen(request_param *req_param)
{
	assert(req_param);
	squid_off_t ad_len = 0, normal_len = 0;
	if (req_param->flags.send_ad_ts == 1) {//allow send ad
	   if (req_param->range_value.ad_range_start == 0) {
	      if (req_param->range_value.ad_range_end == 0) {
		 if (req_param->end_exist && req_param->start_value == 0 && req_param->end_value == 0)
			ad_len = 1; //-r 0-0
		 else
		 	ad_len += req_param->ad_ts_len - req_param->range_value.ad_range_start;
	    
	      } else
		 ad_len += req_param->range_value.ad_range_end - req_param->range_value.ad_range_start;
	   }
	   else if (req_param->range_value.ad_range_start > 0) {
	      if (req_param->range_value.ad_range_end == 0)
	         ad_len += req_param->ad_ts_len - req_param->range_value.ad_range_start;
	      else {
	         if(req_param->start_value !=0 && req_param->start_value == req_param->end_value)
			ad_len = 1; //fix -r 2-2, (if 2 less than ad_ts_len)
		 else
	         	ad_len += req_param->range_value.ad_range_end - req_param->range_value.ad_range_start;
	      }
	   } 
           else //deal start_value is negative
	      //ad_len += req_param->ad_ts_len - abs(req_param->range_value.ad_range_start);
	      ad_len += llabs(req_param->range_value.ad_range_start);
	}
	
	debug(162, 5) ("mod_merge_request getHttpContentLen->AD.len = %"PRINTF_OFF_T"\n", ad_len);
	if (req_param->flags.send_normal_ts == 1) {//allow send normal
	   if (req_param->range_value.normal_range_start == 0) {
		if (req_param->range_value.normal_range_end == 0) {
		   if (req_param->end_exist && req_param->start_value == 0 && req_param->end_value == 0)
			   normal_len = 0;//-r 0-0 
		   else if (req_param->start_value !=0 && req_param->start_value == req_param->end_value) {
			   if (req_param->start_value > req_param->ad_ts_len + req_param->normal_ts_len)
				normal_len = req_param->normal_ts_len; //-r 12-12(ad_len=7, normal_len=4)
			   else
			        normal_len = 1;// for -r 7-7
		   }
		   else if (req_param->end_value == req_param->ad_ts_len){//-r 1-7 (7 is equal ad_ts len)
			   normal_len = 1;
		   }
		   else {//for !range request
			   normal_len += req_param->normal_ts_len - req_param->range_value.normal_range_start;
                   }
		} else
	           normal_len += req_param->range_value.normal_range_end - req_param->range_value.normal_range_start;
	   }
	   else if (req_param->range_value.normal_range_start > 0) {
	      if (req_param->range_value.normal_range_end == 0)
		 normal_len += req_param->normal_ts_len - req_param->range_value.normal_range_start;
	      else { 
		 if(req_param->start_value !=0 && req_param->start_value == req_param->end_value)
			normal_len = 1; //fix -r 8-8 (if 8 more than ad_ts_len)
		 else
	         	normal_len += req_param->range_value.normal_range_end - req_param->range_value.normal_range_start;
	      }
	   }
           else //deal start_value is negative
	      //normal_len += req_param->normal_ts_len - abs(req_param->range_value.normal_range_start);
	      normal_len +=  llabs(req_param->range_value.normal_range_start);
	}
	
	debug(162, 5) ("mod_merge_request req_param->ad_ts_len = [%"PRINTF_OFF_T"],req_param_normal_ts_len = [%"PRINTF_OFF_T"]\n",req_param->ad_ts_len,req_param->normal_ts_len);
	debug(162, 5) ("mod_merge_request req_param->start_value = %"PRINTF_OFF_T",req_param->end_value = %"PRINTF_OFF_T"\n",req_param->start_value, req_param->end_value);
	//don't let range request actual content-length less then all_len
	if (req_param->range_value.ad_range_start >= 0 && req_param->range_value.normal_range_start >= 0)
	{
		//if ((ad_len + normal_len) < (req_param->ad_ts_len + req_param->normal_ts_len - 1))
		if ((ad_len + normal_len) <= (req_param->ad_ts_len + req_param->normal_ts_len - 1))//fix -r 1-10
		{	//fix -r start_value > ad_ts_len cause real_content_len plus 1
			//if(normal_len < req_param->normal_ts_len && req_param->start_value <= req_param->ad_ts_len) 
			if (normal_len < req_param->normal_ts_len) 
			{	if (req_param->start_value > req_param->ad_ts_len)
                                {
				   if (req_param->end_value > req_param->start_value) 
                                   {
				      if (req_param->end_value <= (req_param->ad_ts_len + req_param->normal_ts_len - 1))
					return (ad_len + normal_len + 1);
				      else
					return (ad_len + normal_len);
                                   }
				   else if (req_param->end_exist == 0) //for -r 8- cause plus 1
					return (ad_len + normal_len);
                                }
				else if (req_param->start_value <= req_param->ad_ts_len)
					(void) 0;

				if (req_param->end_exist && req_param->start_value == 0 && req_param->end_value == 0)
					return (ad_len + normal_len); //fix -r 0-0
				//fix -r 7-7 abnormal
				else if (req_param->start_value!=0 && req_param->start_value == req_param->end_value)
					return (ad_len + normal_len);
				else if (req_param->end_value == req_param->ad_ts_len)
					return (ad_len + normal_len);//-r 1-7 (7 is equal ad_ts len)
				else
					return (ad_len + normal_len + 1);//need plus 1 for : -r 1-100 len = (100-1) +1 = 100
			}
		}
		return (ad_len + normal_len);
	}
	else {
		return (ad_len + normal_len);	
	}
}

static void updateHttpRequestRange(clientHttpRequest *http, unsigned int is_send, squid_off_t start, squid_off_t end)
{
	debug(162, 3)("mod_merge_request update_HttpRequestRange start...\n");
	debug(162, 3)("mod_merge_request is_send=%d,start=%"PRINTF_OFF_T",end=%"PRINTF_OFF_T"\n",is_send, start,end);
	restore_http(http);//del original Range
	if (is_send == 0) {
		debug(162, 3)("mod_merge_request update_HttpRequestRange don't pack range\n");
		return;
	}

	debug(162, 3)("mod_merge_request update_HttpRequestRange pack range\n");
	char range_value[64];
	if (end)
		snprintf(range_value, 63, "bytes=%"PRINTF_OFF_T"-%"PRINTF_OFF_T"", start, end);
	else
	{
		if (start >= 0)
			snprintf(range_value, 63, "bytes=%"PRINTF_OFF_T"-", start);
		else
			snprintf(range_value, 63, "bytes=%"PRINTF_OFF_T"", start); //for -r -6
	}

	request_t *r = http->request;
	HttpHeaderEntry eEntry; 
	eEntry.id = HDR_RANGE;
	stringInit(&eEntry.name, "Range");
	stringInit(&eEntry.value, range_value);

	httpHeaderAddEntry(&r->header, httpHeaderEntryClone(&eEntry));
	stringClean(&eEntry.name);
	stringClean(&eEntry.value);

	display_http_range(http);
	debug(162, 3)("mod_merge_request update_HttpRequestRange ending...\n");
}


static void updateHttpResponseContentRange(request_param *r, clientHttpRequest *http, int has_range_header)
{
	assert(r);
	assert(http);
	HttpReply* rep = http->reply;
	assert(rep);
	HttpHeader *hdr = &rep->header;
	assert(hdr);
	
	HttpHdrContRange *content_range = NULL;
	
	if (has_range_header == 1) {
		content_range = httpHeaderGetContRange(hdr);
		assert(content_range);
		debug(162, 5) ("mod_merge_request before updateHttpResponseContentRange->ContRange=[%"PRINTF_OFF_T"]-[%"PRINTF_OFF_T"]/[%"PRINTF_OFF_T"]\n",
				content_range->spec.offset,//start_value
				content_range->spec.length + content_range->spec.offset - 1,//end_value
				content_range->elength);
	} else {
		debug(162, 5) ("mod_merge_request before updateHttpResponseContentRange has no ContRange header\n");
	}

	HttpHdrRangeSpec range_spec;
	squid_off_t g_start = 0, g_end = 0;
	if (1 == r->flags.send_ad_ts) {
		if (r->range_value.ad_range_start >=0) {
			g_start = r->range_value.ad_range_start;
		}
		else {//deal -r -1234 (start = ab_ts_len - 1234)
			g_start = r->ad_ts_len - llabs(r->range_value.ad_range_start); 
		}
		if (r->range_value.ad_range_end > 0) {
			g_end = r->range_value.ad_range_end;
		}
		else if (r->range_value.ad_range_end == 0) {
			g_end = r->ad_ts_len;
		}
	}
	//normal should modify start to range_value->normal_range_start
	if (1 == r->flags.send_normal_ts) {
		if (r->range_value.normal_range_start > 0)
			g_start = r->ad_ts_len + r->range_value.normal_range_start;
		else if (r->range_value.normal_range_start < 0)
			g_start = r->normal_ts_len - llabs(r->range_value.normal_range_start) + r->ad_ts_len;
		else if (r->range_value.normal_range_start == 0) {
			if (1 == r->flags.send_ad_ts) { //ad ts also need to be send
				if (r->range_value.ad_range_start == 0)
					(void) 0;
				else if (r->range_value.ad_range_start != 0)
					(void) 0;
			}
			else if (0 == r->flags.send_ad_ts) {
				g_start = r->ad_ts_len;
			}
		}
		
		if (r->range_value.normal_range_end > 0) {
			g_end = r->range_value.normal_range_end + r->ad_ts_len;
		} else if (r->range_value.normal_range_end == 0) {
			if (r->end_exist && r->start_value == 0 && r->end_value == 0) { // -r 0-0
				g_end = 0;	
			} else if (r->start_value !=0 && r->start_value == r->end_value) {
				if (r->start_value > r->ad_ts_len + r->normal_ts_len)
					g_end = r->ad_ts_len + r->normal_ts_len; 
				else
					g_end = g_start; //-r 7-7 Content-Range error(7 is equal to ad_ts_len)
			} else if (r->end_value == r->ad_ts_len)//-r 1-7 (7 is equal ad_ts len)
				g_end = r->ad_ts_len;
			else 
				g_end = r->ad_ts_len + r->normal_ts_len;
		}
	}

	debug(162, 5) ("mod_merge_request updateHttpResponseContentRange->g_start=[%"PRINTF_OFF_T"]-g_end=[%"PRINTF_OFF_T"]\n",g_start,g_end);

	range_spec.offset = g_start; 
	//range_spec.length = g_end;
	range_spec.length = g_end - g_start + 1;
	//for real content-len >= all_len,let real_content_len <= all_len
	//if ((range_spec.length + content_range->spec.offset - 1) >= (r->ad_ts_len + r->normal_ts_len))
	if (g_end  >= (r->ad_ts_len + r->normal_ts_len))
		range_spec.length -= 1;
		
	httpHeaderAddContRange(hdr, range_spec, r->ad_ts_len + r->normal_ts_len);

 	content_range = httpHeaderGetContRange(hdr);
	assert(content_range);
	debug(162, 5) ("mod_merge_request after updateHttpResponseContentRange->ContRange=[%"PRINTF_OFF_T"]-[%"PRINTF_OFF_T"]/[%"PRINTF_OFF_T"]\n",
			//content_range->spec.offset,//start_value
			g_start,
			//content_range->spec.length + content_range->spec.offset - 1,//end_value
			g_end,//end_value
			content_range->elength);
}

static int updateHttpReplyHeader(clientHttpRequest *http)
{
	debug(162, 5) ("mod_merge_request updateHttpReplyHeader start\n");
	
	char content_len[64];
	squid_off_t all_len = 0;

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	assert(req_param); 
	
	//if normal ts file's http status not 200.ok,do not change content-length
	if (req_param->normal_ts_len < 0) {
		debug(162, 0)("mod_merge_request  get normal ts file content-length less than 0,do not change content-length\n");
		return -1;
	}
	
	//if this url is the last one, do not modify content-length,just return; 
	if (req_param->current_url + 1 == (req_param->urls).count) {
		debug(162,3) ("mod_merge_request updateHttpReplyHeader return and do not modify content-length\n");
		return 0;
	}

	HttpReply* reply = http->reply;
        if (!reply) {
                debug(162, 3)("(mod_merge_request) ->request %s reply is NULL, return\n", http->uri);
                return 0;
        }
	if (http->reply->sline.status != HTTP_OK && http->reply->sline.status != HTTP_PARTIAL_CONTENT){
                debug(162, 3)("mod_merge_request request uri=[%s],http status=[%d]\n", http->uri,http->reply->sline.status);
		return 0;
	}

	HttpHeaderEntry *myheader;
        HttpHeaderPos pos = HttpHeaderInitPos;

	int  has_range_header = 0;//HDR_RANGE

        while ((myheader = httpHeaderGetEntry(&reply->header, &pos))) {
		if (myheader->id == HDR_CONTENT_RANGE) {
			has_range_header = 1;
		}
	}

	if (has_range_header == 0) {
		debug(162, 5) ("mod_merge_request no range ad_ts_len=[%"PRINTF_OFF_T"]\n",reply->content_length);
		req_param->ad_ts_len = reply->content_length;
	}

	if (req_param->orig_request_range == 1) {
		if (reply->sline.status == HTTP_OK)
			reply->sline.status = HTTP_PARTIAL_CONTENT;
	}
	else if (req_param->orig_request_range == 0) {
		if (reply->sline.status == HTTP_PARTIAL_CONTENT)
			reply->sline.status = HTTP_OK;
	}
	debug(162, 5) ("mod_merge_request updateHttpReplyHeader status=[%d],orig_request_range=[%d]\n",reply->sline.status,req_param->orig_request_range);

        pos = HttpHeaderInitPos;
        memset(content_len, 0, sizeof(content_len));

	//update Content-Length
        while ((myheader = httpHeaderGetEntry(&reply->header, &pos))) {
                if (myheader->id == HDR_CONTENT_LENGTH) {
			debug(162, 5)("mod_merge_request old Content-Length=[%s]\n",myheader->value.buf);
			all_len = getHttpContentLen(req_param);
			snprintf(content_len, 64, "%"PRINTF_OFF_T"", all_len);
			stringReset(&myheader->value, content_len);
			debug(162, 5)("mod_merge_request new Content-Length = [%s]\n",myheader->value.buf);
		}
	}

	updateHttpResponseContentRange(req_param, http,has_range_header);

	return 0;
}

static int http_repl_send_end(clientHttpRequest *http)
{
        request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http,mod);
	if (req_param->normal_ts_len < 0) {
		http->flags.done_copying = 1;
		debug(162,1)("mod_merge_request normal_ts_len =[%"PRINTF_OFF_T"]< 0 \n",req_param->normal_ts_len);
		return 0;
	}

	if ((http->reply->sline.status == HTTP_OK) || (http->reply->sline.status == HTTP_PARTIAL_CONTENT)) {
		; //nop
	} else {
		debug(162,1)("mod_merge_request normal ts file's http status=[%d], just return,do not createNewRequest\n", http->reply->sline.status);
		http->flags.done_copying = 1;
		return 0;
	}

        if ((req_param->urls).count > req_param->current_url + 1) {
                debug(162,3)("mod_merge_request set http->flags,done_copying =0;(req_param->urls).count = %d, req_param->current_url = %d\n",(req_param->urls).count, req_param->current_url);
                http->flags.done_copying = 0;
                storeClientUnregister(http->sc,http->entry,http);
                storeUnlockObject(http->entry);
                http->entry = NULL;
                http->sc = NULL;
		req_param->current_url++;
 		//for -r 7- (ad_len:7) (normal_len:4)
                if (http->reply) {
                        httpReplyDestroy(http->reply);
                        http->reply = NULL;
                }

                debug(162,3)("mod_merge_request  url[0] = (%s)\n",(char*)(req_param->urls).items[0]);

                modify_request((char*)(req_param->urls).items[0],http);
		//add normal_ts range header
		updateHttpRequestRange(http, req_param->flags.send_normal_ts, req_param->range_value.normal_range_start, req_param->range_value.normal_range_end);
		//add_second request->range
		clientBuildHttpRequestRange(http);
		
                clientProcessRequest(http);
                return 1;
        }
        else {
                http->flags.done_copying = 1;
        }
        return 0;
}

//0: send header and continue
//2: header already sent, just send body
static void private_repl_send_start(clientHttpRequest *http, int *ret)
{
        request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

        if (!req_param || req_param->urls.count == 0) {
                *ret = 0;
                return;
        }
        if (http->reply->sline.status != HTTP_OK && http->reply->sline.status != HTTP_PARTIAL_CONTENT) {
		*ret = 0;
		return;
        }

	if (req_param->current_url == 0) {
		*ret = 0;
		debug(162, 3) ("mod_merge_request private_repl_send_start req_param->current_url = 0\n");
		return;
	} else if (req_param->current_url == 1) {
		*ret = 2;
		debug(162, 3) ("mod_merge_request private_repl_send_start req_param->current_url = 1,do not send header\n");
		debug(162, 3) ("mod_merge_request ending.............\n");
		return;
	}
        *ret = 0;
	debug(162, 1) ("ERROR:mod_merge_request private_repl_send_start req_param->current_url = %d\n",req_param->current_url);
}

static int func_http_req_read(clientHttpRequest* http)
{
	assert(http);
	assert(http->request);
	assert(http->uri);
	
	int fd= http->conn->fd;

	mod_config *cfg = (mod_config *)cc_get_mod_param(fd, mod);
	if (!cfg) {
		debug(162, 1) ("mod_merge_request -> func_http_req_read cc_get_mod_param failed!\n");
		return 0;
	}
	debug(162, 9) ("mod_merge_request starting ........\n");
	debug(162, 9) ("mod_merge_request http->uri = [%s]\n",http->uri);

	request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if (!req_param) {
		req_param = xcalloc(1,sizeof(request_param));
		cc_register_mod_private_data(REQUEST_PRIVATE_DATA,http,req_param,free_private_data,mod);
		/*Range support :add flags.send_ad_ts, send_normal_ts,default send all*/
		req_param->flags.send_ad_ts = 1;
		req_param->flags.send_normal_ts = 1;

		req_param->range_value.ad_range_start = 0;
		req_param->range_value.ad_range_end = 0;

		req_param->range_value.normal_range_start = 0;
		req_param->range_value.normal_range_end = 0;
		req_param->is_send_body = 1;
		req_param->orig_request_range = 1; //default assume request has a range request
		req_param->end_exist = 0;
		debug(162, 3) ("mod_merge_request init req_param\n\t\t\t send_ad_ts=%d, send_normal_ts=%d \n\t\t\t ad_range_start=%"PRINTF_OFF_T", ad_range_end=%"PRINTF_OFF_T" \n\t\t\t normal_range_start=%"PRINTF_OFF_T", normal_range_end=%"PRINTF_OFF_T"\n",
			req_param->flags.send_ad_ts, req_param->flags.send_normal_ts,
			req_param->range_value.ad_range_start, req_param->range_value.ad_range_end,
			req_param->range_value.normal_range_start, req_param->range_value.normal_range_end);
	}

	assert(req_param);
	char *url = xstrdup(http->uri);
	
	char *pos = NULL;
	if ((pos = strstr(url, cfg->keyword)) == NULL) {
		debug(162, 1) ("mod_merge_request http->uri=[%s] missing [%s]\n",url,cfg->keyword);
		return 0;
	}
	*(pos-1)='\0';
	//case3974:some browser encode special separator to ascii,we need delete it
	//e.g.: "|" to "%7C"
	
	char *basename = strrchr(url,'/');
	if (basename != NULL) {
		basename++;
		char *basename_end = strstr(basename,".ts") + strlen(".ts");
		*basename_end = '\0';

		debug(162, 9) ("mod_merge_request  delete separator url = [%s],basename = [%s],end=[%s]\n",url,basename,basename_end);
	}

	arrayAppend(&req_param->urls,url);
	debug(162, 9) ("mod_merge_request array->url Normal Ts = [%s]\n",(char*)(req_param->urls).items[0]);

	//char *new_url = pos+strlen(cfg->keyword);
	char *new_url = xstrdup(pos+strlen(cfg->keyword));
	int i=0;
	while ((pos = strstr(new_url, cfg->keyword)) != NULL) {
		*(pos-1) = '\0';
		arrayAppend(&req_param->urls,new_url);
		i++;
		debug(162, 9) ("mod_merge_request array->url Ad ts[%d] = [%s]\n",i,(char*)(req_param->urls).items[i]);
		//new_url = pos+strlen(cfg->keyword);
		new_url = xstrdup(pos+strlen(cfg->keyword));
	}
	//del url last '?' or '&'
	char *mark1 = NULL, *mark2 = NULL;
	mark1 = strstr(new_url, "?");
	mark2 = strstr(new_url, "&");

	if (mark1 && mark2) {
		if (mark1 < mark2)
			*mark1 = '\0';
		else
			*mark2 = '\0';
	} else if (mark1 && !mark2) {
		*mark1 = '\0';
	} else if (!mark1 && mark2) {
		*mark2 = '\0';
	} 

	debug(162, 9) ("mod_merge_request new->url = [%s]\n",new_url);
	arrayAppend(&req_param->urls,new_url);

	modify_request((char*)((req_param->urls).items[req_param->urls.count - 1]),http);
	
	debug(162, 9) ("mod_merge_request after modify_request new http->uri = [%s]\n",http->uri);

	CBDATA_INIT_TYPE(prefetch_state_data);
	prefetch_state_data *state = cbdataAlloc(prefetch_state_data);
	state->http = http;
	state->callback = clientRedirectDone;

	cbdataLock(state);
	cbdataLock(state->http);
      	static char buf[4096];
	assert(req_param->urls.count - 2 >= 0);

        snprintf(buf, 4096, "%s", (char*)(req_param->urls).items[req_param->urls.count - 2]);//normal
        snprintf(buf+strlen(buf), 4096, " %s ", "CC_WASU_PREFETCH_TS");//delimiter
        snprintf(buf+strlen(buf), 4096, "%s\n", (char*)(req_param->urls).items[req_param->urls.count - 1]);//ad
        debug(162, 9)("mod_merge_request before helper_submit buf = [%s]\n", buf);
	helperSubmit(prefetchers, buf, prefetchHandleReply, state);
	return 1;
}

static void beforeClientProcessRequest2(clientHttpRequest*  http)
{
        debug(162, 9)("mod_merge_request beforeClientProcessRequest2\n");
	
	request_param* req = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
        assert(req);
	if (req->current_url > 0) {//only deal ad_ts
		debug(162, 9)("mod_merge_request beforeClientProcessRequest2 current_url=[%d]\n",req->current_url);
		return;
	}
		
	add_range(req, http);
	updateHttpRequestRange(http, req->flags.send_ad_ts, req->range_value.ad_range_start, req->range_value.ad_range_end);
	clientBuildHttpRequestRange(http);
}

static void parse_programline(wordlist ** line)
{
        if (*line)
                self_destruct();
        parse_wordlist(line);
}

static void hook_before_sys_init()
{
        debug(162,3)("mod_merge_request hook_before_sys_init start.\n");

        if (prefetchers) {
                helperShutdown(prefetchers);
                helperFree(prefetchers);
                prefetchers = NULL;
        }

        prefetchers = helperCreate("prefetcher");
        wordlist *pwl = NULL;
        char cfg_line2[128];
        strcpy(cfg_line2,"a /usr/local/squid/bin/content_len_prefetch.pl");
        strtok(cfg_line2, w_space);
        parse_programline(&pwl);

        prefetchers->cmdline = pwl;

        prefetchers->n_to_start = helper_proc_num;
        prefetchers->concurrency = 0;
        prefetchers->ipc_type = IPC_STREAM;
        helperOpenServers(prefetchers);
}

static int setSendBodyFlag(int fd, char *bufnotused, size_t size, int errflag, void *date)
{
	clientHttpRequest *http = date;
	debug(162, 3) ("mod_merge_request setSendBodyFlag start.\n");
        request_param *req = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	assert(req);	

	debug(162, 9) ("mod_merge_request setSendBodyFlag req->is_send_body = [%d]\n",req->is_send_body);
	if (req->is_send_body == 0)//unable send body
	{
		display_http_range(http);
		return 0;
	}
	
	debug(162, 9)("mod_merge_request setSendBodyFlag req->current_url = [%d],req->flags.send_ad_ts = [%d],req->flags.send_normal_ts=[%d]\n",req->current_url, req->flags.send_ad_ts,req->flags.send_normal_ts);
	if (req->current_url == 0) { //ad_ts
		if (req->flags.send_ad_ts == 0) {//do not send ad_ts body
			req->is_send_body = 0;
			http->flags.done_copying = 1;
			debug(162, 9) ("mod_merge_request setSendBodyFlag req->is_send_body = [%d]\n",req->is_send_body);
			return 0;
		}
	}

	return 0;
}

int mod_register(cc_module *module)
{
	debug(162, 3)("(mod_merge_request) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
			func_sys_parse_param);
	
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);
	//save ad url ,get the normal ts file content-length by helper
        cc_register_hook_handler(HPIDX_hook_func_private_http_before_redirect,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_before_redirect),
                        func_http_req_read);

	cc_register_hook_handler(HPIDX_hook_func_before_clientProcessRequest2,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_clientProcessRequest2), 
			beforeClientProcessRequest2);

        cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
                        hook_before_sys_init);

	//update content-length = len_of(ad_url) + len_of(normal_url)
        cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
                        updateHttpReplyHeader);

	//do not close fd and send next http->uri
        cc_register_hook_handler(HPIDX_hook_func_private_http_repl_send_end,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_repl_send_end), 
                        http_repl_send_end);
	
	//send Ad url header to client,and do not send normal ts file header
        cc_register_hook_handler(HPIDX_hook_func_private_repl_send_start,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_repl_send_start),
                        private_repl_send_start);

	//According to send_ad_ts or [send_normal_ts] judge whether send ad or [normal] body 
        cc_register_hook_handler(HPIDX_hook_func_private_client_write_complete,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_write_complete), 
                        setSendBodyFlag);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 1;
}

#endif

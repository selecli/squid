#include "squid.h"

#ifdef CC_FRAMEWORK

#define MAX_RANGE_NUM 256
#define BUF_SIZE_256  256
#define BUF_SIZE_1024 1024
#define BUF_SIZE_2048 2048
#define BUF_SIZE_4096 4096

//log info format: time requestedUri cdnIP clientIP "userAgent" statusCode event downloadTime bytesServed objectContentLength lastByteServed "Referer" domain
#define LOG_INFO_FMT "%9ld.%03ld %s %s %s \"%s\" %d %s %" PRINTF_OFF_T " %" PRINTF_OFF_T " %" PRINTF_OFF_T " %d \"%s\" %s %s\n"
#define RECEIPT_URL_FMT "%s?time=%9ld.%03ld&rcpt=%s&cdn=%s&requestedUrl=%s&cdnIP=%s&clientIP=%s&userAgent=\"%s\"&statusCode=%d&event=%s&downloadTime=%" PRINTF_OFF_T "&bytesServed=%" PRINTF_OFF_T "&objectContentLength=%" PRINTF_OFF_T "&lastByteServed=%d&Referer=\"%s\"&domain=%s"

typedef struct receipt_msg_st {
	char *uri;                         //request uri
	char *event;                       //download status[value: abort or success]
	char *cdn_ip;                      //CDN connect IP
	char *client_ip;                   //request client IP
	char *user_agent;                  //User-Agent
	char *refer;                       //Referer
	char *domain;                      //full domain name
	char *rcpt2;					   //add by cwg
	int range_to_end;                  //range to object end, like [Range: 100-]
	int range_is_complex;              //range is complex
	int last_byte_served;              //whether reponse the last byte of the object
	struct timeval start_time;
	http_status status_code;           //response status code
	squid_off_t used_time;             //download used time
	squid_off_t obj_cont_len;          //object content length
	squid_off_t bytes_served;          //reply/response content length, not contain header
	squid_off_t req_max_range_boundary;//max range boundary value, same as "Range: 0-1024,2048-10000", then 10000 is max range boundary value for req_max_range_boundary
} receipt_msg_t;

//async transfer
typedef struct _parserForwardRequest {
	int count;
	request_t *request;
	StoreEntry *entry;
	store_client *sc;
	squid_off_t offset;
	size_t buf_in_use;
	char readbuf[STORE_CLIENT_BUF_SZ];
	struct timeval start;
} parserForwardRequest;

typedef struct _parse_param_value_st {
	char url[BUF_SIZE_1024];
	char cdn[BUF_SIZE_256];
	char rcpt[BUF_SIZE_256];
	char rcpt2[BUF_SIZE_256];
	char logfile[BUF_SIZE_1024];
} parse_param_value_t;

typedef struct range_part_st {
	squid_off_t start;
	squid_off_t end;
	squid_off_t len;
	int end_to_lastbyte;
} range_part_t;

typedef struct range_array_st {
	int count;
	squid_off_t max_range_end;
	squid_off_t range_req_len;                 //request range total length
	range_part_t ranges[MAX_RANGE_NUM];
} range_array_t;

static int async_flag = 0;
static cc_module *mod = NULL;
static FILE *logfile_fp = NULL;
static MemPool *receipt_msg_pool = NULL;
static MemPool * mod_config_pool = NULL;
//static parse_param_value_t *params; 
static char receipt_url[BUF_SIZE_4096];
static range_array_t range_array;
//static squid_off_t multi_range_cont_len = 0;
static squid_off_t send_body_len = 0;
static char *log_path = "/data/proclog/log/squid/apple_receipt.log";

static int freeFdePrivateData(void *data);
static void parserHandleForwardReply(void *data, char *buf, ssize_t size);
static void obtainClientRequestMsg(clientHttpRequest *http , request_t *request);

static void *receiptMsgPoolAlloc(void)
{
	void *obj = NULL;

	if (NULL == receipt_msg_pool) 
	{
		receipt_msg_pool = memPoolCreate("mod_apple_receipt private_data receipt_msg", sizeof(receipt_msg_t));
	}

	return obj = memPoolAlloc(receipt_msg_pool);
}

//add by cwg for case4149 
static void * mod_config_pool_alloc(void)  
{   
	void * obj = NULL;                 
	if (NULL == mod_config_pool)       
		mod_config_pool = memPoolCreate("mod_apple_rectipt config_struct", sizeof(parse_param_value_t)); 
	return obj = memPoolAlloc(mod_config_pool);
}

//add by cwg for case4149  
static int free_cfg(void * data)        
{   
	parse_param_value_t *params = data;
	if (NULL != params)                    
	{
		memPoolFree(mod_config_pool, params);       
		params = NULL;
	}
	return 0;
}

static int funcSysInit(void)
{
	char *ptr;
	char cmd[BUF_SIZE_1024];
	char path[BUF_SIZE_1024];

	//ptr = strrchr(params.logfile, '/');
	ptr = strrchr(log_path, '/');
	if (NULL == ptr) 
	{
		debug(223, 3)("mod_apple_receipt: configure line error for log file path\n");
		return -1;
	}
	//strncpy(path, params.logfile, ptr - params.logfile);
	strncpy(path, log_path, ptr - log_path);
	//if the directory not exist, create it
	if (access(path, F_OK) < 0) 
	{
		snprintf(cmd, BUF_SIZE_1024, "mkdir -p %s", path);
		system(cmd);
		snprintf(cmd, BUF_SIZE_1024, "chown -R squid:squid %s", path);
		system(cmd);
		snprintf(cmd, BUF_SIZE_1024, "chmod 755 %s", path);
		system(cmd);
	}   
	//open log file for writing
	//logfile_fp = fopen(params.logfile, "a"); 
	logfile_fp = fopen(log_path, "a"); 
	if (NULL == logfile_fp) 
	{
		//debug(223, 5)("mod_apple_receipt: open log file failed[%s]\n", params.logfile);
		debug(223, 5)("mod_apple_receipt: open log file failed[%s]\n", log_path);
		return -1;  
	} 

	return 0;
}

static int receiptMsgInit(clientHttpRequest *http)
{
	int fd;
	fde *F = NULL;
	receipt_msg_t *fdemsg = NULL;

	fd = http->conn->fd;
	F = &fd_table[fd];
	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	if(NULL == fdemsg)
	{
		fdemsg = receiptMsgPoolAlloc();
		fdemsg->uri = NULL;
		fdemsg->event = NULL;
		fdemsg->cdn_ip = NULL;
		fdemsg->client_ip = NULL;
		fdemsg->user_agent = NULL;
		fdemsg->refer = NULL;
		fdemsg->domain = NULL;
		fdemsg->used_time = 0;
		fdemsg->obj_cont_len = 0;
		fdemsg->bytes_served = 0;
		fdemsg->range_to_end = 0;
		fdemsg->last_byte_served = 0;
		fdemsg->range_is_complex = 0;
		fdemsg->req_max_range_boundary = 0;
		fdemsg->status_code = HTTP_STATUS_NONE;
		memset(&fdemsg->start_time, 0, sizeof(fdemsg->start_time));
		cc_register_mod_private_data(FDE_PRIVATE_DATA, &fd, fdemsg, freeFdePrivateData, mod);
	}    

	obtainClientRequestMsg(http, http->request);

	return 0; 
}

/* configure module: 
 * comm : mod_apple_receipt  sync  rcpt1 allow apple_test_url
 * async: mod_apple_receipt  async http://www.apple.com/index.html swcdn ccih allow apple_test_url
 */
static int funcSysParseParam(char *cfg_line)
{
	char *token = NULL;
	char *errstr = NULL;
	int size = 0;
	parse_param_value_t *params = NULL;

	assert(cfg_line);
	//get mod_apple_receipt
	token = strtok(cfg_line, w_space);
	if (NULL == token) 
	{
		errstr = "parsing module name 'mod_apple_receipt' failed";
		goto err;
	}
	if(strcmp(token, "mod_apple_receipt")) 
	{
		errstr = "module name is not 'mod_apple_receipt'";
		goto err;
	}
	//need malloc room
	params = (parse_param_value_t *)mod_config_pool_alloc();

	//get log file path
	/*token = strtok(NULL, w_space);
	if (NULL == token) 
	{
		errstr = "parsing log file path failed";
		goto err;
	}
	int size = sizeof(params.logfile);
	strncpy(params.logfile, token, size - 1);
	params.logfile[size - 1] = '\0';*/
	
	//judge sync or async
	token = strtok(NULL, w_space);
	if (NULL == token)
	{
		errstr = "parsing sync or async if failed\n";
		goto err;
	}
	if (strcmp(token, "sync") == 0)
	{
		//get rcpt2
		token = strtok(NULL, w_space);
		if (NULL == token) 
		{
			errstr = "parsing the kind of rcpt2 is failed\n";
			goto err;
		}
		//size = sizeof(params.rcpt2);
		//strncpy(params.rcpt2, token, size - 1);
		//params.rcpt2[size - 1] = '\0';
		size = sizeof(params->rcpt2);
		strncpy(params->rcpt2, token, size - 1);
		params->rcpt2[size - 1] = '\0';
		//get allow or deny
		token = strtok(NULL, w_space);
		if (NULL == token)
		{
			errstr = "parsing 'deny' or 'allow' failed\n";
			goto err; 
		}
		if (strcmp(token, "allow") && strcmp(token, "deny"))
		{
			errstr = "is not 'allow' or 'deny'"; 
			goto err;
		}
		debug(223,1)("config is %s\n",params->rcpt2);
		cc_register_mod_param(mod, params, free_cfg);
		return 0;
	}
/*	if (!strcmp(token, "allow") || !strcmp(token, "deny")) 
	{
		//debug(223,1)("config is %s %s\n",params.logfile, params.rcpt2);
		//cc_register_mod_param(mod, params, free_cfg);
		debug(223,1)("config is %s %s\n",params->logfile, params->rcpt2);
		cc_register_mod_param(mod, params, free_cfg);
		return 0;
	}
	//if async get it
	if (strcmp(token, "async")) 
	{
		errstr = "parsing 'async' failed";
		goto err;
	}*/

	else if (strcmp(token, "async") == 0)
	{
		async_flag = 1;
		//get url
		token = strtok(NULL, w_space);
		if (NULL == token) 
		{
			errstr = "parsing 'url' failed";
			goto err;
		}
		//size = sizeof(params.url);
		//strncpy(params.url, token, size - 1);
		//params.url[size - 1] = '\0';
		size = sizeof(params->url);
		strncpy(params->url, token, size - 1);
		params->url[size - 1] = '\0';
		//get rcpt
		token = strtok(NULL, w_space);
		if (NULL == token) 
		{
			errstr = "parsing 'rcpt' failed";
			goto err;
		}
		//size = sizeof(params.rcpt);
		//strncpy(params.rcpt, token, size - 1);
		//params.rcpt[size - 1] = '\0';
		size = sizeof(params->rcpt);
		strncpy(params->rcpt, token, size - 1);
		params->rcpt[size - 1] = '\0';
		//get cdn 
		token = strtok(NULL, w_space);
		if (NULL == token) 
		{
			errstr = "parsing 'cdn' failed";
			goto err;
		}
		//size = sizeof(params.cdn);
		//strncpy(params.cdn, token, size - 1);
		//params.cdn[size - 1] = '\0';
		size = sizeof(params->cdn);
		strncpy(params->cdn, token, size - 1);
		params->cdn[size - 1] = '\0';
		//get allow or deny
		token = strtok(NULL, w_space);
		if (NULL == token) 
		{
			errstr = "parsing 'allow' or 'deny' failed";
			goto err;
		}
		if (strcmp(token, "allow") && strcmp(token, "deny")) 
		{
			errstr = "is not 'allow' or 'deny'";
			goto err;
		}
		cc_register_mod_param(mod, params, free_cfg); 
		return 0;
	}
	else
	{
		errstr = "is not 'sync' or 'async'";
		goto err;
	}
err:
	//add by cwg for case4149  
	if (params != NULL)
	{
		free_cfg(params);
	}
	debug(223, 3)("mod_apple_receipt: configure line error[%s]\n", errstr);
	return -1;
}

static int freeFdePrivateData(void *data)
{
	receipt_msg_t *fdemsg = (receipt_msg_t *)data;

	if(NULL != fdemsg)
	{
		safe_free(fdemsg->uri);
		safe_free(fdemsg->event);
		safe_free(fdemsg->cdn_ip);
		safe_free(fdemsg->client_ip);
		safe_free(fdemsg->user_agent);
		safe_free(fdemsg->refer);
		safe_free(fdemsg->domain);
		memPoolFree(receipt_msg_pool, fdemsg);
		fdemsg = NULL;
	}

	return 0;
}

static int funcSysCleanup(cc_module *mod)
{
	if (NULL != logfile_fp) 
	{
		fclose(logfile_fp);
	}
	return 0;
}

//obtain client request message
static void obtainClientRequestMsg(clientHttpRequest *http , request_t *request)
{
	int fd;
	char tmp[BUF_SIZE_2048];
	char end[BUF_SIZE_2048];
	char start[BUF_SIZE_2048];
	char range_buf[BUF_SIZE_4096];
	char *p = NULL;
	char *ptr = NULL;
	fde *F = NULL;
	http_hdr_type id;
	receipt_msg_t *fdemsg = NULL;
	HttpHeaderEntry *findname = NULL;

	fd = http->conn->fd;
	F = &fd_table[fd];
	if(1 == F->cctype) //fd is for server, return here
	{
		debug(223, 3)("mod_apple_receipt: it's server, close %d fd  cctype == 1\n", fd);
		return;
	}
	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	if(NULL == fdemsg) 
	{
		debug(223, 3)("mod_apple_receipt: cc_get_mod_private_data is NULL\n" );
		return;
	}
	//get request start time[seconds]
	memcpy(&fdemsg->start_time, &http->start, sizeof(struct timeval));
	//get 'User-Agent' value
	id = httpHeaderIdByNameDef("User-Agent", strlen("User-Agent"));
	findname = httpHeaderFindEntry(&request->header, id);
	fdemsg->user_agent = (NULL != findname) ? xstrdup(findname->value.buf) : xstrdup("-");
	if (NULL == fdemsg->user_agent)
	{
		fdemsg->user_agent = xstrdup("-");
	}
	else
	{
		if ('\0' == fdemsg->user_agent[0])
		{
			safe_free(fdemsg->user_agent);
			fdemsg->user_agent = xstrdup("-");
		}
	}
	//get 'Referer' value
	id = httpHeaderIdByNameDef("Referer", strlen("Referer"));
	findname = httpHeaderFindEntry(&request->header, id);
	fdemsg->refer = (NULL != findname) ? xstrdup(findname->value.buf) : xstrdup("-");
	if (NULL == fdemsg->refer)
	{
		fdemsg->refer = xstrdup("-");
	}
	else
	{
		if ('\0' == fdemsg->refer[0])
		{
			safe_free(fdemsg->refer);
			fdemsg->refer = xstrdup("-");
		}
	}
	//get 'Range' value, may be more than one 'Range'
	id = httpHeaderIdByNameDef("Range", strlen("Range"));
	findname = httpHeaderFindEntry(&request->header, id);
	if (NULL != findname) //have "Range" header
	{  
		memset(&range_array, 0, sizeof(range_array));
		memset(range_buf, 0, BUF_SIZE_4096);
		p = strchr(findname->value.buf, '=');
		if (NULL == p)
		{
			strncpy(range_buf, findname->value.buf, BUF_SIZE_4096 - 1);
			range_buf[BUF_SIZE_4096 - 1] = '\0';
		}
		else
		{
			strncpy(range_buf, p + 1, BUF_SIZE_4096 - 1);
			range_buf[BUF_SIZE_4096 - 1] = '\0';
		}
		ptr = strtok(range_buf, ",");
		while (NULL != ptr)
		{
			if (range_array.count >= MAX_RANGE_NUM) 
				break;
			memset(tmp, 0, BUF_SIZE_2048);
			strncpy(tmp, ptr, BUF_SIZE_2048 - 1);
			tmp[BUF_SIZE_2048 - 1] = '\0';
			p = strchr(tmp, '-');
			if (NULL == p) 
			{
				debug(223, 3)("mod_apple_receipt: range[%s] have no '-'\n", tmp);
				ptr = strtok(NULL, ",");
				continue;
			}
			memset(start, 0, BUF_SIZE_2048);
			int length = p - tmp;
			if (length >= BUF_SIZE_2048)
			{
				strncpy(start, tmp, BUF_SIZE_2048 - 1);
				start[BUF_SIZE_2048 - 1] = '\0';
			}
			else
			{
				strncpy(start, tmp, length);
			}
			range_array.ranges[range_array.count].start = atoll(start);
			debug(223, 3)("mod_apple_receipt: start[%s], end[%s]\n", start, p + 1);
			if ('\0' == *(p + 1))
			{
				range_array.ranges[range_array.count].end_to_lastbyte = 1;
				range_array.count++;
				ptr = strtok(NULL, ",");
				continue;
			}
			memset(end, 0, BUF_SIZE_2048);
			strncpy(end, p + 1, BUF_SIZE_2048 - 1);
			end[BUF_SIZE_2048 - 1] = '\0';
			range_array.ranges[range_array.count].end = atoll(end);
			range_array.count++;
			ptr = strtok(NULL, ",");
		}
	}
}

CBDATA_TYPE(parserForwardRequest);

static void parserForwardDone(parserForwardRequest * fwdreq)
{
	char *url;
	AccessLogEntry al;
	static aclCheck_t *ch;
	MemObject *mem = fwdreq->entry->mem_obj;
	request_t *request = fwdreq->request;
	if (
			200 != mem->reply->sline.status &&  
			204 != mem->reply->sline.status &&
			206 != mem->reply->sline.status
	   )   
	{   
		if (fwdreq->count < 3)  
		{   
			debug(223, 3)("mod_apple_receipt: async tansfer failed %d times\n", fwdreq->count + 1);
			fwdreq->count++;
			fwdreq->start = current_time;
			fwdreq->request->flags.cachable = 0;
			storeUnlockObject(fwdreq->entry);
			url = xstrdup(urlCanonical(fwdreq->request));
			fwdreq->entry = storeCreateEntry(url,
					fwdreq->request->flags,
					fwdreq->request->method);
			fwdreq->sc = storeClientRegister(fwdreq->entry, fwdreq);
			fwdreq->request->etags = NULL;
			fwdreq->request->lastmod = -1; 
			fwdreq->offset = 0;
			fwdStart(-1, fwdreq->entry, fwdreq->request);
			storeClientCopy(fwdreq->sc, fwdreq->entry,
					fwdreq->offset,
					fwdreq->offset,
					STORE_CLIENT_BUF_SZ, fwdreq->readbuf,
					parserHandleForwardReply,
					fwdreq);
			return;
		}
		debug(223, 3)("mod_apple_receipt: async tansfer failed 3 times, URL[%s]\n", mem->url);
	}
	memset(&al, 0, sizeof(al));
	al.icp.opcode = ICP_INVALID;
	al.url = mem->url;
	debug(223, 3) ("parserForwardDone: url='%s'\n", al.url);
	al.http.code = mem->reply->sline.status;
	al.http.content_type = strBuf(mem->reply->content_type);
	al.cache.size = fwdreq->offset;
	al.cache.code = LOG_TCP_ASYNC_MISS;
	al.cache.msec = tvSubMsec(fwdreq->start, current_time);
	if (Config.onoff.log_mime_hdrs)
	{
		Packer p;
		MemBuf mb;
		memBufDefInit(&mb);
		packerToMemInit(&p, &mb);
		httpHeaderPackInto(&request->header, &p);
		al.headers.request = xstrdup(mb.buf);
		packerClean(&p);
		memBufClean(&mb);
	}
	al.http.method = request->method;
	al.http.version = request->http_ver;
	al.hier = request->hier;
	if (request->auth_user_request)
	{
		if (authenticateUserRequestUsername(request->auth_user_request))
			al.cache.authuser = xstrdup(authenticateUserRequestUsername(request->auth_user_request));
		authenticateAuthUserRequestUnlock(request->auth_user_request);
		request->auth_user_request = NULL;
	}
	else if (request->extacl_user)
	{
		al.cache.authuser = xstrdup(request->extacl_user);
	}
	al.request = request;
	al.reply = mem->reply;
	ch = aclChecklistCreate(Config.accessList.http, request, NULL);
	ch->reply = mem->reply;
	if (!Config.accessList.log || aclCheckFast(Config.accessList.log, ch))
		accessLogLog(&al, ch);
	aclChecklistFree(ch);
	storeClientUnregister(fwdreq->sc, fwdreq->entry, fwdreq);
	storeUnlockObject(fwdreq->entry);
	requestUnlink(fwdreq->request);
	safe_free(al.headers.request);
	safe_free(al.headers.reply);
	safe_free(al.cache.authuser);
	cbdataFree(fwdreq);
}

static void parserHandleForwardReply(void *data, char *buf, ssize_t size)
{
	parserForwardRequest *fwdreq = data;
	StoreEntry *e = fwdreq->entry;
	if (EBIT_TEST(e->flags, ENTRY_ABORTED))
	{   
		parserForwardDone(fwdreq);
		return;
	}   
	if (size <= 0)
	{   
		parserForwardDone(fwdreq);
		return;
	}   
	fwdreq->offset += size;
	if (e->mem_obj->reply->sline.status == 304)
	{
		parserForwardDone(fwdreq);
		return;
	}
	storeClientCopy(fwdreq->sc, fwdreq->entry,
			fwdreq->offset,
			fwdreq->offset,
			STORE_CLIENT_BUF_SZ, fwdreq->readbuf,
			parserHandleForwardReply,
			fwdreq);
	debug(223, 3)("mod_apple_receipt: readbuf[%s]\n", fwdreq->readbuf);
}

void parserForwardStart(request_t * req)
{
	char *url = xstrdup(urlCanonical(req));
	parserForwardRequest *fwdreq;
	request_t *request = req;
	CBDATA_INIT_TYPE(parserForwardRequest);
	fwdreq = cbdataAlloc(parserForwardRequest);
	fwdreq->start = current_time;
	fwdreq->request = requestLink(request);
	request->flags.cachable = 0;
	fwdreq->entry = storeCreateEntry(url,
			request->flags,
			request->method);
	fwdreq->sc = storeClientRegister(fwdreq->entry, fwdreq);
	request->etags = NULL;  // Should always be null as this was a cache hit, but just in case..
	request->lastmod = -1;
	fwdStart(-1, fwdreq->entry, fwdreq->request);
	storeClientCopy(fwdreq->sc, fwdreq->entry,
			fwdreq->offset,
			fwdreq->offset,
			STORE_CLIENT_BUF_SZ, fwdreq->readbuf,
			parserHandleForwardReply,
			fwdreq);
}

static HttpHeaderEntry *findHttpReplyHeaderEntry(const HttpHeader *hdr, http_hdr_type id)
{
	HttpHeaderEntry *hdr_entry;
	HttpHeaderPos pos = HttpHeaderInitPos;

	assert(hdr);
	if (!CBIT_TEST(hdr->mask, id)) 
		return NULL;

	while (NULL != (hdr_entry = httpHeaderGetEntry(hdr, &pos))) 
	{
		if (id == hdr_entry->id) 
			return hdr_entry;
	}
	assert(0);

	return NULL;
}

static void obtainReplyToClientMsg(clientHttpRequest *http, HttpReply *rep)
{
	int fd;
	int len; 
	char *tmp = NULL;
	http_hdr_type id;
	receipt_msg_t *fdemsg = NULL;
	HttpHeaderEntry *hdr_entry = NULL;

	if (
			LOG_TCP_HIT == http->log_type ||
			LOG_TCP_REFRESH_HIT == http->log_type ||
			LOG_TCP_REFRESH_FAIL_HIT == http->log_type ||
			LOG_TCP_IMS_HIT == http->log_type ||
			LOG_TCP_NEGATIVE_HIT == http->log_type ||
			LOG_TCP_MEM_HIT == http->log_type ||
			LOG_TCP_OFFLINE_HIT == http->log_type ||
			LOG_TCP_STALE_HIT == http->log_type ||
			LOG_TCP_ASYNC_HIT == http->log_type ||
			LOG_UDP_HIT == http->log_type
	   ) 
	{
		return;
	}
	fd = http->conn->fd;
	fde *F = &fd_table[fd]; 
	//cctype: 0:client->FC; 1:FC->server
	if(1 == F->cctype) 
	{
		debug(223, 3)("mod_apple_receipt: close fd [%d] , because 'cctype is 1[for server's fd]'\n", fd);
		return;
	}
	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
	if(NULL == fdemsg) 
	{
		debug(223, 3)("mod_apple_receipt: no private data\n");
		return;
	}  
	if (http->request->flags.range) 
	{	
		len = strlen("Content-Range");
		id = httpHeaderIdByNameDef("Content-Range", len);
		hdr_entry = NULL;
		hdr_entry = findHttpReplyHeaderEntry(&rep->header, id);
		if (NULL == hdr_entry && http->request->range != NULL && http->request->range->specs.count > 1) 
		{
			fdemsg->obj_cont_len = http->entry->mem_obj->reply->content_length;
		}
		else if(hdr_entry != NULL)
		{
			if(hdr_entry->value.size > 0)
			{
				tmp = strstr(hdr_entry->value.buf, "/") + 1;
				fdemsg->obj_cont_len = atoll(tmp);
			}
			else
			{
				fdemsg->obj_cont_len = 0;
			}
		}
		else
		{
			fdemsg->obj_cont_len = 0;
		}
		//if out-of-range and Content-Range is 0, use Content-Length
		if (fdemsg->obj_cont_len == 0)
		{
			len = strlen("Content-Length");
			id = httpHeaderIdByNameDef("Content-Length", len);
			hdr_entry = NULL;
			hdr_entry = findHttpReplyHeaderEntry(&rep->header, id);
			if (hdr_entry != NULL)   
				fdemsg->obj_cont_len = atoll(hdr_entry->value.buf);
		}
	} 
	else 
	{
		len = strlen("Content-Length");
		id = httpHeaderIdByNameDef("Content-Length", len);
		hdr_entry = NULL;
		hdr_entry = findHttpReplyHeaderEntry(&rep->header, id);
		if (NULL == hdr_entry)   
			fdemsg->obj_cont_len = -1;
		else
			fdemsg->obj_cont_len = atoll(hdr_entry->value.buf);
	}
}

static void getRangeReqLen(squid_off_t obj_len)
{
	int i;

	range_array.max_range_end = -1;

	for (i = 0; i < range_array.count; ++i)
	{	
		if (range_array.ranges[i].start > obj_len - 1)
			range_array.ranges[i].start = 0;
		if (range_array.ranges[i].end_to_lastbyte)
			range_array.ranges[i].end = obj_len - 1;
		if (range_array.ranges[i].end > obj_len -1)
			range_array.ranges[i].end = obj_len - 1;
		range_array.ranges[i].len = range_array.ranges[i].end - range_array.ranges[i].start + 1;
		range_array.range_req_len += range_array.ranges[i].len;
		debug(223, 3)("mod_apple_receipt: range_req_len[%" PRINTF_OFF_T "], obj_len[%" PRINTF_OFF_T "], start[%" PRINTF_OFF_T "], end[%" PRINTF_OFF_T "]\n", range_array.range_req_len, obj_len, range_array.ranges[i].start, range_array.ranges[i].end);
		if (range_array.max_range_end < range_array.ranges[i].end)
			range_array.max_range_end = range_array.ranges[i].end;	
	}
}

static void getMultiRangeCopySize(clientHttpRequest *http,size_t copy_sz)
{
	debug(223, 3)("mod_apple_receipt: copy_sz is %ld\n", (long)copy_sz);

	if( http!= NULL && http->request != NULL && http->request->range != NULL &&http->request->range->specs.count >= 1)
	{
		receipt_msg_t *fdemsg = NULL;
		int	fd = http->conn->fd;
		fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
		if(fdemsg)
		{
			fdemsg->bytes_served += copy_sz;
		}
	}
}

static void getSendBodySize(clientHttpRequest *http, size_t send_len)
{
	debug(223, 3)("mod_apple_receipt: send_len is %ld\n", (long)send_len);

	if((http != NULL && http->request != NULL && http->request->range == NULL))
	{
		receipt_msg_t *fdemsg = NULL;
		int	fd = http->conn->fd;
		fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);
		if(fdemsg)
		{
			fdemsg->bytes_served += send_len;
		}
	}
}


static int handleReceiptLog(clientHttpRequest *http)
{
	int fd;
	size_t n;
	char *ptr, *ptr1;
	fde *F = NULL;

	parse_param_value_t *params = NULL;

	AccessLogEntry *al = NULL;
	receipt_msg_t *fdemsg = NULL;

	fd = http->conn->fd;
	F = &fd_table[fd];
	fdemsg = cc_get_mod_private_data(FDE_PRIVATE_DATA, &fd, mod);

	//add by cwg
	params = (parse_param_value_t *)cc_get_mod_param(fd, mod);

	if (NULL == fdemsg)
	{
		debug(223, 3)("mod_apple_receipt: fdemsg if 'NULL'\n");
		return 0;
	}
	al = &http->al;
	if (NULL == al) 
	{
		debug(223, 3)("mod_apple_receipt: &http->al if 'NULL'\n");
		return 0;
	}
	//parse for request uri and full domain name
	ptr = strstr(al->url, "://");
	if (NULL != ptr) 
	{
		ptr += 3;
		if ('\0' != *ptr)
		{
			ptr1 = strchr(ptr, '/');
			if (NULL != ptr1) 
			{
				fdemsg->uri = xstrdup(ptr1);
				n = ptr1 - ptr;
				fdemsg->domain = xcalloc(n + 1, 1);
				strncpy(fdemsg->domain, ptr, n);
				fdemsg->domain[n] = '\0';
			}
			else
			{
				fdemsg->domain = xstrdup(ptr);
			}
		}
	} 
	if (NULL == fdemsg->uri || '\0' == fdemsg->uri[0]) 
		fdemsg->uri = xstrdup("-");
	if (NULL == fdemsg->domain || '\0' == fdemsg->domain[0]) 
		fdemsg->domain = xstrdup("-");
	//CDN IP
	fdemsg->cdn_ip = xstrdup(inet_ntoa(http->conn->me.sin_addr));
	if (NULL == fdemsg->cdn_ip)
	{
		fdemsg->cdn_ip = xstrdup("-");
	}
	else
	{
		if ('\0' == fdemsg->cdn_ip[0])
		{
			safe_free(fdemsg->cdn_ip);
			fdemsg->cdn_ip = xstrdup("-");
		}
	}
	//client IP
	fdemsg->client_ip = xstrdup(inet_ntoa(al->cache.caddr));
	if (NULL == fdemsg->client_ip)
	{
		fdemsg->client_ip = xstrdup("-");
	}
	else
	{
		if ('\0' == fdemsg->client_ip[0])
		{
			safe_free(fdemsg->client_ip);
			fdemsg->client_ip = xstrdup("-");
		}
	}
	//response content length
	send_body_len = 0;
	//response code, like 200, 206 ...
	fdemsg->status_code = al->http.code;
	//download used time, figure in microsecond
	fdemsg->used_time = al->cache.msec;

	if ( 
			LOG_TCP_HIT == http->log_type ||
			LOG_TCP_REFRESH_HIT == http->log_type ||
			LOG_TCP_REFRESH_FAIL_HIT == http->log_type ||
			LOG_TCP_IMS_HIT == http->log_type ||
			LOG_TCP_NEGATIVE_HIT == http->log_type ||
			LOG_TCP_MEM_HIT == http->log_type ||
			LOG_TCP_OFFLINE_HIT == http->log_type ||
			LOG_TCP_STALE_HIT == http->log_type ||
			LOG_TCP_ASYNC_HIT == http->log_type ||
			LOG_UDP_HIT == http->log_type
	   ) 
	{
		fdemsg->obj_cont_len = contentLen(http->entry);
	}
	if (200 != fdemsg->status_code && 206 != fdemsg->status_code) 
	{
		fdemsg->event = xstrdup("success");
		fdemsg->bytes_served = 0;
		fdemsg->last_byte_served = 0;
		if (HTTP_STATUS_NONE == fdemsg->status_code) 
			fdemsg->obj_cont_len = -1;
		else
			fdemsg->obj_cont_len = 0;

		debug(223, 3)("mod_apple_receipt: response code is not '200' or '206', code is %d\n", fdemsg->status_code);
		//return 0;
	}
	else
	{
		//only transfer 200 or 206 info to apple server
		if (al->request->flags.range) //http->request->range->specs.count > 1) 
		{
			getRangeReqLen(fdemsg->obj_cont_len);
		}
		if (al->request->flags.range)
			debug(223, 3)("mod_apple_receipt: range_array.max_range_end[%" PRINTF_OFF_T "]\n", range_array.max_range_end);
		fdemsg->last_byte_served = al->request->flags.range ? (range_array.max_range_end >= fdemsg->obj_cont_len - 1 && fdemsg->bytes_served >= range_array.range_req_len) : (fdemsg->bytes_served >= fdemsg->obj_cont_len);
		if (200 == fdemsg->status_code)
			fdemsg->event = fdemsg->last_byte_served ? xstrdup("success") : xstrdup("abort");
		if (206 == fdemsg->status_code)
			fdemsg->event = (fdemsg->bytes_served >= range_array.range_req_len) ? xstrdup("success") : xstrdup("abort");
	}
	//write log information: cdn IP or client IP is '127.0.0.1', no write log file
	if (!strcmp(fdemsg->cdn_ip, "127.0.0.1") || !strcmp(fdemsg->client_ip, "127.0.0.1")) 
	{
		debug(223, 3)("mod_apple_receipt: cdn IP or client IP is '127.0.0.1', no write log file\n");	
		return 0;
	}
	if (async_flag) 
	{
		snprintf(receipt_url, 
				BUF_SIZE_4096, 
				RECEIPT_URL_FMT,
				//params.url,
				params->url,
				fdemsg->start_time.tv_sec,
				fdemsg->start_time.tv_usec / 1000,
				//params.rcpt,
				//params.cdn,
				params->rcpt,
				params->cdn,
				fdemsg->uri,
				fdemsg->cdn_ip,
				fdemsg->client_ip,
				fdemsg->user_agent,
				fdemsg->status_code,
				fdemsg->event,
				fdemsg->used_time,
				fdemsg->bytes_served,
				fdemsg->obj_cont_len,
				fdemsg->last_byte_served,
				fdemsg->refer,
				fdemsg->domain
					);
		request_t* receipt_req = urlParse(METHOD_GET, receipt_url);
		if (NULL == receipt_req)
			debug(223, 5)("mod_apple_receipt: create receipt request failed\n");
		else
			parserForwardStart(receipt_req);
	}
	if (NULL == logfile_fp) 
	{
		debug(223, 3)("mod_apple_receipt: log file fp is 'NULL'\n");
		return 0;
	}
	//Log info format: time requestedUrl cdnIP clientIP "User-Agent" statusCode event downloadTime bytesServed objectContentLength lastByteServed "Referer"
	fprintf(logfile_fp, 
			LOG_INFO_FMT, 
			fdemsg->start_time.tv_sec,
			fdemsg->start_time.tv_usec / 1000,
			fdemsg->uri,
			fdemsg->cdn_ip,
			fdemsg->client_ip,
			fdemsg->user_agent,
			fdemsg->status_code,
			fdemsg->event,
			fdemsg->used_time,
			fdemsg->bytes_served,
			fdemsg->obj_cont_len,
			fdemsg->last_byte_served,
			fdemsg->refer,
			fdemsg->domain,
			params->rcpt2
		   );
	//Note: flush the write buffer
	fflush(logfile_fp);

	return 0;
}

int mod_register(cc_module *module)
{
	debug(223, 1)("(mod_apple_receipt) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			funcSysInit);
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			funcSysParseParam);
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			receiptMsgInit);
	cc_register_hook_handler(HPIDX_hook_func_private_client_build_reply_header,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_build_reply_header),
			obtainReplyToClientMsg);
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			funcSysCleanup);
	cc_register_hook_handler(HPIDX_hook_func_private_getMultiRangeOneCopySize,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_getMultiRangeOneCopySize),
			getMultiRangeCopySize);
	cc_register_hook_handler(HPIDX_hook_func_private_getOnceSendBodySize,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_getOnceSendBodySize),
			getSendBodySize);
	cc_register_hook_handler(HPIDX_hook_func_private_record_trend_access_log,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_record_trend_access_log ),
			handleReceiptLog);

	if(reconfigure_in_thread) 
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;

	return 0;
}

#endif


#include "cc_framework_api.h"
#include <stdbool.h>
#ifdef CC_FRAMEWORK
/*
 * author: liubin  Date: 2013/7/25 
 * mod_m3u8_prefetch allow acl
 * mod_m3u8_prefetch beforeAdding=http://www.a.com/ allow acl
 */

#define NOCACHE 0
#define CACHE   1
#define M3U8_PREFETCH_PID_FILE	"/var/run/m3u8_prefetch.pid"
static cc_module* mod = NULL;
static MemPool * mod_config_pool = NULL;
static MemPool * request_param_pool = NULL;

typedef struct _mod_config {
	bool Before_flag;
    char beforeAdding[MAX_URL];
} mod_config;

typedef struct _request_param
{
	int  m3u8_fd;
    long content_length;
    long remain_clen;
    char default_prefix[MAX_URL];
    MemBuf prevBuf;
}request_param;

static void *mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_m3u8_prefetch config_struct", sizeof(mod_config));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_callback(void* data)
{
	mod_config *cfg = (mod_config*) data;
	if(cfg){
        memPoolFree(mod_config_pool,cfg);
        cfg = NULL;
	}
	return 0;
}

static void * request_param_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == request_param_pool)
	{
		request_param_pool = memPoolCreate("mod_m3u8_prefetch private_data request_param", sizeof(request_param));
	}
	return obj = memPoolAlloc(request_param_pool);
}

static int free_request_param(void* data)
{
	request_param *req_param = (request_param *) data;
	if(req_param != NULL){
		close(req_param->m3u8_fd);
        memBufClean(&(req_param->prevBuf));
		memPoolFree(request_param_pool, req_param);
		req_param = NULL;
	}
	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	debug(207,3)("mod_m3u8_prefetch config=[%s]\n", cfg_line);
	char* token = NULL;
	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_m3u8_prefetch"))
			return -1;
	}
	else
	{
		debug(207, 3)("(mod_m3u8_prefetch) ->  parse line error\n");
		return -1;
	}

    debug(207, 3)("(mod_m3u8_prefetch) -> token1 = %s\n",token);
	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(207, 3)("(mod_m3u8_prefetch) ->  use default host!\n");
	}
    
	mod_config *cfg = mod_config_pool_alloc();
    if (strcasecmp(token, "beforeAdding") == 0) {//found this config param
        cfg->Before_flag = true;
        if (NULL == (token = strtok(NULL, w_space))) {
            debug(207, 3)("(mod_m3u8_prefetch) -> config error miss beforeAdding url\n");
            free_callback(cfg);
            return -1;
        }
        if (strncmp(token,"http://",strlen("http://")) != 0) {
            debug(207, 0)("mod_m3u8_prefetch config beforeAdding url not begin with http:// check config token=%s\n",token);
            free_callback(cfg);
            return -1;
        }
        debug(207, 3)("(mod_m3u8_prefetch) -> token2 = %s\n",token);
        strncpy(cfg->beforeAdding,token,strlen(token));
    } else {
        cfg->Before_flag = false;
    }

	debug(207, 2) ("(mod_m3u8_prefetch) ->  before_flag=%s beforeAdding=%s\n",cfg->Before_flag==true?"true":"false",cfg->beforeAdding);

	cc_register_mod_param(mod, cfg, free_callback);
	return 0;
}

static long getContentLength(HttpStateData *httpState)
{
    long len = 0;
	char tmpbuf[65536];
	memset(tmpbuf,0,65536);
	strcpy(tmpbuf, httpState->reply_hdr.buf);

	char* content_line_begin = NULL;
	char* content_line_end = NULL;

	if((content_line_begin = strstr(tmpbuf,"Content-Length:")) != NULL) {
		if((content_line_end = strchr(content_line_begin, '\n')) == NULL)
			debug(207,0)("mod_m3u8_prefetch: FATAL has not complete Content-Length header line \n");

		len = atol(content_line_begin+15);
	}
    debug(207, 3)("(mod_m3u8_prefetch) -> content-length = %ld\n",len);
    return len;
}

static int getHttpResponseParam(HttpStateData *httpState,char *buf, ssize_t len)
{
    HttpReply *reply = httpState->entry->mem_obj->reply;
    request_t *request = httpState->request;
    assert(reply);
    if (reply->sline.status != HTTP_OK) {
        debug(207, 1) ("mod_m3u8_prefetch httpState entry url: %s, reply status = %d is not 200\n",storeUrl(httpState->entry),reply->sline.status);
        return 0;
    }
    request_param *r= cc_get_mod_private_data(REQUEST_T_PRIVATE_DATA, request, mod);
    if (!r) {
        r = request_param_pool_alloc();
		r->m3u8_fd = -1;
        r->content_length = -1; 
        r->remain_clen    = -1;
        strncpy(r->default_prefix, request->canonical, strlen(request->canonical));
        char *end= strstr(r->default_prefix, ".m3u8");
        *end ='\0';
        debug(207, 3) ("Debug input_buf1=[%s]\n",r->default_prefix);
        end = strrchr(r->default_prefix,'/');
        *(end+1) = '\0';
        
        debug(207, 3)("Debug input_buf=[%s]\n",r->default_prefix);
        cc_register_mod_private_data(REQUEST_T_PRIVATE_DATA, request,r, free_request_param,mod);
    }

    long content_length = getContentLength(httpState);
    if (content_length > 0) {
        r->content_length = content_length; 
        r->remain_clen    = content_length;
    }

    debug(207, 1) ("mod_m3u8_prefetch getHttpResponseParam url=%s,content_length=%ld,remain_clen=%ld\n",storeUrl(httpState->entry),r->content_length, r->remain_clen);
    return 0;
}

static char *ReadLine(char *buffer, const size_t len, char **pos,bool *has_crlf)
{
    if (buffer == NULL || len == 0)
        return NULL;

    char *line = NULL;
    char *begin = buffer;
    char *p = begin;
    char *end = p + len;

    while (p < end)
    {
        if ((*p == '\r') || (*p == '\n') || (*p == '\0'))
            break;
        p++;
    }

    /* copy line excluding \r \n or \0 */
    line = xstrndup(begin, p - begin + 1);

    while ((*p == '\r') || (*p == '\n') || (*p == '\0'))
    {
        if (*p == '\n')
            *has_crlf = true;
        else
            *has_crlf = false;

        if (*p == '\0')
        {
            *pos = end;
            break;
        }
        else
        {
            /* next pass start after \r and \n */
            p++;
            *pos = p;
        }   
    }

    return line;
}

static request_t *
modifyRequestUrlstoreUrl(const char *url)
{
    request_t *new_request = NULL;
    new_request = urlParse(METHOD_GET, (char*)url);
    char *new_storeurl = xstrdup((const char *)url);
    char* str = strchr(new_storeurl, '?');
    if (str) {
        debug(207, 3)("modify_request_url_storeurl rid_question: storeurl=[%s] have question\n", new_storeurl);
        *str = '\0';
    }
    
    new_request->store_url = xstrdup((const char *)new_storeurl); 
    
    debug(207, 3)("after modifyRequestUrlstoreUrl url=%s,storeurl=%s,uri=%s\n",urlCanonical(new_request),new_request->store_url,strBuf(new_request->urlpath));
    return requestLink(new_request);
}

static void asyncPrefetchTs(const char *url)
{
    debug(207, 1) ("start asyncPrefetchTs async prefetch: '%s'\n", url);
        
    request_t *request = NULL;
    StoreEntry *entry = NULL;
    request = modifyRequestUrlstoreUrl(url);
    request->flags.cachable = CACHE;
    
    if (storeGetPublicByRequest(request) != NULL) {
        debug(207, 3) ("this entry has already exist,give up async prefetch: '%s'\n", request->store_url);
        return;
    }
    
	entry = storeCreateEntry(urlCanonical(request),
			request->flags,
			request->method);
    if (request->store_url) 
        storeEntrySetStoreUrl(entry, request->store_url);

    if(Config.onoff.collapsed_forwarding)
    {
        EBIT_SET(entry->flags, KEY_EARLY_PUBLIC);
        storeSetPublicKey(entry);
    }

	fwdStart(-1, entry, request);
	requestUnlink(request);
}

static int m3u8_connect()
{
	int fd;
	struct hostent      *site;  
	struct sockaddr_in  myaddress;
	if( (site = gethostbyname("127.0.0.1")) == NULL )
		return -1;

	if( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		return -1;
	}
	memset(&myaddress, 0, sizeof(struct sockaddr_in));
	myaddress.sin_family = AF_INET;
	myaddress.sin_port   = htons(15101);
	memcpy(&myaddress.sin_addr, site->h_addr_list[0], site->h_length);

	int ret = -1;
	ret = connect(fd, (struct sockaddr *)&myaddress, sizeof(struct sockaddr)); 
	if (ret != 0) {
		if ( errno != EINPROGRESS ) {
            close(fd);
			return -1;
		}
		else{
			return -2;
		}
	}

	return fd;
}

static int safe_write(int fd, const char *buffer,int length)
{
	int bytes_left = 0;
	int written_bytes = 0;
	const char *ptr=NULL;

	ptr=buffer;
	bytes_left = length;

	if (length == 0)
		return 0;

	int retry_times = 0;

	while (bytes_left > 0) {
		written_bytes = write(fd, ptr, bytes_left);

		if (written_bytes == 0)
			return length - bytes_left;

		if(written_bytes < 0) {
			if(errno == EINTR) {
				usleep(100000);
				continue;
			} else {
				retry_times++;
				if (retry_times < 5) {
					usleep(100000);
					continue;
				} else {
					debug(207,2)("mod_m3u8_prefetch: fwrite error(%d) for safe_write\n", errno);
					return -1;
				}
			}
		}
		bytes_left -= written_bytes;
		ptr += written_bytes;
	}
	return length;
}

static void sendUrlToM3u8Helper(request_param *r, const char *url)
{
    int fd = -1, w_len = -1;
    int len = strlen(url);

	if (r->m3u8_fd > 0) {
		fd = r->m3u8_fd;// use exist m3u8_fd
	}
    else if ((fd = m3u8_connect()) < 0) {
        debug(207,3)("mod_m3u8_prefetch: mp4 moov_generor is not connected, please check moov_generor \n");
        return;
    }
    if ((w_len = safe_write(fd, url, len) != len )) {
        debug(207,3)("mod_m3u8_prefetch: size which write to moov_generor is wrong,request all cache object content \n");
        close(fd);
		r->m3u8_fd = -1;
    }
    debug(207,3)("mod_m3u8_prefetch sendUrlToM3u8Helper --> fd = %d\n",fd);
    //close(fd);
}

static size_t parseM3u8(char *buf, ssize_t len, request_param *r,mod_config *cfg)
{
    debug(207, 3)("parse_M3u8 input buf len=[%zd],input_buf=[%s]",len,buf);
    char *p_read=NULL, *p_begin, *p_end;
    char *line = NULL;
    bool has_crlf = false;
    char url[MAX_URL]={'\0'};
    assert(buf);

    if (memBufIsNull(&(r->prevBuf))) {
        memBufDefInit(&(r->prevBuf));
    }

    MemBuf curBuf;
    memBufDefInit(&curBuf);
    memBufAppend(&curBuf, r->prevBuf.buf, r->prevBuf.size);
    memBufReset(&(r->prevBuf));
    memBufAppend(&curBuf, buf, len);

    debug(207, 3)("cfg->beforeAdding=%s,Before_flag=%d\n",cfg->beforeAdding,cfg->Before_flag);
    p_begin = curBuf.buf;
    p_end = p_begin + curBuf.size;
    while ((line = ReadLine(p_begin, p_end - p_begin,  &p_read, &has_crlf)) != NULL)
    {
        p_begin = p_read;
        if (has_crlf) {
            if (line[0] == '#')
                continue;
            if (cfg->Before_flag == true) {
                snprintf(url, MAX_URL, "%s%s",cfg->beforeAdding,line);
                debug(207, 3)("has beforeAdding url=%s\n",url);
            } else {
                snprintf(url, MAX_URL, "%s%s",r->default_prefix,line);
                debug(207, 3)("default url=%s\n", url);
            }
            //asyncPrefetchTs(url);
            sendUrlToM3u8Helper(r,url);

            debug(207, 3)("parseLine line = %s,has_crlf=%d\n",line,has_crlf);
        } else {
            debug(207, 3)("prevbuf line= %s,has_crlf=%d,len(line)=%zu\n", line,has_crlf,strlen(line));
            memBufAppend(&(r->prevBuf), line, strlen(line));
        }
        xfree(line);
    }
    
    debug(207, 3) ("parseLine p_read = %s,has_crlf=%d\n",p_read, has_crlf);
    memBufClean(&curBuf); 
    return 0;
}

static int asyncGetTsFromM3u8(HttpStateData * httpState, char *buf, ssize_t len, char* comp_buf, ssize_t comp_buf_len, size_t *comp_size, int buffer_filled)
{
    HttpReply *reply = httpState->entry->mem_obj->reply;
    assert(reply);
    if (reply->sline.status != HTTP_OK) {
        debug(207, 1) ("mod_m3u8_prefetch asyncGetTsFromM3u8 entry url: %s, reply status = %d is not 200 return\n",storeUrl(httpState->entry),reply->sline.status);
        return 0;
    }

	memset(comp_buf, 0, comp_buf_len);
    xmemcpy(comp_buf, buf, len);
    *comp_size = len;
	mod_config *cfg = cc_get_mod_param(httpState->fd, mod);
    assert(cfg);
    request_param *r= cc_get_mod_private_data(REQUEST_T_PRIVATE_DATA, httpState->request, mod);
    assert(r);
    parseM3u8(buf, len, r,cfg);
    return 0;
}

static bool is_m3u8_prefetch_alive()
{
	FILE *fd = NULL;
	char pid[128];
	char buff[128];
	FILE  *procfd;
	char *pos = NULL;
	static int pidnum;

	debug(207,3) ("pidnum : %d\n",pidnum);

	if(pidnum == 0){
		if ((fd = fopen(M3U8_PREFETCH_PID_FILE, "r")) != NULL) {
			fgets(pid, 128, fd);
			pidnum = atoi(pid);
			fclose(fd);
		}
	}

	if(pidnum != 0){
		snprintf(buff, 128, "/proc/%d/status", pidnum);
		if ((procfd = fopen(buff,"r")) != NULL) {
			memset(pid, 0, 128);
			fgets(pid, 128, procfd);

			debug(207,3)("%s -> %s\n",buff,pid);

			fclose(procfd);
			if ((pos = strstr(pid, "m3u8_prefetch")) != NULL) {
				return true;
			}
		}
	}
	else{
		debug(207, 3) ("cannot get m3u8_prefetch pidnum!\n");
	}

	pidnum = 0;
	return false;
}

static void m3u8Event(void *args)
{
    if(!is_m3u8_prefetch_alive()){
        debug(207,3)("m3u8_prefetch dont run,now starting it!\n");
        enter_suid();

        int cid = fork();
        if (cid == 0) {
            int ret = execl("/usr/local/squid/bin/m3u8_prefetch", "m3u8_prefetch","15101",(char *)0);
            if (ret < 0) {
                debug(207,3)("(m3u8) --> execl error : %s\n",xstrerror());
            }
            exit(-1);
        }
        leave_suid();
    }

    eventAdd("m3u8Event", m3u8Event, NULL, 30, 0);	
}

static int hook_init()
{
	debug(207, 1)("(mod_m3u8_prefetch) ->	hook_init:----:)\n");
	eventAdd("m3u8Event", m3u8Event, NULL, 0, 0);	
	return 0;
}

static int hook_cleanup(cc_module *module)
{
	debug(207, 1)("(mod_m3u8_prefetch) ->	hook_cleanup:\n");
	eventDelete(m3u8Event, NULL);
	if(NULL != request_param_pool)
		memPoolDestroy(request_param_pool);
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}

int mod_register(cc_module *module)
{
	debug(207, 1)("(mod_m3u8_prefetch) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

    cc_register_hook_handler(HPIDX_hook_func_sys_init,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
            hook_init);

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_buf_recv_from_s_header,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_buf_recv_from_s_header),
			getHttpResponseParam);

	cc_register_hook_handler(HPIDX_hook_func_private_compress_response_body,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_compress_response_body),
			asyncGetTsFromM3u8);

    cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
            hook_cleanup);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif


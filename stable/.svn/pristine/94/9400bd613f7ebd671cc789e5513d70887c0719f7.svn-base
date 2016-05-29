#include "cc_framework_api.h"
#include "mod_customized_server_side_error_page.h"

#ifdef CC_FRAMEWORK
extern int clientTryParseRequest(ConnStateData * conn);
static cc_module* mod = NULL;
static MemPool * error_page_private_data_pool = NULL;
static MemPool * timeout_pool = NULL;
static MemPool * text_mb_pool = NULL;
static MemPool * mod_config_pool = NULL;
int text_config_len;
//static char **customized_error_text;
//static time_t error_page_expires[20] = 0; //record all the configure_line's error_page_expires ,so the mod's configure_line num should not bigger than 20

extern ErrorState *errorCon(err_type type, http_status, request_t * request);
static int free_error_page_private_data(void* data)
{
	error_page_private_data* pd = (error_page_private_data*) data;
	if(NULL != pd)
	{
		memPoolFree(error_page_private_data_pool, pd);
		pd = NULL;
	}
	return 0;
}

static void * error_page_private_data_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == error_page_private_data_pool)
	{
		error_page_private_data_pool = memPoolCreate("mod_srv_side_errpage private_data error_page_private_data", sizeof(error_page_private_data));	
	}
	return obj = memPoolAlloc(error_page_private_data_pool);
}

static void * timeout_pool_alloc(void)
{
	void * obj = NULL;
	if(NULL == timeout_pool)
	{
		timeout_pool = memPoolCreate("mod_srv_side_errpage private_data timeout", sizeof(int));
	}
	return obj = memPoolAlloc(timeout_pool);
}

static void * text_mb_pool_alloc(void)
{
	void * obj = NULL;
	if(NULL == text_mb_pool)
	{
		text_mb_pool = memPoolCreate("mod_srv_side_errpage private_data text", sizeof(MemBuf));
	}
	return obj = memPoolAlloc(text_mb_pool);
}

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if(NULL == mod_config_pool)	
	{
		mod_config_pool = memPoolCreate("mod_srv_side_errpage config_struct", sizeof(mod_config));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int is_url(char* str)
{
	return strncmp("http://",str,7)? 0 : 1;
}


static int free_timeout(void* data)
{
	int *timeout = (int *)data;
	if(NULL != timeout)
	{
		memPoolFree(timeout_pool, timeout);
		timeout = NULL;
	}
	return 0;
}


static int free_data(void *data)
{
	MemBuf * mb = (MemBuf*)data;
	if(NULL != mb && NULL != mb->buf)
	{
		memFreeBuf(mb->capacity, mb->buf);
		memPoolFree(text_mb_pool, mb);
		mb = NULL;
	}
	return 0;
}

// add by xueye.zhao
// 2013-4-18

static int is_http_move_status(int status)
{
    if(301 == status || 302 == status)
    {
        return 1;
    }

    return 0;
}
// end add

static int clientHandleErrorPage(clientHttpRequest *http)
{
    debug(115, 4)("mod_customized_server_side_error_page clientHandleErrorPage\n");
	request_t *request = http->request;
	int fd = http->conn->fd;
	if(fd_table[fd].cc_run_state[mod->slot] > 0)
	{

		error_page_private_data* pd=NULL;
		pd = error_page_private_data_pool_alloc();

		mod_config* cfg = (mod_config*)cc_get_mod_param(fd,mod);
		
		pd->OfflineTimeToLive= cfg->OfflineTimeToLive;
		pd->TimeToTrigger = squid_curtime + cfg->TriggerTimeSec;
		pd->ResponseStatus = cfg->ResponseStatus;
		pd->recusive_time = 0;
		pd->old_status = 0;
		strncpy(pd->OptionalHttpStatusPattern,cfg->OptionalHttpStatusPattern, strlen(cfg->OptionalHttpStatusPattern));


		cc_register_mod_private_data(REQUEST_PRIVATE_DATA,http, pd, free_error_page_private_data ,mod);
		//int *timeout = timeout_pool_alloc();
		//*timeout = cfg->TriggerTimeSec;
		//debug(115,4)("clientHandleErrorPage: the timeout before register mod private data is: %d \n ",*timeout);
		//cc_register_mod_private_data(FDE_PRIVATE_DATA,&fd, timeout,free_timeout,mod);
        
        // add by xueye.zhao
        // 2013-4-18
        if (!is_http_move_status(cfg->ResponseStatus))
        {
            MemBuf * text = text_mb_pool_alloc();

            memBufInit(text, cfg->customized_error_text.size+1, cfg->customized_error_text.size+1);
            memBufAppend(text, cfg->customized_error_text.buf, cfg->customized_error_text.size);
            debug(115,4)("mod_customized_server_side_error_page clientHanleErrorPage :before cc_register_mod_private_data and the request is: %p\n",request);
            cc_register_mod_private_data(REQUEST_T_PRIVATE_DATA,request,text,free_data,mod);
        }
        //end add
	}
	return 0; 

}

static int free_mod_config(void *data)
{
	mod_config* cfg = (mod_config*)data;
	if(cfg)
	{
		if(NULL != cfg->customized_error_text.buf)
			memBufClean(&cfg->customized_error_text);
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	return 0;
}

static void  errorTryLoadText(MemBuf * text, char *path)
{
	int fd;
	struct stat sb;
//	char *text = NULL;

	//snprintf(path, sizeof(path), "%s/%s", dir, page_name);
#ifdef _SQUID_MSWIN_
	fd = open(path, O_RDONLY | O_BINARY);
#else
	fd = open(path, O_RDONLY | O_TEXT);
#endif
	if (fd < 0 || fstat(fd, &sb) < 0)
	{
		debug(115, 0) ("mod_customized_server_side_error_page errorTryLoadText: '%s': %s\n", path, xstrerror());
		if (fd >= 0)
			file_close(fd);
		//return NULL;
		memBufInit(text, 1*sizeof(char), 1*sizeof(char));
		memBufPrintf(text, "%s", "");
	}
	else
	{
		memBufInit(text, (size_t)sb.st_size+2+1, (size_t)sb.st_size+2+1);
		if (read(fd, text->buf, (int) sb.st_size) != sb.st_size)
		{
			debug(115,0) ("mod_customized_server_side_error_page errorTryLoadText: failed to fully read: '%s': %s\n",
					path, xstrerror());
			memBufClean(text);
		}
		text->size += sb.st_size;
		text->buf[text->size] = '\0';
		debug(115,4)("mod_customized_server_side_error_page errorTryLoadText: the path is: %s point before end and text is: %s.....\n", path,text->buf);
		close(fd);
	}
#ifndef CC_FRAMEWORK
	if (NULL != text->buf && strstr(text->buf, "%s") == NULL)
		memBufAppend(text,"%S", strlen("%S"));	/* add signature */
#endif 
}

/*
 * convert the http uri to file name
 */
static int stringConvert(char* name)
{

	if(strncmp("http://",name,7))
	{
		return 1;
	}
	char* p = name+7;
	while(*p != '\0')
	{
		if(*p == '/')
			*p ='_'; 
		p++;
	}
	debug(115,4)("mod_customized_server_side_error_page: strintConvert name_after is: %s\n",name);
	return 0;
}

static int func_sys_parse_param(char* cfgline)
{
	debug(115,4)("mod_customized_server_side_error_page: func_sys_parse_param\n");
	char* token =NULL;
	if((token = strtok(cfgline, w_space)) == NULL)
	{
		return -1;
	}
	else if(strcmp(token,"mod_customized_server_side_error_page"))
			return -1;
	mod_config *cfg = mod_config_pool_alloc();
	if((token = strtok(NULL,w_space)))
	{
		int len = 0;
		if((len = strlen(token)) > MAX_LOCATION_BYTES)
		{
			debug(115,1)("mod_customized_server_side_error_page the config Location is too large\n");
			return -1;
		}
		strncpy(cfg->location,token,len);

		//	cfg->is_url = 1;
		/* if the dir customized_error_page does not exist, create it */

		if((token = strtok(NULL,w_space)))
		{
			cfg->ResponseStatus = atoi(token);	
		}
		else
		{
			goto parse_err;
		}
		if((token = strtok(NULL,w_space)))
		{
			cfg->TriggerTimeSec = atoi(token);
		}
		else
		{
			goto parse_err;
		}
		if((token = strtok(NULL,w_space)))
		{
			cfg->OfflineTimeToLive=  atoi(token);
			cfg->error_page_expires = 0;
		}
		else
		{
			goto parse_err;
		}
		if((token = strtok(NULL,w_space)))
		{
			if(strlen(token)>=256)
				goto parse_err;
			strncpy(cfg->OptionalHttpStatusPattern,token,strlen(token));
		}
		else
		{
			goto parse_err;
		}
		/*
		   token = strstr(str,token);
		   strtok(token,w_space);
		   */
		cc_register_mod_param(mod, cfg, free_mod_config);

		return 0;
parse_err:
		{
			debug(115,0) ("mod_customized_server_side_error_page configure error\n");
			memPoolFree(mod_config_pool, cfg);
			cfg= NULL;
			return -1;
		}

	}
	return 0;
}


static void func_sys_after_parse_param()
{
	int i;
	for(i = 0 ; i < mod->mod_params.count; i++)
	{
		cc_mod_param *mod_param = mod->mod_params.items[i];
		mod_config *cfg = mod_param->param;

        // add by xueye.zhao
        // 2013-4-18

        if (is_http_move_status(cfg->ResponseStatus))
        {
            continue;
        }

        // end add

        if(is_url(cfg->location))
		{

			struct stat sb;
			if (stat("/data/proclog/log/squid/customized_error_page", &sb) != 0){
				mkdir("/data/proclog/log/squid/customized_error_page", S_IRWXU);
			}
			FILE *fp = popen("/usr/local/squid/bin/get_file_from_url.pl", "r");
			assert(fp);
			while(1)
			{
				char perloutput[512];
				if(!fgets(perloutput, 512, fp))
				{
					break;
				}

				debug(115, 1)("mod_customized_server_side_error_page get_file_from_url.pl said: %s\n", perloutput);
			}
			pclose(fp);


			/*
			   if (stat("/data/proclog/log/squid/customized_error_page", &sb) != 0){
			   mkdir("/data/proclog/log/squid/customized_error_page", S_IRWXU);
			   }
			   */
			if(stringConvert((cfg->location)))
			{
				debug(115,0) ( "mod_customized_server_side_error_page: cfg->location url error\n");
				continue;
			}

			debug(115,4) ( "mod_customized_server_side_error_page: cfg->location is: %s\n", cfg->location);

			//char* tmp = NULL;
			String tmp = StringNull;
			stringInit(&tmp, "/data/proclog/log/squid/customized_error_page/");
			stringAppend(&tmp, cfg->location+7, strlen(cfg->location)-7);
			memset(cfg->location,0,sizeof(cfg->location));
			strncpy(cfg->location,strBuf(tmp),strLen(tmp));
			errorTryLoadText(&cfg->customized_error_text, cfg->location);
			stringClean(&tmp);
		}
		else
		{
			//cfg->is_url = 0;
			errorTryLoadText(&cfg->customized_error_text, cfg->location);
		}
	}
}



/*handle errorEntry here*/
static int repl_send_start(clientHttpRequest *http, int *ret)
{
    debug(115, 4)("mod_customized_server_side_error_page repl_send_start\n");
	int fd= http->conn->fd;
	mod_config* cfg = cc_get_mod_param(fd,mod);
	//mod_config *cfg = ((mod_config*)(param->param));
	if(fd_table[fd].cc_run_state[mod->slot]>0)
	{
		error_page_private_data* pd = (error_page_private_data*) cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);

		if(pd->OfflineTimeToLive > 0&& pd->recusive_time<2 && pd->recusive_time > 0 && cfg->customized_error_text.buf != NULL/* && pd->ResponseStatus <400 */)
		{
			debug(115,4)("customized_server_side_error_page: repl_send_start the recusive_time is: %d\n",pd->recusive_time);
			ErrorState *err;
			err_type page_id;
			int status = pd->ResponseStatus;
			switch(status)
			{
				case HTTP_BAD_REQUEST:
				case HTTP_LENGTH_REQUIRED:
				case HTTP_EXPECTATION_FAILED:
					page_id = ERR_INVALID_REQ;
					break;
				case HTTP_UNAUTHORIZED:
				case HTTP_FORBIDDEN:
				case HTTP_METHOD_NOT_ALLOWED:
				case HTTP_NOT_ACCEPTABLE:
				case HTTP_PROXY_AUTHENTICATION_REQUIRED:
				case HTTP_REQUEST_TIMEOUT:
				case HTTP_CONFLICT:
				case HTTP_GONE:
				case HTTP_PRECONDITION_FAILED:
				case HTTP_UNSUPPORTED_MEDIA_TYPE:
				case HTTP_UNPROCESSABLE_ENTITY:
				case HTTP_LOCKED:
				case HTTP_FAILED_DEPENDENCY:
					page_id = ERR_ACCESS_DENIED;
					break;
				case HTTP_NOT_FOUND :
					page_id = ERR_FTP_NOT_FOUND;
					break;
				case HTTP_REQUEST_ENTITY_TOO_LARGE:
				case HTTP_REQUEST_URI_TOO_LONG:
					page_id = ERR_TOO_BIG;
					break;
				case HTTP_INTERNAL_SERVER_ERROR:
					page_id= ERR_SOCKET_FAILURE;
					break;
				case HTTP_NOT_IMPLEMENTED:
					page_id = ERR_UNSUP_REQ;
					break;
				case HTTP_BAD_GATEWAY:
				case HTTP_GATEWAY_TIMEOUT:
					page_id = ERR_CONNECT_FAIL;
					break;
				case HTTP_SERVICE_UNAVAILABLE:
					page_id = ERR_FTP_UNAVAILABLE;
					break;
				default:
					page_id = ERR_HTTP_SPECIAL;
			}
			//	err = errorCon(page_id,


			err = errorCon(page_id,pd->ResponseStatus,http->request);

			/********************may be leak here**********************************/

			assert(err!=NULL);
			if(squid_curtime > cfg->error_page_expires)
				cfg->error_page_expires = squid_curtime + pd->OfflineTimeToLive;
			//err->request = requestLink(http->request);
			storeClientUnregister(http->sc,http->entry,http);
			http->sc = NULL;
			storeUnlockObject(http->entry);
			http->log_type = LOG_TCP_HIT;
			http->entry = clientCreateStoreEntry(http,http->request->method,null_request_flags);

			debug(115,4)("mod_customized_server_side_error_page repl_send_start: before errorAppendEntry, the err->page_id is: %d, the err->http_status is: %d , the err->type is: %d \n",err->page_id, err->http_status,err->type);
			errorAppendEntry(http->entry,err);
			/********************may be leak here**********************************/
			*ret = 1;

		}

        // add by xueye.zhao
        // 2013-4-18
        else if (pd->OfflineTimeToLive > 0&& pd->recusive_time<2 && pd->recusive_time > 0)
        {
            if(squid_curtime > cfg->error_page_expires)
                cfg->error_page_expires = squid_curtime + pd->OfflineTimeToLive;

            HttpReply *rep = httpReplyCreate();
            http->entry = clientCreateStoreEntry(http, http->request->method, http->request->flags);
            http->log_type = LOG_TCP_MISS;
            storeReleaseRequest(http->entry);
            httpRedirectReply(rep, cfg->ResponseStatus, cfg->location);
            httpReplySwapOut(rep, http->entry);
            storeComplete(http->entry);
            *ret = 1;

            return *ret; 
        }
        // end add
    }
	return *ret;	
}

static int setTimeout(FwdState* fwd, int* timeout)
{
    debug(115, 4)("mod_customized_server_side_error_page setTimeout\n");
	int *num= (int *)cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA,fwd,mod);
	debug(115, 4)("mod_customized_server_side_error_page: setTimout num is: %d\n",*num);
	if(*num>0)
		*timeout = *num;
	 return 0;
}

static int isMatchedStatus(int status,char * pattern)
{
    debug(115, 4)("mod_customized_server_side_error_page isMatchedStatus\n");
	debug(115, 4)("mod_customized_server_side_error_page isMatchedStatus: status and pattern is : %d \t %s\n",status, pattern);
	char* token;
	int s = 0;
	if(( token = strtok(pattern,"|")))
	{
		s = atoi(token);
		if(status == s)
			return 1;
	}
	else
	{
		if( status == 504)
			return 1;
		else
			return 0;
	}
	while( (token = strtok(NULL,"|")))
	{
		s = atoi(token);
		if (status == s)
			return 1;
	}
	return 0;
}

static int process_error_page(clientHttpRequest *http, HttpReply* rep)
{
    debug(115, 4)("mod_customized_server_side_error_page process_error_page\n");

	error_page_private_data* pd = (error_page_private_data*) cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);
	char tmpPattern[200];
	strncpy(tmpPattern,pd->OptionalHttpStatusPattern,strlen(pd->OptionalHttpStatusPattern));
	if(pd->recusive_time==0)
	{
		pd->old_status = rep->sline.status;
	}
	if((pd->OfflineTimeToLive>0) && pd->recusive_time<2 && (isMatchedStatus(pd->old_status, tmpPattern) || rep->sline.status == HTTP_SPECIAL))
	{
		debug(115, 4) ("mod_customized_server_side_error_page process_error_page in module customized_server_side_error_page entered: and rep status line is: %d,and recusive_time is: %d, the private-data's ResponseStatus is %d, OptionalHttpStatusPattern is: %s\n",rep->sline.status,pd->recusive_time,pd->ResponseStatus,pd->OptionalHttpStatusPattern);
		if(pd->recusive_time)
		{
			httpStatusLineSet(&rep->sline,rep->sline.version,pd->ResponseStatus,NULL);
		}
		pd->recusive_time++;
	}
	return 0;
}

static int func_sys_cleanup()
{
	debug(115, 1)("(mod_customized_server_side_error_page) ->     hook_cleanup:---:)\n");
	if(NULL != error_page_private_data_pool)
		memPoolDestroy(error_page_private_data_pool);
	if(NULL != timeout_pool)
		memPoolDestroy(timeout_pool);
	if(NULL != text_mb_pool)
		memPoolDestroy(text_mb_pool);
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}


static int process_miss_in_error_page(clientHttpRequest* http ,int *ret)
{
    debug(115, 4)("mod_customized_server_side_error_page process_miss_in_error_page\n");
	int fd = http->conn->fd;
	if(fd_table[fd].cc_run_state[mod->slot]>0)
	{
		mod_config* cfg = cc_get_mod_param(fd,mod);
		if(squid_curtime < cfg->error_page_expires )
		{
			ErrorState* err = NULL;
			/**********************************/
			err_type page_id;
            int status = cfg->ResponseStatus;
            
            // add by xueye.zhao
            // 2013-4-18

            if (is_http_move_status(status))
            {
                HttpReply *rep = httpReplyCreate();
                http->entry = clientCreateStoreEntry(http, http->request->method, http->request->flags);
                http->log_type = LOG_TCP_HIT;
                storeReleaseRequest(http->entry);
                httpRedirectReply(rep, cfg->ResponseStatus, cfg->location);
                httpReplySwapOut(rep, http->entry);
                storeComplete(http->entry);

                *ret = 1;
                return *ret;
            }

            //end add

			switch(status)
			{
				case HTTP_BAD_REQUEST:
				case HTTP_LENGTH_REQUIRED:
				case HTTP_EXPECTATION_FAILED:
					page_id = ERR_INVALID_REQ;
					break;
				case HTTP_UNAUTHORIZED:
				case HTTP_FORBIDDEN:
				case HTTP_METHOD_NOT_ALLOWED:
				case HTTP_NOT_ACCEPTABLE:
				case HTTP_PROXY_AUTHENTICATION_REQUIRED:
				case HTTP_REQUEST_TIMEOUT:
				case HTTP_CONFLICT:
				case HTTP_GONE:
				case HTTP_PRECONDITION_FAILED:
				case HTTP_UNSUPPORTED_MEDIA_TYPE:
				case HTTP_UNPROCESSABLE_ENTITY:
				case HTTP_LOCKED:
				case HTTP_FAILED_DEPENDENCY:
					page_id = ERR_ACCESS_DENIED;
					break;
				case HTTP_NOT_FOUND :
					page_id = ERR_FTP_NOT_FOUND;
					break;
				case HTTP_REQUEST_ENTITY_TOO_LARGE:
				case HTTP_REQUEST_URI_TOO_LONG:
					page_id = ERR_TOO_BIG;
					break;
				case HTTP_INTERNAL_SERVER_ERROR:
					page_id= ERR_SOCKET_FAILURE;
					break;
				case HTTP_NOT_IMPLEMENTED:
					page_id = ERR_UNSUP_REQ;
					break;
				case HTTP_BAD_GATEWAY:
				case HTTP_GATEWAY_TIMEOUT:
					page_id = ERR_CONNECT_FAIL;
					break;
				case HTTP_SERVICE_UNAVAILABLE:
					page_id = ERR_FTP_UNAVAILABLE;
					break;
				default:
					page_id = ERR_HTTP_SPECIAL;
			}
			/**********************************/
			debug(115, 5)("mod_customized_server_side_error_page: cc_error_page running, don't back source server\n");
			http->al.http.code = HTTP_FORBIDDEN;
			err = errorCon(page_id, cfg->ResponseStatus,http->request);
//			err->request = requestLink(http->request);
			err->src_addr = http->conn->peer.sin_addr;
			http->log_type = LOG_TCP_DENIED;
			http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
			errorAppendEntry(http->entry, err);
			*ret =1;
		}
	}
	return *ret;
		
}

static int set_error_text(ErrorState* err,char** text)
{
    debug(115, 4)("mod_customized_server_side_error_page set_error_text\n");
	request_t *request = err->request;
	MemBuf * mb;
	debug(115,4)("set_error_text: the the http_status  is: %d\n",err->http_status);
    
    if(err->type == ERR_HTTP_SPECIAL)
	{
		debug(115,5)("set_error_text: the err->type is ERR_HTTP_SPECIAL\n");
		assert(request!=NULL);
		if(request->cc_request_private_data)
		{
			mb = cc_get_mod_private_data(REQUEST_T_PRIVATE_DATA,request,mod);
			*text = mb->buf;
			debug(115,6)("set_error_text: the err->type is ERR_HTTP_SPECIAL, and error_text is: %s\n", *text);
		}
	}

	return 0;
}

static void fwd_start(FwdState *fwd)
{
    debug(115, 4)("mod_customized_server_side_error_page fwd_start\n");
	mod_config* cfg = (mod_config*)cc_get_mod_param(fwd->client_fd,mod);
	int *timeout = timeout_pool_alloc();
	*timeout = cfg->TriggerTimeSec;

	cc_register_mod_private_data(FWDSTATE_PRIVATE_DATA,fwd, timeout, free_timeout ,mod);
}

int mod_register(cc_module* module)
{
	strcpy(module->version, "7.0.R.16488.i686");
	debug(115,3)("mod_customized_server_side_error_page entering\n");
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	//module->hook_func_sys_after_parse_param = func_sys_after_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_after_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_after_parse_param),
			func_sys_after_parse_param);

	//module->hook_func_http_req_read = clientHandleErrorPage; 1
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			clientHandleErrorPage);
	//module->hook_func_private_comm_set_timeout = setTimeout; 4
	cc_register_hook_handler(HPIDX_hook_func_private_comm_set_timeout,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_comm_set_timeout),
			setTimeout);
	//module->hook_func_private_repl_send_start = repl_send_start; 7
	cc_register_hook_handler(HPIDX_hook_func_private_repl_send_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_repl_send_start),
			repl_send_start);
	//module->hook_func_private_process_error_page = process_error_page; 6
	cc_register_hook_handler(HPIDX_hook_func_private_process_error_page,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_process_error_page),
			process_error_page);
	//module->hook_func_private_process_miss_in_error_page = process_miss_in_error_page; 2
	cc_register_hook_handler(HPIDX_hook_func_private_process_miss_in_error_page,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_process_miss_in_error_page),
			process_miss_in_error_page);
	//module->hook_func_private_set_error_text = set_error_text; 5
	cc_register_hook_handler(HPIDX_hook_func_private_set_error_text,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_error_text),
			set_error_text);
	//module->hook_func_fwd_start = fwd_start; 3
	cc_register_hook_handler(HPIDX_hook_func_fwd_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_fwd_start),
			fwd_start);

        //module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			func_sys_cleanup);
	//mod=module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif

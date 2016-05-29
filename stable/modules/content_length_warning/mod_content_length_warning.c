#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

#define w_sp  " \t=:"

static cc_module* mod = NULL;
static MemPool * qihu_warning_config_pool = NULL;

//static Logfile* qihu_warninglog =NULL;

typedef struct _qihu_warning_config
{
	int allow_or_deny;
	char *receive_log_path;
	char *send_log_path;
}qihu_warning_config;

//allocate the config struct
static void * qihu_warning_config_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == qihu_warning_config_pool)
	{
		qihu_warning_config_pool = memPoolCreate("mod_qihu_warning_config config config ", sizeof(qihu_warning_config));
	}
	return obj = memPoolAlloc(qihu_warning_config_pool);
}



//collect the config struct which need to free
static int qihu_warning_config_pool_free(void *data)
{
	qihu_warning_config *cfg = (qihu_warning_config *)data;
	if (cfg)
	{
		safe_free(cfg->receive_log_path);
		safe_free(cfg->send_log_path);
		memPoolFree(qihu_warning_config_pool, cfg);
		cfg = NULL;
	}
	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{
	qihu_warning_config *cfg = NULL;

	cfg = qihu_warning_config_pool_alloc();
	assert(cfg_line);
	char *token = strtok(cfg_line, w_space);
	if(strcmp(token, "mod_content_length_warning"))
	{
		debug(149,0)("mod_qihu_warning ----> cfg_line not begin with mod_qihu_warning\n");
		goto out;
	}

	while((token = strtok(NULL,w_sp)) != NULL)
	{
		if(!strcmp(token,"receive_log_path"))			
		{
			if((token=strtok(NULL,w_sp)) != NULL)					
			{
				int len=strlen(token);
				cfg->receive_log_path=xmalloc(len+1);						
				memcpy(cfg->receive_log_path,token,strlen(token));
				cfg->receive_log_path[len]='\0';

			}else{
				goto out;
			}
		
		}
		else if(!strcmp(token,"send_log_path"))
		{
			if((token=strtok(NULL,w_sp)) != NULL)					
			{
				int len=strlen(token);
				cfg->send_log_path=xmalloc(len+1);						
				memcpy(cfg->send_log_path,token,strlen(token));
				cfg->send_log_path[len]='\0';

			}else{
				goto out;
			}
		}else if(!strcmp(token,"allow")){
			cfg->allow_or_deny=1;	
			break;
		}else if(!strcmp(token,"deny")){
			cfg->allow_or_deny=0;
			break;
		}else
			goto out;
	}
	FILE *fp;
	if((fp=fopen(cfg->receive_log_path,"a+"))==NULL)
	{
		debug(149,0)("mod_qihu_warning ---->open receive_log_path:%s error\n",cfg->receive_log_path);
		goto out;
	}
	chmod(cfg->receive_log_path,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
	fp=NULL;

/*	if(cfg->send_log_path)
	{
		if((fp=fopen(cfg->send_log_path,"a+"))==NULL)
		{
			debug(149,0)("mod_qihu_warning ---->open send_log_path:%s error\n",cfg->send_log_path);
			goto out;
		}
		chmod(cfg->send_log_path,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
		fclose(fp);
	}
*/

	cc_register_mod_param(mod, cfg,qihu_warning_config_pool_free);
	return 0;
out:
	if(cfg)
	{
		memPoolFree(qihu_warning_config_pool, cfg);
		cfg = NULL ;
	}
	debug(149,0)("mod_qihu_warning:parse error!\n");
	return -1;
}


/*
 *we need to write the warning log:
 *
 */
static int receive_write_warning_log(HttpStateData* httpState,int len)
{
	qihu_warning_config *cfg =(qihu_warning_config*) cc_get_mod_param(httpState->fwd->client_fd,mod);

	if(!httpState || !httpState->fwd || !httpState->fwd->request)
		return 0;
	// unsigned int fd=httpState->fwd->client_fd;

	StoreEntry *e=NULL;

	if(httpState->entry!=NULL )                                                                            
		e=httpState->entry;                                                                      
	else
		return 0;

	if( !e || !e->mem_obj || !e->mem_obj->reply)
		return 0;
	
	HttpReply *reply = e->mem_obj->reply;
	if(reply->sline.status == HTTP_NOT_FOUND) 
		return 0;

	int real_received_obj_length=(e->mem_obj->inmem_hi) - (e->mem_obj->reply->hdr_sz)+len;
	int header_content_length=e->mem_obj->reply->content_length;
	HttpHeaderEntry *user_agent_entry=httpHeaderFindEntry(&httpState->fwd->request->header,HDR_USER_AGENT);	
	char *agent_str="null";
	if(user_agent_entry)
		agent_str = (char*) strBuf(user_agent_entry->value); // warning
	char *client_ip;
	struct in_addr client_net_ip=httpState->fwd->request->client_addr;
	client_ip=inet_ntoa(client_net_ip);
	if(header_content_length>=0 && (header_content_length != real_received_obj_length) )
	{
		struct tm local_now;
		time_t now=time(NULL);
		localtime_r(&now,&local_now);
		FILE *fp;	
		if((fp=fopen(cfg->receive_log_path,"a+"))==NULL)
		{
			debug(149,0)("mod_qihu_warning ---->open receive_log_path:%s error\n",cfg->receive_log_path);
			return -1;
		}
		
		debug(149,0)("mod_qihu_warning ---->Start to write the log!!!!!\n");
		fprintf(fp,"%04d/%02d/%02d  %02d:%02d:%02d| Error Bad Object<--->real_received_obj_length: %d, Content_length: %d , Client_ip: %s ,url: %s, User-Agent: %s\n",
				(local_now.tm_year+1900),
				(local_now.tm_mon+1),
				local_now.tm_mday,
				local_now.tm_hour,
				local_now.tm_min,
				local_now.tm_sec,
				real_received_obj_length,
				header_content_length,
				client_ip,
				e->mem_obj->url,
				agent_str);	
		debug(149,0)("mod_qihu_warning ---->finished to write the receive_log!!!!!\n\n");
		httpState->fwd->flags.dont_retry = 1;
		fflush(fp);
		fclose(fp);
	}
	return 0;	
}



/*
static int send_write_warning_log(clientHttpRequest *http)
{
	char *log_type_str="none";
	int real_send_obj_length=0;
	int real_content_length	=0;
	qihu_warning_config *cfg =(qihu_warning_config*) cc_get_mod_param(http->conn->fd,mod);
	StoreEntry *entry    =NULL;
	
	if(http ==NULL)
		return 1;

	if(http->entry!=NULL )
		entry = http->entry;
	else if( http->old_entry !=NULL)
		entry = http->old_entry;
	else 
		return 1;
	
	if(entry->mem_obj == NULL || entry ->mem_obj->reply ==NULL)
		return 1;

	real_send_obj_length=http->out.offset - entry->mem_obj->reply->hdr_sz;

	if(http->request->flags.range&& http->reply->sline.status == HTTP_PARTIAL_CONTENT)
	{
		real_content_length =http->reply->content_length;
		debug(149,0)("Is range request,range_length:%d,real_send_length:%d\n",real_content_length,real_send_obj_length);
	}else{
		if(entry->mem_obj->object_sz >= 0)
			real_content_length = contentLen(entry);
		else
			real_content_length =entry->mem_obj->reply->content_length;
	}


	HttpHeaderEntry *user_agent_entry=httpHeaderFindEntry(&http->request->header,HDR_USER_AGENT);	
	char *agent_str="null";
	if(user_agent_entry)
		agent_str=strBuf(user_agent_entry->value);
	char *client_ip;
	struct in_addr client_net_ip=http->request->client_addr;
	client_ip=inet_ntoa(client_net_ip);
	//debug(149,0)("mod_qihu_warning ---->reply->hdr_sz:%d,mem->object_sz:%d\n",entry->mem_obj->reply->hdr_sz,entry->mem_obj->object_sz);
	
	if(real_content_length>=0 && (real_content_length != real_send_obj_length ) )
	{
		struct tm local_now;
		time_t now=time(NULL);
		localtime_r(&now,&local_now);
		FILE *fp;	
		if(http->reply->sline.status == HTTP_PARTIAL_CONTENT)
		{
			return 1;			
		}
		

		if((fp=fopen(cfg->send_log_path,"a+"))==NULL)
		{
			debug(149,0)("mod_qihu_warning ---->open send_log_path:%s error\n",cfg->send_log_path);
			return -1;
		}
		log_type x_log_type=http->log_type;	
		if( (x_log_type ==LOG_TCP_HIT) || (x_log_type == LOG_TCP_REFRESH_HIT) || (x_log_type == LOG_TCP_MEM_HIT) || (x_log_type==LOG_TCP_STALE_HIT) || (x_log_type== LOG_UDP_HIT)  )
			log_type_str ="HIT";
		else if((x_log_type == LOG_TCP_MISS) || (x_log_type == LOG_TCP_REFRESH_MISS) || (x_log_type == LOG_TCP_ASYNC_MISS) || (x_log_type == LOG_UDP_MISS))
			log_type_str ="MISS";
		debug(149,0)("mod_qihu_warning ---->Start to write the send_log!!!!!\n");
//		fprintf(stderr,"mod_qihu_warning ---->Start to write the send log!!!!!\n");
		fprintf(fp,"%04d/%02d/%02d  %02d:%02d:%02d| Error Bad Object<--->hit_or_miss:%s real_send_obj_length: %d,real_content_length: %d , Client_ip: %s ,url: %s, User-Agent: %s\n",
				(local_now.tm_year+1900),
				(local_now.tm_mon+1),
				local_now.tm_mday,
				local_now.tm_hour,
				local_now.tm_min,
				local_now.tm_sec,
				log_type_str,
				real_send_obj_length,
				real_content_length,
				client_ip,
				http->uri,
				agent_str);
		debug(149,0)("mod_qihu_warning ---->finished to write the log!!!!!\n\n");
		fflush(fp);
		fclose(fp);
	}
	return 0;	
}
*/








static int cleanup(cc_module *mod)
{
	//logfileClose(qihu_warninglog);
	//qihu_warninglog= NULL;
	return 0;
}

// module init 
int mod_register(cc_module *module)
{
	debug(149, 9)("(mod_360_warning_log) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	
//	module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
			func_sys_parse_param);
	//module->hook_func_private_set_client_keepalive = set_client_keepalive;
		
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanup);
	cc_register_hook_handler(HPIDX_hook_func_private_360_write_receive_warning_log,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_360_write_receive_warning_log),
			receive_write_warning_log);

/*cc_register_hook_handler(HPIDX_hook_func_private_360_write_send_warning_log,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_360_write_send_warning_log),
			send_write_warning_log);
*/

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
    else
		mod = module;
	return 0;
}

#endif


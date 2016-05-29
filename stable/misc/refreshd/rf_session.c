/*
 * $Id$
 * author : liwei(liwei@chinacache.com)
 * desc   : refreshd session management
 */

#include "refresh_header.h"

//for dir refresh pthread
extern bool stop;

extern void backtrace_info();

static struct list_head session_list;

static uint64_t server_url_count = 0;
rf_thread_stack thread_stack;

int dir_refresh_mode = -1;

void rf_session_init(){
	INIT_LIST_HEAD(&session_list);
	memset(&thread_stack,0,sizeof(rf_thread_stack));
}

void rf_url_free_one(rf_url_list *url){
	if(url->buffer != NULL){
		cc_free(url->buffer);
	}

	cc_free(url);
}
void rf_url_list_free(rf_url_list *list){
	rf_url_list *tmp = list;
	while(list != NULL){
		tmp = list;
		list = list->next;

		rf_url_free_one(tmp);
	}
}

void rf_session_free(rf_cli_session *sess){
	assert(sess != NULL);

	if(sess->params != NULL){
		if((sess->method == RF_FC_ACTION_DIR_EXPIRE) || (sess->method == RF_FC_ACTION_DIR_PURGE)){
			rf_cli_dir_params *params = (rf_cli_dir_params *)sess->params;
			if(params->report_address != NULL){
				cc_free(params->report_address);
			}
		}
		cc_free(sess->params);
	}

	if(sess->url_list != NULL){
		rf_url_list_free(sess->url_list);
	}

	if(sess->peer_ip != NULL){
		cc_free(sess->peer_ip);
	}

	cc_free(sess);
}

void rf_session_dest(){
	rf_cli_session *tmp;

	struct list_head *pos,*n;
	list_for_each_safe(pos,n,&session_list){
		tmp = list_entry(pos,rf_cli_session,i_list);
		rf_session_free(tmp);
	}
}

void rf_session_add(rf_cli_session *sess){
	assert(sess != NULL);

	return list_add_tail(&sess->i_list,&session_list);
}

void rf_session_del(rf_cli_session *sess){
	assert(sess != NULL);
	list_del(&sess->i_list);

	rf_session_free(sess);
}

/**
  * combination by config
  * FC5 的header_combination功能和url存多份功能都会修改掉原始的url，导致刷新不掉
  * 这里通过配置文件还原url，并加入到write_list，发送给squid
  */
int rf_session_url_header_combination2(int fd,rf_cli_session *sess,rf_url_list *url){
//	rf_client *rfc = &fd_table[fd];
//	if(rfc->db_handler == NULL)
//		return RF_OK;

	if(strstr(url->buffer,config.compress_delimiter) != NULL)
	{
		cclog(0, "warning... your url: %s involve the compress_delimiter: %s, it might be an error\n", url->buffer,config.compress_delimiter);
		return RF_OK;
	}

	char data[4096];
	char buffer[4096];
	char new_url[4096];
	memset(data,0,4096);
	memset(buffer,0,4096);

	strcpy(new_url,url->buffer);
	strcat(new_url,config.compress_delimiter);

	int len = rf_fc_gen_buffer(buffer,sess->method,new_url,sess->sessionid,NULL);
	rf_add_write_list(fd,buffer,len,RF_WL_ACTION_ADD_FIRST);
	cclog(4,"###header combination %s",sess->sessionid);

	int i=0;
	for(;i<config.compress_formats_count; i++){
		char *c = config.compress_formats[i];
		memset(new_url,0,4096);
		strcpy(new_url,url->buffer);
	        strcat(new_url,config.compress_delimiter);
		strcat(new_url,c);
		
		cclog(4,"get header combination url : %s",new_url);
		memset(buffer,0,4096);
		len = rf_fc_gen_buffer(buffer,sess->method,new_url,sess->sessionid,NULL);
		rf_add_write_list(fd,buffer,len,RF_WL_ACTION_ADD_FIRST);
		cclog(4,"###header combination %s",sess->sessionid);	
	}
	
	return RF_OK;
}

/*
 *	rf_get_fc_alive
 *	从fd_table里找到正在使用并且已经正常连接的FC
 */
int rf_get_fc_alive(int *array,int *count){
	int index = 0;
	int j = 0;

	for(; index < FD_MAX;index++){
		rf_client *rfc = &fd_table[index];

		if(rfc->inuse && (rfc->type == RF_CLIENT_FC) && rfc->ident){
			array[j++] = index;
		}
	}

	*count = j;

	return 0;
}

/*	
 *	rf_url_hostname
 *	从url中提取出hostname
 */
char * rf_url_hostname(const char *url,char *hostname){
	char *a = strstr(url,"//");
	if(a == NULL)
		return NULL;

	a += 2;

	strcpy(hostname,a);

	char *c = hostname;
	while(*c && *c != '/'){
		c++;
	}

	if(*c != '/')
		return NULL;

	*c = '\0';

	return hostname;
}

/*
 *	rf_session_url_process
 *	刷新掉原有的、以及组合以后的url
 */
int rf_session_url_process(rf_cli_session *sess){
	rf_url_list *url = sess->url_list;
	static char buffer[10240];

	int fc_array[128];
	int count = 0;
	//可能有多个Fc同时连接进来
	rf_get_fc_alive(fc_array,&count);

	sess->url_all_number = count *(sess->url_number)*(config.compress_formats_count + 2);
	cclog(1, "session %s  url_all_number %d\n", sess->sessionid,sess->url_all_number);


	int fd,i;
	while(url){
		if(! count){
			url->ret = 404;
		}
		else{
			for(i = 0; i < count;i++){
				fd = fc_array[i];

				memset(buffer,0,10240);

				if(strlen(url->buffer) >= 4096)
				{
					cclog(0, "session %s url is too large !!!", sess->sessionid);
					return RF_ERR;
				}

				int len = rf_fc_gen_buffer(buffer,sess->method,url->buffer,sess->sessionid,NULL);
				rf_add_write_list(fd,buffer,len,RF_WL_ACTION_ADD_FIRST);

				rf_session_url_header_combination2(fd,sess,url);
			}
		}

		url = url->next;
	}

	if(!count)
		return RF_ERR;

	return RF_OK;
}

/*
 *	rf_session_dir_response
 *	将目录刷新的收条立刻返回给刷新系统
 *	只是告诉刷新系统“收到了”，此时并不知道是否刷新成功
 *
 */
int rf_session_dir_response(rf_cli_session *sess){
	int len = 0;
	static char buffer[10240];
	memset(buffer,0,10240);

	len += sprintf(buffer + len,"%s",RF_XML_HEADER);
	len += sprintf(buffer + len,"<%s_response sessionid=\"%s\">",action_itoa(sess->method),sess->sessionid);
	len += sprintf(buffer + len,"<ret>200</ret>");
	len += sprintf(buffer + len,"</%s_response>",action_itoa(sess->method));
	
	char *http_header = rf_http_header(len);
	cclog(3,"%s%s",http_header,buffer);

	rf_add_write_list(sess->fd,http_header,strlen(http_header),RF_WL_ACTION_ADD_TAIL);
	rf_add_write_list(sess->fd,buffer,len,RF_WL_ACTION_ADD_TAIL);
	rf_add_write_list(sess->fd,NULL,0,RF_WL_ACTION_ADD_TAIL);
	return RF_OK;
}

/*
 *	rf_session_match_callback
 *
 */
int rf_session_match_callback(rf_client *rfc,rf_cli_session *sess,char *url){
	assert(sess != NULL);

	int len = 0;
	if(url)
		len += strlen(url);
	if(sess->sessionid)
		len += strlen(sess->sessionid);
	len += 32; // for method

	char *buffer = cc_malloc(len);

	int rf_len = rf_fc_gen_buffer(buffer,sess->method,url,sess->sessionid,NULL);
	int ret = rf_add_write_list(rfc->fd,buffer,rf_len,RF_WL_ACTION_ADD_TAIL);

	cc_free(buffer);

	return ret;
}

/*
 *	rf_session_dir_process
 *	目录刷新线程中，实际执行刷新操作
 */
void *rf_session_dir_process(void *para){
	int index = 0;
	int fc_num = 0;

	for(; index < FD_MAX ;index++){
		rf_client *rfc = &fd_table[index];

		if((rfc->type == RF_CLIENT_FC) && rfc->inuse && rfc->ident){
				fc_num++;
				rf_match_url(rfc,rf_session_match_callback);
		}

		if(stop == true)
			break;
	}

	if(fc_num == 0)
		cclog(0, "%s", "No FC has been connected ...");

	cclog(0,"%s","pthread finished ...");

	//normal exit
	if(stop != true)
	{
		cclog(5, "thread exit normal, and set all dir session pending, reset the thread time ...");
		int i = 0;
		for(;i<thread_stack.sess_count;i++){
			rf_cli_session *sess = thread_stack.sess_stack[i];
			if(sess->status != RF_SESSION_PENDDING)
				sess->status = RF_SESSION_PENDDING;
		}

		thread_stack.thread_begin = 0;
		thread_stack.thread_end = now;
	}
	else
	{
		cclog(5, "thread exit abnormal ...");
		stop = false;
	}

	return para;
}

/*
 *	rf_session_url_done
 *	完成url刷新,将刷新结果同步发送给刷新系统
 *
 */
int rf_session_url_done(rf_cli_session *sess){
	rf_client *rfc = &fd_table[sess->fd];
	int ret = RF_OK;

	if(! (rfc->inuse && (rfc->type == RF_CLIENT_CLI))){
		cclog(2,"%s has closed the connection before session done!!",sess->peer_ip);
		ret = RF_ERR_SESSION_FD_CLOSED;
		goto OUT;
	}

	int len = 0;
	static char buffer[102400];
	memset(buffer,0,102400);

	rf_url_list *ul = sess->url_list;

	len += sprintf(buffer + len,"%s\n",RF_XML_HEADER);
	len += sprintf(buffer + len,"<%s_response sessionid=\"%s\">\n",action_itoa(sess->method),sess->sessionid);

	while(ul){
		if(ul->ret == 0)
			ul->ret = 408; //timeout;

		if(ul->ret == 1000)
		{
			ul->ret = 200;
			cclog(3, "url: %s is not catched in FC\n", ul->buffer);
		}

		server_url_count++;

		len += sprintf(buffer + len,"<url_ret id=\"%" PRINTF_UINT64_T "\">%" PRINTF_UINT64_T "</url_ret>\n",ul->id,ul->ret);
		ul = ul->next;
	}

	len += sprintf(buffer + len,"</%s_response>",action_itoa(sess->method));
	
	char *http_header = rf_http_header(len);
	cclog(3,"%s%s",http_header,buffer);

	rf_add_write_list(sess->fd,http_header,strlen(http_header),RF_WL_ACTION_ADD_TAIL);
	rf_add_write_list(sess->fd,buffer,len,RF_WL_ACTION_ADD_TAIL);

	cclog(2,"%s from %s has been done!",action_itoa(sess->method),sess->peer_ip);

	ul = sess->url_list;
	cclog(0,"#### now  %d   sess begin %d\n",time(NULL),sess->begin);
	while(ul){
		cclog(1,"%s,%s,%s,%s,%"PRINTF_UINT64_T,sess->sessionid,ul->buffer,sess->peer_ip,action_itoa(sess->method),ul->ret);

		if(strcmp(ul->buffer, config.test_url))
			cc_access_log(0,"%s,%s,%s,%s,%"PRINTF_UINT64_T,sess->sessionid,ul->buffer,sess->peer_ip,action_itoa(sess->method),ul->ret);
		ul = ul->next;
	}

OUT:
	//close it!!
	if(!(rfc->flag & RF_CLI_FLAG_KEEPALIVE)){
			rf_add_write_list(sess->fd,NULL,0,RF_WL_ACTION_ADD_TAIL);
	}

	sess->status = RF_SESSION_FREE;
	return ret;
}

/*
 *	fd_async_connect
 *	目录刷新采用异步上报的机制，该函数连接异步上报的地址
 *
 */
static int fd_async_connect(const char *ip,uint16_t port,int *infd){
	cclog(4,"async connect ip : %s",ip);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if( fd < 0 ) {
		return -1;
	}

	struct sockaddr_in  maddress;
	memset(&maddress, 0, sizeof(struct sockaddr_in));
	if(inet_aton(ip,&maddress.sin_addr) == 0){
		cclog(1,"ip -> %s is invalid!!(%s)",ip,strerror(errno));
		close(fd);
		return -1;
	}

	maddress.sin_family = AF_INET;
	maddress.sin_port   = htons(port);

	int ret = fd_setnonblocking(fd);
	if(ret != 0){
		close(fd);
		return ret;
	}


	int n = 0;
	if( (n = connect(fd , (struct sockaddr *)&maddress, sizeof(struct sockaddr))) < 0) {
		if( errno != EINPROGRESS ) {
			close(fd);
			return -1;
		}
	}

	*infd = fd;
	rf_fd_init(fd);

	//for report fd type
	rf_client *rfc = &fd_table[fd];
	rfc->type = RF_CLIENT_REPORT;

	return 0;
}

/*
 *	rf_session_dir_done
 *	完成目录刷新，如果和上报地址的socket连接已经建立，则将刷新结果发送到上报地址
 *
 */
int rf_session_dir_done(rf_cli_session *sess){
	//int ret = -1;

	if(sess->async_fd <= 0){
		char *report_address = NULL;
		rf_cli_dir_params *param = (rf_cli_dir_params *)sess->params;
		if(param->report_address != NULL){
			report_address = param->report_address;
		}
		else{
			report_address = config.report_ip;
		}

		if(++(sess->report_times) > 5)
		{
			cclog(0, "free session: %s after refreshd has tried to send dir refresh_result for %d times", sess->sessionid, sess->report_times - 1);
			goto free_sess;
		}

		int ret = fd_async_connect(report_address,80,&sess->async_fd);
		if(ret != 0){
			cclog(0,"async_connect error ip : %s !",report_address);
		}

		sess->report_status = RF_SESSION_INIT;
		rf_epoll_add(sess->async_fd,EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR);
		return RF_OK;
	}

	if(sess->report_status == RF_SESSION_SUCCESS)
	{
		goto free_sess;
	}

	if((sess->report_status == RF_SESSION_FAILED) || (sess->report_status == RF_SESSION_CLOSE))
	{
		sess->async_fd = -1;
                return RF_OK;
        }
		

/*
//	if(! is_socket_alive(sess->async_fd)){
	ret = is_socket_alive(sess->async_fd);

	if(ret == EISCONN)
		return RF_OK;

	if((!ret))
	{	
		cclog(0,"async report fd do not alive : %d",sess->async_fd);
		disconnect_fd(sess->async_fd);
		sess->async_fd = -1;
		return RF_OK;
	}
*/
	if(sess->report_status == RF_SESSION_PACKED)
		return RF_OK;

	cclog(2,"build async report buffer, ip: [%s] , fd : [%d]",(((rf_cli_dir_params*)(sess->params))->report_address!=NULL)?((rf_cli_dir_params*)(sess->params))->report_address:config.report_ip, sess->async_fd);

//	rf_epoll_add(sess->async_fd,EPOLLOUT|EPOLLHUP|EPOLLERR);

	int len = 0;
	static char buffer[10240];
	memset(buffer,0,10240);

	len += sprintf(buffer + len,"%s\n",RF_XML_HEADER);
	len += sprintf(buffer + len,"<%s_async_result sessionid=\"%s\">\n",action_itoa(sess->method),sess->sessionid);
	len += sprintf(buffer + len,"<url_number>%" PRINTF_UINT64_T "</url_number>\n",sess->url_number);
	len += sprintf(buffer + len,"<success>%" PRINTF_UINT64_T "</success>\n",sess->success);
	len += sprintf(buffer + len,"<fail>%" PRINTF_UINT64_T "</fail>\n",sess->fail);
	len += sprintf(buffer + len,"<begin_time>%lu</begin_time>\n",sess->begin);
	len += sprintf(buffer + len,"<end_time>%lu</end_time>\n",sess->end);
	len += sprintf(buffer + len,"</%s_async_result>",action_itoa(sess->method));
	
	char *http_header = rf_http_header_post(len);
	cclog(3,"%s%s",http_header,buffer);

	rf_add_write_list(sess->async_fd,http_header,strlen(http_header),RF_WL_ACTION_ADD_TAIL);
	rf_add_write_list(sess->async_fd,buffer,len,RF_WL_ACTION_ADD_TAIL);

	if(sess->log != 1)
	{
		cclog(1,"%s,%s,%s,%s,%" PRINTF_UINT64_T "/%" PRINTF_UINT64_T,sess->sessionid,sess->url_list->buffer,sess->peer_ip,action_itoa(sess->method),sess->success,sess->url_number);
		cclog(2,"%s from %s has been done!",action_itoa(sess->method),sess->peer_ip);
	}

	sess->log = 1;

	//close it!!
	//rf_add_write_list(sess->async_fd,NULL,0,RF_WL_ACTION_ADD_TAIL);

	sess->report_status = RF_SESSION_PACKED;

	return RF_OK;

free_sess:
	sess->status = RF_SESSION_FREE;
	return RF_OK;
}

/*
 *	rf_set_similar_session
 *	session里可能存在相似的目录刷新任务
 *	目录刷新成功后，从session中找相似任务，如果找到，则将相似的session也置为成功
 *
 */
int rf_set_similar_session(rf_cli_session *sess){
	char *url = sess->url_list->buffer;
	struct list_head *pos;
	rf_cli_session *tmp = NULL;

	if(url == NULL)
		return 0;

	list_for_each(pos, &session_list){
		tmp = list_entry(pos,rf_cli_session,i_list);
		if(((tmp->method == RF_FC_ACTION_DIR_EXPIRE) || (tmp->method == RF_FC_ACTION_DIR_PURGE))&& (tmp->status == RF_SESSION_SETUP)){
			if(tmp->url_list->buffer == NULL)
				continue;

			//skip itself
			if(tmp == sess)
				continue;

			if(strncasecmp(url,tmp->url_list->buffer,strlen(url)) == 0){
				tmp->url_number = sess->url_number;
				tmp->success = sess->success;
				tmp->fail = sess->fail;
				tmp->end = sess->end;
				tmp->status = RF_SESSION_SYNC;

				int fc_array[128];
                                int count = 0;
                                int len;
                                int i;
                                int fd;
                                char buffer[10240];

                                rf_get_fc_alive(fc_array, &count);

                                for(i = 0; i < count; i++)
                                {
                                        fd = fc_array[i];
                                
                                        memset(buffer,0,10240);
                                        len = rf_fc_gen_buffer(buffer,RF_FC_ACTION_DIR_REFRESH_FINISH,tmp->url_list->buffer,tmp->sessionid,NULL);
                                        rf_add_write_list(fd,buffer,len,RF_WL_ACTION_ADD_TAIL);
                                }       

				cc_access_log(0,"%s,%s,%s,%s,%" PRINTF_UINT64_T "/%" PRINTF_UINT64_T,tmp->sessionid,tmp->url_list->buffer,tmp->peer_ip,action_itoa(tmp->method),tmp->success,tmp->url_number);

				cclog(2,"session(%s) from %s,%s,%s has been set done by session(%s,%s)!",tmp->sessionid,tmp->peer_ip,action_itoa(tmp->method),tmp->url_list->buffer,sess->sessionid,url);
			}
		}
	}

	return RF_OK;
}

/*
 *	rf_session_one_check
 *	检查、驱动session的状态变化
 *
 */
int rf_session_one_check(rf_cli_session *sess){
	switch (sess->status) {
		//新建立的session
		case RF_SESSION_NEW:
			if(sess->method == RF_FC_ACTION_URL_PURGE || 
					sess->method == RF_FC_ACTION_URL_EXPIRE){
				int ret = rf_session_url_process(sess);
				if(ret != RF_OK){
					cclog(0,"no FC connected, rf_session_url_process error!!");
					sess->status = RF_SESSION_DONE;
					return ret;
				}

				sess->status = RF_SESSION_PENDDING;
			}
			
			if(sess->method == RF_FC_ACTION_DIR_PURGE || 
					sess->method == RF_FC_ACTION_DIR_EXPIRE){

				/********** for realtime dir refresh **********/
                                int fc_array[128];
                                int count = 0;
                                int len;
                                int fd;
                                int i;
                                char buffer[10240];

                                rf_get_fc_alive(fc_array, &count);

                                rf_cli_dir_params *dp = (rf_cli_dir_params *) sess->params;
                                if(dp->action == 0)
                                {
					if(sess->method == RF_FC_ACTION_DIR_EXPIRE)
                                        	dir_refresh_mode = RF_FC_ACTION_DIR_EXPIRE_REFRESH_PREFIX_START;
					else
						dir_refresh_mode = RF_FC_ACTION_DIR_PURGE_REFRESH_PREFIX_START;
                                }
                                else
                                {

					if(sess->method == RF_FC_ACTION_DIR_EXPIRE)
                                        	dir_refresh_mode = RF_FC_ACTION_DIR_EXPIRE_REFRESH_WILDCARD_START;
					else
						dir_refresh_mode = RF_FC_ACTION_DIR_PURGE_REFRESH_WILDCARD_START;
                                }

                                for(i = 0; i < count; i++)
                                {
                                        fd = fc_array[i];

                                        memset(buffer,0,10240);
                                        len = rf_fc_gen_buffer(buffer,dir_refresh_mode,sess->url_list->buffer,sess->sessionid,(int*)&config.dir_wait_timeout);
                                        rf_add_write_list(fd,buffer,len,RF_WL_ACTION_ADD_FIRST);
                                }
                                /********** for realtime dir refresh **********/

				rf_session_dir_response(sess);

				sess->status = RF_SESSION_SETUP;
			}

			break;
		case RF_SESSION_SETUP:
			//dir waiting for dispatch
			break;
		case RF_SESSION_DISPATCH:
			//dir waiting for pthread finished!
			break;
		//刷新任务已经发送给squid，等待squid返回结果
		case RF_SESSION_PENDDING:
			if(sess->method == RF_FC_ACTION_URL_PURGE || 
					sess->method == RF_FC_ACTION_URL_EXPIRE){
				if(sess->url_all_number == sess->success + sess->fail)
				{
					cclog(2,"RF_SESSION_PENDDING->RF_SESSION_DONE \n");
					sess->status = RF_SESSION_DONE;
				}
				if((now - sess->begin) > config.url_wait_timeout) {
					cclog(0,"session(%s) from %s is not return from fc in %d second!",action_itoa(sess->method),sess->peer_ip, config.url_wait_timeout);
					sess->status = RF_SESSION_DONE; // discard the session !!
				}
			}

			//notice the FC that all the refresh request has been submitted !!!
			if(sess->method == RF_FC_ACTION_DIR_PURGE || 
					sess->method == RF_FC_ACTION_DIR_EXPIRE){

                                int fc_array[128];
                                int count = 0;
                                int len;
                                int i;
                                int fd;
                                char buffer[10240];

                                rf_get_fc_alive(fc_array, &count);

                                for(i = 0; i < count; i++)
                                {
                                        fd = fc_array[i];

                                        memset(buffer,0,10240);
                                        len = rf_fc_gen_buffer(buffer,RF_FC_ACTION_DIR_REFRESH_FINISH,sess->url_list->buffer,sess->sessionid,NULL);
                                        rf_add_write_list(fd,buffer,len,RF_WL_ACTION_ADD_TAIL);
                                }
                        
				sess->status = RF_SESSION_PENDDING2;
				sess->last_active = now;
/*
				if(sess->url_number == 0 || sess->url_number == (sess->success + sess->fail)){
					cclog(5,"dir purge session done\n");
					sess->status = RF_SESSION_DONE;
				}

				//目录刷新处于pendding状态，但是120s内却没有收到squid返回过来的刷新结果
				//则认为session处于异常状态，将该session置为done，等待上报结果
				if(sess->last_active && ((now - sess->last_active) > 120)){
					cclog(0,"session(%s) is not response in 120s!",sess->sessionid);
					sess->status = RF_SESSION_DONE;
				}

				if((now + 1 - sess->begin) % 600 == 0){
					cclog(0,"session(%s) from %s ,dir : %s,url_number : %" PRINTF_UINT64_T ",success : %" PRINTF_UINT64_T ",failed : %"PRINTF_UINT64_T ,action_itoa(sess->method),sess->peer_ip,sess->url_list->buffer,sess->url_number,sess->success,sess->fail);
				}
*/
			}

			break;
		//for dir refresh, waiting all FC return the dir refresh result
		case RF_SESSION_PENDDING2:
			if(sess->success == sess->url_number)
			{
				cclog(2, "session(%s) RF_SESSION_DONE\n",sess->sessionid);
				sess->status = RF_SESSION_DONE;
			}
			else
			{
			//	if(sess->last_active && ((now - sess->last_active) > 120))
				if(now - sess->last_active > config.dir_wait_timeout2)
				{
					cclog(0, "session(%s) is not response in %ds after all request sent to FC!", sess->sessionid, config.dir_wait_timeout2);
					sess->status = RF_SESSION_DONE;
				}
			}

			break;
		//session完成，等待上报结果
		case RF_SESSION_DONE:
			sess->end = now;
			sess->fail = sess->url_all_number - sess->success;


			if(sess->method == RF_FC_ACTION_URL_PURGE || 
					sess->method == RF_FC_ACTION_URL_EXPIRE){
				rf_session_url_done(sess);
			}

			if(sess->method == RF_FC_ACTION_DIR_PURGE || 
					sess->method == RF_FC_ACTION_DIR_EXPIRE){
					cclog(3,"session(%s) from %s DONE!,dir : %s,url_number : %" PRINTF_UINT64_T ",success : %" PRINTF_UINT64_T ",failed : %"PRINTF_UINT64_T,action_itoa(sess->method),sess->peer_ip,sess->url_list->buffer,sess->url_number,sess->success,sess->fail);
					cc_access_log(0,"%s,%s,%s,%s,%" PRINTF_UINT64_T "/%" PRINTF_UINT64_T,sess->sessionid,sess->url_list->buffer,sess->peer_ip,action_itoa(sess->method),sess->success,sess->url_number);

				sess->status = RF_SESSION_SYNC;
				rf_set_similar_session(sess);
			}

			break;
		//目录刷新需要首先建立socket连接，该状态表明连接已经建立，可以发送了
		case RF_SESSION_SYNC:
			//for dir refresh
			if((now - sess->last_active) < 5){	//try report it's result every 5s
			       break;
			}else{
				sess->last_active = now;
			}

			//time too long ?
			if((now - sess->end) > 60*5)
			{
				cclog(0, "report the dir refresh result timeout, free the session now!");
				if(sess->async_fd > 0)
				{
					disconnect_fd(sess->async_fd);
                			sess->async_fd = -1;
				}

				sess->status = RF_SESSION_FREE;
				break;
			}

			rf_session_dir_done(sess);
			break;
		//session处理完毕，可以释放
		case RF_SESSION_FREE:
                        /********** for realtime dir refresh **********/
/*
			if(sess->method == RF_FC_ACTION_DIR_PURGE ||
                                        sess->method == RF_FC_ACTION_DIR_EXPIRE){

                                int fc_array[128];
                                int count = 0;
                                int len;
                                int i;
                                int fd;
                                char buffer[4024];

                                rf_get_fc_alive(fc_array, &count);

                                for(i = 0; i < count; i++)
                                {
                                        fd = fc_array[i];

                                        memset(buffer,0,1024);
                                        len = rf_fc_gen_buffer(buffer,RF_FC_ACTION_REALTIME_DIR_REFRESH_FINISH,sess->url_list->buffer,NULL,NULL);
                                        rf_add_write_list(fd,buffer,len,RF_WL_ACTION_ADD_TAIL);
                                }
                        }
*/
                        /********** for realtime dir refresh **********/

			cclog(3,"free session , id: %s,action: %d,status: %d",sess->sessionid,sess->method,sess->status);
			rf_session_del(sess);
			break;
		default :
			cclog(0,"can not process sess status : %d",sess->status);
			sess->status = RF_SESSION_DONE;
			break;
	};

	return RF_OK;
}

/*
 *	rf_start_thread
 *	启动目录刷新线程
 */
static int rf_start_thread(){
	thread_stack.thread_begin = now;
	thread_stack.thread_end = 0;
	stop = false;

	pthread_t ptid;
	if(pthread_create(&ptid,NULL,rf_session_dir_process,NULL) != 0){
		cclog(0,"Start pthread failed!");
		thread_stack.thread_begin = 0;
		thread_stack.thread_end = now;
		return RF_ERR_SESSION_THREAD;
	}                       

	/********** for cancel thread **********/
	/*
	if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0)
	{
		cclog(0, "set pthread to THREAD_CANCEL_ENABLE state failed!");
		thread_stack.thread_begin = 0;
                thread_stack.thread_end = now;
                return RF_ERR_SESSION_THREAD;
        }

	if(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) != 0)
	{
		cclog(0, "set pthread to THREAD_CANCEL_DEFFERED state failed!");
                thread_stack.thread_begin = 0;
                thread_stack.thread_end = now;
                return RF_ERR_SESSION_THREAD;
        }
	*/
	/********** for cancel thread **********/

	pthread_detach(ptid);   
	thread_stack.ptid = ptid;

	cclog(0,"pthread %lu started!",ptid);
	int i = 0;
	for(;i < thread_stack.sess_count;i++){
		cclog(2,"pthread %lu,%s,%s",ptid,action_itoa(thread_stack.sess_stack[i]->method),thread_stack.sess_stack[i]->url_list->buffer);
	}

	return RF_OK;
}

/*
 *	rf_session_check
 */
int rf_session_check(){
	struct list_head *pos,*n;
	rf_cli_session *tmp = NULL;

	bool b_thread = false;
	if((thread_stack.thread_begin == 0) && (now - thread_stack.thread_end) > 120){
		b_thread = true;
		thread_stack.sess_count = 0;
	}

	int sess_c = 0;
	list_for_each_safe(pos,n,&session_list){
		tmp = list_entry(pos,rf_cli_session,i_list);
		cclog(5,"session check , id: %s,action: %d,status: %d",tmp->sessionid,tmp->method,tmp->status);

		rf_session_one_check(tmp);
		sess_c++;
	}
	
	list_for_each_safe(pos,n,&session_list){
		tmp = list_entry(pos,rf_cli_session,i_list);

		if(b_thread && (tmp->method == RF_FC_ACTION_DIR_EXPIRE || tmp->method == RF_FC_ACTION_DIR_PURGE) && (tmp->status == RF_SESSION_SETUP)){
			char *url = tmp->url_list->buffer;
			int i = 0;
			bool a_flag = true;

			for(; i < thread_stack.sess_count;i++){
				char *t_url = thread_stack.sess_stack[i]->url_list->buffer;
				if(strncasecmp(t_url,url,strlen(t_url)) == 0){
					a_flag = false;
					break;
				}
			}

			if(a_flag && (thread_stack.sess_count < SESS_STACK_SIZE - 1)){
				thread_stack.sess_stack[thread_stack.sess_count++] = tmp;
				tmp->status = RF_SESSION_DISPATCH;
			}
		}
	}

	cclog(4,"session count : %d",sess_c);

	if(b_thread && thread_stack.sess_count){
		rf_start_thread();
	}

	if(thread_stack.thread_begin && !thread_stack.thread_end){
		if((now - thread_stack.thread_begin) > config.dir_wait_timeout){
			stop = true;
			// sess status will be set to RF_SESSION_DONE in rf_session_one_check function
			cclog(0, "thread process timeout, cancel the thread ...");
			cclog(5, "set all dir session pending, reset the thread time ...");
			int i = 0;
			for(;i<thread_stack.sess_count;i++){
				tmp = thread_stack.sess_stack[i];
				if((tmp != NULL) && (tmp->status != RF_SESSION_PENDDING))
					tmp->status = RF_SESSION_PENDDING;
			}

			thread_stack.thread_end = now;
			thread_stack.thread_begin = 0;
		}
	}

#ifdef DEBUG
	static time_t last_p = 0;
	static uint64_t last_server_url_count = 0;
	if((now - last_p) > 5){
		last_p = now;
		cclog(5,"server count : %"PRINTF_UINT64_T,server_url_count);
		cclog(5,"last server count : %"PRINTF_UINT64_T,last_server_url_count);
		cclog(5,"server count  per 5second: %"PRINTF_UINT64_T,server_url_count - last_server_url_count);

		last_server_url_count = server_url_count;
	}
#endif

	return RF_OK;
}

rf_cli_session *rf_find_session(char *sessionid){
	struct list_head *pos;
	rf_cli_session *tmp = NULL;

	list_for_each(pos, &session_list){
		tmp = list_entry(pos,rf_cli_session,i_list);
		if(strcmp(tmp->sessionid,sessionid) == 0)
			return tmp;
	}

	return NULL;
}

/*
 *	rf_set_session_url_ret
 *	session即使是url刷新，但也有可能包含多个url，squid返回的结果需要与url对应
 *	该函数找到对应关系，并置成相应结果
 *
 */
int rf_set_session_url_ret(char *url,char *sessionid,int ret){

	int len = 0;
	char *pos;

	pos = strstr(url, config.compress_delimiter);
	
	len = ((pos == NULL)?strlen(url):pos - url);  

	rf_cli_session *sess = rf_find_session(sessionid);

	//FIXME : Do not find session!!
	if(sess == NULL){
		//assert(0);
		cclog(0,"Do not find session : %s !![url : %s]",sessionid,url);
		return RF_OK;
	}

	rf_url_list *tmp_url = sess->url_list;

	while(tmp_url)
	{
		if(strncmp(tmp_url->buffer, url, strlen(tmp_url->buffer)) == 0) 
		{	
			cclog(2,"in session [%s] list sessid [%s]\n",sessionid,sess->sessionid);
			cclog(2,"in url=[%s] list url=[%s]\n", url,tmp_url->buffer);
			if(strlen(tmp_url->buffer) != strlen(url))
			{
				if((url[strlen(tmp_url->buffer)]) && (url[strlen(tmp_url->buffer)]!='@'))
				{
					tmp_url = tmp_url->next;	
					continue;
				}
			}
			if(strcmp(tmp_url->buffer, url) == 0)
			{
					tmp_url->ret = ret;
					cclog(2,"ret= %d  tmpret=%d", ret,tmp_url->ret);
			}
			sess->success ++;
			break;
		}
		
		tmp_url = tmp_url->next;		
	}

	cclog(2,"the sess url_numbuer: %d, recved url_number: %d\n", sess->url_all_number,sess->success + sess->fail);
	cclog(2,"failed url_number: %d\n", sess->fail);

	if(sess->url_all_number == (sess->success + sess->fail))
	{
		sess->status = RF_SESSION_DONE;
		cclog(2,"%s","////////////////////sess done//////////////////////\n");
	}

	return RF_OK;
}


/*
 *	rf_set_session_dir_ret
 *	更新目录刷新的结果
 */
int rf_set_session_dir_ret(char *sessionid,int ret){
	rf_cli_session *sess = rf_find_session(sessionid);

	if(sess == NULL){
		cclog(1,"Do not find session : %s !!",sessionid);
		return RF_OK;
	}

	if(sess->status == RF_SESSION_PENDDING2)
	{
		sess->success += ret;
	}

	if(sess->success == sess->url_number)
	{
		cclog(3,"%s","dir refresh end,set the sess done\n");
		sess->status = RF_SESSION_DONE;
	}

	sess->last_active = now;

	return RF_OK;
}

//for report dir refresh result
int rf_set_session_report_status(int fd, int status){
        struct list_head *pos;
        rf_cli_session *tmp = NULL;

        list_for_each(pos, &session_list){
                tmp = list_entry(pos,rf_cli_session,i_list);
                if(((tmp->method == RF_FC_ACTION_DIR_EXPIRE) || (tmp->method == RF_FC_ACTION_DIR_PURGE))&& (tmp->status == RF_SESSION_SYNC))
		{
			if(tmp->async_fd == fd)
			{
				tmp->report_status = status;

				cclog(3, "find the session,%s,%s,%s,%s",tmp->sessionid,tmp->url_list->buffer,tmp->peer_ip,action_itoa(tmp->method)); 
				return RF_OK;
			}
		}
	}
	cclog(2, "sess with report fd: %d status need be changed to %d, but can't find it in session_list", fd, status);        
	return -1;
}

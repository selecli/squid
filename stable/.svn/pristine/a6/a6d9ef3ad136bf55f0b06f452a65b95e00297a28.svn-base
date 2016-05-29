/*
 * $Id$
 * author : liwei(liwei@chinacache.com)
 * desc   : process request from fc
 */

#include "refresh_header.h"
/*
 * 	rf_notify_squid_verify
 *	为了保证squid与refreshd里的数据一致性,需要定期进行校验
 *	校验过程时候：signal -> refreshd -> squid -> refreshd
 *
 */
void rf_notify_squid_verify(){
	int index = 0;
	for(; index < FD_MAX;index++){
		rf_client *rfc = &fd_table[index];

		if(rfc->inuse && (rfc->type == RF_CLIENT_FC) && rfc->ident){
			if(rfc->status == RF_CLIENT_VERIFY_START)
				continue;

			char buffer[1024];
			memset(buffer,0,1024);
			int len = rf_fc_gen_buffer(buffer,RF_FC_ACTION_VERIFY_START,NULL,NULL,NULL);
			rf_add_write_list(index,buffer,len,RF_WL_ACTION_ADD_TAIL);
		}
	}
}

/**
  * Init the db_handler_bak and change the status of rfc
  */
int rf_url_verify_start(int fd,char *squid_id){
	cclog(0,"url verify start !");
	rf_client *rfc = &fd_table[fd];
	rfc->status = RF_CLIENT_VERIFY_START;

	check_db_bak_handlers(rfc, squid_id);

	return RF_OK;
}

/**
  * drop the old db_file and make the db_file_bak on use
  */
int rf_url_verify_finish(int fd,char *squid_id){
	rf_client *rfc = &fd_table[fd];
	rfc->status = RF_CLIENT_VERIFY_FINISH;

	int i;

	for(i = 0; i < db_num; i++)
	{
		if(rfc->db_bak_handlers[i] != NULL)
		{
			rf_flush(rfc->db_bak_handlers[i]);
			rf_db_close(rfc->db_bak_handlers[i]);
			rfc->db_bak_handlers[i] = NULL;
		}

		if(rfc->db_handlers[i] != NULL)
		{
			rf_db_close(rfc->db_handlers[i]);
			rfc->db_handlers[i] = NULL;
		}

		char *db_file_bak = rf_get_db_bak_file(squid_id, i);
		char *db_file = rf_get_db_file(squid_id, i);

		char cmd_buff[1024];
		memset(cmd_buff,0,1024);
		sprintf(cmd_buff,"%s.%lu",db_file,now);
		if(0 != rename(db_file,cmd_buff))
			cclog(0, "%s rename to : %s failed, %s !", db_file, cmd_buff, strerror(errno));
		cclog(2,"%s rename to : %s",db_file,cmd_buff);

		int ret;
		ret = unlink(cmd_buff);
		if(ret != 0)
			cclog(0,"unlink : %s error ! : %s",cmd_buff,strerror(errno));

		ret = rename(db_file_bak,db_file);
		if(ret != 0){
			cclog(0,"rename : %s",strerror(errno));
		}

		rfc->db_handlers[i] = get_db_handler(rfc->db_files[i]);
	}

	cclog(0,"finish url verify !");

	return RF_OK;
}
int rf_fc_cmd(int fd,rf_fc_input *input){
	cclog(5,"%s action ,url:%s",action_itoa(input->action),input->url);

	rf_client *rfc = &fd_table[fd];
	int ret = 0;

	check_db_handlers(rfc, input->squid_id);
/*
	if(rfc->db_handler == NULL){
		if(rfc->db_file == NULL){
			char *db_file = rf_get_db_file(input->squid_id);
			rfc->db_file = cc_malloc(strlen(db_file) + 1);
			strcpy(rfc->db_file,db_file);

			cclog(3,"fd(%d),db_file : %s",rfc->fd,rfc->db_file);
		}

		rfc->db_handler = get_db_handler(rfc->db_file);
	}
*/
	switch(input->action){
		case RF_FC_ACTION_INFO:
			ret = RF_OK;
			break;
		case RF_FC_ACTION_ADD:
			//Added by sw for is_dir_refresh
			if(!config.is_dir_refresh){
				break;
			}

			int i;

			if(config.if_hash_db)
				i = get_url_filenum(input->url);
			else
				i = 0;

			if(rfc->status == RF_CLIENT_VERIFY_START)
				ret = rf_add_url_real(rfc->db_bak_handlers[i],input->url,strlen(input->url));
			else
				ret = rf_add_url(rfc->db_handlers[i],input->url);
			break;
		case RF_FC_ACTION_VERIFY_START:
			if(rfc->status == RF_CLIENT_VERIFY_START)
			{
				ret = RF_OK;
				break;
			}
			ret = rf_url_verify_start(fd,input->squid_id);
			break;
		case RF_FC_ACTION_VERIFY_FINISH:
			if(rfc->status == RF_CLIENT_VERIFY_START)
				ret = rf_url_verify_finish(fd,input->squid_id);
			else
				ret = RF_OK;
			break;
		case RF_FC_ACTION_URL_PURGE_RET:
		case RF_FC_ACTION_URL_EXPIRE_RET:
			//input == "Squid id \0action\0url\0session,ret\0\n"
			//input->others == "session,ret"
			if(input->others != NULL){
				char *c = input->others;
				while(*c != ',') c++;
				*c++ = '\0';
				ret = rf_set_session_url_ret(input->url,input->others,atoi(c));
			}
			break;
		case RF_FC_ACTION_DIR_REFRESH_RESULT:
			if(input->others != NULL){
				char *c = input->others;
				while(*c != ',') c++;
				*c++ = '\0';
				ret = rf_set_session_dir_ret(input->others,atoi(c));
			}
			break;
		default:
			cclog(2,"can not process action :%s",action_itoa(input->action));
			break;
	}

	if(ret != RF_OK){
		cclog(0,"%s url process error!",action_itoa(input->action));
	}

	return ret;
}

static int rf_fc_parseline(int fd,char *line){
	char *a,*b,*c,*d;
	a = line;
	b = a + strlen(a) + 1;
	c = b + strlen(b) + 1;
	d = c + strlen(c) + 1;

	cclog(5,"squid id : %s,action : %s,url : %s",a,b,c);
//cclog(0,"recv squid --------> squidid: %s, action: %s, url: %s, others: %s others_len: %d\n", a, b, c, d, strlen(d));

	if((! isdigit(*b)) || !strlen(a) ){
		cclog(0,"recv text format error!,fd: %d",fd);
		return RF_ERR_FC_BUFFER;
	}

	if(rfc_current->ident == NULL){
		rfc_current->ident = cc_malloc(strlen(a) + 1);
		strcpy(rfc_current->ident,a);
	}

	rf_fc_input *sin = cc_malloc(sizeof(rf_fc_input));

	sin->squid_id = a;
	sin->action = strtol(b, (char **)NULL, 10);

	if(strlen(c))
		sin->url = c;

	if(strlen(d))
		sin->others = d;

	rf_fc_cmd(fd,sin);
	cc_free(sin);
	return RF_OK;
}

int rf_fc_process(int fd){
	rf_client *rfc = &fd_table[fd];
	assert(rfc);

	static char buffer[BUFFER_SIZE];
	int i,a;
	i = a = 0;

	for(;i<rfc->read_buff_len;i++){
		if(rfc->read_buff[i] == '\n'){
			memset(buffer,0,BUFFER_SIZE);
			memcpy(buffer,rfc->read_buff + a, i - a);

			while(rfc->read_buff[i] == '\n') i++;

			int ret = rf_fc_parseline(fd,buffer);
			if(ret != RF_OK){
				cclog(0,"rf_fc_parseline error!");
				a = i;
				continue;
			}

			a = i;
		}
	}

	if(i >= rfc->read_buff_len  && (a != rfc->read_buff_len)){
		i = rfc->read_buff_len;
		memmove(rfc->read_buff,rfc->read_buff + a,i - a);
		rfc->read_buff_len = i - a;
	}
	else{
		rfc->read_buff_len = 0;
	}

	return RF_OK;
}


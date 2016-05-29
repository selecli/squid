/*
 * $Id$
 * author : liwei(liwei@chinacache.com)
 * desc   : process request from client(not fc)
 */

#include "refresh_header.h"

static int rf_cli_parse_http_header(rf_client *rfc,char *buffer){
	char *li = strtok(buffer,"\r\n");
	while((li = strtok(NULL,"\r\n")) != NULL){
		if(strncmp(li,"Keep-Alive",strlen("Keep-Alive")) == 0){
				rfc->flag |= RF_CLI_FLAG_KEEPALIVE;
				cclog(3,"set client to keep_alive : %d",rfc->flag);
		}
		else if(strncmp(li,"Content-Length",strlen("Content-Length")) == 0){
			while(*li++ != ':'){
				if(*li == '\0')
					break;
			}

			rfc->content_length = strtol((const char *)li, (char **)NULL, 10);
		}
	}

	if(rfc->content_length <= 0)
		return RF_ERR;

	return RF_OK;
}

static void free_xml_buff(rf_client *rfc){
	cc_free(rfc->xml_buff);
	rfc->xml_buff = NULL;
	rfc->xml_buff_len = 0;
}

int  rf_cli_process(int fd){
	rf_client *rfc = &fd_table[fd];
	assert(rfc);

	int ret = 0;
	char *c = rfc->read_buff;
	int left_len = rfc->read_buff_len;
	//The fd's first time entering this method
	if(rfc->content_length == 0){
		static char http_header[10240];
		c = strstr(rfc->read_buff,"\r\n\r\n");

		if(c == NULL){
			cclog(0,"get wrong message from remote client !!");
			return RF_ERR_CLI_MESSAGE;
		}

		memset(http_header,0,10240);

		if(c - rfc->read_buff > 10239)
		{
			cclog(0, "http_header is too large !!!");
			return RF_ERR_CLI_MESSAGE;
		}
		memcpy(http_header,rfc->read_buff,c - rfc->read_buff);

		cclog(5,"receive http header : %s\n",http_header);

		ret = rf_cli_parse_http_header(rfc,http_header);
		if(ret != RF_OK){
			rfc->read_buff_len = 0;
			return ret;
		}

		c += 4; //skip "\r\n\r\n"

		assert(rfc->xml_buff == NULL);
		assert(rfc->xml_buff_len == 0);
		assert(rfc->content_length > 0);

		rfc->xml_buff = cc_malloc(rfc->content_length * sizeof(char) + 1);
		//It might be easier to understand :  rfc->read_buff + rfc->read_buff_len - c
		left_len = rfc->read_buff_len - (c - rfc->read_buff);
	}

	assert(rfc->xml_buff != NULL);

	if(left_len >= rfc->content_length){
		//if we have the full body
		memcpy(rfc->xml_buff + rfc->xml_buff_len ,c,rfc->content_length);
		rfc->xml_buff_len += rfc->content_length;
		//rfc->content_length = 0;        //modified by zh
		//move the content after content-length to the beginning
		memmove(rfc->read_buff,c + rfc->content_length,left_len - rfc->content_length);
		rfc->read_buff_len = left_len - rfc->content_length;
		rfc->content_length = 0;
	}
	else{
		//if we don't have the full body , copy the part of content to xml_buf
		memcpy(rfc->xml_buff + rfc->xml_buff_len,c,left_len);
		rfc->content_length -= left_len;
		rfc->read_buff_len = 0;
		rfc->xml_buff_len += left_len;
		return RF_OK;
	}


	c =rfc->xml_buff;
	cclog(5,"receive xml : %s",c);
	cclog(5,"xml_buff_len : %d",rfc->xml_buff_len);

	rf_cli_session *sess = cc_malloc(sizeof(rf_cli_session));
	ret = rf_xml_parse(sess,c,rfc->xml_buff_len);
	if(ret != RF_OK){
		goto failed;
	}

	if(rf_find_session(sess->sessionid) != NULL){
		cclog(0,"session(%s) has been existed!",sess->sessionid);
		ret = RF_ERR_XML_SESSIONID;
		goto failed;
	}

	sess->fd = fd;

	char *peer_ip = get_peer_ip(fd);
	if(peer_ip != NULL){
		sess->peer_ip = cc_malloc(strlen(peer_ip) + 1);
		strcpy(sess->peer_ip,peer_ip);
	}

	sess->status = RF_SESSION_NEW;
	sess->begin = time(NULL);
	sess->last_active = sess->begin;

	free_xml_buff(rfc);
	rf_session_add(sess);
	return RF_OK;
failed:
	free_xml_buff(rfc);
	rfc->read_buff_len = 0;
	cc_free(sess);
	return ret;
}

/*
 * $Id$
 * author : liwei(liwei@chinacache.com)
 * desc   : async send list
 */

#include "refresh_header.h"

//free write buffer
void rf_wb_free(rf_write_buffer *wb){
	assert(wb != NULL);

	if(wb->buffer != NULL)
		cc_free(wb->buffer);
	cc_free(wb);
}

//free write list
void rf_wl_free(rf_write_list *wl){
	if((wl == NULL) || (wl->first == NULL))
		return;

	rf_write_buffer *tm0,*tm1;
       	tm0 = wl->first;

	while(tm0){
		tm1 = tm0;
		tm0 = tm0->next;

		rf_wb_free(tm1);
	}

	pthread_mutex_destroy(&(wl->mutex));
}

//set write list to zero
void rf_wl_zero(rf_write_list *wl){
	wl->first = wl->last = NULL;
	wl->count = 0;
}

//write list init
void rf_wl_init(rf_write_list *wl){
	assert(wl != NULL);
	pthread_mutex_init(&(wl->mutex),NULL);
	rf_wl_zero(wl);
}

//write list 's operation
void rf_wl_action(rf_write_buffer *wb,rf_write_list *wl,int action){

	assert(wl != NULL);
	pthread_mutex_lock (&(wl->mutex));

	if(action == RF_WL_ACTION_ADD_TAIL){
		assert(wb != NULL);
		if(wl->first == NULL){
			wl->first = wl->last = wb;
		}
		else{
			wl->last->next = wb;
			wl->last = wl->last->next;
		}

		wl->count++;
	}
	else if(action == RF_WL_ACTION_ADD_FIRST){
		assert(wb != NULL);
		if(wl->first == NULL){
			wl->first = wl->last = wb;
		}
		else{

			wb->next = wl->first->next;
			wl->first->next = wb;

			if(wb->next == NULL)
				wl->last = wb;
			
		}

		wl->count++;
	}
	else if(action == RF_WL_ACTION_DEL){
		if(wl->count == 0)
			return;

		wl->first = wl->first->next;
		wl->count--;
	}

	pthread_mutex_unlock (&(wl->mutex));
}

//add to write list tail
void rf_wl_push(rf_write_buffer *wb,rf_write_list *wl,int action){
	rf_wl_action(wb,wl,action);
}

rf_write_buffer *rf_wl_pop(rf_write_list *wl){
	assert(wl != NULL);

	if(wl->first == NULL)
		return NULL;

	rf_write_buffer *wb = wl->first;
	rf_wl_action(NULL,wl,RF_WL_ACTION_DEL);

	return wb;
}

int rf_wl_xsend(int fd){
	static char send_buf[4096];
	rf_client *rfc = &fd_table[fd];
	rf_write_list *wl = rfc->wl;
	if(wl == NULL)
		return RF_OK;

	int len = 0;
	int s_count = 0;

	while(wl->first){
		rf_write_buffer *v = wl->first;

		len = -1;
		errno = 0;
		if(v->len > 0){
				len = write(fd,v->buffer + v->send_len, v->len - v->send_len);
				if(len < 0){
					cclog(2,"write break , fd : %d,err : %s",fd,strerror(errno));

					if(ignore_errno(errno))
						break;
					else{
						cclog(0,"write error! now close it!, fd : %d,err : %s",fd,strerror(errno));
						return RF_ERR_SEND;
					}
				}
				else if(len == 0)
					cclog(9,"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

				cclog(4,"len : %d,send_len : %d",v->len,len);

				if((rfc->type == RF_CLIENT_REPORT) || (rfc->type == RF_CLIENT_CLI))
				{
					memset(send_buf, 0, sizeof(send_buf));
					memcpy(send_buf, v->buffer + v->send_len, len < 4096 ? len : 4095);
					cclog(1,"write buffer success to fd: [%d], peer_ip: [%s], buf_len: [%d], buffer:", fd, get_peer_ip(fd), len);
					cclog(1,"%s\n", send_buf);
				}
					
				v->send_len += len;

		}
		else{
				if(rfc->type == RF_CLIENT_REPORT)
                		{
                        		cclog(1, "refreshd has sent all the dir refresh result to fd: [%d], peer_ip: [%s]", fd, get_peer_ip(fd));
                        		rf_set_session_report_status(fd, RF_SESSION_SUCCESS);
                		}

				cclog(4,"fd : %d,send wl : %d",fd,s_count);
				rf_write_buffer *r = rf_wl_pop(wl);
				rf_wb_free(r);
				return RF_ERR_PEER_CLOSE;
		}

		if(v->send_len == v->len){
			rf_write_buffer *r = rf_wl_pop(wl);
			if(r)
				rf_wb_free(r);

			s_count++;
		}

		//free cpu time
		if(s_count > 1000)
			break;
	}

	pthread_mutex_lock (&(wl->mutex));

	if(wl->first == NULL){
		rf_wl_zero(wl);
		//add for async report dir refresh result
		if(rfc->type == RF_CLIENT_REPORT)
		{
			cclog(3, "refreshd has sent all the dir refresh result to fd: [%d], peer_ip: [%s]", fd, get_peer_ip(fd));
			rf_set_session_report_status(fd, RF_SESSION_SUCCESS);
		}
		rf_epoll_mod(fd,EPOLLIN | EPOLLHUP | EPOLLERR);
	}

	pthread_mutex_unlock(&(wl->mutex));

	return RF_OK;
}

//add to write list
int rf_add_write_list(int fd,char *buffer,int len,int action){
	rf_client *rfc = &fd_table[fd];
	if(! rfc->inuse)
		return RF_OK;

	if(rfc->wl == NULL){
		rfc->wl = cc_malloc(sizeof(rf_write_list));
		rf_wl_init(rfc->wl);
	}

	rf_write_buffer *wb = cc_malloc(sizeof(rf_write_buffer));
	if(len > 0){
			wb->buffer = cc_malloc(len);
			memcpy(wb->buffer,buffer,len);
	}
	wb->len = len;
	wb->send_len = 0;

//	rf_wl_push(wb,rfc->wl,action);
	rf_wl_action(wb, rfc->wl, action);

	rf_epoll_mod(rfc->fd,EPOLLOUT | EPOLLIN | EPOLLHUP | EPOLLERR);
	return RF_OK;
}

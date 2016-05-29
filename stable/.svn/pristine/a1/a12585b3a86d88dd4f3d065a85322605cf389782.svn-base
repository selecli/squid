/*
 * $Id$
 * author : liwei(liwei@chinacache.com)
 * desc   : epoll functions
 */

#include "refresh_header.h"
struct epoll_event events[MAX_EVENTS];
static int kdpfd = -1;

int rf_epoll_add(int fd,int flag){ 
	rf_client *rfc = &fd_table[fd];

	struct epoll_event cevent;
	memset(&cevent, 0, sizeof(struct epoll_event));
	cevent.events = flag;
	cevent.data.fd = fd;

	rfc->epoll_flag = flag;

	if( epoll_ctl(kdpfd, EPOLL_CTL_ADD, fd, &cevent) == -1 ) {
		cclog(2,"epoll_ctl fail: %s\n", strerror(errno));
		return RF_ERR_EPOLL;
	}

	return RF_OK;
}

int rf_epoll_mod(int fd,int flag){ 
	rf_client *rfc = &fd_table[fd];

	struct epoll_event cevent;
	memset(&cevent, 0, sizeof(struct epoll_event));
	cevent.events = flag;
	cevent.data.fd = fd;

	rfc->epoll_flag = flag;

	if( epoll_ctl(kdpfd, EPOLL_CTL_MOD, fd, &cevent) == -1 ) {
		cclog(2,"epoll_ctl fail: %s,fd:%d\n", strerror(errno),fd);
		return RF_ERR_EPOLL;
	}

	return RF_OK;
}

int rf_epoll_remove(int fd){
	rf_client *rfc = &fd_table[fd];
	rfc->epoll_flag = 0;

	return epoll_ctl(kdpfd, EPOLL_CTL_DEL, fd, NULL);
}

void rf_epoll_init(void) {
	if( (kdpfd = epoll_create(MAX_EVENTS)) < 0 ) {
		fprintf(stderr, "ERROR. {%s}", strerror(errno));
		rf_shutdown = true;
	}
	memset(events, 0, sizeof(events));
}

void rf_epoll_dest(void) {
	close(kdpfd);
}

int rf_epoll_wait(epoll_handle_accept *accept_handler,epoll_handle_read_event *read_handler,epoll_handle_write_event *write_handler){
	int nfds;
	int i;
	
	rf_client *rfc;

	nfds = epoll_wait(kdpfd, events, MAX_EVENTS, EPOLL_TIMEOUT);
	if(nfds == -1) {
		if( errno != EINTR ) {
			cclog(2,"ERROR. {%s}\n", strerror(errno));
			return RF_ERR_EPOLL;
		}

		return RF_OK;
	} else if( 0 == nfds) {
		cclog(9,"epoll timeout!");
		return RF_OK;
	}

	int ret = -1;
	int fd;
	for( i = 0 ; i < nfds ; i++ ) {
		if( listen_fd == events[i].data.fd ) {
			accept_handler(listen_fd);
			continue;
		}

		fd = events[i].data.fd;
		rfc = &fd_table[fd];
		if(events[i].events & (EPOLLIN|EPOLLHUP|EPOLLERR)){
			ret = read_handler(fd);
			switch(ret){
				case RF_OK:
					cclog(5,"process success! fd(%d)",fd);
					break;
				case RF_ERR_PEER_CLOSE:
					cclog(2,"close peer socket! close fd(%d),ip(%s)",fd,get_peer_ip(fd));
				//add for async report dir refresh result
					if(rfc->type == RF_CLIENT_REPORT)
					{
						rf_set_session_report_status(fd, RF_SESSION_CLOSE);
					}
				//add for async report dir refresh result

					disconnect_fd(fd);
					break;
				case RF_CLOSE_REPORT:
					if(rfc->type == RF_CLIENT_REPORT)
					{
						cclog(2, "refreshd close report fd: [%d], peer_ip: [%s]", fd, get_peer_ip(fd));
						rf_set_session_report_status(fd, RF_SESSION_SUCCESS);
					}

					disconnect_fd(fd);
					break;
				default:
				//add for async report dir refresh result
					if(rfc->type == RF_CLIENT_REPORT)
					{
						rf_set_session_report_status(fd, RF_SESSION_FAILED);
					}
				//add for async report dir refresh result
					cclog(2,"proccess error! close fd(%d),ip(%s),ret(%d)",fd,get_peer_ip(fd),ret);
					disconnect_fd(fd);
					break;
			}
		}

		if(events[i].events & (EPOLLOUT|EPOLLHUP|EPOLLERR)){
			ret = write_handler(fd);
			switch(ret){
				case RF_OK:
					cclog(5,"process success! fd(%d),ip(%s)",fd,get_peer_ip(fd));
					break;
				case RF_ERR_PEER_CLOSE:
					cclog(2,"close peer socket! close fd(%d),ip(%s)",fd,get_peer_ip(fd));
				//add for async report dir refresh result
					if(rfc->type == RF_CLIENT_REPORT)
					{
						rf_set_session_report_status(fd, RF_SESSION_SUCCESS);
					}
				//add for async report dir refresh result

					disconnect_fd(fd);
					break;
				default:
					//add for async report dir refresh result
                                        rfc = &fd_table[fd];
                                        if(rfc->type == RF_CLIENT_REPORT)
                                        {
						rf_set_session_report_status(fd, RF_SESSION_FAILED);
                                        }
                                	//add for async report dir refresh result
					cclog(2,"proccess error! close fd(%d),ip(%s)",fd,get_peer_ip(fd));
					disconnect_fd(fd);
					break;
			}
		}

	}

	return RF_OK;
}

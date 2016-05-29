#include "detect_orig.h"
struct epoll_event events[MAX_EVENTS];
static int kdpfd = -1;

void disconnect_fd(int fd){

	struct DetectOrigin *rfc = &fd_table[fd];
	cclog(3,"disconnect fd sts = %d",rfc->flag_fd_sts);

	if ( 0 == rfc->inuse )
		return;
		
	detect_epoll_remove(fd);

	memset(rfc->host,0,sizeof(rfc->host));
	memset(rfc->backup,0,sizeof(rfc->backup));
	memset(rfc->filename,0,sizeof(rfc->filename));

	rfc->inuse = 0;

	//set all members to zero
	if(rfc->current_write_buf!= NULL) {
		cc_free(rfc->current_write_buf);
		rfc->current_write_buf = NULL;
	}


	if(rfc->current_read_buf) { 
		cc_free(rfc->current_read_buf);
		rfc->current_read_buf = NULL;
	}

    if( rfc->ip )
        cc_free( rfc->ip );
    if( rfc->code )
        cc_free( rfc->code );

	close(fd);
}

int detect_epoll_add(int fd,int flag){ 

	struct epoll_event cevent;
	memset(&cevent, 0, sizeof(struct epoll_event));
	cevent.events = flag;
	cevent.data.fd = fd;


	if( epoll_ctl(kdpfd, EPOLL_CTL_ADD, fd, &cevent) == -1 ) {
		cclog(0,"epoll_ctl fail: %s", strerror(errno));
		return RF_ERR_EPOLL;
	}
	cclog(3,"epoll_ctl_add fd = %d", fd);

	return RF_OK;
}

int detect_epoll_mod(int fd,int flag){ 

	struct epoll_event cevent;
	memset(&cevent, 0, sizeof(struct epoll_event));
	cevent.events = flag;
	cevent.data.fd = fd;


	if( epoll_ctl(kdpfd, EPOLL_CTL_MOD, fd, &cevent) == -1 ) {
		cclog(0,"epoll_ctl fail: %s", strerror(errno));
		return RF_ERR_EPOLL;
	}

	return RF_OK;
}

int detect_epoll_remove(int fd){

	if (epoll_ctl(kdpfd, EPOLL_CTL_DEL, fd, NULL) == -1)
	{
		cclog(0,"epoll_ctl_del fd = %d\n: %s", fd,strerror(errno));
		return RF_ERR_EPOLL;
	}
	return RF_OK;
}

void detect_epoll_init(void) {
	if( (kdpfd = epoll_create(MAX_EVENTS)) < 0 ) {
		cclog(0, "ERROR. {%s}", strerror(errno));
	}
	memset(events, 0, sizeof(events));
}

void detect_epoll_dest(void) {
	close(kdpfd);
}

int detect_epoll_wait(epoll_handle_read_event *read_handler,epoll_handle_write_event *write_handler){
	int nfds;
	int i;

	nfds = epoll_wait(kdpfd, events, MAX_EVENTS, EPOLL_TIMEOUT);
	cclog(3,"epoll_wait nfds =%d",nfds);
	if(nfds == -1) {
		if( errno != EINTR ) {
			cclog(0,"ERROR. {%s}", strerror(errno));
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

		fd = events[i].data.fd;
		if(0 == fd )
		{
			cclog(0,"fd error,nfds =%d",nfds);
			return RF_ERR_EPOLL;
		}
		if(events[i].events & (EPOLLIN)){
			ret = read_handler(fd);
			if(0 == ret)
				;
			else if(-1 == ret)
				disconnect_fd(fd);
			else
				after_handle_read_event(fd);
		}

		if(events[i].events & (EPOLLOUT)){
			ret = write_handler(fd);
			if(0 == ret)
				;
			else if(-1 == ret)
				disconnect_fd(fd);
			else
				detect_epoll_mod(fd,EPOLLIN | EPOLLHUP | EPOLLERR);
		}
		if(events[i].events & (EPOLLHUP|EPOLLERR)){
			cclog(3,"events[%d].events = EPOLLHUP|EPOLLERR",i);
			disconnect_fd(fd);
		}

	}
	return RF_OK;
}

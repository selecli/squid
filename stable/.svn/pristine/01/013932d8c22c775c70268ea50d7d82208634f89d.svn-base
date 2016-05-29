
#include "server_header.h"

struct epoll_event events[MAX_EVENTS];
static int kdpfd = -1;

extern int listen_fd;
extern bool server_shutdown;

int server_epoll_add(int fd,int flag){ 
	fd_struct *fd_s = &fd_table[fd];

	struct epoll_event cevent;
	memset(&cevent, 0, sizeof(struct epoll_event));
	cevent.events = flag;
	cevent.data.fd = fd;

	fd_s->epoll_flag = flag;

	if( epoll_ctl(kdpfd, EPOLL_CTL_ADD, fd, &cevent) == -1 ) {
		cclog(2,"epoll_ctl fail: %s\n", strerror(errno));
		return ERR_EPOLL;
	}

	return OK;
}

int server_epoll_mod(int fd,int flag){ 
	fd_struct *fd_s = &fd_table[fd];

	struct epoll_event cevent;
	memset(&cevent, 0, sizeof(struct epoll_event));
	cevent.events = flag;
	cevent.data.fd = fd;

	fd_s->epoll_flag = flag;

	if( epoll_ctl(kdpfd, EPOLL_CTL_MOD, fd, &cevent) == -1 ) {
		cclog(2,"epoll_ctl fail: %s,fd:%d\n", strerror(errno),fd);
		return ERR_EPOLL;
	}

	return OK;
}

int server_epoll_remove(int fd){
	cclog(5, "epoll remove fd: %d", fd);

	fd_struct *fd_s = &fd_table[fd];
	fd_s->epoll_flag = 0;

	return epoll_ctl(kdpfd, EPOLL_CTL_DEL, fd, NULL);
}

void server_epoll_init(void) {
	if( (kdpfd = epoll_create(MAX_EVENTS)) < 0 ) {
		fprintf(stderr, "ERROR. {%s}", strerror(errno));
		server_shutdown = true;
	}
	memset(events, 0, sizeof(events));
}

void server_epoll_dest(void) {
	close(kdpfd);
}

int server_epoll_wait(epoll_handle_accept *accept_handler,epoll_handle_read_event *read_handler,epoll_handle_write_event *write_handler){
	int nfds;
	int i;
	
	fd_struct *fd_s;
//	memset(events, 0, (sizeof(struct epoll_event)*MAX_EVENTS));

	nfds = epoll_wait(kdpfd, events, MAX_EVENTS, EPOLL_TIMEOUT);
	if(nfds == -1) {
		if( errno != EINTR ) {
			cclog(2,"ERROR. {%s}\n", strerror(errno));
			return ERR_EPOLL;
		}

		return OK;
	} else if( 0 == nfds) {
		cclog(9,"epoll timeout!");
		return OK;
	}

	int ret = -1;
	int fd;
	for( i = 0 ; i < nfds ; i++ ) {
		if( listen_fd == events[i].data.fd ) {
			accept_handler(listen_fd);
			continue;
		}

		fd = events[i].data.fd;
		fd_s = &fd_table[fd];
		if(events[i].events & (EPOLLIN|EPOLLHUP|EPOLLERR)){
			ret = read_handler(fd);
			switch(ret){
				case OK:
					cclog(5,"process read success! fd(%d)",fd);
					break;
				case ERR_PEER_CLOSE:
					cclog(2,"close peer socket in read event! close fd(%d),ip(%s)",fd,get_peer_ip(fd));

					disconnect_fd(fd, 0);
				//	return OK;
					continue;
				default:
					cclog(2,"proccess read error! close fd(%d),ip(%s),ret(%d)",fd,get_peer_ip(fd),ret);
					disconnect_fd(fd, 1);
				//	return OK;
					continue;
			}
		}

		if(events[i].events & (EPOLLOUT|EPOLLHUP|EPOLLERR)){
			ret = write_handler(fd);
			switch(ret){
				case OK:
					cclog(5,"process write success! fd(%d),ip(%s)",fd,get_peer_ip(fd));
					break;
				case ERR_PEER_CLOSE:
					cclog(2,"close peer socket in write event! close fd(%d),ip(%s)",fd,get_peer_ip(fd));

					disconnect_fd(fd, 0);
				//	return OK;
					continue;
				case CLOSE:
					cclog(5, "session done, close all fds now!");
					disconnect_fd(fd, 0);
				//	return OK;
					continue;
				default:
					cclog(2,"proccess write error! close fd(%d),ip(%s)",fd,get_peer_ip(fd));
					disconnect_fd(fd, 1);
				//	return OK;
					continue;
			}
		}

	}

	return OK;
}

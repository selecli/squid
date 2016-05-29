#include "detect.h"

static int epfd = -1;
static struct epoll_event epoll_events[MAX_EVENTS];

static int epollCreate(void)
{
	epfd = epoll_create(MAX_EVENTS);
	if (epfd < 0)
	{
		dlog(WARNING, NOFD, "epoll_create() failed, ERROR=[%s]", xerror());
		return FAILED;
	}
	xmemzero(epoll_events, sizeof(epoll_events));

	return SUCC;
}

static int epollAdd(const int sockfd, const int events)
{ 
	struct epoll_event evt;

	evt.events = events;
	evt.data.fd = sockfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &evt) < 0) 
	{
		dlog(WARNING, sockfd,
				"epoll_ctl(EPOLL_CTL_ADD) failed, ERROR=[%s]",
				xerror());
		return FAILED;
	}

	return SUCC;
}

static int epollDel(const int sockfd)
{
	if (epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL))
	{
		dlog(WARNING, sockfd,
				"epoll_ctl(EPOLL_CTL_DEL) failed, ERROR=[%s]",
				xerror());
		return FAILED;
	}

	return SUCC;
}

static int epollMod(const int sockfd, const int events)
{ 
	struct epoll_event evt;

	evt.events = events;
	evt.data.fd = sockfd;
	if (epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &evt))
	{
		dlog(WARNING, sockfd,
				"epoll_ctl(EPOLL_CTL_MOD) failed, ERROR=[%s]",
				xerror());
		return FAILED;
	}

	return SUCC;
}

static int epollWait(EWCB *callback)
{
	int i, sockfd, nfds;

	nfds = epoll_wait(epfd, epoll_events, MAX_EVENTS, detect.opts.epollwait);
	if (nfds < 0)
	{
		dlog(WARNING, NOFD, "epoll_wait() error, ERROR=[%s]", xerror());
		return FAILED;
	} 
	if (0 == nfds)
		return SUCC;
	for (i = 0 ; i < nfds; ++i)
	{
		sockfd = epoll_events[i].data.fd;
		if (epoll_events[i].events & EPOLLIN)
		{
			//puts("EPOLLIN");
			callback(EPOLL_IN, sockfd);
		}
		else if (epoll_events[i].events & EPOLLOUT)
		{
			//puts("EPOLLOUT");
			callback(EPOLL_OUT, sockfd);
		}
		else if (epoll_events[i].events & EPOLLERR)
		{
			//puts("EPOLLERR");
			callback(EPOLL_ERR, sockfd);
		}
		else if (epoll_events[i].events & EPOLLHUP)
		{
			//puts("EPOLLHUP");
			callback(EPOLL_HUP, sockfd);
		}
	}

	return SUCC;
}

static int epollDestroy(void)
{
	close(epfd);

	return SUCC;
}

int epollCall(const int calltype, const int sockfd, const int event, EWCB *callback)
{
	switch (calltype)
	{
		case EPOLL_CREATE:
			return epollCreate();
		case EPOLL_ADD:
			return epollAdd(sockfd, event);
		case EPOLL_MOD:
			return epollMod(sockfd, event);
		case EPOLL_WAIT:
			return epollWait(callback);
		case EPOLL_DEL:
			return epollDel(sockfd);
		case EPOLL_DESTROY:
			return epollDestroy();
		default:
			dlog(WARNING, sockfd, "no this type, TYPE=[%d]", calltype);
			return FAILED;
	}

	return SUCC;
}


#include "detect.h"

int sockCheckSockfdValid(const int sockfd)
{
	int sock_errno = 0;
	socklen_t sockerr_len = sizeof(sock_errno);

	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sock_errno, &sockerr_len) < 0)
	{
		dlog(WARNING, sockfd, "getsockopt() failed, ERROR=[%s]", xerror());
		return 0;
	}
	if (0 == sock_errno || EISCONN == sock_errno)
		return 1;
	dlog(WARNING, sockfd, "socket error, ERROR=[%s]\n", xerror());

	return 0;
}

static int sockSetSockfd(const int sockfd, const int type)
{
	int flags;

	//same to ioctl(FIONBIO)
	flags = fcntl(sockfd, F_GETFL);
	if (flags < 0)
	{
		dlog(WARNING, NOFD, 
				"fcntl(F_GETFL) failed, SOCKFD=[%d], ERROR=[%s]",
				sockfd, xerror());
		return FAILED;
	}
	if (SOCK_BLOCKING == type)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;
	if (fcntl(sockfd, F_SETFL, flags) < 0)
	{
		dlog(WARNING, NOFD,
				"fcntl(F_SETFL) failed, SOCKFD=[%d], ERROR=[%s]",
				sockfd, xerror());	
		return FAILED;
	}

	return SUCC;
}

static int sockSetSockfdBlock(const int sockfd) 
{
	sockSetSockfd(sockfd, SOCK_BLOCKING);

	return SUCC;
}

static int sockSetSockfdNonBlock(const int sockfd) 
{
	sockSetSockfd(sockfd, SOCK_NONBLOCKING);

	return SUCC;
}

static int sockSetSockoptAddrReuse(const int sockfd)
{
	int v = 1;

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0)
		return FAILED;

	return SUCC;
}

static int sockGetSockfd(detect_socket_t *sock)
{
	int sockfd;

	sockfd = socket(sock->domain, sock->socktype, sock->protocol);
	if (sockfd < 0)
	{
		dlog(WARNING, NOFD, "socket() failed, ERROR=[%s]", xerror());
		return FAILED;
	}
	sockSetSockoptAddrReuse(sockfd);
	sockSetSockfdNonBlock(sockfd);
	sock->sockfd = sockfd;

	return SUCC;
}

static void httpRecvData(const int sockfd)
{
	int retry;
	ssize_t ret;
	commdata_t *in;

	retry = 3;
	in = &detect.data.index.idx[sockfd].event->unit->comm.in;

	while (in->curlen > 0)
	{
		ret = recv(sockfd, in->buffer + in->offset, in->curlen, 0);
		if (ret < 0)
		{
			if (EAGAIN == errno)
			{
				//retry recieve
				in->handler(sockfd, RECV_REDO);
				return;
			}
			if (EINTR == errno && --retry)
			{
				//if interrupt, retry soon
				usleep(10);
				continue;
			}
			//other error, error handle
			if (in->offset > 0)
			{
				dlog(WARNING, sockfd,
						"recv() failed, few data received, ERROR=[%s]",
						xerror());
			}
			else
			{
				dlog(WARNING, sockfd,
						"recv() failed, no data received, ERROR=[%s]",
						xerror());
			}
			in->handler(sockfd, FAILED);
			return;
		}
		if (0 == ret)
		{
			//already recieved all data
			in->handler(sockfd, DONE);
			return;
		}
		//first byte
		if (0 == in->offset)
			in->handler(sockfd, FIRST_BYTE);
		in->offset += ret;
		in->curlen = in->bufsize - in->offset;
		if (in->curlen <= 0)
		{
			in->handler(sockfd, DONE);
			return;
		}
		in->handler(sockfd, OTHER_BYTE);
	}
	in->handler(sockfd, DONE);
}

static void httpSendData(const int sockfd)
{
	int retry;
	ssize_t ret;
	commdata_t *out;

	retry = 3;
	out = &detect.data.index.idx[sockfd].event->unit->comm.out;

	while (out->curlen > 0)
	{
		ret = send(sockfd, out->buffer + out->offset, out->curlen, 0);
		if (ret < 0)
		{
			if (EINTR == errno && --retry)
			{
				usleep(10);
				continue;
			}
			dlog(WARNING, sockfd, "send() failed, ERROR=[%s]", xerror());
			out->handler(sockfd, FAILED);
			return;
		}
		out->offset += ret;
		out->curlen = out->bufsize - out->offset;
	};
	out->handler(sockfd, SUCC);
}

static void sockSendData(const int sockfd)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	if (PROTOCOL_HTTPS == event->dip->rfc->flags.protocol)
		httpsSendData(sockfd);
	else
		httpSendData(sockfd);
}

static void sockRecvData(const int sockfd)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	if (PROTOCOL_HTTPS == event->dip->rfc->flags.protocol)
		httpsRecvData(sockfd);
	else
		httpRecvData(sockfd);
}

int socketCall(const int calltype, const int sockfd, detect_socket_t *socket)
{
	switch (calltype)
	{
		case SOCK_GETFD:
			return sockGetSockfd(socket);
		case SOCK_VALID:
			return sockCheckSockfdValid(sockfd);
		case SOCK_NONBLOCKING:
			return sockSetSockfdNonBlock(sockfd);
		case SOCK_BLOCKING:
			return sockSetSockfdBlock(sockfd);
		case SOCK_ADDR_REUSE:
			return sockSetSockoptAddrReuse(sockfd);
		case SOCK_SEND:
			sockSendData(sockfd);
			break;
		case SOCK_RECV:
			sockRecvData(sockfd);
			break;
		default:
			dlog(WARNING, NOFD, "no this type, TYPE=[%d]", calltype);
			return FAILED;
	}

	return SUCC;
}


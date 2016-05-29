#include "detect.h"

static void eventDelete(detect_event_t *event);
static void detectConnect(detect_event_t *event);
static void commTime(detect_unit_t *unit, const int type);

static int eventCmp(void *key, void *data)
{
	return (key != data);
}

static void xconnect(detect_event_t *event)
{
	int error = 0;
	int sockfd = event->unit->sockfd;
	detect_socket_t *s = &event->dip->socket;

	commTime(event->unit, COMM_CNTIME);
	if (connect(sockfd, (void*)&s->sockaddr, s->sockaddr_len) < 0)
		error = errno;
	else
		commTime(event->unit, COMM_CDTIME);
	s->handler(sockfd, error);
}

static void xdisconnect(const int sockfd)
{
	detect_event_t *event;

	epollCall(EPOLL_DEL, sockfd, 0, NULL);
	event = detect.data.index.idx[sockfd].event;
	if(event->unit->comm.in.buffer != NULL)
	{
		xfree((void *)&event->unit->comm.in.buffer);
		event->unit->comm.in.buffer = NULL;
	}
	//FIXME: here free comm.out.buffer
	/*
	   if(event->unit->comm.out.buffer != NULL)
	   {
	   xfree((void *)&event->unit->comm.out.buffer);
	   event->unit->comm.out.buffer = NULL;
	   }
	 */
	if (PROTOCOL_HTTPS == event->dip->rfc->flags.protocol)
		httpsClose(sockfd);
	eventDelete(event);
	detect.data.index.idx[sockfd].inuse = 0;
	detect.data.index.idx[sockfd].event = NULL;
	close(sockfd);
}

static void commTime(detect_unit_t *unit, const int type)
{
	switch (type)
	{
		case COMM_CNTIME:
			xtime(TIME_TIME, &unit->cntime);
			break;
		case COMM_CDTIME:
			xtime(TIME_TIME, &unit->cdtime);
			break;
		case COMM_SDTIME:
			xtime(TIME_TIME, &unit->sdtime);
			break;
		case COMM_FBTIME:
			xtime(TIME_TIME, &unit->fbtime);
			break;
		case COMM_EDTIME:
			xtime(TIME_TIME, &unit->edtime);
			break;
		default:
			xabort("commTime: no this type");
	}
}

static int checkHeader(detect_ip_t *dip, const char *hdr)
{
	int i;
	http_headers_t *HDRS = &dip->rfc->hdrs;

	for (i = 0; i < HDRS->num; ++i)
	{
		if (!strcasecmp(HDRS->hdrs[i].name, hdr))
		{
			HDRS->hdrs[i].repeat = 1;
			return i;
		}
	}
	return -1;
}

static void packRequestLine(detect_ip_t *dip, const int dway)
{
	char buffer[BUF_SIZE_4096];
	detect_cfg_t *cfg = dip->rfc;
	detect_socket_t *s = &dip->socket;

	switch (dway)
	{
		case DWAY_COMMON:
			xmemzero(buffer, BUF_SIZE_4096);
			snprintf(buffer, BUF_SIZE_4096,
					"%s %s HTTP/%d.%d\r\n",
					methods[cfg->method1->method],
					cfg->method1->filename,
					HTTP_VERSION_MAJOR,
					HTTP_VERSION_MINOR);
			s->common_buflen = strlen(buffer);
			s->common_buffer = xcalloc(s->common_buflen, 1);
			memcpy(s->common_buffer, buffer, s->common_buflen);
			break;
		case DWAY_REFERER:
			xmemzero(buffer, BUF_SIZE_4096);
			snprintf(buffer, BUF_SIZE_4096,
					"%s %s HTTP/%d.%d\r\n",
					methods[cfg->method2->method],
					cfg->method2->filename,
					HTTP_VERSION_MAJOR,
					HTTP_VERSION_MINOR);
			s->referer_buflen = strlen(buffer);
			s->referer_buffer = xcalloc(s->referer_buflen, 1);
			memcpy(s->referer_buffer, buffer, s->referer_buflen);
			break;
		default:
			xabort("packRequestLine: no this dway");
	}
}

static void expandSendBufferCopy(detect_ip_t *dip, const int dway, const char *buffer)
{
	size_t len, size;
	detect_socket_t *s = &dip->socket;

	switch (dway)
	{
		case DWAY_COMMON:
			len = strlen(buffer);
			size = s->common_buflen + len;
			s->common_buffer = xrealloc(s->common_buffer, size);
			memcpy(s->common_buffer + s->common_buflen, buffer, len);
			s->common_buflen = size;
			break;
		case DWAY_REFERER:
			len = strlen(buffer);
			size = s->referer_buflen + len;
			s->referer_buffer = xrealloc(s->referer_buffer, size);
			memcpy(s->referer_buffer + s->referer_buflen, buffer, len);
			s->referer_buflen = size;
			break;
		default:
			xabort("expandSendBufferCopy: no this dway");
	}
}

static int getPostData(detect_ip_t *dip, const int dway, const char *path)
{
	FILE *fp;
	size_t bufsize = 0;
	char line[BUF_SIZE_4096];

	if (NULL == (fp = xfopen(path, "r")))
		return FAILED;
	do {    
		if (DWAY_COMMON == dway)
			bufsize = dip->socket.common_buflen;
		else if (DWAY_COMMON == dway)
			bufsize = dip->socket.referer_buflen;
		else
			xabort("getPostData: no this dway");
		if (bufsize > MAX_POST_SIZE)
		{
			dlog(WARNING, NOFD, "post data too long");
			break;
		}
		xmemzero(line, BUF_SIZE_4096);
		if (NULL == fgets(line, BUF_SIZE_4096, fp)) 
		{    
			if (0 == feof(fp))
				continue;
			break;
		}
		expandSendBufferCopy(dip, dway, line);
	} while(1);

	fclose(fp);

	return SUCC;
}

static void packRequestHeaders(detect_ip_t *dip, const int dway)
{
	int i, ret;
	struct stat st;
	const char *V = NULL;
	char sendbuf[BUF_SIZE_4096];
	detect_cfg_t *cfg = dip->rfc;

	//Header: User-Agent
	xmemzero(sendbuf, BUF_SIZE_4096);
	ret = checkHeader(dip, "User-Agent");
	V = (ret < 0) ? "ChinaCache" : cfg->hdrs.hdrs[ret].value;
	snprintf(sendbuf, BUF_SIZE_4096, "User-Agent: %s\r\n", V);
	expandSendBufferCopy(dip, dway, sendbuf);
	//Header: Accept
	xmemzero(sendbuf, BUF_SIZE_4096);
	ret = checkHeader(dip, "Accept");
	V = (ret < 0) ? "*/*" : cfg->hdrs.hdrs[ret].value;
	snprintf(sendbuf, BUF_SIZE_4096, "Accept: %s\r\n", V);
	expandSendBufferCopy(dip, dway, sendbuf);
	switch (dway)
	{
		case DWAY_COMMON:
			//Header: Host
			xmemzero(sendbuf, BUF_SIZE_4096);
			ret = checkHeader(dip, "Host");
			V = (ret < 0) ? cfg->method1->hostname : cfg->hdrs.hdrs[ret].value;
			snprintf(sendbuf, BUF_SIZE_4096, "Host: %s\r\n", V);
			expandSendBufferCopy(dip, dway, sendbuf);
			if (METHOD_POST == cfg->method1->method)
			{
				//Header: Content-Type
				xmemzero(sendbuf, BUF_SIZE_4096);
				ret = checkHeader(dip, "Content-Type");
				V = (ret < 0) ? cfg->method1->posttype : cfg->hdrs.hdrs[ret].value;
				snprintf(sendbuf, BUF_SIZE_4096, "Content-Type: %s\r\n", V);
				expandSendBufferCopy(dip, dway, sendbuf);
				//Header: Content-Length
				if (stat(cfg->method1->postfile, &st) < 0)
				{
					dlog(WARNING, NOFD,
							"stat() for post file failed, FILE=[%s]",
							cfg->method1->postfile);
					return;
				}
				xmemzero(sendbuf, BUF_SIZE_4096);
				snprintf(sendbuf, BUF_SIZE_4096, 
						"Content-Length: %ld\r\n",
						st.st_size );
				expandSendBufferCopy(dip, dway, sendbuf);
			}
			break;
		case DWAY_REFERER:
			//Header: Host
			xmemzero(sendbuf, BUF_SIZE_4096);
			ret = checkHeader(dip, "Host");
			V = (ret < 0) ? cfg->method2->hostname : cfg->hdrs.hdrs[ret].value;
			snprintf(sendbuf, BUF_SIZE_4096, "Host: %s\r\n", V);
			expandSendBufferCopy(dip, dway, sendbuf);
			//Header: Referer
			xmemzero(sendbuf, BUF_SIZE_4096);
			ret = checkHeader(dip, "Referer");
			V = (ret < 0) ? cfg->hostname : cfg->hdrs.hdrs[ret].value;
			snprintf(sendbuf, BUF_SIZE_4096, "Referer: %s\r\n", V);
			expandSendBufferCopy(dip, dway, sendbuf);
			if (METHOD_POST == cfg->method2->method)
			{
				//Header: Content-Type
				xmemzero(sendbuf, BUF_SIZE_4096);
				ret = checkHeader(dip, "Content-Type");
				V = (ret < 0) ? cfg->method2->posttype : cfg->hdrs.hdrs[ret].value;
				snprintf(sendbuf, BUF_SIZE_4096, "Content-Type: %s\r\n", V);
				expandSendBufferCopy(dip, dway, sendbuf);
				//Header: Content-Length
				if (stat(cfg->method2->postfile, &st) < 0)
				{
					dlog(WARNING, NOFD,
							"stat() for post file failed, FILE=[%s]",
							cfg->method2->postfile);
					return;
				}
				xmemzero(sendbuf, BUF_SIZE_4096);
				snprintf(sendbuf, BUF_SIZE_4096,
						"Content-Length: %ld\r\n",
						st.st_size);
				expandSendBufferCopy(dip, dway, sendbuf);
			}
			break;
		default:
			xabort("no this dway in packRequestHeaders");
	}
	for (i = 0; i < cfg->hdrs.num; ++i)
	{
		if (cfg->hdrs.hdrs[i].repeat)
			continue;
		xmemzero(sendbuf, BUF_SIZE_4096);
		snprintf(sendbuf, BUF_SIZE_4096,
				"%s: %s\r\n",
				cfg->hdrs.hdrs[i].name,
				cfg->hdrs.hdrs[i].value);
		expandSendBufferCopy(dip, dway, sendbuf);
	}
	xmemzero(sendbuf, BUF_SIZE_4096);
	snprintf(sendbuf, BUF_SIZE_4096, "Connection: close\r\n\r\n");
	expandSendBufferCopy(dip, dway, sendbuf);
	if (DWAY_COMMON == dway && METHOD_POST == cfg->method1->method)
		getPostData(dip, dway, cfg->method1->postfile);
	else if (DWAY_REFERER == dway && METHOD_POST == cfg->method2->method)
		getPostData(dip, dway, cfg->method2->postfile);
}

static void packSendData(detect_ip_t *dip, const int dway)
{
	packRequestLine(dip, dway);
	packRequestHeaders(dip, dway);
}

static void initSendBuffer(detect_ip_t *dip)
{
	packSendData(dip, DWAY_COMMON);
	if (dip->rfc->flags.referer)
		packSendData(dip, DWAY_REFERER);
}

static void parseCode(detect_unit_t *unit, const char *line)
{
	char *ptr;

	if (NULL == line)
		return;
	/* Format: HTTP/x.x <space> <status code> <space> <reason-phrase> CRLF */
	if(strncasecmp(line, "HTTP/", 5))
		return;
	ptr = strchr(line, ' ');
	if (NULL == ptr)
		return;
	while (' ' == *(++ptr)) ;
	memcpy(unit->code, ptr, 3);
	unit->code[3] = '\0';
	unit->code_ok = 1;
}

static void parseResponseCode(detect_unit_t *unit)
{
	int len;
	char *ptr, *ptr1;
	char buf[BUF_SIZE_2048];

	ptr = strstr(unit->comm.in.buffer, "HTTP/1.");
	if (NULL == ptr)
		return;
	xmemzero(buf, BUF_SIZE_2048);
	ptr1 = strstr(unit->comm.in.buffer, "\r\n");    
	if (NULL == ptr1)
	{
		strncpy(buf, ptr, BUF_SIZE_2048);
		len = strlen(buf);
		if (len >= BUF_SIZE_2048)
			buf[BUF_SIZE_2048 - 1] = '\0';
		else
			buf[len - 1] = '\0';
	}
	else
	{
		len = ptr1 - ptr;
		strncpy(buf, ptr, len);
		buf[len] = '\0';
	}
	parseCode(unit, buf);
}

static void parseResponseData(detect_unit_t *unit)
{
	if (unit->code_ok)
		return;
	parseResponseCode(unit);	
}

static void connectFailedHandle(const int sockfd, const int error)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	if (EINPROGRESS == error)
	{
		//if connect is in progressing, here add epoll(EPOLLOUT)
		event->unit->inprogress = 1;
		epollCall(EPOLL_ADD, sockfd, EPOLLOUT|EPOLLHUP|EPOLLERR, NULL);
		return;
	}
	commTime(event->unit, COMM_EDTIME);
	event->unit->pstate = PORT_DOWN;
}

static void connectSucceedTcpHandle(const int sockfd)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	event->unit->flag = CONNECTED;
	xdisconnect(sockfd);
}

static void connectSucceedHttpHandle(const int sockfd)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	event->unit->flag = CONNECTED;
	epollCall(EPOLL_ADD, sockfd, EPOLLOUT|EPOLLHUP|EPOLLERR, NULL);
}

static void connectSucceedHttpsHandle(const int sockfd)
{
	detect_event_t *event;

	httpsConnect(sockfd);
	event = detect.data.index.idx[sockfd].event;
	commTime(event->unit, COMM_CDTIME);
	event->unit->flag = CONNECTED;
	epollCall(EPOLL_ADD, sockfd, EPOLLOUT|EPOLLHUP|EPOLLERR, NULL);
}

static void connectSucceedHandle(const int sockfd)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	event->unit->pstate = PORT_WORK;
	switch (event->dip->rfc->flags.protocol)
	{
		case PROTOCOL_TCP:
			connectSucceedTcpHandle(sockfd);
			break;
		case PROTOCOL_HTTP:
			connectSucceedHttpHandle(sockfd);
			break;
		case PROTOCOL_HTTPS:
			connectSucceedHttpsHandle(sockfd);
			break;
		default:
			xabort("no this event->dip->rfc->flags.protocol type");
	}
}

static void afterConnectHandle(const int sockfd, const int error)
{
	if (error)
		connectFailedHandle(sockfd, error);
	else
		connectSucceedHandle(sockfd);
}

static void afterSendSucceedHandle(const int sockfd)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	commTime(event->unit, COMM_SDTIME);
	event->unit->flag = SENT;
	epollCall(EPOLL_MOD, sockfd, EPOLLIN|EPOLLHUP|EPOLLERR, NULL);
}

static void afterSendFailedHandle(const int sockfd)
{
	commTime(detect.data.index.idx[sockfd].event->unit, COMM_EDTIME);
	xdisconnect(sockfd);
}

static void afterSend(const int sockfd, const int type)
{
	switch (type)
	{
		case SUCC:
			afterSendSucceedHandle(sockfd);
			break;
		case FAILED:
			afterSendFailedHandle(sockfd);
			break;
		default:
			xabort("afterSend: no this type");
	}
}

static void afterRecvHandle(const int sockfd, const int type)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	assert(event != NULL);

	switch (type)
	{
		case FIRST_BYTE:
			commTime(event->unit, COMM_FBTIME);
			parseResponseData(event->unit);
			return;
		case OTHER_BYTE:
			parseResponseData(event->unit);
			return;
		case DONE:
			commTime(event->unit, COMM_EDTIME);
			event->unit->flag = RECVED;
			xdisconnect(sockfd);
			return;
		case FAILED:
			commTime(event->unit, COMM_EDTIME);
			xdisconnect(sockfd);
			return;
		case RECV_REDO:
			event->unit->flag = RECV_REDO;
			return;
		default:
			xabort("afterRecvHandle: no this type");
	}
}

static void beforeSendHandle(const int sockfd)
{
	commdata_t *out;
	detect_socket_t *s;
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	out = &event->unit->comm.out;
	s = &event->dip->socket;

	if (DWAY_COMMON == event->unit->type)
	{
		out->offset = 0;
		out->curlen = s->common_buflen;
		out->bufsize = s->common_buflen;
		out->buffer = s->common_buffer;
		out->handler = afterSend;
	}
	else if (DWAY_REFERER == event->unit->type)
	{
		out->offset = 0;
		out->curlen = s->referer_buflen;
		out->bufsize = s->referer_buflen;
		out->buffer = s->referer_buffer;
		out->handler = afterSend;
	}
	else
	{
		xabort("beforeSendHandle: no this dway");
	}
	event->unit->flag = SENDING;
}

static void beforeRecvHandle(const int sockfd)
{
	size_t rlen;
	commdata_t *in;
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	rlen = event->dip->rfc->rlength;
	if (rlen > DETECT_OBJLEN_MAX)
		rlen = DETECT_OBJLEN_MAX;
	in = &event->unit->comm.in;
	in->flag = READY;
	in->offset = 0;
	in->bufsize = rlen;
	in->buffer = xcalloc(rlen, 1);
	in->curlen = rlen;
	in->handler = afterRecvHandle;
	event->unit->flag = RECVING;
}

static void epollInHandle(const int sockfd)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	if (NULL == event || !detect.data.index.idx[sockfd].inuse)
		return;
	if (RECV_REDO != event->unit->flag)
		beforeRecvHandle(sockfd);
	socketCall(SOCK_RECV, sockfd, NULL);
}

static void epollOutHandle(const int sockfd)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	if (PROTOCOL_TCP == event->dip->rfc->flags.protocol)
	{
		//if TCP connection in progressing
		commTime(event->unit, COMM_CDTIME);
		event->unit->flag = CONNECTED;
		xdisconnect(sockfd);
		return;
	}
	if (event->unit->inprogress)
	{
		//although epoll(EPOLLOUT) is ok after connect() inprogress,
		//wo also need check sockfd valid
		if (!socketCall(SOCK_VALID, sockfd, NULL))
		{
			commTime(event->unit, COMM_EDTIME);
			xdisconnect(sockfd);
			return;
		}
		if (PROTOCOL_HTTPS == event->dip->rfc->flags.protocol)
		{
			if (FAILED == httpsConnect(sockfd))
			{
				xdisconnect(sockfd);
				return;
			}
		}
		commTime(event->unit, COMM_CDTIME);
		event->unit->flag = CONNECTED;
		event->unit->inprogress = 0;
	}
	beforeSendHandle(sockfd);
	socketCall(SOCK_SEND, sockfd, NULL);
}

static void epollExceptionHandle(const int sockfd, const int type)
{
	char *prefix;
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	prefix = (EPOLL_HUP == type) ? "EPOLLHUP" : "EPOLLERR";
	if (event->unit->comm.in.offset > 0)
		dlog(ERROR, NOFD, "%s, few data received, IP=[%s]", prefix, event->dip->ip);
	else
		dlog(ERROR, NOFD, "%s, no data received, IP=[%s]", prefix, event->dip->ip);
	commTime(event->unit, COMM_EDTIME);
	xdisconnect(sockfd);
}

static void epollWaitHandle(const int type, const int sockfd)
{
	switch (type)
	{
		case EPOLL_IN:
			epollInHandle(sockfd);
			break;
		case EPOLL_OUT:
			epollOutHandle(sockfd);
			break;
		case EPOLL_HUP:
		case EPOLL_ERR:
			epollExceptionHandle(sockfd, type);
			break;
		default:
			xabort("epollWaitHandle: no this type");
	}
}

static void eventCheckTimeout(detect_event_t *event)
{
	double usedtime;
	double checktime;

	xtime(TIME_TIME, &checktime);
	usedtime = checktime - event->unit->cntime;
	if (usedtime > event->dip->rfc->timeout)
	{
		if (RECVED == event->unit->flag)
			dlog(WARNING, event->unit->sockfd,
					"timeout, all data received, USEDTIME=[%lf]", usedtime);
		else if (event->unit->comm.in.offset > 0)
			dlog(WARNING, event->unit->sockfd,
					"timeout, few data received, USEDTIME=[%lf]", usedtime);
		else
			dlog(WARNING, event->unit->sockfd, "timeout, no data received, USEDTIME=[%lf]", usedtime);
		commTime(event->unit, COMM_EDTIME);
		xdisconnect(event->unit->sockfd);
	}
}

static void eventStart(detect_event_t *event)
{
	runningSched(300);
	if (event->flag)
	{
		eventCheckTimeout(event);
		return;
	}
	event->flag = 1;
	detectConnect(event);
}

static void eventDelete(detect_event_t *event)
{
	llistDelete(detect.data.ellist, event, eventCmp);
}

static int socketInit(detect_ip_t *dip)
{
	int ret;

	dip->socket.protocol = 0;
	dip->socket.domain = AF_INET;
	dip->socket.socktype = SOCK_STREAM;
	dip->socket.sockaddr.sin_family = AF_INET;
	dip->socket.sockaddr.sin_port = htons(dip->rfc->port);
	dip->socket.sockaddr_len = detect.tsize.sockaddr_in_size;
	dip->socket.handler = afterConnectHandle;
	ret = inet_pton(AF_INET, dip->ip, &dip->socket.sockaddr.sin_addr);
	if (ret < 0)
	{
		dlog(WARNING, NOFD, "inet_aton() failed, ERROR=[%s]", xerror());
		return FAILED;
	}
	if (0 == ret)
	{
		dlog(WARNING, NOFD, "inet_aton(): ip is invalid");
		return FAILED;
	}
	initSendBuffer(dip);

	return SUCC;
}

static void nodetectAssign(detect_ip_t *dip, detect_cfg_t *cfg)
{
	dip->dop.cutime = 0;
	dip->dop.fbtime = 0;
	dip->dop.dltime = 0;
	dip->dop.ustime = 0;
	dip->dop.oktimes = 0;
	outputGetDway(dip);
	outputGetIpType(dip);
	dip->dop.port = cfg->port;
	dip->dop.times = cfg->times;
	strcpy(dip->dop.code, "000");
	dip->dop.pstate = xstrdup("work");
	dip->dop.detect = xstrdup("nodetect");
	dip->dop.hostname = xstrdup(cfg->hostname);
	strncpy(dip->dop.sip, dip->ip, BUF_SIZE_16);
	strncpy(dip->dop.lip, detect.other.localaddr, BUF_SIZE_16);
}

static void ipCreateNodetectHandle(detect_cfg_t *cfg)
{
	int i;
	detect_ip_t dip, *rfc;

	for (i = 0; i < cfg->ips.num; ++i)
	{
		runningSched(300);
		xmemzero(&dip, detect.tsize.detect_ip_t_size);
		strncpy(dip.ip, cfg->ips.ip[i].ip, BUF_SIZE_16);
		dip.iptype = cfg->ips.ip[i].iptype;
		dip.rfc = cfg;
		nodetectAssign(&dip, cfg);
		rfc = llistInsert(detect.data.pllist, &dip, TAIL);
		cfg->ips.ip[i].rfc = rfc;
	}
	//xfree((void *)&cfg->ips.ip);
}

static void ipCommonCreateDetectHandle(detect_cfg_t *cfg)
{
	int i, j, times;
	detect_ip_t dip, *rfc;

	times =  (cfg->flags.referer) ? cfg->times * 2 : cfg->times;
	for (i = 0; i < cfg->ips.num; ++i)
	{   
		runningSched(300);
		xmemzero(&dip, detect.tsize.detect_ip_t_size);
		dip.rfc = cfg;
		dip.times = times;
		dip.iptype = cfg->ips.ip[i].iptype;
		strncpy(dip.ip, cfg->ips.ip[i].ip, BUF_SIZE_16);
		dip.units = xcalloc(dip.times, detect.tsize.detect_unit_t_size);
		for (j = 0; j < dip.times; ++j)
		{
			dip.units[j].flag = READY;
			if (dip.rfc->flags.referer && j >= dip.rfc->times)
				dip.units[j].type = DWAY_REFERER;
			else
				dip.units[j].type = DWAY_COMMON;
		}
		if (FAILED == socketInit(&dip))
			continue;
		rfc = llistInsert(detect.data.pllist, &dip, TAIL);
		cfg->ips.ip[i].rfc = rfc;
	}
	//xfree((void *)&cfg->ips.ip);
}

static void ipCentralCreateDetectHandle(detect_cfg_t *cfg)
{
	void *entry;
	detect_ip_t dip;
	int i, j, find, times;
	char uri[BUF_SIZE_2048];
	char hashstr[BUF_SIZE_2048];
	hash_factor_t **head, *node, *cur;

	times =  (cfg->flags.referer) ? cfg->times * 2 : cfg->times;
	for (i = 0; i < cfg->ips.num; ++i)
	{
		runningSched(300);
		find = 0;
		xmemzero(uri, BUF_SIZE_2048);
		xmemzero(hashstr, BUF_SIZE_2048);
		xmemzero(&dip, detect.tsize.detect_ip_t_size);
		snprintf(uri, BUF_SIZE_2048, "%s%s",
				cfg->method1->hostname, cfg->method1->filename);
		snprintf(hashstr, BUF_SIZE_2048, "%s%s", cfg->ips.ip[i].ip, uri);
		head = &hashtable[hashKey(hashstr, MAX_HASH_NUM)];
		for (cur = *head; cur != NULL; cur = cur->next)
		{
			if (!strcmp(cur->ip, cfg->ips.ip[i].ip) && !strcmp(cur->uri, uri))
			{
				find = 1;
				dip.once.nodetect = 1;
				dip.once.rfc = cur->rfc;
				break;
			}
		}
		dip.rfc = cfg;
		dip.iptype = cfg->ips.ip[i].iptype;
		strncpy(dip.ip, cfg->ips.ip[i].ip, BUF_SIZE_16);
		if (!find)
		{
			dip.times = times;
			dip.units = xcalloc(dip.times, detect.tsize.detect_unit_t_size);
			for (j = 0; j < dip.times; ++j)
			{
				dip.units[j].flag = READY;
				if (dip.rfc->flags.referer && j >= dip.rfc->times)
					dip.units[j].type = DWAY_REFERER;
				else
					dip.units[j].type = DWAY_COMMON;
			}
			if (FAILED == socketInit(&dip))
				continue;
		}
		entry = llistInsert(detect.data.pllist, &dip, TAIL);
		cfg->ips.ip[i].rfc = entry;
		if (!find)
		{
			node = xcalloc(1, detect.tsize.hash_factor_t_size);
			node->rfc = entry;
			strcpy(node->uri, uri);
			strncpy(node->ip, dip.ip, BUF_SIZE_16);
			if (NULL == *head)
				node->next = NULL;
			else
				node->next = *head;
			*head = node;
		}
	}
	//xfree((void *)&cfg->ips.ip);
}

static void ipCreateKeepHandle(detect_cfg_t *cfg)
{
	int i;
	detect_ip_t dip, *rfc;

	for (i = 0; i < cfg->ips.num; ++i)
	{
		runningSched(300);
		xmemzero(&dip, detect.tsize.detect_ip_t_size);
		strncpy(dip.ip, cfg->ips.ip[i].ip, BUF_SIZE_16);
		dip.rfc = cfg;
		//if not find last result, no detect
		if (FAILED == keepLastResult(&dip))
		{
			cfg->flags.detect = 0;
			nodetectAssign(&dip, cfg);
		}
		else
		{
			outputGetIpType(&dip);
			dip.dop.detect = xstrdup("reuse");
			dip.dop.hostname = xstrdup(cfg->hostname);
			strncpy(dip.dop.sip, dip.ip, BUF_SIZE_16);
			strncpy(dip.dop.lip, detect.other.localaddr, BUF_SIZE_16);
		}
		rfc = llistInsert(detect.data.pllist, &dip, TAIL);
		cfg->ips.ip[i].rfc = rfc;
	}
	//xfree((void *)&cfg->ips.ip);
}

static void ipCommonCreate(detect_cfg_t *cfg)
{
	switch (cfg->flags.detect)
	{
		case 0: //no detect
			ipCreateNodetectHandle(cfg);
			break;
		case 1: //detect
			ipCommonCreateDetectHandle(cfg);
			break;
		case 2: //keep using last detect result
			ipCreateKeepHandle(cfg);
			break;
		default:
			xabort("ipCommonCreate: no this type");
	}
}

static void hashtableCheckCreate(detect_cfg_t *cfg)
{
	switch (cfg->flags.detect)
	{
		case 0: //no detect
			ipCreateNodetectHandle(cfg);
			break;
		case 1: //detect
			ipCentralCreateDetectHandle(cfg);
			break;
		case 2: //keep using last detect result
			ipCreateKeepHandle(cfg);
			break;
		default:
			xabort("hashtableCheckCreate: no this type");
	}
}

static void hashtableSafeFree(void)
{
	int i;
	hash_factor_t *cur, *next;

	for (i = 0; i < MAX_HASH_NUM; ++i)
	{
		for (cur = hashtable[i]; cur != NULL; cur = next)
		{
			next = cur->next;
			xfree((void *)&cur);
		}
		hashtable[i] = NULL;
	}
	xfree((void *)&hashtable);
}

static void ipCentralCreate(detect_cfg_t *cfg)
{
	hashtableCheckCreate(cfg);
}

static void ipCreateTravel(void *data)
{
	detect_cfg_t *cfg = data;

	assert(cfg != NULL);
	if (detect.opts.F.central)
		ipCentralCreate(cfg);
	else
		ipCommonCreate(cfg);
}

static int createIpList(void)
{
	int number = 0;

	hashtableCreate();
	number = llistTravel(detect.data.cllist, ipCreateTravel);
	dlog(COMMON, NOFD, "total number of hostname, NUMBER=[%d]", number);
	if (detect.opts.F.print)
	{
		fprintf(stdout, " ==> ip list created, total number: %d\n", number);
		fflush(stdout);
	}
	hashtableSafeFree();
	//lastfp only for creating ip list
	if (detect.other.lastfp != NULL)
	{
		fclose(detect.other.lastfp);
		detect.other.lastfp = NULL;
	}

	return SUCC;
}

static int setSockfdIndexTable(detect_event_t *event)
{
	size_t size;
	data_index_t *idx;
	detect_index_t *index;

	index = &detect.data.index;
	if (event->unit->sockfd >= index->attr.max_capacity)
	{
		index->attr.max_capacity = event->unit->sockfd + index->attr.grow_step;
		size = index->attr.max_capacity * detect.tsize.data_index_t_size;
		index->idx = xrealloc(index->idx, size);
	}
	idx = index->idx + event->unit->sockfd;
	xmemzero(idx, detect.tsize.data_index_t_size);
	idx->inuse = 1;
	idx->event = event;
	if (event->unit->sockfd > index->max_sockfd)
		index->max_sockfd = event->unit->sockfd;
	++index->attr.current_num;

	return SUCC;
}

static int beforeConnectHandle(detect_event_t *event)
{
	socketCall(SOCK_GETFD, 0, &event->dip->socket);
	event->unit->sockfd = event->dip->socket.sockfd;

	return setSockfdIndexTable(event);
}

static int runningBreakCheck(void)
{
	if (detect.runstate.starttime + detect.opts.killtime < time(NULL))
		return 1;

	return llistIsEmpty(detect.data.ellist);
}

static void  detectConnect(detect_event_t *event)
{
	event->unit->flag = CONNECTING;
	if (FAILED == beforeConnectHandle(event))
	{
		xdisconnect(event->unit->sockfd);
		return;
	}
	xconnect(event);
}

static void eventCreateTravel(void *data)
{
	int i;
	detect_event_t event;
	detect_ip_t *dip = data; 

	if (dip->rfc->flags.detect != 1 || dip->once.nodetect)
		return; 

	for (i = 0; i < dip->times; ++i)
	{
		runningSched(300);
		xmemzero(&event, detect.tsize.detect_event_t_size);
		event.flag = 0;
		event.dip = dip;
		event.unit = dip->units + i;
		llistInsert(detect.data.ellist, &event, TAIL);
	}
}

static void createEventList(void)
{
	int number = 0;

	llistTravel(detect.data.pllist, eventCreateTravel);
	number = llistNumber(detect.data.ellist);
	dlog(COMMON, NOFD, "total number of event, NUMBER=[%d]", number);
	if (detect.opts.F.print)
	{
		fprintf(stdout, " ==> event list created, total number: %d\n", number);
		fflush(stdout);
	}
}

static void eventTravel2(void *data)
{
	detect_event_t *event = data;

	assert(event != NULL);
	eventStart(event);
}

static void eventLoop(void)
{
	llistTravel2(detect.data.ellist, eventTravel2, detect.opts.travel);
}

static void createIndexTable(void)
{
	detect_index_t *index;

	index = &detect.data.index;
	index->attr.current_num = 0;
	index->attr.max_capacity = index->attr.grow_step;
	index->idx = xcalloc(index->attr.max_capacity, detect.tsize.data_index_t_size);
}

static void createEpoll(void)
{
	epollCall(EPOLL_CREATE, 0, 0, NULL);
}

static void readyToRunning(void)
{
	debug(BEFORE_READY);
	createIpList();
	createIndexTable();
	createEpoll();
	createEventList();
	debug(AFTER_READY);
}

static void detectRunning(void)
{
	debug(BEFORE_DETECT);

	do {
		if (runningBreakCheck())
			break;
		eventLoop();
		epollCall(EPOLL_WAIT, 0, 0, epollWaitHandle);
	} while (1);

	debug(AFTER_DETECT);
}

static void destroyAfterRunning(void)
{
	time(&detect.runstate.endtime);
	epollCall(EPOLL_DESTROY, 0, 0, NULL);
}

static void detectStart(void)
{
	readyToRunning();
	detectRunning();
	destroyAfterRunning();
}

int detectCall(const int calltype)
{
	detectStart();
	return STATE_OUTPUT;
}


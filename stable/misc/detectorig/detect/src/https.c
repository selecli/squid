#include "detect.h"

//NOTE: https connect and SSL_connect, socket must be blocked
//after https connect and SSL_connect, we can set non-blocking

int httpsConnect(const int sockfd)
{
	SSL *ssl;   
	SSL_CTX *ctx;   
	detect_event_t *event;
	struct timeval tv = {10, 0};

	socketCall(SOCK_BLOCKING, sockfd, NULL);
	event = detect.data.index.idx[sockfd].event;
	SSL_load_error_strings();   
	SSL_library_init();   
	ctx = SSL_CTX_new(SSLv23_client_method());   
	if (NULL == ctx)
	{   
		dlog(ERROR, sockfd,
				"SSL_CTX_new() failed, ERROR=[%s]\n",
				ERR_reason_error_string(ERR_get_error()));   
		return FAILED;
	}   
	event->unit->ctx = ctx;
	ssl = SSL_new(ctx);   
	if (NULL == ssl)
	{   
		dlog(ERROR, sockfd,
				"SSL_new() failed, ERROR=[%s]\n",
				ERR_reason_error_string(ERR_get_error()));   
		return FAILED;
	}   
	event->unit->ssl = ssl;
	if (0 == SSL_set_fd(ssl, sockfd))
	{   
		dlog(ERROR, sockfd,
				"SSL_set_fd() failed, ERROR=[%s]\n",
				ERR_reason_error_string(ERR_get_error()));   
		return FAILED;
	}   
	RAND_poll();   
	while (0 == RAND_status())
	{   
		unsigned short rand_ret = rand() % 65536;   
		RAND_seed(&rand_ret, sizeof(rand_ret));   
	}   
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (SSL_connect(ssl) != 1)
	{   
		dlog(ERROR, sockfd,
				"SSL_connect(), ERROR=[%s]\n",
				ERR_reason_error_string(ERR_get_error()));   
		return FAILED;
	}   
	socketCall(SOCK_NONBLOCKING, sockfd, NULL);

	return SUCC;
}

void httpsSendData(const int sockfd)
{
	int ret, retry;
	commdata_t *out;
	detect_unit_t *unit;

	unit = detect.data.index.idx[sockfd].event->unit;
	out = &unit->comm.out;
	retry = 3;

	while (out->curlen > 0)
	{   
		ret = SSL_write(unit->ssl, out->buffer + out->offset, out->curlen);   
		if (ret < 0)
		{   
			if (EINTR == errno && --retry)
			{   
				usleep(10);
				continue;
			}   
			dlog(WARNING, sockfd,
					"send() failed, ERROR=[%s]",
					ERR_reason_error_string(ERR_get_error()));   
			out->handler(sockfd, FAILED);
			return;
		}   
		out->offset += ret;
		out->curlen = out->bufsize - out->offset;
	};  
	out->handler(sockfd, SUCC);
}

void httpsRecvData(const int sockfd)
{
	int ret, retry;
	commdata_t *in;
	detect_unit_t *unit;

	unit = detect.data.index.idx[sockfd].event->unit;
	in = &unit->comm.in;
	retry = 3;

	while (in->curlen > 0)
	{
		ret = SSL_read(unit->ssl, in->buffer + in->offset, in->curlen);
		if (ret < 0)
		{
			if ((EAGAIN == errno || EINTR == errno) && --retry)
			{
				usleep(10);
				continue;
			}
			if (in->offset > 0)
			{
				dlog(WARNING, sockfd,
						"recv() failed, few data received, ERROR=[%s]",
						ERR_reason_error_string(ERR_get_error()));   
			}
			else
			{
				dlog(WARNING, sockfd,
						"recv() failed, no data received, ERROR=[%s]",
						ERR_reason_error_string(ERR_get_error()));   
			}
			in->handler(sockfd, FAILED);
			return;
		}
		if (0 == ret)
		{
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

void httpsClose(const int sockfd)
{
	detect_event_t *event;

	event = detect.data.index.idx[sockfd].event;
	if (NULL != event->unit->ssl)
	{
		SSL_shutdown(event->unit->ssl);
		SSL_free(event->unit->ssl);    
	}
	if (NULL != event->unit->ctx)
		SSL_CTX_free(event->unit->ctx);   
	ERR_free_strings();   
}


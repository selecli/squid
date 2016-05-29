#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SIZE_256  256
#define SIZE_512  512
#define SIZE_1KB  1024
#define SIZE_2KB  2048
#define SIZE_4KB  4096
#define SIZE_8KB  8192
#define SIZE_16KB 16384
#define SIZE_32KB 32768

#define REFRESH_EXPIRE      ((unsigned int)1)
#define REFRESH_PURGE       ((unsigned int)1 << 1)
#define REFRESH_DIR         ((unsigned int)1 << 2)
#define REFRESH_DIR_EXPIRE  (((unsigned int)1) | ((unsigned int)1 << 2))
#define REFRESH_DIR_PURGE   (((unsigned int)1 << 1) | ((unsigned int)1 << 2))

#define APP_NAME    "preloader_client"
#define APP_VERSION "1.0.1"

#define DEFAULT_PORT 21108
#define DEFAULT_HOST "127.0.0.1"

#define SESSIONID_PREFIX "RFC"

#define xerror() strerror(errno)
#define xfree(ptr) if (NULL != ptr) {free(ptr); ptr = NULL;}

typedef enum {
	DATA_TYPE_NONE,
	DATA_TYPE_URL,
	DATA_TYPE_OTHER,
} DataType_t;

typedef enum {
	METHOD_NONE,
	METHOD_URL_EXPIRE,
	METHOD_URL_PURGE,
	METHOD_DIR_EXPIRE,
	METHOD_DIR_PURGE,
} RefreshMethod_t;

typedef struct data_buffer_st DataBuffer_t;
typedef struct data_buffer_st HttpRequestHeader_t;

struct data_buffer_st {
	char *buffer;        /* memory block */
	size_t data_length;  /* data length */
	size_t buffer_size;  /* memory block size */
	DataBuffer_t *next;
};

typedef struct {
	char *sessionid;
	unsigned int url_id_seed;
	unsigned int url_count;
	unsigned int max_url_number;
	unsigned int buffer_count;
	unsigned long total_length;
	DataBuffer_t *head;
	DataBuffer_t *tail;
} HttpRequestBody_t;

typedef struct {
	HttpRequestHeader_t *header;
	HttpRequestBody_t *body;
} HttpRequest_t;

typedef struct {
	/* nothing */
} HttpReply_t;

typedef struct {
	int fd;
	int using;
	time_t start;   /* start time, for checking reply-waiting timeout */
	HttpRequest_t *request;
	HttpReply_t *reply;
} HttpEntry_t;

typedef struct {
	int capacity;
	int number;
	HttpEntry_t **entries;   /* insert in the order of FD */
} HttpEntryTable_t;

typedef struct {
	long action;
	unsigned int port;
	unsigned int number;
	unsigned int divide_number;
	char *url;
	char *host;
	char *url_file;
	char *dir_path;
	RefreshMethod_t method;
} RefreshOption_t;

typedef struct {
	unsigned int Pointer_t;     /* pointer type size */
	unsigned int DataBuffer_t;
	unsigned int HttpEntry_t;
	unsigned int HttpRequest_t;
	unsigned int HttpRequestHeader_t;
	unsigned int HttpRequestBody_t;
} DataTypeSize_t;

const char * ActionStr[] = {
	"preload",
	"refresh",
	"refresh, preload",
	"hangup",
	"remove"
};

static int g_epoll_fd = -1;
static int g_epoll_timeout = 20;
static int g_epoll_maxevents = 1024;
static struct epoll_event g_epoll_events[1024];

static FILE *g_fp = NULL;
static time_t g_start_time = 0;
static unsigned int g_process_id = 0;
static unsigned int g_refresh_type = 0;
static HttpEntryTable_t g_het = {0, 0, NULL}; 
static DataTypeSize_t g_tsize = {0, 0, 0, 0, 0, 0};
static RefreshOption_t g_opts = {0, 0, 1, 1, NULL, NULL, NULL, NULL, METHOD_NONE};


static double g_submit_usedtime   = 0.0;
static double g_refresh_usedtime  = 0.0;
static unsigned int g_remain_url_number = 1;
static unsigned int g_submit_url_number = 0;
static struct timeval g_submit_start     = {0, 0};
static struct timeval g_submit_finish    = {0, 0};
static struct timeval g_refresh_finish   = {0, 0};

static void printUsage(void);

/*************************** Tools ****************************
 *                 Memory And String Process                  *
 **************************************************************/

static void *xmalloc(size_t sz) 
{
	void *m = NULL;

	assert(0 != sz); 
	m = malloc(sz);
	assert(NULL != m);

	return m;
}

static void *xcalloc(size_t n, size_t sz) 
{
	void *m = NULL;

	assert(0 != n && 0 != sz); 
	m = calloc(n, sz);
	assert(NULL != m);

	return m;
}

static void *xrealloc(void *ptr, size_t sz) 
{
	void *m = NULL;

	/* We don't want sz == 0, but ptr can be NULL.
	 * If ptr is NULL, the call(realloc()) is equivalent to malloc().
	 */
	assert(0 != sz); 
	m = realloc(ptr, sz);
	assert(NULL != m);

	return m;
}

static char *xstrdup(const char *src)
{
	size_t l = 0;
	char *dst = NULL;

	assert(NULL != src);
	l = strlen(src);
	dst = xmalloc(l + 1); 
	memcpy(dst, src, l); 
	dst[l] = '\0';

	return dst;
}

/*********************** Preload Submit ***********************
 *                                                            *
 **************************************************************/

static void *dataBufferAllocate(const size_t size)
{
	DataBuffer_t *db = NULL;

	db = xmalloc(g_tsize.DataBuffer_t);
	db->buffer = xcalloc(1, size);
	db->buffer_size = size;
	db->data_length = 0;
	db->next = NULL;

	return db;
}

static void *dataBufferLimitAllocate(const size_t size)
{
	size_t kb = 0;
	size_t sz = 0;
	DataBuffer_t *db = NULL;

	kb = (size + 1) / SIZE_1KB + (!!(size + 1) % SIZE_1KB);

	switch (kb)
	{
		case 1:
			sz = SIZE_1KB;
			break;
		case 2:
			sz = SIZE_2KB;
			break;
		case 4:
			sz = SIZE_4KB;
			break;
		case 8:
			sz = SIZE_8KB;
			break;
		case 16:
			sz = SIZE_16KB;
			break;
		case 32:
			sz = SIZE_32KB;
			break;
		default:
			sz = size + 1;
			break;
	}

	db = xmalloc(g_tsize.DataBuffer_t);
	db->buffer = xcalloc(1, sz);
	db->buffer_size = sz;
	db->data_length = 0;
	db->next = NULL;

	return db;
}

static void eraseSpaceCRLF(char *buffer)
{
	char *p_CR, *p_LF;
	char *p_char = buffer;

	if (NULL == buffer || '\0' == *buffer)
	{
		return;
	}

	/* erase space */
	while (' ' == *p_char || '\t' == *p_char) ++p_char;
	if (p_char > buffer)
	{
		memmove(buffer, p_char, strlen(p_char));
	}

	/* erase CR, LF */
	p_CR = strchr(buffer, '\r');
	p_LF = strchr(buffer, '\n');
	if (NULL != p_CR)
	{
		*p_CR = '\0';
	}
	if (NULL != p_LF)
	{
		*p_LF = '\0';
	}
}

static int ignoreError(int errorno)
{
	switch (errorno)
	{
		case EAGAIN:
		case EINTR:
			return 1;
		default:
			return 0;
	}

	return 0;
}

static int fdSetNonblocking(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
	{
		return -1;
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		return -1;
	}

	return 0;
}

static void httpEntryTableInsert(int fd, HttpRequest_t *request)
{
	size_t new_memory_size;
	size_t new_memset_size;
	void *new_memset_begin;
	HttpEntry_t *entry = NULL;

	if (fd >= g_het.capacity)
	{
		new_memory_size = fd * g_tsize.Pointer_t;
		new_memset_begin = g_het.entries + g_het.capacity;
		new_memset_size = (fd - g_het.capacity + 1) * g_tsize.Pointer_t;
		g_het.entries = xrealloc(g_het.entries, new_memory_size);
		memset(new_memset_begin, 0, new_memset_size);
		g_het.capacity = fd;
	}

	entry = xcalloc(1, g_tsize.HttpEntry_t);
	entry->fd = fd;
	entry->using = 1;
	entry->reply = NULL;
	entry->request = request;
	entry->start = time(NULL);
	g_het.entries[fd] = entry;
	g_het.number++;
}

static void httpRequestBodyBufferAppend(
		HttpRequest_t *request,
		const char *buffer,
		const size_t length,
		const DataType_t type)
{
	DataBuffer_t *db = NULL;
	DataBuffer_t **next = NULL;
	HttpRequestBody_t *body = request->body;

	if (0 == body->buffer_count)
	{
		next = &body->tail;
		*next = dataBufferAllocate(SIZE_32KB);
		body->head = body->tail;
		body->buffer_count++;
	}
	else
	{
		if (body->tail->data_length + length > body->tail->buffer_size)
		{
			next = &body->tail->next;
			*next = dataBufferAllocate(SIZE_32KB);
			body->tail = *next;
			body->buffer_count++;
		}
	}

	db = body->tail;
	memcpy(db->buffer + db->data_length, buffer, length);
	db->data_length += length;
	body->total_length += length;
	if (DATA_TYPE_URL == type)
	{
		body->url_count++;
		g_submit_url_number++;
		g_remain_url_number--;
	}
}

static void httpRequestBodyUrlAppend(HttpRequest_t *request, const char *url)
{
	size_t offset = 0;
	char buffer[SIZE_4KB];

	if (NULL == url || '\0' == *url)
	{
		/* url is NULL or url length is 0 */
		return;
	}

	memset(buffer, 0, SIZE_4KB);
	/* url max length is 4KB */
	offset += snprintf(buffer + offset, SIZE_4KB - offset,
			"<url id=\"%u\">%s</url>\n", request->body->url_id_seed++, url);

	httpRequestBodyBufferAppend(request, buffer, offset, DATA_TYPE_URL);
}

static void httpRequestBodyOptionUrlAppend(HttpRequest_t *request)
{
	httpRequestBodyUrlAppend(request, g_opts.url);
	/* free it here, in case of resubmit */
	xfree(g_opts.url);
}

static void httpRequestBodyFileUrlAppend(HttpRequest_t *request)
{
	char buffer[SIZE_4KB];
	HttpRequestBody_t *body = request->body;

	if (NULL == g_opts.url_file)
	{
		/* not specified the url file */
		return;
	}

	if (NULL == g_fp)
	{
		g_fp = fopen(g_opts.url_file, "r");
		if (NULL == g_fp)
		{
			fprintf(stderr, "Error: fopen() error -- %s\n", xerror());
			exit(1);
		}
	}

	while (body->url_count < body->max_url_number)
	{
		memset(buffer, 0, SIZE_4KB);
		if (NULL == fgets(buffer, SIZE_4KB, g_fp))
		{
			if (feof(g_fp))
			{
				/* reached the end of the file */
				g_remain_url_number = 0;
				break;
			}
			fprintf(stderr, "Error: fgets() error -- %s\n", xerror());
			exit(1);
		}
		eraseSpaceCRLF(buffer);
		httpRequestBodyUrlAppend(request, buffer);
	}
}

static void httpRequestBodyCreateStep1(HttpRequest_t *request, const int number)
{
	char buffer[SIZE_256];

	request->body = xcalloc(1, g_tsize.HttpRequestBody_t);
	request->body->head = NULL;
	request->body->tail = NULL;
	request->body->url_count = 0;
	request->body->url_id_seed = 0;
	request->body->buffer_count = 0;
	request->body->total_length = 0;
	request->body->max_url_number = number;
	memset(buffer, 0, SIZE_256);
	snprintf(buffer, SIZE_256,
			"%3.3s%05u%010ld",
			SESSIONID_PREFIX, g_process_id, g_start_time++);
	request->body->sessionid = xstrdup(buffer);
}

static void httpRequestBodyCreateStep2(HttpRequest_t *request)
{
	size_t offset = 0;
	char buffer[SIZE_4KB];

	memset(buffer, 0, SIZE_4KB);

	offset += sprintf(buffer + offset,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

	offset += sprintf(buffer + offset,
			"<preload_task sessionid=\"%s\">\n",
			request->body->sessionid);

	offset += sprintf(buffer + offset,
			"<action>%s</action>\n", ActionStr[g_opts.action]);

	offset += sprintf(buffer + offset,
			"<limit_rate>200k</limit_rate>\n");

	offset += sprintf(buffer + offset,
			"<check_type>md5</check_type>\n");

	offset += sprintf(buffer + offset,
			"<nest_track_level>1</nest_track_level>\n");

	offset += sprintf(buffer + offset,
			"<preload_address>127.0.0.1:80</preload_address>\n");

	offset += sprintf(buffer + offset,
			"<report_address need=\"yes\">127.0.0.1:80</report_address>\n");

	offset += sprintf(buffer + offset,
			"<url_list>\n");

	httpRequestBodyBufferAppend(request, buffer, offset, DATA_TYPE_OTHER);
}

static void	httpRequestBodyCreateStep3(HttpRequest_t *request)
{
	httpRequestBodyOptionUrlAppend(request);
	httpRequestBodyFileUrlAppend(request);
}

static void httpRequestBodyCreateStep4(HttpRequest_t *request)
{
	size_t offset = 0;
	char buffer[SIZE_1KB];

	memset(buffer, 0, SIZE_1KB);

	offset += snprintf(buffer + offset, SIZE_1KB - offset,
			"</url_list>\n");

	offset += snprintf(buffer + offset, SIZE_1KB - offset,
			"</preload_task>\n");

	httpRequestBodyBufferAppend(request, buffer, offset, DATA_TYPE_OTHER);
}

static int httpRequestBodyCreateStep5(HttpRequest_t *request)
{
	/* check body validity */

	if (0 == request->body->url_count)
	{
		fprintf(stderr, "Error: refresh url number can't be 0\n");
		return -1;
	}

	return 0;
}

static void httpRequestHeaderCreate(HttpRequest_t *request)
{
	size_t offset = 0;
	char buffer[SIZE_1KB];

	assert(NULL != request);
	memset(buffer, 0, SIZE_1KB);

	offset += snprintf(buffer + offset, SIZE_1KB - offset,
			"POST http://www.test.com.cn http/1.1\r\n");
	offset += snprintf(buffer + offset, SIZE_1KB - offset,
			"Content-Length: %ld\r\n", request->body->total_length);
	offset += snprintf(buffer + offset, SIZE_1KB - offset,
			"Connection: close\r\n");
	offset += snprintf(buffer + offset, SIZE_1KB - offset,
			"\r\n");

	request->header = dataBufferLimitAllocate(offset);
	memcpy(request->header->buffer, buffer, offset);
	request->header->data_length = offset;
}

static int httpRequestBodyCreate(HttpRequest_t *request, const int number)
{
	assert(NULL != request);

	httpRequestBodyCreateStep1(request, number);
	httpRequestBodyCreateStep2(request);
	httpRequestBodyCreateStep3(request);
	httpRequestBodyCreateStep4(request);
	if (httpRequestBodyCreateStep5(request) < 0)
	{
		/* invalid body */
		return -1;
	}

	return 0;
}

static void httpRequestHeaderSend(int fd)
{
	ssize_t n;
	ssize_t n_send = 0;
	HttpRequest_t *request = g_het.entries[fd]->request;
	DataBuffer_t *hdr = request->header;

	fprintf(stdout, "\n");
	fprintf(stdout, "SESSIONID           ACTION          NUMBER  STATUS  DST HOST\n");
	fprintf(stdout, "------------------  --------------  ------  ------  ---------------\n");
	fprintf(stdout, "%s  Preload Submit  %-6u  %-6s  %s\n",
			request->body->sessionid, request->body->url_count, "OK", g_opts.host);
	fprintf(stdout, "\n");

	while (n_send < hdr->data_length)
	{
		n = send(fd, hdr->buffer + n_send, hdr->data_length - n_send, 0);
		if (n < 0)
		{
			if (ignoreError(errno))
			{
				usleep(1);
				continue;
			}
			fprintf(stderr, "Error: send() header error -- (%d) %s\n", errno, xerror());
			exit(1);
		}
		n_send += n;
	}

	/* Success */
	fprintf(stdout, "%s\n", hdr->buffer);
}

static void httpRequestBodySend(int fd)
{
	int i;
	ssize_t n, n_send;
	HttpRequest_t *request = g_het.entries[fd]->request;
	HttpRequestBody_t *body = request->body;
	DataBuffer_t *db = body->head;

	for (i = 0; i < body->buffer_count; ++i)
	{
		n_send = 0;

		while (n_send < db->data_length)
		{
			n = send(fd, db->buffer + n_send, db->data_length - n_send, 0);
			if (n < 0)
			{
				if (ignoreError(errno))
				{
					usleep(1);
					continue;
				}
				fprintf(stderr, "Error: send() body error -- (%d) %s\n", errno, xerror());
				exit(1);
			}
			n_send += n;
		}
		fprintf(stdout, "%s\n", db->buffer);
		db = db->next;
		if (NULL == db)
		{
			break;
		}
	}
}

static void httpRequestSendStart(int sockfd)
{
	struct epoll_event evt;

	evt.data.fd = sockfd;
	/* FIXME: EPOLLOUT ?? */
	evt.events = EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR;
	if (epoll_ctl(g_epoll_fd, EPOLL_CTL_MOD, sockfd, &evt) < 0)
	{
		fprintf(stderr, "Error: epol_ctl() error -- %s\n", xerror());
		exit(1);
	}
}

static void httpRequestSendDone(int sockfd)
{
	struct epoll_event evt;

	evt.data.fd = sockfd;
	evt.events = EPOLLIN|EPOLLHUP|EPOLLERR;
	if (epoll_ctl(g_epoll_fd, EPOLL_CTL_MOD, sockfd, &evt) < 0)
	{
		fprintf(stderr, "Error: epol_ctl() error -- %s\n", xerror());
		exit(1);
	}
}

static void httpRequestSend(int fd)
{
	httpRequestSendStart(fd);
	httpRequestHeaderSend(fd);
	httpRequestBodySend(fd);
	httpRequestSendDone(fd);
}

static void httpReplyRecv(int fd)
{
	int retry, loop;
	ssize_t n, n_recv;
	char buffer[SIZE_32KB];
	HttpRequest_t *request = g_het.entries[fd]->request;

	fprintf(stdout, "\n");
	fprintf(stdout, "SESSIONID           ACTION         NUMBER  STATUS  SRC HOST\n");
	fprintf(stdout, "------------------  -------------  ------  ------  ---------------\n");
	fprintf(stdout, "%s  Preload Reply  %-6u  %-6s  %s\n",
			request->body->sessionid, request->body->url_count, "OK", g_opts.host);
	fprintf(stdout, "\n");

	for (retry = n_recv = 0, loop = 1; loop; retry = n_recv = 0)
	{
		memset(buffer, 0, SIZE_32KB);

		do {
			n = recv(fd, buffer + n_recv, SIZE_32KB - n_recv, 0);
			if (n < 0)
			{
				if (ignoreError(errno))	
				{
					if (retry < 5)
					{
						usleep(1);
						continue;
					}
				}
				close(fd);
				loop = 0;
				fprintf(stderr, "Error: recv() error -- %s\n", xerror());
				break;
			}
			if (0 == n)
			{
				/* peer shutdown */
				close(fd);
				loop = 0;
				break;
			}
			n_recv += n;
		} while (n_recv < SIZE_32KB);

		fprintf(stdout, "%s\n", buffer);
	}
}

static HttpRequest_t *httpRequestCreate2(void)
{
	HttpRequest_t *request = NULL;

	request = xcalloc(1, g_tsize.HttpRequest_t);
	request->header = NULL;
	request->body = NULL;

	return request;
}

static int httpRequestCreate(int fd, const int number)
{
	HttpRequest_t *request = httpRequestCreate2();

	if (httpRequestBodyCreate(request, number) < 0)
	{
		xfree(request);
		return -1;
	}
	/* header packer must be here, after body packed,
	 * we must obtain the Content-Length value from body.
	 */
	httpRequestHeaderCreate(request);
	httpEntryTableInsert(fd, request);

	return 0;
}

static void refreshSubmitOne(const unsigned int number)
{
	int sockfd;
	struct sockaddr_in peer;
	struct epoll_event evts;

	memset(&peer, 0, sizeof(struct sockaddr_in));

	peer.sin_family = AF_INET;
	peer.sin_port = htons(g_opts.port);
	if (inet_pton(AF_INET, g_opts.host, (void *)&peer.sin_addr) <= 0)
	{
		fprintf(stderr, "Error: invalid IP -- %s\n", g_opts.host);
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		fprintf(stderr, "Error: socket() error -- %s\n", xerror());
		exit(1);
	}

	if (fdSetNonblocking(sockfd))
	{
		fprintf(stderr, "Error: set fd non-blocking error -- %s\n", xerror());
		exit(1);
	}

	if (connect(sockfd, (void *)&peer, sizeof(peer)) < 0)
	{
		if (EINPROGRESS != errno)
		{
			fprintf(stderr, "Error: connect() error -- %s\n", xerror());
			exit(1);
		}
		/* connect OK or EINPROGRESS, listen the event -- EPOLLOUT */
	}

	if (httpRequestCreate(sockfd, number) < 0)
	{
		close(sockfd);
		return;
	}

	evts.data.fd = sockfd;
	evts.events = EPOLLOUT|EPOLLHUP|EPOLLERR;
	if (epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, sockfd, &evts) < 0)
	{
		fprintf(stderr, "Error: epol_ctl() error -- %s\n", xerror());
		exit(1);
	}
}

static void mainRefreshSubmitStart(void)
{
	unsigned int m, n, number;

	g_remain_url_number = g_opts.number;
	m = g_opts.number / g_opts.divide_number;
	n = g_opts.number % g_opts.divide_number;

	gettimeofday(&g_submit_start, NULL);

	do {
		if (m && m <= g_remain_url_number)
		{
			number = m;
		}
		else if (n)
		{
			number = n;
		}
		else
		{
			break;
		}
		refreshSubmitOne(number);
	} while (g_remain_url_number);

	gettimeofday(&g_submit_finish, NULL);
}

static void mainDestroy(void)
{
	/* epoll close */
	if (close(g_epoll_fd) < 0)
	{   
		fprintf(stderr, "Error: close() error -- %s\n", xerror());
	}

	/* FIXME: memory free: process exit will free it */
}

static void mainLoopRunning(void)
{
	int i, n;

	for ( ; g_het.number > 0; )
	{
		n = epoll_wait(g_epoll_fd, g_epoll_events, g_epoll_maxevents, g_epoll_timeout);
		if (n < 0)
		{
			fprintf(stderr, "Error: epoll_wait() error -- %s\n", xerror());
			exit(1);
		}
		if (0 == n)
		{
			/* do nothing */
			continue;
		}
		for (i = 0; i < n; ++i)
		{
			if (g_epoll_events[i].events & EPOLLIN)
			{
				httpReplyRecv(g_epoll_events[i].data.fd);
				/* FIXME: make sure it can be remove */
				g_het.number--;
			}
			if (g_epoll_events[i].events & EPOLLOUT)
			{
				httpRequestSend(g_epoll_events[i].data.fd);
			}
			if (g_epoll_events[i].events & EPOLLHUP)
			{
				g_het.number--;
				fprintf(stderr, "Error: EPOLLHUP on fd -- %d\n", g_epoll_events[i].data.fd);
			}
			if (g_epoll_events[i].events & EPOLLERR)
			{
				g_het.number--;
				fprintf(stderr, "Error: EPOLLERR on fd -- %d\n", g_epoll_events[i].data.fd);
			}
		}
	}

	/* refresh finished */
	gettimeofday(&g_refresh_finish, NULL);
}

static void mainPrintStatistics(void)
{
	double start, s_finish, r_finish;

	start = g_submit_start.tv_sec * 1000000 + g_submit_start.tv_usec;
	s_finish = g_submit_finish.tv_sec * 1000000 + g_submit_finish.tv_usec;
	r_finish = g_refresh_finish.tv_sec * 1000000 + g_refresh_finish.tv_usec;
	g_submit_usedtime = (s_finish - start) / 1000000.0;
	g_refresh_usedtime = (r_finish - start) / 1000000.0;

	fprintf(stdout, "\n");
	fprintf(stdout, "PROGRAM NAME      ACTION   NUMBER  SUBMIT TIME  PRELOAD TIME  DST HOST\n");
	fprintf(stdout, "----------------  -------  ------  -----------  ------------  ---------------\n");
	fprintf(stdout, "preloader_client  Preload  %-6d  %011.3lf  %012.3lf  %s\n",
			g_submit_url_number, g_submit_usedtime, g_refresh_usedtime, g_opts.host);
	fprintf(stdout, "\n");
}

static void dataTypeSizeInitialize(void)
{
	g_tsize.Pointer_t = sizeof(void *);
	g_tsize.DataBuffer_t = sizeof(DataBuffer_t);
	g_tsize.HttpEntry_t = sizeof(HttpEntry_t);
	g_tsize.HttpRequest_t = sizeof(HttpRequest_t);
	g_tsize.HttpRequestHeader_t = sizeof(HttpRequestHeader_t);
	g_tsize.HttpRequestBody_t = sizeof(HttpRequestBody_t);
}

static void mainInitialize(void)
{
	/* process start time and id */
	g_start_time = time(NULL);
	g_process_id = getpid();

	/* epoll create */
	g_epoll_fd = epoll_create(g_epoll_maxevents);
	if (g_epoll_fd < 0)
	{
		fprintf(stderr, "Error: epoll_create() error -- %s\n", xerror());		
		exit(1);
	}

	/* data type size */
	dataTypeSizeInitialize();

	/* http entry table initialize */
	g_het.number = 0;
	/* 0, 1, 2 may have been opened, so +3 */
	/* FIXME: g_het.capacity = g_opts.divide_number * 2 + 3; */
	g_het.capacity = 65535;
	g_het.entries = xcalloc(1, g_tsize.Pointer_t * g_het.capacity);
}

/*************************** Main *****************************
 *               Parse Command Line Options                   *
 **************************************************************/

static int checkOptionValidity(void)
{
	switch (g_refresh_type)
	{
		case REFRESH_EXPIRE:
			g_opts.method = METHOD_URL_EXPIRE;
			break;
		case REFRESH_PURGE:
			g_opts.method = METHOD_URL_PURGE;
			break;
		case REFRESH_DIR_EXPIRE:
			g_opts.method = METHOD_DIR_EXPIRE;
			break;
		case REFRESH_DIR_PURGE:
			/* current, purge is the same as expire */
			g_opts.method = METHOD_DIR_EXPIRE;
			break;
		default:
			/* otherwise, error options */
			return 0;
	}

	return (NULL != g_opts.url || NULL != g_opts.url_file);
}

static void trySetDefaultAddress(void)
{
	/* default host */
	if (NULL == g_opts.host)
	{
		g_opts.host = xstrdup(DEFAULT_HOST);	
	}

	/* default port */
	if (0 == g_opts.port || g_opts.port > 65535)
	{
		g_opts.port = DEFAULT_PORT;	
	}
}

static void optionParseComplete(void)
{
	/* check options validity */
	if (checkOptionValidity())
	{
		fprintf(stderr, "Error: use options error -- %u\n\n", g_refresh_type);
		printUsage();
		exit(1);
	}

	trySetDefaultAddress();

	if (g_opts.action < 0 || g_opts.action > 4)
	{
		g_opts.action = 0;
	}

	/* default divide number */
	if (g_opts.divide_number <= 0)
	{
		g_opts.divide_number = 1;
	}
}

static void printVersion(void)
{
	fprintf(stdout,
			"\033[1m%s %s\033[0m\n\
			//\rBuildTime: %s %s\n\
			\rCopyright (C) 2013 ChinaCache\n",
			APP_NAME, APP_VERSION,
			__DATE__, __TIME__);

	/* whatever, flush the stdout */
	fflush(stdout);
}

static void printUsage(void)
{
	int i;
	const char *usage[] = {
		"\nUsage: preloader_client [OPTION] <argument>",
		"\t-a/--action rule   rule value: 0 or 1",
		"\t                   for url recursion rule:    0 -- yes;    1 -- no",
		"\t                   for directory match rule:  0 -- prefix; 1 -- regular",
		"\t-d/--directory     directory refresh",
		"\t-D/--divide N      divide into N url blocks to submit",
		"\t-e/--expire        url or directory expire",
		"\t-f/--file path     specify the url file",
		"\t-h/--help          show help information",
		"\t-H/--host          hostname or IP which will be submitted to",
		"\t-n/--number        url number",
		"\t-p/--port          port which will be submitted to",
		"\t-u/--url url       url submitted for refreshing",
		"\t-v/--version       display the version",
		NULL
	};

	for (i = 0; usage[i] != NULL; ++i)
	{
		fprintf(stdout, "%s\n", usage[i]);
	}

	/* whatever, flush the stdout */
	fflush(stdout);
}

static void mainParseOptions(int argc, char **argv)
{
	int opt, index;
	const char *optstr = ":a:dD:ef:hH:n:p:u:v";
	struct option options[] = {
		{"action",    1, NULL, 'a'},    /* dir match rule: prefix; regular */
		{"directory", 0, NULL, 'd'},    /* directory refresh */
		{"divide",    1, NULL, 'D'},    /* divide into N url blocks to submit */
		{"expire",    0, NULL, 'e'},    /* expire the url or directory */
		{"file",      1, NULL, 'f'},    /* specify the url file */
		{"help",      0, NULL, 'h'},    /* print help information */
		{"host",      1, NULL, 'H'},    /* hostname or IP which will be submitted to */
		{"number",    1, NULL, 'n'},    /* url number */
		{"port",      1, NULL, 'p'},    /* port which will be submitted to */
		{"url",       1, NULL, 'u'},    /* print help information */
		{"version",   0, NULL, 'v'},    /* display the version */
		{NULL,        0, NULL,  0 },
	};

	if (argc < 2 || NULL == argv)
	{
		/* first, check command line arguments */
		fprintf(stderr, "Error: preloader_client requires at least one option\n");
		printUsage();	
		exit(1);
	}

	do {
		opt = getopt_long(argc, argv, optstr, options, &index);
		switch (opt)
		{
			case 'a':
				g_opts.action = strtol(optarg, NULL, 10);
				break;
			case 'd':
				g_refresh_type |= REFRESH_DIR;
				break;
			case 'D':
				g_opts.divide_number = strtol(optarg, NULL, 10);
				break;
			case 'e':
				g_refresh_type |= REFRESH_EXPIRE;
				break;
			case 'f':
				if (NULL != g_opts.url_file)
				{
					fprintf(stderr, "Error: -f/--file only can be used once in command line\n");
					exit(1);
				}
				g_opts.url_file = xstrdup(optarg);
				break;
			case 'h':
				printUsage();
				exit(0);
				break;
			case 'H':
				if (NULL != g_opts.host)
				{
					fprintf(stderr, "Error: -H/--host only can be used once in command line\n");
					exit(1);
				}
				g_opts.host = xstrdup(optarg);
				break;
			case 'n':
				g_opts.number = (unsigned int)strtol(optarg, NULL, 10);
				break;
			case 'p':
				g_opts.port = (unsigned int)strtol(optarg, NULL, 10);
				break;
			case 'u':
				if (NULL != g_opts.url)
				{
					fprintf(stderr, "Error: -u/--url only can be used once in command line\n");
					exit(1);
				}
				g_opts.url = xstrdup(optarg);
				break;
			case 'v':
				printVersion();
				exit(0);
				break;
			case ':':
				fprintf(stderr, "Error: option [-%c] requires an argument\n", optopt);
				printUsage();
				exit(1);
			case '?':
				fprintf(stderr, "Error: option [-%c] unknown\n", optopt);				
				printUsage();
				exit(1);
			case -1:
				/* all command line options parsed OK */
			default:
				/* otherwise */
				break;
		}
	} while (-1 != opt);


	/* must be here, after all options parsed */
	optionParseComplete();
}

int main(int argc, char **argv)
{
	mainParseOptions(argc, argv);
	mainInitialize();
	mainRefreshSubmitStart();
	mainLoopRunning();
	mainPrintStatistics();
	mainDestroy();

	return 0;
}


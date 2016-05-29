#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_lscs.h>
#include <ngx_event.h>
#include <assert.h>

struct HttpMsgBuf { 
    char *buf;
    size_t size;
    /* offset of first/last byte of headers */
    int h_start, h_end, h_len;
    /* offset of first/last byte of request, including any padding */
    int req_start, req_end, r_len;
    int m_start, m_end, m_len;
    int u_start, u_end, u_len;
    int v_start, v_end, v_len;
    int v_maj, v_min;
};

typedef enum{
	normal,
	space,
	term
} char_type;

static ngx_uint_t alive_backends_num = 0;
static ngx_str_t **alive_backends = NULL;

static void ngx_lscs_init_request(ngx_event_t *ev);
static void ngx_lscs_close_connection(ngx_connection_t *c);
static void ngx_lscs_process_request_line(ngx_event_t *rev);
static void ngx_lscs_extract_uri(u_char * buf, size_t len, u_char **uri_start, ssize_t *uri_len, ngx_int_t *completed);
static ngx_uint_t ngx_lscs_hash(u_char * uri, size_t uri_len, ngx_uint_t buckets);
static ngx_uint_t ELF_Hash(u_char *uri, size_t uri_len, ngx_uint_t buckets);
static ngx_int_t ngx_lscs_get_peer_urlhash(ngx_peer_connection_t *pc, void *data);
static void ngx_lscs_send_fd(ngx_event_t *wev);
static void ngx_lscs_read_reply(ngx_event_t *rev);
static void HttpMsgBufInit(struct HttpMsgBuf * hmsg, char *buf, ssize_t size);
int httpMsgParseRequestLine(struct HttpMsgBuf * hmsg);
//static void ngx_lscs_connect_peer(ngx_event_t * wev);

static void HttpMsgBufInit(struct HttpMsgBuf * hmsg, char *buf, ssize_t size)
{
        hmsg->buf = buf;
        hmsg->size = size;
        hmsg->req_start = hmsg->req_end = -1; 
        hmsg->h_start = hmsg->h_end = -1;
        hmsg->r_len = hmsg->u_len = hmsg->m_len = hmsg->v_len = hmsg->h_len = 0; 
}

void ngx_lscs_init_connection(ngx_connection_t *c)
{
	ngx_event_t *rev;
	rev = c->read;
	rev->handler = ngx_lscs_init_request;
	c->write->handler = ngx_lscs_empty_handler;
	//printf("cleanup=%p\n",c->pool->cleanup);
	//c->pool->cleanup = NULL;
#if (NGX_STAT_STUB)
	(void) ngx_atomic_fetch_add(ngx_stat_reading, 1);
#endif

	if (rev->ready) {
		ngx_lscs_init_request(rev);
		return; 
	}

	ngx_add_timer(rev, c->listening->post_accept_timeout);
	ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0,
			"set timeout=%d",  c->listening->post_accept_timeout);
	
	if (ngx_handle_read_event(rev, 0) != NGX_OK) {
#if (NGX_STAT_STUB)
		(void) ngx_atomic_fetch_add(ngx_stat_reading, -1);
#endif
		//printf("connection close here:%d\n", __LINE__);
		ngx_lscs_close_connection(c);
	}
}

void ngx_lscs_empty_handler(ngx_event_t *wev)
{
	return; 
}

static void ngx_lscs_close_connection(ngx_connection_t *c)
{
	//printf("cleanup=%p\n",c->pool->cleanup);
	if(!c || c->fd < 0 || c->destroyed)
	{
		return;
	}
	
	ngx_pool_t  *pool;  

	ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0,
			"close lscs connection: %d", c->fd); 

#if (NGX_STAT_STUB)
	(void) ngx_atomic_fetch_add(ngx_stat_active, -1);
#endif

	c->destroyed = 1;

	pool = c->pool;

	ngx_close_connection(c);

	ngx_destroy_pool(pool);
}

static void ngx_lscs_init_request(ngx_event_t *rev)
{
	ngx_connection_t           *c;
	ngx_lscs_request_t         *r;
	ngx_lscs_port_t            *port;
	struct sockaddr_in         *sin;
	ngx_lscs_in_addr_t         *addr;
	ngx_lscs_addr_conf_t       *addr_conf;

	c = rev->data;

		ngx_log_debug3(NGX_LOG_DEBUG_LSCS, c->log, 0, "In ngx_lscs_init_request,rev->ready = %d,rev->timedout = %d,c->fd = %d\n", rev->ready,rev->timedout,c->fd);

	if (rev->timedout) {
		ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out,connection close!");

//		ngx_close_connection(c);
		ngx_lscs_close_connection(c);
		return; 
	}
	
//	if(!rev->ready)
//	{
//		ngx_log_debug3(NGX_LOG_DEBUG_LSCS, c->log, 0, "In ngx_lscs_init_request,rev->ready = %d,c->fd = %d,rev->timedout = %d;return and not check timedout!!!\n", rev->ready,c->fd,rev->timedout);
//		return;
//	}
	c->requests++;

		ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "In ngx_lscs_init_request,sizeof(ngx_lscs_request_t) = %d,sizeof(ngx_connection_t) = %d,sizeof(ngx_event_t) = %d\n", sizeof(ngx_lscs_request_t),sizeof(ngx_connection_t),sizeof(ngx_event_t));
	r = ngx_pcalloc(c->pool, sizeof(ngx_lscs_request_t));
	if (r == NULL) {
		ngx_lscs_close_connection(c);
		return;
	}
	
	c->data = r;
	c->sent = 0;
	r->signature = NGX_LSCS_MODULE;

	/* find the server configuration for the address:port */

	port = c->listening->servers;

	r->connection = c;

	if (port->naddrs > 1) {

		/*
		 * there are several addresses on this port and one of them
		 * is an "*:port" wildcard so getsockname() in ngx_lscs_server_addr()
		 * is required to determine a server address
		 */ 

		if (ngx_connection_local_sockaddr(c, NULL, 0) != NGX_OK) {
			//printf("connection close here:%d\n", __LINE__);
			ngx_lscs_close_connection(c);
			return;
		}

		switch (c->local_sockaddr->sa_family) {
			default: /* AF_INET */
				sin = (struct sockaddr_in *) c->local_sockaddr;

				addr = port->addrs;

				/* the last address is "*" */
				ngx_uint_t i;
				for (i = 0; i < port->naddrs - 1; i++) {
					if (addr[i].addr == sin->sin_addr.s_addr) {
						break;
					}
				}

				addr_conf = &addr[i].conf;

				break;
		}

	} else {
		switch (c->local_sockaddr->sa_family) {

			default: /* AF_INET */
				addr = port->addrs;
				addr_conf = &addr[0].conf;
				break;
		}
	}
	ngx_lscs_core_srv_conf_t *cscf = addr_conf->default_server;
	
	r->main_conf = cscf->ctx->main_conf;
	r->srv_conf = cscf->ctx->srv_conf;

	ngx_lscs_process_request_line(rev);

}

static int ngx_check_request_header(const char *check_url,int url_len)
{
	int ret = 0;
	if(check_url == strstr(check_url,"GET"))
			ret = 1;
	else if(check_url == strstr(check_url,"POST"))
			ret = 1;
	else if(check_url == strstr(check_url,"HEAD"))
			ret = 1;
	else if(check_url == strstr(check_url,"PUT"))
			ret = 1;
	else if(check_url == strstr(check_url,"PUT"))
			ret = 1;
	else if(check_url == strstr(check_url,"DELETE"))
			ret = 1;
	else if(check_url == strstr(check_url,"TRACE"))
			ret = 1;
	else if(check_url == strstr(check_url,"CONNECT"))
			ret = 1;
	else
			ret = -1;


return ret;
}

static void ngx_lscs_process_request_line(ngx_event_t *rev)
{
	ssize_t peeked_len = 0,len_save = 0,check_val;
	u_int loop = 1;
	//GET /u/r/i/filename.abc HTTP1.x
	ngx_connection_t *c = rev->data;
	ngx_lscs_request_t *r = c->data;
	u_char *host_start = NULL,*host_end = NULL,*head_end = NULL;
	//	static u_char url_buf[NGX_LSCS_MAX_URL];
	u_char *url_buf = realloc(NULL,NGX_LSCS_BUFFSIZE);
	memset(url_buf,0,NGX_LSCS_BUFFSIZE);
	peeked_len = c->peek(c, url_buf, NGX_LSCS_BUFFSIZE);
	ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "c->peek read return val peeked_len = %d\n", peeked_len);
	ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "c->peek read url_buf = %s\n", url_buf);

	if(peeked_len < 5)
	{
		//printf("connection close here:%d\n", __LINE__);
		ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "1 peek read return val peeked_len <= 5 ,fd = %d ;then close connect,now!\n", c->fd);
//		ngx_lscs_close_connection(c);
		ngx_lscs_close_connection(r->connection);
		free(url_buf);
		return;
	}

	struct HttpMsgBuf *msg;

	msg = malloc(sizeof(struct HttpMsgBuf));
	HttpMsgBufInit(msg, (char *)url_buf,peeked_len);
	if(httpMsgParseRequestLine(msg) != 1)
	{
		ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "request line unsuport! fd = %d <= 0;then close connect,now!\n", c->fd);
//		ngx_lscs_close_connection(c);
		ngx_lscs_close_connection(r->connection);
		free(url_buf);
		free(msg);
		return;

	}
	free(msg);
	check_val = ngx_check_request_header((const char *)url_buf,peeked_len);
	if(check_val == -1){
		ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "request line check error! fd = %d <= 0;then close connect,now!\n", c->fd);
//		ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "ERROR!Illegal request!!!");
//		ngx_lscs_close_connection(c);
		ngx_lscs_close_connection(r->connection);
		free(url_buf);
		return;
	}

	int host_len = 0;
	u_char host_buf[NGX_LSCS_MAX_URL];
	memset(host_buf,0,NGX_LSCS_MAX_URL);
	len_save = peeked_len;
	if((hash_method == HASH_URL || hash_method == HASH_HUR)){
		while(((host_start = (u_char *)strstr((char *)url_buf,"Host:")) == NULL || (host_end = (u_char *)strstr((char *)host_start,"\r\n")) == NULL) && (head_end = (u_char *)strstr((char *)url_buf,"\r\n\r\n")) == NULL){
			loop++;
			url_buf = realloc(url_buf,loop*NGX_LSCS_BUFFSIZE);
			memset(url_buf,0,loop*NGX_LSCS_BUFFSIZE);
			peeked_len = c->peek(c, url_buf, loop*NGX_LSCS_BUFFSIZE);
			if(peeked_len == len_save)
				break;
			else
				len_save = peeked_len;
			ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "peek read return val peeked_len = %d\n", peeked_len);
			if(peeked_len < 5)
			{
				//printf("connection close here:%d\n", __LINE__);
				ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "2 peek read return val peeked_len = %d <= 0;then close connect,now!\n", peeked_len);
//				ngx_lscs_close_connection(c);
				ngx_lscs_close_connection(r->connection);
				free(url_buf);
				return;
			}

			msg = malloc(sizeof(struct HttpMsgBuf));
			HttpMsgBufInit(msg, (char *)url_buf,peeked_len);
			if(httpMsgParseRequestLine(msg) != 1)
			{
//				ngx_lscs_close_connection(c);
				ngx_lscs_close_connection(r->connection);
				free(url_buf);
				free(msg);
				return;

			}
			free(msg);
			check_val = ngx_check_request_header((const char *)url_buf,peeked_len);
			if(check_val == -1){
				ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "ERROR!Illegal request!!!");
//				ngx_lscs_close_connection(c);
				ngx_lscs_close_connection(r->connection);
				free(url_buf);
				return;
			}
			ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "URL_BUF not big enough in while loop %d :old url_buf = %s\n", loop,url_buf);

		}
		ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "After peek read:old url_buf = %s\n", url_buf);
		if(host_start != NULL){
			if((host_start + 5 - url_buf) >= peeked_len){
//				hash_method = HASH_URH;
				strncpy((char *)host_buf,"nohost",7);
				host_len = 7;
			}
			host_start += 5;
			while(*host_start == ' '){
				host_start++;
				if(host_start - url_buf >= peeked_len){
//					hash_method = HASH_URH;
					strncpy((char *)host_buf,"nohost",7);
					host_len = 7;
				}

			}
			if(hash_method != HASH_URH){
				if(host_end == NULL){
					host_end = url_buf + peeked_len - 1;
				}
				host_len = host_end-host_start;
				if(host_len >= NGX_LSCS_MAX_URL){
					ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "Error:host_len longer than NGX_LSCS_MAX_URL:host_len = %d\n", host_len);
//					ngx_lscs_close_connection(c);
					ngx_lscs_close_connection(r->connection);
					free(url_buf);
					return;
				}
				strncpy((char *)host_buf, (const char *)host_start, (size_t)host_len);
			}
		}
		else{
			strncpy((char *)host_buf, "nohost",7);
			host_len = 7;
		}
	ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "host_buf = %s\n", host_buf);
	ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "host_len =  %d\n", host_len);
	}

	u_char *pla = NULL,*name_start = NULL,fname_buf[256];
	u_char uri_buf[NGX_LSCS_MAX_URL];
	u_int name_len = 0;
	ngx_int_t completed;
	memset(fname_buf,0,256);
	memset(uri_buf,0,NGX_LSCS_MAX_URL);
//again:
	switch (hash_method){
		case HASH_URI:
			ngx_lscs_extract_uri(url_buf, peeked_len, &r->uri_start, &r->uri_len, &completed);
			if((pla = (u_char *)strchr((char *)r->uri_start, '?')) != NULL)
			{
				if (pla - r->uri_start < r->uri_len)
					r->uri_len = pla - r->uri_start;
			}
			ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "In HASH_URI:r->uri_len = %d\n", r->uri_len);
			ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "In HASH_URI:url_buf = %s\n", url_buf);
			break;
		case HASH_URL:
//			if(host_start == NULL){
//				hash_method = HASH_URI;
//				goto again;
//			}else{
				ngx_lscs_extract_uri(url_buf, peeked_len, &r->uri_start, &r->uri_len, &completed);
				if((pla = (u_char *)strchr((char *)r->uri_start,'?')) != NULL)
				{
					if (pla - r->uri_start < r->uri_len)
						r->uri_len = pla - r->uri_start;
				}
				strncpy((char *)uri_buf, (const char *)r->uri_start, (size_t)r->uri_len);
				memset(url_buf,0,loop*NGX_LSCS_BUFFSIZE);
				strncpy((char *)url_buf, (const char *)host_buf, (size_t)host_len);
				strncat((char *)url_buf, (const char *)uri_buf, (size_t)r->uri_len);
				ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "In HASH_URL:uri_buf = %s\n", uri_buf);
				ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "In HASH_URL:host_buf+uri_buf = url_buf = %s\n", url_buf);
				ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "In HASH_URL:uri_len =  %d\n", r->uri_len);
				r->uri_len = r->uri_len + host_len;
				r->uri_start = url_buf;
//			}
			break;
		case HASH_URH:
			ngx_lscs_extract_uri(url_buf, peeked_len, &r->uri_start, &r->uri_len, &completed);
			ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "In HASH_URH:url_buf = %s\n", url_buf);

			if(!completed)
			{
				int bits = 0;
				size_t temp = r->uri_len;
				while(temp > 0)
				{
					temp = temp >> 1;
					bits ++;
				}
				r->uri_len = 1 << bits;

				//TODO: stats
			}
			break;
		case HASH_HUR:
//			if(host_start == NULL){
//				hash_method = HASH_FNAME;
//				goto again;
//			}else{
				ngx_lscs_extract_uri(url_buf, peeked_len, &r->uri_start, &r->uri_len, &completed);
				if((pla = (u_char *)strchr((char *)r->uri_start,'?')) != NULL)
				{
					if (pla - r->uri_start < r->uri_len)
						r->uri_len = pla - r->uri_start;
				}
				strncpy((char *)uri_buf, (const char *)r->uri_start, (size_t)r->uri_len);
				name_start = (u_char *)strrchr((char *)uri_buf,'/');
				name_len = r->uri_len - (name_start - uri_buf);
				ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "In HASH_HUR:name_len = %d\n", name_len);
				strncpy((char *)fname_buf, (const char *)name_start, (size_t)name_len);
				memset(url_buf,0,loop*NGX_LSCS_BUFFSIZE);
				strncpy((char *)url_buf, (const char *)host_buf, (size_t)host_len);
				strncat((char *)url_buf, (const char *)fname_buf, (size_t)name_len);
				ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "In HASH_HUR:host_buf + fname_buf = url_buf = %s\n", url_buf);
				r->uri_len = name_len + host_len;
				r->uri_start = url_buf;
//			}
			break;
		case HASH_FNAME:
			ngx_lscs_extract_uri(url_buf, peeked_len, &r->uri_start, &r->uri_len, &completed);
			if((pla = (u_char *)strchr((char *)r->uri_start,'?')) != NULL)
			{
				if (pla - r->uri_start < r->uri_len)
					r->uri_len = pla - r->uri_start;
			}
			strncpy((char *)uri_buf, (const char *)r->uri_start, (size_t)r->uri_len);
			name_start = (u_char *)strrchr((char *)uri_buf, '/');
			name_len = r->uri_len - (name_start - uri_buf);
			strncpy((char *)fname_buf, (const char *)name_start, (size_t)name_len);
			memset(url_buf,0,loop*NGX_LSCS_BUFFSIZE);
			strncat((char *)url_buf, (const char *)fname_buf, (size_t)name_len);
			ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "In HASH_FNAME:fname_buf = url_buf = %s\n", url_buf);
			r->uri_len = name_len;
			r->uri_start = url_buf;
			break;
		case HASH_ROUND_ROBIN:
			//do nothing!
			break;
		default:
			ngx_log_error(NGX_LOG_INFO, c->log, 0, "hash_meth undefined!!!error!!!\n");
			break;	
	}

	//The complete uri process
	//TODO: long connection


	//r->peer.url_buf = url_buf; //make the place of url_buf for free.
	r->peer.log = c->log;
	r->peer.log_error = NGX_ERROR_ERR;

#if (NGX_THREADS)
	r->peer.lock = &r->connection->lock;
#endif
	r->peer.data = r;
	//TODO: configurable
	r->peer.get = ngx_lscs_get_peer_urlhash;

	ngx_lscs_core_srv_conf_t *cscf = ngx_lscs_get_module_srv_conf(r, ngx_lscs_core_module);
	ngx_uint_t nelts = cscf->backends->nelts;
	ngx_uint_t backend_togo = ngx_lscs_hash(r->uri_start, r->uri_len, nelts);
	ngx_str_t *backnames = cscf->backends->elts;
	ngx_uint_t n, m;
	ngx_int_t rc = NGX_ERROR;

	if (NULL == alive_backends)
	{
		alive_backends_num = nelts;
		alive_backends = malloc(nelts * sizeof(ngx_str_t *));	
		for (n = 0; n < nelts; ++n)
		{
			alive_backends[n] = &backnames[n];
		}
	}
	r->backname = &backnames[backend_togo];
	rc = ngx_event_connect_peer(&r->peer);
	if(NGX_OK == rc)
	{
		ngx_uint_t insert_flag = 0;

		for (n = 0; n < alive_backends_num; ++n)
		{
			//sort by backnames's address
			if (alive_backends[n] == r->backname)
			{
				insert_flag = 1;
				break;
			}
			if (alive_backends[n] > r->backname)
			{
				for (m = alive_backends_num; m > n; --m)
				{
					alive_backends[m] = alive_backends[m - 1];
				}
				alive_backends[n] = r->backname;
				++alive_backends_num;
				insert_flag = 1;
				break;
			}
		}
		if (0 == insert_flag)
		{
			alive_backends[alive_backends_num] = r->backname;
			++alive_backends_num;
		}
	}
	else
	{
		for (n = 0; n < alive_backends_num; ++n)
		{
			if (r->backname == alive_backends[n])
			{
				for (m = n; m < alive_backends_num - 1; ++m)
				{
					alive_backends[m] = alive_backends[m + 1];
				}
				--alive_backends_num;
				break;
			}
		}
		while (alive_backends_num != 0)
		{
			backend_togo = ELF_Hash(r->uri_start, r->uri_len, alive_backends_num);
			r->backname = alive_backends[backend_togo];
			rc = ngx_event_connect_peer(&r->peer);
			if (rc == NGX_OK)
				break;
			for (n = backend_togo; n < alive_backends_num - 1; ++n)
			{
				alive_backends[n] = alive_backends[n + 1];
			}
			--alive_backends_num;
		}
	}
	free(url_buf);
	if(rc != NGX_OK)
	{
		ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0,
				"connect peer failed! fd = %d;then close connect,now!\n", c->fd);
		ngx_del_event(r->peer.connection->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
		ngx_close_connection(r->peer.connection);
		ngx_lscs_close_connection(r->connection);
		return;
	}
	c = r->peer.connection;
	c->data = r;
	//c->pool = r->connection->pool;
	c->log = r->connection->log;
	c->read->log = c->log;
	c->write->log = c->log;

	c->read->handler = ngx_lscs_empty_handler;
	ngx_handle_read_event(c->read, 0);
	c->write->handler = ngx_lscs_send_fd;
	if(ngx_handle_write_event(c->write, 0) != NGX_OK)
	{
		//printf("connection close here:%d\n", __LINE__);
		ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "handle write failed! fd = %d <= 0;then close connect,now!\n", c->fd);
		ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
		ngx_close_connection(c);
		ngx_lscs_close_connection(r->connection);
		return;
	}
	r->connection->read->handler = ngx_lscs_empty_handler;
	r->connection->write->handler = ngx_lscs_empty_handler;
	ngx_handle_read_event(r->connection->read, 0);
	ngx_handle_write_event(r->connection->write, 0);
}

static ngx_int_t ngx_lscs_get_peer_urlhash(ngx_peer_connection_t *pc, void *data)
{
	ngx_lscs_request_t *r = data;
	/*
	   ngx_lscs_core_srv_conf_t *cscf = ngx_lscs_get_module_srv_conf(r, ngx_lscs_core_module);
	   ngx_uint_t backend_togo = ngx_lscs_hash(r->uri_start, r->uri_len, cscf->backends->nelts);

	   ngx_str_t *backnames = cscf->backends->elts;
	   ngx_str_t backname = backnames[backend_togo];
	 */

	pc->connection = r->connection;
	pc->cached = 0;
	pc->socklen = offsetof(struct sockaddr_un,sun_path) + r->backname->len;
	pc->sockaddr = ngx_pcalloc(r->connection->pool, pc->socklen);
	struct sockaddr_un addr_UNIX;
	memset(&addr_UNIX,0,sizeof (addr_UNIX));
	addr_UNIX.sun_family = AF_LOCAL;
	//strcpy(addr_UNIX.sun_path, (char *)(backnames[backend_togo].data));
	strcpy(addr_UNIX.sun_path, (char *)(r->backname->data));
	memcpy(pc->sockaddr, &addr_UNIX, pc->socklen);
	//pc->name = &backnames[backend_togo];
	pc->name = r->backname;
	pc->tries = 0;

	//free(r->peer.url_buf);
	return NGX_OK;
}

static ngx_uint_t ngx_lscs_hash(u_char * uri, size_t uri_len, ngx_uint_t buckets)
{
	u_char* str = uri;
	ngx_uint_t seed = 131; // 31 131 1313 13131 131313 etc..
	ngx_uint_t hash = 0;
	static ngx_uint_t round_robin;

	if(hash_method == HASH_ROUND_ROBIN){ //round_robin methed
		if(round_robin >= buckets - 1){
			round_robin = 0;
		}
		else{
			round_robin++;
		}
	}
	else{       //not round_robin methed
		size_t offset;
		for (offset = 0; offset < uri_len; offset ++)
		{
			hash = hash * seed + (*str++);
		}       

		ngx_uint_t ret = ((hash << 1) >> 1) % buckets;

		return ret;
	}
	return round_robin;
}

static ngx_uint_t ELF_Hash(u_char *uri, size_t uri_len, ngx_uint_t buckets)
{
	size_t offset;
	u_char *key = NULL;
	unsigned long h = 0;

	for (offset = 0, key = uri; offset < uri_len; ++offset)
	{   
		h = (h<<4) + *key++;

		unsigned long g = h & 0Xf0000000L;

		if(g)
			h ^= g >> 24; 
		h &= ~g; 
	}   

	return h % buckets;
}

static void ngx_lscs_send_fd(ngx_event_t *wev)
{
	wev->handler = ngx_lscs_empty_handler;
	ngx_connection_t *cback = wev->data;
	ngx_lscs_request_t *r = cback->data;
	ngx_connection_t *c = r->connection;

	ngx_socket_t fd = c->fd;

	int retval; 
	struct msghdr msg;
	struct cmsghdr *p_cmsg;
	struct iovec vec;
	char cmsgbuf[CMSG_SPACE(sizeof(fd))];
	ngx_socket_t *p_fds; 

	unsigned short request_len = 0;

	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);   // == 16

	p_cmsg = CMSG_FIRSTHDR(&msg);
	p_cmsg->cmsg_level = SOL_SOCKET;
	p_cmsg->cmsg_type = SCM_RIGHTS;
	p_cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

	p_fds = (int *) CMSG_DATA(p_cmsg);
	*p_fds = fd;
	msg.msg_controllen = p_cmsg->cmsg_len;

	msg.msg_name = NULL; 
	msg.msg_namelen = 0;
	msg.msg_iov = &vec; 
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	/* "To pass file descriptors or credentials you need to send/read at
	 *          *          * least on byte" (man 7 unix)
	 *                   *                   */
	vec.iov_base = &request_len;
	vec.iov_len = sizeof(request_len);
	ngx_log_debug1(NGX_LOG_DEBUG_LSCS, c->log, 0, "sendmsg sent fd = %d\n", fd);
	retval = sendmsg(cback->fd, &msg, 0);
	if(retval <= 0)
	{
		ngx_lscs_close_connection(c);
		return;
	}
	//ngx_close_connection(cback);
	cback->read->handler = ngx_lscs_read_reply;
	cback->write->handler = ngx_lscs_empty_handler;
	ngx_del_event(cback->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
	if(ngx_handle_read_event(c->read, 0) != NGX_OK)
	{
		//printf("connection close here:%d\n", __LINE__);
		ngx_close_connection(cback);
	}
	/*
	   if(ngx_handle_write_event(c->write, 0) != NGX_OK)
	   {
	//printf("connection close here:%d\n", __LINE__);
	ngx_close_connection(cback);
	}
	 */
	//printf("connection close here:%d\n", __LINE__);
	ngx_lscs_close_connection(c);

}

static void ngx_lscs_read_reply(ngx_event_t *rev)
{
	ngx_connection_t *cback = rev->data;
	//ngx_lscs_request_t *r = cback->data;
	//ngx_connection_t *c = r->connection;

	ngx_close_connection(cback);
	//ngx_lscs_close_connection(c);
}

#define ngx_get_url_char_type(ch) ((ch) == ('\r') || (ch) == ('\n') || (ch) == ('\0'))? (term) : \
	(((ch) == ' ' || (ch) == '\t')? (space) : normal )
static void ngx_lscs_extract_uri(u_char * buf, size_t len, u_char **uri_start, ssize_t *uri_len, ngx_int_t *completed)
{
	enum{
		sw_start,
		sw_space_before_method,
		sw_method,
		sw_space_before_uri,
		sw_uri,
		sw_finish
	} state;

	u_char *c = buf;
	state = sw_start;
	(*uri_len) = -1;
	(*uri_start) = NULL;
	size_t offset = 0;
	(*completed) = 0;
	while(offset < len)
	{
		char_type ct = ngx_get_url_char_type(*c);
		//printf("%c %d, offset=%d\n", *c, ct, offset);
		switch (ct)
		{
			case normal:
				switch(state)
				{
					case sw_start:
						state = sw_method;
						break;
					case sw_space_before_method:
						state = sw_method;
						break;
					case sw_method:
						state = sw_method;
						break;
					case sw_space_before_uri:
						state = sw_uri;
						*uri_start = c;
						*uri_len = 1;
						break;
					case sw_uri:
						state = sw_uri;
						(*uri_len) ++;
						break;
					case sw_finish:
						return;
				}
				break;
			case space:
				switch(state)
				{
					case sw_start:
						state = sw_space_before_method;
						break;
					case sw_space_before_method:
						state = sw_space_before_method;
						break;
					case sw_method:
						state = sw_space_before_uri;
						break;
					case sw_space_before_uri:
						state = sw_space_before_uri;
						break;
					case sw_uri:
						state = sw_finish;
						(*completed) = 1;
						return;
						break;
					case sw_finish:
						return;
				}
				break;
			case term:
				(*completed) = 1;
				return;
		}
		c ++;
		offset ++;
	}
}

static int xisspace(char c)
{
	return (((c == ' ') || (c == '\t')) ? 1 : 0);
}

static int xisdigit(char c)
{
	return (((c >= '0') && (c <= '9')) ? 1 : 0) ;
}

int httpMsgParseRequestLine(struct HttpMsgBuf * hmsg) 
{
	int i = 0;
	int retcode;
	int maj = -1, min = -1;
	int last_whitespace = -1, line_end = -1;
	const char *t;

	/* Find \r\n - end of URL+Version (and the request) */
	t = memchr(hmsg->buf, '\n', hmsg->size);
	if (!t) 
	{       
		retcode = 0;
		goto finish; 
	}       
	/* XXX this should point to the -end- of the \r\n, \n, etc. */
	hmsg->req_end = t - hmsg->buf;
	i = 0;  

	/* Find first non-whitespace - beginning of method */
	for (; i < hmsg->req_end && (xisspace(hmsg->buf[i])); i++);
	if (i >= hmsg->req_end)
	{       
		retcode = 0;
		goto finish; 
	}       
	hmsg->m_start = i;
	hmsg->req_start = i;
	hmsg->r_len = hmsg->req_end - hmsg->req_start + 1;
	/* Find first whitespace - end of method */
	for (; i < hmsg->req_end && (!xisspace(hmsg->buf[i])); i++);
	if (i >= hmsg->req_end)
	{
		retcode = -1;
		goto finish;
	}
	hmsg->m_end = i - 1;
	hmsg->m_len = hmsg->m_end - hmsg->m_start + 1;

	/* Find first non-whitespace - beginning of URL+Version */
	for (; i < hmsg->req_end && (xisspace(hmsg->buf[i])); i++);
	if (i >= hmsg->req_end)
	{
		retcode = -1;
		goto finish;
	}
	hmsg->u_start = i;

	/* Find \r\n or \n - thats the end of the line. Keep track of the last whitespace! */
	for (; i <= hmsg->req_end; i++)
	{
		/* If \n - its end of line */
		if (hmsg->buf[i] == '\n')
		{
			line_end = i;
			break;
		}
		/* we know for sure that there is at least a \n following.. */
		if (hmsg->buf[i] == '\r' && hmsg->buf[i + 1] == '\n')
		{
			line_end = i;
			break;
		}
		/* If its a whitespace, note it as it'll delimit our version */
		if (hmsg->buf[i] == ' ' || hmsg->buf[i] == '\t')
		{
			last_whitespace = i;
		}
	}
	if (i > hmsg->req_end)
	{
		retcode = -1;
		goto finish;
	}
	/* At this point we don't need the 'i' value; so we'll recycle it for version parsing */

	/*
	 *          * At this point: line_end points to the first eol char (\r or \n);
	 *                   * last_whitespace points to the last whitespace char in the URL.
	 *                            * We know we have a full buffer here!
	 *                                     */
	if (last_whitespace == -1)
	{
		maj = 0;
		min = 9;
		hmsg->u_end = line_end - 1;
		assert(hmsg->u_end >= hmsg->u_start);
	}
	else
	{
		/* Find the first non-whitespace after last_whitespace */
		/* XXX why <= vs < ? I do need to really re-audit all of this .. */
		for (i = last_whitespace; i <= hmsg->req_end && xisspace(hmsg->buf[i]); i++);
		if (i > hmsg->req_end)
		{
			retcode = -1;
			goto finish;
		}
		/* is it http/ ? if so, we try parsing. If not, the URL is the whole line; version is 0.9 */
		if (i + 5 >= hmsg->req_end || (strncasecmp(&hmsg->buf[i], "HTTP/", 5) != 0))
		{
			maj = 0;
			min = 9;
			hmsg->u_end = line_end - 1;
			assert(hmsg->u_end >= hmsg->u_start);
		}
		else
		{
			/* Ok, lets try parsing! Yes, this needs refactoring! */
			hmsg->v_start = i;
			i += 5;

			/* next should be 1 or more digits */
			maj = 0;
			for (; i < hmsg->req_end && (xisdigit(hmsg->buf[i])) && maj < 65536; i++)
			{
				maj = maj * 10;
				maj = maj + (hmsg->buf[i]) - '0';
			}
			if (i >= hmsg->req_end || maj >= 65536)
			{
				retcode = -1;
				goto finish;
			}
			/* next should be .; we -have- to have this as we have a whole line.. */
			if (hmsg->buf[i] != '.')
			{
				retcode = 0;
				goto finish;
			}
			if (i + 1 >= hmsg->req_end)
			{
				retcode = -1;
				goto finish;
			}
			/* next should be one or more digits */
			i++;
			min = 0;
			for (; i < hmsg->req_end && (xisdigit(hmsg->buf[i])) && min < 65536; i++)
			{
				min = min * 10;
				min = min + (hmsg->buf[i]) - '0';
			}

			if (min >= 65536)
			{
				retcode = -1;
				goto finish;
			}

			/* Find whitespace, end of version */
			hmsg->v_end = i;
			hmsg->v_len = hmsg->v_end - hmsg->v_start + 1;
			hmsg->u_end = last_whitespace - 1;
		}
	}
	hmsg->u_len = hmsg->u_end - hmsg->u_start + 1;

	/*
	 *          * Rightio - we have all the schtuff. Return true; we've got enough.
	 *                   */
	retcode = 1;
	assert(maj != -1);
	assert(min != -1);
finish:
	hmsg->v_maj = maj;
	hmsg->v_min = min;
	return retcode;
}


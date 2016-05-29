#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <pthread.h>

#define true 	1
#define false   0 
typedef int bool;

typedef enum {
    HASH_URI = 0,
    HASH_URL,
    HASH_URH,
    HASH_HUR,
    HASH_FNAME,
    HASH_ROUND_ROBIN,
} lscs_method_t;

#define contain_of(type,member,ptr)    (type *)( (size_t)ptr -  ((size_t)&((type *)0)->member) )	

typedef struct {
    ngx_str_t     unix_sock_name;
    ngx_array_t  *keepalive_connection_pool;
}ngx_lscs_backend_node_t;


typedef struct {
    ngx_int_t 		lscs_enabled;
    ngx_flag_t     	hash_method;
    ngx_array_t    *lscs_backends;
    ngx_uint_t      round_robin;
} ngx_http_lscs_loc_conf_t;


typedef struct {
    ngx_lscs_backend_node_t  *backend_togo;
    ngx_int_t                 lscs_send_failed;
    unsigned short            request_len;
    struct msghdr            *msg;
} ngx_http_lscs_ctx_t;


static void * ngx_http_lscs_create_loc_conf(ngx_conf_t *cf);

static char * ngx_http_lscs_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);


static char * ngx_http_lscs_enable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static char * ngx_http_lscs_backend(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static char * ngx_http_lscs_hash_method (ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_lscs_handler(ngx_http_request_t *r);

static ngx_int_t  ngx_lscs_send_fd(ngx_event_t *ev);

static void ngx_lscs_event_send_fd(ngx_event_t *ev);

static void ngx_http_lscs_event_connect(ngx_event_t *ev);

static u_char *  ngx_cc_strrchr(u_char *str, ngx_int_t len, u_char ch);

static void
     ngx_http_lscs_push_back_connection(ngx_http_request_t *r, 
     				   ngx_lscs_backend_node_t *node, 
                                   ngx_connection_t *pc); 

/*static void ngx_http_lscs_read_timeout (ngx_event_t *ev);
 */


static ngx_uint_t 
ngx_lscs_hash(ngx_str_t * hash_string, ngx_uint_t buckets, ngx_int_t hash_method, ngx_uint_t round_robin);

static ngx_command_t  ngx_http_lscs_commands[] = {
    { ngx_string("lscs"),
      NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_http_lscs_enable,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("hash_meth"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LIF_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_lscs_hash_method,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL},

    { ngx_string("backends"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_lscs_backend,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL},
      ngx_null_command
};


static ngx_http_module_t  ngx_http_lscs_module_ctx = {
    NULL,                          /* preconfiguration */
    NULL,           			   /* postconfiguration */

    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */

    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */

    ngx_http_lscs_create_loc_conf, /* create location configuration */
    ngx_http_lscs_merge_loc_conf,  /* merge location configuration */
};


ngx_module_t  ngx_http_lscs_module = {
    NGX_MODULE_V1,
    &ngx_http_lscs_module_ctx, /* module context */
    ngx_http_lscs_commands,   /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

static char * 
ngx_http_lscs_enable(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_lscs_loc_conf_t *llcf = conf;
    ngx_str_t *value;
    
    value = cf->args->elts;
    if(cf->args->nelts != 2)  
        return "invalid args amount";

    if(ngx_strncmp(value[1].data,"on",2) == 0)
    {
        llcf->lscs_enabled = 1;
       
        ngx_http_core_loc_conf_t *clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
        clcf->handler = ngx_http_lscs_handler;
    }
    else
        llcf->lscs_enabled = 0;
    return NGX_CONF_OK;	
}

static char * 
ngx_http_lscs_backend (ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_lscs_loc_conf_t *llcf = conf;
    ngx_str_t *value;

	ngx_lscs_backend_node_t *backend;
    
    value = cf->args->elts;
    if(cf->args->nelts != 2)  
        return "invalid args count";
    if(llcf->lscs_enabled != 1)
        return "ngx_http_lscs_backend() must config command \"lscs on\" before this command";
    
    if(llcf->lscs_backends == NGX_CONF_UNSET_PTR) {
        if((llcf->lscs_backends = ngx_array_create(cf->pool, 64, sizeof(ngx_lscs_backend_node_t) )) == NULL)
        return "ngx_http_lscs_enable() create llcf->lscs_backends failed";
    }

    backend = ngx_array_push(llcf->lscs_backends);
    if(backend == NULL)
        return "ngx_http_lscs_backend() execute ngx_array_push(llcf->lscs_backends) failed";
    backend->unix_sock_name = value[1];
    backend->keepalive_connection_pool =  ngx_array_create(cf->pool,  16, sizeof(ngx_connection_t *));
    return NGX_CONF_OK;	
}

static char * 
ngx_http_lscs_hash_method (ngx_conf_t *cf, ngx_command_t *cmd,void *conf)
{
    ngx_http_lscs_loc_conf_t *llcf = conf;
    ngx_str_t *value = cf->args->elts;
    if(cf->args->nelts != 2)
        return "invalid args count";

    if (ngx_strncasecmp(value[1].data, (u_char *) "uri",sizeof("uri")) == 0) { 
        llcf->hash_method = HASH_URI;
    } else if (ngx_strncasecmp(value[1].data, (u_char *) "url",sizeof("url")) == 0) { 
	    llcf->hash_method = HASH_URL;
    } else if (ngx_strncasecmp(value[1].data, (u_char *) "urh",sizeof("ruh")) == 0) { 
	    llcf->hash_method = HASH_URH;
    } else if (ngx_strncasecmp(value[1].data, (u_char *) "hur",sizeof("hur")) == 0) { 
        llcf->hash_method = HASH_HUR;
    } else if (ngx_strncasecmp(value[1].data, (u_char *) "fname",sizeof("fname")) == 0) { 
        llcf->hash_method = HASH_FNAME;
    } else if (ngx_strncasecmp(value[1].data, (u_char *) "round-robin",sizeof("round_robin")) == 0) { 
        llcf->hash_method = HASH_ROUND_ROBIN;
    } else if (value[1].data == NULL) { 
        llcf->hash_method = HASH_URH;  //default;
    } else { 
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
					"invalid value \"%s\" in \"%s\" directive, "
					"it must be \"uri\" or \"url\" or \"urh\" or \"hur\" or \"fname\"",
					value[1].data, cmd->name.data);
        return NGX_CONF_ERROR;
    }   
   llcf->lscs_enabled = 1;
   if( llcf->lscs_backends == NGX_CONF_UNSET_PTR && (llcf->lscs_backends = ngx_array_create(cf->pool,64,sizeof(ngx_lscs_backend_node_t) ) ) == NULL)
        return "ngx_http_lscs_enable() create llcf->lscs_backends failed";
   ngx_http_core_loc_conf_t *clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
   clcf->handler = ngx_http_lscs_handler;
   return NGX_CONF_OK;
}


static ngx_uint_t 
ngx_lscs_hash(ngx_str_t * hash_string, ngx_uint_t buckets, ngx_int_t hash_method, ngx_uint_t round_robin)
{
    ngx_uint_t seed = 131; // 31 131 1313 13131 131313 etc..
    ngx_uint_t hash = 0;

    if(hash_string == NULL)
        return round_robin;
    u_char* str = hash_string->data;
    ngx_int_t uri_len = hash_string->len;

    if(hash_method == HASH_ROUND_ROBIN){ //round_robin methed
        return round_robin;
    }else{//not round_robin methed
        size_t offset;
        for (offset = 0; offset < (size_t )uri_len; offset ++)
        {
	        hash = hash * seed + (*str++);
        }       
        ngx_uint_t ret = ((hash << 1) >> 1) % buckets;

        return ret;
    }
}


static u_char *
ngx_cc_strrchr(u_char *str, ngx_int_t len, u_char ch)
{
	u_char *last = str + len;
	while(last >= str) {
		if (*last == ch)	
			return last;
		last--;
	}
	return NULL;
}

ngx_str_t * 
ngx_lscs_get_hash_string(ngx_http_request_t *r)
{
    ngx_http_lscs_loc_conf_t *llcf = ngx_http_get_module_loc_conf(r,ngx_http_lscs_module);

    if(llcf == NULL ||( llcf->hash_method > HASH_ROUND_ROBIN  &&  llcf->hash_method < HASH_URI ))
        return NULL;
	
    ngx_str_t *hash_string = ngx_pcalloc(r->pool,sizeof(ngx_str_t));
    ngx_int_t url_len = 0; 
    ngx_int_t fname_len = 0; 

    u_char *p = NULL;  
    u_char *q = NULL;
    ngx_int_t n = llcf->hash_method;
    
    if(r->unparsed_uri.data == NULL || r->uri.data == NULL)
        return NULL;
    if(r->headers_in.host == NULL) {
        r->headers_in.host = ngx_pcalloc(r->pool, sizeof(ngx_table_elt_t));
        r->headers_in.host->value.data = (u_char *)"no_host";
        r->headers_in.host->value.len = 7;
    }
    switch(n){
    case HASH_URI:  /*uri without the ?xxx=xx1*/
        *hash_string = r->uri;
        break;
    case HASH_URL:  /*host+uri*/
        url_len = (r->headers_in.host)->value.len +
                   r->uri.len + 2;
        hash_string->data = ngx_pcalloc(r->pool,url_len *sizeof(char));
        p = hash_string->data;

        p = ngx_cpymem(p,(r->headers_in.host)->value.data,(r->headers_in.host)->value.len);

        p = ngx_cpymem(p,r->uri.data,r->uri.len );
             
        hash_string->len = (p - hash_string->data);
        break;
    case HASH_URH:  /*unparsed_uri*/  
        *hash_string = r->unparsed_uri; 
        break;
    case HASH_HUR:  /*host + file_name, the file_name doesn't include path*/
        q =(u_char *)ngx_cc_strrchr(r->uri.data , r->uri.len, '/'); /*Get the last file_name part*/
        if (NULL == q)
            return NULL;
        fname_len = r->uri.len - (q - r->uri.data);
        url_len = (r->headers_in.host)->value.len + fname_len + 2;
        hash_string->data = ngx_pcalloc(r->pool,url_len *sizeof(char));
        p = hash_string->data;

        p = ngx_cpymem(p,(r->headers_in.host)->value.data,(r->headers_in.host)->value.len);

        p = ngx_cpymem(p, q, fname_len);
        hash_string->len = (p - hash_string->data);
        break;
    case HASH_FNAME: /*FNAME*/
        q = (u_char *)ngx_cc_strrchr(r->uri.data , r->uri.len , '/');
        if (NULL == q)
            return NULL;
        fname_len = r->uri.len - (q - r->uri.data);

        hash_string->data = ngx_pcalloc(r->pool,url_len *sizeof(char));
        p = hash_string->data;
        p = ngx_cpymem(p, q, fname_len);
        hash_string->len = fname_len;
        break;
    case HASH_ROUND_ROBIN:
        return NULL;
    default:
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "LSCS_error:lscs configure a error hash_method: %d",
                      llcf->hash_method);
        break;
	
    }
    return hash_string;
}


static ngx_int_t 
ngx_lscs_get_peer_urlhash(ngx_peer_connection_t *pc, void *data)
{
    ngx_http_request_t *r = data;
    ngx_uint_t backend_togo;
    ngx_http_lscs_loc_conf_t *llcf = ngx_http_get_module_loc_conf(r, ngx_http_lscs_module);

    if (llcf->hash_method == HASH_ROUND_ROBIN) {
        llcf->round_robin = (++llcf->round_robin) % (llcf->lscs_backends->nelts);
        backend_togo = llcf->round_robin ;
    } else {
        ngx_str_t *hash_string = NULL;
        hash_string = ngx_lscs_get_hash_string(r);
        if(hash_string == NULL)
            return NGX_ABORT;
        backend_togo = ngx_lscs_hash( hash_string, 
                                             llcf->lscs_backends->nelts,
                                             llcf->hash_method,
                                             llcf->round_robin);
    }

    ngx_lscs_backend_node_t *backnodes = llcf->lscs_backends->elts;
    ngx_lscs_backend_node_t  backnode  = backnodes[backend_togo];


    pc->connection = r->connection;
    pc->cached = 0;
    pc->socklen = offsetof(struct sockaddr_un,sun_path) + backnode.unix_sock_name.len;
    pc->sockaddr = ngx_pcalloc(r->connection->pool, pc->socklen);
   
    struct sockaddr_un addr_UNIX;
    memset(&addr_UNIX,0,sizeof (addr_UNIX));
    addr_UNIX.sun_family = AF_LOCAL;
    ngx_memcpy(addr_UNIX.sun_path, (char *)(backnode.unix_sock_name.data),backnode.unix_sock_name.len);
    ngx_memcpy(pc->sockaddr, &addr_UNIX, pc->socklen);
    

    pc->name =  &backnodes[backend_togo].unix_sock_name;

    ngx_http_lscs_ctx_t * ctx = ngx_http_get_module_ctx(r,ngx_http_lscs_module);
    ctx->backend_togo = &backnodes[backend_togo];


    char buf[8];
    ngx_int_t i = backnode.keepalive_connection_pool->nelts;
    ngx_connection_t **array = backnode.keepalive_connection_pool->elts;
             
    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, 
                   "LSCS_debug(%s), backend:%V, free_connection:%d", 
                   __FUNCTION__, 
                   &backnode.unix_sock_name,
                   i);

    while (--i >= 0) {
        ngx_connection_t *c = array[i];
        ssize_t n = recv(c->fd, (char *) buf, 1, MSG_PEEK);

        if (n == -1 && (ngx_socket_errno == NGX_EAGAIN || ngx_socket_errno == NGX_EINTR)) {
            pc->connection = c;
            array[i] = NULL; 
            backnode.keepalive_connection_pool->nelts--;
            ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "LSCS_debug(%s): Get a connection from pool", __FUNCTION__);
            return NGX_DONE;

        } else {
            array[i] = NULL; 
            backnode.keepalive_connection_pool->nelts--;
            ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "LSCS_debug(%s): close a connection from pool", __FUNCTION__);
            ngx_close_connection(c);
        }
    }
    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "LSCS_debug(%s): need create a new connection", __FUNCTION__);
    return NGX_OK;
}

/*
static void
ngx_http_lscs_read_timeout (ngx_event_t *ev)
{
    ngx_connection_t   *c = ev->data;
    ngx_http_request_t *r = c->data;
    ngx_http_lscs_ctx_t * ctx = ngx_http_get_module_ctx(r,ngx_http_lscs_module);
    
    if(ctx == NULL || 
       ctx->msg == NULL || 
       ctx->backend_togo == NULL ||
       r->upstream->peer.connection == NULL)
    {
        r->count = 1;
        ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "LSCS_error(%s):, module ctx not inited, close request \n", __FUNCTION__);
        ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    ngx_http_lscs_push_back_connection(r, ctx->backend_togo, r->upstream->peer.connection);
    ngx_http_close_request(r,  NGX_HTTP_REQUEST_TIME_OUT);
}
*/




static void
ngx_lscs_event_send_fd(ngx_event_t *ev)
{
	ssize_t           n;
	u_char            test_buf[100];
    ngx_connection_t *c = ev->data;
    ngx_http_request_t *r = c->data;
    ngx_http_lscs_ctx_t * ctx = ngx_http_get_module_ctx(r,ngx_http_lscs_module);
    ngx_event_t *rev = c->read;

    if(ctx == NULL || ctx->msg == NULL)
    {
        r->count = 1;
        ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "LSCS_error(%s):, module ctx not inited, close request \n", __FUNCTION__);
        ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    if (rev->timedout) {
        ngx_log_error(NGX_LOG_ERR, c->log, NGX_ETIMEDOUT, 
                      "LSCS_error(%s):ngx_lscs_event_send_fd() client timed out", __FUNCTION__);
        c->timedout = 1;
        r->count = 1;
		/*BUG:  Need push back to the connection pool!*/
        ngx_http_lscs_push_back_connection(r, ctx->backend_togo, r->upstream->peer.connection);
        ngx_http_close_request(r,  NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }
	ngx_memset(test_buf, 0, 100);
	n = c->recv(c, test_buf, 1);
	if (n == 0) {
        r->count = 1;
        ngx_log_error(NGX_LOG_ERR, c->log, NGX_ETIMEDOUT, 
                      "LSCS_error(%s):ngx_lscs_event_send_fd() client abort the request", __FUNCTION__);
        ngx_http_lscs_push_back_connection(r, ctx->backend_togo, r->upstream->peer.connection);
        ngx_http_close_request(r,  NGX_HTTP_REQUEST_TIME_OUT);
	}
    
    ngx_connection_t *cback = r->upstream->peer.connection;
    ngx_int_t  retval;
    retval = sendmsg(cback->fd, ctx->msg, 0);

    if(retval != sizeof(ctx->request_len) ) 
    {
        ctx->lscs_send_failed++;
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "LSCS_error(%s):sendmsg sent failed,connection: %p,retried: %d times\n",
                       __FUNCTION__, cback, ctx->lscs_send_failed);
        if (ngx_socket_errno != EINTR && ngx_socket_errno != EAGAIN )
        {

            ngx_log_error(NGX_LOG_ERR, r->connection->log, ngx_socket_errno, 
                      "LSCS_error(%s):sendmsg failed, meet big error\n",
                       __FUNCTION__);

            /*if (cback->fd != -1) {
                ngx_close_connection(cback);
		        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "LSCS_error(%s): new_bug, big error, connection: %p,retried: %d times\n",
                       __FUNCTION__, cback,ctx->lscs_send_failed);
            }*/
            ev->handler = ngx_http_lscs_event_connect;        
            ngx_http_lscs_push_back_connection(r, ctx->backend_togo, r->upstream->peer.connection);
        } else
            ev->handler = ngx_lscs_event_send_fd;
        
        ngx_add_timer(c->write, (ngx_msec_t)800);
        /*
        c->write->active = 0;
        c->write->ready  = 0;
        ngx_del_event(c->write, NGX_WRITE_EVENT, 0);
        if(ngx_handle_write_event(c->write, 0) != NGX_OK )
        {    
             ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                   "LSCS_error(%s):handle write failed! fd = %d <= 0;then close connect,now!\n", 
                     __FUNCTION__, c->fd);

             ngx_close_connection(cback); 
             ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
             r->count = 1;
             ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
             return;
        }    
        */
        return ;
    } 
    ngx_http_lscs_push_back_connection(r, ctx->backend_togo, r->upstream->peer.connection);

    r->count = 1;
    ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
    ngx_http_close_request(r, NGX_DONE);
    return;
}

static void
ngx_http_lscs_push_back_connection(ngx_http_request_t *r, 
				   ngx_lscs_backend_node_t *node, 
                                   ngx_connection_t *pc) {

    ngx_uint_t k = node->keepalive_connection_pool->nelts;
    if(node->keepalive_connection_pool->nalloc > k) {
        ngx_connection_t **array = node->keepalive_connection_pool->elts;
        array[k] = (void *)pc;
        node->keepalive_connection_pool->nelts++;
    	ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, 
                "LSCS_debug(%s)push the back fd is:%d to:%V, current count:%ui", 
                 __FUNCTION__, 
                 pc->fd,
                 &node->unix_sock_name,
                 node->keepalive_connection_pool->nelts);
    } else {

    	ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                "LSCS_debug(%s) need close this connection:%d,beacause over the nalloc:%ui, current count:%ui", 
                 __FUNCTION__, 
                 pc->fd,
                 &node->unix_sock_name,	
                 node->keepalive_connection_pool->nalloc,
                 node->keepalive_connection_pool->nelts);
        ngx_close_connection(pc);
    }
}





static ngx_int_t 
ngx_lscs_send_fd(ngx_event_t *ev)
{
    int retval; 
    struct msghdr *msg = NULL;
    ngx_connection_t *c = ev->data;
    ngx_http_request_t *r = c->data;
    ngx_connection_t *cback = r->upstream->peer.connection;

    ngx_http_lscs_ctx_t * ctx = ngx_http_get_module_ctx(r,ngx_http_lscs_module);
    if(ctx == NULL)
    {
        ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
        return NGX_ERROR;
    }
    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, 
                         "LSCS_error(%s):start to send_fd, the back fd is:%d", 
                         __FUNCTION__, cback->fd);

    ngx_socket_t fd = c->fd;

    if (ctx->msg == NULL) {
        msg = ngx_pcalloc(r->pool,sizeof(struct msghdr));
        struct cmsghdr *p_cmsg;
        struct iovec *vec = ngx_pcalloc(r->pool,sizeof(struct iovec));
        //    char cmsgbuf[CMSG_SPACE(sizeof(fd))];

        ssize_t len = CMSG_SPACE(sizeof(fd));
        char *cmsgbuf    = ngx_pcalloc(r->pool, len);
        ngx_socket_t *p_fds; 

        msg->msg_control = cmsgbuf;
        msg->msg_controllen = len;   // == 16

        p_cmsg = CMSG_FIRSTHDR(msg);
        p_cmsg->cmsg_level = SOL_SOCKET;
        p_cmsg->cmsg_type = SCM_RIGHTS;
        p_cmsg->cmsg_len = CMSG_LEN(sizeof(ngx_socket_t));

        p_fds = (int *) CMSG_DATA(p_cmsg);
        *p_fds = fd;
        msg->msg_controllen = p_cmsg->cmsg_len;

        msg->msg_name    = NULL; 
        msg->msg_namelen = 0;
        msg->msg_iov     = vec; 
        msg->msg_iovlen  = 1;
        msg->msg_flags   = 0;

        /* "To pass file descriptors or credentials you need to send/read at
         *  least on byte" (man 7 unix)
         * */
        vec->iov_base = &ctx->request_len;
        vec->iov_len = sizeof(ctx->request_len);
        ctx->msg = msg;
    } else
        msg = ctx->msg;

    retval = sendmsg(cback->fd, msg, 0);
    //ngx_connection_t *pc = cback;

    if(retval < 0 ) {
        if (ngx_socket_errno != EINTR && ngx_socket_errno != EAGAIN )
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, ngx_socket_errno, 
                         "LSCS_error(%s):sendmsg() Got fatal error, the fd is:%d", 
                         __FUNCTION__, cback->fd);
            ngx_close_connection(cback);
            c->write->handler = ngx_http_lscs_event_connect;        
            c->read->handler = ngx_http_lscs_event_connect;        
        } else {
            c->write->handler = ngx_lscs_event_send_fd;
            c->read->handler =  ngx_lscs_event_send_fd;        
		}
        ctx->lscs_send_failed++;
        ngx_add_timer(c->write, (ngx_msec_t)500);
        /*
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                         "LSCS_error(%s):sendmsg() failed, we need do again, the request fd:%d", 
                         __FUNCTION__, c->fd);

        c->write->active = 0;
        c->write->ready  = 0;
        ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);

        if (ngx_handle_write_event(c->write, 0) != NGX_OK) {
             ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                         "LSCS_error(%s):handle write failed! fd = %d;then close connect,now!\n", 
                         c->fd);
             ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
             return NGX_ERROR;
        }    
        */
        r->count = 2;
        return NGX_OK;
    } 
    
    ngx_http_lscs_push_back_connection(r, ctx->backend_togo, r->upstream->peer.connection);
    
    /*
    ngx_lscs_backend_node_t *node = ctx->backend_togo;
    ngx_uint_t k = node->keepalive_connection_pool->nelts;
    if(node->keepalive_connection_pool->nalloc > k)
    {
        ngx_connection_t **array = node->keepalive_connection_pool->elts;
        array[k] = (void *)pc;
        node->keepalive_connection_pool->nelts++;
    	ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, 
                "LSCS_debug(%s)push back a backend to:%V, current count:%d", 
                 __FUNCTION__, 
                 &node->unix_sock_name,
                 node->keepalive_connection_pool->nelts);
    } else {
    	ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, 
                "LSCS_debug(%s) need close this connection,beacause over the nalloc:%d, current count:%d", 
                 __FUNCTION__, 
                 &node->unix_sock_name,
		 node->keepalive_connection_pool->nalloc,
                 node->keepalive_connection_pool->nelts);
 
        ngx_close_connection(pc);
    }
    */
    return NGX_DONE;
}



void 
ngx_http_lscs_event_connect (ngx_event_t *ev)
{
    ngx_connection_t   *c = ev->data;
    ngx_http_request_t *r = c->data;
    ngx_event_t *rev = c->read;

    ngx_int_t rc;
    ngx_http_lscs_ctx_t * ctx = ngx_http_get_module_ctx(r,ngx_http_lscs_module);

    if (rev->timedout) {
        ngx_log_error(NGX_LOG_ERR, c->log, NGX_ETIMEDOUT, 
                "LSCS_error(%s): ngx_http_lscs_event_connect() client timed out,start close client",
                __FUNCTION__);
        c->timedout = 1;
        r->count = 1;
        c->write->active   = 0;
        ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
        ngx_http_lscs_push_back_connection(r, ctx->backend_togo, r->upstream->peer.connection);
        ngx_http_close_request(r,  NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }
    rc = ngx_event_connect_peer(&r->upstream->peer);

    if(rc != NGX_OK && rc != NGX_DONE)
    {    
        ctx->lscs_send_failed++;
        c->write->handler = ngx_http_lscs_event_connect;
        c->read->handler = ngx_http_lscs_event_connect;
        if (r->upstream->peer.connection)
            ngx_close_connection(r->upstream->peer.connection);

        ngx_add_timer(c->write, (ngx_msec_t)500);

        /*
        ngx_log_error(NGX_LOG_ERR, c->log, NGX_ETIMEDOUT, 
                "LSCS_error(%s): ngx_http_lscs_event_connect() client failed",
                 __FUNCTION__);
        c->write->ready   = 0;
        c->write->active   = 0;

        if (ngx_handle_write_event(c->write, 0) != NGX_OK )
        {    
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                    "LSCS_error(%s):handle write failed! fd = %d;then close connect,now!\n", 
                     __FUNCTION__, c->fd);
            ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
            r->count = 1;
            ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        }   
        */
        
        return;
    }  
    ngx_connection_t *pc = r->upstream->peer.connection;
    pc->data       = r;
    pc->log        = r->connection->log;
    pc->read->log  = r->connection->log;
    pc->write->log = r->connection->log;
    pc->write->data = pc;
    //   ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "connect peer successfully! the new fd is %d\n",pc->fd);

    if(pc->write->active || pc->write->ready)
        ngx_del_event(pc->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);

    if(pc->read->active || pc->read->ready)
        ngx_del_event(pc->read, NGX_READ_EVENT, NGX_CLOSE_EVENT);

    c->write->handler = ngx_lscs_event_send_fd;
    c->read->handler = ngx_lscs_event_send_fd;
    rc = ngx_lscs_send_fd(c->write);
    if (rc == NGX_ERROR){
        r->count = 1;
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                         "LSCS_error(%s):Get big error in ngx_lscs_send_fd(),close request \n", __FUNCTION__);
        ngx_http_close_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
    }
}


static ngx_int_t
ngx_http_lscs_handler(ngx_http_request_t *r)
{
    /* Start to do the lscs transmit operation */
    ngx_int_t rc;
    ngx_connection_t *c = r->connection;
    r->keepalive = 0;
    r->lingering_close = 0;
    ngx_http_lscs_loc_conf_t *llcf = ngx_http_get_module_loc_conf(r, ngx_http_lscs_module);

    c->data = r;
    c->write->data = c;

    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, 
                         "LSCS_error(%s):Got a new request", __FUNCTION__);
    if(llcf->lscs_enabled != 1 || llcf->lscs_backends == NULL)
    {
        ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "LSCS_error(%s):,close request \n", __FUNCTION__);

        return NGX_ERROR;
    }
        
    if (ngx_http_upstream_create(r) != NGX_OK) {
        ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                      "LSCS_error(%s):ngx_http_upstream_create() error,close request \n", __FUNCTION__);

        return NGX_ERROR;
    }
    ngx_add_timer(c->read, 900*1000); //Default timeout is 15min
    /*
    c->read->handler = ngx_http_lscs_read_timeout;

    if(c->write->active || c->write->ready)
        ngx_del_event(c->write, NGX_WRITE_EVENT, 0);

    if(c->read->active || c->read->ready)
        ngx_del_event(c->read, NGX_READ_EVENT, 0);
    */

    ngx_http_lscs_ctx_t       *ctx;
    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_lscs_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }
    ngx_http_set_ctx(r, ctx, ngx_http_lscs_module);


    ctx->lscs_send_failed = 0;
    r->upstream->peer.get  = ngx_lscs_get_peer_urlhash ;
    r->upstream->peer.data = r;
            
    rc = ngx_event_connect_peer(&r->upstream->peer);
    if( rc == NGX_ABORT )
    {
        return NGX_ABORT;
    } else if(rc != NGX_OK && rc != NGX_DONE) {    
        if(r->upstream->peer.connection)
            ngx_close_connection(r->upstream->peer.connection);
        c->write->handler = ngx_http_lscs_event_connect;
        c->read->handler = ngx_http_lscs_event_connect;
        ngx_add_timer(c->write, (ngx_msec_t)500);
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                         "LSCS_error(%s): ngx_event_connect_peer() failed, switch ngx_http_lscs_event_connect",
                         __FUNCTION__);
        /*

        if(ngx_handle_write_event(c->write, 0) != NGX_OK )
        {    
             ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                         "LSCS_error(%s):handle write failed! fd = %d ;then close connect,now!\n",
                          __FUNCTION__, c->fd);
             ngx_del_event(c->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);
             return NGX_ERROR;
        } 
        */

        r->count = 2;

        ctx->lscs_send_failed++;
        return NGX_OK;
     }        

     ngx_connection_t *pc = r->upstream->peer.connection;
     pc->data       = r;
     pc->log        = r->connection->log;
     pc->read->log  = r->connection->log;
     pc->write->log = r->connection->log;
     pc->write->data = pc;
//   ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "connect peer successfully! the new fd is %d\n",pc->fd);
     
     if(pc->write->active || pc->write->ready)
         ngx_del_event(pc->write, NGX_WRITE_EVENT, NGX_CLOSE_EVENT);

     if(pc->read->active || pc->read->ready)
         ngx_del_event(pc->read, NGX_READ_EVENT, NGX_CLOSE_EVENT);

    c->data = r;
    c->write->data = c;

     c->write->handler = ngx_lscs_event_send_fd;
     c->read->handler = ngx_lscs_event_send_fd;
     rc = ngx_lscs_send_fd(c->write);
     return rc;
}

static void *
ngx_http_lscs_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_lscs_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_lscs_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

    conf->lscs_enabled = 1;
    conf->lscs_backends = NGX_CONF_UNSET_PTR;
    conf->hash_method = NGX_CONF_UNSET;
    conf->round_robin = 0;
//    conf->enable = NGX_CONF_UNSET;
    return conf;
}

static char *
ngx_http_lscs_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_lscs_loc_conf_t *prev = parent; 
    ngx_http_lscs_loc_conf_t *conf = child;

    ngx_conf_merge_value(conf->lscs_enabled, prev->lscs_enabled, 1);
    
    ngx_conf_merge_value(conf->hash_method, prev->hash_method, HASH_URI);
    
    ngx_conf_merge_ptr_value(conf->lscs_backends, prev->lscs_backends, NULL);

    ngx_conf_merge_uint_value(conf->round_robin, prev->round_robin, 0);

    return NGX_CONF_OK;
}


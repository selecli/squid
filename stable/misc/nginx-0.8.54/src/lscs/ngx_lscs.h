#ifndef __NGX_LSCS_H_INCLUDED__
#define __NGX_LSCS_H_INCLUDED__
#include <ngx_core.h>
#include <ngx_config.h>
#include <ngx_event_connect.h>

#include <ngx_lscs_config.h>

#define NGX_LSCS_MODULE         0x5343534C     /* "LSCS" */

#define ngx_lscs_conf_get_module_main_conf(cf, module)                       \
	    ((ngx_lscs_conf_ctx_t *) cf->ctx)->main_conf[module.ctx_index]
#define ngx_lscs_conf_get_module_srv_conf(cf, module)                        \
	    ((ngx_lscs_conf_ctx_t *) cf->ctx)->srv_conf[module.ctx_index]
#define NGX_LSCS_MAX_URL	4096
#define NGX_LSCS_BUFFSIZE   4096

enum{
	HASH_URI = 0,
	HASH_URL,
	HASH_URH,
	HASH_HUR,
	HASH_FNAME,
	HASH_ROUND_ROBIN
} hash_method;

typedef struct {
	u_char                  sockaddr[NGX_SOCKADDRLEN];
	socklen_t               socklen;

	/* server ctx */
	ngx_lscs_conf_ctx_t    *ctx;   

	unsigned                bind:1; 
	unsigned                wildcard:1;
} ngx_lscs_listen_t;

typedef struct {
	union { 
		struct sockaddr        sockaddr;
		struct sockaddr_in     sockaddr_in;
#if (NGX_HAVE_INET6)
		struct sockaddr_in6    sockaddr_in6;
#endif
#if (NGX_HAVE_UNIX_DOMAIN)
		struct sockaddr_un     sockaddr_un;
#endif
		u_char                 sockaddr_data[NGX_SOCKADDRLEN];
	} u;    

	socklen_t                  socklen;

	unsigned                   set:1;  
	unsigned                   default_server:1;
	unsigned                   bind:1; 
	unsigned                   wildcard:1;
#if (NGX_LSCS_SSL)
	unsigned                   ssl:1;  
#endif
#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
	unsigned                   ipv6only:2;
#endif

	int                        backlog;
	int                        rcvbuf; 
	int                        sndbuf; 
#if (NGX_HAVE_SETFIB)
	int                        setfib; 
#endif

#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
	char                      *accept_filter;
#endif
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
	ngx_uint_t                 deferred_accept;
#endif

	u_char                     addr[NGX_SOCKADDR_STRLEN + 1];
} ngx_lscs_listen_opt_t;

typedef struct {
	/* ngx_lscs_in_addr_t or ngx_lscs_in6_addr_t */
	void                   *addrs; 
	ngx_uint_t              naddrs; 
} ngx_lscs_port_t;


typedef struct {            
	void                       *(*create_main_conf)(ngx_conf_t *cf);
	char                       *(*init_main_conf)(ngx_conf_t *cf, void *conf);

	void                       *(*create_srv_conf)(ngx_conf_t *cf);
	char                       *(*merge_srv_conf)(ngx_conf_t *cf, void *prev,
			void *conf);
} ngx_lscs_module_t;

typedef struct {
	ngx_log_t		*error_log;
	ngx_array_t		server_names;
	ngx_msec_t              timeout;
	ngx_msec_t              resolver_timeout;

	ngx_flag_t              so_keepalive;

	ngx_str_t               server_name;

	u_char                 *file_name;
	ngx_int_t               line;   

	ngx_resolver_t         *resolver;

	/* server ctx */
	ngx_lscs_conf_ctx_t    *ctx;   
	ngx_array_t	       *backends;
	ngx_str_t              *hash_val;

	ngx_uint_t		listen;
	size_t			connection_pool_size;
	ngx_msec_t		client_header_timeout;
} ngx_lscs_core_srv_conf_t;

typedef struct {
	ngx_array_t             servers;     /* ngx_lscs_core_srv_conf_t */
	ngx_array_t             listen;      /* ngx_lscs_listen_t */
	ngx_array_t		*ports;

	ngx_uint_t		server_names_hash_max_size;
	ngx_uint_t		server_names_hash_bucket_size;
} ngx_lscs_core_main_conf_t;

typedef struct {
	int                     family; 
	in_port_t               port;   
	ngx_array_t             addrs;       /* array of ngx_lscs_conf_addr_t */
} ngx_lscs_conf_port_t;

typedef struct ngx_lscs_server_name_s {
#if (NGX_PCRE)
		ngx_lscs_regex_t          *regex; 
#endif
		ngx_lscs_core_srv_conf_t  *server;   /* virtual name server conf */
		ngx_str_t                  name;   
} ngx_lscs_server_name_t;

typedef struct {
	ngx_hash_combined_t              names;  

	ngx_uint_t                       nregex; 
	ngx_lscs_server_name_t          *regex; 
} ngx_lscs_virtual_names_t;


typedef struct {
	ngx_lscs_conf_ctx_t    *ctx;   
	ngx_str_t               addr_text;
	ngx_lscs_core_srv_conf_t  *default_server;
	ngx_lscs_virtual_names_t *virtual_names;
} ngx_lscs_addr_conf_t;

typedef struct {
	in_addr_t               addr;   
	ngx_lscs_addr_conf_t    conf;   
} ngx_lscs_in_addr_t;

typedef struct {
	uint32_t		signature; // "LSCS"
	ngx_str_t		uri;
	ngx_int_t		completed_uri;
	ngx_connection_t	*connection;
	u_char * uri_start;
	ssize_t uri_len;
	ngx_peer_connection_t	peer;
	void                    **ctx;  
	void                    **main_conf;
	void                    **srv_conf;
	ngx_str_t *backname;
} ngx_lscs_request_t;

extern ngx_uint_t    ngx_lscs_max_module;
extern ngx_module_t  ngx_lscs_core_module;

void ngx_lscs_empty_handler(ngx_event_t *wev);
void ngx_lscs_init_connection(ngx_connection_t *c);

typedef struct {
	ngx_lscs_listen_opt_t	opt;
	ngx_hash_t		hash;
	struct sockaddr        *sockaddr;
	socklen_t               socklen;

	ngx_lscs_conf_ctx_t    *ctx;   

	ngx_lscs_core_srv_conf_t  *default_server;

	ngx_hash_wildcard_t       *wc_head;
	ngx_hash_wildcard_t       *wc_tail;
	ngx_array_t                servers;

	unsigned                bind:1; 
	unsigned                wildcard:1;
} ngx_lscs_conf_addr_t;

ngx_int_t ngx_lscs_add_listen(ngx_conf_t *cf, ngx_lscs_core_srv_conf_t *cscf, ngx_lscs_listen_opt_t *lsopt);

#endif

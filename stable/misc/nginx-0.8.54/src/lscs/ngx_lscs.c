/*
*  * Copyright (C) Xiaosi chinatopsquid
 *   */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_lscs.h>
#include <ngx_event.h>

static char *ngx_lscs_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
//static ngx_int_t ngx_lscs_add_ports(ngx_conf_t *cf, ngx_array_t *ports, ngx_lscs_listen_t *listen);
static ngx_int_t ngx_lscs_optimize_servers(ngx_conf_t *cf, ngx_lscs_core_main_conf_t *cmcf, ngx_array_t *ports);
static ngx_int_t ngx_lscs_add_addrs(ngx_conf_t *cf, ngx_lscs_port_t *mport, ngx_lscs_conf_addr_t *addr); 
static ngx_int_t ngx_lscs_cmp_conf_addrs(const void *one, const void *two);
static ngx_int_t ngx_lscs_add_addresses(ngx_conf_t *cf, ngx_lscs_core_srv_conf_t *cscf,	ngx_lscs_conf_port_t *port, ngx_lscs_listen_opt_t *lsopt);
static ngx_int_t ngx_lscs_add_address(ngx_conf_t *cf, ngx_lscs_core_srv_conf_t *cscf, ngx_lscs_conf_port_t *port, ngx_lscs_listen_opt_t *lsopt);
static ngx_int_t ngx_lscs_add_server(ngx_conf_t *cf, ngx_lscs_core_srv_conf_t *cscf, ngx_lscs_conf_addr_t *addr);
static ngx_int_t ngx_lscs_server_names(ngx_conf_t *cf, ngx_lscs_core_main_conf_t *cmcf, ngx_lscs_conf_addr_t *addr);
static ngx_int_t ngx_lscs_init_listening(ngx_conf_t *cf, ngx_lscs_conf_port_t *port);
static ngx_listening_t * ngx_lscs_add_listening(ngx_conf_t *cf, ngx_lscs_conf_addr_t *addr);

ngx_uint_t  ngx_lscs_max_module;

static ngx_command_t  ngx_lscs_commands[] = {

	{ ngx_string("lscs"),
		NGX_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
		ngx_lscs_block,
		0,      
		0,      
		NULL }, 

	ngx_null_command
};

static ngx_core_module_t  ngx_lscs_module_ctx = {
	ngx_string("lscs"),
	NULL,   
	NULL    
};

ngx_module_t  ngx_lscs_module = {
	NGX_MODULE_V1,
	&ngx_lscs_module_ctx,                  /* module context */
	ngx_lscs_commands,                     /* module directives */
	NGX_CORE_MODULE,                       /* module type */
	NULL,                                  /* init master */
	NULL,                                  /* init module */
	NULL,                                  /* init process */
	NULL,                                  /* init thread */
	NULL,                                  /* exit thread */
	NULL,                                  /* exit process */
	NULL,                                  /* exit master */
	NGX_MODULE_V1_PADDING
};



static int ngx_libc_cdecl ngx_lscs_cmp_dns_wildcards(const void *one, const void *two)
{
	ngx_hash_key_t  *first, *second;

	first = (ngx_hash_key_t *) one; 
	second = (ngx_hash_key_t *) two; 

	return ngx_dns_strcmp(first->key.data, second->key.data);
}

static char *ngx_lscs_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	char                        *rv;
	ngx_uint_t                   m, mi, s;
	ngx_conf_t                   pcf;
	ngx_lscs_module_t           *module;
	ngx_lscs_conf_ctx_t         *ctx;
	ngx_lscs_core_srv_conf_t   **cscfp;
	ngx_lscs_core_main_conf_t   *cmcf;

	/* the main lscs context */

	ctx = ngx_pcalloc(cf->pool, sizeof(ngx_lscs_conf_ctx_t));
	if (ctx == NULL) {
		return NGX_CONF_ERROR;
	}

	*(ngx_lscs_conf_ctx_t **) conf = ctx;

	/* count the number of the lscs modules and set up their indices */

	ngx_lscs_max_module = 0;
	for (m = 0; ngx_modules[m]; m++) {
		if (ngx_modules[m]->type != NGX_LSCS_MODULE) {
			continue;
		}

		ngx_modules[m]->ctx_index = ngx_lscs_max_module++;
	}


	/* the lscs main_conf context, it is the same in the all lscs contexts */

	ctx->main_conf = ngx_pcalloc(cf->pool,
			sizeof(void *) * ngx_lscs_max_module);
	if (ctx->main_conf == NULL) {
		return NGX_CONF_ERROR;
	}


	/*
	 * the lscs null srv_conf context, it is used to merge
	 * the server{}s' srv_conf's
	 */

	ctx->srv_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_lscs_max_module);
	if (ctx->srv_conf == NULL) {
		return NGX_CONF_ERROR;
	}

	/*
	 *create the main_conf's, the null srv_conf's, and the null loc_conf's
	 *of the all lscsmodules
	 */

	for (m = 0; ngx_modules[m]; m++) {
		if (ngx_modules[m]->type != NGX_LSCS_MODULE) {
			continue;
		}

		module = ngx_modules[m]->ctx;
		mi = ngx_modules[m]->ctx_index;

		if (module->create_main_conf) {
			ctx->main_conf[mi] = module->create_main_conf(cf);
			if (ctx->main_conf[mi] == NULL) {
				return NGX_CONF_ERROR;
			}
		}

		if (module->create_srv_conf) {
			ctx->srv_conf[mi] = module->create_srv_conf(cf);
			if (ctx->srv_conf[mi] == NULL) {
				return NGX_CONF_ERROR;
			}
		}
	}


	/* parse inside the lscs{} block */

	pcf = *cf;
	cf->ctx = ctx;

	cf->module_type = NGX_LSCS_MODULE;
	cf->cmd_type = NGX_LSCS_MAIN_CONF;
	rv = ngx_conf_parse(cf, NULL);

	if (rv != NGX_CONF_OK) {
		*cf = pcf;
		return rv;
	}


	/* init lscs{} main_conf's, merge the server{}s' srv_conf's */

	cmcf = ctx->main_conf[ngx_lscs_core_module.ctx_index];
	cscfp = cmcf->servers.elts;

	for (m = 0; ngx_modules[m]; m++) {
		if (ngx_modules[m]->type != NGX_LSCS_MODULE) {
			continue;
		}

		module = ngx_modules[m]->ctx;
		mi = ngx_modules[m]->ctx_index;

		/* init lscs{} main_conf's */

		//cf->ctx = ctx;

		if (module->init_main_conf) {
			rv = module->init_main_conf(cf, ctx->main_conf[mi]);
			if (rv != NGX_CONF_OK) {
				*cf = pcf;
				return rv;
			}
		}

		for (s = 0; s < cmcf->servers.nelts; s++) {

			/* merge the server{}s' srv_conf's */

			//cf->ctx = cscfp[s]->ctx;

			if (module->merge_srv_conf) {
				rv = module->merge_srv_conf(cf,
						ctx->srv_conf[mi],
						cscfp[s]->ctx->srv_conf[mi]);
				if (rv != NGX_CONF_OK) {
					*cf = pcf;
					return rv;
				}
			}
		}
	}

	*cf = pcf;
/*
	if (ngx_array_init(&ports, cf->temp_pool, 4, sizeof(ngx_lscs_conf_port_t))
			!= NGX_OK)
	{
		return NGX_CONF_ERROR;
	}

	listen = cmcf->listen.elts;

	for (i = 0; i < cmcf->listen.nelts; i++) {
		if (ngx_lscs_add_ports(cf, &ports, &listen[i]) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
	}
*/

	if( ngx_lscs_optimize_servers(cf, cmcf, cmcf->ports) != NGX_OK)
	{
		return NGX_CONF_ERROR;
	}

	return NGX_CONF_OK;
}

static ngx_int_t ngx_lscs_add_server(ngx_conf_t *cf, ngx_lscs_core_srv_conf_t *cscf, ngx_lscs_conf_addr_t *addr)
{
	ngx_uint_t                  i;      
	ngx_lscs_core_srv_conf_t  **server;

	if (addr->servers.elts == NULL) { 
		if (ngx_array_init(&addr->servers, cf->temp_pool, 4,
					sizeof(ngx_lscs_core_srv_conf_t *))
				!= NGX_OK) 
		{       
			return NGX_ERROR;
		}       

	} else {
		server = addr->servers.elts;
		for (i = 0; i < addr->servers.nelts; i++) {
			if (server[i] == cscf) { 
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
						"a duplicate listen %s", addr->opt.addr);
				return NGX_ERROR;
			}       
		}       
	}

	server = ngx_array_push(&addr->servers);
	if (server == NULL) { 
		return NGX_ERROR;
	}

	*server = cscf; 

	return NGX_OK; 
}

/*
static ngx_int_t
ngx_lscs_add_ports(ngx_conf_t *cf, ngx_array_t *ports,
		ngx_lscs_listen_t *listen)
{
	in_port_t              p;
	ngx_uint_t             i;
	struct sockaddr       *sa;
	struct sockaddr_in    *sin;
	ngx_lscs_conf_port_t  *port;
	ngx_lscs_conf_addr_t  *addr;

	sa = (struct sockaddr *) &listen->sockaddr;

	switch (sa->sa_family) {
		default: // AF_INET 
			sin = (struct sockaddr_in *) sa;
			p = sin->sin_port;
			break;
	}

	port = ports->elts;
	for (i = 0; i < ports->nelts; i++) {
		if (p == port[i].port && sa->sa_family == port[i].family) {

			// a port is already in the port list 

			port = &port[i];
			goto found;
		}
	}

	// add a port to the port list 

	port = ngx_array_push(ports);
	if (port == NULL) {
		return NGX_ERROR;
	}

	port->family = sa->sa_family;
	port->port = p;

	if (ngx_array_init(&port->addrs, cf->temp_pool, 2,
				sizeof(ngx_lscs_conf_addr_t))
			!= NGX_OK)
	{
		return NGX_ERROR;
	}

found:

	addr = ngx_array_push(&port->addrs);
	if (addr == NULL) {
		return NGX_ERROR;
	}

	addr->sockaddr = (struct sockaddr *) &listen->sockaddr;
	addr->socklen = listen->socklen;
	addr->ctx = listen->ctx;
	addr->bind = listen->bind;
	addr->wildcard = listen->wildcard;
	
	return NGX_OK;
}

*/

static ngx_int_t ngx_lscs_optimize_servers(ngx_conf_t *cf, ngx_lscs_core_main_conf_t *cmcf, ngx_array_t *ports)
{
	ngx_uint_t             p, a;
	ngx_lscs_conf_port_t  *port;
	ngx_lscs_conf_addr_t  *addr;

	if (ports == NULL) {
		return NGX_OK;
	}

	port = ports->elts;
	for (p = 0; p < ports->nelts; p++) {

		ngx_sort(port[p].addrs.elts, (size_t) port[p].addrs.nelts,
				sizeof(ngx_lscs_conf_addr_t), ngx_lscs_cmp_conf_addrs);

		/*
		 *          * check whether all name-based servers have the same
		 *                   * configuraiton as a default server for given address:port
		 *                            */

		addr = port[p].addrs.elts;
		for (a = 0; a < port[p].addrs.nelts; a++) {

			if (addr[a].servers.nelts > 1
#if (NGX_PCRE)
					|| addr[a].default_server->captures
#endif
			   )
			{
				if (ngx_lscs_server_names(cf, cmcf, &addr[a]) != NGX_OK) {
					return NGX_ERROR;
				}
			}
		}

		if (ngx_lscs_init_listening(cf, &port[p]) != NGX_OK) {
			return NGX_ERROR;
		}
	}

	return NGX_OK;
}

static ngx_int_t ngx_lscs_init_listening(ngx_conf_t *cf, ngx_lscs_conf_port_t *port)
{
	ngx_uint_t                 i, last, bind_wildcard;
	ngx_listening_t           *ls;
	ngx_lscs_port_t           *hport;
	ngx_lscs_conf_addr_t      *addr;

	addr = port->addrs.elts;
	last = port->addrs.nelts;

	/*
	 *      * If there is a binding to an "*:port" then we need to bind() to
	 *           * the "*:port" only and ignore other implicit bindings.  The bindings
	 *                * have been already sorted: explicit bindings are on the start, then
	 *                     * implicit bindings go, and wildcard binding is in the end.
	 *                          */

	if (addr[last - 1].opt.wildcard) {
		addr[last - 1].opt.bind = 1;
		bind_wildcard = 1;

	} else {
		bind_wildcard = 0;
	}

	i = 0;

	while (i < last) {

		if (bind_wildcard && !addr[i].opt.bind) {
			i++;
			continue;
		}

		ls = ngx_lscs_add_listening(cf, &addr[i]);
		if (ls == NULL) {
			return NGX_ERROR;
		}

		hport = ngx_pcalloc(cf->pool, sizeof(ngx_lscs_port_t));
		if (hport == NULL) {
			return NGX_ERROR;
		}

		ls->servers = hport;

		if (i == last - 1) {
			hport->naddrs = last;

		} else {
			hport->naddrs = 1;
			i = 0;
		}

		switch (ls->sockaddr->sa_family) {

#if (NGX_HAVE_INET6)
			case AF_INET6:
				if (ngx_lscs_add_addrs6(cf, hport, addr) != NGX_OK) {
					return NGX_ERROR;
				}
				break;
#endif
			default: /* AF_INET */
				if (ngx_lscs_add_addrs(cf, hport, addr) != NGX_OK) {
					return NGX_ERROR;
				}
				break;
		}

		addr++;
		last--;
	}

	return NGX_OK;
}

static ngx_listening_t * ngx_lscs_add_listening(ngx_conf_t *cf, ngx_lscs_conf_addr_t *addr)
{
	ngx_listening_t           *ls;
	ngx_lscs_core_srv_conf_t  *cscf;

	ls = ngx_create_listening(cf, &addr->opt.u.sockaddr, addr->opt.socklen);
	if (ls == NULL) {
		return NULL;
	}

	ls->addr_ntop = 1;

	ls->handler = ngx_lscs_init_connection;

	cscf = addr->default_server;
	ls->pool_size = cscf->connection_pool_size;
	ls->post_accept_timeout = cscf->client_header_timeout;

	ls->logp = cscf->error_log;
	ls->log.data = &ls->addr_text;
	ls->log.handler = ngx_accept_log_error;

#if (NGX_WIN32)
	{
		ngx_iocp_conf_t  *iocpcf;

		iocpcf = ngx_event_get_conf(cf->cycle->conf_ctx, ngx_iocp_module);
		if (iocpcf->acceptex_read) {
			ls->post_accept_buffer_size = cscf->client_header_buffer_size;
		}
	}
#endif

	ls->backlog = addr->opt.backlog;
	ls->rcvbuf = addr->opt.rcvbuf;
	ls->sndbuf = addr->opt.sndbuf;

#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
	ls->accept_filter = addr->opt.accept_filter;
#endif

#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
	ls->deferred_accept = addr->opt.deferred_accept;
#endif

#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
	ls->ipv6only = addr->opt.ipv6only;
#endif

#if (NGX_HAVE_SETFIB)
	ls->setfib = addr->opt.setfib;
#endif

	return ls;
}

static ngx_int_t ngx_lscs_server_names(ngx_conf_t *cf, ngx_lscs_core_main_conf_t *cmcf, ngx_lscs_conf_addr_t *addr)
{
	ngx_int_t                   rc;
	ngx_uint_t                  n, s;
	ngx_hash_init_t             hash;
	ngx_hash_keys_arrays_t      ha;
	ngx_lscs_server_name_t     *name;
	ngx_lscs_core_srv_conf_t  **cscfp;
#if (NGX_PCRE)
	ngx_uint_t                  regex, i;

	regex = 0;
#endif

	ngx_memzero(&ha, sizeof(ngx_hash_keys_arrays_t));

	ha.temp_pool = ngx_create_pool(16384, cf->log);
	if (ha.temp_pool == NULL) {
		return NGX_ERROR;
	}

	ha.pool = cf->pool;

	if (ngx_hash_keys_array_init(&ha, NGX_HASH_LARGE) != NGX_OK) {
		goto failed;
	}

	cscfp = addr->servers.elts;

	for (s = 0; s < addr->servers.nelts; s++) {

		name = cscfp[s]->server_names.elts;

		for (n = 0; n < cscfp[s]->server_names.nelts; n++) {

#if (NGX_PCRE)
			if (name[n].regex) {
				regex++;
				continue;
			}
#endif

			rc = ngx_hash_add_key(&ha, &name[n].name, name[n].server,
					NGX_HASH_WILDCARD_KEY);

			if (rc == NGX_ERROR) {
				return NGX_ERROR;
			}

			if (rc == NGX_DECLINED) {
				ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
						"invalid server name or wildcard \"%V\" on %s",
						&name[n].name, addr->opt.addr);
				return NGX_ERROR;
			}

			if (rc == NGX_BUSY) {
				ngx_log_error(NGX_LOG_WARN, cf->log, 0,
						"conflicting server name \"%V\" on %s, ignored",
						&name[n].name, addr->opt.addr);
			}
		}
	}

	hash.key = ngx_hash_key_lc;
	hash.max_size = cmcf->server_names_hash_max_size;
	hash.bucket_size = cmcf->server_names_hash_bucket_size;
	hash.name = "server_names_hash";
	hash.pool = cf->pool;

	if (ha.keys.nelts) {
		hash.hash = &addr->hash;
		hash.temp_pool = NULL;

		if (ngx_hash_init(&hash, ha.keys.elts, ha.keys.nelts) != NGX_OK) {
			goto failed;
		}
	}

	if (ha.dns_wc_head.nelts) {

		ngx_qsort(ha.dns_wc_head.elts, (size_t) ha.dns_wc_head.nelts,
				sizeof(ngx_hash_key_t), ngx_lscs_cmp_dns_wildcards);

		hash.hash = NULL;
		hash.temp_pool = ha.temp_pool;

		if (ngx_hash_wildcard_init(&hash, ha.dns_wc_head.elts,
					ha.dns_wc_head.nelts)
				!= NGX_OK)
		{
			goto failed;
		}

		addr->wc_head = (ngx_hash_wildcard_t *) hash.hash;
	}

	if (ha.dns_wc_tail.nelts) {

		ngx_qsort(ha.dns_wc_tail.elts, (size_t) ha.dns_wc_tail.nelts,
				sizeof(ngx_hash_key_t), ngx_lscs_cmp_dns_wildcards);

		hash.hash = NULL;
		hash.temp_pool = ha.temp_pool;

		if (ngx_hash_wildcard_init(&hash, ha.dns_wc_tail.elts,
					ha.dns_wc_tail.nelts)
				!= NGX_OK)
		{
			goto failed;
		}

		addr->wc_tail = (ngx_hash_wildcard_t *) hash.hash;
	}

	ngx_destroy_pool(ha.temp_pool);

#if (NGX_PCRE)

	if (regex == 0) {
		return NGX_OK;
	}

	addr->nregex = regex;
	addr->regex = ngx_palloc(cf->pool, regex * sizeof(ngx_lscs_server_name_t));
	if (addr->regex == NULL) {
		return NGX_ERROR;
	}

	i = 0;

	for (s = 0; s < addr->servers.nelts; s++) {

		name = cscfp[s]->server_names.elts;

		for (n = 0; n < cscfp[s]->server_names.nelts; n++) {
			if (name[n].regex) {
				addr->regex[i++] = name[n];
			}
		}
	}

#endif

	return NGX_OK;

failed:

	ngx_destroy_pool(ha.temp_pool);

	return NGX_ERROR;
}

static ngx_int_t ngx_lscs_add_addrs(ngx_conf_t *cf, ngx_lscs_port_t *hport,     ngx_lscs_conf_addr_t *addr)
{
	ngx_uint_t                 i;
	ngx_lscs_in_addr_t        *addrs;
	struct sockaddr_in        *sin;
	ngx_lscs_virtual_names_t  *vn;

	hport->addrs = ngx_pcalloc(cf->pool,
			hport->naddrs * sizeof(ngx_lscs_in_addr_t));
	if (hport->addrs == NULL) {
		return NGX_ERROR;
	}

	addrs = hport->addrs;

	for (i = 0; i < hport->naddrs; i++) {

		sin = &addr[i].opt.u.sockaddr_in;
		addrs[i].addr = sin->sin_addr.s_addr;
		addrs[i].conf.default_server = addr[i].default_server;
#if (NGX_LSCS_SSL)
		addrs[i].conf.ssl = addr[i].opt.ssl;
#endif

		if (addr[i].hash.buckets == NULL
				&& (addr[i].wc_head == NULL
					|| addr[i].wc_head->hash.buckets == NULL)
				&& (addr[i].wc_tail == NULL
					|| addr[i].wc_tail->hash.buckets == NULL)
#if (NGX_PCRE)
				&& addr[i].nregex == 0
#endif
		   )
		{
			continue;
		}

		vn = ngx_palloc(cf->pool, sizeof(ngx_lscs_virtual_names_t));
		if (vn == NULL) {
			return NGX_ERROR;
		}

		addrs[i].conf.virtual_names = vn;

		vn->names.hash = addr[i].hash;
		vn->names.wc_head = addr[i].wc_head;
		vn->names.wc_tail = addr[i].wc_tail;
#if (NGX_PCRE)
		vn->nregex = addr[i].nregex;
		vn->regex = addr[i].regex;
#endif
	}

	return NGX_OK;
}

static ngx_int_t ngx_lscs_cmp_conf_addrs(const void *one, const void *two)
{
	ngx_lscs_conf_addr_t  *first, *second;

	first = (ngx_lscs_conf_addr_t *) one;
	second = (ngx_lscs_conf_addr_t *) two;

	if (first->wildcard) {
		/* a wildcard must be the last resort, shift it to the end */
		return 1;
	}

	if (first->bind && !second->bind) {
		/* shift explicit bind()ed addresses to the start */
		return -1;
	}

	if (!first->bind && second->bind) {
		/* shift explicit bind()ed addresses to the start */
		return 1;
	}

	/* do not sort by default */

	return 0;
}

ngx_int_t ngx_lscs_add_listen(ngx_conf_t *cf, ngx_lscs_core_srv_conf_t *cscf, ngx_lscs_listen_opt_t *lsopt)
{
	in_port_t                   p;
	ngx_uint_t                  i;
	struct sockaddr            *sa;
	struct sockaddr_in         *sin;
	ngx_lscs_conf_port_t       *port;
	ngx_lscs_core_main_conf_t  *cmcf;
#if (NGX_HAVE_INET6)
	struct sockaddr_in6        *sin6;
#endif

	cmcf = ngx_lscs_conf_get_module_main_conf(cf, ngx_lscs_core_module);

	if (cmcf->ports == NULL) {
		cmcf->ports = ngx_array_create(cf->temp_pool, 2,
				sizeof(ngx_lscs_conf_port_t));
		if (cmcf->ports == NULL) {
			return NGX_ERROR;
		}
	}

	sa = &lsopt->u.sockaddr;

	switch (sa->sa_family) {

#if (NGX_HAVE_INET6)
		case AF_INET6:
			sin6 = &lsopt->u.sockaddr_in6;
			p = sin6->sin6_port;
			break;
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
		case AF_UNIX:
			p = 0;
			break;
#endif

		default: /* AF_INET */
			sin = &lsopt->u.sockaddr_in;
			p = sin->sin_port;
			break;
	}

	port = cmcf->ports->elts;
	for (i = 0; i < cmcf->ports->nelts; i++) {

		if (p != port[i].port || sa->sa_family != port[i].family) {
			continue;
		}

		/* a port is already in the port list */

		return ngx_lscs_add_addresses(cf, cscf, &port[i], lsopt);
	}

	/* add a port to the port list */

	port = ngx_array_push(cmcf->ports);
	if (port == NULL) {
		return NGX_ERROR;
	}

	port->family = sa->sa_family;
	port->port = p;
	port->addrs.elts = NULL;

	return ngx_lscs_add_address(cf, cscf, port, lsopt);
}

static ngx_int_t ngx_lscs_add_addresses(ngx_conf_t *cf, ngx_lscs_core_srv_conf_t *cscf,	ngx_lscs_conf_port_t *port, ngx_lscs_listen_opt_t *lsopt)
{
	u_char                *p;
	size_t                 len, off;
	ngx_uint_t             i, default_server;
	struct sockaddr       *sa;
	ngx_lscs_conf_addr_t  *addr;
#if (NGX_HAVE_UNIX_DOMAIN)
	struct sockaddr_un    *saun;
#endif
#if (NGX_lscs_SSL)
	ngx_uint_t             ssl;
#endif

	/*
	 *      * we can not compare whole sockaddr struct's as kernel
	 *           * may fill some fields in inherited sockaddr struct's
	 *                */

	sa = &lsopt->u.sockaddr;

	switch (sa->sa_family) {

#if (NGX_HAVE_INET6)
		case AF_INET6:
			off = offsetof(struct sockaddr_in6, sin6_addr);
			len = 16;
			break;
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
		case AF_UNIX:
			off = offsetof(struct sockaddr_un, sun_path);
			len = sizeof(saun->sun_path);
			break;
#endif

		default: /* AF_INET */
			off = offsetof(struct sockaddr_in, sin_addr);
			len = 4;
			break;
	}

	p = lsopt->u.sockaddr_data + off;

	addr = port->addrs.elts;

	for (i = 0; i < port->addrs.nelts; i++) {

		if (ngx_memcmp(p, addr[i].opt.u.sockaddr_data + off, len) != 0) {
			continue;
		}

		/* the address is already in the address list */

		if (ngx_lscs_add_server(cf, cscf, &addr[i]) != NGX_OK) {
			return NGX_ERROR;
		}

		/* preserve default_server bit during listen options overwriting */
		default_server = addr[i].opt.default_server;

#if (NGX_lscs_SSL)
		ssl = lsopt->ssl || addr[i].opt.ssl;
#endif

		if (lsopt->set) {

			if (addr[i].opt.set) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
						"a duplicate listen options for %s", addr[i].opt.addr);
				return NGX_ERROR;
			}

			addr[i].opt = *lsopt;
		}

		/* check the duplicate "default" server for this address:port */

		if (lsopt->default_server) {

			if (default_server) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
						"a duplicate default server for %s", addr[i].opt.addr);
				return NGX_ERROR;
			}

			default_server = 1;
			addr[i].default_server = cscf;
		}

		addr[i].opt.default_server = default_server;
#if (NGX_lscs_SSL)
		addr[i].opt.ssl = ssl;
#endif

		return NGX_OK;
	}

	/* add the address to the addresses list that bound to this port */

	return ngx_lscs_add_address(cf, cscf, port, lsopt);
}


/*
 *  * add the server address, the server names and the server core module
 *   * configurations to the port list
 *    */

static ngx_int_t ngx_lscs_add_address(ngx_conf_t *cf, ngx_lscs_core_srv_conf_t *cscf, ngx_lscs_conf_port_t *port, ngx_lscs_listen_opt_t *lsopt)
{
	ngx_lscs_conf_addr_t  *addr;

	if (port->addrs.elts == NULL) {
		if (ngx_array_init(&port->addrs, cf->temp_pool, 4,
					sizeof(ngx_lscs_conf_addr_t))
				!= NGX_OK)
		{
			return NGX_ERROR;
		}
	}

	addr = ngx_array_push(&port->addrs);
	if (addr == NULL) {
		return NGX_ERROR;
	}

	addr->opt = *lsopt;
	addr->hash.buckets = NULL;
	addr->hash.size = 0;
	addr->wc_head = NULL;
	addr->wc_tail = NULL;
#if (NGX_PCRE)
	addr->nregex = 0;
	addr->regex = NULL;
#endif
	addr->default_server = cscf;
	addr->servers.elts = NULL;

	return ngx_lscs_add_server(cf, cscf, addr);
}

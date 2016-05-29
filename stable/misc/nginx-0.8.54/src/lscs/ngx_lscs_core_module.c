#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_lscs.h>

static void * ngx_lscs_core_create_main_conf(ngx_conf_t *cf);
static void * ngx_lscs_core_create_srv_conf(ngx_conf_t *cf);
static char * ngx_lscs_core_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
static char * ngx_lscs_core_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char * ngx_lscs_core_listen(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char * ngx_lscs_core_hash_meth(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char * ngx_lscs_core_coredump_lmit_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_lscs_core_backends(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_lscs_core_error_log(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_lscs_module_t  ngx_lscs_core_module_ctx = {
	ngx_lscs_core_create_main_conf,        /* create main configuration */
	NULL,                                  /* init main configuration */
	ngx_lscs_core_create_srv_conf,         /* create server configuration */
	ngx_lscs_core_merge_srv_conf           /* merge server configuration */
};

static ngx_command_t  ngx_lscs_core_commands[] = {
	{ ngx_string("server"),
		NGX_LSCS_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
		ngx_lscs_core_server,
		0,      
		0,      
		NULL }, 
	{ ngx_string("listen"),
		NGX_LSCS_SRV_CONF|NGX_CONF_1MORE,
		ngx_lscs_core_listen,
		NGX_LSCS_SRV_CONF_OFFSET,
		0,      
		NULL },
	{ ngx_string("hash_meth"),
		NGX_LSCS_SRV_CONF|NGX_CONF_1MORE,
		ngx_lscs_core_hash_meth,
		NGX_LSCS_SRV_CONF_OFFSET,
		0,      
		NULL },
	{ ngx_string("coredump_dir"),
		NGX_LSCS_SRV_CONF|NGX_CONF_1MORE,
		ngx_lscs_core_coredump_lmit_set,
		NGX_LSCS_SRV_CONF_OFFSET,
		0,      
		NULL },
	{ ngx_string("backends"),
		NGX_LSCS_SRV_CONF|NGX_CONF_1MORE,
		ngx_lscs_core_backends,
		NGX_LSCS_SRV_CONF_OFFSET,
		0,
		NULL },
	{ ngx_string("error_log"),
		NGX_LSCS_MAIN_CONF|NGX_LSCS_SRV_CONF|NGX_CONF_1MORE,
		ngx_lscs_core_error_log,
		NGX_LSCS_SRV_CONF_OFFSET,
		0,      
		NULL }, 
	ngx_null_command
};

ngx_module_t  ngx_lscs_core_module = {
	NGX_MODULE_V1,
	&ngx_lscs_core_module_ctx,             /* module context */
	ngx_lscs_core_commands,                /* module directives */
	NGX_LSCS_MODULE,                       /* module type */
	NULL,                                  /* init master */
	NULL,                                  /* init module */
	NULL,                                  /* init process */
	NULL,                                  /* init thread */
	NULL,                                  /* exit thread */
	NULL,                                  /* exit process */
	NULL,                                  /* exit master */
	NGX_MODULE_V1_PADDING
};

static void *ngx_lscs_core_create_main_conf(ngx_conf_t *cf)
{
	ngx_lscs_core_main_conf_t  *cmcf;

	cmcf = ngx_pcalloc(cf->pool, sizeof(ngx_lscs_core_main_conf_t));
	if (cmcf == NULL) {
		return NULL;
	}

	if (ngx_array_init(&cmcf->servers, cf->pool, 4,
				sizeof(ngx_lscs_core_srv_conf_t *))
			!= NGX_OK)
	{
		return NULL;
	}

	if (ngx_array_init(&cmcf->listen, cf->pool, 4, sizeof(ngx_lscs_listen_t))
			!= NGX_OK)
	{
		return NULL;
	}

	return cmcf;
}


static void * ngx_lscs_core_create_srv_conf(ngx_conf_t *cf)
{
	ngx_lscs_core_srv_conf_t  *cscf;

	cscf = ngx_pcalloc(cf->pool, sizeof(ngx_lscs_core_srv_conf_t));
	if (cscf == NULL) {
		return NULL;
	}

	cscf->timeout = NGX_CONF_UNSET_MSEC;
	cscf->resolver_timeout = NGX_CONF_UNSET_MSEC;
	cscf->so_keepalive = NGX_CONF_UNSET;

	cscf->resolver = NGX_CONF_UNSET_PTR;

	cscf->file_name = cf->conf_file->file.name.data;
	cscf->line = cf->conf_file->line;

	cscf->connection_pool_size = NGX_CONF_UNSET_SIZE;
	cscf->client_header_timeout = NGX_CONF_UNSET_MSEC;

	return cscf;
}


static char *ngx_lscs_core_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_lscs_core_srv_conf_t *prev = parent;
	ngx_lscs_core_srv_conf_t *conf = child;

	ngx_conf_merge_size_value(conf->connection_pool_size,
			prev->connection_pool_size, 256);
	ngx_conf_merge_msec_value(conf->client_header_timeout,
			prev->client_header_timeout, 30000); 

	if (conf->error_log == NULL) {
		if (prev->error_log) {
			conf->error_log = prev->error_log;
		} else {
			conf->error_log = &cf->cycle->new_log;
		}
	}
	return NGX_CONF_OK;
}

static char * ngx_lscs_core_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	char                       *rv;
	void                       *mconf;
	ngx_uint_t                  m;
	ngx_conf_t                  pcf;
	ngx_lscs_module_t          *module;
	ngx_lscs_conf_ctx_t        *ctx, *lscs_ctx;
	ngx_lscs_core_srv_conf_t   *cscf, **cscfp;
	ngx_lscs_core_main_conf_t  *cmcf;

	ctx = ngx_pcalloc(cf->pool, sizeof(ngx_lscs_conf_ctx_t));
	if (ctx == NULL) {
		return NGX_CONF_ERROR;
	}

	lscs_ctx = cf->ctx;
	ctx->main_conf = lscs_ctx->main_conf;

	/* the server{}'s srv_conf */

	ctx->srv_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_lscs_max_module);
	if (ctx->srv_conf == NULL) {
		return NGX_CONF_ERROR;
	}

	for (m = 0; ngx_modules[m]; m++) {
		if (ngx_modules[m]->type != NGX_LSCS_MODULE) {
			continue;
		}

		module = ngx_modules[m]->ctx;

		if (module->create_srv_conf) {
			mconf = module->create_srv_conf(cf);
			if (mconf == NULL) {
				return NGX_CONF_ERROR;
			}

			ctx->srv_conf[ngx_modules[m]->ctx_index] = mconf;
		}
	}

	/* the server configuration context */

	cscf = ctx->srv_conf[ngx_lscs_core_module.ctx_index];
	cscf->ctx = ctx;

	cmcf = ctx->main_conf[ngx_lscs_core_module.ctx_index];

	cscfp = ngx_array_push(&cmcf->servers);
	if (cscfp == NULL) {
		return NGX_CONF_ERROR;
	}

	*cscfp = cscf;


	/* parse inside server{} */

	pcf = *cf;
	cf->ctx = ctx;
	cf->cmd_type = NGX_LSCS_SRV_CONF;

	rv = ngx_conf_parse(cf, NULL);

	*cf = pcf;

	return rv;
}


static char * ngx_lscs_core_listen(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_lscs_core_srv_conf_t *cscf = conf;

	ngx_str_t              *value, size;
	ngx_url_t               u;
	ngx_uint_t              n;
	ngx_lscs_listen_opt_t   lsopt;

	cscf->listen = 1;

	value = cf->args->elts;

	ngx_memzero(&u, sizeof(ngx_url_t));

	u.url = value[1];
	u.listen = 1;
	u.default_port = 80;

	if (ngx_parse_url(cf->pool, &u) != NGX_OK) {
		if (u.err) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
					"%s in \"%V\" of the \"listen\" directive",
					u.err, &u.url);
		}

		return NGX_CONF_ERROR;
	}

	ngx_memzero(&lsopt, sizeof(ngx_lscs_listen_opt_t));

	ngx_memcpy(&lsopt.u.sockaddr, u.sockaddr, u.socklen);

	lsopt.socklen = u.socklen;
	lsopt.backlog = NGX_LISTEN_BACKLOG;
	lsopt.rcvbuf = -1;
	lsopt.sndbuf = -1;
#if (NGX_HAVE_SETFIB)
	lsopt.setfib = -1;
#endif
	lsopt.wildcard = u.wildcard;

	(void) ngx_sock_ntop(&lsopt.u.sockaddr, lsopt.addr,
			NGX_SOCKADDR_STRLEN, 1);

	for (n = 2; n < cf->args->nelts; n++) {

		if (ngx_strcmp(value[n].data, "default_server") == 0
				|| ngx_strcmp(value[n].data, "default") == 0)
		{
			lsopt.default_server = 1;
			continue;
		}

		if (ngx_strcmp(value[n].data, "bind") == 0) {
			lsopt.set = 1;
			lsopt.bind = 1;
			continue;
		}

#if (NGX_HAVE_SETFIB)
		if (ngx_strncmp(value[n].data, "setfib=", 7) == 0) {
			lsopt.setfib = ngx_atoi(value[n].data + 7, value[n].len - 7);

			if (lsopt.setfib == NGX_ERROR) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
						"invalid setfib \"%V\"", &value[n]);
				return NGX_CONF_ERROR;
			}

			continue;
		}
#endif
		if (ngx_strncmp(value[n].data, "backlog=", 8) == 0) {
			lsopt.backlog = ngx_atoi(value[n].data + 8, value[n].len - 8);
			lsopt.set = 1;
			lsopt.bind = 1;

			if (lsopt.backlog == NGX_ERROR || lsopt.backlog == 0) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
						"invalid backlog \"%V\"", &value[n]);
				return NGX_CONF_ERROR;
			}

			continue;
		}

		if (ngx_strncmp(value[n].data, "rcvbuf=", 7) == 0) {
			size.len = value[n].len - 7;
			size.data = value[n].data + 7;

			lsopt.rcvbuf = ngx_parse_size(&size);
			lsopt.set = 1;
			lsopt.bind = 1;

			if (lsopt.rcvbuf == NGX_ERROR) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
						"invalid rcvbuf \"%V\"", &value[n]);
				return NGX_CONF_ERROR;
			}

			continue;
		}

		if (ngx_strncmp(value[n].data, "sndbuf=", 7) == 0) {
			size.len = value[n].len - 7;
			size.data = value[n].data + 7;

			lsopt.sndbuf = ngx_parse_size(&size);
			lsopt.set = 1;
			lsopt.bind = 1;

			if (lsopt.sndbuf == NGX_ERROR) {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
						"invalid sndbuf \"%V\"", &value[n]);
				return NGX_CONF_ERROR;
			}

			continue;
		}

		if (ngx_strncmp(value[n].data, "accept_filter=", 14) == 0) {
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
			lsopt.accept_filter = (char *) &value[n].data[14];
			lsopt.set = 1;
			lsopt.bind = 1;
#else
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
					"accept filters \"%V\" are not supported "
					"on this platform, ignored",
					&value[n]);
#endif
			continue;
		}

		if (ngx_strcmp(value[n].data, "deferred") == 0) {
#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
			lsopt.deferred_accept = 1;
			lsopt.set = 1;
			lsopt.bind = 1;
#else
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
					"the deferred accept is not supported "
					"on this platform, ignored");
#endif
			continue;
		}

		if (ngx_strncmp(value[n].data, "ipv6only=o", 10) == 0) {
#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
			struct sockaddr  *sa;

			sa = &lsopt.u.sockaddr;

			if (sa->sa_family == AF_INET6) {

				if (ngx_strcmp(&value[n].data[10], "n") == 0) {
					lsopt.ipv6only = 1;

				} else if (ngx_strcmp(&value[n].data[10], "ff") == 0) {
					lsopt.ipv6only = 2;

				} else {
					ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
							"invalid ipv6only flags \"%s\"",
							&value[n].data[9]);
					return NGX_CONF_ERROR;
				}

				lsopt.set = 1;
				lsopt.bind = 1;

			} else {
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
						"ipv6only is not supported "
						"on addr \"%s\", ignored", lsopt.addr);
			}

			continue;
#else
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
					"bind ipv6only is not supported "
					"on this platform");
			return NGX_CONF_ERROR;
#endif
		}

		if (ngx_strcmp(value[n].data, "ssl") == 0) {
#if (NGX_lscs_SSL)
			lsopt.ssl = 1;
			continue;
#else
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
					"the \"ssl\" parameter requires "
					"ngx_lscs_ssl_module");
			return NGX_CONF_ERROR;
#endif
		}

		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
				"the invalid \"%V\" parameter", &value[n]);
		return NGX_CONF_ERROR;
	}

	if (ngx_lscs_add_listen(cf, cscf, &lsopt) == NGX_OK) {
		return NGX_CONF_OK;
	}

	return NGX_CONF_ERROR;
}

static char *ngx_lscs_core_hash_meth(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_str_t        *value; 

	value = cf->args->elts;

	if (ngx_strcasecmp(value[1].data, (u_char *) "uri") == 0) { 
		hash_method = HASH_URI;

	} else if (ngx_strcasecmp(value[1].data, (u_char *) "url") == 0) { 
		hash_method = HASH_URL;
	} else if (ngx_strcasecmp(value[1].data, (u_char *) "urh") == 0) { 
		hash_method = HASH_URH;
	} else if (ngx_strcasecmp(value[1].data, (u_char *) "hur") == 0) { 
		hash_method = HASH_HUR;
	} else if (ngx_strcasecmp(value[1].data, (u_char *) "fname") == 0) { 
		hash_method = HASH_FNAME;
	} else if (ngx_strcasecmp(value[1].data, (u_char *) "round-robin") == 0) { 
		hash_method = HASH_ROUND_ROBIN;
	} else if (value[1].data == NULL) { 
		hash_method = HASH_URH;  //default;

	} else { //error;
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
				"invalid value \"%s\" in \"%s\" directive, "
				"it must be \"uri\" or \"url\" or \"urh\" or \"hur\" or \"fname\"",
				value[1].data, cmd->name.data);
		return NGX_CONF_ERROR;
	}

	return NGX_CONF_OK;
}

static char * ngx_lscs_core_coredump_lmit_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{	
	ngx_str_t        *value; 
	value = cf->args->elts;
	char pathbuf[256];

	struct rlimit rlim;

	if (getrlimit(RLIMIT_CORE, &rlim) == 0)
	{       
		rlim.rlim_cur = rlim.rlim_max;
		setrlimit(RLIMIT_CORE, &rlim); 
	} 

	if(ngx_strcasecmp(value[1].data, (u_char *) "none") == 0){
		return NGX_CONF_OK;
	}
	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "value[0].data = %s value[1].data = %s\n", value[0].data,value[1].data);

	if (value[1].data)
	{       
		if (ngx_strcasecmp(value[1].data, (u_char *) "none") == 0)
		{       
			return NGX_CONF_OK;
		}       
		else if (chdir((const char *)(value[1].data)) == 0)
		{      
			ngx_log_debug1(NGX_LOG_DEBUG_CORE, cf->log, 0, "Set Current Directory to %s", value[1].data); 
			return NGX_CONF_OK; 
		}       
		else    
		{       
			ngx_log_debug1(NGX_LOG_DEBUG_CORE, cf->log, 0, "chdir: %s:Failed!", value[1].data); 
			return NGX_CONF_OK;
		}       
	}       
	/* If we don't have coredump_dir or couldn't cd there, report current dir */
	if (getcwd(pathbuf, 256))
	{       
		ngx_log_debug1(NGX_LOG_DEBUG_CORE, cf->log, 0, "Current Directory is %s\n", value[1].data); 
	}       
	else    
	{       
		ngx_log_debug1(NGX_LOG_DEBUG_CORE, cf->log, 0, "WARNING: Can't find current directory, getcwd:%s Failed!\n",value[1].data); 
	}
	return NGX_CONF_OK;
}

static char *ngx_lscs_core_backends(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_lscs_core_srv_conf_t *cscf = conf;

	if (cscf->backends == NULL) { 
		cscf->backends = ngx_array_create(cf->pool, 4,
				sizeof(ngx_str_t));
		if (cscf->backends == NULL) { 
			return NGX_CONF_ERROR;
		}       
	}

	ngx_str_t *value;
	value = cf->args->elts;

	ngx_str_t *backend = ngx_array_push(cscf->backends);

	backend->data = ngx_pstrdup(cf->pool, &value[1]);
	backend->len = value[1].len;

	return NGX_CONF_OK;
}

static char *ngx_lscs_core_error_log(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{     
	ngx_lscs_core_srv_conf_t *clcf = conf;

	ngx_str_t  *value;

	if (clcf->error_log) {
		return "is duplicate";
	} 

	value = cf->args->elts;

	clcf->error_log = ngx_log_create(cf->cycle, &value[1]);
	if (clcf->error_log == NULL) {
		return NGX_CONF_ERROR;
	} 

	if (cf->args->nelts == 2) {
		clcf->error_log->log_level = NGX_LOG_ERR;
		return NGX_CONF_OK;
	}

	return ngx_log_set_levels(cf, clcf->error_log);
} 

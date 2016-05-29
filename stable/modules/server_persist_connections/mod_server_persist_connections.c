#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK


static cc_module* mod = NULL;


struct _pconn
{
	hash_link hash;		/* must be first */
	int *fds;
	int nfds_alloc;
	int nfds;
};
typedef struct
{
	char *host;
	u_short port;
	struct sockaddr_in S;
	CNCB *callback;
	void *data;
	struct in_addr in_addr;
	int fd;
	int tries;
	int addrcount;
	int connstart;
} ConnectStateData;


typedef struct
{
	char * ip_addr;
	int port;
	char proto;
    int pconn_timeout;
} server;
CBDATA_TYPE(server);

typedef struct{
	int idle; // the idle num always be
	int max_conn; // max conn with server
	char* server_ips;
	int time_out;
}persist_config;


static int free_persist_config(void* data)
{
	persist_config *cfg = data;
	if(cfg)
		safe_free(cfg->server_ips);
	safe_free(cfg);
	return 0;
}
void free_server(void * s)
{
	server *ss = s;
	if(ss)
	{
		safe_free(ss->ip_addr);
	}
}
/**
  * return 0 if parse ok, -1 if error
  */
static int func_sys_parse_param(char *cfg_line)
{
	//Must be [mod_cc_fast on] or [mod_cc_fast off]
	assert(cfg_line);
	char *t = strtok(cfg_line, w_space);
	if(strcmp(t, "mod_server_persist_connections"))
	{
		debug(151,0)("mod_server_persist_connections cfg_line not begin with mod_server_persist_connections!\n");
		return -1;
	}
	
	persist_config *cfg;
	cfg = xcalloc(1,sizeof(persist_config));
	//memset(cfg,0,sizeof(struct ccv_config));
	t = strtok(NULL, w_space); /**idle num**/
	if(t == NULL)
	{
		return -1;
	}
	cfg->idle = atoi(t);	
	t = strtok(NULL,w_space);   /**max connection num**/
	if(t == NULL)
	{
		return -1;
	}
	cfg->max_conn = atoi(t);
	if(cfg->idle > cfg->max_conn)
	{
		cfg->max_conn = cfg->idle;
	}
	t =strtok(NULL,w_space);   /**upper layer ip_list!!**/
	if(t == NULL)
	{
		debug(151,0)("mod_server_persist_connections: no server_ip list \n");
		return -1;
	}
	cfg->server_ips = xstrdup(t);

	t =strtok(NULL,w_space);   /*tcp_keepalive time set for persistent_connection**/
	if(t == NULL)
	{
		debug(151,0)("mod_server_persist_connections: no tcp_keepalive setting!!\n");
		return -1;
	}
	cfg->time_out=atoi(t);

	cc_register_mod_param(mod, cfg, free_persist_config);
	return 0;
}

static void local_handler(const ipcache_addrs* a,void *data)
{
	return;
}

static int set_name_as_ip(FwdState *fwdState, char** name)
{
	const ipcache_addrs *addrs = NULL;
	ipcache_entry *i = NULL;
	if ((addrs = ipcacheCheckNumeric(*name)))
	{
		return 1;
	}
	i = ipcache_get(*name);
	if (NULL == i)
	{
		/* miss */
		return 0;
	}
	else if(i->flags.negcached == 1)
	{
		return 0;
	}
	else if (ipcacheExpiredEntry(i))
	{
		/* hit, but expired -- bummer */
		char *host=xstrdup(*name);
		*name = inet_ntoa(*(i->addrs.in_addrs));
		ipcache_nbgethostbyname(host, local_handler, NULL);
		//ipcacheRelease(i);
//		i = NULL;
		safe_free(host);
		return 0 ;
	}
	else
	{
		 *name = inet_ntoa(*(i->addrs.in_addrs));
	}
	debug(151,3)("mod_server_persist_connections-->set_name_as_ip: set name as %s\n ",*name);
	return 1 ;
}


/*static void set_upperlayer_persist_connection_tcp_keepalive(FwdState *fwdState)
{

	persist_config *cfg = cc_get_mod_param(fwdState->client_fd,mod);
	fde *F=&fd_table[fwdState->server_fd];
	if(ip && strstr(cfg->server_ips,F->ipaddr))
		commSetTcpKeepalive(fd, cfg->tcp_keepalive_idle, cfg->tcp_keepalive_interval, cfg->tcp_keepalive_timeout);

}
*/


static void set_upperlayer_persist_connection_timeout(FwdState *fwdState,int server_fd)
{

	persist_config *cfg = cc_get_mod_param(fwdState->client_fd,mod);
	fde *F=&fd_table[fwdState->server_fd];
	if(strstr(cfg->server_ips,F->ipaddr) && server_fd >0 )	
	{
		commSetTimeout(server_fd,cfg->time_out,NULL,NULL);
		debug(153, 4) ("set_upperlayer_persist_connection_timeout:update the upperlayer_timeout to %d\n",cfg->time_out);
	}
}


static void
private_fwdNegotiateSSL(int fd,void *data)
{
	SSL *ssl = fd_table[fd].ssl;
	int ret;

	server *s=(server *)data;

	//unsigned char key[22];
	errno = 0;
	ERR_clear_error();
	if ((ret = SSL_connect(ssl)) <= 0)
	{
		int ssl_error = SSL_get_error(ssl, ret);
		switch (ssl_error)
		{
			case SSL_ERROR_WANT_READ:
				commSetSelect(fd, COMM_SELECT_READ, private_fwdNegotiateSSL, s, 0);
				return;
			case SSL_ERROR_WANT_WRITE:
				commSetSelect(fd, COMM_SELECT_WRITE, private_fwdNegotiateSSL,s, 0);
				return;
			default:
				debug(153, 1) ("private_fwdNegotiateSSL: Error negotiating SSL connection on FD %d: %s (%d/%d/%d)\n", fd, ERR_error_string(ERR_get_error(), NULL), ssl_error, ret, errno);
			cbdataFree(s);
			comm_close(fd);
			return ;
		}
	}
	
	pconnPush(fd,s->ip_addr,s->port,NULL,NULL,0);
	commSetTimeout(fd,s->pconn_timeout,NULL,NULL);
	debug(153, 4) ("private_fwdNegotiateSSL: succeed negotiating SSL connection on FD:%d \n",fd);
	return;
}


static void
private_fwdInitiateSSL(int fd,server *s)
{
	SSL *ssl;
	SSL_CTX *sslContext = NULL;

	sslContext = Config.ssl_client.sslContext;
	assert(sslContext);
	if ((ssl = SSL_new(sslContext)) == NULL)
	{
		debug(153, 1) ("private_fwdInitiateSSL: Error allocating handle: %s\n",ERR_error_string(ERR_get_error(), NULL));
		cbdataFree(s);
		comm_close(fd);
		return ;
	}
	SSL_set_fd(ssl, fd);
	fd_table[fd].ssl = ssl;
	fd_table[fd].read_method = &ssl_read_method;
	fd_table[fd].write_method = &ssl_write_method;
	fd_note(fd, "private_Negotiating SSL");
    private_fwdNegotiateSSL(fd,s);
	return;
}

static void private_openIdleConnDone(int fd,int status, void* data)
{
	server* s = data;
	debug(151,3)("mod_server_persist_connections-->private_openIdleConnDone: ip_addr == %s \t port == %d, fd == %d\n",s->ip_addr, s->port,fd);
	if(fd>0)
	{
		if(s->proto == PROTO_HTTPS)
		{
			private_fwdInitiateSSL(fd,s); 
		}else{
			pconnPush(fd,s->ip_addr,s->port,NULL,NULL,0);
	        commSetTimeout(fd,s->pconn_timeout,NULL,NULL);
			cbdataFree(s); //Does fwdConnectIdleTimeOut will free!!!
		}
	}
}

static int set_host_as_ip(int fd,char** host)
{
	*host = fd_table[fd].ipaddr;
	debug(151,3)("mod_server_persist_connections-->set_host_as_ip: set host as %s\n",*host);
	return 0;
}
static void
fwdConnectIdleTimeout(int fd, void *data)
{
	server* s = data;
//	debug(151, 2) ("mod_server_connections --> fwdConnectIdleTimeout: FD %d \n", fd);
	comm_close(fd);
	cbdataFree(s);
}

static void private_openAnIdleConn(char* server_ip,int server_port , struct in_addr outgoing , unsigned short tos, int ctimeout,FwdState *fwdState)
{

	int fd = comm_openex(SOCK_STREAM,IPPROTO_TCP,outgoing,0,COMM_NONBLOCKING,tos,server_ip);
	if (fd < 0)
	{
		debug(151, 4) ("private_openAnIdleConn: %s\n", xstrerror());
		return;
	}
//	debug(151,3)("mod_server_persist_connections-->private_openAnIdleConn: openIdleConn fd == %d,timeout:%d\n",fd,ctimeout);
	server *s = cbdataAlloc(server);
    s->ip_addr = xstrdup(server_ip);
	s->port = server_port;
	s->proto = fwdState->request->protocol; 
    s->pconn_timeout=ctimeout;

	commSetTimeout(fd,ctimeout,fwdConnectIdleTimeout,s);
/*	if(ctimeout>36000)
		commSetTcpKeepalive(fd, 3600, ctimeout/36000, ctimeout);
	else
		commSetTcpKeepalive(fd, 3600, 100, 360000);
*/
	commSetTcpKeepalive(fd, 1000, 10000, 360000);
	commConnectStart(fd, server_ip, server_port, private_openIdleConnDone, s);
}


/*
static int get_perisit_connection_again(ipcache_addrs* ia, void* data)
{
	if (Config.onoff.server_pconns)
	{
		struct in_addr outgoing;
		unsigned short tos;
		ConnectStateData* cs = data;
		FwdState * fwdState = cs->data;
		outgoing = getOutgoingAddr(fwdState->request);
		tos = getOutgoingTOS(fwdState->request);
		char* ip_addr = inet_ntoa(ia->in_addrs[ia->cur]);
		int idle;
		int fd = pconnPop(ip_addr, cs->port, NULL, NULL, 0, &idle);
		if(fd == -1)
		{
			debug(151,3)("mod_server_persist_connections-->get_persist_connection_again :  pconnPop fd == -1\n");

		}
		else
		if(fd == cs->fd)
		{
			debug(151,1)("mod_server_persist_connections-->get_persist_connection_again : cs->fd == pconnPop fd, it's an error\n");
		}
		else
		{
			close_handler* p;
			for(p = fd_table[cs->fd].close_handler; p !=NULL;p=p->next)
			{
				comm_add_close_handler(fd,p->handler,p->data);
			}
			if(fd_table[cs->fd].timeout_handler)
			{
				commSetTimeout(fd,
						fd_table[cs->fd].timeout,
						fd_table[cs->fd].timeout_handler,
						fd_table[cs->fd].timeout_data);
			}
			for(p = fd_table[cs->fd].close_handler;p !=NULL;p = p->next)
			{
				   comm_remove_close_handler(cs->fd,p->handler,p->data);
			}
//			fd_table[cs->fd].close_handler = NULL;
			if(fd_table[cs->fd].timeout_handler)
			{
				commSetTimeout(fd,
						fd_table[cs->fd].timeout,
						NULL,
						NULL);
			}
//			fd_table[cs->fd].close_handler = NULL;
			comm_close(cs->fd);
			debug(151,4)("mod_server_persist_connections-->get_persist_connection_again : close old fd %d created, and use persist connections fd %d\n",cs->fd,fd);
			fwdState->server_fd = fd;
			cs->fd = fd;
		}

	}
	return 0;

}
*/

static int private_openIdleConn( int idle,FwdState *fwdState,char* name,char* domain,  int ctimeout)
{
	if(fwdState->request->protocol == PROTO_HTTP  || fwdState->request->protocol == PROTO_HTTPS )
	{

		struct in_addr outgoing;
		unsigned short tos;

		debug(151,3)("mod_server_persist_connections-->private_openIdleConn : Opening idle connections for '%s' \n",name);

		outgoing = getOutgoingAddr(fwdState->request);
		tos = getOutgoingTOS(fwdState->request);
		int fd = fwdState->client_fd;
		persist_config *cfg = cc_get_mod_param(fd,mod);
		unsigned short port;
		port = fwdState->request->port;
		debug(151,3)("mod_server_persist_connections-->private_openIdleConn : cfg->server_ips == %s cfg->idle == %d idle = %d name == %s  \n",cfg->server_ips,cfg->idle,idle,name);
		while (idle < cfg->idle && strstr(cfg->server_ips,name))
		{

			private_openAnIdleConn(name, port , outgoing, tos, cfg->time_out,fwdState);
			idle++;
		}
	}
	return 0;

}

static int sys_init()
{
	CBDATA_INIT_TYPE_FREECB(server, free_server);
	return 0;
	
}

static int should_keep_alive(HttpStateData * httpState, int * keep_alive)
{
	debug(151,6)("mod_server_persist_connections : should_keep_alive enter, keep_alive == %d\n",*keep_alive);
	if(*keep_alive == 0)
		return 0;
	persist_config *cfg = cc_get_global_mod_param(mod);
	char* ipaddr = fd_table[httpState->fd].ipaddr;
	int	port = fd_table[httpState->fd].remote_port;


	hash_link *hptr;
	hptr = pconnLookup(ipaddr, port, NULL, NULL, 0);
	debug(151,6)("mod_server_persist_connections : should_keep_alive hptr == %p ipaddr = %s\n",hptr,ipaddr);
	if (hptr != NULL)
	{
		struct _pconn *p = (struct _pconn*)hptr;
			debug(151,6)("mod_server_persist_connections : should_keep_alive p->nfds == %d  cfg->max_conn == %d \n",p->nfds,cfg->max_conn);
		if(p->nfds > cfg->max_conn)
		{
			debug(151,6)("mod_server_persist_connections : should_keep_alive p->nfds == %d is larger than cfg->max_conn == %d ,so we set keep_alive flag to zerooooooooooooooooooooooooooooooo\n",p->nfds,cfg->max_conn);
			*keep_alive = 0;
		}

	}
	return 0;
}

/*static void dup_persist_fd(void *data)
{

	ConnectStateData *cs = data;
	int idle;
	int	fd = pconnPop(inet_ntoa((cs->in_addr)), cs->port, NULL, NULL, 0, &idle);
	debug(151,3)("mod_server_persist_connections:dup_persist_fd pconnPop fd == %d\t ,and cs->fd == %d , %s\n",fd, cs->fd,inet_ntoa(cs->in_addr));
	if(fd!=-1 && fd!=cs->fd )
	{
	debug(151,4)("mod_server_persist_connections:dup_persist_fd fd != cs->fd, should dup\n");
		dup2(fd,cs->fd);
	}

}
*/


int mod_register(cc_module *module)
{
	debug(151, 1)("(mod_ccv) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init), 
			sys_init);
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
			func_sys_parse_param);
	//module->hook_func_http_req_parsed = set_ccv;

	cc_register_hook_handler(HPIDX_hook_func_private_set_name_as_ip,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_name_as_ip), 
			set_name_as_ip);

	cc_register_hook_handler(HPIDX_hook_func_private_http_repl_read_end,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_repl_read_end), 
			should_keep_alive);
	/*
	cc_register_hook_handler(HPIDX_hook_func_commConnectDnsHandle,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_commConnectDnsHandle), 
			get_perisit_connection_again);
			*/
	cc_register_hook_handler(HPIDX_hook_func_private_openIdleConn,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_openIdleConn), 
			private_openIdleConn);
	cc_register_hook_handler(HPIDX_hook_func_private_set_host_as_ip,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_host_as_ip), 
			set_host_as_ip);

//	cc_register_hook_handler(HPIDX_hook_func_private_set_upperlayer_persist_connection_tcp_keepalive,
//			module->slot, 
//			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_upperlayer_persist_connection_tcp_keepalive), 
//			set_upperlayer_persist_connection_tcp_keepalive);
//
	cc_register_hook_handler(HPIDX_hook_func_private_set_upperlayer_persist_connection_timeout,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_upperlayer_persist_connection_timeout), 
			set_upperlayer_persist_connection_timeout);
//	cc_register_hook_handler(HPIDX_hook_func_private_dup_persist_fd,
//			module->slot, 
//			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_dup_persist_fd), 
//			dup_persist_fd);
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif


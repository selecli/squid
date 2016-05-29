#include "cc_framework_api.h"
#include "m_ipcache.h"

#include <sys/types.h>
#include <sys/param.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <unistd.h>  


#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;
#define COUNT 15
#define MAXINTERFACES 10

extern void _fwdInitiateSSL(FwdState * fwdState);
extern void _fwdDispatch(FwdState * fwdState);
extern void _fwdConnectStart(void *data);
extern void cc_copy_fd_params(fde *client, fde* server);
//extern PF _fwdServerClosed;
extern PF fwdServerClosed;

typedef struct _mod_config{
int m_ttl;	
}mod_config;

static MemPool * mod_config_pool = NULL;

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL; 
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_dst_ip config_struct", sizeof(mod_config));
	}
	return obj = memPoolAlloc(mod_config_pool);
}
static int free_mod_config(void *data)
{
	mod_config *cfg = (mod_config *)data; 
	if (cfg)
	{
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL; 
	}
	return 0;
}
//callback: free the data
/*
static int free_callback(void* param)
{
	if(param)
		xfree(param);

	return 0;
}
*/

enum 
{
	INIT,
	OPEN,
	CLOSE_CALLED,
};

static struct in_addr mod_local_addr[COUNT];
static int mod_local_addr_count = 1;
static int mult_outgoing = 1;

void init_local_addr()
{
	int fd, intrface = 0;
	struct ifreq buf[MAXINTERFACES];
	struct ifconf ifc;

	if((fd = socket(AF_INET,SOCK_DGRAM,0)) >= 0)
	{
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = (caddr_t) buf;
		if (!ioctl(fd, SIOCGIFCONF, (char *) &ifc))
		{
			intrface = ifc.ifc_len / sizeof(struct ifreq);
			debug(186, 3)("(mod_dst_ip) -> IF num: %d\n", intrface);

			while (intrface-- > 0)
			{
				ioctl(fd, SIOCGIFFLAGS, (char *) &buf[intrface]);

				if (buf[intrface].ifr_flags & IFF_UP)
				{
					if (!(ioctl(fd, SIOCGIFADDR, (char *) &buf[intrface])))
					{
						debug(186, 3)("(mod_dst_ip) -> IF: [%s], IP address is: [%s]\n", 
									buf[intrface].ifr_name, 
										inet_ntoa(((struct sockaddr_in *)
											(&buf[intrface].ifr_addr))->sin_addr));
						if(!strcmp("lo", buf[intrface].ifr_name))
							continue;

						mod_local_addr[mod_local_addr_count++] = ((struct sockaddr_in *)(&buf[intrface].ifr_addr))->sin_addr;
					}
				}
				else
				{
					debug(186, 1)("(mod_dst_ip) -> IF: [%s] is DOWN\n", buf[intrface].ifr_name);
				}
			}

			debug(186, 3)("(mod_dst_ip) -> IF num: %d\n", mod_local_addr_count);
		}

		close(fd);
	}
}

typedef struct dns_data
{
	char *host;	
	unsigned short port;
	int ctimeout; 
	int ret;
	FwdState *fwdState;
	unsigned char TOS;

	struct in_addr addr;	//local addr or outgoing ip 
	struct sockaddr_in S[COUNT];	//dst addr

	int fd[COUNT];		//fd
	char fd_status[COUNT];	//used to mark fd status
	int dst_addr_count;	//dst ip count
	int local_addr_count;	//local ip count
	int fd_count;		//valid fd
	int all_fd_count;	//all fd valid and invalid
	int host_numeric;	//whether host is in ip mode, such as: xxx.xxx.xxx.xxx
	int outgoing_set;	//whether the outgoing ip is alread set
	int hit;		//host hit ip_pair_cache(local and dst)
} _dns_data;

//typedef struct dns_data _dns_data;
CBDATA_TYPE(_dns_data);

// confige line example:
static int func_sys_parse_param(char *cfg_line)
{
	char cfgline[1024];
	char *pos;

	CBDATA_INIT_TYPE(_dns_data);
	memset(mod_local_addr, 0, sizeof(mod_local_addr));
	mod_local_addr_count = 0;
	mult_outgoing = 1;

	ipcache_pair_init();
	init_local_addr();

	strncpy(cfgline, cfg_line, 1024); 	
	if((pos = strtok(cfgline, w_space)) == NULL)
		goto err;
	else if(strcmp(pos,"mod_dst_ip"))
		goto err;
		
	mod_config *cfg = mod_config_pool_alloc();
	if((pos = strtok(NULL,w_space)) != NULL)
		if(!strncasecmp(pos,"timeout=", 8))
		{
			pos += 8;
		}

	if((m_ttl = cfg->m_ttl = atoi(pos)) > 1)
	{
		debug(186, 3)("(mod_dst_ip) -> ip_pair cache time: %d\n", m_ttl);
	}
	else
	{
		m_ttl = cfg->m_ttl = 300;
		debug(186, 3)("(mod_dst_ip) -> ip_pair cache time: %d\n", m_ttl);
	}
	cc_register_mod_param(mod, cfg, free_mod_config);
	return 0;
err:
	if (cfg)
	{   
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
	debug(186, 1)("mod_dst_ip ->  parse err, cfgline=%s\n", cfg_line);
	return -1;

/*

	struct mod_conf_param *data = NULL;
	char* token = NULL;

	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_negative_ttl"))
			goto out;	
	}
	else
	{
		debug(94, 3)("(mod_negative_ttl) ->  parse line error\n");
		goto out;	
	}

	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(94, 3)("(mod_negative_ttl) ->  parse line data does not existed\n");
		goto out;
	}

	data = xmalloc(sizeof(struct mod_conf_param));
	data->negative_ttl = atoi(token);
	debug(94, 2) ("(mod_negative_ttl) ->  netative_ttl=%d\n", data->negative_ttl);
	if(0 > data->negative_ttl)
	{
		debug(94, 2)("(mod_negative_ttl) ->  data set error\n");
		goto out;
	}

	cc_register_mod_param(mod, data, free_callback);
	return 0;
	
out:
	return 0;
	
out:
	if (data)
		xfree(data);
	return -1;		
*/
}

static int check_numeric(char *host)
{
	struct in_addr ip;
	
	if (!safe_inet_addr(host, &ip))
                return 0;
	else
		return 1;
}

static void init_dst_ip(const ipcache_addrs * ia, struct dns_data *data)
{
	struct dns_data *dns_data = data;
	int addr_count;
	int i;

	addr_count = ia->count;

	if(addr_count > COUNT)
		addr_count = COUNT;

	for(i = 0; i < addr_count; i++)
	{
		dns_data->S[i].sin_family = AF_INET;
		dns_data->S[i].sin_addr = ia->in_addrs[i];
		debug(186, 3)("(mod_dst_ip) -> init dst ip[%d]: %s\n", i, inet_ntoa(ia->in_addrs[i]));
		dns_data->S[i].sin_port = htons(dns_data->port);
		dns_data->dst_addr_count++;
	}

	return;
}

static void init_dst_ip_hit(struct in_addr dst_ip, struct dns_data* data)
{
	struct dns_data *dns_data = data;

	dns_data->S[0].sin_family = AF_INET;
	dns_data->S[0].sin_addr = dst_ip;
	dns_data->S[0].sin_port = htons(dns_data->port);
	dns_data->dst_addr_count = 1;

	return;
}

static void commConnHandle(int fd, void *data)
{
	struct dns_data *dns_data = data;
	FwdServer *fs = dns_data->fwdState->servers;
	int i;
	int found;

	struct in_addr local_addr;
	struct in_addr dst_addr;

	debug(186, 9)("(mod_dst_ip) -> commConnHandle fd: %d data: %p fwdState: %p url:%s\n", fd, data, dns_data->fwdState, storeUrl(dns_data->fwdState->entry));

	found = 0;
//	for(i = 0; i < COUNT; i++)
	for(i = 0; i < dns_data->all_fd_count; i++)
	{
		if(dns_data->fd[i] == fd)
		{
			debug(186, 3)("(mod_dst_ip) -> find fd: %d\n", fd);

			if(dns_data->fd_status[i] != OPEN)
			{
				debug(186, 3)("(mod_dst_ip) -> but it's status is not Open !!!\n");
				continue;
			}
			else
			{
				found = 1;
				break;
			}
		}
	}

	if(found)
		debug(186, 3)("(mod_dst_ip) -> in ConnHandle fd : %d found\n", fd);
	else
		debug(186, 2)("(mod_dst_ip) -> in ConnHandle fd: %d not found\n", fd);

	switch (comm_connect_addr(fd, &(dns_data->S[i%(dns_data->dst_addr_count)])))
	{
		case COMM_INPROGRESS:
//			debug(186, 5) ("commConnectHandle: FD %d: COMM_INPROGRESS\n", fd);
			debug(186, 3)("(mod_dst_ip) -> fd: %d, COMM_INPROGRESS\n", fd);
			commSetSelect(fd, COMM_SELECT_WRITE, commConnHandle, data, 0);
			break;
		case COMM_OK:
			debug(186, 3)("(mod_dst_ip) -> fd: %d, COMM_OK\n", fd);
			ipcacheMarkGoodAddr(dns_data->host, dns_data->S[i%(dns_data->dst_addr_count)].sin_addr);
		//	for(i = 0; i < COUNT; i++)
			for(i = 0; i < dns_data->all_fd_count; i++)
			{
				if((dns_data->fd[i] != fd) && (dns_data->fd_status[i] == OPEN))
				{
					debug(186, 3)("(mod_dst_ip) -> conn ok, so we close other fd: %d\n", dns_data->fd[i]);
					dns_data->fd_status[i] = CLOSE_CALLED;
					dns_data->fd_count--;
					comm_close(dns_data->fd[i]);
				}

				if(dns_data->fd[i] == fd)
				{
					//add ip pair into cache table
					if(dns_data->hit)
					{
						(void)0;
					}
					else
					{
						if(dns_data->outgoing_set)
						{
							local_addr = dns_data->addr;
						}
						else
						{
							local_addr = mod_local_addr[i/(dns_data->dst_addr_count)];
						}
					
						dst_addr = dns_data->S[i%(dns_data->dst_addr_count)].sin_addr;

						ipcache_pair_add(dns_data->host, local_addr, dst_addr);

						debug(186, 3)("(mod_dst_ip) -> fd: %d, conn OK:\n", fd);
						debug(186, 3)("(mod_dst_ip) -> local_addr: %s\n", inet_ntoa(local_addr));
						debug(186, 3)("(mod_dst_ip) -> dst_addr: %s \n", inet_ntoa(dst_addr));

					}
				}
		
			}

			dns_data->fwdState->server_fd = fd;
			dns_data->fwdState->n_tries++;
			debug(186, 3)("(mod_dst_ip) -> dns_data->fwdState->server_fd: %d url:%s\n", dns_data->fwdState->server_fd, storeUrl(dns_data->fwdState->entry));
			dns_data->fwdState->called = 1;
			//dns_data->conn_ok = 1;
			cc_copy_fd_params(&dns_data->fwdState->old_fd_params, &fd_table[dns_data->fwdState->server_fd]);
			//comm_add_close_handler(fd, _fwdServerClosed, dns_data->fwdState);
			comm_add_close_handler(fd, fwdServerClosed, dns_data->fwdState);

			//   commConnectCallback(cs, COMM_OK);
			if (Config.onoff.log_ip_on_direct && fs->code == HIER_DIRECT)
               			 hierarchyNote(&dns_data->fwdState->request->hier, fs->code, fd_table[fd].ipaddr);
#if USE_SSL
			if ((fs->peer && fs->peer->use_ssl) ||
					(!fs->peer && dns_data->fwdState->request->protocol == PROTO_HTTPS))
			{
				dns_data->fwdState->data = NULL;
				_fwdInitiateSSL(dns_data->fwdState);
				cbdataUnlock(dns_data);
				if(dns_data->host)
				{
					xfree(dns_data->host);
					dns_data->host = NULL;
				}
				cbdataFree(dns_data);
				return;
			}
#endif
			dns_data->fwdState->data = NULL;
			_fwdDispatch(dns_data->fwdState);
			cbdataUnlock(dns_data);
			if(dns_data->host)
			{
				xfree(dns_data->host);
				dns_data->host = NULL;
			}
			cbdataFree(dns_data);
			break;
		default:
			debug(186, 3)("(mod_dst_ip) -> fd: %d, default\n", fd);

			ipcacheMarkBadAddr(dns_data->host, dns_data->S[i].sin_addr);
			ip_cache_release_by_name(dns_data->host);

			//for(i = 0; i < COUNT; i++)
			for(i = 0; i < dns_data->all_fd_count; i++)
			{
				if((dns_data->fd[i] == fd) && (dns_data->fd_status[i] == OPEN))
				{
					debug(186, 3)("(mod_dst_ip) -> default fd: %d\n", fd);
					dns_data->fd_status[i] = CLOSE_CALLED;
					dns_data->fd_count--;
					comm_close(fd);
				}
			}

			if(dns_data->fd_count == 0)
			{
				debug(186, 3)("(mod_dst_ip) -> all tries failed\n");
//				dns_data->ret = -1;
				dns_data->fwdState->called = 1;
				dns_data->fwdState->data = NULL;
				_fwdConnectStart(dns_data->fwdState);
				cbdataUnlock(dns_data);
				if(dns_data->host)
				{
					xfree(dns_data->host);
					dns_data->host = NULL;
				}
				cbdataFree(dns_data);
				return;
			}
			break;
	}

	return;
}

static void commConnAdd(int fd, int i, void *data)
{
	struct dns_data *dns_data = data;
	debug(186, 6)("(mod_dst_ip) -> comm add conn fd: %d for %s \n", fd, storeUrl(dns_data->fwdState->entry));
	debug(186, 9)("(mod_dst_ip) -> data: ((( %p ))), fwdState: %p\n", data, dns_data->fwdState);

	comm_connect_addr(fd, &(dns_data->S[i]));
	commSetSelect(fd, COMM_SELECT_WRITE, commConnHandle, data, 0);

	return;
}

static void fwd_conn_timeout(int fd, void *data)
{
	//	(void *)0;
	/*
	   debug(186, 3)("(mod_dst_ip) -> fwd_conn_timeout fd: %d\n", fd);
	   comm_close(fd);
	 */

	struct dns_data *dns_data = data;
	int i;
	int found = 0;

	//for(i = 0; i < COUNT; i++)
	for(i = 0; i < dns_data->all_fd_count; i++)
	{
		if(dns_data->fd[i] == fd)
		{
			found = 1;
			debug(186, 3)("(mod_dst_ip) -> find it\n");

			ipcacheMarkBadAddr(dns_data->host, dns_data->S[i].sin_addr);

			if(dns_data->fd_status[i] == OPEN)
			{
				debug(186, 3)("(mod_dst_ip) -> conn timeout fd: %d\n", fd);
				dns_data->fd_status[i] = CLOSE_CALLED;
				dns_data->fd_count--;
				comm_close(fd);
			}

			break;
		}
	}

	if(dns_data->fd_count == 0)
	{
		debug(186, 3)("(mod_dst_ip) -> all tries failed\n");
	//	dns_data->ret = -1;
		dns_data->fwdState->called = 1;
		dns_data->fwdState->data = NULL;
		_fwdConnectStart(dns_data->fwdState);
		cbdataUnlock(dns_data);
		if(dns_data->host)
		{
			xfree(dns_data->host);
			dns_data->host = NULL;
		}
		cbdataFree(dns_data);
		return;
	}
}

static void commDnsHandle(const ipcache_addrs * ia, void *data)
{
	struct dns_data *dns_data = data;
	struct in_addr local_addr;
	FwdServer *fs = dns_data->fwdState->servers;
	FwdState *fwdState = dns_data->fwdState;
	int addr_count;
	int i,j,k;
	int fd;
	int m_mod_local_addr_count = -1;

	const char *url = storeUrl(dns_data->fwdState->entry);
///////////////////////////
	const ipcache_addr_pair* addr_pair;

	if((addr_pair = ipcache_get_ip_pair_by_name(dns_data->host, 0)) != NULL)
	{

		debug(186, 5)("(mod_dst_ip) -> dns_data->host: %s HIT local addr: %s\n", dns_data->host, inet_ntoa(addr_pair->local_ip));
		debug(186, 5)("(mod_dst_ip) -> dns_data->host: %s HIT dst addr: %s\n", dns_data->host, inet_ntoa(addr_pair->dst_ip));
		dns_data->hit = 1;
		addr_count = dns_data->dst_addr_count = 1;
		m_mod_local_addr_count = 1;

		init_dst_ip_hit(addr_pair->dst_ip, dns_data);
	}
	else
	{

		if (ia == NULL)
		{
			debug(186, 3)("(mod_dst_ip) -> ia == NULL \n");
			//	dns_data->ret = -1;
			goto failed;
		}

		init_dst_ip(ia, dns_data);
		addr_count = dns_data->dst_addr_count;

		if(addr_count == -1)
		{
			debug(186, 3)("(mod_dst_ip) -> addr_count == -1 \n");
			//	dns_data->ret = -1;
			goto failed;
		}

		if(dns_data->outgoing_set)
			m_mod_local_addr_count = 1;
		else
			m_mod_local_addr_count = mod_local_addr_count;

	}


	debug(186, 3)("(mod_dst_ip) -> local_addr_count: [%d] dst_ip_count: [%d] \n", m_mod_local_addr_count, addr_count);

	k = 0;
	for(j = 0; j < m_mod_local_addr_count; j++)
	{
		if(dns_data->hit)
		{
			local_addr = addr_pair->local_ip;
		}
		else
		{
			if(dns_data->outgoing_set)
			{
				local_addr = dns_data->addr;
			}
			else
			{
				local_addr = mod_local_addr[j];
			}
		}

		for(i = 0; i < addr_count; i++, k++)
		{
			if(k == COUNT)
				return;

			/* ?????????
			   if(dns_data->addr.s_addr == INADDR_ANY)
			   {
			   local_addr = dns_data->addr;
			   }
			 */

			fd = comm_openex(SOCK_STREAM,
					IPPROTO_TCP,
					//		dns_data->addr,
					local_addr,
					0,
					COMM_NONBLOCKING,
					dns_data->TOS,
					url);
			dns_data->fd[k] = fd;
			dns_data->all_fd_count++;

			if(fd < 0)
			{
				debug(186, 3)("(mod_dst_ip) -> fd: %d < 0 \n", fd);
				continue;
			}

			debug(186, 5)("(mod_dst_ip) -> comm_open success fd: %d \n", fd);

//			cbdataLock(dns_data);

			dns_data->fd_status[k] = OPEN;
			dns_data->fd_count++;

			commSetTimeout(fd, dns_data->ctimeout, fwd_conn_timeout, dns_data);


			if (fs->peer)
			{
				hierarchyNote(&fwdState->request->hier, fs->code, fs->peer->name);
			}
			else
			{
#if LINUX_TPROXY
				if (fwdState->request->flags.tproxy)
				{

					itp.v.addr.faddr.s_addr = fwdState->src.sin_addr.s_addr;
					itp.v.addr.fport = 0;

					/* If these syscalls fail then we just fallback to connecting
					 * normally by simply ignoring the errors...
					 */
					itp.op = TPROXY_ASSIGN;
					if (setsockopt(fd, SOL_IP, IP_TPROXY, &itp, sizeof(itp)) == -1)
					{
						debug(20, 1) ("tproxy ip=%s,0x%x,port=%d ERROR ASSIGN\n",
								inet_ntoa(itp.v.addr.faddr),
								itp.v.addr.faddr.s_addr,
								itp.v.addr.fport);
					}
					else
					{
						itp.op = TPROXY_FLAGS;
						itp.v.flags = ITP_CONNECT;
						if (setsockopt(fd, SOL_IP, IP_TPROXY, &itp, sizeof(itp)) == -1)
							if (setsockopt(fd, SOL_IP, IP_TPROXY, &itp, sizeof(itp)) == -1)
							{
								debug(20, 1) ("tproxy ip=%x,port=%d ERROR CONNECT\n",
										itp.v.addr.faddr.s_addr,
										itp.v.addr.fport);
							}
					}
				}
#endif
				hierarchyNote(&fwdState->request->hier, fs->code, fwdState->request->host);
			}


//			commConnHandle(fd, dns_data);
			commConnAdd(fd, i, dns_data);
		}
	}


	debug(186, 6)("(mod_dst_ip) -> comm Add end  ===========================\n");

	if(dns_data->fd_count == 0)
	{
		debug(186, 3)("(mod_dst_ip) -> fd_count == 0 \n");
		goto failed;
	}

	return;

failed:
	//	dns_data->ret = -1;
	dns_data->fwdState->called = 1;
	dns_data->fwdState->data = NULL;
	_fwdConnectStart(dns_data->fwdState);
	cbdataUnlock(dns_data);
	if(dns_data->host)
	{
		xfree(dns_data->host);
		dns_data->host = NULL;
	}
	cbdataFree(dns_data);

	return;
}

static int func_private_dst_ip(const char *host, unsigned short port, int ctimeout, 
						struct in_addr addr, unsigned char TOS, FwdState* fwdState)
{
	static int numeric = 0;
	static int set = 0;

	debug(186, 3)("(mod_dst_ip) -> fwdState: [%p] host: [%s] url: [%s]\n", fwdState, host, storeUrl(fwdState->entry));

	if(fwdState->called == 1)
	{
		debug(186, 3)("(mod_dst_ip) -> called == 1\n");
		return 0;
	}

	if(check_numeric((char *)host))
	{
		debug(186, 3)("(mod_dst_ip) -> host is numeric\n");
//		return 0;
		numeric = 1;
	}

	if(addr.s_addr != INADDR_ANY)
	{
		debug(186, 3)("(mod_dst_ip) -> outgoing ip is already set\n");
		set = 1;
	}

	//outgoing ip and host ip is ok?
	if(numeric && set)
	{
		debug(186, 3)("(mod_dst_ip) -> outgoing ip and host ip is already set\n");
		return 0;
	}

//	struct dns_data dns_data;
//	struct dns_data * m_dns_data = xmalloc(sizeof(struct dns_data));	
	struct dns_data * m_dns_data = cbdataAlloc(_dns_data);

	memset(m_dns_data , 0, sizeof(_dns_data));
	m_dns_data->host = xstrdup(host);
	m_dns_data->port = port;
	m_dns_data->ctimeout = ctimeout;
	m_dns_data->ret = 0;
	m_dns_data->addr = addr;
	m_dns_data->TOS = TOS;
	m_dns_data->fwdState = fwdState;
	m_dns_data->host_numeric = numeric;
	m_dns_data->outgoing_set = set;

	fwdState->data = m_dns_data;

	cbdataLock(m_dns_data);

//	c = cbdataAlloc(generic_cbdata);
//       c->data = i;

	debug(186, 6)("(mod_dst_ip) -> alloc cbdata: %p for fwdState: %p, url: %s \n", m_dns_data, fwdState, storeUrl(fwdState->entry));

	ipcache_nbgethostbyname(host, commDnsHandle, m_dns_data);

/*
	if(m_dns_data->ret == -1)
	{
		debug(186, 1)("(mod_dst_ip) -> ret == -1\n");
		cbdataFree(m_dns_data);
		return 0;
	}
*/
	
	return 1;
}


static int func_private_dst_ip2(FwdState* fwdState)
{

	debug(186, 3)("(mod_dst_ip) -> fwdState free: %p\n", fwdState);

	if(fwdState->data == NULL)
	{
		debug(186, 3)("(mod_dst_ip) -> fwdState free fwdState->data == NULL\n");
		return 0;
	}

	debug(186, 3)("(mod_dst_ip) -> fwdState free fwdState->data: %p\n", fwdState->data);

	struct dns_data *dns_data = fwdState->data;

	//	if(fwdState->server_fd == -1)

	int i;

	for(i = 0; i < dns_data->all_fd_count; i++)
	{

		if(dns_data->fd_status[i] == OPEN)
		{
			debug(186, 3)("(mod_dst_ip) -> close fd: %d\n", dns_data->fd[i]);
			dns_data->fd_status[i] = CLOSE_CALLED;
			dns_data->fd_count--;
			comm_close(dns_data->fd[i]);
		}

	}

	cbdataUnlock(dns_data);
	if(dns_data->host)
	{
		xfree(dns_data->host);
		dns_data->host = NULL;
	}
	cbdataFree(dns_data);

	fwdState->data = NULL;

	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(186, 1)("(mod_dst_ip) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_private_negative_ttl = func_private_negative_ttl;
	cc_register_hook_handler(HPIDX_hook_func_private_dst_ip,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_dst_ip),
			func_private_dst_ip);

	cc_register_hook_handler(HPIDX_hook_func_private_dst_ip2,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_dst_ip2),
			func_private_dst_ip2);

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	mod = module;

	return 0;
}
#endif

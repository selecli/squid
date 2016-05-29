#include "squid.h"
#include <string.h>
#include <stdio.h>
#include "ua_cache.h"

#define CA_SERVER_PATH "/usr/local/squid/bin/ca_serverd"
#define CA_SERVER_PIDFILE  "/var/run/ca_serverd.pid"

#define MAX_CA_IP_NUM 10

static cc_module *mod = NULL;
char config_host[64];
char CA_host[128];
unsigned short config_port = 80;
unsigned short CA_port = 80;

char *name = "X-CC-HOST-IP";

extern void _fwdConnectStart(void *data);

typedef struct _request_t_param
{
	unsigned char hit_ua;		//hit ua
	unsigned char has___ca__0;	//uri has "__ca__=0"
	unsigned char has___ca__;	//uri has "__ca__"  0 or !0
} request_t_param;

typedef struct _ca_ip_struct
{
	char ca_ip[32];			//ip string in num format like: 192.168.100.137
} ca_ip_struct;

ca_ip_struct ca_ips[MAX_CA_IP_NUM];
static int ca_ip_num = 0;
static int hash_ca = 0;

enum
{
	IP_HOST,
	IP_FILE
};

static int ca_ip_type = IP_HOST;		//get ip from host or config
static int pos = 0;


static MemPool * request_t_param_pool = NULL;

static void * request_t_param_pool_alloc(void)
{       
        void * obj = NULL;
        if (NULL == request_t_param_pool)
        {
                 request_t_param_pool = memPoolCreate("mod_ca private_data request_t_param", sizeof(request_t_param));
        }
        return obj = memPoolAlloc(request_t_param_pool);
}       

static int free_request_t_param(void* data)
{               
        request_t_param *param = (request_t_param *) data;
        if(param != NULL){
                memPoolFree(request_t_param_pool, param);
                param = NULL;
        }       
        return 0;
}               

static void init_ca_ip(char *file)
{
	FILE *fp;

	assert(file);

        if((fp = fopen(file, "r")) == NULL)
        {
                debug(184, 1)("can not open file: %s", file);
                abort();
        }

	ca_ip_num = 0;

	while(fgets(ca_ips[ca_ip_num].ca_ip, 32 - 1, fp))
        {
/*
                if(ca_ips[ca_ip_num].ca_ip[strlen(ca_ips[ca_ip_num].ca_ip) - 1] == '\n')
                        ca_ips[ca_ip_num].ca_ip[strlen(ca_ips[ca_ip_num].ca_ip) - 1] = '\0';
*/
		if(strtok(ca_ips[ca_ip_num].ca_ip, w_space) == NULL)
			continue;

                if(strlen(ca_ips[ca_ip_num].ca_ip) == 0)
                        continue;

		ca_ip_num++;

		if(ca_ip_num == MAX_CA_IP_NUM -1)
		{
			debug(184, 1)("too many ca ips in file, ignore them\n");
			break;
		}
	}

	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}

	return;
}

static unsigned long ELFhash(const char *key)
{       
        unsigned long h = 0;
                
        while(*key)
        {
                h = (h<<4) + *key++;

                unsigned long g = h & 0Xf0000000L;

                if(g)
                        h ^= g >> 24;
                h &= ~g;
        }
        
        return h;
}

static int get_pos(const char *uri)
{       
        if(!hash_ca)
        {
                pos++;
                return ((pos >= ca_ip_num)?(pos = 0):pos);
        }       

        assert(uri && strlen(uri));

        return ELFhash(uri)%ca_ip_num;
}

static char * get_ca_ip(const char *req)
{
	if(ca_ip_num <= 0)
	{
		debug(184, 1)("(mod_ca) -> ca_ip_num <= 0, shit !!!\n");
		abort();
	}

	return ca_ips[get_pos(req)].ca_ip;
}

static int rf_start_ca_server()
{
        enter_suid();

        int cid = fork();
        if (cid == 0)   //child
        {
                int ret = execl(CA_SERVER_PATH, "ca_serverd","-d",(char *)0);;
                if (ret < 0)
                {
                        debug(184,2)("(mod_ca) --> execl error : %s\n",xstrerror());
                }
                exit(-1);
        }
        else if (cid < 0)
        {
                debug(184,2) ("fork error!!\n");
                leave_suid();
                return -1;
        }

        leave_suid();
        return 0;
}

static pid_t get_running_pid(void);
static void rf_check_ca_server()
{
        pid_t pid = get_running_pid();

        if (pid < 0)
        {
                debug(184,2)("(mod_ca) --> ca server stopped! Now starting it ...\n");


                int ret = rf_start_ca_server();
                if (ret != 0)
                {
                        debug(184,2)("(mod_ca) --> ERROR : start ca_server error!!!\n");
                }
        }

        eventAdd("rf_check_ca_server", rf_check_ca_server, NULL, 60, 0);
}

static pid_t get_running_pid(void)
{                       
        static pid_t last_pid = -1;

        if( last_pid == -1 ) {
                FILE *fd; 
                if( (fd = fopen(CA_SERVER_PIDFILE, "r")) == NULL )
                        return last_pid;
                        
                char pid[128];
                fgets(pid, sizeof(pid), fd);
                fclose(fd);

                last_pid = atoi(pid);
                if( last_pid < 0 ) {
                        last_pid = -1;
                        return -1;
                }
        }

        assert(last_pid != -1);

        char buff[1024];
        FILE *procfd;
        snprintf(buff, sizeof(buff), "/proc/%d/status", last_pid);

        if( (procfd = fopen(buff,"r")) == NULL ) {
                last_pid = -1;
		return last_pid;
        }

        fgets(buff, sizeof(buff), procfd);
        fclose(procfd);
        if( strstr(buff, "ca_server") == NULL ) {
                last_pid = -1;
                return last_pid;
        }

        return last_pid;
}


int ca_server_init()
{
	 pid_t pid = get_running_pid();

        if (pid < 0)
        {
                debug(184,2)("(mod_ca) --> ca server stopped! Now starting it ...\n");
                int ret = rf_start_ca_server();
                if (ret != 0)
                {
                        debug(184,2)("(mod_ca) --> ERROR : start ca_server error!!!\n");
                }

                cc_sleep(3);
                pid_t pid = get_running_pid();
                if (pid < 0)
                {
                        debug(184,2)("(mod_ca) --> ERROR : ca_server is not running !!!\n");
                        abort();
                }
        }

        eventAdd("rf_check_ca_server", rf_check_ca_server, NULL, 60, 0);

	return 0;
}
static int hook_init()
{
	debug(184, 1)("(mod_ca) ->	hook_init:----:)\n");

	ca_server_init();

	return 0;
}


static int hook_cleanup(cc_module *module)
{
	debug(184, 1)("(mod_ca) ->	hook_cleanup:\n");
	return 0;
}

static int add_store_url(clientHttpRequest *http)
{
	static char buffer[4096];
	HttpHeaderEntry *he;
	int need_return = 0;

	request_t_param *req_t_param = request_t_param_pool_alloc();
	memset(req_t_param, 0, sizeof(request_t_param));

        cc_register_mod_private_data(REQUEST_T_PRIVATE_DATA, http->request, req_t_param, free_request_t_param, mod);

	debug(184, 5)("(mod_ca) -> read request url %s\n",http->uri);
	if( !http || !http->uri || !http->request ) {
		debug(184, 3)("(mod_ca) -> http = NULL, return \n");
		return 0;
	}

	// __ca__=0 will submit to webserver
	if(strstr(http->uri, "__ca__=0") != NULL)
	{
		debug(184, 3)("(mod_ca) -> request uri has [__ca__=0]\n");
		req_t_param->has___ca__0 = 1;
		need_return = 1;
	}

	// __ca__= will submit to CA directly
	if(strstr(http->uri, "__ca__=") != NULL)
	{
		req_t_param->has___ca__ = 1;
		debug(184, 3)("(mod_ca) -> request uri has [__ca__=]\n");
	}

	//no UA ? send to webserver!
	if((he = httpHeaderFindEntry(&http->request->header, HDR_USER_AGENT)) == NULL)
	{
		debug(184, 3)("(mod_ca) -> http request has no UA !!!\n");
		need_return = 1;
	}
		
	//no hit UA ? general request! send to webserver!
	/*
	if(!hit_UA(strBuf(he->value)))
        {
		debug(184, 3)("(mod_ca) -> http request UA: %s is MISS UA_table\n", strBuf(he->value));
		need_return = 1;
	}
	*/

	UAcache_entry * i = NULL;
	if(he != NULL)
	{
		i = UAcache_get(strBuf(he->value));

		if(i == NULL)
		{
			debug(184, 3)("(mod_ca) -> http request UA: %s is MISS UA_table\n", strBuf(he->value));
			need_return = 1;
		}
		else
		{
			req_t_param->hit_ua = 1;
			debug(184, 3)("(mod_ca) -> http request UA: %s is HIT UA_table\n", strBuf(he->value));
		}
	}

	if(need_return)
	{
		debug(184, 3)("(mod_ca) -> we don't need to change store_url\n");
		return 0;
	}

	//at here, the request wanna to get the CA content...
	if(http->request->store_url)
		safe_free(http->request->store_url);

	strcpy(buffer, http->uri);
	strcat(buffer, "@@@");
	//strcat(buffer, strBuf(he->value));
	strcat(buffer, i->type);

	http->request->store_url = xstrdup(buffer);
		
	debug(184, 5)("mod_ca: http->store_url %s\n" ,http->request->store_url);
	return 0;
}

static int func_sys_parse_param(char *cfg_line)
{

	static char buffer[128];
	char *tmp;
	char *token;

	if( strstr(cfg_line, "allow") == NULL )
		return 0;

	if( strstr(cfg_line, "mod_ca") == NULL )
		return 0;


	if( (tmp = strstr(cfg_line, "UA_file=")) == NULL)
	{
		debug(184, 1)("(mod_ca) ->	parse_param: UA_file is not found in config_line!\n");
		abort();
	}

	strncpy(buffer, tmp + strlen("UA_file="), 127);

	
	if ((token = strtok(buffer, w_space)) == NULL)
	{
		debug(184, 1)("(mod_ca) ->	parse_param: can not parse UA_file!\n");
		abort();
	}
	else
	{
		debug(184, 3)("(mod_ca) ->	parse_param: UA_file: %s!\n", token);

		if(parse_UA_file(token))
		{
			debug(184, 3)("(mod_ca) ->	parse_param: parse UA_file to hash table failed %s!\n", token);
			abort();
		}
	}

	memset(buffer, 0, sizeof(buffer));
	if((tmp = strstr(cfg_line, "objserver_ip=")) != NULL)
	{
		strncpy(buffer, tmp + strlen("objserver_ip="), 127);

		if((token = strtok(buffer, w_space)) != NULL)
		{
			strncpy(config_host, token, 63);
		}
		else
		{
			strcpy(config_host, "127.0.0.1");
		}
	}
	else
	{
		strcpy(config_host, "127.0.0.1");
	}
	
	memset(buffer, 0, sizeof(buffer));

	if((tmp = strstr(cfg_line, "objserver_port=")) != NULL)
	{
		strncpy(buffer, tmp + strlen("objserver_port="), 127);

		if((token = strtok(buffer, w_space)) != NULL)
		{
			config_port = atoi(token);
		}
		else
		{
			config_port = 80;
		}
	}
	else
	{
		config_port = 80;
	}
		
	debug(184, 3)("(mod_ca) -> objserver_ip: [%s], objserver_port: [%d]\n", config_host, config_port);	

	memset(buffer, 0, sizeof(buffer));
	if((tmp = strstr(cfg_line, "ca_host=")) != NULL)
	{
		strncpy(buffer, tmp + strlen("ca_host="), 127);

		if((token = strtok(buffer, w_space)) != NULL)
		{
			strncpy(CA_host, token, 127);
		}
		else
		{
			debug(184,1)("(mod_ca) -> can't get ca_host from config_line, abort...\n");
			abort();
		}
	}

	memset(buffer, 0, sizeof(buffer));
	if((tmp = strstr(cfg_line, "type=conf")) != NULL)
	{
		ca_ip_type = IP_FILE;
		debug(184,3)("(mod_ca) -> we will get ca ip from file: %s \n", CA_host);

		init_ca_ip(CA_host);

		if(ca_ip_num == 0)
		{
			debug(184, 1)("mod_ca) -> parse get nothing ip... failed!");
			abort();
		}
		else
		{
			debug(184, 3)("mod_ca) -> get %d ips \n", ca_ip_num);
		}
	}
	else
	{	ca_ip_type = IP_HOST;
		debug(184,3)("(mod_ca) -> we will get ca ip from dns parse\n");
	}

	
	memset(buffer, 0, sizeof(buffer));

	if((tmp = strstr(cfg_line, "ca_port=")) != NULL)
	{
		strncpy(buffer, tmp + strlen("ca_port="), 127);

		if((token = strtok(buffer, w_space)) != NULL)
		{
			CA_port = atoi(token);
		}
		else
		{
			CA_port = 80;
		}
	}
	else
	{
		CA_port = 80;
	}

	if(strstr(cfg_line, "hash") != NULL)
	{
		hash_ca = 1;
		debug(184, 3)("(mod_ca) -> hash mod\n");
	}
	else
	{
		hash_ca = 0;
		debug(184, 3)("(mod_ca) -> do not hash\n");
	}
		
	debug(184, 3)("(mod_ca) -> CA_host: [%s], CA_port: [%d]\n", CA_host, CA_port);	

	return 0;
}

void add_host_ip_real(char *host, int port, const ipcache_addrs * ia, void *data)
{
	FwdState *fwdState = data;
	struct in_addr in_addr;
	char *ip;
	char ip_port_buf[64];

	assert(ia->cur < ia->count);
        in_addr = ia->in_addrs[ia->cur];
        if (Config.onoff.balance_on_multiple_ip)
                ipcacheCycleAddr(host, NULL);

	ip = inet_ntoa(in_addr);

	memset(ip_port_buf, 0, sizeof(ip_port_buf));
	snprintf(ip_port_buf, 63, "%s:%d", ip, port);
	//sprintf(ip_port_buf, "%s:%d", inet_ntoa(in_addr), port);

	debug(184, 3)("(mod_ca) -> add_host_ip_real url: %s, host: %s, ip_port: %s\n", 
								storeUrl(fwdState->entry), host, ip_port_buf);

	httpHeaderAddEntry(&fwdState->request->header, httpHeaderEntryCreate(HDR_OTHER, name, ip_port_buf));


	return;
}

static void commDnsHandle(const ipcache_addrs * ia, void *data)
{
	FwdState *fwdState = data;
	char *host;
	int port;

	debug(184, 3)("(mod_ca) -> commDnsHandle url: %s\n", storeUrl(fwdState->entry));

	FwdServer *fs = fwdState->servers;
	assert(fs);

	if (fs->peer)
	{
		host = fs->peer->host;
		port = fs->peer->http_port;
	}
	else
	{
		host = fwdState->request->host;
		port = fwdState->request->port;
	}

	if (ia == NULL)
        {
                debug(184, 1) ("commConnectDnsHandle: Unknown host: %s\n", host);
		goto end;
	}

	add_host_ip_real(host, port, ia, data);

end:
	_fwdConnectStart(fwdState);

	return;
}



static void add_host_ip_header(char* host, FwdState *fwdState)
{
	ipcache_nbgethostbyname(host, commDnsHandle, fwdState);
	return;
}

static int httpHeaderCheckExistByName(HttpHeader *h, char *name)
{
        HttpHeaderPos pos = HttpHeaderInitPos;
        HttpHeaderEntry *curHE;
        while ((curHE = httpHeaderGetEntry(h, &pos)))
        {
                if(!strcmp(strBuf(curHE->name), name))
                {
                        debug(184,3)("Host-IP header is found in request!\n");
                        return 1;
                }
        }

        debug(184,3)("No Host-IP header found in request!\n");
        return 0;
}

static int check_numeric(char *host)
{
        struct in_addr ip;

        if (!safe_inet_addr(host, &ip))
                return 0;
        else
                return 1;
}

static void drop_ca_param(char* param, char* url)
{
        char *s;
        char *e;

        s = strstr(url, param);
        if(s)
        {
                e = strstr(s, "&");
                if( e == 0 )
                        *(s-1) = 0;
                else {
                        memmove(s, e+1, strlen(e+1));
                        *(s+strlen(e+1)) = 0;
                }
        }

	return;
}

void remove_ca(FwdState *fwdState)
{
	static char buf[4096];
	request_t *request = fwdState->request;

	strcpy(buf, "shit error !!!");

	if(strLen(request->urlpath))
	{
		strncpy(buf, strBuf(request->urlpath), 4095);
	}
        debug(184,3)("before remove \"__ca__=0\", request url: %s\n", buf);

	drop_ca_param("__ca__=0", buf);
        debug(184,3)("after remove \"__ca__=0\", request url: %s\n", buf);

	stringReset(&request->urlpath, buf);

	return;
}

//request will be sent to CA or ObjServer or WebServer
static int hook_add_host_ip_header(FwdState *fwdState)
{
	debug(184, 3)("(mod_ca) -> hook_add_host_ip_header url: %s\n", storeUrl(fwdState->entry));

	char *host;

	request_t_param *req_t_param = cc_get_mod_private_data(REQUEST_T_PRIVATE_DATA, fwdState->request, mod);
	if(req_t_param == NULL)
	{
		debug(184, 1)("(mod_ca) -> hook_add_host_ip_header, req_t_param == NULL\n");
		return 0;
	}


	// __ca__= will submit to webserver or ca, we only add host-ip to the only request that will be sent to ca_server
	if(req_t_param->has___ca__)
	{
		debug(184, 3)("(mod_ca) -> request uri has [__ca__=], return \n");
		return 0;
	}

	if(req_t_param->hit_ua == 0)
	{
		debug(184, 3)("(mod_ca) -> in hook_add_host_ip_header request MISS UA !\n");
		return 0;
	}

	//method GET and HIT UA and url has no "__ca__" 

	debug(184, 3)("(mod_ca) -> in fwdState request HIT UA table !!!\n");

	FwdServer *fs = fwdState->servers;
	assert(fs);

	if (fs->peer)
	{
		host = fs->peer->host;
	}
	else
	{
		host = fwdState->request->host;
	}

	if(check_numeric(host))
	{
		debug(184, 3)("(mod_ca) -> host: %s is numeric\n", host);
		return 0;
	}


	debug(184, 3)("(mod_ca) -> host: %s\n", host);

	add_host_ip_header(host, fwdState);

	return 1;
}
	

static void hook_change_hostport(FwdState *fwdState, char** host, unsigned short* port)
{

	debug(184, 3)("(mod_ca) -> hook_change_hostport request url: %s\n", storeUrl(fwdState->entry));
	debug(184, 3)("(mod_ca) -> hook_change_hostport before change, host: [%s], port: [%d]\n", *host, *port);

	request_t_param *req_t_param = cc_get_mod_private_data(REQUEST_T_PRIVATE_DATA, fwdState->request, mod);
	if(req_t_param == NULL)
	{
		debug(184, 1)("(mod_ca) -> hook_change_hostport, req_t_param == NULL\n");
		return;
	}

	if(req_t_param->has___ca__0)
	{
		debug(184, 3)("(mod_ca) -> \"__ca__=0\" found, so we only remove it, then send to web server !!!\n");
		remove_ca(fwdState);

		return;
	}

	if(req_t_param->hit_ua == 0)
	{
                debug(184, 3)("(mod_ca) -> in fwdState request change host port MISS UA table !!!\n");
	}
	else
	{
                debug(184, 3)("(mod_ca) -> in fwdState request HIT UA table !!!\n");

		//send to local ca_server
		if(strstr(storeUrl(fwdState->entry), "__ca__=") == NULL)
		{
			if(!httpHeaderCheckExistByName(&fwdState->request->header, name))
                        {
				debug(184, 1)("(mod_ca) -> in fwdState change host and port, we can not get the host ip header!!!\n");
                                return;
                        }

        		*host = config_host;
        		*port = config_port;
                	
			debug(184, 3)("(mod_ca) -> in fwdState host and port changed to ObjServer!!!\n");
		}
		else	//__ca__=!0, send to CA
		{
			debug(184, 3)("(mod_ca) -> \"__ca__=!0\" found, so we change the host and port to CA!!!\n");

			if(ca_ip_type == IP_HOST)
				*host = CA_host;
			else
				*host = get_ca_ip(storeUrl(fwdState->entry));

			*port = CA_port;
		}
	}

        debug(184, 3)("(mod_ca) -> hook_change_hostport after processed, host: [%s], port: [%d]\n", *host, *port);

	return;
}

int mod_register(cc_module *module)
{
	debug(184, 1)("(mod_ca) ->	mod_register:\n");

	strcpy(module->version, "7.0.R.16488.i686");

	//module->hook_func_sys_init = hook_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			hook_init);
	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	//module->hook_func_http_req_read_second = add_range;
	cc_register_hook_handler(HPIDX_hook_func_http_req_read_second,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read_second),
			add_store_url);

	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);

	cc_register_hook_handler(HPIDX_hook_func_private_add_host_ip_header_for_ca,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_add_host_ip_header_for_ca),
                        hook_add_host_ip_header);

	//module->hook_func_private_preload_change_hostport = hook_change_hostport;
        cc_register_hook_handler(HPIDX_hook_func_private_preload_change_hostport,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_preload_change_hostport),
                        hook_change_hostport);


	mod = module;

	return 0;
}


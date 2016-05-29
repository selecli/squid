/**************************************************************************
 *   Copyright (C) 2013
 *
 *   File ID:
 *   File name: mod_range.c
 *   Summary:
 *   Description: 
 *      1. Use to Select Back to Server's ip;
 *      2. Use to Obtain Server's ip by url; 
 *      3. Use to Obtain Server's ip by url , base64 decode; 
 *   Configure: Match  ACL
 ___________________________________________________________________________
 *   Version: V1.0, Create mod_polling_ServerIP;
 *   Author: Yiming.Zhang
 ___________________________________________________________________________
 *   Version: V2.0, Modify mod_polling_ServerIP
 *   Author: Yandong.Nian
 *   Date: 2013-03-04
 **************************************************************************/
#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;
LOCAL_ARRAY(char, ip_addr, SQUIDHOSTNAMELEN);

#define PMOD_NAME "mod_polling_ServerIP"
#define LIST_IP 1
#define SRCH_ST 2
#define SRCH_BS 3
#define SCAL_IP 4
#define SET_PORT 5
#define RECORD_IP 6
#define FIL_SIZE 100
#define MARK_SZ 20
#define LIST_URL 7


struct mod_conf_param
{
    int iclass;    //1 mean dst ipList; 2 mean search switch dst ip; 3 mean decode base64, and search switch dst ip//
    int count;
    dlink_node *ipList;
    dlink_node *ipHead;
    String search;
    String key_s;;
    String key_e;;
    in_addr_t port;
};



static dlink_node *serverIpCreat(char *v)
{
    struct in_addr ip;
    if (0 == safe_inet_addr(v, &ip))
    {
        debug(174, 1)("%s %s, isn't ip %s\n",PMOD_NAME, __func__, v);
        return NULL;
    }
    dlink_node *link = memAllocate(MEM_DLINK_NODE);
    struct in_addr *s_ip=  xcalloc(1, sizeof(struct in_addr));
    debug(174, 2)("%s %s Server IP %s\n", PMOD_NAME, __func__,v);
    inet_aton(v, s_ip);
    link->data = s_ip;
    link->prev = link->next = NULL;
    return link;
}


static int addTailListDone(struct mod_conf_param *param, char *v)
{
	
    dlink_node *L = serverIpCreat(v);
    if(NULL == L)
        return 1;
    param->count += 1;
    if(NULL == param->ipHead)
    {
        param->ipHead = param->ipList = L;
        return 0;
    }
    param->ipList->next = L;
    param->ipList = L;
    return 0;
}
static int addUrlListDone(struct mod_conf_param *param, char *v)
{
    dlink_node *L = NULL;
	L = memAllocate(MEM_DLINK_NODE);
    if( NULL == L )
        return 1;
	L->data = xstrdup(v); 
    param->count += 1;
	debug(174, 3)("%s %s %d addUrlListDone L->data  %s to v %s  data %s\n", 
                PMOD_NAME, __func__, param->iclass, (char *)L->data ,(char *)v,(char *)param->ipList->data);
    if(NULL == param->ipHead)
    {
        param->ipHead = param->ipList = L;
        return 0;
    }
    param->ipList->next = L;
    param->ipList = L;
    return 0;
}

static int parseServerIp(struct mod_conf_param *param, char *acl_name)
{
    int find = 0;
    acl *a = Config.aclList;
    if(reconfigure_in_thread)
        a = Config_r.aclList;
    for ( ; 0 == find && a; a = a->next)
    {
        if (!strcasecmp(a->name, acl_name))
        {
            wordlist *w, *v;
            v = w = aclDumpGeneric(a);
            while (v != NULL)
            {
                addTailListDone(param, v->key);
                v = v->next;
            }
            wordlistDestroy(&w);
            find = 1;
        }
    }
    if(0 == find) debug(174, 1)("%s %s don't find dst %s\n", PMOD_NAME, __func__, acl_name);
    return find;
}


//add by wuzhiyuan
static int parseServerUrl(struct mod_conf_param *param, char *acl_name)
{
    int find = 0;
    acl *a = Config.aclList;
    if(reconfigure_in_thread)
        a = Config_r.aclList;
    for ( ; 0 == find && a; a = a->next)
    {
        if (!strcasecmp(a->name, acl_name))
        {
            wordlist *w, *v;
            v = w = aclDumpGeneric(a);
            while (v != NULL)
            {
                addUrlListDone(param, v->key);
                v = v->next;
            }
            wordlistDestroy(&w);
            find = 1;
        }
    }
	debug(174, 3)("%s %s parseServerUrl a->name %s acl_name  %s\n",
                PMOD_NAME, __func__, a->name,acl_name);
    if(0 == find) debug(174, 1)("%s %s don't find dst %s\n", PMOD_NAME, __func__, acl_name);
    return find;
}


static int config_param_free(void* data)
{
    struct mod_conf_param* param = data;
    if(param)
    {
        debug(174, 3)("%s %s iclass %d\n", PMOD_NAME, __func__, param->iclass);
        int loop = 0;
        if(param->ipHead)
        { 
            dlink_node *next = NULL, *l_hdr = param->ipHead->next;
            param->ipHead->next = NULL;
            while(l_hdr)
            {
                next = l_hdr->next;
                struct in_addr *s_ip = l_hdr->data;
                safe_free(s_ip);
                l_hdr->prev = l_hdr->next = l_hdr->data = NULL;
                memFree(l_hdr, MEM_DLINK_NODE);
                l_hdr = next;
                loop++;
            }
        }
        param->ipList = param->ipHead = NULL;
        stringClean(&param->search);
        stringClean(&param->key_s);
        stringClean(&param->key_e);
        debug(174, 3)("%s %s loop %d iclass %d\n", PMOD_NAME, __func__, loop ,param->iclass);
        param->iclass = param->port = 0;
        safe_free(param);
    }
    return -1;
}

static int serverIpParse3(char *t, struct mod_conf_param * param)
{
    char *s = strchr(t, ':');
    if(NULL != s) 
    {   
        *s =  '\0';
        stringInit(&param->key_s, t); 
        if((s = (strchr(t = s + 1, ':'))))
        {   
            *s =  '\0';
            stringInit(&param->key_e, t); 
            stringInit(&param->search, s+1);
            return 0;
        }   
    } 
    return -1;
}

static int serverIpScaleList(char *s, struct mod_conf_param *param)
{
    char *t = strtok(s, "|");
    int loop = 0;
    struct in_addr ip;
    if(NULL == t)
        return param->count;
    stringClean(&param->search);
    stringInit(&param->search, t);
    debug(174, 2)("%s %s Scale Host %s\n", PMOD_NAME, __func__, t); 
    while((t = strtok(NULL, "|")))
    {
        if((s = strchr(t, ':')) && '\0' != *(++s) && safe_inet_addr(s, &ip) && (loop = atoi(t)) > 0)
        {
            debug(174, 2)("%s %s Scale Loop %d %s\n", PMOD_NAME, __func__, loop, s); 
            while((loop > 0))
            {
                addTailListDone(param, s);
                loop--;
            }
        }
    }
    return param->count;
}


static int parseServerPort(char *p, in_addr_t *port) 
{
    if(p && port && !strncmp(p, "port:", strlen("port:")) && 
            (p += strlen("port:")) && xisdigit(*p) && strlen(p) <= 8)
    {
        int ps = atoi(p);
        if(65535 >= ps && 0 < ps )
        {
            *port = ps;
            return 1;
        }
    }
    return 0;
}

static int hookServerIpParseConfigure(char *cfg_line)
{
    char *s = NULL, *p = NULL, *t = strtok(cfg_line, w_space);

    if (!t || strcmp(t, PMOD_NAME))
        return -1;
    if (!(t = strtok(NULL, w_space)) || !strcmp(t, "allow") || !strcmp(t, "deny"))
    {
        debug(174, 3)("%s %s does not existed\n", PMOD_NAME, __func__);
        return -1;
    }

    p = strtok(NULL, w_space);

    struct mod_conf_param  *param = xmalloc(sizeof(struct mod_conf_param));
    memset(param, 0, sizeof(struct mod_conf_param));

    parseServerPort(p, &param->port);

    if(0 == param->port && parseServerPort(t, &param->port))
    {
        param->iclass = SET_PORT;
        param->count = 1;
    }

    if(!strncmp(t, "dst_ip:", strlen("dst_ip:")) && strlen(t) > strlen("dst_ip:"))
    {
        t += strlen("dst_ip:");
        parseServerIp(param, t);
        if(NULL == param->ipHead || 0 >= param->count)
        {
            debug(174, 1)("%s %s dst_ip count %d ipList %p\n", PMOD_NAME, __func__, param->count, param->ipHead);
            return config_param_free(param);
        }
        debug(174, 3)("%s %s dst_ip:%s param->ipList %p class %d\n",
                PMOD_NAME, __func__, t, param->ipList, param->iclass);
        param->iclass = LIST_IP;
        param->ipList->next = param->ipHead;
    }

	//add by wuzhiyuan
	if(!strncmp(t, "dst_url:", strlen("dst_url:")) && strlen(t) > strlen("dst_url:"))
    {
        t += strlen("dst_url:");
        
        parseServerUrl(param, t);
        if(NULL == param->ipHead || 0 >= param->count)
        {
            debug(174, 1)("%s %s dst_ip count %d ipList %p\n", PMOD_NAME, __func__, param->count, param->ipHead);
            return config_param_free(param);
        }
        
        param->iclass = LIST_URL;
        param->ipList->next = param->ipHead;
		debug(174, 3)("%s %s dst_url param->ipList %p class %s iclass %d\n",
                PMOD_NAME, __func__, t, param->ipList, param->iclass);
    }

    if(!strncmp(t, "search:", strlen("search:")) && strlen(t) > strlen("search:"))
    {
        t += strlen("search:");
        stringInit(&param->search, t);
        param->iclass = SRCH_ST;
        debug(174, 3)("%s %s dst_ip:%s param->search:%s class %d\n",
                PMOD_NAME, __func__, t, param->search.buf, param->iclass);
        param->count = 1;
    }

    if(!strncmp(t, "decode:", strlen("decode:")) && strlen(t) > strlen("decode:"))
    {
        s = t;
        t += strlen("decode:");
        if(serverIpParse3(t, param))
        {
            debug(174, 3)("%s %s Prase Error %s\n", PMOD_NAME, __func__, s);
            return config_param_free(param);
        }
        param->iclass = SRCH_BS;
        param->count = 1;
    }

    if(!strncmp(t, "scale:", strlen("scale:")) && strlen(t) > strlen("scale:"))
    {
        s = t;
        t += strlen("scale:");
        serverIpScaleList(t, param);
        if(0 >= param->count || NULL == param->ipHead)
        {
            debug(174, 3)("%s %s Prase Error %s\n", PMOD_NAME, __func__, s);
            return config_param_free(param);
        }
        param->ipList->next = param->ipHead;
        param->iclass = SCAL_IP;
    }

    if(0 == param->iclass || 0 == param->count)
    {
        debug(174, 1)("%s %s Class %d  port %d\n", PMOD_NAME, __func__, param->iclass, param->port);
        return config_param_free(param);
    }
    cc_register_mod_param(mod, param, config_param_free);
    return 0;		
}

static int parseIpDone(const char *s_addr)
{
    int len = 0;
    if(!s_addr || (len = strcspn(s_addr, ",&")) <= 0)
    {   
        debug(174, 5)("%s %s %p %d\n", PMOD_NAME, __func__, s_addr, len);
        return 0;
    }   
    len = len > 32 ? 32 : len; ip_addr[0] = '\0';
    memset(ip_addr, '\0', SQUIDHOSTNAMELEN);
    strncat(ip_addr, s_addr, len);
    struct in_addr ip; 
    if(safe_inet_addr(ip_addr, &ip))
    {   
        debug(174, 5)("%s %s is Ip  parseIpDone %s\n", PMOD_NAME, __func__, ip_addr);
        return 1;
    }   
    long server  = atoll(ip_addr);
    ip.s_addr = ntohl(server);
    debug(174, 5)("%s %s %s %ld direct swtich parseIpDone %s\n",
            PMOD_NAME, __func__, ip_addr, server, inet_ntoa(ip));
    if(server)
    {   
        memset(ip_addr, '\0', SQUIDHOSTNAMELEN);
        strcat(ip_addr, inet_ntoa(ip));
        debug(174, 5)("%s switch %s %s\n", PMOD_NAME, __func__, ip_addr);
        return 1;
    }    
    return 0;
}


static void  serverIpSetHostDone(struct mod_conf_param  *param, char **host, const char *url)
{
    if(param->ipList && param->ipList->data)
    {
        struct in_addr *ip= param->ipList->data;
        param->ipList = param->ipList->next;
        memset(ip_addr, '\0', SQUIDHOSTNAMELEN);
        strncat(ip_addr, inet_ntoa(*ip),SQUIDHOSTNAMELEN -1);
        debug(174, 3)("%s %s %d ipList change %s to %s %s\n", 
                PMOD_NAME, __func__, param->iclass, *host , ip_addr, url);
        *host = (char *)ip_addr;
    }

}

//add by wuzhiyuan
static void  serverUrlSetHostDone(struct mod_conf_param  *param, char **host, const char *url)
{
    if(param->ipList && param->ipList->data)
    {
        param->ipList = param->ipList->next;
		memset(ip_addr, '\0', SQUIDHOSTNAMELEN);
        xstrncpy(ip_addr, (char *)param->ipList->data,SQUIDHOSTNAMELEN -1);
        debug(174, 3)("%s %s %d ipList change %s to %s url %s param->ipList->data %s \n", 
                PMOD_NAME, __func__, param->iclass, *host , ip_addr,url,(char *)param->ipList->data);
		*host = (char *)param->ipList->data;
    }
}


static void serverIpSetHostScale(struct mod_conf_param  *param, char** host, const char *url)
{
    serverIpSetHostDone(param, host, url);
}

static void serverIpSetHostList(struct mod_conf_param  *param, char** host, const char *url)
{
    serverIpSetHostDone(param, host, url);
}
static void serverUrlSetHostList(struct mod_conf_param  *param, char** host, const char *url)
{
    serverUrlSetHostDone(param, host, url);
}

static void serverIpSetHostSearch(struct mod_conf_param  *param, char** host, const char *url)
{
    char *key = NULL;
    int is_ip = 0;
    if((key = (strstr(url, param->search.buf))) && (is_ip = parseIpDone(key + param->search.len)))
    {
        *host = (char *)ip_addr;
        debug(174, 3)("%s %s search %s change %s\n", PMOD_NAME, __func__, *host, ip_addr);
        return;
    }
    debug(174, 3)("%s %s search %s key %p fail is_ip %d %s\n",
            PMOD_NAME, __func__, param->search.buf, key, is_ip, url);
}

static void serverIpSetHostBase64(struct mod_conf_param  *param, char** host, const char *url)
{
    char *s = (char *)url;
    char *e = NULL;
    int is_ip = 0;

    if(!(s = strstr(url, "://")))
    {
        debug(174, 3)("%s %s no find ://\n", PMOD_NAME, __func__);
        return ;
    }
    s += 3;

    if(param->key_s.buf)
    {
        if(!(s = strstr(s, param->key_s.buf)))
        {
            debug(174, 3)("%s %s start key %s search fail  %s\n", 
                    PMOD_NAME, __func__, param->key_s.buf, url);
            return ;
        }
        s += param->key_s.len;
    }

    int i_len = strlen(s);

    if(param->key_e.buf) 
    {
        if(!(e = strstr(s , param->key_e.buf)) && !param->key_s.buf)
        {
            debug(174, 3)("%s %s end key %s search fail  %s\n",
                    PMOD_NAME, __func__, param->key_e.buf, url);
            return;
        }
        if(e)
            i_len = e - s;
    }
    String base = StringNull;
    stringAppend(&base, s, i_len);
    const char * s_result = base64_decode(base.buf);
    debug(174, 7)("%s %s encode base64 %s\n", PMOD_NAME, __func__, base.buf);     
    debug(174, 7)("%s %s decode base64 %s\n", PMOD_NAME, __func__, s_result);     
    stringClean(&base);
    s = NULL;
    if(s_result && (s = strstr(s_result, param->search.buf)) && (is_ip = parseIpDone(s + param->search.len)));
    {
        debug(174, 3)("%s %s %s change to %s\n", PMOD_NAME, __func__, *host, ip_addr); 
        *host = (char *)ip_addr;
        return;
    }
}

static void
hookServerIpSelectDone(FwdState *fwd, char** host, unsigned short* port)
{
    const char *url = urlCanonical(fwd->request);
    struct mod_conf_param  *param = cc_get_mod_param_by_FwdState(fwd, mod);
    if(param && param->port && SET_PORT == param->iclass)
    {
        *port = param->port;
        debug(174, 3)("%s %s %d change -->%d %s\n",
                PMOD_NAME, __func__, fwd->request->port, param->port, url);
        return ;
    }
    if (NULL == param || fwd->old_fd_params.cc_run_state[mod->slot] > param->count)
    {
        debug(174, 3)("%s DNS PARSE %s\n", PMOD_NAME, url);
        return;
    }
    if(param && param->port)
    {
        *port = param->port;
        debug(174, 3)("%s %s %d change -->%d %s\n",
                PMOD_NAME, __func__, fwd->request->port, param->port, url);
    }
    fwd->old_fd_params.cc_run_state[mod->slot]++;
    fwd->n_tries = 0;
    fwd->origin_tries = 0;
    memset(ip_addr, '\0', SQUIDHOSTNAMELEN);

    if(param->search.buf && SCAL_IP == param->iclass && !strCmp(param->search, *host))
    {
        serverIpSetHostScale(param, host, url);
        return ;
    }
    if( LIST_IP == param->iclass )
    {
        serverIpSetHostList(param, host, url);
        return ;
    }
	if( LIST_URL == param->iclass )
    {
        serverUrlSetHostList(param, host, url );
        return ;
    }
    if(SRCH_ST == param->iclass && param->search.buf && url)
    {
        serverIpSetHostSearch(param, host, url);
        return ;
    }
    if(url && param && SRCH_BS == param->iclass && param->search.buf && (param->key_s.buf || param->key_e.buf))
    {
        serverIpSetHostBase64(param, host, url);
        return ;
    }
}

#undef LIST_IP
#undef SRCH_ST
#undef SRCH_BS
#undef SCAL_IP
#undef SET_PORT 
#undef RECORD_IP 
#undef FIL_SIZE 
#undef MARK_SZ 


int mod_register(cc_module *module)
{
    debug(174, 1)("%s init: init module\n", PMOD_NAME);

    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
            hookServerIpParseConfigure);

    cc_register_hook_handler(HPIDX_hook_func_private_preload_change_hostport,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_preload_change_hostport),
            hookServerIpSelectDone);

    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else 
        mod = module;
    return 0;
}
#undef PMOD_NAME
#endif

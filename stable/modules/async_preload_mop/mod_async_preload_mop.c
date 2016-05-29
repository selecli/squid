/**************************************************************************
 *   Copyright (C) 2013
 *
 *   File ID:
 *   File name: mod_async_preload_mop.c
 *   Summary:
 *   Description: 
 *   Configure: Match  ACL
 *   Version: V2.0
 *   Author: Yandong.Nian
 *   Date: 2013-03-04
 *   LastModified: 2013-10-18/Yandong.Nian;
 **************************************************************************/
#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK
#define PNAME "mod_async_preload_mop"
#define PRELOAD_BUF_SIZE 4096
#define OP_MAX_HIGH 4 
#define ASYNC_SINGLE 2
#define ASYNC_DOUBLE 3

#define DEBUG_NO 203  
extern void fwdStartExtend(int fd, StoreEntry * e, request_t * r, fde *fd_match);
static cc_module* mod = NULL; 
static long long g_MopConfigParamCount = 0;
static long long g_FlexiCachePreloadCont= 0;
static int g_FlexiCacheOpPrefetchLimit= 0;
static int g_FlexiCacheOpPrefetchCount= 0;
static time_t g_FlexiCacheOpDownTime = 0;
char delimit[] = "@@@";
char *opLevelStr[] = {"low", "medium", "high"};


typedef struct _mod_mop_config_t
{
    uint8_t op_level;
    uint8_t fetch;
}mop_config_t;

typedef struct _FlexiCachePreloadData_t
{
    request_t *request;
    StoreEntry *entry; 
    store_client *sc;
    squid_off_t offset; 
    short op_flags;
    struct timeval start;
}FcPreload_t;

CBDATA_TYPE(FcPreload_t);
char sPreloadBuf[PRELOAD_BUF_SIZE];


static mop_config_t * 
mopConfigAllocPool(void)
{
    mop_config_t * pm = xcalloc(sizeof(mop_config_t), 1);
    if (pm) g_MopConfigParamCount++;
    return pm;
}

static int 
mopConfigFreeData(void *data)
{
    mop_config_t * pm =  data;
    if (pm)
    {
        g_MopConfigParamCount--;
        debug(210, 2)("%s %s Config Param Count %lld\n",
                PNAME, __func__, g_MopConfigParamCount);
        safe_free(pm);
    }
    pm = data = NULL;
    return 1;
}

static inline void *
cc_get_mod_range_param(int fd)
{
    if(fd >= 0 && fd < SQUID_MAXFD && fd_table[fd].matched_mod_params)
        return cc_get_mod_param(fd, mod);
    return NULL;
}

static int 
mopParseParamDone(char *t, const char *p, int max, int min)
{
    if (NULL == t || NULL == p)
        return -1;
    int value = 0;
    int t_size = strlen(t);
    int p_size = strlen(p);
    if (t_size <= p_size)
        return -1;
    if (strncasecmp(t, p, p_size))
        return -1;
    t += p_size;
    value = atoi(t);

    if (value >= max)
        return -1;
    if (value < min)
        return -1;
    return value;
}

static int 
mopParseParamStart(int *level, int *method)
{
    char *t = strtok(NULL, w_space);
    char *p = strtok(NULL, w_space);
    *level = mopParseParamDone(t, "level:", OP_MAX_HIGH, 0); 

    *method = 0;
    if (p && !strcasecmp(p , "async_single")) *method = ASYNC_SINGLE;
    if (p && !strcasecmp(p , "async_double")) *method = ASYNC_DOUBLE;
    if (-1 == *level)
    {   
        return -1; 
    }  
    return 0;
}

static int 
hook_func_sys_parse_param(char *cfg_line)
{
    char *t = NULL;
    int op_level = 0;
    int op_method= 0;

    if(NULL == (t = strtok(cfg_line, w_space)) || strcmp(t, PNAME))
    {
        return -1;
    }
    if (mopParseParamStart(&op_level, &op_method))
    {
        return -1;
    }
    mop_config_t * cfg = mopConfigAllocPool();
    t= strtok(NULL, w_space);
    if (t && !strncasecmp(t, "TIMES=", strlen("TIMES=")))
    {
        g_FlexiCacheOpPrefetchLimit = atoi(t+ strlen("TIMES="));
        debug(DEBUG_NO, 1)("%s %s g_FlexiCacheOpPrefetchLimit = %d\n",
                PNAME, __func__, g_FlexiCacheOpPrefetchLimit);
    }
    cfg->op_level = op_level;
    cfg->fetch = op_method;
    debug(DEBUG_NO, 1)("%s %s level=%d method=%d\n",
            PNAME, __func__, op_level, op_method);
    cc_register_mod_param(mod, cfg, mopConfigFreeData);
    return 0;
}

static void 
mopStoreResetUrl(clientHttpRequest *http, char *key)
{
    if (key)
    {
        safe_free(http->request->store_url);
        http->request->store_url = xstrdup(key);
    }
}

static void
g_FlexiCacheAsyncFwdStart(clientHttpRequest *http, StoreEntry *e, request_t *r)
{
    request_t *http_request = http->request;
    http->request = requestLink(r);
    int fd = http->conn->fd;
    short int * cc_run_state = fd_table[http->conn->fd].cc_run_state;
    fd_table[fd].cc_run_state = NULL;
    Array * matched_mod_params = fd_table[fd].matched_mod_params;
    fd_table[fd].matched_mod_params = NULL;
    cc_client_acl_check(http, 0); 
    requestUnlink(http->request);
    http->request = http_request;

    debug(DEBUG_NO, 5)("%s %s fwdStart %s\n",PNAME,__func__,storeLookupUrl(e));
    fwdStartExtend(-1, e, r, &fd_table[http->conn->fd]);

    fde *F = &fd_table[fd];
    debug(DEBUG_NO, 5)("%s %s cc_run_state=%p matched_mod_params=%p %s\n",PNAME,__func__,
            F->cc_run_state, F->matched_mod_params, storeLookupUrl(e));
    if (F->cc_run_state)
    {
        memPoolFree(cc_run_state_pool, F->cc_run_state);
    }
    fd_table[fd].cc_run_state = cc_run_state;
    cc_run_state = NULL;

    if (F->matched_mod_params)
    {
        int i, j;
        cc_mod_param *mod_param = NULL;
        for (i = 0; i < Config.cc_all_mod_num; i++)
        {    
            for (j = 0; j < F->matched_mod_params[i].count; j++) 
            {        
                mod_param = (cc_mod_param*)F->matched_mod_params[i].items[j];
                mod_param->count--;     // dereference a fd from cc_mod_param
            }        
            arrayClean(&F->matched_mod_params[i]);
        }    
        memPoolFree(matched_mod_params_pool, F->matched_mod_params);
        F->matched_mod_params = NULL; 
        mod_param = NULL;
    }
    fd_table[fd].matched_mod_params = matched_mod_params;
    matched_mod_params = NULL;
}

static request_t * 
g_FlexiCacheAsyncCreatRequest(request_t *old_req, char *url)
{
    request_t * new_req = urlParse(old_req->method, url);
    if(new_req == NULL)
    {
        debug(DEBUG_NO, 3)("%s %s urlParse fail %s\n",
        PNAME, __func__, urlCanonical(old_req));
        return NULL;
    }
    httpHeaderAppend(&new_req->header, &old_req->header);
    httpHeaderDelById(&new_req->header, HDR_RANGE);
    httpHeaderDelById(&new_req->header, HDR_REQUEST_RANGE);
    httpHeaderDelById(&new_req->header, HDR_IF_RANGE);
    new_req->http_ver = old_req->http_ver;
    new_req->client_addr = old_req->client_addr;
    new_req->client_port = old_req->client_port;
#if FOLLOW_X_FORWARDED_FOR
    new_req->indirect_client_addr = old_req->indirect_client_addr;
#endif                                      //  FOLLOW_X_FORWARDED_FOR 
    new_req->my_addr = old_req->my_addr;
    new_req->my_port = old_req->my_port;
    new_req->range = NULL;
    new_req->flags = old_req->flags;
    new_req->flags.redirected = 1;
    new_req->flags.range = 0;
    new_req->flags.cachable = 1;
    new_req->lastmod = -1;
    new_req->etags = NULL;
    if (old_req->auth_user_request)
    {
        new_req->auth_user_request = old_req->auth_user_request;
        authenticateAuthUserRequestLock(new_req->auth_user_request);
    }
    if (old_req->body_reader)
    {
        new_req->body_reader = old_req->body_reader;
        new_req->body_reader_data = old_req->body_reader_data;
        old_req->body_reader = NULL;
        old_req->body_reader_data = NULL;
    }
    new_req->content_length = old_req->content_length;
    if (strBuf(old_req->extacl_log))
        new_req->extacl_log = stringDup(&old_req->extacl_log);
    if (old_req->extacl_user)
        new_req->extacl_user = xstrdup(old_req->extacl_user);
    if (old_req->extacl_passwd)
        new_req->extacl_passwd = xstrdup(old_req->extacl_passwd);
    if(old_req->store_url)
        new_req->store_url = xstrdup(old_req->store_url);
    else
        new_req->store_url = xstrdup(url);
    debug(DEBUG_NO, 3)("%s %s for preload %s\n", PNAME, __func__, urlCanonical(new_req));
    return new_req;
}

static void 
g_FlexiCacheAsyncAccessLog(FcPreload_t * fword)
{
    AccessLogEntry al;
    static aclCheck_t *ch; 
    MemObject *mem = fword->entry->mem_obj;
    request_t *request = fword->request;
    memset(&al, 0, sizeof(al));
    al.icp.opcode = ICP_INVALID;

    al.url = mem->url;
    if (mem->store_url) al.url = mem->store_url;
    debug(DEBUG_NO, 9) ("%s : url='%s'\n",__func__, al.url);
    al.http.code = mem->reply->sline.status;
    al.http.content_type = strBuf(mem->reply->content_type);
    al.cache.size = fword->offset;
    al.cache.code = LOG_TCP_ASYNC_MISS;
    al.cache.msec = tvSubMsec(fword->start, current_time);; 
    if (Config.onoff.log_mime_hdrs)
    {
        Packer p;
        MemBuf mb;
        memBufDefInit(&mb);
        packerToMemInit(&p, &mb);
        httpHeaderPackInto(&request->header, &p);
        al.headers.request = xstrdup(mb.buf);
        packerClean(&p);
        memBufClean(&mb);
    }
    al.http.method = request->method;
    al.http.version = request->http_ver;
    al.hier = request->hier;
    if (request->auth_user_request)
    {
        if (authenticateUserRequestUsername(request->auth_user_request))
            al.cache.authuser = xstrdup(authenticateUserRequestUsername(request->auth_user_request));
        authenticateAuthUserRequestUnlock(request->auth_user_request);
        request->auth_user_request = NULL;
    }
    else if (request->extacl_user)
    {
        al.cache.authuser = xstrdup(request->extacl_user);
    }
    al.request = request;
    al.reply = mem->reply;
    ch = aclChecklistCreate(Config.accessList.http, request, NULL);
    ch->reply = mem->reply;
    if (!Config.accessList.log || aclCheckFast(Config.accessList.log, ch))
        accessLogLog(&al, ch);
    aclChecklistFree(ch);
    safe_free(al.headers.request);
    safe_free(al.headers.reply);
    safe_free(al.cache.authuser);
}

static void 
g_FlexiCacheAsyncComplete(FcPreload_t * fword)
{
    storeResumeRead(fword->entry);
    g_FlexiCacheAsyncAccessLog(fword);
    requestUnlink(fword->request);
    storeClientUnregister(fword->sc, fword->entry, fword);
    storeUnlockObject(fword->entry);
    cbdataFree(fword);
    g_FlexiCachePreloadCont--;
    debug(DEBUG_NO, 4)("%s %s FcPreload_t=%lld\n", PNAME, __func__, g_FlexiCachePreloadCont);
}

static void
g_FlexiCacheAsyncHandleReply(void *data, char *buf, ssize_t size)
{
    FcPreload_t* fword = data;
    StoreEntry *e = fword->entry;
    HttpReply *rep = storeEntryReply(e);
    int f_cachable = 1;
    if (f_cachable && (rep->sline.status != 200 || !EBIT_TEST(e->flags, ENTRY_CACHABLE)))
    {
        debug(DEBUG_NO, 5)("%s %s no_cache status != 200 or cachable=0 %s\n",
                PNAME, __func__, storeLookupUrl(fword->entry));
        f_cachable = 0;
    }
    if(f_cachable && (EBIT_TEST(e->flags, ENTRY_ABORTED) || EBIT_TEST(e->flags, RELEASE_REQUEST)))
    {
        debug(DEBUG_NO, 5)("%s %s no_cache Abort or Release %s\n",
                PNAME, __func__, storeLookupUrl(fword->entry));
        f_cachable = 0;
    }
    fword->offset = e->mem_obj->inmem_hi;
    String XXOP = httpHeaderGetByName(&rep->header, "X-Original-Content-Length");
    debug(DEBUG_NO, 5)("%s %s OP-Flags=%d status=%d Nginx Fail XXOP=%s %s\n",
		    PNAME, __func__, fword->op_flags, rep->sline.status, XXOP.buf, storeLookupUrl(fword->entry));
    if (fword->op_flags)
    {
        g_FlexiCacheOpPrefetchCount--;
        if (200 != rep->sline.status || NULL == XXOP.buf)
        {
            if (NULL == XXOP.buf) g_FlexiCacheOpDownTime = squid_curtime + 10;
            debug(DEBUG_NO, 5)("%s %s Nginx Fail g_FlexiCacheOpDownTime=%ld %s\n",
                PNAME, __func__, (long)g_FlexiCacheOpDownTime, storeLookupUrl(fword->entry));
            f_cachable = 0;
        }
    }
    stringClean(&XXOP);
    debug(DEBUG_NO, 5)("%s %s f_cachable=%d %s\n",
		    PNAME, __func__, f_cachable, storeLookupUrl(fword->entry));
    if (0 == f_cachable)
    {
        storeReleaseRequest(e);
    }
    g_FlexiCacheAsyncComplete(fword);
}

static int
g_FlexiCacheAsyncDone(clientHttpRequest *http, request_t *req, int op)
{
    char *url = (char *)urlCanonical(req);
    CBDATA_INIT_TYPE(FcPreload_t);
    FcPreload_t *fword = cbdataAlloc(FcPreload_t);
    g_FlexiCachePreloadCont++;

    if ((fword->op_flags = op)) g_FlexiCacheOpPrefetchCount++;
    fword->start = current_time;
    fword->request = requestLink(req);
    StoreEntry *entry = storeCreateEntry(url, req->flags, req->method);
    if (req->store_url)
        storeEntrySetStoreUrl(entry, req->store_url);
    debug(DEBUG_NO, 3)("%s %s store_url %s\n", PNAME, __func__, req->store_url);
    if(Config.onoff.collapsed_forwarding)
    {
        EBIT_SET(entry->flags, KEY_EARLY_PUBLIC);
        storeSetPublicKey(entry);
    }
    fword->offset = 0;
    fword->sc = storeClientRegister((fword->entry = entry), fword);
    entry->mem_obj->refresh_timestamp = squid_curtime;
    g_FlexiCacheAsyncFwdStart(http, fword->entry, fword->request);
    storeClientCopy(fword->sc, fword->entry, fword->offset, fword->offset, PRELOAD_BUF_SIZE, sPreloadBuf,
            g_FlexiCacheAsyncHandleReply, fword);
    return 0;
}


static int
g_FlexiCacheAsyncStart(clientHttpRequest *http, const char *store_url, int op)
{
    char *uri = NULL;
    if(urlCanonical(http->request))
    {
        uri = xstrdup(urlCanonical(http->request));
    }
    if (NULL == uri && http->uri)
        uri = xstrdup(http->uri);
    if(NULL == uri) return 0;
    request_t *req = g_FlexiCacheAsyncCreatRequest(http->request, uri);
    safe_free(uri);
    if(NULL == req)
    {
        return  0; 
    }

    if (store_url)
    {   
        safe_free(req->store_url);
        req->store_url = xstrdup(store_url);
    }  
    g_FlexiCacheAsyncDone(http, req, op);
    return 0;
}

static int 
hookMopTryAsyncPreloadStart(clientHttpRequest *http)
{
    const char *url = urlCanonical(http->request);
    if (http->request->store_url) url = http->request->store_url;
    mop_config_t * cfg = cc_get_mod_range_param(http->conn->fd);

    if (METHOD_GET != http->request->method || NULL == cfg ||NULL == url)
    {
        debug(DEBUG_NO, 3)("%s %s method=%s cfg=%p %s\n",PNAME, __func__,
                RequestMethods[http->request->method].str,cfg, http->uri);
        return 0;
    }

    StoreEntry *n_e = NULL;
    StoreEntry *o_e = NULL;
    int p_size = 0;
    int p_slot = 0;
	int i_loop = 0;
    int p_original_obj = 0;
    String key   = StringNull;
	char cname[25]; 
	char *h= NULL;
	char *p= NULL;
    stringReset(&key, url);
    n_e = storeGetPublic(key.buf, http->request->method);
    p_size = key.len;
    if (cfg && cfg->op_level < OP_MAX_HIGH && cfg->op_level > 0)
    {
        p_slot = cfg->op_level -1;
        stringAppend(&key, delimit, strlen(delimit));
        stringAppend(&key, opLevelStr[p_slot], sizeof(opLevelStr[p_slot])); 
    }
    if (cfg && cfg->fetch)
    n_e = storeGetPublic(url, http->request->method);
    if (n_e && 0 == storeEntryValidToSend(n_e))
        n_e = NULL;
    o_e = storeGetPublic(key.buf, http->request->method);
    HttpReply *rep = storeEntryReply(o_e);
    if (o_e && 0 == storeEntryValidToSend(o_e))
        o_e = NULL;
    mopStoreResetUrl(http, key.buf);
    if (cfg && NULL == o_e && (n_e || ASYNC_DOUBLE == cfg->fetch))
    {
	    debug(DEBUG_NO, 5)("%s %s g_FlexiCacheOpPrefetchLimit=%d g_FlexiCacheOpPrefetchCount=%d g_FlexiCacheOpDownTime = %ld\n", 
                PNAME, __func__, g_FlexiCacheOpPrefetchLimit, g_FlexiCacheOpPrefetchCount,(long)g_FlexiCacheOpDownTime);
        if (g_FlexiCacheOpDownTime <= squid_curtime && 
                (g_FlexiCacheOpPrefetchLimit <= 0 ||  
                g_FlexiCacheOpPrefetchCount < g_FlexiCacheOpPrefetchLimit))
        {
            debug(DEBUG_NO, 3)("%s %s OP Fetch Start add header %s\n",
                    PNAME, __func__, http->uri);
		
            httpHeaderAddEntry(&http->request->header, httpHeaderEntryCreate(HDR_X_CACHE, NULL, "CC-Need-Process"));
			memset(cname,'\0',25);
			h = inet_ntoa(http->request->my_addr);
			while(*h !='\0')
			{
				cname[i_loop++]=*h;
				h++;
			}
			
			cname[i_loop++]=':';
			
			p = (char*)xitoa(http->request->port);
			while(*p!='\0')
			{
				cname[i_loop++]=*p;
				p++;
			}
			
			httpHeaderAddEntry(&http->request->header, httpHeaderEntryCreate(HDR_OTHER, "X-CC-Origin", cname));
			//httpHeaderAddEntry(&http->request->header, httpHeaderEntryCreate(HDR_OTHER, "X-CC-Origin", xitoa(http->request->port)));
            g_FlexiCacheAsyncStart(http, NULL, 1); 
            httpHeaderDelById(&http->request->header, HDR_X_CACHE);
        }
        p_original_obj = 1;
    }

    key.buf[p_size] = '\0';
    rep = storeEntryReply(o_e);
    if (o_e && o_e->store_status == STORE_PENDING && (NULL == rep || 200 != rep->sline.status))
    {
        p_original_obj = 1;
        debug(DEBUG_NO, 3)("%s %s %d o_e=%p is NULL OR is status =%d %s\n",
			PNAME, __func__,__LINE__, rep, rep?rep->sline.status: -1, http->uri);
        o_e = NULL;
    }
    /*
    if (cfg && NULL == n_e && (ASYNC_DOUBLE == cfg->fetch || ASYNC_SINGLE == cfg->fetch))
    {
        p_original_obj = 1;
        debug(DEBUG_NO, 3)("%s %s NOP Fetch Start %s\n",PNAME, __func__, http->uri);
        g_FlexiCacheAsyncStart(http, url, 0); 
    }
    */
    if (NULL == n_e || 1 == p_original_obj)
    {
        mopStoreResetUrl(http, key.buf);
    }
    debug(DEBUG_NO, 9)("%s %s StoreUrl %s\n",PNAME, __func__, http->request->store_url);
    stringClean(&key);
    return 0;
}


int mod_register(cc_module *module)
{
    debug(DEBUG_NO, 1)("(%s) ->  init: init module\n", PNAME);

    strcpy(module->version, "7.0.R.16488.x86_64");
    //modules frame
    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
            hook_func_sys_parse_param);

    cc_register_hook_handler(HPIDX_hook_func_before_clientProcessRequest2,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_clientProcessRequest2), 
            hookMopTryAsyncPreloadStart);

    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else
        mod = module;
    return 0;
}
#undef PRELOAD_BUF_SIZE
#undef PNAME
#undef DEBUG_NO 
#undef OP_MAX_HIGH
#undef ASYNC_SINGLE 
#undef ASYNC_DOUBLE 
#endif

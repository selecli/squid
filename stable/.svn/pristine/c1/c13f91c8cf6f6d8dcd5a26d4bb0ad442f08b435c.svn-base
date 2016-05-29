/**************************************************************************
 *	Copyright (C) 2012
 *
 *	File ID:
 *	File name: mod_async_load.c
 *	Summary:
 *	Description: Use to fetch complete object by client touching
 *  Configure: on|off
 *	Version: V1.0
 *	Author: Yandong.Nian
 *	Date: 2013-03-04
 **************************************************************************/
#include "cc_framework_api.h"
#include <time.h>
#include <stdbool.h>
#include "protos.h"
#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;
static char holdvideo = 0, flagsrange = 0,  parseparam = 1;
static  squid_off_t prConfigOffset = 102400;
static int serviceLoadStart(request_t *request, char *uri, int flags, clientHttpRequest *owner);
/*type: 0 range and delete header; 1 == video, 3 == 302 start */
static int checkHeaderExist(request_t *r, int type); 
static void fwdStartBefore(request_t *r,char *name, char * value);
long ll_count = 0;

/*
 * asyncPreLoadDate For Async to Service's Structs
 */
typedef struct _asyncPreLoadDate
{
	request_t *request;
	StoreEntry *entry; 
	store_client *sc;
	squid_off_t offset; 
	size_t buf_in_use;
	char readbuf[STORE_CLIENT_BUF_SZ];
	char start;
	int clientfd; 
	clientHttpRequest *owner;
}asyncPreLoadDate;
CBDATA_TYPE(asyncPreLoadDate);

static int free_parseparam(void *data)
{
	return 0;
}

static int
sys_func_parse_param(char *cfg_line)
{
	char *token = NULL, *range = NULL, *video = NULL;
	if( NULL == strstr(cfg_line, "on") && NULL == strstr(cfg_line, "off"))
		return -1;
	range = strstr(cfg_line, "ignore_range");
	video = strstr(cfg_line, "hold_video");
	if((token = strtok(cfg_line, w_space)) == NULL || strcmp(token, "mod_async_load"))
		return -1;
	if((token = strtok(NULL, w_space)) == NULL)
		return -1;
    prConfigOffset = strto_off_t(token, NULL, 10);
    flagsrange = range ? 1 : 0;
    holdvideo = video ? 1 : 0;
    debug(203, 1)("(mod_async_load) func_parse_param[ prConfigOffset %ld; Flagsrange %d; Holdvideo %d ]\n",(long)prConfigOffset, flagsrange, holdvideo);
    cc_register_mod_param(mod, &parseparam, free_parseparam);
    return 0;
}

static int clientCheckHitStart(clientHttpRequest * http)
{
    if(0 == http->redirect.status && flagsrange && http->entry && http->request->range)
    {
        if (http->request->flags.nocache)
        {
            http->entry = NULL;
            debug(203, 5)("mod_async_load request nocache [Set CLIENT_REFRESH_MISS] %s\n", http->uri);
            return LOG_TCP_CLIENT_REFRESH_MISS;
        }

        if (httpHdrRangeWillBeComplex(http->request->range)) 
        {
            http->request->flags.cachable = 0;
            http->entry = NULL;
            debug(203, 5)("mod_async_load Complex Range [Set MISS] %s\n", http->uri);
            return LOG_TCP_MISS;
        }
        
        if (STORE_OK == http->entry->store_status )  return LOG_TCP_HIT;
            
        squid_off_t  min_offset = httpHdrRangeFirstOffset(http->request->range);
        squid_off_t  cur_offset = http->entry->mem_obj ? http->entry->mem_obj->inmem_hi : 0;
        char check = 0;
        if (min_offset >= 0 && (min_offset <= cur_offset + prConfigOffset))
            check = 1;

        debug(203, 5)("mod_async_load [Set %s] %s\n",check ? "HIT" : "MISS", http->uri);
        debug(203, 7)("mod_async_load [Set %s] %ld %s [%ld] [%ld]\n",check ? "HIT" : "MISS",(long)min_offset, check ? "<=": ">", (long)cur_offset, (long)prConfigOffset); 
        
        if(check) return LOG_TCP_HIT;

        http->request->flags.cachable = 0;
		http->entry = NULL;
		return LOG_TCP_MISS;
	}
	return 0;
}

 
static int 
service_fd_acl_check(asyncPreLoadDate *fword, int flags) 
{
	if(0 == flags)
		fwdStartBefore(fword->request, "Async-Load", "Free-Async-Whole");
	else if(fword->owner || fword->start || 1 == flags || 3 == flags)
		fwdStartBefore(fword->request, "Async-Load", "Hold-Async-Video");
	else if(2 == flags)
		fwdStartBefore(fword->request, "Async-Load", "Free-Async-Video");

	ConnStateData *connState = cbdataAlloc(ConnStateData);
	clientHttpRequest *http = cbdataAlloc(clientHttpRequest); 
	http->request = requestLink(fword->request);
	http->conn = connState;
	http->conn->fd = fword->clientfd;
	cc_client_acl_check(http, 0);
	http->conn->fd =  0;
	http->conn = NULL;
	requestUnlink(http->request);
	http->request = NULL; 
	cbdataFree(connState);
	cbdataFree(http);
	return 0;
}

static request_t * parseNewRequest(request_t *old_request, char *url)
{
	debug(203, 3)("Async Fetch parseNewRequest\n");    
	request_t * new_request = urlParse(old_request->method, url);
	if(new_request == NULL)
	{
		debug(203, 3)("mod_async_load parseNewRequest urlParse fail\n");
		return NULL;
	}
	httpHeaderAppend(&new_request->header, &old_request->header);
	httpHeaderDelById(&new_request->header, HDR_RANGE);
	httpHeaderDelById(&new_request->header, HDR_REQUEST_RANGE);
	httpHeaderDelById(&new_request->header, HDR_IF_RANGE);
	new_request->http_ver = old_request->http_ver;
	new_request->client_addr = old_request->client_addr;
	new_request->client_port = old_request->client_port;
#if FOLLOW_X_FORWARDED_FOR
	new_request->indirect_client_addr = old_request->indirect_client_addr;
#endif										//  FOLLOW_X_FORWARDED_FOR 
	new_request->my_addr = old_request->my_addr;
	new_request->my_port = old_request->my_port;
	new_request->range = NULL;
	new_request->flags = old_request->flags;
	new_request->flags.redirected = 1;
	new_request->flags.range = 0;
	new_request->flags.cachable = 1;
	new_request->lastmod = -1;
	new_request->etags = NULL;
	if (old_request->auth_user_request)
	{
		new_request->auth_user_request = old_request->auth_user_request;
		authenticateAuthUserRequestLock(new_request->auth_user_request);
	}
	if (old_request->body_reader)
	{
		new_request->body_reader = old_request->body_reader;
		new_request->body_reader_data = old_request->body_reader_data;
		old_request->body_reader = NULL;
		old_request->body_reader_data = NULL;
	}
	new_request->content_length = old_request->content_length;
	if (strBuf(old_request->extacl_log))
		new_request->extacl_log = stringDup(&old_request->extacl_log);
	if (old_request->extacl_user)
		new_request->extacl_user = xstrdup(old_request->extacl_user);
	if (old_request->extacl_passwd)
		new_request->extacl_passwd = xstrdup(old_request->extacl_passwd);
	if(old_request->store_url)
		new_request->store_url = xstrdup(old_request->store_url);
	else
	{
		char *url = (char *)urlCanonical(old_request);
		new_request->store_url = url ? xstrdup(url): NULL;
	}
	return new_request;
}


static void clientForwordDone(asyncPreLoadDate *fword)
{
    fd_close(fword->clientfd);
    debug(203, 5)("mod_async_load Complete Load uri %s\n", urlCanonical(fword->request));
	requestUnlink(fword->request);
	storeClientUnregister(fword->sc, fword->entry, fword);
	storeUnlockObject(fword->entry);
    clientHttpRequest * http = fword->owner;	
    fword->owner = NULL;
    cbdataFree(fword);
    ll_count--;
    debug(203, 4)("mod_async_load asyncPreLoadDate Count : %ld\n",ll_count);
    if(http)
	{
		debug(203, 3)("async load Miss_Wait Start %s\n", http->uri);
		if(http->request) clientProcessRequest(http);
		cbdataUnlock(http);
	}
}

static void 
HandleForwordReply(void *data, char *buf, ssize_t size)
{
	asyncPreLoadDate* fword = data;
	MemObject *mem = fword->entry->mem_obj;
	if (mem->reply->sline.status != 200 || size <= 0 || EBIT_TEST(fword->entry->flags, ENTRY_ABORTED)) 
	{
		clientForwordDone(fword);
		return; 
	}
	if(NULL == fword->owner && !fword->start && mem->reply->hdr_sz && mem->reply->hdr_sz < mem->inmem_hi)
	{
		clientForwordDone(fword);
		return;
	}
	fword->offset += size;
	storeClientCopy(fword->sc, fword->entry, fword->offset, fword->offset, STORE_CLIENT_BUF_SZ, fword->readbuf, 
			HandleForwordReply, fword);
}
/* ret : -1 --> Range; 1 --> miss_wait; 2 -->miss_start*/

static int 
AsyncToServiceLoadStart(request_t *data, int *ret)
{
	if(NULL == ret || NULL == data)
		return 1;
	char * uri = NULL;
	request_t *request= NULL;
	if(*ret == -1)
	{
		request = data;
		if(!flagsrange|| NULL == (uri = (char *)urlCanonical(request)))
			return (*ret =1);
        debug(203, 3)("async load is Range %s\n", uri);
		*ret = serviceLoadStart(request, uri, 0, NULL);
		return *ret;
	}
	clientHttpRequest *http = (clientHttpRequest *)data;
	if(NULL == (request = http->request) || NULL == (uri = http->uri))
		return 1;

	if(1 == *ret){
		debug(203, 3)("async load is Miss_Wait %s\n", uri);
		request->flags.nocache = 0;
        request->flags.cachable = 1;
        return serviceLoadStart(request, uri, *ret, http);
	}

	if(2 == *ret || 3 == *ret){
		httpHeaderDelById(&request->header, HDR_RANGE);
		httpHeaderDelById(&request->header, HDR_REQUEST_RANGE);
		if(request->range)
		{       
			httpHdrRangeDestroy(request->range);
			request->range = NULL; 
			request->flags.range = 0;
		}  
		debug(203, 3)("async load is Miss_Start %s\n", uri);
		serviceLoadStart(request, uri, *ret, NULL);
		return 0;
	}
	return 1;
}

static int 
serviceLoadStart(request_t *request, char *uri, int flags, clientHttpRequest *owner)
{
	StoreEntry *entry = NULL;
	request_t *req = NULL;
    if(request->store_url && NULL == (entry = storeGetPublic(request->store_url,METHOD_GET)))
		entry = storeGetPublicByRequest(request);

	if(entry || NULL == (req = parseNewRequest(request, uri)))
	{
		debug(203, 3)("mod_async_load %s\n", entry ? "StoreEntry is exist!": "parseNewRequest Fail");
		return entry ? 0 : 1;
	}

	int fd = -1, i = 0;
	while ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 && ++i <= 5);
	if(fd < 0)
	{
		debug(203, 3)("forward fail %s\n", uri);
		requestUnlink(req);
		return 1;
	}
	fd_open(fd, FD_SOCKET, "Async forward");
    CBDATA_INIT_TYPE(asyncPreLoadDate);
    asyncPreLoadDate *fword = cbdataAlloc(asyncPreLoadDate);
    ll_count++;
    fword->clientfd= fd;
    fword->start = (holdvideo && flags);
	debug(203, 4)("mod_async_load flas %d fword->start %d owner is %s\n",flags, fword->start, owner ? "Exist": "NULL");
	fword->request = requestLink(req);
	owner ? cbdataLock(fword->owner = owner) : NULL;

	service_fd_acl_check(fword, flags);

	entry = storeCreateEntry(uri, req->flags, req->method);

    if (request->store_url)
        storeEntrySetStoreUrl(entry, request->store_url);
	if(Config.onoff.collapsed_forwarding)
	{
		EBIT_SET(entry->flags, KEY_EARLY_PUBLIC);
		storeSetPublicKey(entry);
	}

	fword->sc = storeClientRegister((fword->entry = entry), fword);
	fwdStart(fword->clientfd, fword->entry, fword->request);
	storeClientCopy(fword->sc, fword->entry, fword->offset, fword->offset, STORE_CLIENT_BUF_SZ, fword->readbuf,
			HandleForwordReply, fword);
	return 0; 
}

static int 
checkHeaderExist(request_t *r, int type)
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *e;
	if (r == NULL ||!CBIT_TEST(r->header.mask, HDR_OTHER))
		return 0;
	while ((e = httpHeaderGetEntry(&r->header, &pos)))
	{
		if(HDR_OTHER == e->id && !type && !strcmp(strBuf(e->name), "Fetched"))
			return 1;
		if(HDR_OTHER == e->id && type && !strcmp(strBuf(e->name), "Async-Load"))
		{	
			if(type == 1) 
				return 1;
			if(type == 2)
			{
				httpHeaderDelByName(&r->header, strBuf(e->name));
				return 1;
			}
		}
	}
	return 0;
}
static void fwdStartBefore(request_t *r, char *name, char * value)
{
	HttpHeaderEntry eEntry; 
	eEntry.id = HDR_OTHER;
	stringInit(&eEntry.name, name); 
	debug(203, 4)("mod_async_load add Header [%s] [%s] %s\n",name, value, urlCanonical(r));	
	stringInit(&eEntry.value, value);
	httpHeaderAddEntry(&r->header, httpHeaderEntryClone(&eEntry));
	stringClean(&eEntry.name);
	stringClean(&eEntry.value);
}

static int set_range_request_cachable(clientHttpRequest * http)
{
	if(http && http->request && http->request->range && flagsrange && http->request->flags.cachable)
	{
		int fd = http->conn->fd;
		http->request->flags.cachable = 0;
		cc_register_mod_private_data(FDE_PRIVATE_DATA, &fd, &parseparam , free_parseparam, mod);
	}
	return 0;
}

static void hook_change_request(request_t *request, FwdState *fwdState)
{
	int clientfd = fwdState->client_fd;
    if(request->range && !request->flags.cachable && flagsrange&& NULL != cc_get_mod_private_data(FDE_PRIVATE_DATA, &clientfd, mod))
		request->flags.cachable = 1;
}

static void async_quick_abort_min_before(StoreEntry * entry, int *abort)
{
	request_t *r = entry->mem_obj->request;
    if(0 == checkHeaderExist(r, 1))
        return;
    *abort = 0;
    if(entry->mem_obj->store_url)
    {
        debug(203, 4)("Entry Store_url %s is async StoreEntry\n",entry->mem_obj->store_url);
        return;
    }
    if(entry->mem_obj->url)
    {
        debug(203, 4)("Entry url %s is async StoreEntry\n",entry->mem_obj->url);
        return ;
    }
    debug(203, 4)("Entry hash.key %s is async StoreEntry\n",entry->hash.key? storeKeyText(entry->hash.key):"exist");
    return ;
}

static int 
private_clientBuildRangeHeader(clientHttpRequest *http, HttpReply * rep)
{
    if(NULL == http || NULL == http->entry)
        return 0;
    if(rep && rep->sline.status == HTTP_OK && rep->content_length < 0)
    {
        if ((rep->content_length = httpHeaderGetSize(&rep->header, HDR_CONTENT_LENGTH)) < 0)
            rep->content_length = contentLen(http->entry);
        if (rep->content_length < 0 && http->entry->mem_obj && http->entry->mem_obj->reply)    
            rep->content_length = http->entry->mem_obj->inmem_hi - http->entry->mem_obj->reply->hdr_sz; 
    }

    if(http->entry->mem_obj && http->entry->mem_obj->reply && http->entry->mem_obj->reply->content_length < 0)
    {
        HttpReply * reply = http->entry->mem_obj->reply;
        if((reply->content_length = httpHeaderGetSize(&reply->header, HDR_CONTENT_LENGTH)) < 0)
            reply->content_length = contentLen(http->entry);
        if(reply->content_length < 0 )
            reply->content_length = http->entry->mem_obj->inmem_hi - reply->hdr_sz; 
    }

    if(rep && rep->sline.status == HTTP_OK && 0 == http->request->flags.cachable)
        http->request->flags.cachable = 1;
    return 0;
}

static int 
DecreaseRegisetCount(StoreEntry * e)
{
    request_t *r = e->mem_obj->request;
    int head_has = checkHeaderExist(r, 2);
    if(!head_has)
        return 0;
    HttpReply * reply = e->mem_obj->reply;
    if(reply->content_length < 0 && reply->sline.status == HTTP_OK)
    {
        if((reply->content_length = httpHeaderGetSize(&reply->header, HDR_CONTENT_LENGTH)) < 0)
            reply->content_length = contentLen(e);
        if(reply->content_length < 0)
            reply->content_length = e->mem_obj->inmem_hi - e->mem_obj->reply->hdr_sz;
    }
    return 0;
}

int mod_register(cc_module *module)
{
	debug(203, 1)("mod_async_load init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			sys_func_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_httpHdrRangeOffsetLimit,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_httpHdrRangeOffsetLimit),
			AsyncToServiceLoadStart);
	
	cc_register_hook_handler(HPIDX_hook_func_quick_abort_check,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_quick_abort_check),
			async_quick_abort_min_before); 

	cc_register_hook_handler(HPIDX_hook_func_private_process_miss_in_error_page,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_process_miss_in_error_page),
			set_range_request_cachable);

	cc_register_hook_handler(HPIDX_hook_func_private_preload_change_fwdState,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_preload_change_fwdState),
			hook_change_request);

	cc_register_hook_handler(HPIDX_hook_func_private_http_req_process2,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_req_process2),
			clientCheckHitStart);
    
    cc_register_hook_handler(HPIDX_hook_func_store_complete,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_store_complete),
            DecreaseRegisetCount);

    cc_register_hook_handler(HPIDX_hook_func_private_clientBuildRangeHeader,
            module->slot,
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_clientBuildRangeHeader),
            private_clientBuildRangeHeader);
    
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif


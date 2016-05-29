/**************************************************************************
 *   Copyright (C) 2013
 *
 *   File ID:
 *   File name: mod_range.c
 *   Summary:
 *   Description: 
 *  	1.Use to Response Error Range Request;
 *		2.Use to Parse Private Range from Url; 
 *		3.Use to Support Complex Range Request; 
 *		4.Use to Range Request Preload whole Object; 
 *		5.Use to Store 206 Response Object; 
 *   Configure: Match  ACL
 *   Version: V2.0
 *   Author: Yandong.Nian
 *   Date: 2013-03-04
 *   LastModified: 2013-10-18/Yandong.Nian;
 **************************************************************************/
#include "cc_framework_api.h"
#ifdef CC_FRAMEWORK
typedef enum 
{
    NONU,
    LETV 
}TYPE;

typedef struct _mod_config
{
    struct {                    //parse range from url
        char *prStart;          //range start keyword
        char *prEnd;            //range end keyword
        char sensitive;          //Upper and lower sensitive
    }url_range;

    struct {                    //preload whole object
        char fast;              //fast store preload;
        char preload;           //make flags if config preload
        squid_off_t offset;     //preload limit offset 
    }whole;        
    char store;                 //store 206 response
    char sup_complex;           //support complex range request
    http_status rep_status;     //response client Error http status 416
    TYPE channel;                //levt [leshi];
}mod_config;

typedef struct _mod_range_preload_data
{
    request_t *request;
    StoreEntry *entry; 
    store_client *sc;
    squid_off_t offset; 
    int clientfd; 
    char fast;
}mod_range_preload_data;
CBDATA_TYPE(mod_range_preload_data);

static cc_module* mod = NULL; 
static MemPool * mod_config_pool = NULL;
static long long ll_count = 0;
char delimit[] = "@@@";
char store_buff[STORE_CLIENT_BUF_SZ];
static int part_slot = -1;
int version_size = 2;
static void * 
mod_config_pool_alloc(void)
{
    void * obj = NULL;
    if (NULL == mod_config_pool)
    {
        mod_config_pool = memPoolCreate("mod_range config_struct", sizeof(mod_config));
    }
    return obj = memPoolAlloc(mod_config_pool);
}

static int
prConfig_free_callback(void *data)
{
    mod_config * prConfig = data; 
    if(NULL != prConfig) 
    {
        safe_free(prConfig->url_range.prStart);
        safe_free(prConfig->url_range.prEnd);
        memPoolFree(mod_config_pool, prConfig);
        prConfig = NULL;
    }
    return 0;
}

static int hook_init()
{
    int i = 0; 
    cc_module* mod_s =  NULL;
    for(i = 0; i < cc_modules.count; i++) 
    {    
        mod_s = cc_modules.items[i];
        if(mod_s->flags.param && mod_s->has_acl_rules && !strcmp(mod_s->name, "mod_partition_store"))
        {    
            part_slot = i; 
            break;
        }    
    }    
    String ver = StringNull;
    stringInit(&ver, mod->version);
    char *ver1 = NULL;
    if((ver1 = strrchr(strBuf(ver), '.')))
    {
        ver.buf[ver1 - ver.buf] = '\0';
        if((ver1 = strrchr(strBuf(ver), '.')))
        version_size = ver1 - ver.buf;
    }
    debug(170, 2)("mod_range %s version_size  %d part_slot %d\n", strBuf(ver), version_size, part_slot);
    stringClean(&ver);
    return 0;
}

static int 
hook_cleanup(cc_module *module)
{
    debug(170, 2)("(mod_range) -> hook_cleanup:\n");
    if(NULL != mod_config_pool)
    {
        memPoolDestroy(mod_config_pool);
        mod_config_pool = NULL;
    }
    return 0;
}
static inline void *
cc_get_mod_range_param(int fd)
{
    if(fd >= 0 && fd < SQUID_MAXFD && fd_table[fd].matched_mod_params)
        return cc_get_mod_param(fd, mod);
    return NULL;

}
static int 
parse_url_range(mod_config* prConfig, String *sConfig, int *parse_count)
{
    char * prStart = NULL, *prEnd = NULL;
    prConfig->url_range.sensitive = 1;
    if((prStart =strtok(NULL, w_space)) && !strcasecmp(prStart, "-i"))
    {
        prConfig->url_range.sensitive = 0;
        *parse_count = *parse_count + 1;
        stringAppend(sConfig," ", 1); 
        stringAppend(sConfig, prStart, strlen(prStart));
        prStart = strtok(NULL, w_space);
    }
    
    if(prStart == NULL || strlen(prStart) <= 0 || (prEnd = strtok(NULL, w_space)) == NULL || strlen(prEnd) <= 0)
    {
        debug(170, 1)("(mod_range) -> parse url_range Error [prStart : %s] [prEnd : %s]\n", prStart, prEnd);
        return 1;
    }
    *parse_count = *parse_count + 2;
    safe_free(prConfig->url_range.prStart);
    safe_free(prConfig->url_range.prEnd);
    prConfig->rep_status = 416;
    stringAppend(sConfig," ", 1);
    prConfig->url_range.prStart = xmalloc(strlen(prStart) + 1);
    strncpy(prConfig->url_range.prStart, prStart, strlen(prStart));
    prConfig->url_range.prStart[strlen(prStart)] = '\0';
    stringAppend(sConfig, prStart, strlen(prStart));

    stringAppend(sConfig," ", 1);
    stringAppend(sConfig, prEnd, strlen(prEnd));
    prConfig->url_range.prEnd =  xmalloc(strlen(prEnd) + 1);
    strncpy(prConfig->url_range.prEnd, prEnd, strlen(prEnd));
    prConfig->url_range.prEnd[strlen(prEnd)] = '\0';
    return 0;
}

static int 
hook_func_sys_parse_param(char *cfg_line)
{
    TYPE channel = NONU;
    if(strstr(cfg_line, "letv_channel"))
    {
        channel = LETV;
    }
    char *token = NULL;
    if((token = strtok(cfg_line, w_space)) == NULL || strcmp(token, "mod_range"))
    {
        debug(170,0)("(mod_range): func_sys_parse_param error,mod_name:[%s]\n", token);
        return -1;
    }
    mod_config* prConfig = mod_config_pool_alloc();
    memset(prConfig, 0 , sizeof(mod_config));
    prConfig->rep_status = 200;
    prConfig->sup_complex = 1;
    String sConfig  = StringNull;
    strCat(sConfig, "mod_range");
    int occurError = 0, parse_count = 0;
    while(0 == occurError && (token = strtok(NULL, w_space)))
    {
        strCat(sConfig, " ");
        strCat(sConfig, token);
        if(!strcmp(token,"allow") || !strcmp(token, "deny")) 
          break;

        if(!strcmp(token, "store_range") && ++parse_count)
            prConfig->store = 1;
        else if (!strncmp(token, "no_fast", strlen("no_fast")) && ++parse_count)
            prConfig->whole.fast = 0; 
        else if (!strncmp(token, "status=",strlen("status=")) && ++parse_count)
        {
            token += strlen("status=");
            prConfig->rep_status = atoi(token);
            if(prConfig->rep_status != 200 && prConfig->rep_status != 416 && prConfig->rep_status != 404)
            {
                debug(170,1)("(mod_range) parse_param status is %d force set 416\n", prConfig->rep_status);
                prConfig->rep_status = 416;
            }
        }
        else if (!strncmp(token, "preload", strlen("preload")) && ++parse_count)
        {
            prConfig->whole.preload = 1;
            prConfig->whole.fast = 1;
            prConfig->whole.offset = 102400;         //default 100KB
            token += strlen("preload");
            if(*token == '=' && ++token && xisdigit(*token))
                prConfig->whole.offset = strto_off_t(token, NULL, 10);
        }
        else if (!strcmp(token, "url_range") && ++parse_count)
            occurError = parse_url_range(prConfig, &sConfig, &parse_count);
        else if (!strcmp(token, "support_complex"))
            ++parse_count;
        else if (!strcmp(token, "disable_complex") && ++parse_count)
            prConfig->sup_complex = 0;
        else
        {
            if(parse_count > 9)
                break;
            occurError = 1;
            debug(170,0)("(mod_range) parse_param no match Configure Type Configure Type is\npreload or preload=xxx (back to server limit range)\nstatus or status=xxx (Error status)\nstore_range\nurl_range rangeStart rangeEnd\nPlease Check your Confuigure!!\n");
        }
    }

    if(occurError > 0 || parse_count <= 0)
    {
        debug(170,1)("(mod_range) parse_param %d %s %d\n",parse_count, occurError > 0 ? "Parse Occur Fail" : "Parse Success Count is", parse_count);
        debug(170,1)("(mod_range) parse_param Faile %s\n",strBuf(sConfig));
        stringClean(&sConfig);
        prConfig_free_callback(prConfig);
        return -1;
    }
    while((token = strtok(NULL, w_space)))
    {
        strCat(sConfig, " ");
        strCat(sConfig, token);
    }
    debug(170,3)("[%s]\n",strBuf(sConfig));
    if (prConfig->whole.preload && prConfig->store)
    {
        prConfig->store = 0;
        debug(170,1)("(mod_range) parse occur conflict between store_range and preload ,Set store_range invalid!!\n");
    }
    stringClean(&sConfig);
    prConfig->channel = channel;
    cc_register_mod_param(mod, prConfig, prConfig_free_callback);
    return 0;
}


/*#################### Check and Porcess Client Error Range Part Start #########################*/
static int 
HdrRangeSpecParseCheck(const char *field, int flen, short *fail_count, short *spec_count)
{
    HttpHdrRangeSpec spec =	{-1, -1};
    const char *p;
    if (flen < 2)
        return (*fail_count += 1);
    squid_off_t last_pos = -1;
    if (*field == '-')
    {
        if (!httpHeaderParseSize(field + 1, &spec.length))
            return (*fail_count += 1);
        /* must have a '-' somewhere in _this_ field */
    }
    else if (!((p = strchr(field, '-')) || (p - field >= flen)))
    { 
        debug(170, 5) ("ignoring invalid (missing '-') range-spec near: '%s'\n", field);
        return  (*fail_count += 1);
    }
    else
    {
        if (!httpHeaderParseSize(field, &spec.offset))
            return (*fail_count += 1);
        p++;
        /* do we have last-pos ? */
        if (p - field < flen)
        {
            if (!httpHeaderParseSize(p, &last_pos))
                return (*fail_count += 1);
            spec.length = ((last_pos + 1) >= spec.offset) ? ((last_pos + 1) - spec.offset) : 0;
        }
    }
    if ((spec.offset == -1 && spec.length <= 0) || (spec.length != -1 && !spec.length))
        return (*fail_count += 1);
    return (*spec_count += 1);
}


static int 
CheckClientRangeDone(HttpHeader *hdr, http_hdr_type id, const mod_config *prConfig)
{
    HttpHeaderEntry *e = NULL;
    if(NULL == (e =httpHeaderFindEntry(hdr, id)))
        return -1;
    const char *item;
    const char *pos = NULL; 
    int ilen;
    if (strNCaseCmp(e->value, "bytes=", 6))
    {
        debug(170, 5)("mod_range parse Error %s:%s\n",strBuf(e->name), strBuf(e->value));
        return 1;
    }
    pos = strBuf(e->value) + 6; 
    short fail_count = 0, spec_count = 0; 
    while (strListGetItem(&e->value, ',', &item, &ilen, &pos))
        HdrRangeSpecParseCheck(item, ilen, &fail_count , &spec_count);
    if(fail_count && spec_count == 0) 
    {    
        debug(170, 5)("mod_range parse Error %s:%s\n",strBuf(e->name), strBuf(e->value));
        return 1;
    }
    return 0;
}
static int 
hook_request_header_check_range(clientHttpRequest *http)
{
    HttpHeader *hdr = NULL;
    mod_config *prConfig = NULL;
    if (!http || !http->request ||  http->request->method != METHOD_GET)
      return 0;
    if(NULL == (hdr = &http->request->header) || NULL == (prConfig = cc_get_mod_range_param(http->conn->fd)))
      return 0;

    int error1 = CheckClientRangeDone(hdr, HDR_RANGE, prConfig);
    int error2 = CheckClientRangeDone(hdr, HDR_REQUEST_RANGE, prConfig);
    if(prConfig->rep_status != 200 && (error1 + error2 >= 2 || (error2 + error1 == 0 && error1 != 0)))
    {
        
        debug(170, 5)("mod_range Error Range response %d  %s\n", prConfig->rep_status, http->uri);
        http->log_type = LOG_TCP_RANGE_DENIED;
        ErrorState *err = errorCon(ERR_RANGE_NOT_SATISFIABLE, prConfig->rep_status, http->orig_request);
        /* avoid mod_errorpage set 404*/
        if(prConfig->rep_status == 416)
        {
            err->page_id = ERR_RANGE_NOT_SATISFIABLE;
            err->type = ERR_RANGE_NOT_SATISFIABLE;
            err->http_status = 416; 
        }
        http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
        errorAppendEntry(http->entry, err);
        return 1;	
    }
    if(error1 == 1)
        httpHeaderDelById(hdr, HDR_RANGE);
    if(error2 == 1)
        httpHeaderDelById(hdr, HDR_REQUEST_RANGE);
    return 0;
}


static int
hook_check_response_for_range(clientHttpRequest * http, HttpReply *rep)
{
    if(rep && rep->sline.status == 200)
    {
        if(!http->request->range && http->sc->range)
        {
            http->sc->range = NULL;
            http->sc->range_iter = NULL;
        }
        if(part_slot >= 0 && fd_table[http->conn->fd].cc_run_state[part_slot])
        {
            debug(170, 3)("mod_range preload conflict with mod_partion_store, mod_range disable\n");
            return 0;
        }
        if(http->request->range && !httpHdrRangeCanonize(http->request->range, rep->content_length))
        {
            httpHdrRangeDestroy(http->request->range);
            http->request->range = NULL;
            http->sc->range = NULL;     
            http->sc->range_iter = NULL; 
            mod_config *prConfig = cc_get_mod_range_param(http->conn->fd);
            if(prConfig == NULL || prConfig->rep_status == 200||  LETV == prConfig->channel) 
              return 0; 
            if (http->reply)
              httpReplyDestroy(http->reply);
            http->reply = NULL;
            http->request->flags.range = 0;     //Must set flags.range = 0, or create endless loop
            http->flags.hit = 0; 
            storeClientUnregister(http->sc, http->entry, http);
            http->sc = NULL; 
            storeUnlockObject(http->entry);
            http->log_type = LOG_TCP_RANGE_DENIED;
            ErrorState *err = errorCon(ERR_RANGE_NOT_SATISFIABLE, prConfig->rep_status, http->orig_request);
            /* avoid mod_errorpage set 404*/
            if(prConfig->rep_status == 416)
            {
                err->page_id = ERR_RANGE_NOT_SATISFIABLE;
                err->type = ERR_RANGE_NOT_SATISFIABLE;
                err->http_status = 416;
            }
            http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
            debug(170, 7)("mod_range client range error response  %s\n", http->uri);
            errorAppendEntry(http->entry, err);
            return 1;
        }
    }
    return 0;
}
/*#################### Check and Porcess Client Error Range Part End #########################*/



/*#################### Parse Range From Url Part #########################*/
static char * 
SearchKey(char *Base, const char *key, int sen)
{
    if(NULL == Base || NULL == key)
        return NULL;
    char *prStart = NULL;
    if(sen)
        return (prStart = strstr(Base, key));
    else
        return (prStart = strcasestr(Base, key));


}
static void 
DeleteRangeUrl(char *strUrl, const char *prStart, const char *prEnd, int del_count, int sen)
{
    char *delStart = NULL, *delEnd= NULL;
    if(strUrl && del_count && (delStart = SearchKey(strUrl, prStart, sen)))
    {
        if((delEnd = strstr(delStart, "&")))
            memcpy(delStart, delEnd + 1, strlen(delEnd));
        else if (*(delStart -1) == '&' || *(delStart -1) == '?')
            *(delStart -1) = '\0';
        else 
            delStart[0] = '\0';
        del_count--;
    }

    if(strUrl && del_count && (delStart = SearchKey(strUrl, prEnd, sen))) 
    {
        if((delEnd = strstr(delStart, "&")))
            memcpy(delStart, delEnd + 1, strlen(delEnd));
        else if (*(delStart -1) == '&' || *(delStart -1) == '?')
            *(delStart -1) = '\0';
        else
            delStart[0] = '\0';
    }
}

static squid_off_t 
GetParamNumber(char* strBase ,int len)
{
    if(strBase != NULL)
    {   
        char *end = NULL;
        strBase += len;
        squid_off_t offset = strto_off_t(strBase, &end, 10);   
        if(strBase == end)
            return -1; 
        return offset < -1 ? -1: offset;
    }   
    return -1; 
}

static int 
hook_parse_range_from_url(clientHttpRequest *http)
{
    mod_config *prConfig = cc_get_mod_range_param(http->conn->fd);
    if(prConfig && prConfig->url_range.prStart && prConfig->url_range.prEnd && http->uri)
    {
        int sen = prConfig->url_range.sensitive; 
        char *sprStart = SearchKey(http->uri,prConfig->url_range.prStart, sen);
        char *sprEnd = NULL;
        int del = 2;
        if(sprStart && !strcmp(prConfig->url_range.prEnd, "-"))
        {
            sprEnd = strstr(sprStart, prConfig->url_range.prEnd); 
            del = 1;
        }
        else if (!strcmp(prConfig->url_range.prEnd, "-"))
            return 1;
        else 
            sprEnd = SearchKey(http->uri,prConfig->url_range.prEnd, sen);
        if (NULL == sprEnd && NULL == sprStart)
            return 1;
        squid_off_t iStart = GetParamNumber(sprStart, strlen(prConfig->url_range.prStart)); 
        squid_off_t iEnd = GetParamNumber(sprEnd, strlen(prConfig->url_range.prEnd));
        int should_return = 0;
        if((iStart > iEnd  && iEnd >= 0) || (iStart < 0  && iEnd <= 0)) 
        {
            if(prConfig->rep_status != 200)
            {
                ErrorState *err = NULL;
                err = errorCon(ERR_INVALID_URL, 416, http->request);

                err->page_id = ERR_INVALID_URL;
                err->type = ERR_INVALID_URL;
                err->http_status = 416;
                err->url = xstrdup(http->uri);
                http->al.http.code = err->http_status;
                http->log_type = LOG_TCP_DENIED;
                http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
                errorAppendEntry(http->entry, err);
                debug(170, 3) ("mod_range: reply 416 start(%" PRINTF_OFF_T ") end(%" PRINTF_OFF_T ")\n", iStart, iEnd);
                return 3;
            }
            should_return = 1;
        }

        if(LETV != prConfig->channel) //Get rid of levt
        {
            DeleteRangeUrl(http->uri, prConfig->url_range.prStart, prConfig->url_range.prEnd, del, sen);
            DeleteRangeUrl(http->request->urlpath.buf, prConfig->url_range.prStart, prConfig->url_range.prEnd, del, sen);
            http->request->urlpath.len = strlen(http->request->urlpath.buf);
            DeleteRangeUrl(http->orig_request->urlpath.buf, prConfig->url_range.prStart, prConfig->url_range.prEnd, del, sen); 
            http->orig_request->urlpath.len = strlen(http->orig_request->urlpath.buf);
            safe_free(http->request->canonical);
            safe_free(http->orig_request->canonical);
            /*
               DeleteRangeUrl(http->request->canonical, prConfig->url_range.prStart, prConfig->url_range.prEnd, del, sen);
               DeleteRangeUrl(http->orig_request->canonical, prConfig->url_range.prStart, prConfig->url_range.prEnd, del, sen);
             */
            debug(170,3)("mod_range url_range %s\n", http->uri);
        }
        if(should_return)
        {
            debug(170,3)("mod_range url_range Not Add Range end(%" PRINTF_OFF_T ") < start(%" PRINTF_OFF_T ") %s\n",iEnd, iStart, http->uri);
            return 1;
        }
        char value[50];
        int len = 0;
        if(iStart == -1)
            len = snprintf(value, 50, "bytes=-%" PRINTF_OFF_T "", iEnd);
        else if (iEnd == -1)
            len = snprintf(value, 50, "bytes=%" PRINTF_OFF_T "-", iStart);
        else
            len = snprintf(value, 50, "bytes=%" PRINTF_OFF_T "-%" PRINTF_OFF_T "", iStart, iEnd);
        value[len] = '\0';
        HttpHeaderEntry *e = NULL;
        HttpHeader *hdr = &http->request->header;
        if (((e = httpHeaderFindEntry(hdr, HDR_RANGE)) || (e =httpHeaderFindEntry(hdr,HDR_REQUEST_RANGE))))
        {
            stringAppend(&e->value, ", ", 2);
            stringAppend(&e->value, value + 6, strlen(value + 6)); 
            debug(170, 3) ("mod_range url_range Append [%s] to Range [%s] for %s\n", value + 6, strBuf(e->value), http->uri);
        }
        else
        {
            httpHeaderAddEntry(&(http->request->header), httpHeaderEntryCreate(HDR_RANGE, "Range", value));
            debug(170, 3) ("mod_range url_range Add Range %s for %s\n",value, http->uri);
        }
    }
    return 1;
}
/*#################### Parse Range From Url Part End #########################*/


/*#################### Store Range Part Start #########################*/
static int 
hook_store_range_make_public(int fd, HttpStateData *httpState)
{
    if(!httpState->request || httpState->request->method != METHOD_GET || httpState->entry == NULL || NULL == httpState->entry->mem_obj->reply)
        return 0;
    if(httpState->entry->mem_obj->reply->sline.status != HTTP_PARTIAL_CONTENT)
        return 0; 
    mod_config *prConfig = NULL;
    if((prConfig = cc_get_mod_range_param(fd)) && prConfig->store)
    {
        debug(170,3)("mod_range: store range response 206 %s\n", urlCanonical(httpState->request));
        return 6;
    }
    return 0;
}


static int 
hook_store_range_modify_store_url(clientHttpRequest *http)
{
    if(http->request->method != METHOD_GET)
        return -1;
    mod_config *prConfig = cc_get_mod_range_param(http->conn->fd);
    if(prConfig == NULL ||!prConfig->store)
        return -1;
    
    char *store = xcalloc(1, 4096);
    char *prStart = strstr(http->uri, "://"); 
    prStart += 3;
    strncpy(store, http->uri, prStart - http->uri);
    char *prEnd = strstr(prStart, "/");
    if(prEnd)
        prStart = prEnd + 1;
    if((prEnd = strstr(prStart, "?")))
        strncat(store, prStart, prEnd - prStart);
    else
        strncat(store, prStart, strlen(prStart));

    safe_free(http->request->store_url);
    StoreEntry *e = NULL;
    if ((e = storeGetPublic(store, METHOD_GET)))
    {
        safe_free(http->request->store_url);
        http->request->store_url = xstrdup(store);
        debug(170, 3)("mod_range: store range object HIT modify store_url=%s\n", http->request->store_url);  
    }
    else
    {
        HttpHeader *hdr = &http->request->header;
        HttpHeaderEntry *range = httpHeaderFindEntry(hdr, HDR_RANGE);
        if(range == NULL && NULL == (range = httpHeaderFindEntry(hdr, HDR_REQUEST_RANGE)))
        {
            http->request->store_url = xstrdup(store);
            safe_free(store);
            return -1;
        }

        if (strNCaseCmp(range->value, "bytes=", 6))
        {
            debug(170, 5)("mod_range Error %s:%s MISS modify store_url=%s\n",strBuf(range->name), strBuf(range->value),store);
            http->request->store_url = xstrdup(store);
            httpHeaderDelById(hdr, range->id);
            safe_free(store);
            return 0;
        }
        const char *item;
        const char *pos = strBuf(range->value) + 6; 
        int ilen, count = 0;
        short fail_count = 0, spec_count = 0;
        while (strListGetItem(&range->value, ',', &item, &ilen, &pos))
        {
            HdrRangeSpecParseCheck(item, ilen, &fail_count , &spec_count);
            if(spec_count == 1)
            {
                if(count > 0) 
                  strncat(store, "," , 1);
                else 
                {
                    strncat(store, delimit, strlen(delimit));
                    strncat(store, "Range:bytes=", strlen("Range:bytes=")); 
                }
                strncat(store, item , ilen);
                count++;
            }
            spec_count = 0;
        }
        http->request->store_url = xstrdup(store);
        debug(170, 3)("mod_range: store range object MISS modify store range =%s\n", http->request->store_url);
    }
    safe_free(store);
    return 0;
}

/*#################### Store Range Part End #########################*/


/*#################### Preload Whole Object Part Start #########################*/
static void
clientAclCheck(mod_range_preload_data *fword)
{
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
}

static request_t * 
createRequest(request_t *old_request, char *url)
{
    request_t * new_request = urlParse(old_request->method, url);
    if(new_request == NULL)
    {
        debug(170, 3)("mod_range preload urlParse fail %s\n",urlCanonical(old_request));
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
#endif                                      //  FOLLOW_X_FORWARDED_FOR 
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
        new_request->store_url = xstrdup(url);
    debug(170, 3)("mod_range createRequest for preload %s\n", urlCanonical(new_request));
    return new_request;
}

static void 
rangePreloadFastStore(StoreEntry *e) 
{
    if(NULL == e)
        return;
    if (e->store_status == STORE_PENDING) 
        storeSwapOut(e);
    if (e->mem_status != NOT_IN_MEMORY && SWAPOUT_WRITING == e->swap_status && e->mem_obj->swapout.sio != NULL && e->mem_obj->swapout.sio->offset > 0)
        storeSetMemStatus(e, NOT_IN_MEMORY);

    const MemObject *mem = e->mem_obj;
    dlink_node *node;
    store_client *sc;

    for (node = mem->clients.head; node; node = node->next)
    {
        sc = node->data;
        if(sc->type != STORE_DISK_CLIENT)
            sc->type = STORE_DISK_CLIENT;
    }
    return;
}

/*
static squid_off_t 
hook_fast_store_preload_object(const StoreEntry *e, int *shouldReturn)
{
    *shouldReturn = 0;
    if(e == NULL || e->store_status == SWAPOUT_DONE || e->mem_obj == NULL)
        return 0;
    if (!EBIT_TEST(e->flags, ENTRY_CACHABLE) || EBIT_TEST(e->flags, RELEASE_REQUEST) || EBIT_TEST(e->flags, ENTRY_ABORTED))
        return 0;                 

    if(e->mem_obj->request && httpHeaderHas(&e->mem_obj->request->header, HDR_X_WEBLUKER))
    {
        *shouldReturn = 1;
        debug(170,9) ("mod_range fast_store preload object %s\n", e->mem_obj->url);
        return e->mem_obj->inmem_hi + 1;
    }
    return 0;           
}
*/
static void 
clientForwordDone(mod_range_preload_data * fword, int cache, char *desc)
{
    if(cache)
        debug(170, 5)("mod_range preload complete %s %s\n", desc, storeLookupUrl(fword->entry));
    else
    {
        storeReleaseRequest(fword->entry);
        httpHeaderDelById(&fword->request->header, HDR_X_WEBLUKER);
        debug(170, 5)("mod_range preload no_cache %s %s\n", desc, storeLookupUrl(fword->entry));
    }
    requestUnlink(fword->request);
    storeClientUnregister(fword->sc, fword->entry, fword);
    storeUnlockObject(fword->entry);
    cbdataFree(fword);
    ll_count--;
    debug(170, 4)("mod_range_preload_data Count : %lld\n",ll_count);
}

static void
handleForwordReply(void *data, char *buf, ssize_t size)
{
    mod_range_preload_data * fword = data;
    if(fword->clientfd >= 0)
    {
        fd_close(fword->clientfd);
        fword->clientfd = -1;
    }
    StoreEntry *e = fword->entry;
    if (e->mem_obj->reply->sline.status != 200) 
    {
        clientForwordDone(fword, 0, "NO HTTP_OK");
        return;
    }
    if(EBIT_TEST(e->flags, ENTRY_ABORTED))
    {
        clientForwordDone(fword, 0, "ENTRY_ABORTED");
        return;
    }
    if(EBIT_TEST(e->flags, RELEASE_REQUEST))
    {    
        clientForwordDone(fword, 0, "RELEASE_REQUEST");
        return;
    }
    if(!EBIT_TEST(e->flags, ENTRY_CACHABLE))
    {
        clientForwordDone(fword, 0, "NO CACHABLE");
        return;
    }
    if(fword->fast && fword->request) /*&& httpHeaderHas(&fword->request->header, HDR_X_WEBLUKER)*/
    {
        rangePreloadFastStore(fword->entry);
    }
    if(SWAPOUT_DONE == e->swap_status)
    {
        clientForwordDone(fword, 1, "CACHE SWAPOUT_DONE");
        return; 
    }
    if(size <= 0 )
    {    
        clientForwordDone(fword, 1, "CACHE SIZE TOO SMALL");
        return;
    }

    fword->offset += size;
    storeClientCopy(fword->sc, fword->entry, fword->offset, fword->offset, STORE_CLIENT_BUF_SZ, store_buff,
                handleForwordReply, fword);
}

static int
cdnRangePreloadDone(request_t *req, int fd)
{
    fd_open(fd, FD_SOCKET, "Async forward");
    CBDATA_INIT_TYPE(mod_range_preload_data);
    mod_range_preload_data *fword = cbdataAlloc(mod_range_preload_data);
    ll_count++;
    char *url = (char *)urlCanonical(req);
    httpHeaderAddEntry(&req->header, httpHeaderEntryCreate(HDR_X_WEBLUKER, NULL, "Back-Server"));
    debug(170, 4)("mod_range add Header [Webluker-Edge] [Back-Server] fd %d %s\n", fd, url);
    fword->clientfd= fd;
    fword->request = requestLink(req);
    clientAclCheck(fword);
    StoreEntry *entry = storeCreateEntry(url, req->flags, req->method);
    if (req->store_url)
      storeEntrySetStoreUrl(entry, req->store_url);
    if(Config.onoff.collapsed_forwarding)
    {
        EBIT_SET(entry->flags, KEY_EARLY_PUBLIC);
        storeSetPublicKey(entry);
    }
    fword->fast = 0;
    mod_config *prConfig = cc_get_mod_range_param(fd); 
    if(prConfig && !prConfig->whole.fast)        //redirect_location jump more count
        fword->fast = 0;
    fword->offset = 0;
    fword->sc = storeClientRegister((fword->entry = entry), fword);
    fwdStart(fword->clientfd, fword->entry, fword->request);
    storeClientCopy(fword->sc, fword->entry, fword->offset, fword->offset, STORE_CLIENT_BUF_SZ, store_buff,
                handleForwordReply, fword);
    return 0;
}


static int
hook_range_preload_check(HttpStateData *httpState, HttpHeader* hdr) 
{
    if(hdr && !httpHeaderDelById(hdr, HDR_X_CACHE))
    {
        httpHeaderDelById(hdr, HDR_X_WEBLUKER);
        debug(170, 5)("mod_range preload not find Miss-Range\n");
        return 0;
    }
    if(!httpState || !hdr || !httpState->orig_request || NULL == httpState->orig_request->range)
    {
        debug(170, 3)("mod_range preload don't find Range\n");
        return 0;
    }
    if(httpState->orig_request->method != METHOD_GET || httpState->fd < 0)
    {
        debug(170, 3)("mod_range preload isn't GET Request %d\n", httpState->fd);
        return 0;
    }
    mod_config *prConfig = cc_get_mod_range_param(httpState->fd);
    if(!prConfig || !prConfig->whole.preload)
    {
        debug(170, 3)("mod_range preload don't find param %p\n", prConfig); 
        return 0;
    }
    httpState->orig_request->flags.cachable = 1;
    if(part_slot >= 0 && fd_table[httpState->fd].cc_run_state[part_slot])
    {
        debug(170, 3)("mod_range preload conflict with mod_partion_store, mod_range disable\n");
        return 0;
    }

    if(httpState && httpState->entry && httpState->entry->mem_obj && httpState->entry->mem_obj->ims_entry)
    {
        debug(170, 3)("mod_range preload not find is IMS\n");
        return 0;
    }
    request_t *request = httpState->orig_request;
    StoreEntry *entry = storeGetPublicByRequestMethod(request, request->method);
    if(entry && entry != httpState->entry)
    {    
        debug(170, 3)("mod_range preload StoreEntry is exist!\n");
        return 0;
    } 

    char *uri = NULL;
    if(urlCanonical(request))
    {
        uri = xstrdup(urlCanonical(request));
    }
    if(uri && prConfig && LETV == prConfig->channel)
    {
        httpHeaderDelById(&httpState->orig_request->header, HDR_RANGE);
        httpHdrRangeDestroy(httpState->orig_request->range);
        httpState->orig_request->range = NULL;
        if(httpState->orig_request != httpState->request)
        {
            httpHeaderDelById(&httpState->request->header, HDR_RANGE); 
            if(httpState->request->range) httpHdrRangeDestroy(httpState->request->range);
            httpState->request->range = NULL;
        }
        DeleteRangeUrl(uri, prConfig->url_range.prStart, prConfig->url_range.prEnd, 2, prConfig->url_range.sensitive);    
    }
    if(uri && prConfig && prConfig->whole.preload && httpHeaderHas(hdr, HDR_RANGE))
    {   
        request_t *req = NULL;

        if(!(req =createRequest(request, uri)))
        {
            debug(170, 3)("mod_range preload createRequest Fail\n");
            safe_free(uri);
            return  0; 
        }

        int fd = -1, i = 0;
        while ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 && ++i <= 5);
        safe_free(uri);
        if(fd < 0)
        {
            debug(170, 3)("mod_range forward Creat fd fail Unlink Request %s\n", uri);
            safe_free(uri);
            requestUnlink(req);
            req = NULL;
            return 0;
        }
        safe_free(uri);
        storeReleaseRequest(httpState->entry);
        cdnRangePreloadDone(req, fd);
        return 0;
    }
    safe_free(uri);
    return 0;
}


static void 
checkClientRangeHitDone(clientHttpRequest * http, mod_config *prConfig)
{
    if(http->entry && http->request->range && prConfig && prConfig->whole.preload)
    {
        http->log_type = LOG_TCP_HIT;
        if (STORE_OK == http->entry->store_status)
             return;

        long long offset = -1, length = 0;
        HttpHdrRangePos pos = HttpHdrRangeInitPos;
        const HttpHdrRangeSpec *spec;
        while ((spec = httpHdrRangeGetSpec(http->request->range, &pos)))
        {   
            if (spec->offset < offset || offset == -1)
            {
                offset = spec->offset;
                length = spec->length;
            }
        }
        long long load = http->entry->mem_obj ? http->entry->mem_obj->inmem_hi : 0;
        if(offset >=  0 && offset <= load + prConfig->whole.offset + 1)
            return;
        if(offset <0 && http->entry->mem_obj && http->entry->mem_obj->reply && http->entry->mem_obj->reply->content_length > 0)
        {
            squid_off_t start = http->entry->mem_obj->reply->content_length - length;
            if(load + prConfig->whole.offset >= start)
                return;
            offset = start;
        }
        load +=prConfig->whole.offset;
        debug(170, 5)("mod_range [Set MISS] [%lld] > [%lld] %s\n",(long long)offset,(long long)load, http->uri);
//        http->request->flags.cachable = 0;
        http->entry = NULL;
        http->log_type = LOG_TCP_MISS;
        return ;
    }
    return ;
}

static void 
hook_preload_quick_abort_check(StoreEntry * e, int *abort)
{
    request_t *r = e->mem_obj->request;
    if(r && httpHeaderHas(&r->header, HDR_X_WEBLUKER))
    {
        debug(170, 4)("mod_range Preload StoreEntry Store_url %s\n", storeLookupUrl(e));
        *abort = 0;
    }
}

static int 
hook_set_range_request_cachable(clientHttpRequest * http)
{
    if(http->request == NULL||http->request->range == NULL||!http->request->flags.cachable)
        return 0;
    mod_config *prConfig = cc_get_mod_range_param(http->conn->fd);
    if(http->request->method == METHOD_GET && prConfig && prConfig->whole.preload)
    {
        squid_off_t limit = Config.rangeOffsetLimit;
        squid_off_t offset = prConfig->whole.offset;
        squid_off_t  min = httpHdrRangeFirstOffset(http->request->range);
        if(min < 0 || min > prConfig->whole.offset || !limit|| (limit != -1 && min > limit) ||  Config.quickAbort.min != -1)
        {
            httpHeaderDelById(&http->request->header, HDR_X_CACHE);
            httpHeaderAddEntry(&http->request->header, httpHeaderEntryCreate(HDR_X_CACHE, NULL, "Miss-Range"));
            http->request->flags.cachable = 0;
            debug(170, 4)("mod_range request Set cachable=0 for Preload add Powered-By-ChinaCache Miss-Range %s\n",http->uri);
            return 0;
        }
        debug(170, 9)("mod_range [%ld] [%ld match limit %ld %s\n",(long)min, (long)offset, (long)limit, http->uri);
    }
    return 0;
}

/*
static void 
hook_change_request_cachable(request_t *request, FwdState *fwdState)
{
    if(request->range == NULL || request->flags.cachable || request->method != METHOD_GET)
        return;
    mod_config *prConfig = cc_get_mod_range_param(fwdState->client_fd);
    if(prConfig && prConfig->whole.preload && checkHeaderExist(request, 0))
    {
        request->flags.cachable = 1;
        debug(170, 4)("mod_range range request cachable = 1 for Preload  in %s\n", urlCanonical(request));
    }
}
*/
/*#################### Porcess Client Complex Range Part Start #########################*/
/*
static int 
RangeWillBeComplexCheck(const HttpHdrRange * range)
{
    if(range == NULL || range->specs.count <= 1 || NULL == range->specs.items)
        return 0;
    HttpHdrRangePos pos = HttpHdrRangeInitPos;
    const HttpHdrRangeSpec *spec;
    squid_off_t offset = 0;
    assert(range);
    while ((spec = httpHdrRangeGetSpec(range, &pos))) 
    {   
        if (-1 == spec->offset)  // ignore unknowns 
          continue;
        if (spec->offset < offset) return 1;

        offset = spec->offset;

        if (spec->length != -1)   // avoid  unknowns 
          offset += spec->length;
    }   
    return 0;
}
*/
static void
hook_check_support_complex(clientHttpRequest *http, int *supp)
{
    if(http->request->range == NULL)
        return;
    mod_config *prConfig = cc_get_mod_range_param(http->conn->fd);
    if(http->log_type  == LOG_TAG_NONE && http->sc == NULL && prConfig && prConfig->whole.preload)
        checkClientRangeHitDone(http, prConfig);
    if(NULL == prConfig || 0 == prConfig->sup_complex)
        return;
    *supp = 2; 
    return;
}

/*
static int 
hook_complex_range_process(StoreEntry *e, store_client *sc)
{
    MemObject *mem = e->mem_obj;
    if ((sc->copy_offset >= mem->inmem_lo && sc->copy_offset < mem->inmem_hi) || STORE_DISK_CLIENT == sc->type)
      return 0;
    sc->type = STORE_DISK_CLIENT;
    return 0;
}
*/
static int modifyHeader2WithReply(int fd, HttpReply *reply)
{
    if(HTTP_OK == reply->sline.status)
    {
        debug(170, 2)("(mod_range) -> del cr when http_ok\n");
        httpHeaderDelById(&reply->header, HDR_CONTENT_RANGE);
    }
    return 0;
}

static int func_http_repl_read_start(int fd, HttpStateData *data)
{
    assert(data);
    return modifyHeader2WithReply(fd, data->entry->mem_obj->reply);

}

static void client_cache_hit_start(int fd, HttpReply *rep, StoreEntry *e)
{
    assert(rep);
    modifyHeader2WithReply(fd, rep);
}

/*#################### Porcess Client Complex Range Part End #########################*/


int mod_register(cc_module *module)
{
    debug(170, 1)("(mod_range) ->  init: init module\n");

    strcpy(module->version, "7.0.R.16488.i686");
                //modules frame
    cc_register_hook_handler(HPIDX_hook_func_sys_init,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
                hook_init);

    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
                hook_func_sys_parse_param);

    cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
                hook_cleanup);
                //client error range process
    cc_register_hook_handler(HPIDX_hook_func_client_access_check_done,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_access_check_done),
                hook_request_header_check_range);

    cc_register_hook_handler(HPIDX_hook_func_send_header_clone_before_reply,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_send_header_clone_before_reply),
                hook_check_response_for_range);
                //client complex range process
    cc_register_hook_handler(HPIDX_hook_func_support_complex_range,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_support_complex_range),
                hook_check_support_complex);

    /*
    cc_register_hook_handler(HPIDX_hook_func_store_client_copy3,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_store_client_copy3),
                hook_complex_range_process);
                */
                //pares range from url
    cc_register_hook_handler(HPIDX_hook_func_http_req_parsed,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_parsed),
                hook_parse_range_from_url);
                //store  range response
    cc_register_hook_handler(HPIDX_hook_func_http_req_read_second,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read_second),
                hook_store_range_modify_store_url);

    cc_register_hook_handler(HPIDX_hook_func_http_repl_cachable_check,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_cachable_check),
                hook_store_range_make_public);
                //preload whole object
    cc_register_hook_handler(HPIDX_hook_func_http_req_send_exact,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send_exact),
                hook_range_preload_check);

    cc_register_hook_handler(HPIDX_hook_func_quick_abort_check,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_quick_abort_check),
                hook_preload_quick_abort_check);

    cc_register_hook_handler(HPIDX_hook_func_private_process_miss_in_error_page,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_process_miss_in_error_page),
                hook_set_range_request_cachable);

    /*
       cc_register_hook_handler(HPIDX_hook_func_private_preload_change_fwdState,
       module->slot,
       (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_preload_change_fwdState),
       hook_change_request_cachable);

    cc_register_hook_handler(HPIDX_hook_func_private_storeLowestMemReaderOffset,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_storeLowestMemReaderOffset), 
                hook_fast_store_preload_object);
    */
    //module->hook_func_http_repl_read_start = func_http_repl_read_start;
    cc_register_hook_handler(HPIDX_hook_func_http_repl_read_start,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_start),
            func_http_repl_read_start);

    //hook_func_client_cache_hit_start
    cc_register_hook_handler(HPIDX_hook_func_client_cache_hit_start,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_cache_hit_start),
            client_cache_hit_start);

    if(reconfigure_in_thread)
        mod = (cc_module*)cc_modules.items[module->slot];
    else
        mod = module;
    return 0;
}
#endif

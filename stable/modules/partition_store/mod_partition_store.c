#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK

static cc_module* mod = NULL;

/*pei zhi xiang*/
struct partition_store_config
{
    squid_off_t splice_size;
    squid_off_t pretext_send;
};

//static void createNewRequest(char *r, clientHttpRequest *http);
/* clientHttpRequest si you shu ju */
struct clientHttpRequest_pd
{
    HttpHdrRange *orig_range;
    char *orig_s_range;
    squid_off_t splice_num;
    squid_off_t splice_size;
    squid_off_t max_splice_num;
    int bind;
    int header_sent;
    squid_off_t elength;
    time_t lastmod;
    time_t expires;
    time_t timestamp;
    char * meta_url;
    int status_302;
    HttpReply * first_fragment_reply;
    log_type first_fragment_log_type;
    int fragment_req_No;
    time_t first_fragment_lastmod;
    int should_release_req;
    int headerentry_once_null;
    HttpReply * ims_rep;
    int HEADreq_run_state_set;
};

/* request_t si you shu ju */
struct request_pd
{
    HttpHdrRange * index_range;
    String value; 
};

/* free the request_t's private data'*/
static int free_request_pd(void* data)
{
    debug(146, 3)("free the private data of request_t\n");
    struct request_pd * pd = data;
    if(pd)
    {
        if (pd->index_range)
        {
            httpHdrRangeDestroy(pd->index_range);
            pd->index_range = NULL;
        }
        stringClean(&pd->value);
    }
    safe_free(pd);
    return 0;
}

/* free the config structure*/
static int free_config(void* data)
{
    safe_free( data);
    return 0;
}

/* set the header entry  storeKey 
 *
 * */
static cache_key *
storePrivateKeyByHeader(const char* url )
{
    static cache_key digest[SQUID_MD5_DIGEST_LENGTH];
    const method_t method = METHOD_GET;
    unsigned char m = (unsigned char) method;
    char * header = "@@@header";
    char *store_url = xcalloc(1, strlen(url)+10);
    strcat(store_url, url);
    strcat(store_url, header);
    debug(146, 3) ("mod_partition_store-->: storePrivateKeyByHeader: url is %s\n", store_url);
    SQUID_MD5_CTX M;
    SQUID_MD5Init(&M);
    SQUID_MD5Update(&M, &m, sizeof(m));
    SQUID_MD5Update(&M, (unsigned char *) store_url, strlen(store_url));
    SQUID_MD5Final(digest, &M);
    safe_free(store_url);
    return digest;
}

static cache_key *
storePrivateKeyByRangeSpec(const char* url, squid_off_t offset, squid_off_t length)
{
    static cache_key digest[SQUID_MD5_DIGEST_LENGTH];
    const method_t method = METHOD_GET;
    unsigned char m = (unsigned char) method;
    SQUID_MD5_CTX M;
    SQUID_MD5Init(&M);
    SQUID_MD5Update(&M, &m, sizeof(m));
    SQUID_MD5Update(&M, (unsigned char *) url, strlen(url));
    SQUID_MD5Update(&M, &offset, sizeof(squid_off_t));
    SQUID_MD5Update(&M, &length, sizeof(squid_off_t));
    SQUID_MD5Final(digest, &M);
    return digest;
}

static StoreEntry * storeGetHeaderEntryByUrl(const char* url)
{
    return storeGet(storePrivateKeyByHeader(url));
}

struct _AddHeaderEntryState{
    char *url;
    cache_key * key;
    StoreEntry *e;
};

typedef struct _AddHeaderEntryState AddHeaderEntryState;


CBDATA_TYPE(AddHeaderEntryState);
static void
free_AddHeaderEntryState(void *data)
{
    AddHeaderEntryState *state = data;
    debug(146, 3) ("mod_partition_store-->: free_AddHeaderEntryState: %p\n", data);
    if (!EBIT_TEST(state->e->flags, ENTRY_ABORTED))
    {
        debug(146, 3) ("mod_partition_store-->: free_AddHeaderEntryState: state->flags is not entry_aborted\n");
        storeBuffer(state->e);
        storeTimestampsSet(state->e);
        storeComplete(state->e);
        storeTimestampsSet(state->e);
        storeBufferFlush(state->e);
    }
    storeUnlockObject(state->e);
    state->e = NULL;
    safe_free(state->url);
    state->key = NULL;
    //safe_free(state);
}

static void getReplyUpdateByIMSRep(clientHttpRequest *http)
{
    struct clientHttpRequest_pd * cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);     
    if (cpd && !cpd->ims_rep)
    {
        cpd->ims_rep = httpReplyClone(http->old_entry->mem_obj->reply);
    }
    storeTimestampsSet(http->old_entry);

    squid_off_t clen =  -1;
    if(http->old_entry->mem_obj->reply->sline.status == 200)
      clen = http->old_entry->mem_obj->reply->content_length; 
    if (http->old_entry->mem_obj->reply->sline.status == 206 && http->old_entry->mem_obj->reply->content_range)
      clen = http->old_entry->mem_obj->reply->content_range->elength;

    http->request->lastmod = -1;
    safe_free(http->request->etag);
    //http->request->flags.cache_validation = 0; 

    if(clen > 0 && cpd && cpd->splice_size > 0)
    {
        clen = clen / cpd->splice_size + 1;
        squid_off_t splice = 0;
        char * url = NULL; 
        if (http->request->store_url)
          url = xstrdup(http->request->store_url);
        else 
          url = xstrdup(urlCanonical(http->request));
        StoreEntry * e = storeGet(storePrivateKeyByHeader(url));
        if(e)
        {
            e->timestamp = http->old_entry->timestamp;  
            e->expires = http->old_entry->expires;
            debug(146,3) ("mod_partition_store Update header StoreEntry Date: %ld Exp : %ld %s\n",e->timestamp, e->expires, http->uri);
        }
        for( ; splice <= clen ; splice++)
        {
            if((e = storeGet(storePrivateKeyByRangeSpec(url, splice * cpd->splice_size, cpd->splice_size))) && e->lastmod == http->old_entry->lastmod)
            {
                e->timestamp = http->old_entry->timestamp;
                e->expires = http->old_entry->expires;
                debug(146,3) ("mod_partition_store Update %lld splice StoreEntry %s\n", (long long)splice, http->uri);
            }
            else if(e)
              debug(146,3) ("mod_partition_store Update %lld splice StoreEntry Fail lastmod != %ld %s\n", (long long)splice, http->old_entry->lastmod, http->uri);

        }
        safe_free(url);
    }
}

static void
storeAddHeaderEntry( const char* url, HttpReply* rep, clientHttpRequest *http)
{
    debug(146,3) ("mod_partition_store-->: storeAddHeaderEntry : url is : %s\n",url);
    struct clientHttpRequest_pd * cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);     
    HttpReply *reply = NULL;
    if (cpd && cpd->ims_rep && rep->sline.status == 304)
    {
        reply = httpReplyClone(cpd->ims_rep);
        debug(146, 9)("getted 304 updated reply\n");
        httpReplyDestroy(cpd->ims_rep);
        cpd->ims_rep = NULL;
    }
    else if (http->entry->mem_obj->reply && (http->entry->mem_obj->reply->sline.status == 200 || http->entry->mem_obj->reply->sline.status == 206))
    {
        reply = httpReplyClone(http->entry->mem_obj->reply);
        debug(146, 9)("mod_partition_store mom_obj reply\n");
    }
    else if (rep && (rep->sline.status == 200 || rep->sline.status == 206))
    {
        reply = httpReplyClone(rep);
        debug(146, 9)("mod_partition_store reply to client\n");
    }
    else 
    {
        debug(146, 9)("mod_partition_store: do not update header entry\n");
        return;
    }
    AddHeaderEntryState * state;
    request_flags flags = null_request_flags;
    CBDATA_INIT_TYPE_FREECB(AddHeaderEntryState, free_AddHeaderEntryState);
    state = cbdataAlloc(AddHeaderEntryState);
    state->url = xstrdup(url);
    if(state->key == NULL)
    {
        state->key = storePrivateKeyByHeader(url);
        debug(146,3) ("mod_partition_store-->: storeAddHeaderEntry : key : %s\n",storeKeyText(state->key));
    }

    flags.cachable = 1;
    state->e = new_StoreEntry(STORE_ENTRY_WITH_MEMOBJ,url);
    StoreEntry *e = state->e;
    e->lock_count = 1;
    MemObject* mem = e->mem_obj;
    mem->method = METHOD_GET;
    if(flags.cachable)
    {
        EBIT_SET(e->flags,ENTRY_CACHABLE);
        EBIT_CLR(e->flags,RELEASE_REQUEST);
    }
    //storeSetMemStatus(e, NOT_IN_MEMORY);
    e->store_status = STORE_PENDING;
    storeSetMemStatus(e, NOT_IN_MEMORY);
    e->swap_status = SWAPOUT_NONE;
    e->swap_filen = -1;
    e->swap_dirn = -1;
    e->refcount = 0;
    e->lastref = squid_curtime;
    e->timestamp = -1;		/* set in storeTimestampsSet() */
    e->ping_status = PING_NONE;
    EBIT_SET(e->flags, ENTRY_VALIDATED);
    EBIT_CLR(e->flags, KEY_PRIVATE);
    StoreEntry *e2= NULL;
    if ((e2 = (StoreEntry *) hash_lookup(store_table,state->key)))
    {
        storeSetPrivateKey(e2);
        storeRelease(e2);
        state->key = storePrivateKeyByHeader(url);
    }
    if(e->hash.key)
    {
        storeHashDelete(e);
    }
    storeHashInsert(e,state->key);
    if(e->swap_filen > -1)
      storeDirSwapLog(e,SWAP_LOG_ADD);
    e->store_status = STORE_PENDING;
    storeBuffer(e);
    if( httpHeaderHas(&reply->header,HDR_CONTENT_LENGTH) )
    {
        httpHeaderDelById(&reply->header,HDR_CONTENT_LENGTH);
        reply->content_length = -1;
    }
    httpReplySwapOut(reply, state->e);
    debug(146,2) ("mod_partition_store-->: storeAddHeaderEntry : swapout out reply to state->e\n");
    cbdataFree(state);
}
/**
 * return 0 if parse ok, -1 if error
 */
static int func_sys_parse_param(char *cfg_line)
{
    //Must be [mod_cc_fast on] or [mod_cc_fast off]
    assert(cfg_line);
    char *t = strtok(cfg_line, w_space);

    if(strcmp(t, "mod_partition_store"))
    {
        debug(146,0)("mod_partition_store cfg_line not begin with mod_partition_store!\n");
        return -1;
    }

    struct partition_store_config *cfg = xcalloc(1,sizeof(struct partition_store_config));
    t = strtok(NULL, w_space);
    cfg->splice_size =strto_off_t(t,NULL,10) ;
    cfg->pretext_send = 0;
    if(( t = strtok(NULL, w_space)) && xisdigit(*t))
    {
        cfg->pretext_send = strto_off_t(t,NULL,10);
        cfg->pretext_send = cfg->pretext_send  > 0 ? cfg->pretext_send : 0;
    }
    cc_register_mod_param(mod, cfg, free_config);
    return 0;
}

static int free_clientHttpRequest_pd(void * data)
{
    debug(146,3)("mod_partition_store-->: free_clientHttpRequest_pd\n");
    struct clientHttpRequest_pd * pd = data;
    if(pd)
    {
        if(pd->orig_range)
        {
            httpHdrRangeDestroy(pd->orig_range);
            pd->orig_range = NULL;
        }
        safe_free(pd->orig_s_range);
        safe_free(pd->meta_url);
        if(pd->first_fragment_reply) 
        {
            httpReplyDestroy(pd->first_fragment_reply);
            pd->first_fragment_reply = NULL;
        }
    }
    safe_free(pd);
    return 0;
};

/* chuang jian clientHttpRequest de si you shu ju */
static int clientHttpRequestCreateOrigRange(clientHttpRequest *http)
{
    if(http->request->method != METHOD_GET)
    {
        fd_table[http->conn->fd].cc_run_state[mod->slot] = 0;
        return 0;
    }
    struct partition_store_config * cfg = cc_get_mod_param(http->conn->fd,mod);
    if(cfg)
    {
        struct clientHttpRequest_pd * cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);
        if(cpd!=NULL)
        {
            free_clientHttpRequest_pd(cpd);
        }
        struct clientHttpRequest_pd * pd = xcalloc(1,sizeof(struct clientHttpRequest_pd));
        pd->splice_size = (squid_off_t)(cfg->splice_size);
        pd->header_sent = 0;
        pd->elength = 0;
        pd->splice_num = 0;
        pd->bind = 1;
        pd->lastmod = -1;
        pd->timestamp = -1;
        pd->expires = -1;
        pd->meta_url = NULL;
        pd->status_302 = 0;
        pd->max_splice_num = -1;
        pd->fragment_req_No = 0;
        pd->HEADreq_run_state_set = 0;
        pd->first_fragment_log_type = 0;
        pd->first_fragment_reply = NULL;
        pd->orig_s_range = NULL;
        HttpHeaderEntry *e = httpHeaderFindEntry(&http->request->header, HDR_RANGE);
        if(e || (e =  httpHeaderFindEntry(&http->request->header, HDR_REQUEST_RANGE)))
        {
            pd->orig_s_range = xstrdup(e->value.buf);
        }
        cpd = pd;
        cpd->orig_range = httpHeaderGetRange(&http->request->header);
        cc_register_mod_private_data(REQUEST_PRIVATE_DATA,http,cpd,free_clientHttpRequest_pd,mod );
        debug(146,2)("mod_partition_store cpd->orig_range %p %s\n", cpd->orig_range, http->uri);
        if(cfg->pretext_send && cpd->orig_range && cpd->orig_range->specs.count == 1)
        {
            HttpHdrRangeSpec * spec = cpd->orig_range->specs.items[0]; 
            if(spec && spec->offset >=0 && spec->length >= 0)
            {
                spec->length += cfg->pretext_send;
                debug(146,2)("mod_partition_store pretext send %" PRINTF_OFF_T " to %" PRINTF_OFF_T " %s\n",cfg->pretext_send, spec->length + spec->offset, http->uri);
            }
        
        }
    }
    return 0;
}

static int beforeClientProcessRequest2(clientHttpRequest*  http)
{
    struct clientHttpRequest_pd * cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);
    if (cpd && cpd->orig_range && cpd->orig_range && cpd->orig_range->specs.count > 1) 
    {    
        fd_table[http->conn->fd].cc_run_state[mod->slot] = 0; 
        http->request->flags.cachable = 0; 
        debug(146,2)("mod_partition_store multiple Range Back to Server %s\n", http->uri);
        return 0;
    }    
    debug(146,2)("mod_partition_store-->: beforeClientProcessRequest2\n" );

    if (!cpd || cpd->status_302 || !http->request)
        return 0;

    struct request_pd * rpd = cc_get_mod_private_data(REQUEST_T_PRIVATE_DATA, http->request, mod);
    if(rpd == NULL)
    {
        rpd = xcalloc(1,sizeof(struct request_pd));
        cc_register_mod_private_data(REQUEST_T_PRIVATE_DATA, http->request,rpd, free_request_pd,mod);
        debug(146, 3)("register request_t private data when fragNo=%d\n", cpd->fragment_req_No);
    }
    if(rpd->index_range)
    {    
        httpHdrRangeDestroy(rpd->index_range);
        rpd->index_range = NULL;
    }   

    if(cpd->orig_range)
    {
        HttpHdrRangePos pos = HttpHdrRangeInitPos;
        const HttpHdrRangeSpec *spec;
        spec = httpHdrRangeGetSpec(cpd->orig_range, &pos);
        if(cpd->splice_num == 0 && spec->offset > 0) 
        {
            cpd->splice_num = spec->offset / cpd->splice_size;
            debug(146,2)("mod_partition_store set cpd->splice_num %lld %s\n", cpd->splice_num, http->uri);
        }
        if (spec->offset < 0 && cpd->splice_num == 0)
        {    
            debug(146,2)("mod_partition_store unknown elength %s\n", http->uri); 
        } 
    }

    squid_off_t index_range_start = cpd->splice_num * cpd->splice_size;
    squid_off_t index_range_end = (cpd->splice_num +1 )* cpd->splice_size-1;

    char index_range_buf[100];
    snprintf(index_range_buf,100,"bytes=%" PRINTF_OFF_T "-%" PRINTF_OFF_T "",index_range_start,index_range_end);

    stringClean(&rpd->value);
    stringInit(&rpd->value, index_range_buf);
    debug(146,2)("mod_partition_store request->range %s %s\n",rpd->value.buf, http->uri);
    rpd->index_range = httpHdrRangeParseCreate(&rpd->value);

    squid_off_t req_range_start = 0; 
    squid_off_t req_range_end = 0;

    if(cpd->orig_range)
    {
        HttpHdrRangePos pos = HttpHdrRangeInitPos;
        const HttpHdrRangeSpec *spec;
        spec = httpHdrRangeGetSpec(cpd->orig_range, &pos);
        squid_off_t orig_offset = spec->offset;
        if (spec->offset < 0 && cpd->splice_num == 0)
            orig_offset = 0;
        req_range_start = orig_offset - cpd->splice_num * cpd->splice_size; 
        if (orig_offset <  cpd->splice_num * cpd->splice_size)
        req_range_start = 0; 
        if(spec->length < 0)
           req_range_end = cpd->splice_size -1;
        else
           req_range_end = orig_offset + spec->length > (cpd->splice_num+1)* cpd->splice_size ? cpd->splice_size - 1 : orig_offset + spec->length -1 - cpd->splice_num * cpd->splice_size;
    } 
    else 
    {
        req_range_start =  0 ;
        req_range_end = cpd->splice_size -1 ;
    }
    char req_range_buf[100];
    snprintf(req_range_buf,100,"bytes=%" PRINTF_OFF_T "-%" PRINTF_OFF_T "",req_range_start,req_range_end);
    if(http->request->range)
    {
        httpHdrRangeDestroy(http->request->range);
        http->request->range = NULL;
    }
    String str = StringNull;
    stringInit(&str, req_range_buf);
    http->request->range = httpHdrRangeParseCreate(&str);
    stringClean(&str);
    if (http->request->range)
      http->request->flags.range = 1;
    else
      http->request->flags.range = 0;
    httpHeaderDelById(&http->request->header,HDR_RANGE);
    httpHeaderAddEntry(&http->request->header, httpHeaderEntryCreate(HDR_RANGE,"Range",req_range_buf));
    debug(146,2)("mod_partition_store Range %lld + %s\n", (long long)cpd->splice_num * cpd->splice_size, req_range_buf);
    memset(&http->range_iter,0,sizeof(HttpHdrRangeIter));
    return 0;
}


static int
hook_check_range_spec(clientHttpRequest *http, HttpReply * rep)
{
	struct clientHttpRequest_pd * cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod); 
    if(http && http->request && rep && (rep->sline.status == 200 || rep->sline.status == 206))
    {
        debug(146,3)("mod_partition_store check_range_spec http Status %d %s\n",rep->sline.status, http->uri);
        if (http->reply)
          httpReplyDestroy(http->reply);
        http->reply = NULL;
        if(!cpd || http->request->method == METHOD_HEAD)
          return 0;
        if (cpd->bind == 1 && !cpd->elength)
        {
            if (rep->sline.status == 200 && rep->content_length > 0)
              cpd->elength = rep->content_length;
            if (rep->sline.status == 206 && rep->content_range && rep->content_range->elength > 0)
              cpd->elength = rep->content_range->elength;
            if (!cpd->elength)
            {
                debug(146, 1)("mod_partition_store can't known Content-Length sent Request To Server %s\n", http->uri);
                fd_table[http->conn->fd].cc_run_state[mod->slot] = 0; 
                cpd->bind = -1;
                storeClientUnregister(http->sc, http->entry, http);
                http->sc = NULL;
                storeUnlockObject(http->entry);
                httpHeaderDelById(&http->request->header,HDR_RANGE);
                if(cpd->orig_range)
                {
                    HttpHdrRangeSpec * spec = cpd->orig_range->specs.items[0];
                    if(spec && (spec->offset != -1 || spec->length != -1))
                    {
                        char value[100]; 
                        if (spec->offset == -1)
                          snprintf(value ,100,"bytes=-%" PRINTF_OFF_T "", spec->length);
                        else if(spec->length == -1)
                          snprintf(value ,100,"bytes=-%" PRINTF_OFF_T "", spec->offset);
                        else
                          snprintf(value ,100,"bytes=%" PRINTF_OFF_T "-%" PRINTF_OFF_T "",spec->offset, spec->offset + spec->length -1);
                        httpHeaderAddEntry(&http->request->header, httpHeaderEntryCreate(HDR_RANGE,"Range", value));
                        debug(146, 1)("mod_partition_store  Resume client Range : %s\n" ,value);
                    }
                    httpHdrRangeDestroy(cpd->orig_range);
                    cpd->orig_range = NULL;
                }
                clientProcessRequest(http);
                return 1;
            }
        }

        if(cpd->bind  == 1 && cpd->orig_range)
        {  
            cpd->bind = 0;
            debug(146,2)("mod_partition_store known Content-Length = %lld %s\n",(long long)cpd->elength, http->uri);
            if (httpHdrRangeCanonize(cpd->orig_range, cpd->elength))
            {
                HttpHdrRangeSpec * spec = cpd->orig_range->specs.items[0];
                if(cpd->splice_num != spec->offset / cpd->splice_size)
                {
                    cpd->splice_num = spec->offset / cpd->splice_size;
                    debug(146,2)("mod_partition_store Start spilce is %ld splice_size is %ld Range: bytes=%lld-%lld\n", (long)cpd->splice_num, (long)cpd->splice_size, (long long)spec->offset, (long long)spec->offset + spec->length - 1);
                    storeClientUnregister(http->sc, http->entry, http);
                    http->sc = NULL;
                    storeUnlockObject(http->entry);
                    clientProcessRequest(http);
                    return 1; 
                }
            }
            else
            {
                debug(146,2)("mod_partition_store Range is Error %s\n", http->uri);
                httpHdrRangeDestroy(http->request->range);
                http->request->range = NULL;
                http->sc->range = NULL;     
                http->sc->range_iter = NULL;
                httpHdrRangeDestroy(cpd->orig_range);
                cpd->orig_range = NULL;
                httpHeaderDelById(&rep->header,HDR_RANGE);
                if(cpd->splice_num)
                {    
                    debug(146,3)("mod_partition_store response 200 %s\n", http->uri);
                    cpd->splice_num = 0; 
                    storeClientUnregister(http->sc, http->entry, http);
                    http->sc = NULL;
                    storeUnlockObject(http->entry);
                    clientProcessRequest(http);
                    return 1;
                } 
            }
        }
        cpd->bind = 0;
/*
        if (cpd->fragment_req_No == 0)
        {
            cpd->first_fragment_reply = httpReplyClone(rep);
            cpd->first_fragment_log_type = http->log_type;
        }
*/
        StoreEntry * http_entry = http->entry;
        if (http_entry->lastmod != cpd->first_fragment_lastmod && cpd->fragment_req_No > 0) 
        {    
            cpd->should_release_req = 1;    
            debug(146, 4)("mod_partition_store-->: should_release_req was setted\n");
        }    
        if (http_entry->lastmod != cpd->first_fragment_lastmod && cpd->fragment_req_No == 0)
        {    
            cpd->first_fragment_lastmod = http_entry->lastmod;
        }    

        if((rep->sline.status == HTTP_MOVED_TEMPORARILY || rep->sline.status ==HTTP_MOVED_PERMANENTLY )&& cpd->header_sent)
        {     
            cpd->status_302 = 0; 
        }     
        else if(cpd->header_sent && (rep->sline.status >= 400 || rep->sline.status < 200))
        {     
            cpd->should_release_req = 1; 
            debug(146, 3)("mod_partition_store-->:should_release_req was setted for error sline status\n");
        }
        const char* url = urlCanonical(http->request);
        if (http->request->store_url)
          url = (http->request->store_url);
        debug(146,5)("mod_partition_store-->: header entry url is : %s\n",url);
        StoreEntry * headerE = storeGetHeaderEntryByUrl(url);
        if(headerE == NULL && cpd->status_302 == 0)
        {
            debug(146,3)("mod_partition_store-->: header entry is NULL, create it\n");
            storeAddHeaderEntry(url, rep, http);
        }
        else if(headerE && cpd->status_302 == 0)
        {
            if(headerE->lastmod < http_entry->lastmod)
            {
                debug(146,3)("mod_partition_store rep->lastmod is newer than headerE's Update headerEntry %s\n",http->uri);
                storeAddHeaderEntry(url, rep, http);
            }
        }

        if (cpd->header_sent && cpd->should_release_req)
        {
            debug(146,3)("mod_partition_store set COMM_ERR_CLOSING in clientWriteComplete for some Irreconcilable errors\n");
            clientWriteComplete(http->conn->fd, NULL,0,COMM_ERR_CLOSING,http);
            return 1;
        }
    }
    if(http && rep && cpd && cpd->fragment_req_No > 0 && (rep->sline.status != 200 && rep->sline.status != 206))
    {    
        debug(146,1)("mod_partition_store set COMM_ERR_CLOSING in clientWriteComplete Status %d\n", rep->sline.status);
        clientWriteComplete(http->conn->fd, NULL,0,COMM_ERR_CLOSING,http);
        return 1;
    } 
    return 0;
}


static int supportPurgeRefresh(clientHttpRequest * http)
{
    struct clientHttpRequest_pd * cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);
    if (cpd)
    {
        if (cpd->fragment_req_No == 0)
        {
            if (http->entry)
            {
                char * url;
                if (http->request->store_url)
                {
                    url = xstrdup(http->request->store_url);
                }
                else
                {
                    url = xstrdup(urlCanonical(http->request));
                }
                StoreEntry* header_e = storeGetHeaderEntryByUrl(url);
                if (!header_e)
                {
                    storeRelease(http->entry);
                    http->entry = NULL;
                    int count = 0;
                    struct partition_store_config * config = cc_get_mod_param(http->conn->fd, mod);
                    int splice_num = 0;
                    squid_off_t length = config->splice_size;
                    squid_off_t offset;
                    StoreEntry * e = NULL;
                    while (1)
                    {
                        if (count == 10)
                          break;
                        offset = (squid_off_t)splice_num * config->splice_size;
                        e = storeGet(storePrivateKeyByRangeSpec(url, offset, length));
                        if (e)
                        {
                            storeRelease(e);
                            debug(146,3)("mod_partition_store-->: force fragment %d miss for purge refresh\n", splice_num);
                            e = NULL;
                            count = 0;
                        }
                        else
                        {
                            count ++;
                        }
                        splice_num ++;
                    }
                    return LOG_TCP_MISS;
                }
                safe_free(url);
            }
        }
    }
    return 0;
}

/*  storeKeyPublicByRequestMethod , wo men zai zhe li jiang range tou jia ru dao ji suan key de yin su zhong*/
static int cc_storeKeyPublicByRequestMethod(SQUID_MD5_CTX *M,request_t *r)
{
    struct request_pd * rpd = cc_get_mod_private_data(REQUEST_T_PRIVATE_DATA,r,mod);
    if(rpd)
    {
        HttpHdrRange *index_range = rpd->index_range;
        if(index_range)
        {
            HttpHdrRangePos pos = HttpHdrRangeInitPos;
            const HttpHdrRangeSpec *spec;
            spec = httpHdrRangeGetSpec(index_range, &pos);
            debug(146,2)("mod_partition_store-->: cc_storeKeyPublicByRequestMethod: update key by range->offset (%" PRINTF_OFF_T "), range length (%" PRINTF_OFF_T ")\n",spec->offset,spec->length);
            SQUID_MD5Update(M, &(spec->offset), sizeof(squid_off_t));
            SQUID_MD5Update(M,&(spec->length),sizeof(squid_off_t));
        }
    }
    return 0;
}

/* before send header to client ,we should check if the header has already been sent , and modity the header consistent with the orig request , this function should be called in  clientCheckHeaderDone*/

static int set_status_line(clientHttpRequest *http, HttpReply *rep)
{

    struct clientHttpRequest_pd * cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);
    if (!cpd || !rep || cpd->header_sent || (rep->sline.status != 206 && rep->sline.status == 200))
        return 0;
    if (cpd->orig_range && cpd->orig_range && cpd->orig_range->specs.count > 1) 
    {
        fd_table[http->conn->fd].cc_run_state[mod->slot] = 0;
        http->request->flags.cachable = 0;
        return 0;
    }
    if ((cpd->orig_range == NULL || (http->request->method == METHOD_HEAD && cpd->HEADreq_run_state_set == 0)) && rep->sline.status == HTTP_PARTIAL_CONTENT)
    {
        debug(146,3)("mod_partition_store http sline.status is 206 and Not Range, set sline.status == 200\n");
        httpStatusLineSet(&rep->sline, rep->sline.version, HTTP_OK, NULL);
        if(cpd->first_fragment_reply)
          cpd->first_fragment_reply->sline.status = HTTP_OK;
    }
    if(NULL == cpd->orig_range) 
    {    
        HttpHeaderEntry *e = httpHeaderFindEntry(&rep->header, HDR_CONTENT_RANGE);
        char *s_mark = NULL;
        squid_off_t content_len = cpd->elength;
        if(content_len > 0)
        {
            httpHeaderDelById(&rep->header, HDR_CONTENT_LENGTH);
            httpHeaderDelById(&rep->header, HDR_CONTENT_RANGE);
            httpHeaderPutSize(&rep->header, HDR_CONTENT_LENGTH, content_len);
            debug(146,3)("mod_partition_store reply Content-Length: %ld\n", (long)content_len);
            return 0;
        }
        else if(e && (s_mark = strchr(e->value.buf, '/')) && *(s_mark+1) != '\0') 
        {
            char s_size[64];
            s_size[0] = '\0';
            strcat(s_size, s_mark+1);
            httpHeaderDelById(&rep->header, HDR_CONTENT_LENGTH);
            httpHeaderDelById(&rep->header, HDR_CONTENT_RANGE);
            httpHeaderAddEntry(&rep->header, httpHeaderEntryCreate(HDR_CONTENT_LENGTH, NULL, s_size));
            debug(146,3)("mod_partition_store reply Content-Length: %s\n", s_size);
            return 0;
            //httpHeaderPutSize(&rep->header, HDR_CONTENT_LENGTH, content_len);
        }
        httpHeaderDelById(&rep->header, HDR_CONTENT_RANGE);
        debug(146,3)("mod_partition_store no set reply Content-Length: d %ld| e %p\n", (long)content_len, e);
        return 0;
    }
    if(cpd->orig_range)
    {
        httpHeaderDelById(&rep->header, HDR_CONTENT_RANGE);
        httpHeaderDelById(&rep->header, HDR_CONTENT_LENGTH);
        HttpHdrRangeSpec * spec = cpd->orig_range->specs.items[0];
        squid_off_t content_len = spec->length > 0 ? spec->length : 0;
        char len[100];
        snprintf(len ,100,"bytes %" PRINTF_OFF_T "-%" PRINTF_OFF_T "/%" PRINTF_OFF_T "",spec->offset,spec->offset + spec->length -1, cpd->elength);
        httpHeaderAddEntry(&rep->header, httpHeaderEntryCreate(HDR_CONTENT_RANGE, NULL, len));
        httpHeaderPutSize(&rep->header, HDR_CONTENT_LENGTH, content_len);
        debug(146,3)("mod_partition_store reply Content-Range : %s\n", len);
    }
    return 0;
}

static int clientSendHeader(clientHttpRequest *http,MemBuf *mb)
{
    if(http && http->reply && (http->reply->sline.status == 200 || http->reply->sline.status == 206))
    {
        struct clientHttpRequest_pd * cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);
        if ((cpd && cpd->orig_range && cpd->orig_range && cpd->orig_range->specs.count > 1)|| http->request->method == METHOD_HEAD) 
        {    
            fd_table[http->conn->fd].cc_run_state[mod->slot] = 0; 
            http->request->flags.cachable = 0; 
            return 0;
        } 
        if(cpd && cpd->header_sent == 1)
          return 1;
        if(cpd) cpd->header_sent = 1;
    }
   return 0;
}

static int clientBeforeReplySendEnd(clientHttpRequest *http)
{
    if(http && http->reply && (http->reply->sline.status == 200 || http->reply->sline.status == 206))
    {
        struct clientHttpRequest_pd * cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);
        if(cpd)
        {
            int need_next_r=0;
            if(!cpd->status_302)
            {
                if (http->reply->sline.status == 200 || http->reply->sline.status == 206)
                {
                    cpd->splice_num++;
                    if(cpd->orig_range)
                    {
                        HttpHdrRangePos pos = HttpHdrRangeInitPos;
                        const HttpHdrRangeSpec *spec;
                        spec = httpHdrRangeGetSpec(cpd->orig_range, &pos);
                        squid_off_t elength =0;
                        if(cpd->elength >0 && cpd->elength< spec->offset + spec->length)
                          elength = cpd->elength;
                        else 
                          elength = spec->offset + spec->length;

                        if((cpd->splice_num *  cpd->splice_size) <=  elength -1 )
                          need_next_r =1;

                        if (!need_next_r)
                          debug(146,2)("mod_partition_store OrigRange = %" PRINTF_OFF_T "-%" PRINTF_OFF_T ", Content-Length : %" PRINTF_OFF_T "\n", spec->offset, spec->offset + spec->length - 1, spec->length);
                    }
                    else if(cpd->splice_num *  cpd->splice_size <= cpd->elength - 1 && (http->reply->sline.status == 200 || http->reply->sline.status == 206))
                      need_next_r = 1;
                }
            }
            if (cpd->fragment_req_No == 0 && http->reply && !cpd->first_fragment_reply)
            {
                cpd->first_fragment_reply = httpReplyClone(http->reply);
                cpd->first_fragment_log_type = http->log_type;
            }

            if(http->request->range)
            {
                const HttpHdrRangeSpec *spec = stackTop(&http->request->range->specs);

                debug(146,1)("mod_partition_store out.offset=%lld total_length %lld %s\n",
                        (long long)http->out.offset, (long long)spec->offset+spec->length + http->reply->hdr_sz, http->uri);

                if(http->out.offset != (spec->offset + spec->length + http->reply->hdr_sz))
                {
                    debug(146,1)("mod_partition_store offset=%lld length %lld  total_length %lld out.offset %lld %s\n",
                            (long long)spec->offset,(long long)spec->length,(long long)spec->offset+spec->length+http->reply->hdr_sz,(long long)http->out.offset,http->uri);
                    need_next_r = 0;
                }
            }
            if(need_next_r)
            {
                StoreEntry* e = http->entry;
                store_client *sc = http->sc;
                //sc->callback = NULL;
                storeClientUnregister(sc, e, http);
                storeUnlockObject(e);
                http->entry = NULL;//	 saved in e 
                http->sc = NULL;
                debug(146,2)("mod_partition_store-->:clientBeforeReplySendEnd : need send next splice %d %s\n", (int)cpd->splice_num ,http->uri);
                if(http->reply)
                {
                    httpReplyDestroy(http->reply);
                    http->reply = NULL;
                }
                http->request->lastmod = -1;
                http->flags.done_copying = 0; //clear the done_copying flag
                cpd->fragment_req_No += 1;
                clientProcessRequest(http);
                return 1;
            }
            debug(146,2)("mod_partition_store-->:clientBeforeReplySendEnd : has completed response %" PRINTF_OFF_T " x %" PRINTF_OFF_T " Content-Length : %" PRINTF_OFF_T " %s\n", cpd->splice_num, cpd->splice_size, cpd->elength, http->uri);
            /*
            if (http->reply && cpd->first_fragment_reply)
            {
                httpReplyDestroy(http->reply);
                http->reply = cpd->first_fragment_reply;
            }
            http->log_type = cpd->first_fragment_log_type;
            */
        }
    }
    return 0;
}
static void 
hook_restore_range(clientHttpRequest *http)
{
    if(http)
    {
        httpHeaderDelById(&http->request->header, HDR_RANGE);
        struct clientHttpRequest_pd * cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);
        if(cpd && cpd->orig_s_range)
        {
            debug(146,2)("mod_partition_store clientBeforeReplySendEnd  Add Orig Range %s\n", http->uri);
            httpHeaderAddEntry(&http->request->header, httpHeaderEntryCreate(HDR_RANGE, NULL, cpd->orig_s_range));
        }
        if (cpd && cpd->first_fragment_reply)
        {
            if(http->reply)
              httpReplyDestroy(http->reply);
            http->reply = cpd->first_fragment_reply;
            cpd->first_fragment_reply = NULL;
        }
        if(cpd && cpd->first_fragment_log_type)
        {
            http->log_type = cpd->first_fragment_log_type;
        }
    }
}
static int 
hook_send_partion(HttpStateData *httpState, HttpHeader* hdr)
{
    if (!httpState || !httpState->orig_request|| httpState->orig_request->method == METHOD_HEAD)
        return 0;
    struct request_pd * rpd = cc_get_mod_private_data(REQUEST_T_PRIVATE_DATA, httpState->orig_request,mod);
    struct partition_store_config * cfg = cc_get_mod_param(httpState->fd,mod);
    if(rpd && rpd->value.buf && cfg)
	{
        httpHeaderDelById(hdr, HDR_RANGE);
        httpHeaderDelById(hdr, HDR_REQUEST_RANGE);
        debug(146,2)("mod_partition_store Send %s to Server\n", rpd->value.buf);
        httpHeaderAddEntry(hdr, httpHeaderEntryCreate(HDR_RANGE, "Range", rpd->value.buf)); 
	}
	return 0;
}

static int make_range_cacheable()
{
	debug(146,2)("mod_partition_store-->: make_range_cacheable: enter\n");
	return 1;
}

static int change_entry_values( clientHttpRequest *http, StoreEntry *e)
{
    debug(146,5)("mod_partition_store-->: change_entry_values: uri is %s , mem->url is: %s , store_url = %s , http->request->urlcannoical is %s \n",http->uri,e->mem_obj->url,e->mem_obj->store_url,urlCanonical(http->request) );
	char* url;
	if (http->request->store_url)
	{
		url = xstrdup(http->request->store_url);
	}
	else    
	{
		url = xstrdup(urlCanonical(http->request));
	}

	StoreEntry* header_e = storeGetHeaderEntryByUrl(url);
	safe_free(url);
	if(header_e)
	{
		struct clientHttpRequest_pd *cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);
		assert(cpd);
		if (cpd->fragment_req_No == 0)
		{
			if (e->lastmod < header_e->lastmod)		
			{
				debug(146, 3)("first fragment checking expire: return 3 for e->lastmod < header_e->lastmod\n");
				return 3;
			}
			else if (e->lastmod > header_e->lastmod)
			{
				header_e->lastmod = e->lastmod;
				header_e->timestamp = e->timestamp;
				header_e->expires = e->expires;
				debug(146, 3)("first fragment checking expire: return 0 for e->lastmod > header_e->lastmod\n");
				return 0;
			}
			else
			{
				e->lastmod = header_e->lastmod;
				e->timestamp = header_e->timestamp;
				e->expires = header_e->expires;
				debug(146, 3)("first fragment checking expire: return 0\n");
				return 0;
			}
		}
		else
		{
			if (cpd->first_fragment_lastmod == e->lastmod)
			{
				debug(146, 3)("other fragment checking expire:return 2 cpd->first_fragment_lastmod == e->lastmod\n");
				return 2;
			}		
			else if (cpd->first_fragment_lastmod < e->lastmod)
			{
				cpd->should_release_req = 1;
                debug(146, 3)("other fragment checking expire:return 2 cpd->first_fragment_lastmod < e->lastmod");
				return 2;
			}
            else if (cpd->first_fragment_lastmod > e->lastmod && cpd->first_fragment_lastmod != header_e->lastmod)
			{
				cpd->should_release_req = 1;
				debug(146, 3)("other fragment checking expire:return 2 cpd->first_fragment_lastmod > e->lastmod\n");
				return 2;
			}
			else
			{
				debug(146, 3)("other fragment checking expire: return 3\n");
				return 3;	
			}
		}
	}
	else
	{
		debug(146, 3)("fragment checking expire: return 1 for headerE == NULL\n");
		return 1;
	}
}

void ignoreIMSHdrOfNext_r(clientHttpRequest *http)
{
	 struct clientHttpRequest_pd *cpd = cc_get_mod_private_data(REQUEST_PRIVATE_DATA,http,mod);
	 if (cpd)
	 {
		if (cpd->fragment_req_No > 0)
		{
			if (http->is_modified == 0)
				http->is_modified = -1;
		}
	 }
}

static int private_clientBuildRangeHeader(clientHttpRequest *http)
{
	debug(146,4)("mod_partition_store-->: private_clientBuildRangeHeader: enter\n");
	return 1;
}

static int should_close_fd(clientHttpRequest *http)
{
	return 1;
}
static int process_excess_data(HttpStateData *httpState)
{
	debug(146,3)("process_excess_data: enter\n");
	fwdComplete(httpState->fwd);
	return 1;
}

static int checkModPartitionStoreEnable(void)
{
	return 1;
}

// module init 
int mod_register(cc_module *module)
{
	debug(146, 1)("(mod_partition_store) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
    //parse configure Param	
    cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param), 
                func_sys_parse_param);
    //Support Purge Refresh of Client Side
    cc_register_hook_handler(HPIDX_hook_func_private_http_req_process2,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_req_process2),
                supportPurgeRefresh);
    //Store origin range if request has 
    cc_register_hook_handler(HPIDX_hook_func_http_req_read_three,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read_three), 
                clientHttpRequestCreateOrigRange);
    //Process partion range
    cc_register_hook_handler(HPIDX_hook_func_before_clientProcessRequest2,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_clientProcessRequest2), 
                beforeClientProcessRequest2);
    //Modify Reply Sline 
    cc_register_hook_handler(HPIDX_hook_func_private_set_status_line,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_set_status_line),
                set_status_line);
    //Judge whether response complete
    cc_register_hook_handler(HPIDX_hook_func_private_http_repl_send_end,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_repl_send_end),
                clientBeforeReplySendEnd);
    //Find StoreEntry base on Size
    cc_register_hook_handler(HPIDX_hook_func_storeKeyPublicByRequestMethod,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_storeKeyPublicByRequestMethod),
                cc_storeKeyPublicByRequestMethod);

    /*---------Process when Object is missed----------------------------*/
    //Send range header to server
    cc_register_hook_handler(HPIDX_hook_func_http_req_send_exact,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_send_exact), 
                hook_send_partion);

    //Store Range Response
    cc_register_hook_handler(HPIDX_hook_func_private_store_partial_content,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_store_partial_content), 
                make_range_cacheable);

    cc_register_hook_handler(HPIDX_hook_func_private_process_excess_data,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_process_excess_data),
                process_excess_data);
    //Range request Support
    cc_register_hook_handler(HPIDX_hook_func_private_clientBuildRangeHeader,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_clientBuildRangeHeader), 
                private_clientBuildRangeHeader);
    /*-----------------------Process HIT------------------------*/
    //Update HeaderEntry
    cc_register_hook_handler(HPIDX_hook_func_private_client_cache_hit_start,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_cache_hit_start),
                change_entry_values);
    //Process Condition Request
    cc_register_hook_handler(HPIDX_hook_func_private_checkModPartitionStoreEnable,
                module->slot,
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_checkModPartitionStoreEnable),
                checkModPartitionStoreEnable);
    //wo should close client fd when response partion Object 
    cc_register_hook_handler(HPIDX_hook_func_private_should_close_fd,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_should_close_fd), 
                should_close_fd);

    cc_register_hook_handler(HPIDX_hook_func_send_header_clone_before_reply,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_send_header_clone_before_reply),
                hook_check_range_spec);

    cc_register_hook_handler(HPIDX_hook_func_private_before_send_header,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_before_send_header), 
                clientSendHeader);
    //-------------------------- IMS------------------------------------
    //IMS 304 Respons 
    cc_register_hook_handler(HPIDX_hook_func_private_getReplyUpdateByIMSRep,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_getReplyUpdateByIMSRep), 
                getReplyUpdateByIMSRep);
    //IMS Process Next
    cc_register_hook_handler(HPIDX_hook_func_client_process_hit_start,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_process_hit_start), 
                ignoreIMSHdrOfNext_r);

    cc_register_hook_handler(HPIDX_hook_func_private_change_accesslog_info,
                module->slot, 
                (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_change_accesslog_info), 
                hook_restore_range);
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}
#endif

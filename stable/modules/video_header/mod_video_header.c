#include "cc_framework_api.h"
#include <stdbool.h>

#ifdef CC_FRAMEWORK

void memFree(void *p, int type);

CBDATA_TYPE(hlv_state);
void clientCacheHit(void *data, HttpReply * rep);
void clientProcessRequest(clientHttpRequest *);
static cc_module *mod = NULL;

typedef enum{
	HIT_FLV,
	HIT_MP4,
	HIT_OTHER,
	MISS
} media_type;

struct mod_config
{
	//int	type;// 0 means flv, 1 means mp4
	char   header[128];
};


typedef struct _request_param
{
	int flag;
	media_type is_real_flv;
	int succ;
	int is_http_miss_wait;

} request_param;

typedef struct _store_param
{
	char *flv_pre;
	int flv_len;
	clientHttpRequest *http;
        int store_complete_count;
        int cc_delay_status;
        int is_cbdatalock;

} store_param;

typedef struct _hlv_state
{
	clientHttpRequest *http;
	StoreEntry *entry;	
	store_client *sc;
	char *buf;
	size_t buf_size;
	size_t offset;	
	STLVCB *callback;
	void *callback_data;
} hlv_state;

static int free_cfg(void *data)
{
	safe_free(data);
	return 0;
}


static int free_hit_param(void* data)
{
	hlv_state *state = (hlv_state *) data;
	if(state != NULL){
		cbdataUnlock(state->callback_data);

		if (state->sc)
		{
			storeClientUnregister(state->sc, state->entry, state);
			state->sc = NULL;
		}
		if (state->entry)
		{
			storeUnlockObject(state->entry);
			state->entry = NULL;
		}

		if (state->buf)
		{
			memFree(state->buf, MEM_STORE_CLIENT_BUF);
			state->buf = NULL;
		}
		state->callback = NULL;
		state->callback_data = NULL;

		cbdataFree(state);
		state = NULL;
	}
	return 0;
}


static int free_store_param(void* data)
{
	store_param *param = (store_param *) data;
	if(param != NULL){
		param->store_complete_count = 0;
                param->is_cbdatalock = 0;
                param->http = NULL;
		safe_free(param->flv_pre);
		safe_free(param);
		param = NULL;
	}
	return 0;
}

static int free_request_param(void* data)
{
	request_param *req_param = (request_param *) data;
	if(req_param != NULL){
		safe_free(req_param);
		req_param = NULL;
	}
	return 0;
}

static void revert_int(char *s, char *d, int len)
{
	int i = 0;
	for(i = len -1 ; i >= 0; i--) {
		*(s+i) = *d;
		d++;
	}
}

static int flvHandleRangeHeaderForBytes(request_t *req, int range_len)
{
	//handle range message
        char range_value[64];
        HttpHeaderEntry *range = NULL;
        HttpHeaderEntry e;

	if(req == NULL){
		debug(140, 2)("mod_video_header: http request is null in flvHandleRangeHeader \n");
		return -1;
	}

        range = httpHeaderFindEntry(&req->header, HDR_RANGE);
        if(range != NULL){
                debug(140, 3)("mod_video_header:  range for request and not must add range header: %s \n",req->store_url);
                return -1;
        }

        snprintf(range_value, 31, "bytes=%d-%d", 0, range_len);
        e.id = HDR_RANGE;
        stringInit(&e.name, "Range");
        stringInit(&e.value, range_value);

        debug(140, 3)("mod_video_header:  range for request header: %s \n",range_value);
        httpHeaderAddEntry(&req->header, httpHeaderEntryClone(&e));

        if (req->range){
                httpHdrRangeDestroy(req->range);
        }
 
	//httpHdrRangeDestroy(req->range);
        req->range = NULL;
        req->range = httpHeaderGetRange(&req->header);

        stringClean(&e.name);
        stringClean(&e.value);
	return 0;

}

static void storeMp4CopyCallBack(void *data, char *buf, ssize_t size)
{
	debug(140, 9)("mod_video_header: Starting mp4 copy callback...\n");
	hlv_state *state = data;
	clientHttpRequest *http = state->http;

	if (!cbdataValid(state->callback_data)){
		debug(140, 3)("mod_video_header: cbdataValid is 0 and not find the video header \n");
		free_hit_param(state);
                return;
		//goto free_hit_params;
	}

	char rbuf[32] = {0};
	if( size < 0 ){
                debug(140, 3)("mod_f4v: f4v cache object is error\n");
                goto free_hit_params;
        }

	size_t http_len = headersEnd(buf, size);

	char *mp4_buf = &buf[http_len];

	//read ftyp atom
	size_t video_header_size = 0;

	if(mp4_buf[4] != 'f' || mp4_buf[5] != 't' || mp4_buf[6] != 'y' || mp4_buf[7] != 'p'){
                debug(140, 3)("mod_video_header: open the cache object is not mp4 ftyp format %s \n",http->uri);
                goto free_hit_params;
        }

	int ftyp_size = 0;
	revert_int(rbuf, &mp4_buf[0], 4);
	memcpy(&ftyp_size, &rbuf, 4);

	debug(140, 5)("mod_video_header: ftyp len  %ld\n",(long int)ftyp_size);

	if(ftyp_size + http_len + 8 > size){
		debug(140, 3)("mod_video_header: ftyp data size %lu > buf size  %ld \n",(unsigned long)ftyp_size,(long int)size);
		goto free_hit_params;
	
	}

	video_header_size = ftyp_size;
	//read moov atom
	if(mp4_buf[ftyp_size + 4] != 'm' || mp4_buf[ftyp_size +5] != 'o' || mp4_buf[ftyp_size+6] != 'o' || mp4_buf[ftyp_size+7] != 'v'){
                debug(140, 3)("mod_video_header: open the cache object is not mp4 moov format %s \n",http->uri);
                goto free_hit_params;
        }

	int moov_size = 0;
	revert_int(rbuf, &mp4_buf[ftyp_size], 4);
	memcpy(&moov_size, &rbuf, 4);
	if(moov_size <= 0){
		debug(140, 3)("mod_video_header: moov data size %lu > buf size  %ld \n",(unsigned long)moov_size,(long int)size);
		goto free_hit_params;
	
	}

	debug(140, 5)("mod_video_header: moov len  %ld\n",(long int)moov_size);
	video_header_size += moov_size;
	video_header_size = video_header_size -1;

	debug(140, 3)("mod_video_header: mp4 video header len  %ld\n",(long int)video_header_size);
	//get stroreEntry private data
	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, state->entry, mod);

	if(str_param != NULL) {
		str_param->flv_pre = NULL;
		str_param->flv_len = video_header_size;
	}


	if(flvHandleRangeHeaderForBytes(http->request, video_header_size)){
		safe_free(str_param->flv_pre);
		str_param->flv_len = 0;
	}

/*	http->sc->callback = NULL;
	http->sc->flags.disk_io_pending = 0;
        if(http->sc->copy_buf != NULL){
                 debug(140,2)("clean the sc->copy_buf \n");
                 memFree(http->sc->copy_buf, MEM_STORE_CLIENT_BUF);
        }
*/	
	storeClientCopyHeaders(http->sc, http->entry,
                                       clientCacheHit,
                                       http);
	
	free_hit_param(state);
	return;

	free_hit_params:
		storeClientCopyHeaders(http->sc, http->entry,
                                       clientCacheHit,
                                       http);
	
		free_hit_param(state);


	
}

static void storeHlvCopyCallBack(void *data, char *buf, ssize_t size)
{
	debug(140, 9)("mod_video_header: Starting flv copy callback...\n");
	hlv_state *state = data;
	clientHttpRequest *http = state->http;

	if (!cbdataValid(state->callback_data)){
		debug(140, 3)("mod_video_header: cbdataValid is 0 and not find video header \n");
		free_hit_param(state);
                return;
	///	goto free_hit_params;
	}

	char rbuf[32] = {0};
	if( size < 0 ){
                debug(140, 3)("mod_f4v: f4v cache object is error\n");
                goto free_hit_params;
        }

	size_t http_len = headersEnd(buf, size);

	char *flv_buf = &buf[http_len];

	//read script data about tag12 onMetadata
	int first = 0;
	int head_sz = 13;
	int tag_size = 15;
	uint8_t sort_flag = 0;
	size_t video_header_size = 0;

	if(flv_buf[first] != 'F' || flv_buf[first+1] != 'L' || flv_buf[first+2] != 'V'){
		debug(140, 3)("mod_video_header: hlv open the cache object is not flv format %s \n",http->uri);
		goto free_hit_params;
	}

	int8_t script = 0;
	memcpy(&script, &flv_buf[first+13], 1);
	int script_size = 0;
	revert_int(rbuf, &flv_buf[first+14], 3);
	memcpy(&script_size, &rbuf, 3);
	
	debug(140, 3)("storeHlvCopy: script size = %d \n",script_size);
	//jump script data and read AVCDecoderConfigurationRecord data	 
	size_t second = script_size + head_sz + tag_size;
	video_header_size += second;

	if(second + http_len > size){
		debug(140, 3)("mod_video_header: script data size %lu > buf size  %ld \n",(unsigned long)second+http_len,(long int)size);
		goto free_hit_params;
	
	}

	//read video control data
	memcpy(&sort_flag, &flv_buf[second], 1);
	if(sort_flag !=8 && sort_flag != 9){
		debug(140, 3)("mod_video_header: not flv video tag type 8 or 9 %d \n",sort_flag);
		goto free_hit_params;
	}

	int tag1_size = 0;
	int local = second + 1;
	revert_int(rbuf, &flv_buf[local], 3);
	memcpy(&tag1_size, &rbuf, 3);

	//read leaf audio control data 
	local += tag1_size + tag_size;

	if(local + http_len > size){
		debug(140, 3)("mod_video_header: tag1 data size %lu > buf size  %ld \n",(unsigned long)local+http_len,(long int)size);
		goto free_hit_params;
	
	}
	
	int tag2_size = 0;
	memcpy(&sort_flag, &flv_buf[local-1], 1);
	if(sort_flag !=8 && sort_flag != 9){
		debug(140, 3)("mod_video_header: not hlv left audio tag type 8 or 9 %d \n",sort_flag);
		goto free_hit_params;
	}
	revert_int(rbuf, &flv_buf[local], 3);
	memcpy(&tag2_size, &rbuf, 3);

	//read right audio control data

	int tag3_size = 0;

	local += tag2_size +tag_size;

	if(local + http_len > size){
		debug(140, 3)("mod_video_header: tag2 data size %lu > buf size  %ld \n",(unsigned long)local+http_len,(long int)size);
		goto free_hit_params;
	}

	memcpy(&sort_flag, &flv_buf[local-1], 1);
	if(sort_flag !=8 && sort_flag != 9){
		debug(140, 3)("mod_video_header: not hlv left audio tag type 8 or 9 %d \n",sort_flag);
		goto free_hit_params;
	}

	revert_int(rbuf, &flv_buf[local], 3);
	memcpy(&tag3_size, &rbuf, 3);

	video_header_size += tag1_size + tag_size;
	video_header_size += tag2_size + tag_size;
	video_header_size += tag3_size + tag_size;
	video_header_size = video_header_size -1;


	debug(140, 5)("mod_video_header: flv video len  %ld\n",(long int)video_header_size);
	//get stroreEntry private data
	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, state->entry, mod);

	if(str_param != NULL) {
		str_param->flv_pre = NULL;
		str_param->flv_len = video_header_size;
	}


	if(flvHandleRangeHeaderForBytes(http->request, video_header_size)){
		safe_free(str_param->flv_pre);
		str_param->flv_len = 0;
	}

	//http->sc->callback = NULL;
	storeClientCopyHeaders(http->sc, http->entry,
                                       clientCacheHit,
                                       http);
	
	free_hit_param(state);
	return;

	free_hit_params:
		storeClientCopyHeaders(http->sc, http->entry,
                                       clientCacheHit,
                                       http);
		free_hit_param(state);

}

static int hook_get_hlv_header(clientHttpRequest *http){
	
	StoreEntry *entry = http->entry;
	if(entry == NULL){
		return 0;
	}
	
	request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if( req_param == NULL ) {
		return 0;
	}

	//if(req_param->is_real_flv != HIT_FLV || req_param->flag == 0){
	if(req_param->flag == 0){
		return 0;
	}
	
	
	if( req_param->flag == 2 )
		return 0;

	assert(req_param->flag == 1);
	
	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	if(cfg == NULL){
		return 0;
	}


	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, http->entry, mod);

	if(str_param == NULL || str_param->flv_len == 0) {

		str_param = xmalloc(sizeof(store_param));
		memset(str_param,0,sizeof(store_param));

		str_param->flv_len = 0;
		str_param->flv_pre = NULL;

		debug(140, 3)("video code is null and must read cache file from disk \n");
		cc_register_mod_private_data(STORE_ENTRY_PRIVATE_DATA, http->entry, str_param, free_store_param, mod);
		CBDATA_INIT_TYPE(hlv_state);
		//hlv_state *state = xmalloc(sizeof(hlv_state));	
		hlv_state *state = cbdataAlloc(hlv_state);
		state->entry = http->entry;
        	storeLockObject(state->entry);
		state->callback_data = http;
		state->callback = NULL;
		cbdataLock(http);
	
		state->http = http;
		state->buf  = memAllocate(MEM_STORE_CLIENT_BUF);
		state->buf_size = STORE_CLIENT_BUF_SZ;
		state->sc = storeClientRegister(state->entry, state);
		state->offset = 0;

		if(req_param->is_real_flv == HIT_FLV){
			storeClientCopy(state->sc, state->entry, state->offset, state->offset, state->buf_size,state->buf, storeHlvCopyCallBack, state);
		}
		else if(req_param->is_real_flv == HIT_MP4){
			storeClientCopy(state->sc, state->entry, state->offset, state->offset, state->buf_size,state->buf, storeMp4CopyCallBack, state);
			
		}
		return 1;
	}
	//if(str_param->flv_len != NULL){
	flvHandleRangeHeaderForBytes(http->request, str_param->flv_len);

	return 0;
	
}

static int hook_init()
{
	debug(140, 1)("(mod_video_header) ->	hook_init:----:)\n");
	return 0;
}


static int hook_cleanup(cc_module *module)
{
	debug(140, 1)("(mod_video_header) ->	hook_cleanup:\n");
	return 0;
}

static void modify_request(clientHttpRequest * http,char *new_uri){

	debug(140, 5)("mod_video_header: start, uri=[%s]\n", http->uri);
	request_t* old_request = http->request;
	
	request_t* new_request = urlParse(old_request->method, new_uri);

	debug(140, 5)("mod_video_header: start, new_uri=[%s]\n", new_uri);
	if (new_request)
	{
		safe_free(http->uri);
		http->uri = xstrdup(urlCanonical(new_request));
		safe_free(http->log_uri);
		http->log_uri = xstrdup(urlCanonicalClean(old_request));
		new_request->http_ver = old_request->http_ver;
		httpHeaderAppend(&new_request->header, &old_request->header);
		new_request->client_addr = old_request->client_addr;
		new_request->client_port = old_request->client_port;
#if FOLLOW_X_FORWARDED_FOR
		new_request->indirect_client_addr = old_request->indirect_client_addr;
#endif // FOLLOW_X_FORWARDED_FOR 
		new_request->my_addr = old_request->my_addr;
		new_request->my_port = old_request->my_port;
		new_request->flags = old_request->flags;
		new_request->flags.redirected = 1;
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

		safe_free(old_request->store_url);

		requestUnlink(old_request);
		http->request = requestLink(new_request);
	}

}


//safeguard chain: 1 remove ip 2 remove the part after "?"
static char *store_qq_key_url(char* url){

        char *new_url = xstrdup(url);
        int has_302_domain = 0;

        char *str = "http://";
        char *p = strstr(new_url,str);
        if ( NULL == p)
        {
                goto fail;
        }

        p = p + 7;
        char hostname[128];
        memset(hostname,0,128);

       //host name
        char *tmp1 = NULL;
        tmp1 = strchr(p,'/');
        if ( NULL == tmp1 )
        {
                goto fail;
        }
        strncpy(hostname,p,tmp1-p);

        debug(140, 3)("mod_video_header:hostname =%s \n",hostname);
        if(strstr(hostname,"ccgslb.net") != NULL){
                has_302_domain = 1;
        }

        tmp1++;

        char *tmp2 = NULL;
        char *tmp3 = NULL;

        if(has_302_domain){
                tmp2 = strchr(tmp1,'/');
                if ( NULL == tmp2 )
                {
                        goto fail;
                }

                tmp2++;
		tmp3 = strchr(tmp2,'/');
		if ( NULL == tmp3 )
		{
		     goto fail;
		}
		tmp3++;	

       }
        else{
                 tmp3 = tmp1;
        }

        char *quesurl = strchr(tmp3,'?');
        if(quesurl != NULL){
                *quesurl = 0;
        }

        char *result_url = xstrdup(tmp3);
        debug(140, 3)("mod_video_header : store_url=%s \n",result_url);
        safe_free(new_url);
        return result_url;

fail:
        safe_free(new_url);
        return NULL;

}

static char *remove_cpis_domain_key_url(char* url){

        char new_url[1024];
        memset(new_url,0,1024);


        char *str = "http://";
        char *p = strstr(url,str);
        if ( NULL == p)
        {
                goto fail;
        }

        p = p + 7;
        char hostname[128];
        memset(hostname,0,128);

       //host name
        char *tmp1 = NULL;
        tmp1 = strchr(p,'/');
        if ( NULL == tmp1 )
        {
                goto fail;
        }
        strncpy(hostname,p,tmp1-p);

        if(strstr(hostname,"ccgslb.net") != NULL && strstr(hostname,"cpis.") != NULL){

                memcpy(new_url, str,7);
                strcat(new_url,tmp1+1);
        }
	else{
		return NULL;
	}

        return xstrdup(new_url);

fail:
        return NULL;

}


static int add_range(clientHttpRequest *http)
{
	debug(140, 3)("(mod_video_header) -> read request finished, add range: \n");

	if( !http || !http->uri || !http->request ) {
		debug(140, 3)("http = NULL, return \n");
		return 0;
	}
		
	if((strstr(http->uri, "type=flv")) == NULL && strstr(http->uri, "type=mp4") == NULL) {
		debug(140, 3) ("not find type=flv|mp4 return %s \n",http->uri);
		return 0;
	}

	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);

    	debug(140, 5) ("mod_video_header: header paramter %s \n",cfg->header);
	char *tmp_header = strstr(http->uri, cfg->header);

	if( tmp_header == NULL ) {
		debug(140, 3) ("not find key header and return %s \n",cfg->header);
		return 0;
	}

	char *storeurl = xstrdup(http->uri); 
  	char *s;
        char *e;

        s = strstr(storeurl, cfg->header);
        e = strstr(s, "&");
        if( e == 0 )
                *(s-1) = 0;
        else {
                memmove(s, e+1, strlen(e+1));
                *(s+strlen(e+1)) = 0;
        }

	safe_free(http->request->store_url);
        http->request->store_url = store_qq_key_url(http->uri);


	char *new_url = remove_cpis_domain_key_url(storeurl);
        if(new_url != NULL){
                modify_request(http,new_url);
                safe_free(new_url);
        }
	else{
		modify_request(http,storeurl);
	}

	safe_free(storeurl);

	request_param *req_param = xmalloc(sizeof(request_param));
        memset(req_param,0,sizeof(request_param));
        cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, req_param, free_request_param, mod);
	
	req_param->flag = 1;
	req_param->succ = -1;

	if((strstr(http->uri, "type=flv")) != NULL){
		req_param->is_real_flv = HIT_FLV;
	}
	else if((strstr(http->uri, "type=mp4")) != NULL){
		req_param->is_real_flv = HIT_MP4;
	}

    	debug(140, 5) ("mod_video_header: header paramter %s \n",cfg->header);
    	debug(140, 5) ("mod_video_header: paramter is_real_flv= %d \n", req_param->is_real_flv);	

	return 0;
}

static int mod_reply_header(clientHttpRequest *http)
{
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		return 0;
	}

	//if(req_param->is_real_flv != HIT_FLV || req_param->flag == 0){
	if(req_param->flag == 0){
		return 0;
	}
	
	assert(req_param->flag == 1);
	
	HttpReply *rep = http->reply;

	if( rep->sline.status == HTTP_PARTIAL_CONTENT )
		rep->sline.status = HTTP_OK;

	return 0;
}


static int func_sys_parse_param(char *cfg_line)
{
	if( strstr(cfg_line, "allow") == NULL )
		return 0;

	if( strstr(cfg_line, "mod_video_header") == NULL )
		return 0;

	char *token = NULL;

	if ((token = strtok(cfg_line, w_space)) == NULL)
		return 0;


	struct mod_config *cfg = xmalloc(sizeof(struct mod_config));
	memset(cfg, 0, sizeof(struct mod_config));
	
	
	token = strtok(NULL, w_space);
	strcpy(cfg->header,token);

    	debug(140, 5) ("mod_video_header: paramter %s \n",cfg->header);
	
	cc_register_mod_param(mod, cfg, free_cfg);
	return 0;
}

static int is_disable_sendfile(store_client *sc, int fd) 
{
	sc->is_zcopyable = 0;
	return 1;
}


static void restore_http(clientHttpRequest *http){

	request_t *req = http->request;	
	if(req == NULL){
                debug(140, 2)("mod_video_header: http request is null in restore_http \n");
                return;
        }       
        
	HttpHeaderEntry *range = NULL;
        range = httpHeaderFindEntry(&req->header, HDR_RANGE);
        if(range != NULL){
                
        	httpHeaderDelById(&req->header, HDR_RANGE);
        }
	return;

}
static int mod_video_header_http_req_process(clientHttpRequest *http){
	
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		return 0;
	}

	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	assert(cfg);
	if (http->log_type == LOG_TCP_MISS && req_param->succ == 1){
                debug(140,3)("mod_video_header: reqeust whole mp4 file is fail and starting drag\n");
                req_param->flag = 0;

        }           	
	else if (http->log_type == LOG_TCP_MISS ){
		debug(140,3)("mod_video_header: reques store_url and %s miss to web server \n",http->request->store_url);
		req_param->flag = 0;
		restore_http(http);
		//req_param->is_http_miss_wait = 1;
		req_param->is_http_miss_wait = 0;
		/*char new_uri[1024];
		if(strstr(http->uri,"?") == NULL){
			
			snprintf(new_uri,1024,"%s%s%s%s%lld",http->uri,"?",cfg->start,"=",req_param->mp4_start);
		}
		else{
			snprintf(new_uri,1024,"%s%s%s%lld",http->uri,cfg->start,"=",req_param->mp4_start);
		}

		modify_request(http,new_uri);*/
	}
	else
	{
		debug(140,3)("can not enter helper because http->log_type=%d\n ", http->log_type);
                StoreEntry *e = NULL;
                e = storeGetPublic(http->request->store_url,METHOD_GET);

                if(e == NULL|| e->store_status != STORE_OK || e->swap_status != SWAPOUT_DONE){

                        debug(140, 2) ("mod_mp4: mp4 pending and redirect to upper layer to query start data \n");
                        req_param->flag = 0; //add for internal miss query whole implement
                        //http->request->flags.cachable = 0;//no cache
                        //modify_request(http,req_param->uri);
			restore_http(http);
                        http->entry = NULL;
                        http->log_type = LOG_TCP_MISS;

                }
                else{
                        debug(140, 3) ("mp4 hit request http->uri = %s\n", http->uri);
                }

               return 0;
	}

	debug(140,3)("mod_video_header: request param new uri=%s\n ",http->uri);
	return 0;
}
static int checkHasMp4(clientHttpRequest *http)
{
        /*mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
        if(cfg->website != QQ){
                return 0;
        }
*/
        if(strstr(http->uri,".mp4") == NULL){
                return 0;
        }

         //get store_entry and path

        StoreEntry *e = NULL;
        char * uri = store_qq_key_url(http->uri);
        debug(140,3)("mod_video_header :check has mp4:, modify uri=[%s]\n" ,uri);
        e = storeGetPublic(uri,METHOD_GET);
        safe_free(uri);

        if(e == NULL){
                char *new_uri= xstrdup(http->uri);
                char *tmp_uri = strstr(new_uri,".mp4");
                tmp_uri++;
                *tmp_uri = 'f';
                tmp_uri++;
                *tmp_uri = 'l';
                tmp_uri++;
                *tmp_uri = 'v';
                debug(140, 3) ("mod_video_header: modify request http->uri = %s\n", new_uri);

                modify_request(http,new_uri);

                safe_free(new_uri);
        }
        else{
                debug(140, 3) ("mod_video_header:mp4 hit request http->uri = %s\n", http->uri);
        }

        return 0;

}

/*
static int client_create_store_entry(clientHttpRequest *http, StoreEntry *e){

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL  || req_param->is_http_miss_wait != 1){
		return 0;
	}

	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, e, mod);

        if(str_param == NULL) {
              
               str_param = xmalloc(sizeof(store_param));
               memset(str_param,0,sizeof(store_param));

               str_param->store_complete_count = 1;
	       //str_param->cc_delay_status = 1;
               str_param->http = http;
                
               cc_register_mod_private_data(STORE_ENTRY_PRIVATE_DATA, e, str_param, free_store_param, mod);
	}
	else{
                str_param->store_complete_count = 1;
	        //str_param->cc_delay_status = 1;
		str_param->http = http;
	}

        str_param->flv_len = 0;
	str_param->is_cbdatalock = 1;
	cbdataLock(str_param->http);

	debug(140,5)("mod_video_header: when create store entry, we dont create store client for delaying client sending data \n");
	return 0;
}

static int mod_mp4_invoke_handlers(StoreEntry *e)
{
	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, e, mod);

        if(str_param == NULL) {
		return 0;
       	} 

        if(e->mem_obj->reply->sline.status != HTTP_OK){
		request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, str_param->http, mod);
		if( req_param != NULL){
			req_param->succ = 1;
		}

                debug(140,2)("mp4_delay_event reply request is not http 200 ok and set cc_delay_status = 0 in invokeHandles \n");
                str_param->store_complete_count = 0;
		return 0;
        }

        //if(str_param->store_complete_count == 1 || str_param->store_complete_count == 2){
        if(str_param->store_complete_count == 1){
               debug(140, 9) ("mod_video_header: store_complete_count<=2 for mp4 and delay sending data to client \n");
               return 1;
        }

	return 0;	
}
static int mod_mp4_store_complete(StoreEntry *e)
{
	
	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, e, mod);
	if(str_param == NULL) {
		return 0;
       	}


	if(e->mem_obj->reply->sline.status != HTTP_OK){
		request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, str_param->http, mod);
		if( req_param != NULL){
			req_param->succ = 1;
		}

		if(str_param->is_cbdatalock){
	       		str_param->is_cbdatalock = 0;
			cbdataUnlock(str_param->http);
		}

		str_param->store_complete_count = 0;

		debug(140,2)("mod_video_header: http reply is not http 200 ok and return \n");
              	return 0;
       	}
	
	str_param->store_complete_count++;

	if(str_param->store_complete_count >= 3){
		debug(140,3)("SWAPOUT_DONE and not must deal this !\n");
		return 0;
	}

	//if(str_param->store_complete_count >= 3 &&e->store_status == STORE_OK && e->swap_status == SWAPOUT_DONE){
	if(str_param->store_complete_count >= 2 &&e->store_status == STORE_OK){
		debug(140, 2) ("mod_video_header: set store_complete_count = 0 for mp4 store complete count %d \n", str_param->store_complete_count);
		str_param->store_complete_count = 0;
		//str_param->cc_delay_status = 0;
	}
	else{
		debug(140, 2) ("mod_video_header: set store_complete_count < 2 condition is adaptable and wait \n");
		return 0;
	}
	
	debug(140,3)("call client process request and start drag or drog !\n");

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, str_param->http, mod);
	if( req_param != NULL){
		
		debug(140,3)("request all mp4 object is successful and request again for starting drag \n");
		struct mod_config *cfg = cc_get_mod_param(str_param->http->conn->fd, mod);
		if(cfg != NULL){
			//req_param->mp4_flag = cfg->flag;
		}
		else{
			//req_param->mp4_flag = 1;
		}
	}
	//req_param->is_http_miss_wait = 0;
	int valid = cbdataValid(str_param->http);

	if(str_param->is_cbdatalock){
       		str_param->is_cbdatalock = 0;
		cbdataUnlock(str_param->http);
	}
	//cbdataUnlock(str_param->http);

	if(valid){
		req_param->flag = 1;
		clientHttpRequest *http = str_param->http;	
		http->sc->callback = NULL;
		http->sc->flags.disk_io_pending = 0;
		if(http->sc->copy_buf != NULL){
			 debug(140,2)("clean the sc->copy_buf \n");
			 memFree(http->sc->copy_buf, MEM_STORE_CLIENT_BUF);
		}

	
		clientProcessRequest(str_param->http);
	}
	else{
		debug(140,2)("calling clientProcessRequest fail because client close the connection \n");
	}
	
	return 0;	
}

static int mod_mp4_sc_unregister(StoreEntry* e,clientHttpRequest* http){

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	debug(140,2)("clean the sc->copy_buf \n");
	if( req_param != NULL && req_param->is_http_miss_wait == 1){
		req_param->is_http_miss_wait = 0;
		debug(140,2)("clean the sc->copy_buf \n");
		http->sc->callback = NULL;
		if(http->sc->copy_buf != NULL){
			debug(140,2)("clean the sc->copy_buf \n");
			memFree(http->sc->copy_buf, MEM_STORE_CLIENT_BUF);
		}

		return 1;
	}
	return 0;
}
*/

int mod_register(cc_module *module)
{
	debug(140, 1)("(mod_video_header) ->	mod_register:\n");

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
			add_range);

	//module->hook_func_http_repl_send_start = mod_reply_header;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			mod_reply_header);

	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);

	//module->hook_func_private_store_client_copy_hit = hook_get_hlv_header;
	cc_register_hook_handler(HPIDX_hook_func_private_store_client_copy_hit,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_store_client_copy_hit),
			hook_get_hlv_header);

	//module->hook_func_client_set_zcopyable = is_disable_sendfile;
	cc_register_hook_handler(HPIDX_hook_func_client_set_zcopyable,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_set_zcopyable),
			is_disable_sendfile);
	
	//module->hook_func_http_req_process = mod_video_header_http_req_process;
        cc_register_hook_handler(HPIDX_hook_func_http_req_process,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
                        mod_video_header_http_req_process);
        //module->hook_func_http_before_redirect = clientHandleMp4;
	//cc_register_hook_handler(HPIDX_hook_func_http_before_redirect,
          //              module->slot, 
            //            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_before_redirect),
              //          clientHandleMp4);
	//module->hook_func_http_client_create_store_entry = client_create_store_entry;
/*        cc_register_hook_handler(HPIDX_hook_func_http_client_create_store_entry,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_client_create_store_entry),
                        client_create_store_entry);
	
	//module->hook_func_store_complete = mod_mp4_store_complete;
        cc_register_hook_handler(HPIDX_hook_func_store_complete,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_store_complete),
                        mod_mp4_store_complete);
	//module->hook_func_invoke_handlers = mod_mp4_invoke_handlers;
        cc_register_hook_handler(HPIDX_hook_func_invoke_handlers,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_invoke_handlers),
                        mod_mp4_invoke_handlers);
	//module->hook_func_private_sc_unregister = mod_mp4_sc_unregister;
        cc_register_hook_handler(HPIDX_hook_func_private_sc_unregister,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_sc_unregister),
                        mod_mp4_sc_unregister);
*/	
//	module->hook_func_client_access_check_done = checkHasMp4;	
	cc_register_hook_handler(HPIDX_hook_func_client_access_check_done,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_access_check_done),
                        checkHasMp4);
	
	mod = module;

	return 0;
}
#endif

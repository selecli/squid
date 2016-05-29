#include "squid.h"
#include <string.h>

void memFree(void *p, int type);
CBDATA_TYPE(hlv_state);
void clientCacheHit(void *data, HttpReply * rep);
static cc_module *mod = NULL;

struct mod_config
{
	char	start[128];
	char	end[128];
	char    type[128];
};

typedef struct _request_param
{
	int flag;

} request_param;

typedef struct _store_param
{
	char *flv_pre;
	int flv_len;
	int offset;

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

static MemPool * request_param_pool = NULL;
static MemPool * store_param_pool = NULL;
static MemPool * mod_config_pool = NULL;

static void * request_param_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == request_param_pool)
	{
		request_param_pool = memPoolCreate("mod_flv private_data request_param", sizeof(request_param));
	}
	return obj = memPoolAlloc(request_param_pool);
}

static void * store_param_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == store_param_pool)
	{
		store_param_pool = memPoolCreate("mod_flv private_data store_param", sizeof(store_param));
	}
	return obj = memPoolAlloc(store_param_pool);
}

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_flv config_sturct", sizeof(struct mod_config));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_cfg(void *data)
{
	struct mod_config * cfg =(struct mod_config*)(data);
	if(cfg != NULL)
	{
		memPoolFree(mod_config_pool, cfg);
		cfg = NULL;
	}
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
		memFreeBuf((size_t)param->flv_len, param->flv_pre);
		memPoolFree(store_param_pool, param);
		param = NULL;
	}
	return 0;
}

static int free_request_param(void* data)
{
	request_param *req_param = (request_param *) data;
	if(req_param != NULL){
		memPoolFree(request_param_pool, req_param);
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

static int flvHandleRangeHeader(request_t *req, int len)
{
	//handle range message
	char range_value[64];
	HttpHeaderEntry *range = NULL;
	HttpHeaderEntry e;
	char *end = NULL;
	int end_value = 0;

	if(req == NULL){
		debug(116, 2)("mod_flv: http request is null in flvHandleRangeHeader \n");
		return -1;
	}

	range = httpHeaderFindEntry(&req->header, HDR_RANGE);
	if(range == NULL){
		
		debug(116, 3)("mod_flv: not range for request : %s \n",req->store_url);
		return -1;
	}
	assert(range);
	int old_value = atoi((range->value.buf)+6);
	int new_value = old_value - len;

	if(new_value < 0){
		debug(116, 2)("mod_flv: error: new locaton of range < 0 and start in uri : %d \n",old_value + 13);
		return -1;
	}
	end = strstr(range->value.buf+6, "-");
	if(end)
		end_value = atoi(end+1);
	httpHeaderDelById(&req->header, HDR_RANGE);
	if(!end_value || new_value <= end_value)
		snprintf(range_value, 31, "bytes=%d-", new_value);
	else 
		snprintf(range_value, 31, "bytes=%d-%d", new_value, end_value);
	e.id = HDR_RANGE;
	stringInit(&e.name, "Range");
	stringInit(&e.value, range_value);

	httpHeaderAddEntry(&req->header, httpHeaderEntryClone(&e));
	/*	range = httpHeaderFindEntry(&req->header, HDR_RANGE);
		assert(range);
		printf("%s\n",strBuf(range->value)); */
	httpHdrRangeDestroy(req->range);
	req->range = NULL;
	req->range = httpHeaderGetRange(&req->header);

	stringClean(&e.name);
	stringClean(&e.value);
	return 0;

}


static void storeHlvCopyCallBack(void *data, char *buf, ssize_t size)
{
	debug(116, 9)("mod_flv: Starting...\n");
	hlv_state *state = data;
	clientHttpRequest *http = state->http;

	if (!cbdataValid(state->callback_data)){
		
		debug(116, 3)("mod_flv: cbdataValid is 0 and not flv drag or droy \n");
		goto free_hit_params;
	}

	char rbuf[32] = {0};
	size_t http_len = headersEnd(buf, size);

	//read script data about tag12 onMetadata
	int first = http_len;
	int head_sz = 13;
	int tag_size = 15;
	uint8_t sort_flag = 0;

	if(buf[first] != 'F' || buf[first+1] != 'L' || buf[first+2] != 'V'){
		debug(116, 3)("mod_flv: hlv open the cache object is not flv format %s \n",http->uri);
		goto free_hit_params;
	}

	int8_t script = 0;
	memcpy(&script, &buf[first+13], 1);
	int script_size = 0;
	revert_int(rbuf, &buf[first+14], 3);
	memcpy(&script_size, &rbuf, 3);
	
	//debug(116, 3)("storeHlvCopy: script size = %d \n",script_size);
	//jump script data and read AVCDecoderConfigurationRecord data	 
	int second = script_size + head_sz + tag_size + first;

	if(second > size){
		debug(116, 3)("mod_flv: script data size %d > buf size  %ld \n",second,(long int)size);
		goto free_hit_params;
	
	}
	char *flv_buf = &buf[second];

	//read video control data
	memcpy(&sort_flag, &flv_buf[0], 1);
	if(sort_flag !=8 && sort_flag != 9){
		debug(116, 3)("mod_flv: not flv video tag type 8 or 9 %d \n",sort_flag);
		goto free_hit_params;
	}

	int tag1_size = 0;
	int local = 1;
	revert_int(rbuf, &flv_buf[local], 3);
	memcpy(&tag1_size, &rbuf, 3);

	//read leaf audio control data 
	local +=tag1_size +tag_size;

	if(second + local > size){
		debug(116, 3)("mod_flv: tag1 data size %d > buf size  %ld \n",second+local,(long int)size);
		goto free_hit_params;
	
	}
	
	int tag2_size = 0;
	memcpy(&sort_flag, &flv_buf[local-1], 1);
	if(sort_flag !=8 && sort_flag != 9){
		debug(116, 3)("mod_flv: not hlv left audio tag type 8 or 9 %d \n",sort_flag);
		goto free_hit_params;
	}
	revert_int(rbuf, &flv_buf[local], 3);
	memcpy(&tag2_size, &rbuf, 3);
	//debug(116, 3)("storeHlvCopy: tag2 size = %d \n",tag2_size);
	/*//read right audio control data
	unsigned int tag3_size = 0;
	local += tag2_size +tag_size;
	memcpy(&sort_flag, &flv_buf[local-1], 1);
	if(sort_flag !=8 && sort_flag != 9)
		return;
	revert_int(rbuf, &flv_buf[local], 3);
	memcpy(&tag3_size, &rbuf, 3);
	//copy AVCDecoderConfigurationRecord data from source file
	int copy_len = tag1_size +tag2_size +tag3_size + 3*15;*/
	int copy_len = tag1_size +tag2_size + 2*tag_size;

	//debug(116, 3)("storeHlvCopy: copy size = %d \n",copy_len);
	if(copy_len + second > size){
		debug(116, 3)("mod_flv: hlv code is too big %d \n",copy_len);
		goto free_hit_params;
	}
	size_t flv_len;
	char *copy_buf = (char *)memAllocBuf((size_t)copy_len, &flv_len);
	memcpy(copy_buf, flv_buf, copy_len);
	
	debug(116, 5)("mod_flv: AVCDecoderConfigurationRecord len  %d\n",copy_len);
	//debug(116, 5)("mod_flv: AVCDecoderConfigurationRecord data %s\n",copy_buf);
	//get stroreEntry private data
	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, state->entry, mod);

	//debug(116, 5)("mod_flv: AVCDecoderConfigurationRecord len  %d\n",copy_len);
	if(str_param != NULL) {
		str_param->flv_pre = copy_buf;
		str_param->flv_len = (int)flv_len;
		str_param->offset = copy_len;
	}

	if(flvHandleRangeHeader(http->request, copy_len)){
		memFreeBuf((size_t)str_param->flv_len, str_param->flv_pre);
		str_param->flv_pre = NULL;
		str_param->flv_len = 0;
		str_param->offset = 0;
	}
	
	storeClientCopyHeaders(http->sc, http->entry,
                                       clientCacheHit,
                                       http);
	
	free_hit_param(state);
	return;	

	free_hit_params:
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


	if( req_param->flag == 2 )
		return 0;

	assert(req_param->flag == 1);
	
	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	if(cfg == NULL){
		return 0;
	}

	if(strcasecmp(cfg->type,"hlv") == 0){

		store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, http->entry, mod);

		if(str_param == NULL) {
			str_param = store_param_pool_alloc(); 
	
			str_param->flv_len = 0;
			str_param->offset = 0;
			str_param->flv_pre = NULL;

			debug(116, 3)("hlv code is null and must read cache file from disk %s \n", cfg->type);
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
			storeClientCopy(state->sc, state->entry, state->offset, state->offset, state->buf_size,state->buf, storeHlvCopyCallBack, state);	
			return 1;
		}

		if(str_param->flv_pre !=NULL){
			flvHandleRangeHeader(http->request, str_param->offset);
		}
		
	}

	return 0;
	
}

static int hook_init()
{
	debug(116, 1)("(mod_flv) ->	hook_init:----:)\n");
	return 0;
}


static int hook_cleanup(cc_module *module)
{
	debug(116, 1)("(mod_flv) ->	hook_cleanup:\n");
	if(NULL != store_param_pool)
		memPoolDestroy(store_param_pool);
	if(NULL != request_param_pool)
		memPoolDestroy(request_param_pool);
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	return 0;
}


/* 去掉url参数中的start和end，但保留其它的 */
static void drop_startend(char* start, char* end, char* url)
{
//	fprintf(stderr, "in drop_startend\n");
//	fprintf(stderr, "start = %s  end = %s\n", start, end);
//	fprintf(stderr, "url = %s\n", url);
	char *s;
	char *e;

	//去掉start
	s = strstr(url, start);
	if(s == NULL)
		return;

	e = strstr(s, "&");
	if( e == 0 )
		*(s-1) = 0;
	else {
//		fprintf(stderr, "be move\n");
//		fprintf(stderr, "s = %s\n", s);
//		fprintf(stderr, "e = %s\n", e+1);
		memmove(s, e+1, strlen(e+1));
		*(s+strlen(e+1)) = 0;
//		fprintf(stderr, "s = %s\n", s);
	}

	//去掉end，如果有的话
	s = strstr(url, end);
	if( s ) {
		e = strstr(s, "&");
		if( e == 0 )
			*(s-1) = 0;
		else {
			memmove(s, e+1, strlen(e+1));
			*(s+strlen(e+1)) = 0;
		}
	}

//	fprintf(stderr, "url = %s \n\n\n", url);
}


static int add_range(clientHttpRequest *http)
{
	debug(116, 3)("(mod_flv) -> read request finished, add range %s: \n",http->uri);

	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);

	//参数不完整，忽略该请求
	if( !http || !http->uri || !http->request ) {
		debug(116, 3)("http = NULL, return \n");
		return 0;
	}

	char *tmp;

	//没有?，返回
	if( (tmp = strchr(http->uri, '?')) == NULL ) {
		debug(116, 3)("<%s> not find ?, return \n", http->uri);
		return 0;
	}


	//没有找到start的变量名，返回
	if( (tmp = strstr(tmp, cfg->start)) == NULL ) {
		debug(116, 3) ("not find start return %s %s %s \n",http->uri,tmp,cfg->start);
		return 0;
	}

	tmp--;
	if( *tmp != '?' && *tmp != '&' ) {
		debug(116, 3)("<%s> has not start variable, return \n", http->uri);
		return 0;
	}
	tmp++;

	//找起始偏移量
	int start_value;
	tmp += strlen(cfg->start);
	start_value = atol(tmp);

	int end_value = 0;
	if( (tmp = strstr(tmp, cfg->end)) ) {
		tmp--;
		if( *tmp == '?' || *tmp == '&' ) {
			tmp++;
			tmp += strlen(cfg->end);
			end_value = atoi(tmp);
		}
	}

	if( start_value > 13 )
		start_value -= 13;
	else
		start_value = 0;


	drop_startend(cfg->start, cfg->end, http->uri);
	if( http->uri[strlen(http->uri)-1] == '?')
		http->uri[strlen(http->uri)-1] = 0;


//	debug(116, 1)("http->urlpath.bur = %s\n", http->request->urlpath.buf);
	drop_startend(cfg->start, cfg->end, http->request->urlpath.buf);
	if( http->request->urlpath.buf[strlen(http->request->urlpath.buf)-1] == '?')
		http->request->urlpath.buf[strlen(http->request->urlpath.buf)-1] = 0;
	http->request->urlpath.len=strlen(http->request->urlpath.buf);


//	debug(116, 1)("http->request->canonical = %s\n", http->request->canonical);
	drop_startend(cfg->start, cfg->end, http->request->canonical);
	if( http->request->canonical[strlen(http->request->canonical)-1] == '?')
		http->request->canonical[strlen(http->request->canonical)-1] = 0;

//	debug(116, 1)("http->request->canonical = %s\n", http->request->canonical);
//	debug(116, 1) ("new log->uri = %s\n", http->uri);
//	debug(116, 1) ("new urlpath.buf = %s\n", http->request->urlpath.buf);


	char range_value[32];
	if( end_value )
		snprintf(range_value, 31, "bytes=%d-%d", start_value, end_value);
	else
		snprintf(range_value, 31, "bytes=%d-", start_value);


	HttpHeaderEntry eEntry;
	eEntry.id = HDR_RANGE;
	stringInit(&eEntry.name, "Range");
	stringInit(&eEntry.value, range_value);

	httpHeaderAddEntry(&http->request->header, httpHeaderEntryClone(&eEntry));
	stringClean(&eEntry.name);
	stringClean(&eEntry.value);

	request_param *req_param = request_param_pool_alloc();
	
	req_param->flag = 1;

	cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, req_param, free_request_param, mod);

	debug(116, 3)("(mod_flv) -> read request finished, add range: done start=%d end=%d \n",start_value,end_value);

	return 0;
}


static void add_flv_header(clientHttpRequest* http, char* buff, int size)
{
	request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if( req_param == NULL ) {
		return;
	}


	if( req_param->flag == 2 )
		return;

	assert(req_param->flag == 1);

	if( size < 13 )
		return;

	const char flv[13] = {'F', 'L', 'V', 1, 5, 0, 0, 0, 9, 0, 0, 0, 0};
	memcpy(buff, flv, 13);
	
	req_param->flag = 2;
 	
	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	if(cfg == NULL){
		return ;
	}
	
	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, http->entry, mod);

	if(str_param == NULL ) {
		return;
	}
	if(strcasecmp(cfg->type,"hlv") == 0){

		debug(116, 5)("mod_flv: copy AVCDecoderConfigurationRecord len  %d\n",str_param->offset);
		debug(116, 5)("mod_flv: copy AVCDecoderConfigurationRecord data %s\n",str_param->flv_pre);
		if(str_param->flv_pre && (size-13 > str_param->offset))
	 		memcpy(buff+13, str_param->flv_pre, str_param->offset);
 		/*safe_free(str_param->flv_pre);
		str_param->flv_pre = NULL;*/

	}


}


static int mod_reply_header(clientHttpRequest *http)
{
//	int *flag = cc_get_mod_private_data(REQUEST_T_PRIVATE_DATA, http->request, mod);
	request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if( req_param == NULL )
		return 0;

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

	if( strstr(cfg_line, "mod_flv") == NULL )
		return 0;

	char *token = NULL;

	if ((token = strtok(cfg_line, w_space)) == NULL)
		return 0;

	struct mod_config *cfg = mod_config_pool_alloc();
	
	token = strtok(NULL, w_space);
	strncpy(cfg->start, token, 126);
	strcat(cfg->start, "=");

	token = strtok(NULL, w_space);
	strncpy(cfg->end, token, 126);
	strcat(cfg->end, "=");

	
	token = strtok(NULL, w_space);
	int isStdFlv = 1;
	//check if the type is hlv 
	if((token != NULL) && ((strcasecmp(token,"allow")!=0) && ( strcasecmp(token,"deny")!=0 ))){
		strncpy(cfg->type, token, 127);
		isStdFlv = 0;
	}
	//default support standard flv type	
	if(isStdFlv){
		strncpy(cfg->type, "flv",127);
	}
	
	debug(116, 5) ("mod_flv: paramter start= %s end= %s type= %s \n", cfg->start, cfg->end, cfg->type );	
	
	cc_register_mod_param(mod, cfg, free_cfg);
	return 0;
}


static int is_disable_sendfile(store_client *sc, int fd) 
{
	sc->is_zcopyable = 0;
	return 1;
}

int mod_register(cc_module *module)
{
	debug(116, 1)("(mod_flv) ->	mod_register:\n");

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
	//module->hook_func_http_req_read = add_range;
	cc_register_hook_handler(HPIDX_hook_func_http_req_read,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_read),
			add_range);
	//module->hook_func_http_repl_send_start = mod_reply_header;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			mod_reply_header);
	//module->hook_func_private_clientPackMoreRanges = add_flv_header;
	cc_register_hook_handler(HPIDX_hook_func_private_clientPackMoreRanges,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_clientPackMoreRanges),
			add_flv_header);
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
	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;

	return 0;
}


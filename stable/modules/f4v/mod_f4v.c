#include "cc_framework_api.h"
#include <stdbool.h>

#define QQVIDEO

#ifdef CC_FRAMEWORK
static helper *media_prefetchers = NULL;
static int g_helper_refernce_count = 0;
void clientProcessRequest(clientHttpRequest *);
static void modify_request(clientHttpRequest * http,char *new_uri);
typedef void CHRP(clientHttpRequest *);
static void restore_http(clientHttpRequest *http);

static int is_start_helper = 0;
static int helper_proc_num = 60;
static int is_load_helper = 0;

typedef struct _media_prefetch_state_data{
	clientHttpRequest * http; 
	CHRP *callback;
}media_prefetch_state_data;

CBDATA_TYPE(media_prefetch_state_data);



void memFree(void *p, int type);

CBDATA_TYPE(hlv_state);
void clientCacheHit(void *data, HttpReply * rep);
static cc_module *mod = NULL;

typedef enum{
	BYTES,
	TIMES,
	BYTES_CP_ALL,
	OTHERS
}drag_type;


typedef enum{
	STANDARD,
	YOUKU,
	TUDOU,
	KU6,
	QQ,
	SOHU,
	SINA,
	FLV56,
	VIDEO56,
}store_type;

typedef enum {
	IGNORE_END,		//ignore end=xxx,return [range:start-]
	NORMAL
}download_mod;

struct mod_config
{
	char	start[128];
	char	end[128];
	drag_type	type;//type:1 means seconds, 0 means bytes
	int     miss_strategy; //miss strategy for 0 : query_total :query total object, 1 miss_wait  2 miss_start 3 query_start
	int 	has_right_auto;
	store_type website;
	int 	cp_all;
};

typedef enum{
	HIT_FLV,
	HIT_OTHER,
	MISS
} media_type;


typedef struct _request_param
{
	int flag;
	int start;
	int end;
	char *uri;
	media_type is_real_flv;
	int succ;
	int http_miss_wait;
    int cur_pos;            //add by zh for send more than once	
	int StreamMod;		//add for 56flv h=N [N = 0: h263 | N >0: h264]
	int HeadOffset;		//add for 56flv flv offset 
	int times;	        //add for 56flv deal start=N if N > content_length;

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
	MemBuf tmp_buf;
	size_t buf_size;
	size_t tmp_buf_offset;
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
		 request_param_pool = memPoolCreate("mod_f4v private_data request_param", sizeof(request_param));		
	}
	return obj = memPoolAlloc(request_param_pool);
}

static void * store_param_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == store_param_pool)
	{
		store_param_pool = memPoolCreate("mod_f4v private_data store_param", sizeof(store_param));
	}
	return obj = memPoolAlloc(store_param_pool);
}

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_f4v config_sturct", sizeof(struct mod_config));	
	}
     g_helper_refernce_count++;
	return obj = memPoolAlloc(mod_config_pool);
}

static int free_cfg(void *data)
{
	struct mod_config * cfg = (struct mod_config*)data;
    if(NULL != cfg)
    {
        memPoolFree(mod_config_pool, cfg);
        cfg = NULL;
        g_helper_refernce_count--;
        if(g_helper_refernce_count <= 0 && media_prefetchers)
        {
            helperShutdown(media_prefetchers);
            helperFree(media_prefetchers);
            media_prefetchers = NULL;
            g_helper_refernce_count = 0;
            debug(137,1)("mod_f4v  shutdown helper %d\n", g_helper_refernce_count);
        }
    }
    return 0;
}

static int httpHeaderCheckExistByName(HttpHeader *h, char *name)
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *curHE;
	while ((curHE = httpHeaderGetEntry(h, &pos)))
	{
		if(!strcmp(strBuf(curHE->name), name))
		{
			debug(137,3)("Internal X-CC-Media-Prefetch request!\n");
			return 1;
		}
	}

	debug(137,3)("Not internal X-CC-Media-Prefetch request!\n");
	return 0;
}

static void parse_programline(wordlist ** line)
{       
        if (*line)      
                self_destruct();
        parse_wordlist(line);
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
			memFree(state->buf, MEM_64K_BUF);
			state->buf = NULL;
		}
		memBufClean(&(state->tmp_buf));
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
		if(param->flv_pre != NULL)
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
		safe_free(req_param->uri);
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

//static int flvHandleRangeHeaderForBytes(request_t *req, int len, struct mod_config *cfg)
static int flvHandleRangeHeaderForBytes(clientHttpRequest *http, int len, struct mod_config *cfg)
{
        //handle range message
        char range_value[32];  
	HttpHeaderEntry *range = NULL;
	HttpHeaderEntry e;
	char *end = NULL;
	int end_value = 0;
	request_t *req = http->request;
	if(req == NULL){
		debug(137, 2)("mod_f4v: http request is null in flvHandleRangeHeader \n");
		return -1;
	}

	range = httpHeaderFindEntry(&req->header, HDR_RANGE);
	if(range == NULL){ 
		debug(137, 0)("mod_f4v: not range for request : %s \n",req->store_url);
		return -1;
	}
	assert(range);
	int old_value = atoi((range->value.buf)+6);
	int new_value = old_value - len;

	debug(137,5)("mod_f4v: old_value: %d, len: %d\n", old_value, len);

	if(new_value < 0){
		return -1;
	}
	end = strstr(range->value.buf+6, "-");
	if(end)
		end_value = atoi(end+1);                                                        
	httpHeaderDelById(&req->header, HDR_RANGE);
	if(!end_value || end_value < new_value)
	{                                                                                       
		snprintf(range_value, 31, "bytes=%d-", new_value);                              
	}                                                                                       
	else
	{  
		assert(new_value <= end_value);                                                 
		snprintf(range_value, 31, "bytes=%d-%d", new_value, end_value);
	}                          

	debug(137, 3)("mod_f4v: storeFlvCopy: range start index %s \n" , range_value);
	e.id = HDR_RANGE;
	stringInit(&e.name, "Range");
	stringInit(&e.value, range_value);
	httpHeaderAddEntry(&req->header, httpHeaderEntryClone(&e));

	if (req->range){
		httpHdrRangeDestroy(req->range);
	}

        req->range = NULL;
        req->range = httpHeaderGetRange(&req->header);

        stringClean(&e.name);
        stringClean(&e.value);
        return 0;

}
/*
static int flvHandleRangeHeaderForBytes_old(clientHttpRequest *http,request_t *req, int len)
{
	//handle range message
        char range_value[32];
        HttpHeaderEntry *range = NULL;
        HttpHeaderEntry e;
        char *end = NULL;
        int end_value = 0;
	request_t *req = http->request;
	if(req == NULL){
		debug(137, 2)("mod_f4v: http request is null in flvHandleRangeHeader \n");
		return -1;
	}

        range = httpHeaderFindEntry(&req->header, HDR_RANGE);
        if(range == NULL){
                debug(137, 0)("mod_f4v: not range for request : %s \n",req->store_url);
                return -1;
        }
        assert(range);
        int old_value = atoi((range->value.buf)+6);
        int new_value = old_value - len;

	debug(137,5)("mod_f4v: old_value: %d, len: %d\n", old_value, len);

	if(new_value < 0){
		debug(137, 0)("mod_f4v: error: new locaton of range < 0 and start in uri : %d \n",old_value + 13);
		return -1;
	}
        end = strstr(range->value.buf+6, "-");
        if(end)
			end_value = atoi(end+1);
        httpHeaderDelById(&req->header, HDR_RANGE);
        if(!end_value || end_value < new_value)
		{
			snprintf(range_value, 31, "bytes=%d-", new_value);
		}
        else
		{
			assert(new_value <= end_value);
            snprintf(range_value, 31, "bytes=%d-%d", new_value, end_value);
		}
        e.id = HDR_RANGE;
        stringInit(&e.name, "Range");
        stringInit(&e.value, range_value);

        debug(137, 5)("mod_f4v: result range for request : %s \n",range_value);

        httpHeaderAddEntry(&req->header, httpHeaderEntryClone(&e));
        if (req->range){	
		httpHdrRangeDestroy(req->range);
        }
        req->range = NULL;
        req->range = httpHeaderGetRange(&req->header);

        stringClean(&e.name);
        stringClean(&e.value);

	return 0;

}
*/

static int flvHandleRangeHeaderForTimes(request_t *req, size_t range_start,size_t range_end)
{
        //handle range message
        char range_value[32];
        //HttpHeaderEntry *range = NULL;
        HttpHeaderEntry e;

	if(req == NULL){
		debug(137, 2)("mod_f4v: http request is null in flvHandleRangeHeader \n");
		return -1;
	}

	if(range_start <0)
		range_start = 0;
		
	if(range_end > 0 && range_end > range_start)
	{
		assert(range_start <= range_end );

		snprintf(range_value, 31, "bytes=%lu-%lu", (unsigned long)range_start,(unsigned long)range_end);
	}
	else
	{
		snprintf(range_value, 31, "bytes=%lu-", (unsigned long)range_start);
	}

	debug(137, 3)("mod_f4v: storeFlvCopy: range start index %s \n" , range_value);
	e.id = HDR_RANGE;
	stringInit(&e.name, "Range");
	stringInit(&e.value, range_value);
	httpHeaderAddEntry(&req->header, httpHeaderEntryClone(&e));

	if (req->range){	
		httpHdrRangeDestroy(req->range);
	}

        req->range = NULL;
        req->range = httpHeaderGetRange(&req->header);

        stringClean(&e.name);
        stringClean(&e.value);
	return 0;

}

static int calc_flv_range_value(clientHttpRequest *http, char *onmetadata, ssize_t size)
{
	debug(137, 3)("mod_f4v: Starting calc the range value...\n");

	char rbuf[32] = {0};

	request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if(req_param == NULL)
	{
		debug(137, 2)("mod_f4v: req_param is NULL ...\n");
		goto free_hit_params;
	}

	assert(req_param->flag == 1);
	
	int start = req_param->start;
	int end   = req_param->end;
#ifndef QQVIDEO
	int start_last = 0;
	int end_last = 0;
#endif
	int start_index = -1;
	int end_index   = -1;
	size_t file_start = 0;
	size_t file_end   = 0;

	int hasend = 0;
#ifdef QQVIDEO
	if(end > 0 && end >= start){
		hasend = 1;
	}
#else
	if(end > 0 && end > start){
		hasend = 1;
	}
#endif

	//unsigned char *filepos = buf + 571;
	unsigned char *keyframes = (unsigned char *)memmem(onmetadata,size,"keyframes",9);

	if(!keyframes){
		
		debug(137, 3)("storeFlvCopy: has not keyframes array \n");
		goto free_hit_params;
	}
	//unsigned char *filepos = (unsigned char *)memmem(buf,first_len,"filepositions",13);
	char *filepos = (char *)memmem(onmetadata,size,"filepositions",13);

	if(!filepos){
		
		debug(137, 3)("storeFlvCopy: has not filepos array \n");
		goto free_hit_params;
	}
	if(filepos[13] != 10){
	
		debug(137, 3)("storeFlvCopy: filepos type is not array %d \n",filepos[13]);
		goto free_hit_params;
	}

	//unsigned char *times = buf + 987;
	char *times = (char *)memmem(keyframes,size,"times",5);
	if(!times){
		
		debug(137, 3)("storeFlvCopy: has not time array \n");
		goto free_hit_params;
	}
	
	debug(137, 3)("storeFlvCopy: format is right %d \n" ,times[5]);

	if(times[5] != 10){
		
		debug(137, 3)("storeFlvCopy: times type is not array %d \n" ,times[5]);
		goto free_hit_params;
	}

	unsigned int times_size = 0;

	revert_int(rbuf, &times[6],4);
	memcpy(&times_size, &rbuf,4);
	
	//debug(137, 3)("storeFlvCopy: times array size is %d \n",times_size);
	int i  = 0;
	for(i = 0; i<times_size; i++){
		if(times[10+i*9] != 0){
			 debug(137, 3)("storeFlvCopy: array elements type is not number %d \n" ,times[10+i*9]);
			 continue;
		}
		double timepos = 0;
		revert_int(rbuf, &times[10+i*9+1],8);
		memcpy(&timepos, &rbuf, 8);
		//debug(137, 3)("storeFlvCopy: time is %f %d \n",timepos,i);

		if(start_index == -1){
			debug(137, 3)("storeFlvCopy: timepos is %f \n",timepos);
			
#ifdef QQVIDEO
			if ((start - timepos) < 0)
			{
				start_index = i - 1;
			}
			debug(137, 9)("storeFlvCopy:[%d] start_index=%d,start=%d,timepos=[%d]\n",i,start_index,start,(int)timepos);
#else
			int start_curr = abs(start - timepos);

		debug(137, 3)("storeFlvCopy: timepos is %f \n",timepos);
			int start_curr = abs(start - timepos);

			if(start_curr > start_last && i != 0){
				start_index = i - 1;
			} 
			else{
				start_last = start_curr;
			}
			debug(137, 9)("storeFlvCopy:[%d] start_index=%d,start=%d,timepos=[%d],start_curr=[%d],start_last=[%d]\n",i,start_index,start,(int)timepos,start_curr,start_last);
#endif
		}
		
		if(hasend && end_index == -1){
#ifdef QQVIDEO
			if ((end - timepos)< 0 )
			{
				end_index = i -1;
			}
#else
			int end_curr = abs(end - timepos);
			if(end_curr > end_last && i != 0){
				end_index = i - 1;
			} 
			else{
				end_last = end_curr;
			}
#endif
			debug(137, 3)("storeFlvCopy:[%d] end_index=%d,end=%d,timepos=[%d]\n",i,end_index,end,(int)timepos);
		}

	}
	
#ifdef QQVIDEO
	debug(137, 3)("before set storeFlvCopy: end index %d\n" ,end_index);
	/* let end_index behind start_index */
	if(start == end)
	{
		if(start_index < i - 1)
			end_index = start_index + 1;
	}

#endif

	debug(137, 3)("storeFlvCopy: i =  %d\n", i);
	debug(137, 3)("storeFlvCopy: start  = %d end = %d\n", start, end);
	debug(137, 3)("storeFlvCopy: start index of key frames %d \n" ,start_index);
	debug(137, 3)("storeFlvCopy: end   index of key frames %d \n" ,end_index);

	unsigned int filepos_size = 0;

	revert_int(rbuf, &filepos[14],4);
	memcpy(&filepos_size, &rbuf,4);
	
	//debug(137, 3)("storeFlvCopy: filepos array size is %d \n",filepos_size);
	//int i  = 0;
	for(i = 0; i<filepos_size; i++){
		if(filepos[18+i*9] != 0){
			 debug(137, 3)("storeFlvCopy: array elements type is not number %d \n" ,filepos[18+i*9]);
			 continue;
		}
		if(i == start_index){
			double timepos2 = 0;
			revert_int(rbuf, &filepos[18+i*9+1],8);
			memcpy(&timepos2, &rbuf, 8);
			file_start = timepos2;
			debug(137, 3)("storeFlvCopy: start filepos is %f %ld \n",timepos2,(long int)file_start);
		}

		if(!hasend){
			continue;
		}

		if(i == end_index){
			double timepos2 = 0;
			revert_int(rbuf, &filepos[18+i*9+1],8);
			memcpy(&timepos2, &rbuf, 8);
			file_end = timepos2;
			debug(137, 3)("storeFlvCopy:   end filepos is %f %ld \n",timepos2,(long int)file_end);
		}

	}

	debug(137, 9)("storeFlvCopy: hasend = %d ,i =  %d\n", hasend, i);

	if(file_start - size >= 0)
                return flvHandleRangeHeaderForTimes(http->request, file_start - size,file_end);	

	free_hit_params:
		return -1;

}


static void storeHlvCopyCallBack(void *data, char *buf, ssize_t size)
{
	debug(137, 9)("mod_f4v: Starting hlv copy callback...\n");
	hlv_state *state = data;
	clientHttpRequest *http = state->http;


	if (!cbdataValid(state->callback_data)){
		debug(137, 3)("mod_f4v: cbdataValid is 0 and not flv drag or drog \n");
		free_hit_param(state);
		return; 
		//goto free_hit_params;
	}

	char rbuf[32] = {0};

	if( (size < 0) || ((size == 0) && (state->tmp_buf_offset > 0)) || ( state->tmp_buf_offset > (STORE_CLIENT_BUF_SZ*10))){
		debug(137, 3)("mod_f4v: f4v cache object is error\n");
		goto free_hit_params;
	}
    memBufAppend(&(state->tmp_buf), buf, size);
    state->tmp_buf_offset += size;
    size = state->tmp_buf_offset;
	size_t http_len = headersEnd(state->tmp_buf.buf, size);
	char *flv_buf = &state->tmp_buf.buf[http_len];
//	char *flv_buf = &buf[http_len];

	//read script data about tag12 onMetadata
	int first = 0;
	int head_sz = 13;
	int tag_size = 15;
	uint8_t sort_flag = 0;

	if(flv_buf[first] != 'F' || flv_buf[first+1] != 'L' || flv_buf[first+2] != 'V'){
		debug(137, 3)("mod_f4v: hlv open the cache object is not flv format %s \n",http->uri);
		goto free_hit_params;
	}

	int8_t script = 0;
	memcpy(&script, &flv_buf[first+13], 1);
	int script_size = 0;
	revert_int(rbuf, &flv_buf[first+14], 3);
	memcpy(&script_size, &rbuf, 3);
	
	//debug(137, 3)("storeHlvCopy: script size = %d \n",script_size);
	//jump script data and read AVCDecoderConfigurationRecord data	 
	size_t second = script_size + head_sz + tag_size;

	if(second + http_len > size){
		debug(137, 3)("mod_f4v: script data size %lu > buf size  %ld \n",(unsigned long)second+http_len,(long int)size);
//		goto free_hit_params;
//		state->tmp_buf_first_offset = size;
		debug(137, 3)("mod_f4v: state tmp_offset: %ld, state buf_size: %ld\n", (long int)state->tmp_buf_offset, (long int)state->buf_size);
		goto re_copy;
	}

	debug(137, 3)("mod_f4v: script data size %lu ;  buf size  %ld \n",(unsigned long)second+http_len,(long int)size);

	//read video control data
	memcpy(&sort_flag, &flv_buf[second], 1);
	if(sort_flag !=8 && sort_flag != 9){
		debug(137, 3)("mod_f4v: not flv video tag type 8 or 9 %d \n",sort_flag);
		goto free_hit_params;
	}

	int tag1_size = 0;
	int local = second + 1;
	revert_int(rbuf, &flv_buf[local], 3);
	memcpy(&tag1_size, &rbuf, 3);

	//read leaf audio control data 
	local += tag1_size + tag_size;
	debug(137,5)("mod_f4v, tag1_size %d,tag_size %d local %d\n",tag1_size,tag_size,local);

	if(local + http_len > size){
		debug(137, 3)("mod_f4v: tag1 data size %lu > buf size  %ld \n",(unsigned long)local+http_len,(long int)size);
		goto re_copy;
	}
	
	int tag2_size = 0;
	memcpy(&sort_flag, &flv_buf[local-1], 1);
	if(sort_flag !=8 && sort_flag != 9){
		debug(137, 3)("mod_f4v: not hlv left audio tag type 8 or 9 %d \n",sort_flag);
		goto free_hit_params;
	}
	revert_int(rbuf, &flv_buf[local], 3);
	memcpy(&tag2_size, &rbuf, 3);

	//read right audio control data
	size_t copy_len = tag1_size +tag2_size + 2*tag_size;
	
	debug(137, 3)("(mod_f4v): second = [%ld],copy_len = [%lu] \n",(unsigned long)second,(unsigned long)copy_len);//56flv test
	debug(137,5)("mod_f4v, tag2_size %d,tag_size %d copy_len %d\n",tag2_size,tag_size,copy_len);
	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	if(cfg == NULL){
		goto free_hit_params;
	}

	request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if( req_param == NULL ) {
		goto free_hit_params;
	}

	int tag3_size = 0;
	if(cfg->has_right_auto){


		local += tag2_size +tag_size;

		if(local + http_len > size){
			debug(137, 3)("mod_f4v: tag2 data size %lu > buf size  %ld \n",(unsigned long)local+http_len,(long int)size);
		//	goto free_hit_params;
			goto re_copy;
		}
	
		memcpy(&sort_flag, &flv_buf[local-1], 1);
		if(sort_flag !=8 && sort_flag != 9){
			debug(137, 3)("mod_f4v: not hlv left audio tag type 8 or 9 %d \n",sort_flag);
			goto free_hit_params;
		}
	
		revert_int(rbuf, &flv_buf[local], 3);
		memcpy(&tag3_size, &rbuf, 3);
		copy_len += tag3_size + tag_size;

		debug(137, 3)("mod_f4v: copy len  %lu and tag3 size  %d \n",(unsigned long)copy_len,tag3_size + tag_size);

	}

	debug(137,5)("mod_f4v, tag3_size %d,tag_size %d copy_len %d\n",tag3_size,tag_size,copy_len);
	debug(137,5)("mod_f4v: AVCDecoderConfigurationRecord second  %ld flv buf[%s]\n",(long int)second,flv_buf);
	if((cfg->type == TIMES) || (cfg->type == BYTES_CP_ALL || cfg->website == VIDEO56 || cfg->website == TUDOU)){
		debug(137, 3)("mod_f4v: copy full video header\n");
		copy_len += second;
		if(copy_len+http_len > size){
			debug(137, 3)("mod_f4v: hlv code is too big %lu > buf size %ld \n",(unsigned long)copy_len + http_len,(long int)size);
		//	goto free_hit_params;
			goto re_copy;
		}
	}
	else{
		//add for 56flv
		if (cfg->website != FLV56)
			flv_buf = flv_buf + second;
//		copy_len += second;

		if (cfg->website == FLV56)
		{
			if(req_param->StreamMod > 0) //h264
			{
				copy_len += second;
			}
			else //h263
			{
				flv_buf = flv_buf + second;
			}
		}
		debug(137, 3)("mod_f4v: copy_len = [%lu] \n",(unsigned long)copy_len);//56flv test
		if(copy_len+second+http_len > size){
			debug(137, 3)("mod_f4v: hlv code is too big %lu > buf size %ld \n",(unsigned long)copy_len+second+http_len, (long int)size);
		//	goto free_hit_params;
			goto re_copy;
		}
	}
	size_t flv_len;
	char *copy_buf = (char *)memAllocBuf((size_t)copy_len, &flv_len);
	memcpy(copy_buf, flv_buf, copy_len);

	debug(137, 5)("mod_f4v: AVCDecoderConfigurationRecord len [%ld]\n",(unsigned long)copy_len);
	debug(137, 5)("mod_f4v: copy_len=[%ld],flv_len=[%ld]\n",(unsigned long)copy_len,(unsigned long)flv_len);
	//get stroreEntry private data
	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, state->entry, mod);

	if(str_param != NULL) {
		debug(137, 3)("mod_f4v: copy_buf first[%c]\n", copy_buf[0]);
		str_param->flv_pre = copy_buf;
		str_param->flv_len = (int)flv_len;
		str_param->offset = (int)copy_len;
	}

	if(cfg->type == TIMES){	
		//if(calc_flv_range_value(http, str_param->flv_pre, str_param->flv_len)){
		if(calc_flv_range_value(http, str_param->flv_pre, str_param->offset)){
			memFreeBuf((size_t)str_param->flv_len, str_param->flv_pre);
			str_param->flv_pre = NULL; 
			str_param->flv_pre = NULL;
			str_param->flv_len = 0;
			str_param->offset = 0;
		}
	}
	else{

		//if(flvHandleRangeHeaderForBytes(http->request, copy_len, cfg->website)){
		if(flvHandleRangeHeaderForBytes(http, copy_len, cfg)){
			memFreeBuf((size_t)str_param->flv_len, str_param->flv_pre);
			str_param->flv_pre = NULL; 
			str_param->flv_pre = NULL;
			str_param->flv_len = 0;
			str_param->offset = 0;
		}
	}

	storeClientCopyHeaders(http->sc, http->entry,
                                       clientCacheHit,
                                       http);
	
	free_hit_param(state);
	return;
	re_copy:
		storeClientCopy(state->sc, state->entry, state->tmp_buf_offset, state->tmp_buf_offset, state->buf_size,state->buf, storeHlvCopyCallBack, state);
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
		debug(137, 1)("mod_f4v: entry is NULL \n");
		return 0;
	}
	
	request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if( req_param == NULL ) {
		debug(137, 1)("mod_f4v: req_param is NULL \n");
		return 0;
	}

	if(req_param->is_real_flv != HIT_FLV || req_param->flag == 0){
		debug(137, 1)("mod_f4v: not fit \n");
		return 0;
	}
	
	debug(137, 5)("mod_f4v: AVCDecoderConfigurationRecord len  \n");
	if( req_param->flag == 2 )
	{
		debug(137, 5)("mod_f4v: req_param->flag == 2, return  \n");
		return 0;
	}

	assert(req_param->flag == 1);
	
	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	if(cfg == NULL){
		debug(137, 5)("mod_f4v: cfg == NULL, return  \n");
		return 0;
	}


	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, http->entry, mod);

	if(str_param == NULL) {
		str_param = store_param_pool_alloc();

		str_param->flv_len = 0;
		str_param->flv_pre = NULL;
		str_param->offset = 0;

		debug(137, 3)("flv code is null and must read cache file from disk %d \n", cfg->type);
		cc_register_mod_private_data(STORE_ENTRY_PRIVATE_DATA, http->entry, str_param, free_store_param, mod);
		CBDATA_INIT_TYPE(hlv_state);
		hlv_state *state = cbdataAlloc(hlv_state);
		state->entry = http->entry;
        storeLockObject(state->entry);
		state->callback_data = http;
		state->callback = NULL;
		cbdataLock(http);
	
		state->http = http;
		state->buf  = memAllocate(MEM_64K_BUF);
		state->buf_size = 65536;

		memBufDefInit(&(state->tmp_buf));
		state->tmp_buf_offset = 0;

		state->sc = storeClientRegister(state->entry, state);
		state->offset = 0;
		storeClientCopy(state->sc, state->entry, state->offset, state->offset, state->buf_size,state->buf, storeHlvCopyCallBack, state);	
		return 1;
	}
	if(str_param->flv_pre != NULL){
		if(cfg->type == BYTES)
		{
			debug(137, 5)("mod_f4v-> cfg->type == BYTES \n");
			//flvHandleRangeHeaderForBytes(http->request, str_param->flv_len);
			//flvHandleRangeHeaderForBytes(http->request, str_param->offset,cfg);//support 56flv
			flvHandleRangeHeaderForBytes(http, str_param->offset,cfg);//support 56flv
			return 0;
		}
		if(cfg->type == BYTES_CP_ALL)
		{
			debug(137, 5)("mod_f4v-> cfg->type == BYTES_CP_ALL \n");
			//flvHandleRangeHeaderForBytes(http->request, str_param->flv_len);
			//flvHandleRangeHeaderForBytes(http->request, str_param->offset,cfg);
			flvHandleRangeHeaderForBytes(http, str_param->offset,cfg);
			return 0;
		}
		else
		{
			debug(137, 5)("mod_f4v-> cfg->type == TIMES\n");
			calc_flv_range_value(http, str_param->flv_pre, str_param->offset);
			return 0;
		}
	}
	else
	{
		debug(137, 1)("mod_f4v: str_param->flv_pre is NULL \n");
	}

	debug(137, 5)("mod_f4v->out the get header function direct\n");

	return 0;
}

static int hook_init()
{
	debug(137, 1)("(mod_f4v) ->	hook_init:----:)\n");
	return 0;
}


static int hook_cleanup(cc_module *module)
{
	debug(137, 1)("(mod_f4v) ->	hook_cleanup:\n");
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
		memmove(s, e+1, strlen(e+1));
		*(s+strlen(e+1)) = 0;
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

        debug(137, 3)("mod_f4v:hostname =%s \n",hostname);
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
	        if(NULL == tmp3 )
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
        debug(137, 3)("mod_f4v : store_url=%s \n",result_url);
        safe_free(new_url);
        return result_url;

fail:
        safe_free(new_url);
        return NULL;

}


//standard safeguard chain: store_url = the part of url which is before "?"
static char *store_key_url(char* url){
	
 	char * uri = xstrdup(url);
        char *tmpuri = strstr(uri,"?");
        if(tmpuri != NULL)
                *tmpuri = 0;

	debug(137, 3)("(mod_f4v) -> store_url is %s \n", uri);
	return uri;
}

//sina flv.sina.fastwebcdn.com  safeguard chain: 
static char *store_sinacdn_key_url(char* url){

        char new_url[1024];
        memset(new_url,0,1024);

        char* str1 = strstr(url, "://");
        if(NULL == str1)
        {
                return NULL;
        }

        str1 += 3;

        memcpy(new_url, url, str1-url);

        char* str2 = strchr(str1, '/');


        if(NULL == str2)
        {
                return NULL;
        }

        strncat(new_url, str1, str2-str1);
        //memcpy(new_url, url, str2-url);

        str2 += 1;
        char* str4 = strchr(str2, '/');
        if(NULL == str4)
        {
                return NULL;
        }

        //strncat(new_url, str2, str4-str2);

        str4 += 1;

        char* str3 = strchr(str4, '/');
 	if(NULL == str3)
        {
                return NULL;
        }

        strcat(new_url,str3);

	char *quesurl = strchr(new_url,'?');
        if(quesurl != NULL){
                *quesurl = 0;
        }


        debug(137, 3)("store_url is %s \n", new_url);
        return xstrdup(new_url);

}


//youku safeguard chain: 1 remove ip 2 remove random directory 
static char *store_youku_key_url(char* url){

        char new_url[1024];
        memset(new_url,0,1024);

        char* str1 = strstr(url, "://");
        if(NULL == str1)
        {
                return NULL;
        }

        str1 += 3;

        memcpy(new_url, url, str1-url);

        char* str2 = strchr(str1, '/');


        if(NULL == str2)
        {
                return NULL;
        }

        //memcpy(new_url, url, str2-url);

        str2 += 1;
        char* str4 = strchr(str2, '/');
        if(NULL == str4)
        {
                return NULL;
        }

        strncat(new_url, str2, str4-str2);

        str4 += 1;

        char* str3 = strchr(str4, '/');
 	if(NULL == str3)
        {
                return NULL;
        }

        strcat(new_url,str3);

	char *quesurl = strchr(new_url,'?');
        if(quesurl != NULL){
                *quesurl = 0;
        }


        debug(137, 3)("store_url is %s \n", new_url);
        return xstrdup(new_url);

}

//tudou safeguard chain: 1 remove ip 2 remove key and posky
static char *store_tudou_key_url(char* url){
        
        if((strstr(url,"key=") == NULL)||(strstr(url,"posky=")==NULL)){
                return NULL;
        }

        char *new_url = xstrdup(url);
        char *s;
        char *e;
       
        s = strstr(new_url, "://");
        s += 3;
        e = strstr(s, "/");
        if( e != NULL)
        {
             memmove(s, e+1, strlen(e+1));
             *(s+strlen(e+1)) = 0;
        }

        char *quesurl = strchr(new_url,'?');
        if(quesurl != NULL){
                *quesurl = 0;
        }

        //drop_startend("key=","posky=",new_url);

	debug(137, 3)("(mod_f4v) -> store_url is %s \n", new_url);
        return new_url;
}

/*
static void drop_parameter(char* start, char* url)
{
        char *s;
        char *e;

        //???￥?μ′art
        s = strstr(url, start);

	if(s == NULL){
		return;
	}

        e = strstr(s, "&");
        if( e == 0 )
                *(s-1) = 0;
        else {
                memmove(s, e+1, strlen(e+1));
                *(s+strlen(e+1)) = 0;
        }
}
*/

//tudou safeguard chain: 1 remove ip 2 remove the part after "?"
static char *store_ku6_key_url(char* url){

        /*if((strstr(url,"key=") == NULL)||(strstr(url,"tm=")==NULL)){
                return NULL;
        }*/

        char *new_url = xstrdup(url);
        char *s;
        char *e;

        s = strstr(new_url, "://");
        s += 3;
        e = strstr(s, "/");
        if( e != NULL)
        {
             memmove(s, e+1, strlen(e+1));
             *(s+strlen(e+1)) = 0;
        }

        char *quesurl = strchr(new_url,'?');
        if(quesurl != NULL){
                *quesurl = 0;
        }

        debug(137, 3)("store_url=%s \n", new_url);
        //drop_startend("tm=","key=",new_url);

        return new_url;
}


//56video safeguard chain: 1 remove the part before flvdownload 2 remove the part after "?"
static char *store_56_key_url(char* url){

        char *new_url = xstrdup(url);
        char *s;

        s = strstr(new_url, "flvdownload");
	
        if( s != NULL)
        {
    	     int slen = strlen(s);
             memmove(new_url, s, slen);
             *(new_url+slen) = 0;
        }

        char *quesurl = strchr(new_url,'?');
        if(quesurl != NULL){
                *quesurl = 0;
        }

        debug(137, 3)("mod_f4v: 56 video store_url=%s \n", new_url);

        return new_url;
}

//sinavideo safeguard chain: 
static char *store_sina_key_url(char* url){

	char *new_url = NULL;	
        if(strstr(url, "http://flv.sina.fastwebcdn.com") != NULL){
		new_url = store_sinacdn_key_url(url);
	}
	else if(strstr(url, "v.iask.com") != NULL){
		new_url = store_ku6_key_url(url);	
	}
	else if(strstr(url, "str=") != NULL && strstr(url, "pid=") != NULL){
		new_url = store_ku6_key_url(url);	
	}

        debug(137, 3)("mod_f4v: sina video store_url=%s \n", new_url);

        return new_url;
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

static void modify_request(clientHttpRequest * http,char *new_uri){

	debug(137, 5)("mod_f4v: start, uri=[%s]\n", http->uri);
	request_t* old_request = http->request;
	
	request_t* new_request = urlParse(old_request->method, new_uri);

	debug(137, 5)("mod_f4v: start, new_uri=[%s]\n", new_uri);
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

static void remove_safeguard_chain(clientHttpRequest *http,store_type type){
	
        safe_free(http->request->store_url);
	if(type == STANDARD)
	        http->request->store_url = store_key_url(http->uri);
	else if(type == YOUKU)
	        http->request->store_url = store_youku_key_url(http->uri);
	else if(type == TUDOU)
	{
	        http->request->store_url = store_tudou_key_url(http->uri);
			if(http->request->store_url == NULL)
				http->request->store_url = store_key_url(http->uri);
	}
	else if(type == KU6)
	        http->request->store_url = store_ku6_key_url(http->uri);
	else if(type == QQ){
	        http->request->store_url = store_qq_key_url(http->uri);
		char *new_url = remove_cpis_domain_key_url(http->uri);
        	if(new_url != NULL){
               		modify_request(http,new_url);
               		safe_free(new_url);
        	}
	}
	else if(type == SOHU)
	        http->request->store_url = store_key_url(http->uri);
	else if(type == SINA)
	        http->request->store_url = store_sina_key_url(http->uri);
	else if(type == VIDEO56)
	        http->request->store_url = store_56_key_url(http->uri);
	else if(type == FLV56)
	        http->request->store_url = store_key_url(http->uri);

}

static int checkStrIsDigit(char *pStr, int len)
{
	int i;
	for (i = 0;i < len; i++)
	{
		if (0 == isdigit(*(pStr+i)))
		{
			debug(137, 3)("(mod_f4v) request url parameter value is not digit.\n");
			return -1;
		}
	}

	return 0;
}



static int check56FlvRequestUrlValidity(char *str, int *StreamMod, int *HeadOffset,download_mod *d_mod)
{
	assert(str);
	debug(137, 3)("(mod_f4v) check56FlvRequestUrlValidity starting...\n");
	
	if ((str = strstr(str,"m=")) == NULL) {
		debug(137, 3)("cannot find 'm=' in request parameter,check the request url\n");
		return -1;
	}
	
	if (*(str+2) == 's' || *(str+2) == 'S')
	{	debug(137, 3)("(mod_f4v) find m=s download mod is normal.\n");
		*d_mod = NORMAL;
	}
	else
	{
		debug(137, 3)("(mod_f4v) cannot find m=s download mod is ignore end value.\n");
		*d_mod = IGNORE_END;
	}

	if ((str = strstr(str,"h=")) == NULL) {
		debug(137, 3)("(mod_f4v) cannot find 'h=' in request parameter,check the request url.\n");
		return -1;
	}

	char *ptrStreamMod = str+2;//h>0 
	char *ptrHeadOffset = strstr(ptrStreamMod, "&h1=");//h1>0 
	if (ptrHeadOffset == NULL) {
		debug(137, 3)("(mod_f4v) cannot find '&h1=' in request parameter,check the request url.\n");
		return -1;
	}
	
	int StreamModLen = ptrHeadOffset - ptrStreamMod;
	char *ptrEnd = strstr(ptrHeadOffset+4,"&");
	int HeadOffsetLen = 0;

	if (ptrEnd == NULL)
	{
		HeadOffsetLen = strlen(ptrHeadOffset+4);
	}
	else 
	{
		HeadOffsetLen = ptrEnd - ptrHeadOffset-4;
	}
	
	if (checkStrIsDigit(ptrStreamMod, StreamModLen) < 0)
	{
		return -1;
	}

	if (checkStrIsDigit(ptrHeadOffset+4, HeadOffsetLen) < 0)
	{
		return -1;
	}

	*StreamMod = atol(ptrStreamMod); //h=xxx
	*HeadOffset = atol(ptrHeadOffset+4);//h1=xxx

	debug(137, 3)("(mod_f4v) StreamMod(h) = [%d],HeadOffset(h1) = [%d]\n", *StreamMod, *HeadOffset);
	if (*HeadOffset < *StreamMod)
	{
		debug(137, 3)("(mod_f4v) h1 < h error!\n");
		return -1;
	}

	return 0;
}

static int 
check_startvalue_lessthan_contentlength(clientHttpRequest *http, HttpReply * rep)
{
	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	if(cfg == NULL){
		return 0;
	}

	if (cfg->website != FLV56)
		return 0;

	request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if (req_param == NULL){
		return 0;
	}

	if (req_param->times == 0)
		req_param->times = 1;
	else if (req_param->times == 1)
		return 0;
    squid_off_t content_length = rep->content_length;
	debug(137, 3) ("(mod_f4v): flv full content_length = [%"PRINTF_OFF_T"],log_type = [%d],tcp_miss = [%d]\n",content_length,http->log_type,LOG_TCP_MISS);
	debug(137, 3) ("(mod_f4v): request start=[%d],HeadOffset(h1)=[%d]\n",req_param->start,req_param->HeadOffset);
	
	if (http->log_type == LOG_TCP_MISS)
	{
		debug(137, 3) ("(mod_f4v): log_type = [%d],tcp_miss = [%d]\n",http->log_type,LOG_TCP_MISS);
		return 0;
	}
	if (req_param->start > content_length || req_param->HeadOffset > content_length)
	{
        debug(137, 3) ("(mod_f4v):403 start value > content_length or headoffset > content_length\n");
        ErrorState *err = errorCon(ERR_ACCESS_DENIED, HTTP_FORBIDDEN, http->orig_request);
        storeClientUnregister(http->sc, http->entry, http);
        http->sc = NULL;
        if (http->reply)
          httpReplyDestroy(http->reply);
        http->reply = NULL;
        storeUnlockObject(http->entry);
        http->log_type = LOG_TCP_DENIED;
        http->entry = clientCreateStoreEntry(http, http->request->method, null_request_flags);
        errorAppendEntry(http->entry, err);
		return 1;
	}

	return 0;
}
#if 0
static void drop_args(char *arg, char *url)
{
	char *s;
	char *e;

	s = strstr(url, arg);
	if(s == NULL)
		return;

	e = strstr(s, "&");
	if( e == NULL )
		*(s-1) = 0;
	else {
		memmove(s, e+1, strlen(e+1));
		*(s+strlen(e+1)) = 0;
	}
}
#endif

static int add_range(clientHttpRequest *http)
{
	debug(137, 3)("(mod_f4v) -> read request finished, add range: \n");

	//参数不完整，忽略该请求
	if( !http || !http->uri || !http->request ) {
		debug(137, 3)("http = NULL, return \n");
		return 0;
	}

	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);

	if(cfg->website == QQ){
		if((strstr(http->uri, "type=flv")) == NULL ) {
			debug(137, 3) ("not find type=flv return %s \n",http->uri);
			return 0;
		}

		char *tmp_header = strstr(http->uri, "head=1");
		if( tmp_header != NULL ) {
			debug(137, 3) ("mod_f4v: find key head=1 and return\n");
			return 0;
		}

	}
	
	if(strstr(http->uri,".flv") == NULL && strstr(http->uri,".f4v") == NULL && strstr(http->uri,".hlv") == NULL && strstr(http->uri,".letv") == NULL)
	{
		if (cfg->website == FLV56)
			http->redirect.status = HTTP_FORBIDDEN;
		debug(137, 3)("(mod_f4v) ->has not flv video file %s \n", http->uri);
		return 0;
	}

	//fix one bug which url has start but not get request
	if(http->request->method != METHOD_GET ){
		debug(137, 1)("client url is not GET request and return %s \n", http->uri);
		return 0;
	}

	remove_safeguard_chain(http,cfg->website);

	char *tmp;

	//没有?，返回
	if( (tmp = strchr(http->uri, '?')) == NULL ) {
		debug(137, 3)("<%s> not find ?, return \n", http->uri);
		return 0;
	}

	//56flv begin
	int StreamMod = 0;
	int HeadOffset = 0;
	download_mod d_mod = NORMAL; 
	if (cfg->website == FLV56)
	{
		if ((strstr(tmp, cfg->start)!= NULL) && check56FlvRequestUrlValidity(tmp, &StreamMod, &HeadOffset,&d_mod) < 0)
		{
			http->redirect.status = HTTP_FORBIDDEN;
			return 0;
		}
		char *new_uri = xstrdup(http->uri);
		
		//case3518:56flv drag failed when use upper level
		//drop_args("m=", new_uri);
		modify_request(http,new_uri);
		safe_free(new_uri);
	}
	//56flv end
	
	//没有找到start的变量名，返回
	if( (tmp = strstr(tmp, cfg->start)) == NULL ) {
		if (cfg->website == FLV56 && d_mod == NORMAL)
		{	//first send date <= 52000000
			char range_value[32];
			snprintf(range_value, 31, "bytes=%d-%d",0 ,51199999);
			HttpHeaderEntry eEntry;
			eEntry.id = HDR_RANGE;
			stringInit(&eEntry.name, "Range");
			stringInit(&eEntry.value, range_value);

			httpHeaderAddEntry(&http->request->header, httpHeaderEntryClone(&eEntry));
			stringClean(&eEntry.name);
			stringClean(&eEntry.value);
		}

		debug(137, 3) ("not find start return %s %s %s \n",http->uri,tmp,cfg->start);
		return 0;
	}

	tmp--;
	if( *tmp != '?' && *tmp != '&' ) {
		debug(137, 3)("<%s> has not start variable, return \n", http->uri);
		return 0;
	}
	tmp++;

	//找起始偏移量
	int start_value;
	tmp += strlen(cfg->start);

	//56flv begin
	if (cfg->website == FLV56)
	{
		char *tmp2 = strstr(tmp,"&");
		int start_len;
		if (tmp2 == NULL)
		{
			start_len = strlen(tmp);
		}
		else
		{
			start_len = tmp2 - tmp;	
		}
		
		if (checkStrIsDigit(tmp, start_len) < 0)
		{
			http->redirect.status = HTTP_FORBIDDEN;
			return 0;
		}
	}
	//56flv end

	start_value = atol(tmp);

	int end_value = 0;
	if( (tmp = strstr(tmp, cfg->end)) ) {
		tmp--;
		if( *tmp == '?' || *tmp == '&' ) {
			tmp++;
			tmp += strlen(cfg->end);

			//56flv begin
			if (cfg->website == FLV56)
			{
				char *tmp2 = strstr(tmp,"&");
				int end_len;
				if (tmp2 == NULL)
				{
					end_len = strlen(tmp);
				}
				else
				{
					end_len = tmp2 - tmp;
				}

				if (checkStrIsDigit(tmp, end_len) < 0)
				{
					http->redirect.status = HTTP_FORBIDDEN;
					return 0;
				}
			}
			//56flv end

			end_value = atol(tmp);
		}
		debug(137, 3)("mod_f4v: start_value = [%d],end_value = [%d]\n",start_value, end_value);
		if (cfg->website == FLV56 && end_value < start_value)
			http->redirect.status = HTTP_FORBIDDEN;
	}

/*
	drop_startend(cfg->start, cfg->end, http->uri);
	if( http->uri[strlen(http->uri)-1] == '?')
		http->uri[strlen(http->uri)-1] = 0;


	drop_startend(cfg->start, cfg->end, http->request->urlpath.buf);
	if( http->request->urlpath.buf[strlen(http->request->urlpath.buf)-1] == '?')
		http->request->urlpath.buf[strlen(http->request->urlpath.buf)-1] = 0;
	http->request->urlpath.len=strlen(http->request->urlpath.buf);


	drop_startend(cfg->start, cfg->end, http->request->canonical);
	if( http->request->canonical[strlen(http->request->canonical)-1] == '?')
		http->request->canonical[strlen(http->request->canonical)-1] = 0;
*/
	restore_http(http);//remove range header if request url has 'start=N'

	if(cfg->type == BYTES || cfg->type == BYTES_CP_ALL){
		if(cfg->type == BYTES)
		{
			if( start_value > 13)
			{
				if ((cfg->website == FLV56 && HeadOffset == 0) || (cfg->website != FLV56))//56,h263 and h264 cp all
					start_value -= 13;
			}
			else
				start_value = 0;
		}
		char range_value[32];

		//56flv begin
		debug(137, 3)("(mod_f4v) -> d_mod = [%d] IGNORE_END = [%d]\n",d_mod,IGNORE_END);

		if(cfg->website == FLV56)
		{
			if(d_mod == IGNORE_END)
				snprintf(range_value, 31, "bytes=%d-", start_value);
			else if (d_mod == NORMAL)
			{
				if (end_value)
				{
		//			if (end_value > 50*1024*1024)
		//				snprintf(range_value, 31, "bytes=%d-%d", start_value, 50*1024*1024);
		//			else 
						snprintf(range_value, 31, "bytes=%d-%d", start_value, end_value);
				}
				else
					snprintf(range_value, 31, "bytes=%d-", start_value);
			}
		}

		if(cfg->website != FLV56)
		{
        	if( end_value )
                	snprintf(range_value, 31, "bytes=%d-%d", start_value, end_value);
        	else
                	snprintf(range_value, 31, "bytes=%d-", start_value);
		}
		//56flv end	
        	HttpHeaderEntry eEntry;
        	eEntry.id = HDR_RANGE;
        	stringInit(&eEntry.name, "Range");
        	stringInit(&eEntry.value, range_value);

       		httpHeaderAddEntry(&http->request->header, httpHeaderEntryClone(&eEntry));
        	stringClean(&eEntry.name);
        	stringClean(&eEntry.value);
	}
	
	request_param *req_param = request_param_pool_alloc();
        cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, req_param, free_request_param, mod);

	req_param->flag = 1;
	req_param->succ = -1;
	req_param->start = start_value;
	req_param->end   = end_value;
	req_param->is_real_flv = HIT_FLV;
	req_param->cur_pos = 0;
	req_param->StreamMod = StreamMod;
	req_param->HeadOffset = HeadOffset;
	req_param->times = 0;

	debug(137, 3)("(mod_f4v) -> read request finished, add range: done start=%d end=%d \n",start_value,end_value);
	
	debug(137, 5)("mod_f4v: paramter start= %s end= %s type= %d miss_strategy= %d right_auto= %d website = %d \n", cfg->start, cfg->end, cfg->type,cfg->miss_strategy,cfg->has_right_auto,cfg->website);
        
	//remove_safeguard_chain(http,cfg->website);

	return 0;
}


static void add_flv_header(clientHttpRequest* http, char* buff, int size)
{
	if(http == NULL)
	{
		debug(137, 1)("(mod_f4v) ->http request in add_flv_header is NULL, return now!\n");
		return;
	}
	request_param *req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if( req_param == NULL ) {
		debug(137, 2)("(mod_f4v) ->http request param in add_flv_header is NULL, return now!\n");
		return;
	}


	if(req_param->is_real_flv != HIT_FLV || req_param->flag == 0){
		debug(137, 2)("(mod_f4v) ->http request param in add_flv_header is not fit, return now!\n");
		return;
	}
	
	if( req_param->flag == 2 )
		return;

	assert(req_param->flag == 1);

	if( size < 13 )
	{
		debug(137, 1)("(mod_f4v) ->buffer size in add_flv_header is not large enough, return now!\n");
		return;
	}

	req_param->flag = 2;
 	
	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	if(cfg == NULL){
		debug(137, 1)("(mod_f4v) ->config mod param in add_flv_header is NULL, return now!\n");
		return ;
	}
	
	
	if(cfg->type == BYTES&&cfg->website != VIDEO56&& cfg->website != TUDOU && !cfg->cp_all){
		if (cfg->website != FLV56)
		{
			const unsigned char flv[13] = {'F', 'L', 'V', 1, 1, 0, 0, 0, 9, 0, 0, 0, 9};
			memcpy(buff, flv, 13);
		}
		else if (cfg->website == FLV56 && req_param->StreamMod == 0)//56flv h263 mod
		{
			const unsigned char flv[13] = {'F', 'L', 'V', 1, 5, 0, 0, 0, 9, 0, 0, 0, 0};
			memcpy(buff, flv, 13);
		}

		store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, http->entry, mod);

		if(str_param == NULL ) {
			debug(137, 1)("(mod_f4v) ->entry private data in add_flv_header is NULL, return now!\n");
			return;
		}

		debug(137, 5)("mod_f4v: buff size: %d\n", size);
		debug(137, 5)("mod_f4v: copy f4v len  %d\n",str_param->offset);
//		debug(137, 3)("mod_f4v: copy_buf first[%c]\n", str_param->flv_pre[0]);
		debug(137, 5)("mod_f4v: req_param->cur_pos = [%d]\n", req_param->cur_pos);
		debug(137, 5)("mod_f4v: str_param->offset = [%d]\n", str_param->offset);
		//56flv
		if (cfg->website == FLV56) 
		{
			if (req_param->StreamMod > 0)//56flv h264 mod
			{
				if(str_param->flv_pre)
				{
					int left = str_param->offset - req_param->cur_pos;
					int cp_len;
					if(size > left)
					{
						cp_len = left;
					}
					else
					{
						cp_len = size;
						req_param->flag = 1;
					}

					memcpy(buff, str_param->flv_pre + req_param->cur_pos, cp_len);
					
					if(req_param->flag == 1)
						req_param->cur_pos += size;
				}
			}
			else if (str_param->flv_pre && (size > str_param->offset)) //h263
				memcpy(buff+13, str_param->flv_pre, str_param->offset);
		}
		else
		{
			if(str_param->flv_pre && (size > str_param->offset))
				memcpy(buff+13, str_param->flv_pre, str_param->offset);
		}
	}
	else if(cfg->type == BYTES_CP_ALL){
		store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, http->entry, mod);

		if(str_param == NULL ) {
			debug(137, 1)("(mod_f4v) ->entry private data in add_flv_header is NULL, return now!\n");
			return;
		}

		debug(137, 5)("mod_f4v: buff size: %d\n", size);
		debug(137, 5)("mod_f4v: copy f4v len  %d\n",str_param->offset);
//		debug(137, 3)("mod_f4v: copy_buf first[%c]\n", str_param->flv_pre[0]);
		if(str_param->flv_pre)
		{
			int left = str_param->offset - req_param->cur_pos;
			int cp_len;

			if(size > left)
			{
				cp_len = left;
			}
			else
			{
				cp_len = size;
				req_param->flag = 1;
			}

			memcpy(buff, str_param->flv_pre + req_param->cur_pos, cp_len);

			if(req_param->flag == 1)
				req_param->cur_pos += size;
		}
	}
	else if(cfg->type == TIMES ||cfg->website == VIDEO56 || cfg->website == TUDOU || cfg->cp_all){
		store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, http->entry, mod);
		if(str_param == NULL ) {
			return;
		}

		debug(137, 5)("mod_f4v: copy f4v len  %d\n",str_param->offset);
		debug(137, 5)("mod_f4v: copy f4v data %s\n",str_param->flv_pre);

		if(str_param->flv_pre)
		{
			//int left = str_param->flv_len - req_param->cur_pos;
			int left = str_param->offset - req_param->cur_pos;
			int cp_len;

			if(size > left)
			{
				cp_len = left;
			}
			else
			{
				cp_len = size;
				req_param->flag = 1;
			}

			memcpy(buff, str_param->flv_pre + req_param->cur_pos, cp_len);

			if(req_param->flag == 1)
				req_param->cur_pos += size;
		}
	}
}


static int mod_reply_header(clientHttpRequest *http)
{
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
		if(cfg == NULL){
			return 0;
		}
		if (cfg->website == FLV56) {
			if(http->reply->sline.status == HTTP_PARTIAL_CONTENT )
				http->reply->sline.status = HTTP_OK;
		}
		return 0;
	}

	if(req_param->is_real_flv != HIT_FLV || req_param->flag == 0){
		return 0;
	}
	
	assert(req_param->flag == 1);

	HttpReply *rep = http->reply;

	if( rep->sline.status == HTTP_PARTIAL_CONTENT )
		rep->sline.status = HTTP_OK;

	return 0;
}

static void mediaPrefetchHandleReplyForMissStart(void *data, char *reply)
{
        media_prefetch_state_data* state = data;
                
        if(strcmp(reply, "OK"))
        {
                debug(137,3)("mod_f4v can not prefetch media! reply=[%s] \n",reply);
        }
        else{
                debug(137,3)("mod_f4v: quering whole object is success reply=[%s] \n",reply);
        }

        cbdataFree(state);
}


static int isConfKey(char *token){
	if((token != NULL) && ((strcasecmp(token,"allow")!=0)
                        && ( strcasecmp(token,"deny")!=0 )
                        && ( strcasecmp(token,"miss_wait")!=0 )
                        && ( strcasecmp(token,"miss_start")!=0 )
                        && ( strcasecmp(token,"query_whole")!=0 )
                        && ( strcasecmp(token,"query_start")!=0 )
                        && ( strcasecmp(token,"right_auto")!=0 )
                        && ( strcasecmp(token,"youku")!=0 )
                        && ( strcasecmp(token,"tudou")!=0 )
                        && ( strcasecmp(token,"ku6")!=0 )
                        && ( strcasecmp(token,"qq")!=0 )
                        && ( strcasecmp(token,"sohu")!=0 )
                        && ( strcasecmp(token,"sina")!=0 )
                        && ( strcasecmp(token,"flv56")!=0 )
                        && ( strcasecmp(token,"standard")!=0))){
 
                return 0;
        }
        return 1;

}

static int func_sys_parse_param(char *cfg_line)
{
	if( strstr(cfg_line, "allow") == NULL )
		return 0;

	if( strstr(cfg_line, "mod_f4v") == NULL )
		return 0;
		
	int miss_strategy = 0;
	is_start_helper = 0;
	int cp_all = 0;

   	if( strstr(cfg_line, "miss_wait") != NULL )
        {
                miss_strategy = 1;
                is_start_helper = 1;
        }
        else if(strstr(cfg_line, "miss_start") != NULL )
        {
                miss_strategy = 2;
                is_start_helper = 1;
        }
        else if(strstr(cfg_line, "query_start") != NULL )
        {
                miss_strategy = 3;
                is_start_helper = 0;
        }
	else if(strstr(cfg_line, "query_whole") != NULL )
        {
                miss_strategy = 0;
                is_start_helper = 0;
        }
	
	int has_right_auto = 0;
	if( strstr(cfg_line, "right_auto") != NULL )
	{
		has_right_auto = 1;
	}
	
	store_type website_type = STANDARD;
	if( strstr(cfg_line, "standard") != NULL )
	{
		 website_type = STANDARD;
	}
	else if( strstr(cfg_line, "youku") != NULL )
	{
		 website_type = YOUKU;
	}
	else if( strstr(cfg_line, "tudou") != NULL )
	{
		 website_type = TUDOU;
	}
	else if( strstr(cfg_line, "ku6") != NULL )
	{
		 website_type = KU6;
	}
	else if( strstr(cfg_line, "qqvideo") != NULL )
	{
		 website_type = QQ;
	}
	else if( strstr(cfg_line, "sohu") != NULL )
	{
		 website_type = SOHU;
	}
	else if( strstr(cfg_line, "sina") != NULL )
	{
		 website_type = SINA;
	}
	else if (strstr(cfg_line, "flv56") != NULL)
	{
		 website_type = FLV56;
	}
	else if( strstr(cfg_line, "56video") != NULL )
	{
		 website_type = VIDEO56;
	}

	if(strstr(cfg_line, "cp_all") != NULL)
	{
		cp_all = 1;
	}
		

	char *token = NULL;

	if ((token = strtok(cfg_line, w_space)) == NULL)
		return 0;

	struct mod_config *cfg = mod_config_pool_alloc();
	memset(cfg, 0, sizeof(struct mod_config));
	
	token = strtok(NULL, w_space);
	strncpy(cfg->start, token, 126);
	strcat(cfg->start, "=");

	token = strtok(NULL, w_space);
	strncpy(cfg->end, token, 126);
	strcat(cfg->end, "=");

	int type = 0;//default support flv type for bytes
	//check the type type:1 means seconds, 0 means bytes
        int isflag = 1;
        int paravalue = -1;
        int isFinished = 0;
        //check if the flag type

        token = strtok(NULL, w_space);
        if(!isConfKey(token)){
                paravalue = atoi(token);
                if(paravalue == 0 || paravalue == 1 || paravalue == 2){
                        isflag = 0;
                        type = paravalue;
                }
                else if(paravalue > 3){
						if(helper_proc_num != paravalue)
						{
							debug(137,3)("f4v:change helper numbers --by w.yuan for debug\n");
							helper_proc_num = paravalue;
							is_load_helper = 1;
						}
                }
        }
        else{
                 isFinished = 1;
        }

        paravalue = -1;
        token = strtok(NULL, w_space);
        if(isFinished == 0 && !isConfKey(token)){
                paravalue = atoi(token);
                if(paravalue == 0 || paravalue == 1 || paravalue == 2){
                        isflag = 0;
                        type = paravalue;
                }
                else if(paravalue > 3){
						if(helper_proc_num != paravalue)
						{
							debug(137,3)("f4v:change helper numbers --by w.yuan for debug\n");
							helper_proc_num = paravalue;
							is_load_helper = 1;
						}
                }
        }
        else{
                 isFinished = 1;
        }

	if(type == 1)
		cfg->type = TIMES;
	else if(type == 2)
		cfg->type = BYTES_CP_ALL;
	else
		cfg->type = BYTES;

	token = strtok(NULL, w_space);
	if((token != NULL) && ((strcasecmp(token,"allow")!=0) 
			&& ( strcasecmp(token,"deny")!=0 )
			&& ( strcasecmp(token,"miss_wait")!=0 )
			&& ( strcasecmp(token,"miss_start")!=0 )
			&& ( strcasecmp(token,"query_whole")!=0 )
			&& ( strcasecmp(token,"query_start")!=0 )
			&& ( strcasecmp(token,"right_auto")!=0 )
			&& ( strcasecmp(token,"youku")!=0 )
			&& ( strcasecmp(token,"tudou")!=0 )
			&& ( strcasecmp(token,"ku6")!=0 )
			&& ( strcasecmp(token,"qqvideo")!=0 )
			&& ( strcasecmp(token,"sohu")!=0 )
			&& ( strcasecmp(token,"sina")!=0 )
			&& ( strcasecmp(token,"56video")!=0 )
			&& ( strcasecmp(token,"standard")!=0))){
		helper_proc_num = atoi(token);
	}

	cfg->miss_strategy = miss_strategy;
	cfg->has_right_auto = has_right_auto;
	cfg->website = website_type;
	cfg->cp_all = cp_all;

    	debug(137, 5) ("mod_f4v: paramter start= %s end= %s type= %d miss_strategy= %d right_auto= %d website= %d helper_proc_num=%d \n", cfg->start, cfg->end, cfg->type,cfg->miss_strategy,cfg->has_right_auto,cfg->website,helper_proc_num);
	
	cc_register_mod_param(mod, cfg, free_cfg);
	return 0;
}

static int is_disable_sendfile(store_client *sc, int fd) 
{
	sc->is_zcopyable = 0;
	return 1;
}

static void mediaPrefetchHandleReply(void *data, char *reply)
{
	media_prefetch_state_data* state = data;
		
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, state->http, mod);
	if( req_param == NULL){
		return ;
	}

/*	
	if(strcmp(reply, "OK"))
	{
		debug(137,1)("mod_f4v can not prefetch media!\n");
		request_t *r = state->http->request;
		ErrorState *err = errorCon(ERR_FTP_NOT_FOUND, HTTP_NOT_FOUND, r);
		err->url = xstrdup(state->http->uri);
		state->http->entry = clientCreateStoreEntry(state->http, r->method, null_request_flags);
		errorAppendEntry(state->http->entry, err);
		return;
	}

*/
        if(reply == NULL){

                debug(137,1)("mod_f4v reply is null \n");
                req_param->succ = 1;

        }
        else{
                if(strcmp(reply, "OK"))
                {
                        debug(137,1)("mod_f4v can not prefetch media! reply=[%s] \n",reply);
                        state->http->request->flags.cachable = 0;
                        req_param->succ = 1;
                }
                else{
                        debug(137,1)("***mod_f4v can cache***\n");
                        state->http->request->flags.cachable = 1;
                        req_param->succ = 0;//0 means helper download res from upserver successfully
                }
        }

	debug(137,3)("mod_f4v prefetched media, and it should be a hit!\n");

	int valid = cbdataValid(state->http);
	cbdataUnlock(state->http);
	cbdataUnlock(state);
	if(valid)
	{
		state->callback(state->http);
	}
	cbdataFree(state);
}

static void restore_http(clientHttpRequest *http){

	request_t *req = http->request;	
	if(req == NULL){
                debug(137, 2)("mod_f4v: http request is null in restore_http \n");
                return;
        }       
        
	HttpHeaderEntry *range = NULL;
        range = httpHeaderFindEntry(&req->header, HDR_RANGE);
        if(range != NULL){
                
        	httpHeaderDelById(&req->header, HDR_RANGE);
        }
	return;

}

static int mod_f4v_http_req_process(clientHttpRequest *http){
	
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		return 0;
	}
	if(req_param->is_real_flv == HIT_OTHER){

                return 0;
        }

	struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
	assert(cfg);
	
	int has_header = httpHeaderCheckExistByName(&http->request->header, "X-CC-Media-Prefetch");
	if (http->log_type == LOG_TCP_MISS && req_param->succ == 1){
		debug(137,3)("mod_f4v: curl reqeust is fail and fetch all whole file \n");
		req_param->succ = 0;
		req_param->flag = 0;
		restore_http(http);

	}
	else if (http->log_type == LOG_TCP_MISS && cfg->miss_strategy == 0){
		debug(137,3)("mod_f4v: request store_url %s and miss to query all whole object to web server \n",http->request->store_url);
		req_param->flag = 0;
		restore_http(http);
		
		char *new_uri = xstrdup(http->uri);
		drop_startend(cfg->start, cfg->end, new_uri);
		modify_request(http,new_uri);

		safe_free(new_uri);
		//http->miss_status = 0;


	}
	else if( http->log_type == LOG_TCP_MISS && !has_header && cfg->miss_strategy == 1 && req_param->succ != 0)
	{
		debug(137,3)("sending data to helper because miss_wait http->log_type=%d, cfg->miss_strategy=%d\n ", http->log_type, cfg->miss_strategy);
		debug(137,3)("client address : %s,url : %s\n", inet_ntoa(http->request->client_addr),http->uri);
		req_param->http_miss_wait = 1;

		//http->miss_status = 1;
	
		CBDATA_INIT_TYPE(media_prefetch_state_data);
		media_prefetch_state_data *state = cbdataAlloc(media_prefetch_state_data);
		
		/* case 2864: no cache for partial file */
		//http->request->flags.cachable = 0;
		state->http = http;
		state->callback = clientProcessRequest;

		cbdataLock(state);
		cbdataLock(state->http);

		char *new_uri = xstrdup(http->uri);
        drop_startend(cfg->start, cfg->end, new_uri);

        static char buf[4096];
        snprintf(buf, 4096, "%s\n", new_uri);
		safe_free(new_uri);

		helperSubmit(media_prefetchers, buf, mediaPrefetchHandleReply, state);
		return 1;
	}
	else if( http->log_type == LOG_TCP_MISS && cfg->miss_strategy == 2&& !has_header &&req_param->succ != 0)
        {
                debug(137,3)("mod_f4v: request miss_start object because http->log_type=%d, cfg->miss_strategy=%d\n", http->log_type, cfg->miss_strategy);
                StoreEntry *e = NULL;
                if(http->request->store_url == NULL)
                {
                    debug(137,3)("mod_f4v: request uri: %s, but it has no store_url!\n", http->uri);
                    return 0;
                }
                e = storeGetPublic(http->request->store_url,METHOD_GET);

		if(http->request->store_url)
                {
                        e = storeGetPublic(http->request->store_url,METHOD_GET);
                }
                else
                {
                        debug(137,0)("Warnning mod_f4v: http->uri: [%s] in f4v mod, but has no store_url!!!\n", http->uri);

                        return 0;
                }

		//http->miss_status = 2;
                if(e == NULL){//query whole object

                        CBDATA_INIT_TYPE(media_prefetch_state_data);
                        media_prefetch_state_data *state = cbdataAlloc(media_prefetch_state_data);
                        state->http = NULL;
                        state->callback = NULL;

                        char *new_uri = xstrdup(http->uri);
                        drop_startend(cfg->start, cfg->end, new_uri);

                        debug(137, 2) ("mod_f4v: flv miss and redirect to upper layer to query whole data %s \n",new_uri);
                        static char buf[4096];
                        snprintf(buf, 4096, "%s\n", new_uri);
                        safe_free(new_uri);
                        helperSubmit(media_prefetchers, buf, mediaPrefetchHandleReplyForMissStart, state);
                }

		if(e == NULL|| e->store_status != STORE_OK){

			debug(137, 2) ("mod_f4v: f4v pending and redirect to upper layer to query start data %s\n",http->uri);
			req_param->flag = 0;
			restore_http(http);
			http->request->flags.cachable = 0;//no cache
			safe_free(http->request->store_url);
			//modify_request(http,http->uri);

			http->entry = NULL;
			http->log_type = LOG_TCP_MISS;


		}
                else{
                        debug(137, 3) ("flv hit request http->uri = %s\n", http->uri);
                }

               return 0;
        }
	else if( http->log_type == LOG_TCP_HIT && cfg->miss_strategy == 2&& !has_header &&req_param->succ != 0)
        {
                debug(137,3)("mod_f4v: request miss_start hit object because http->log_type=%d, cfg->miss_strategy=%d\n", http->log_type, cfg->miss_strategy);
                StoreEntry *e = NULL;

		if(http->request->store_url == NULL)
		{
			debug(137,3)("mod_f4v: request uri: %s, but it has no store_url!\n", http->uri);
			return 0;
		}
		e = storeGetPublic(http->request->store_url,METHOD_GET);

		if(http->request->store_url)
                {
                        debug(137,3)("mod_f4v: request store_url is: %s\n", http->request->store_url);
                        e = storeGetPublic(http->request->store_url,METHOD_GET);
                }
                else
                {
                        debug(137,0)("Warnning mod_f4v: http->uri: [%s] in f4v mod, but has no store_url!!!\n", http->uri);
                        return 0;
                }

		if(e == NULL){
			debug(137, 2) ("mod_f4v: flv hit but has not store entry  %s \n",http->uri);
                        return 0;
                }

		if(e->store_status != STORE_OK){

			debug(137, 2) ("mod_f4v: f4v pending and redirect to upper layer to query start data %s\n",http->uri);
                        req_param->flag = 0;
                        restore_http(http);
			http->request->flags.cachable = 0;//no cache
                        safe_free(http->request->store_url);
			//modify_request(http,http->uri);

			http->entry = NULL;
			http->log_type = LOG_TCP_MISS;

		}
		else{
			debug(137, 3) ("flv hit request http->uri = %s\n", http->uri);
		}

		return 0;
	}
	else 
	{
		debug(137,3)("mod_f4v: query start for http->log_type=%d, cfg->miss_wait=%d\n ", http->log_type,cfg->miss_strategy);
	}

	return 0;
}

static void hook_before_sys_init()
{
	struct stat statbuf_of_this_load;
	static struct stat statbuf_of_last_load;
	static int reload_count = 0;
	
	if(is_start_helper == 0){

		debug(137,3)("mod_f4v: module cfg is_start_helper=%d and don't start helper program\n",is_start_helper);
		
		if (media_prefetchers)
                {
                        helperShutdown(media_prefetchers);
                        helperFree(media_prefetchers);
                        media_prefetchers = NULL;
                }

		return;	
	}


	debug(137,3)("mod_f4v: module cfg is_start_helper=%d and start helper program\n",is_start_helper);
	debug(137,3)("mod_f4v: helper process number  =%d \n",helper_proc_num);

	stat("/usr/local/squid/bin/media_prefetch.pl", &statbuf_of_this_load);
	if(!reload_count++)
	{
		is_load_helper = 1;
	}
	else if(statbuf_of_this_load.st_mtime != statbuf_of_last_load.st_mtime)
	{
		is_load_helper = 1;
	}
	memcpy(&statbuf_of_last_load, &statbuf_of_this_load, sizeof(struct stat));

	if (media_prefetchers && is_load_helper)
	{
		debug(137,3)("f4v:end helper --by w.yuan for debug\n");
        helperShutdown(media_prefetchers);
        helperFree(media_prefetchers);
        media_prefetchers = NULL;
	}

	if(is_load_helper)
	{
		is_load_helper = 0;
		debug(137,3)("f4v:start helper --by w.yuan for debug\n");
		media_prefetchers = helperCreate("media_prefetcher");
		wordlist *pwl = NULL; 
		char cfg_line2[128];
		strcpy(cfg_line2,"a /usr/local/squid/bin/media_prefetch.pl");
		strtok(cfg_line2, w_space);
		parse_programline(&pwl);
	
		media_prefetchers->cmdline = pwl;
		media_prefetchers->n_to_start = helper_proc_num;
		media_prefetchers->concurrency = 0;
		media_prefetchers->ipc_type = IPC_STREAM;
		helperOpenServers(media_prefetchers);
	}

}

/*
static void cleanup_registered_deferfunc_when_reconfig()
{
        if (media_prefetchers)
        {
                helperShutdown(media_prefetchers);
                helperFree(media_prefetchers);
                media_prefetchers = NULL;
        }
}
*/

// like mp4 mod_mp4_client_send_more_data function to release entry of MISS/200 reply of IMS request  
// ( http://xxxx...?start=123)  hit, but expired, then If-Modify-Since ??? ... <---------------> MISS/200
static int mod_f4v_release_incomplete_entry(clientHttpRequest * http, MemBuf *mb,char *buf, ssize_t size)
{
	//fix one bug for http 404 NOT FOUND buf ,only 200 ok http reply we can support mp4 drag and drop
	if(http->reply->sline.status != HTTP_OK ){
		return 0;
	}

	if(strstr(http->uri,".flv") == NULL && strstr(http->uri,".f4v") == NULL && strstr(http->uri,".hlv") == NULL && strstr(http->uri,".letv") == NULL){
		debug(137, 3)("(mod_f4v) ->has not flv video file %s \n", http->uri);
		return 0;
	}

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if( req_param == NULL){
		//debug(121,5)("mod_mp4: when send more data ,request param is null\n");
		return 0;
	}

	if(req_param->is_real_flv != HIT_FLV || req_param->flag == 0){
		return 0;
	}

	StoreEntry *entry = http->entry;

	if(!entry || entry->swap_dirn == -1){
#if 1   
		//fix case 2927
		if (entry) {
			if (entry->mem_obj->store_url) {
				if (storeGet(storeKeyPublic(entry->mem_obj->store_url,METHOD_GET)))
				{
					storeRelease(storeGet(storeKeyPublic(entry->mem_obj->store_url,METHOD_GET)));
					debug(137, 2)("(mod_f4v) ->     release incomplete IMS reply entry\n");

					// avoid add_flv_header
					req_param->is_real_flv = HIT_OTHER;
				}
				else
				{
					debug(137, 3)("(mod_f4v) -> no.1\n");
				}
			}
			else
			{
				debug(137, 3)("(mod_f4v) -> no.2\n");
			}
		}
		else
		{
			debug(137, 3)("(mod_f4v) -> no.3\n");
		}
#endif
	}
	else
	{
		debug(137, 9)("(mod_f4v) -> ok\n");
	}

	return 0;
}

static int checkHasMp4(clientHttpRequest *http)
{
        if(strstr(http->uri,".mp4") == NULL){
                return 0;
        }

        struct mod_config *cfg = cc_get_mod_param(http->conn->fd, mod);
        if(cfg->website != QQ){
                return 0;
        }
        //get store_entry and path

        StoreEntry *e = NULL;
        char * uri = store_qq_key_url(http->uri);
        debug(137,3)("mod_f4v :check has mp4:, modify uri=[%s]\n" ,uri);
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
                debug(137, 3) ("mod_f4v: modify request http->uri = %s\n", new_uri);

                modify_request(http,new_uri);

                safe_free(new_uri);
        }
        else{
                debug(137, 3) ("mod_f4v:mp4 hit request http->uri = %s\n", http->uri);
        }

        return 0;

}

int mod_register(cc_module *module)
{
	debug(137, 1)("(mod_f4v) ->	mod_register:\n");

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
	
	//module->hook_func_http_req_process = mod_f4v_http_req_process;
        cc_register_hook_handler(HPIDX_hook_func_http_req_process,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
                        mod_f4v_http_req_process);
	
	//module->hook_func_before_sys_init = hook_before_sys_init;

	cc_register_hook_handler(HPIDX_hook_func_before_sys_init,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_sys_init),
                        hook_before_sys_init);
		
	//module->hook_func_client_access_check_done = checkHasMp4;	
	cc_register_hook_handler(HPIDX_hook_func_client_access_check_done,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_access_check_done),
                        checkHasMp4);
	
	//module->hook_func_private_mp4_send_more_data = mod_mp4_client_send_more_data;
        cc_register_hook_handler(HPIDX_hook_func_private_mp4_send_more_data,
                        module->slot, 
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_mp4_send_more_data),
                        mod_f4v_release_incomplete_entry);

        cc_register_hook_handler(HPIDX_hook_func_send_header_clone_before_reply,
                    module->slot, 
                    (void **)ADDR_OF_HOOK_FUNC(module, hook_func_send_header_clone_before_reply),
                    check_startvalue_lessthan_contentlength);
/*	
	cc_register_hook_handler(HPIDX_hook_func_module_reconfigure,
                        module->slot,
                        (void **)ADDR_OF_HOOK_FUNC(module, hook_func_module_reconfigure),
                        cleanup_registered_deferfunc_when_reconfig);
*/

	//mod = module;

	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif


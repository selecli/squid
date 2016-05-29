#include "cc_framework_api.h"
#include "sunday.h"
#include <zlib.h>

#ifdef CC_FRAMEWORK
#define MAX_HX_CONTENT_LEN 256
#define MAX_CHARSET_LEN 20
#define CHARSET_COUNT 5

static cc_module* mod = NULL;

static char* searchContent(const char * prevBuf, const int prevBufLen, const char * buf, const int bufLen, const char *charset, char *lastPart, int *lastPartLen);

typedef struct _sc_param
{
	String charset;
	char* prevBuf;
	int   prevBufLen;
	z_stream *strm;
	int getCharsetRetries;
	int isCharsetDetermined;
} sc_param;

typedef struct _mod_conf_param {
	String filter_path;	
}mod_conf_param;


static MemPool * sc_param_pool = NULL;
static MemPool * mod_config_pool = NULL;
static MemPool * z_stream_pool = NULL;

static void * sc_param_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == sc_param_pool)
	{
		sc_param_pool = memPoolCreate("mod_polityword_filter private_data sc_param", sizeof(sc_param));
	}
	return obj = memPoolAlloc(sc_param_pool);
}

static void * mod_config_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == mod_config_pool)
	{
		mod_config_pool = memPoolCreate("mod_polityword_filter config_struct", sizeof(mod_conf_param));
	}
	return obj = memPoolAlloc(mod_config_pool);
}

static void * z_stream_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == z_stream_pool)
	{
		z_stream_pool = memPoolCreate("mod_polityword_filter private_data sc_param->z_stream", sizeof(z_stream));
	}
	return obj = memPoolAlloc(z_stream_pool);
}

static int free_callback(void* param_data)
{
	mod_conf_param *param = (mod_conf_param*) param_data;
	if(param){
		stringClean(&param->filter_path);
		memPoolFree(mod_config_pool, param);
		param = NULL;
	}
	return 0;
}

int free_sc_param(void* data)
{
	debug(122,4)("free_sc_param\n");
	sc_param *param = (sc_param *) data;
	if(param != NULL){
		stringClean(&param->charset);
		memFreeBuf(MAX_HX_CONTENT_LEN, param->prevBuf);
		if(param->strm)
		{
			inflateEnd(param->strm);
			memPoolFree(z_stream_pool, param->strm);
			param->strm = NULL;
		}
		memPoolFree(sc_param_pool, param);
		param = NULL;
	}
	return 0;
}

static int createError(HttpStateData * httpState, int isStoreEntryReset){
	
	int fd = httpState->fd;
	if(isStoreEntryReset)
	{
		storeEntryReset(httpState->entry);
		fwdFail(httpState->fwd, errorCon(ERR_FTP_NOT_FOUND,HTTP_NOT_FOUND, httpState->fwd->request));
	}
	httpState->fwd->flags.dont_retry = 1;
	comm_close(fd);
	
	return 0;
}

static HttpHeaderEntry* httpHeaderFindEntry2(const HttpHeader * hdr, http_hdr_type id)
{
	HttpHeaderPos pos = HttpHeaderInitPos;
	HttpHeaderEntry *e;         
	assert(hdr);

	/* check mask first */
	if (!CBIT_TEST(hdr->mask, id))
		return NULL;    
	/* looks like we must have it, do linear search */
	while ((e = httpHeaderGetEntry(hdr, &pos))) {
		if (e->id == id)    
			return e;       
	}
	/* hm.. we thought it was there, but it was not found */
	assert(0);            
	return NULL;        /* not reached */
}

static const char * httpHeaderGetStr2(const HttpHeader * hdr, http_hdr_type id)
{
	HttpHeaderEntry *e;
	if ((e = httpHeaderFindEntry2(hdr, id)))
	{
		return strBuf(e->value);
	}
	return NULL;
}
//static char * normalizeCharset(char * in)
static void normalizeCharset(String * charset, char * in)
{
	//char *ret = xcalloc(MAX_CHARSET_LEN, sizeof(char));
	if(!strcasecmp(in, "gb2312") ||
		!strcasecmp(in, "gb-2312") ||
		!strcasecmp(in, "gbk") ||
		!strcasecmp(in, "gb18030") ||
		!strcasecmp(in, "gb-18030")
		)
	{
//		strcpy(ret, "gb2312");
		stringInit(charset, "gb2312");
	}
	else if(!strcasecmp(in, "utf8") ||
		!strcasecmp(in, "utf-8")
			)
	{
		//strcpy(ret, "utf8");
		stringInit(charset, "utf8");
	}
	else if(!strcasecmp(in, "big5")
			)
	{
		//strcpy(ret, "big5");
		stringInit(charset, "big5");
	}
	debug(122,3)("normalizeCharset from [%s] to [%s]\n", in, charset->buf);
//	return ret;
}

static int func_buf_recv_from_s_header(HttpStateData * httpState, char *buf, ssize_t len)
{
	StoreEntry *entry = httpState->entry;
	if(len == 0){
		return 0;
	}

	sc_param *data = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, mod);
	if(data == NULL){
		data = sc_param_pool_alloc();
		data->getCharsetRetries = 3;
		data->isCharsetDetermined = 0;
		//get content-type charset of http header		
		HttpReply *reply = entry->mem_obj->reply;
		const char* content_type = httpHeaderGetStr(&reply->header, HDR_CONTENT_TYPE);
		if(content_type != NULL){
			char *charset = strstr(content_type,"charset=");
			if(charset != NULL){
				char *token = strtok(charset + 8, " ;");
		//		data->charset = normalizeCharset(token);
				normalizeCharset(&data->charset, token);
				
				data->isCharsetDetermined = 1;
				debug(122,3)("got charset from header: %s\n ",strBuf(data->charset));
			}
		
			if(!strstr(content_type, "text") && !strstr(content_type, "javascript"))
			{
				char * dontcheck = "dontcheck";
				data->isCharsetDetermined = 1;
				stringInit(&data->charset, dontcheck);
				
			}
		}
		
		//get content-encoding from http header
		const char * content_encoding = httpHeaderGetStr2(&reply->header, HDR_CONTENT_ENCODING);
		if(content_encoding && !strcmp(content_encoding, "gzip"))
		{
			z_stream *strm = z_stream_pool_alloc();
			inflateInit2(strm, 47);
			data->strm = strm;
		}
		else
		{
			data->strm = NULL;
		}
   		size_t temp;
   		data->prevBuf = memAllocBuf(MAX_HX_CONTENT_LEN, &temp);
		data->prevBufLen = 0;
		
		//register into storeEntry
    		cc_register_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, data, free_sc_param, mod);	
	}
	return 0;
}
	
static int func_buf_recv_from_s_body(HttpStateData * httpState, char *buf, ssize_t len, int buffer_filled)
{
	StoreEntry *entry = httpState->entry;
	if(len == 0){
		return 0;
	}

	sc_param *data = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, mod);
	assert(data);

	char *bufInflate = NULL;
	size_t bufInflateLen = len;
	size_t allocLen = 0;

	if(data->strm)
	{
		bufInflateLen = len * 16;
		
		bufInflate = memAllocBuf(bufInflateLen, &allocLen);
		bufInflateLen = allocLen;
		
		data->strm->next_in = (Bytef *)buf;
		data->strm->avail_in = len;
		data->strm->next_out = (Bytef *)bufInflate;
		data->strm->avail_out = bufInflateLen;

		int err = inflate(data->strm, Z_NO_FLUSH);

		if(data->strm->avail_out < bufInflateLen)
		{
			bufInflateLen = bufInflateLen - data->strm->avail_out;
		}
		else
		{
			if(err != Z_OK && err != Z_STREAM_END)
			{
				debug(122, 1)("Unable to extract data, checking with NULL, %d in url[%s]\n",err, entry->mem_obj->url);
			}

			memFreeBuf(allocLen, bufInflate);

			return 0;
		}
	}
	else
	{
		bufInflateLen = len;
		
		bufInflate = memAllocBuf(bufInflateLen, &allocLen);
		bufInflateLen = allocLen;
		memcpy(bufInflate, buf, len);
	
	}

	//try to get charset from meta http-equiv
	if(!data->isCharsetDetermined && data->getCharsetRetries > 0)
	{

		assert(!strLen(data->charset));
		char * p;
		p = strstr(bufInflate, "meta");
		if(p)
		{
			p = strstr(p, "http-equiv");
		}

		if(p)
		{
			p = strstr(p, "Content-Type");
		}

		if(p)
		{
			p = strstr(p, "charset=");
		}

		if(p)
		{
			char *token = strtok(p + 8 , "\"' /<");
			if(token)
			{
//				data->charset = normalizeCharset(token);
				normalizeCharset(&data->charset, token);
				data->isCharsetDetermined = 1;

				//debug(122,3)("got charset in content [%s], buf=[%s%s], len=%d\n", entry->mem_obj->url, data->prevBuf?data->prevBuf:"" ,buf, len);
			}
		}

		data->getCharsetRetries --;

		if(data->getCharsetRetries == 0)
		{
			debug(122,3)("not get charset in content [%s]\n", entry->mem_obj->url);
		}
	}
	else
	{
		debug(122,3)("wont get charset in content retry=%d, determined=%d\n", data->getCharsetRetries, data->isCharsetDetermined);
	}
	char *non_hx = searchContent(data->prevBuf,data->prevBufLen,bufInflate,bufInflateLen,strBuf(data->charset),data->prevBuf, &data->prevBufLen);

	if(bufInflate != buf)
	{
		memFreeBuf(allocLen, bufInflate);
	}
	if(non_hx != NULL ){
		debug(122,1)("WARNING: Non-Hexie word %s appeared in %s\n ", non_hx, entry->mem_obj->url);
		memset((char *)buf, 0, len);
		createError(httpState, 0);
		return 1;
	}
	else{
		debug(122,4)("there is not existed the policy word: %s%s, len=%ld\n ",data->prevBuf?data->prevBuf:"" ,buf, (long int)len);
	}

	return 0;
}	

char charsets[CHARSET_COUNT] [MAX_CHARSET_LEN]= {"gb2312gbkgb18030","gb2312gbkgb18030","utf8","utf8","big5"};
typedef struct _keyword
{
	char content[MAX_HX_CONTENT_LEN];
	int length;
	char charset[MAX_CHARSET_LEN];
	struct _keyword *next;
	void * algo_specific;
} keyword;
static keyword * firstHxKey;
static int maxHxKeyLen;
static MemPool * keyword_pool = NULL;

static void * keyword_pool_alloc(void)
{
	void * obj = NULL;
	if (NULL == keyword_pool)
	{
		keyword_pool = memPoolCreate("mod_polityword_filter other_data keyword", sizeof(keyword));		
	}
	return obj = memPoolAlloc(keyword_pool);
}

void initHxKeyWords(const char *path)
{
	while(firstHxKey)
	{
		keyword *tobefreed = firstHxKey;
		firstHxKey = firstHxKey->next;
		sunday_freeSpecific(tobefreed->algo_specific);
		memPoolFree(keyword_pool, tobefreed);
		tobefreed = NULL;
	}

	assert(!access("/usr/local/squid/bin/gbtoall", X_OK));
	char *perlscript = "/usr/local/squid/bin/gbtoall %s";
	char getWordCommand[512];
	if(path)
	{
		snprintf(getWordCommand, 512,  perlscript, path);
	}
	else
	{
		snprintf(getWordCommand, 512, perlscript, "/usr/local/squid/etc/hxwords");
	}
	debug(122,1)("reading hxkeyword file with command %s\n", getWordCommand);
	FILE *allEncodingGetter = popen(getWordCommand, "r");
	assert(allEncodingGetter);
	int cur = 0;
	char content[MAX_HX_CONTENT_LEN];
	firstHxKey = NULL;
	maxHxKeyLen = 0;
	while(1)
	{
		if(!fgets(content, MAX_HX_CONTENT_LEN, allEncodingGetter))
			break;
		keyword *k = keyword_pool_alloc();
		int length = strlen(content);
		//remove \r\n
		while(content[length - 1] == '\r' || content[length - 1] == '\n')
		{
			content[length - 1] = '\0';
			length = length - 1;
		}
		if(firstHxKey && !strcmp(firstHxKey->content, content))
		{
			debug(122,1)("content of hxword has same simp and trad mode [%s]\n", content);
			k->length = 0;
		}
		else
		{
			strcpy(k->content, content);
			debug(122,1)("content of hxword is [%s]\n", k->content);
			k->length = length;
		}
		strcpy(k->charset, charsets[cur % CHARSET_COUNT]);
		k->algo_specific = sunday_getSpecific(k->content, k->length);
			
		k->next = firstHxKey;
		firstHxKey = k;
		if(k->length > maxHxKeyLen)
		{
			maxHxKeyLen = k->length;
		}
		cur ++;
	}
	int closeret = pclose(allEncodingGetter);
	if(closeret < 0)
	{
		debug(122, 1)(strerror(errno));
	}
}

char * searchContent(const char * prevBuf, const int prevBufLen, const char * buf, const int bufLen, const char *charset, char *lastPart, int *lastPartLen)
{
	keyword * k = firstHxKey;	

	char * hayStack;
	size_t hayStackLen;
	size_t dummy = 0;
	if(prevBufLen)
	{
		hayStackLen = prevBufLen + bufLen;
		hayStack = memAllocBuf(hayStackLen, &dummy);
		memcpy(hayStack, prevBuf, sizeof(char) * prevBufLen);
		memcpy(hayStack + prevBufLen, buf, sizeof(char) * bufLen);
	}
	else
	{
		hayStackLen = bufLen;
		hayStack = (char *)buf;
	}


	while(k)
	{
		if(charset != NULL && !strstr(k->charset, charset))
		{
			k = k->next;
			debug(122,3)("got charset, saved one comparation!\n");
			continue;
		}
		if(k->length == 0)
		{
			//This may be a trad mode same as simp mode
			k = k->next;
			continue;
		}
		int foundcount = memmem(hayStack, hayStackLen, k->content, k->length)?1:-1;
		
		if(foundcount > 0)
		{
			if(hayStack != buf)
			{
				memFreeBuf(hayStackLen, hayStack);
			}
			return k->content;
		}
		else
		{
		}
		k = k->next;
	}


	
	*lastPartLen = maxHxKeyLen < hayStackLen ? maxHxKeyLen : hayStackLen;
	memcpy(lastPart, hayStack + hayStackLen - *lastPartLen, *lastPartLen);
	if(hayStack != buf)
	{
		memFreeBuf(hayStackLen, hayStack);
	}


	return NULL;
}

static int func_sys_parse_param(char *cfg_line)
{
	mod_conf_param *data = NULL;
	char* token = NULL;

	if ((token = strtok(cfg_line, w_space)) != NULL)
	{
		if(strcmp(token, "mod_polityword_filter"))
			goto out;	
	}
	else
	{
		debug(122, 3)("(mod_polityword_filter) ->  parse line error\n");
		goto out;	
	}

	if (NULL == (token = strtok(NULL, w_space)))
	{
		debug(122, 3)("(mod_polityword_filter) ->  parse line data does not existed\n");
		goto out;
	}

	data = mod_config_pool_alloc();
	stringInit(&data->filter_path,token);
	debug(122, 2) ("(mod_polityword_filter) ->  filter_path=%s\n", strBuf(data->filter_path));

	cc_register_mod_param(mod, data, free_callback);
	initHxKeyWords(strBuf(data->filter_path));
	
	char * encodingSpec = "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=";
	//where +10 meas 2 "s and some other blanks
	int encodingSpecLen = strlen(encodingSpec) + MAX_CHARSET_LEN + 10;
	if(encodingSpecLen > maxHxKeyLen)
	{
		maxHxKeyLen = encodingSpecLen;
	}
	
	return 0;
	
out:
	if (data)
		free_callback(data);
	return -1;		
}


static int hook_cleanup(cc_module *module)
{
	debug(122, 1)("(mod_policyword_filter) ->     hook_cleanup:\n");
	if(NULL != sc_param_pool)
		memPoolDestroy(sc_param_pool);
	if(NULL != z_stream_pool)
		memPoolDestroy(z_stream_pool);
	if(NULL != mod_config_pool)
		memPoolDestroy(mod_config_pool);
	sunday_destroySpecificPool();
	return 0;
}


// module init 
int mod_register(cc_module *module)
{
	debug(122, 1)("(mod_policyword_filter) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	//module->hook_func_buf_recv_from_s_header = func_buf_recv_from_s_header;
	cc_register_hook_handler(HPIDX_hook_func_buf_recv_from_s_header,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_buf_recv_from_s_header),
			func_buf_recv_from_s_header);

	//module->hook_func_buf_recv_from_s_body = func_buf_recv_from_s_body;
	cc_register_hook_handler(HPIDX_hook_func_buf_recv_from_s_body,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_buf_recv_from_s_body),
			func_buf_recv_from_s_body);

	//module->hook_func_sys_parse_param = func_sys_parse_param;
	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);
	//module->hook_func_sys_cleanup = hook_cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			hook_cleanup);
	//mod = module;
	//mod = (cc_module*)cc_modules.items[module->slot];
	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif

/* Author: chenqi */
/* Date:   2013/04/11 */

#include "cc_framework_api.h"
#include "mod_client_compress.h"
#ifdef CC_FRAMEWORK
static cc_module* mod = NULL;

/* module parameter  */
typedef struct _mod_conf_param {
	int   comp_level;
	String header;
} mod_conf_param;

static int free_callback(void* param_data)
{
	mod_conf_param *param = (mod_conf_param*) param_data;
	if (param){
		stringClean(&param->header);
		safe_free(param);
	}
	return 0;
}

/* clientHttpRequest private data */
typedef struct _request_param {
	int http_version; //1 1.1 0 1.0
	int has_acceptencoding_header;  //0 means no 1 yes
	int has_contentencoding_header;  //0 means no 1 yes
    int whole_request;                  //0 means range Request, 1 means whole Request
	gz_stream  *gz;
} request_param;

static int free_request_param(void* data)
{
	request_param *req_param = (request_param *) data;
	if (req_param != NULL){
		if (req_param->gz) {
			debug(144, 9)("(mod_client_compress) -> destroy gz object\n");
			destroy(req_param->gz);
		}
		safe_free(req_param);
		req_param = NULL;
	}
	return 0;
}


/* parse module parameter */
// mod_client_compress 6 X-Accept-Encoding allow acl
static int func_sys_parse_param(char *cfg_line)
{
	mod_conf_param *data = NULL;
	char* token = NULL;

	if ((token = strtok(cfg_line, w_space)) != NULL) {
		if (strcmp(token, "mod_client_compress"))
			return -1;	
	} else {
		debug(144, 3)("mod_client_compress ->  parse line error\n");
		return -1;	
	}

	if (NULL == (token = strtok(NULL, w_space))) {
		debug(144, 1)("mod_client_compress ->  parse line comp_level data does not existed\n");
		return -1;
	}

	data = xcalloc(1, sizeof(mod_conf_param));
	data->comp_level = atoi(token);
		
	if (NULL == (token = strtok(NULL, w_space))) {
		debug(144, 1)("(mod_client_compress) ->  parse line header data does not existed\n");
		free_callback(data);
		return -1;
	}
	stringInit(&data->header, token);
	debug(144, 2)("mod_client_compress: comp_level=%d, header=%s\n", data->comp_level, strBuf(data->header));
	cc_register_mod_param(mod, data, free_callback);
	return 0;
}

/* compress, then send to client */
#define BUF_LEN (2*SQUID_TCP_SO_RCVBUF + 1)
static int gunzip_body_data(clientHttpRequest * http, MemBuf *mb, char *buf, ssize_t size) 
{
    /*if(0 == http->request->flags.range || NULL == http->request->range)
    {
        debug(144, 3)("(mod_client_compress) -> request->flags.range is %d request->range %p\n",http->request->flags.range, http->request->range);
        return 0;
        }*/
    mod_conf_param *cfg = cc_get_mod_param(http->conn->fd, mod);
	if (cfg == NULL) {
		debug(144, 3)("(mod_client_compress) -> mod conf parameter is null \n");
		return 0;
	}

	request_param* data = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if (data == NULL) {
		debug(144, 3)("(mod_client_compress) -> req_param is null \n");
		return 0;
	}

	if (!data->has_acceptencoding_header || data->has_contentencoding_header) {
		debug(144, 3)("(mod_client_compress) -> no have [%s] or have Content-Encoding\n", strBuf(cfg->header));
		return 0;
	}

	LOCAL_ARRAY(char, comp_buf, BUF_LEN);
	memset(comp_buf, 0, BUF_LEN);
	size_t comp_size = 0;
	size_t write_bytes = 0;
	buf = mb->buf;
	size = mb->size;
	// initialize gzip 
	if (NULL == data->gz) {
		char param[64] = {0};
		memset(param,0,64);
		snprintf(param, 64,"%c%c%d", 'w', 'b', cfg->comp_level);
		data->gz = gz_open(param);
		if (data->gz == NULL) {
			debug(144, 3)("(mod_client_compress) -> gz open failed and dont start compress \n");
			// gz_open failed 
			data->has_acceptencoding_header = 0;
			return 0;
		}
		// Mark Beginning
		snprintf(comp_buf,BUF_LEN,"%c%c%c%c%c%c%c%c%c%c",gz_magic[0],gz_magic[1],Z_DEFLATED,0,0,0,0,0,0,0x03);
		write_bytes = gz_write(data->gz, comp_buf + 10, buf, size, &comp_size);
		comp_size += 10;
	} else {
		write_bytes = gz_write(data->gz, comp_buf, buf, size, &comp_size);
	}

	assert(comp_size < BUF_LEN);
	if (write_bytes != size ) 
		debug(144,2)("mod_client_compress->compress error happened\n");

	if (comp_size > size)
		debug(144,2)("mod_client_compress: after compress content is bigger %d > %d \n", comp_size, size);

	if (1 == http->flags.done_copying) {
		// Mark Ending
		gz_close(data->gz, comp_buf, &comp_size);
	}
	memBufReset(mb);
	memBufAppend(mb, comp_buf, comp_size);
    debug(144,7)("mod_client_compress->compress success\n");
	return 0;
}


/* clientCheckHeaderDone  */
static int mod_reply_header(clientHttpRequest *http)
{
	mod_conf_param *cfg = cc_get_mod_param(http->conn->fd, mod);
	if (cfg == NULL){
		debug(144, 3)("(mod_client_compress) -> mod conf parameter is null \n");
		return 0;
	}

	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if (req_param == NULL) {
		debug(144, 3)("(mod_client_compress) -> clientHttpRequest private data is null \n");
		return 0;
	}

	if (0 == req_param->has_acceptencoding_header) {
		debug(144, 3)("mod_client_compress: request [%s] has no [%s] header \n",http->uri,strBuf(cfg->header));
		return 0;
	}

	HttpReply *rep = http->reply;
	HttpHeader *hdr = &rep->header;

	if (httpHeaderHas(hdr, HDR_CONTENT_ENCODING))
    {
		debug(144, 3) ("(mod_client_compress)->response got header [Content-Encoding], content data is comressed \n");
		req_param->has_contentencoding_header = 1;
		return 0;
	} else {
		req_param->has_contentencoding_header = 0;
		httpHeaderPutStr(hdr, HDR_CONTENT_ENCODING, "gzip");
	}

    if (req_param->http_version == 1)
    {
        if (req_param->has_acceptencoding_header)
        {
            /* Non-Chunked Response */
            if (!httpHeaderHas(hdr, HDR_TRANSFER_ENCODING))
            {
                debug(144, 5)("mod_client_compress: give chunked response to client\n");
                httpHeaderPutStr(hdr, HDR_TRANSFER_ENCODING, "Chunked");
            }
            http->request->flags.chunked_response = 1;
            rep->content_length = -1;
            httpHeaderDelById(hdr, HDR_CONTENT_LENGTH);
        }
    }
    if(req_param->whole_request && http->reply->sline.status == HTTP_PARTIAL_CONTENT)
    {
        http->reply->sline.status = HTTP_OK;
        httpHeaderDelById(hdr, HDR_CONTENT_RANGE);
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
            return 1;
    }    
	return 0;
}

static int check_XAccepe_EncodingAndRange(clientHttpRequest *http)
{
    debug(144, 9) ("mod_client_compress: client request http->uri = %s\n", http->uri);
    if(http->request->method != METHOD_GET && http->request->method != METHOD_POST && http->request->method != METHOD_HEAD) 
    {
        debug(144, 1)("mod_client_compress: client url is not GET/POST/HEAD request and return %s \n", http->uri);
        return 0;
    }

    mod_conf_param *cfg = NULL;
    if (NULL == (cfg = cc_get_mod_param(http->conn->fd, mod)))
    {
        debug(144, 5)("(mod_client_compress) -> mod conf parameter is null \n");
        return 0;
    }
    request_t* request = http->request;
    if (0 == httpHeaderCheckExistByName(&request->header, (char *)strBuf(cfg->header)))
        return 0;

    request_param *req_param = xcalloc(1,sizeof(request_param));
    cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, req_param, free_request_param, mod);
    req_param->has_acceptencoding_header = 1; 
    req_param->whole_request = 0;
    /* not sure http/1.0 can support this module */
    req_param->http_version = 0;
    if (request->http_ver.major > 1 || (request->http_ver.major == 1 && request->http_ver.minor >= 1))
    {
        debug(144,2)("mod_client_compress: client request' http is 1.1\n");
        req_param->http_version = 1;
    }
    HttpHeader *hdr = &request->header;
    if (NULL == httpHeaderFindEntry(hdr, HDR_RANGE) && NULL == httpHeaderFindEntry(hdr, HDR_REQUEST_RANGE))
    {
        httpHeaderAddEntry(hdr, httpHeaderEntryCreate(HDR_RANGE, "Range", "bytes=0-")); 
        req_param->whole_request = 1;
        http->request->flags.range = 1;
    }
    HttpHeaderEntry *e ;
    if((e = httpHeaderFindEntry(hdr, HDR_RANGE)) || (e = httpHeaderFindEntry(hdr, HDR_REQUEST_RANGE)))
    {
        debug(144,7)("mod_client_compress: Range is %s\n",strBuf(e->value));
        return 0;
    }
    debug(144,2)("mod_client_compress: NOT Find  Range\n");
    return 0;
}

static int is_disable_sendfile(store_client *sc, int fd)
{
	sc->is_zcopyable = 0;
	return 1;
}


// modele init 
int mod_register(cc_module *module)
{
	debug(144, 1)("(mod_client_compress) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_private_client_send_more_data,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_send_more_data),
			gunzip_body_data);

    cc_register_hook_handler(HPIDX_hook_func_private_add_http_orig_range,
            module->slot, 
            (void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_add_http_orig_range), 
            check_XAccepe_EncodingAndRange);

	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			mod_reply_header);
/*
	cc_register_hook_handler(HPIDX_hook_func_before_clientProcessRequest2,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_before_clientProcessRequest2),
			mod_http_req_process);
*/
	cc_register_hook_handler(HPIDX_hook_func_client_set_zcopyable,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_set_zcopyable),
			is_disable_sendfile);

	if (reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif


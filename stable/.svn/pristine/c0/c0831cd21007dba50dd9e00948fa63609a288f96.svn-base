/* this file has been adjusted by chenqi @2013 */
/* updated by chenqi @ 2013/04/01 */
/* More Rubust */
#include "cc_framework_api.h"
#include <zlib.h>

#ifdef CC_FRAMEWORK

#define TRYFREE(p) {if (p) xfree(p);}
static int Z_BUFSIZE = 8096;
static cc_module* mod = NULL;
static int const gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

/* gunzip data */
typedef struct _gz_stream {
        z_stream stream;
        int      z_err;   /* error code for last stream operation */
        int      z_eof;   /* set if end of input file */
        FILE     *file;   /* .gz file */
        Byte     *inbuf;  /* input buffer */
        Byte     *outbuf; /* output buffer */
        uLong    crc;     /* crc32 of uncompressed data */
        char     *msg;    /* error message */
        char     *path;   /* path name for debugging only */
        int      transparent; /* 1 if input file is not a .gz file */
        char     mode;    /* 'w' or 'r' */
        z_off_t  start;   /* start of compressed data in file (header skipped) */
        z_off_t  in;      /* bytes into deflate or inflate */
        z_off_t  out;     /* bytes out of deflate or inflate */
        int      back;    /* one character push-back */
        int      last;    /* true if push-back is last character */
} gz_stream;

static int destroy(gz_stream *s)
{
    int err = Z_OK;

    if (!s) return Z_STREAM_ERROR;

    if (s->stream.state != NULL) {
        if (s->mode == 'w') {
            err = deflateEnd(&(s->stream));
        } else if (s->mode == 'r') {
            err = inflateEnd(&(s->stream));
        }
    }
    if (s->z_err < 0) err = s->z_err;
   if(s){
	  TRYFREE(s->inbuf);
	  TRYFREE(s->outbuf);
  	  TRYFREE(s);
	}	
    return err;
}


/* module parameter  */
typedef struct _mod_conf_param {
	int   min_length;
	int   comp_level;
	int   uncompress_type; //1 means server 0 means client default 0
} mod_conf_param;

static int free_callback(void* param_data)
{
	mod_conf_param *param = (mod_conf_param*) param_data;
	if (param){
		safe_free(param);
	}
	return 0;
}

/* clientHttpRequest private data */
typedef struct _request_param {
	int http_version; //1 1.1 0 1.0
	int has_acceptencoding_header;  //0 means no 1 yes
	int has_contentencoding_header;  //0 means no 1 yes
	z_stream  *gunzip_strm;
} request_param;

static int free_request_param(void* data)
{
	request_param *req_param = (request_param *) data;
	if (req_param != NULL){
		if (req_param->gunzip_strm) {
			inflateEnd(req_param->gunzip_strm);
			safe_free(req_param->gunzip_strm);
		}
		safe_free(req_param);
		req_param = NULL;
	}
	return 0;
}

/* StoreEntry private data */
typedef struct _store_param {
	long remain_clen;
} store_param;

static int free_store_param(void* data) 
{
	store_param *param = (store_param *) data;
	if (param != NULL) {
		param->remain_clen = -1;
		safe_free(param);
	}
	return 0;
}

/* FwdState private data */
typedef struct _sc_param {
	gz_stream *gz;
	int active_compress_flag;
} sc_param;

static int free_sc_param(void* data)
{
	sc_param *param = (sc_param *) data;
	if(param != NULL){
		if(param->gz) {
			debug(143, 9)("(mod_active_compress) -> destroy gz object\n");
			destroy(param->gz);
		}
		safe_free(param);
	}
	return 0;
}

/* parse module parameter */
static int func_sys_parse_param(char *cfg_line)
{
	mod_conf_param *data = NULL;
	char* token = NULL;

	if ((token = strtok(cfg_line, w_space)) != NULL) {
		if(strcmp(token, "mod_active_compress"))
			return -1;	
	} else {
		debug(143, 3)("(mod_active_compress) ->  parse line error\n");
		return -1;	
	}

	if (NULL == (token = strtok(NULL, w_space))) {
		debug(143, 1)("(mod_active_compress) ->  parse line min_length data does not existed\n");
		return -1;
	}

	data = xmalloc(sizeof(mod_conf_param));
	memset(data,0,sizeof(mod_conf_param));
	data->min_length = atoi(token);

		
	if (NULL == (token = strtok(NULL, w_space))) {
		debug(143, 1)("(mod_active_compress) ->  parse line comp_level data does not existed\n");
		free_callback(data);
		return -1;
	}
	
	data->comp_level = atoi(token);

	if (NULL == (token = strtok(NULL, w_space))) {
		debug(143, 1)("(mod_active_compress) ->  parse line comp_level data does not existed\n");
		free_callback(data);
		return -1;
	}

	if (strcmp(token,"server") == 0 ){
		data->uncompress_type = 1;
	}
	else
		data->uncompress_type = 0;

	debug(143, 2) ("(mod_active_compress) ->  comp_level=%d \n", data->comp_level);
	debug(143, 2) ("(mod_active_compress) ->  min_length=%d \n", data->min_length);
	debug(143, 2) ("(mod_active_compress) ->  uncpmpress_type=%d \n", data->uncompress_type);
	cc_register_mod_param(mod, data, free_callback);
	return 0;
}


/* gnu-zip API */
static int do_flush(gz_stream *file, int flush, char *trans_buf, size_t *new_len2)
{
    uInt len;
    int done = 0;
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

    s->stream.avail_in = 0; /* should be zero already anyway */

    for (;;) {

        len = Z_BUFSIZE - s->stream.avail_out;
	debug(143,9)("do_flush : len:%d s->stream.next_out: %p s->outbuf %p\n", len, s->stream.next_out, s->outbuf);

        if (len != 0) {
		
	    debug(143,9)("do_flush : avail out has some data :%d %p\n", len, s->outbuf);
            memcpy(trans_buf+*new_len2, s->outbuf, len);
			*new_len2 += len;
            s->stream.next_out = s->outbuf;
            s->stream.avail_out = Z_BUFSIZE;
        }
        if (done) break;
        s->out += s->stream.avail_out;
        s->z_err = deflate(&(s->stream), flush);
        s->out -= s->stream.avail_out;

	debug(143,9)("do_flush after deflate s->stream.avail_out %d\n", s->stream.avail_out);

        /* Ignore the second of two consecutive flushes: */
        if (len == 0 && s->z_err == Z_BUF_ERROR) s->z_err = Z_OK;

        /* deflate has finished flushing only when it hasn't used up
         * all the available space in the output buffer:
         */
        done = (s->stream.avail_out != 0 || s->z_err == Z_STREAM_END);

        if (s->z_err != Z_OK && s->z_err != Z_STREAM_END) break;
    }
    return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
}

static int gz_close(gz_stream *file, char *trans_buf, size_t *new_len)
{
	gz_stream *s = (gz_stream*)file;
	uLong ss = 0;
	if (s == NULL) return Z_STREAM_ERROR;
	if (s->mode == 'w') {
		if (do_flush (file, Z_FINISH, trans_buf, new_len) != Z_OK) {
			debug(143,9)("gz_close : do_flush is error %d\n",  s->z_err);
			return -1;
		}
		
		memcpy(trans_buf+*new_len, (char *)&s->crc, 4);
		
		*new_len += 4;
		ss = (uLong)(s->in & 0xffffffff);
		memcpy(trans_buf+*new_len, (char *)&ss, 4);
		*new_len += 4;
	}
	return 0;
}

static int  gz_write(gz_stream *file, char *dst_buf, const char *buf, size_t len, size_t *new_len)
{
	gz_stream *s = (gz_stream*)file;
	int i = 0;

	if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

	s->stream.next_in = (Bytef*)buf;
	s->stream.avail_in = len;

	while (s->stream.avail_in != 0) {

		if (s->stream.avail_out == 0) {

			debug(143,9)("gz_write : avail out is zero and used s->outbuf\n");
			s->stream.next_out = s->outbuf;
			/*if (fwrite(s->outbuf, 1, Z_BUFSIZE, s->file) != Z_BUFSIZE) {
				s->z_err = Z_ERRNO;
				break;
			}*/
			s->stream.avail_out = Z_BUFSIZE;
		}

		s->in += s->stream.avail_in;
		s->out += s->stream.avail_out;
		s->z_err = deflate(&(s->stream), Z_SYNC_FLUSH);
		s->in -= s->stream.avail_in;
		s->out -= s->stream.avail_out;
		if(s->stream.avail_out < Z_BUFSIZE) {
			debug(143,9)("gz_write compress bytes %d, deflate avail_out length %d\n", s->stream.avail_out, Z_BUFSIZE - s->stream.avail_out);
		//	if(fwrite(s->outbuf, 1, Z_BUFSIZE - s->stream.avail_out, s->file) != Z_BUFSIZE - s->stream.avail_out)
		//		printf("error\n");
			memcpy(dst_buf + i, s->outbuf, Z_BUFSIZE - s->stream.avail_out);
			i += Z_BUFSIZE - s->stream.avail_out;

			s->stream.next_out = s->outbuf;
			s->stream.avail_out = Z_BUFSIZE;
		}
		if (s->z_err != Z_OK) break;
	}
	s->crc = crc32(s->crc, (const Bytef *)buf, len);

	*new_len = i;
	return (int)(len - s->stream.avail_in);
}

static gz_stream *gz_open(char *flag)
{
	int err;
	int level = Z_DEFAULT_COMPRESSION; /* compression level */
	int strategy = Z_DEFAULT_STRATEGY; /* compression strategy */
	char mode[20] = {"wb6"};
	if(flag) {
		memset(mode, 0, 20);
		snprintf(mode, 20, "%s", flag);
	}
	char *p = mode;
	gz_stream *s;
	char fmode[80]; /* copy of mode, without the compression level */
	char *m = fmode;

	s = (gz_stream *)xmalloc(sizeof(gz_stream));
	if (!s) return Z_NULL;

	s->stream.zalloc = (alloc_func)0;
	s->stream.zfree = (free_func)0;
	s->stream.opaque = (voidpf)0;
	s->stream.next_in = s->inbuf = Z_NULL;
	s->stream.next_out = s->outbuf = Z_NULL;
	s->stream.avail_in = s->stream.avail_out = 0;
	s->file = NULL;
	s->z_err = Z_OK;
	s->z_eof = 0;
	s->in = 0;
	s->out = 0;
	s->back = EOF;
	s->crc = crc32(0L, Z_NULL, 0);
	s->msg = NULL;
	s->transparent = 0;
	s->mode = '\0';
	do {
		if (*p == 'r') s->mode = 'r';
		if (*p == 'w' || *p == 'a') s->mode = 'w';
		if (*p >= '0' && *p <= '9') {
			level = *p - '0';
		} else if (*p == 'f') {
			strategy = Z_FILTERED;
		} else if (*p == 'h') {
			strategy = Z_HUFFMAN_ONLY;
		} else if (*p == 'R') {
			strategy = Z_RLE;
		} else {
			*m++ = *p; /* copy the mode */
		}
	} while (*p++ && m != fmode + sizeof(fmode));
	if (s->mode == '\0'){
		 safe_free(s);
		 return Z_NULL;
	}

	debug(143, 9)("(mod_active_compress) -> gz_open buffer size = %ld \n",(long int)Z_BUFSIZE);

	if (s->mode == 'w') {
#ifdef NO_GZCOMPRESS
		err = Z_STREAM_ERROR;
#else
		err = deflateInit2(&(s->stream), level,
				Z_DEFLATED, -MAX_WBITS, 8, strategy);
		/* windowBits is passed < 0 to suppress zlib header */
		s->stream.next_out = s->outbuf = (Byte*)xmalloc(Z_BUFSIZE);
#endif
		if (err != Z_OK || s->outbuf == Z_NULL) {
			safe_free(s->outbuf);
			safe_free(s);
			return Z_NULL;
		}
	} else {
		s->stream.next_in  = s->inbuf = (Byte*)xmalloc(Z_BUFSIZE);

		err = inflateInit2(&(s->stream), -MAX_WBITS);
		if (err != Z_OK || s->inbuf == Z_NULL) {
			safe_free(s->inbuf);
                        safe_free(s);
			return Z_NULL;
		}
	}
	s->stream.avail_out = Z_BUFSIZE;

	errno = 0;
	if (s->mode == 'w') {
//		sprintf(head_buf, "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
//				Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, Z_DEFAULT_STRATEGY);
		s->start = 10L;
	}
	else {
	//	check_header(s);
	//	s->start = 10L;
	}

	return s;
}
	
/* uncompress */
static int gunzip_body_data(clientHttpRequest * http, MemBuf *mb, char *buf, ssize_t size) 
{
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);

	if (req_param == NULL) {
		debug(143, 3)("(mod_active_compress) -> req_param is null \n");
		return 0;
	}

	mod_conf_param *cfg = cc_get_mod_param(http->conn->fd, mod);
	if (cfg == NULL) {
		debug(143, 3)("(mod_active_compress) -> mod conf parameter is null \n");
		return 0;
	}

	if ((cfg->uncompress_type == 0 &&
			!req_param->has_acceptencoding_header && 
			req_param->has_contentencoding_header == 1 && 
			req_param->http_version == 1) || 
			cfg->uncompress_type == 1  )
	{ 
		debug(143,5)("mod_active_compress: uncompress content before send to client");
		if (req_param->gunzip_strm == NULL) {
			req_param->gunzip_strm = xcalloc(1, sizeof(z_stream));
			inflateInit2(req_param->gunzip_strm, 47);
		}
		z_stream* strm = req_param->gunzip_strm;
		char *bufInflate = NULL;
		size_t bufInflateLen = size;
		size_t allocLen = 0;
		bufInflateLen = size * 16;

		bufInflate = memAllocBuf(bufInflateLen, &allocLen);
		bufInflateLen = allocLen;

		strm->next_in = (Bytef *)buf;
		strm->avail_in = size;
		strm->next_out = (Bytef *)bufInflate;
		strm->avail_out = bufInflateLen;

		int err = inflate(strm, Z_NO_FLUSH);

		if (strm->avail_out < bufInflateLen) {
			bufInflateLen = bufInflateLen - strm->avail_out;
		} else {
			if (err != Z_OK && err != Z_STREAM_END) {
				debug(143, 1)("Unable to extract data, checking with NULL, in url\n");
			}
		}

		debug(143, 3)("extract data for client, size =  %d \n",bufInflateLen);
		memBufReset(mb);
		memBufAppend(mb, bufInflate, bufInflateLen);
		memFreeBuf(allocLen, bufInflate);
		return 1;
	}
	else
		debug(143,5)("mod_active_compress: do not need to uncompress content before send to client\n");
	return 0;
}

/* compress body */
static int func_buf_recv_from_s_body(HttpStateData * httpState, char *buf, ssize_t len, char* comp_buf, ssize_t comp_buf_len, ssize_t *comp_size, int buffer_filled)
{
    if (len <= 0 || comp_buf == NULL || buf == NULL) {
        debug(143, 0)("(mod_active_compress) -> func_buf_recv_from_s_body len <=0 or buf == NULL \n");
        return 0;
    }

    mod_conf_param *cfg = cc_get_mod_param(httpState->fd, mod);
    if(cfg == NULL){
        debug(143, 0)("(mod_active_compress) -> mod conf parameter is null \n");
        return len;
    }

    sc_param *data = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, mod);
    if(data == NULL) {
        debug(143, 3)("(mod_active_compress) -> sc_param is null \n");
		return len;
    }

    store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, httpState->entry, mod);
    if(str_param == NULL) {
        debug(143, 3)("(mod_active_compress) -> str_param is null \n");
		return len;
    }

    debug(143, 3)("(mod_active_compress) -> data->gz[%p] data->active_compress_flag[%d]\n",data->gz,data->active_compress_flag);


    if (str_param->remain_clen == -37)	// gzopen failed
        return len;
	else if (str_param->remain_clen >=0 )
		str_param->remain_clen -= len;

    debug(143, 5)("(mod_active_compress) -> consume %zd\t remain %ld\n",len,str_param->remain_clen);

    memset(comp_buf, 0, comp_buf_len);
    *comp_size = 0;
    int write_bytes = 0;

    if (data->gz) {
        debug(143,9)("request->active_compress_flag: %d\n", data->active_compress_flag);
        if (data->active_compress_flag == 0) {
            snprintf(comp_buf,comp_buf_len,"%c%c%c%c%c%c%c%c%c%c",gz_magic[0],gz_magic[1],Z_DEFLATED,0,0,0,0,0,0,0x03);
            write_bytes = gz_write(data->gz, comp_buf+10, buf, len, comp_size);
            data->active_compress_flag++;
            *comp_size += 10;
        }
        else 
			write_bytes = gz_write(data->gz, comp_buf, buf, len, comp_size);

        assert(*comp_size <= comp_buf_len);

        if (write_bytes != len ) 
            debug(143,0)("mod_active_compress->compress error happened\n");

        if (*comp_size > len) 
            debug(143,3)("mod_active_compress: warning after comress length %d > %d before compress \n",*comp_size,len );

        if (httpState->entry->mem_obj->reply->content_length > 0 && str_param->remain_clen <= 0) {
            gz_close(data->gz, comp_buf, comp_size);
            httpState->eof = 1;
        }
    }

    debug(143, 3)("(mod_active_compress) -> after compress length httpReadReply buffer %d \n",*comp_size);
    debug(143, 3)("(mod_active_compress) -> after compress write_bytes %d \n",write_bytes);
    return 1;
}


/* chunked response */
static int func_buf_end_recv_from_s_body(HttpStateData * httpState, char *buf, ssize_t len )
{
	debug(143,5)("mod_active_compress: buf[%s\n]  len[%d]\n",buf,len);
	sc_param *data = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, mod);
	if (data) {
		data->gz = NULL;
		httpState->eof = 1;
	}
	return 0;
}


/* http version */
static int enable_http_version(clientHttpRequest *http)
{
	if (!http->conn->port->http11) {
		debug(143, 5) ("mod_active_compress: set http versiont to http 1.1\n");
		http->conn->port->http11 = 1;
	}
	return 0;
}

static int disable_http_version(clientHttpRequest *http)
{

	if(http->conn->port->http11) {
		debug(143, 5) ("mod_active_compress set http versiont to http 1.0\n");
		http->conn->port->http11 = 0;
	}
	http->reply->content_length = -1;
	http->entry->mem_obj->reply->content_length= -1;     
	debug(142, 5)("(mod_active_compress) ->  change http->reply->content_length to :%d\n", (int)(http->reply->content_length));
	return 0;
}


/* clientCheckHeaderDone  */
static int mod_reply_header(clientHttpRequest *http)
{
	request_param* req_param = cc_get_mod_private_data(REQUEST_PRIVATE_DATA, http, mod);
	if (req_param == NULL) {
		debug(143, 3)("(mod_active_compress) -> clientHttpRequest private data is null \n");
		return 0;
	}

	mod_conf_param *cfg = cc_get_mod_param(http->conn->fd, mod);
	if (cfg == NULL){
		debug(143, 3)("(mod_active_compress) -> mod conf parameter is null \n");
		return 0;
	}

	HttpReply *rep = http->reply;
	HttpHeader *hdr = &rep->header;

	enable_http_version(http);

	if (req_param->http_version == 1) {
		if (httpHeaderHas(&http->reply->header, HDR_CONTENT_ENCODING)){
			debug(143, 3) ("(mod_active_compress)->response got header [Content-Encoding] and content data is comressed \n");
			req_param->has_contentencoding_header = 1;
		} else {
			req_param->has_contentencoding_header = 0;
		}


		if (cfg->uncompress_type == 1) {
			return 0;
		}

		debug(143,5)("(mod_active_compress)->cfg->uncompress_type[%d],req_param->has_acceptencoding_header[%d]\n",cfg->uncompress_type,req_param->has_acceptencoding_header);


		if ((cfg->uncompress_type == 0 && req_param->has_acceptencoding_header) 
				|| cfg->uncompress_type == 1 ) {
            int  has_transfer_encoding = httpHeaderHas(&http->reply->header, HDR_TRANSFER_ENCODING);
			if (has_transfer_encoding && rep->content_length < 0 ) {
				// Chunked Response
				debug(143, 5)("(mod_active_compress) -> give chunked response to client\n");
				http->request->flags.chunked_response = 1;
				rep->content_length = -1;
				httpHeaderDelById(hdr, HDR_CONTENT_LENGTH);
			} else {
				// Non-Chunked Response
				if (http->flags.hit) {		// HIT
					// Recalculate Content-Length
					httpHeaderDelById(hdr, HDR_CONTENT_LENGTH);
					rep->content_length = contentLen(http->entry);

					if (rep->content_length >= 0){
						httpHeaderPutSize(hdr, HDR_CONTENT_LENGTH, rep->content_length);
					} else {
						debug(143, 5)("(mod_active_compress) -> HIT give chunked response to client\n");
                        httpHeaderPutStr(hdr, HDR_TRANSFER_ENCODING, "Chunked");
						http->request->flags.chunked_response = 1;
						rep->content_length = -1;
					}
				} else { // MISS
					debug(143, 5)("(mod_active_compress) -> MISS give chunked response to client\n");
					httpHeaderPutStr(hdr, HDR_TRANSFER_ENCODING, "Chunked");
					http->request->flags.chunked_response = 1;
					rep->content_length = -1;
					httpHeaderDelById(hdr, HDR_CONTENT_LENGTH);
				}
			}
		}
	}
	  return 0; 
}

static int mod_http_req_process(clientHttpRequest *http)
{

	debug(143, 9) ("mod_active_compress: client request http->uri = %s\n", http->uri);

	if (http->request->method != METHOD_GET 
			&& http->request->method != METHOD_POST 
			&& http->request->method != METHOD_HEAD) {
		debug(143, 1)("mod_active_compress: client url is not GET/POST request and return %s \n", http->uri);
		return 0;
	}

	if (http->request->flags.range == 1) {
		debug(143, 1)("mod_active_compress: client url is range request and return %s \n", http->uri);
		return 0;
	}

	request_param *req_param = xmalloc(sizeof(request_param));
	memset(req_param,0,sizeof(request_param));
	cc_register_mod_private_data(REQUEST_PRIVATE_DATA, http, req_param, free_request_param, mod);

	request_t* request = http->request;
	if (request->http_ver.major > 1 
			|| (request->http_ver.major == 1 && request->http_ver.minor >= 1)) {
		req_param->http_version = 1;
        int accept_encoding = httpHeaderHas(&request->header, HDR_ACCEPT_ENCODING);
		if (accept_encoding ){
			req_param->has_acceptencoding_header = 1;
			return 0;
		} else {
			req_param->has_acceptencoding_header = 0;
		}

	} else {
		debug(143,2)("mod_active_compress: client request' http is 1.0 or http_port has not http11 of supporting http 1.1 conf \n");
		req_param->http_version = 0;
		return 0;
	}
	return 0;
}

static void transfer_done_precondition(clientHttpRequest *http, int *precondition)
{
	*precondition = (http->request->flags.chunked_response == 0);
}

static int is_disable_sendfile(store_client *sc, int fd)
{
	sc->is_zcopyable = 0;
	return 1;
}

static int valid_content_length(StoreEntry *entry)
{

	store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, entry, mod);
	if (str_param && str_param->remain_clen > 0 ){
		debug(143, 1) ("mod_active_compress has remain %ld data to has not download and abort this object \n",str_param->remain_clen );
		return 1;
	}
	return 0;	
}

static int private_repl_read_start(int fd, HttpStateData *httpState)
{
	httpState->entry->mem_obj->reply->content_length = -1;
	return 0;
}


/* as we compress response body, but not add Content-Encoding to entry, it may cause many trouble */
/* may be we could add Content-Encoding: gzip to disk, in case next read from StoreEntry */
static int func_is_active_compress(int fd, HttpStateData *httpState, size_t hdr_size, size_t* done)
{
    mod_conf_param *cfg = cc_get_mod_param(httpState->fd, mod);
    if(cfg == NULL){
        debug(143, 0)("(mod_active_compress) -> mod conf parameter is null \n");
        return 0;
    }

    int need_to_compress = 1; 
    HttpReply *rep = httpState->entry->mem_obj->reply;
    HttpHeader *hdr = &rep->header;

    if (!httpHeaderHas(&httpState->request->header, HDR_ACCEPT_ENCODING)) {
        debug(143,5)("mod_actvice_compress: dont need to compress\n");
        need_to_compress = 0;
    }

    if (httpHeaderHas(hdr, HDR_CONTENT_ENCODING))
		need_to_compress = 0;

    if(cfg->uncompress_type ==1 )
		need_to_compress = 1;

    if(need_to_compress == 0){
        debug(143, 3)("(mod_active_compress) -> content type is not checking is fail and dont compress content data \n");
        return 0;
    }

	long content_length = -1;
	if (httpHeaderHas(hdr, HDR_CONTENT_LENGTH)) {
		content_length = rep->content_length; 
		if (cfg->min_length > content_length) {
			debug(143,3)("mod_active_compress: content length too small to compress! \n");
			return 0;
		}
	}
	/* chunked response not support persistent connection, so dont compress it */
	else if (httpState->flags.chunked){
		debug(143,3)("mod_active_compress: do not compress chunked-response\n");
		return 0;
	}
	else {
		debug(143,3)("mod_active_compress: no Content-Length header\n");
		return 0;
	}

	// ok, this response should  be compressed...
    sc_param *data = cc_get_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, mod);
    if(data == NULL) {
        debug(143, 3)("(mod_active_compress) -> sc_param is null \n");
        data = xcalloc(1, sizeof(sc_param));
        cc_register_mod_private_data(FWDSTATE_PRIVATE_DATA, httpState->fwd, data, free_sc_param, mod);
    }

    store_param *str_param = cc_get_mod_private_data(STORE_ENTRY_PRIVATE_DATA, httpState->entry, mod);
    if(str_param == NULL) {
        debug(143, 3)("(mod_active_compress) -> str_param is null \n");
        str_param = xmalloc(sizeof(store_param));
		str_param->remain_clen = content_length;
        cc_register_mod_private_data(STORE_ENTRY_PRIVATE_DATA, httpState->entry, str_param, free_store_param, mod);
    }

	// initialize gzip 
    if (!data->gz && data->active_compress_flag == 0) {
        char param[64] = {0};
        memset(param,0,64);
        snprintf(param, 64,"%c%c%d", 'w', 'b', cfg->comp_level);
        data->gz = gz_open(param);
        if (data->gz == NULL) {
            debug(143, 3)("(mod_active_compress) -> gz open failed and dont start compress \n");
			str_param->remain_clen = -37;	// as a mark that gzopen failed
            return 0;
        }
    }

	char ce_hdr[] = "Content-Encoding: gzip\r\n\r\n";
	MemBuf *mb = &httpState->reply_hdr;
	MemBuf tmp;
	memBufDefInit(&tmp);
	memBufAppend(&tmp, mb->buf, hdr_size - 2);
	memBufAppend(&tmp, ce_hdr, sizeof(ce_hdr));
	storeAppend(httpState->entry, tmp.buf, tmp.size - 1);
	debug(143, 9)("mod_active_compress: NEW HTTP REPLY HDR:\n---------\n%s\n----------\n", tmp.buf);
	/* Re-Parse reply header, add Content-Encoding */
	httpReplyParse(rep, tmp.buf, tmp.size - 1);
	memBufClean(&tmp);
	return 1;
}

// modele init 
int mod_register(cc_module *module)
{
	debug(143, 1)("(mod_active_compress) ->  init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_private_compress_response_body,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_compress_response_body),
			func_buf_recv_from_s_body);

	cc_register_hook_handler(HPIDX_hook_func_private_end_compress_response_body,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_end_compress_response_body),
			func_buf_end_recv_from_s_body);

	cc_register_hook_handler(HPIDX_hook_func_http_repl_read_end,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_read_end), 
			private_repl_read_start);

	cc_register_hook_handler(HPIDX_hook_func_sys_parse_param,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_parse_param),
			func_sys_parse_param);

	cc_register_hook_handler(HPIDX_hook_func_private_client_send_more_data,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_client_send_more_data),
			gunzip_body_data);

	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_start,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_start),
			mod_reply_header);

	cc_register_hook_handler(HPIDX_hook_func_http_req_process,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_req_process),
			mod_http_req_process);

	cc_register_hook_handler(HPIDX_hook_func_client_set_zcopyable,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_client_set_zcopyable),
			is_disable_sendfile);

	cc_register_hook_handler(HPIDX_hook_func_transfer_done_precondition,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_transfer_done_precondition),
			transfer_done_precondition);

	cc_register_hook_handler(HPIDX_hook_func_private_store_entry_valid_length,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_store_entry_valid_length),
			valid_content_length);

	cc_register_hook_handler(HPIDX_hook_func_http_repl_send_end,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_send_end),
			disable_http_version);

	// new hook handler added by chenqi
	cc_register_hook_handler(HPIDX_hook_func_is_active_compress,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_is_active_compress),
			func_is_active_compress);

	if (reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;
	return 0;
}

#endif


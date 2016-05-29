#ifdef CC_FRAMEWORK
#ifndef CLIENT_COMPRESS
#define CLIENT_COMPRESS

#include <zlib.h>
#define TRYFREE(p) {if (p) xfree(p);}
static int Z_BUFSIZE = 8096;
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

int destroy(gz_stream *s)
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

int do_flush(gz_stream *file, int flush, char *trans_buf, size_t *new_len2)
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
		 *          * all the available space in the output buffer:
		 *                   */
		done = (s->stream.avail_out != 0 || s->z_err == Z_STREAM_END);

		if (s->z_err != Z_OK && s->z_err != Z_STREAM_END) break;
	}
	return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
}

int gz_close(gz_stream *file, char *trans_buf, size_t *new_len)
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

int  gz_write(gz_stream *file, char *dst_buf, const char *buf, size_t len, size_t *new_len)
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

gz_stream *gz_open(char *flag)
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

	debug(143, 9)("(mod_range_compress) -> gz_open buffer size = %ld \n",(long int)Z_BUFSIZE);

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
		s->start = 10L;
	}

	return s;
}

#endif
#endif

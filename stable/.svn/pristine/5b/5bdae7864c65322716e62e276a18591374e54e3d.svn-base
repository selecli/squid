#include "preloader.h"

static struct http_header_st *http_build_header_2(const char *start, const char *end);

static void http_header_destroy(struct http_header_st *hdr)
{
    safe_process(free, hdr->name);
    safe_process(free, hdr->value);
    safe_process(free, hdr);
}

static void http_headers_destroy(struct array_st *hdrs)
{
    array_clean(hdrs);
}

static void http_request_destroy(struct http_request_st *request)
{
    safe_process(free, request->uri);
    safe_process(free, request->proto);
    http_headers_destroy(&request->header);
    safe_process(data_buf_destroy, request->body.dbuf);
    safe_process(free, request);
}

static void http_reply_destroy(struct http_reply_st *reply)
{
    http_headers_destroy(&reply->header);
    safe_process(data_buf_destroy, reply->body.dbuf);
    safe_process(free, reply);
}

void http_destroy(struct http_st *http)
{
    http_request_destroy(http->request);
    http_reply_destroy(http->reply);
    safe_process(free, http);
}

static struct http_request_st *http_request_create(void)
{
    struct http_request_st *request;

    request = xcalloc(1, g_tsize.http_request_st);
    request->body.dbuf = data_buf_create(SIZE_64KB);
    request->body.length = 0;

    return request;
}

static struct http_reply_st *http_reply_create(void)
{
    struct http_reply_st *reply;

    reply = xcalloc(1, g_tsize.http_reply_st);
    reply->body.dbuf = data_buf_create(SIZE_64KB);
    reply->body.length = 0;

    return reply;
}

struct http_st *http_create(void)
{
    struct http_st *http;

    http = xcalloc(1, g_tsize.http_st);
    http->request = http_request_create();
    http->reply = http_reply_create();

    return http;
}

int http_check_request_method(const int method)
{
    switch (method)
    {
        case HTTP_METHOD_POST:
        return 0;
        /* other */
        default:
        return -1;
    }

    return -1;
}

static int http_method_convert(const char *method)
{
    if (0 == strcmp(method, "GET"))
    {
        return HTTP_METHOD_GET;
    }

    if (0 == strcmp(method, "POST"))
    {
        return HTTP_METHOD_POST;
    }

    /* unsupport method. */

    return HTTP_METHOD_NOT_SUPPORT;
}

static const char *http_method_convert2(const int method)
{
    switch (method)
    {
        case HTTP_METHOD_GET:
        return "GET";
        case HTTP_METHOD_POST:
        return "POST";
        default:
        return "GET";
    }

    return "GET";
}

static const char *http_status_convert(const int status)
{
    switch (status)
    {
        case HTTP_STATUS_OK:
        return "OK";

        case HTTP_STATUS_BAD_REQUEST:
        return "Bad Request";

        case HTTP_STATUS_FORBIDDEN:
        return "Forbidden";

        case HTTP_STATUS_NOT_ALLOWED:
        return "Method Not Allowed";

        case HTTP_STATUS_PROCESS_TIMEOUT:
        return "Timeout";

        case HTTP_STATUS_INTERNAL_SERVER_ERROR:
        return "Internal Server Error";

        default:
        return "Unknown";
    }
}

void http_parse_request_line(struct http_request_st *request, struct data_buf_st *dbuf)
{
    int found_flag;
    int major, minor;
    size_t line_len;
    char * method;
    const char * start, *end;
    const char * line_end;
    const char * line_end_ptr;

    /* HTTP request line format: Method URI HTTP/x.x */

    if (0 == dbuf->offset)
    {
        /* No data, return. */
        return;
    }

    /* HTTP request line should be end with "\n" or "\r\n". */
    line_end_ptr = memchr(dbuf->buf, '\n', dbuf->offset);
    if (NULL == line_end_ptr)
    {
        if (dbuf->offset >= g_global.cfg.http.max_request_header_size)
        {
            request->parsed.error = 1;
            request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
            return;
        }
        /* Not found the HTTP request line boundary charactors,
        keep recving data for next parsing. */
        return;
    }
    line_len = line_end_ptr - dbuf->buf + 1;
    if (line_len >= g_global.cfg.http.max_request_header_size)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    line_end = line_end_ptr - 1;
    if (line_end < dbuf->buf)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    if ('\r' == *line_end)
    {
        line_end--;
        if (line_end < dbuf->buf)
        {
            request->parsed.error = 1;
            request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
            return;
        }
    }

    /* HTTP request method */
    for (start = dbuf->buf; start <= line_end && xisspace(*start); ++start) ;
    if (start > line_end)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    for (end = start; end <= line_end && (0 == xisspace(*end)); ++end) ;
    if (end > line_end)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    if (0 == xisspace(*end))
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    method = xstrndup(start, end - start);
    request->method = http_method_convert(method);
    safe_process(free, method);
    if (http_check_request_method(request->method) < 0)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }

    /* HTTP request URI */
    for (start = end; start <= line_end && xisspace(*start); ++start) ;
    if (start > line_end)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    for (end = start; end <= line_end && (0 == xisspace(*end)); ++end) ;
    if (end > line_end)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    if (0 == xisspace(*end))
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    request->uri = xstrndup(start, end - start);

    /* HTTP request version: HTTP/major.minor, eg: HTTP/1.1 */
    for (start = end; start <= line_end && xisspace(*start); ++start) ;
    if (start > line_end)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    if (start + 4 > line_end)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    if (strncasecmp(start, "HTTP/", 5))
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }

    /* HTTP protocol Major */
    major = 0;
    found_flag = 0;

    for (start += 5; start <= line_end && isdigit(*start) && major < 65535; ++start)
    {
        major *= 10;
        major += (*start) - '0';
        found_flag = 1;
    }
    if (start > line_end
        || start + 1 > line_end
        ||'.' != *start
        || 0 == found_flag
        || major >= 65535)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }

    /* HTTP protocol Minor */
    minor = 0;
    found_flag = 0;

    for (start += 1; start <= line_end && isdigit(*start) && minor < 65535; ++start)
    {
        minor *= 10;
        minor += *start - '0';
        found_flag = 1;
    }
    if (0 == found_flag || minor > 65535)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    request->major = major;
    request->minor = minor;

    line_end_ptr += 1;
    dbuf->offset -= line_end_ptr - dbuf->buf;
    memmove(dbuf->buf, line_end_ptr, dbuf->offset);
    request->parsed.line = 1;
    }

static const char *http_find_header_end(struct data_buf_st *dbuf)
{
    char *pos;
    int count = 0;

    for (pos = dbuf->buf; pos < dbuf->buf + dbuf->offset; ++pos)
    {
        switch (*pos)
        {
            case '\r':
            break;
            case '\n':
            count++;
            if (2 == count)
            {
                return pos;
            }
            break;
            default:
            count = 0;
            break;
        }
    }

    return NULL;
}

static const char *http_find_header_end_2(const char *hdr_start, const char *hdr_end)
{
    const char *pos;

    for (pos = hdr_start; pos <= hdr_end; ++pos)
    {
        switch (*pos)
        {
            case '\r':
            return pos - 1;
            case '\n':
            return pos - 1;
            default:
            break;
        }
    }

    return NULL;
}

static void http_parse_header(struct array_st *hdrs, struct data_buf_st *dbuf, struct http_parsed_st *parsed)
{
    int req_hdr_len;
    const char *pos;
    const char *req_hdr_end;
    const char *hdr_start, *hdr_end;
    struct http_header_st *hdr;

    if (0 == dbuf->offset)
    {
        /* No data, return. */
        return;
    }

    req_hdr_end = http_find_header_end(dbuf);
    if (NULL == req_hdr_end)
    {
        if (dbuf->offset > g_global.cfg.http.max_request_header_size)
        {
            parsed->error = 1;
            parsed->error_type = HTTP_ERROR_PARSE_HEADER;
            return;
        }
        return;
    }

    req_hdr_len = req_hdr_end - dbuf->buf + 1;

    if (req_hdr_len > g_global.cfg.http.max_request_header_size)
    {
        parsed->error = 1;
        parsed->error_type = HTTP_ERROR_PARSE_HEADER;
        return;
    }

    hdr_start = dbuf->buf;

    for (pos = dbuf->buf; pos < req_hdr_end; ++pos)
    {
        if ('\n' == *pos)
        {
            hdr_end = http_find_header_end_2(hdr_start, pos);
            if (hdr_end - hdr_start + 1 < 3)
            {
                hdr_start = pos + 1;
                continue;
            }
            hdr = http_build_header_2(hdr_start, hdr_end);
            if (NULL == hdr)
            {
                parsed->error = 1;
                parsed->error_type = HTTP_ERROR_PARSE_HEADER;
                http_headers_destroy(hdrs);
                return;
            }
            http_add_header(hdrs, hdr);
            hdr_start = pos + 1;
            continue;
        }
    }

    req_hdr_end += 1;
    dbuf->offset -= req_hdr_end - dbuf->buf;
    memmove(dbuf->buf, req_hdr_end, dbuf->offset);
    parsed->header = 1;
    }

static void http_parse_request_header(struct http_request_st *request, struct data_buf_st *dbuf)
{
    http_parse_header(&request->header, dbuf, &request->parsed);
}

static void http_parse_request_body(struct http_request_st *request, struct data_buf_st *dbuf)
{
    if (request->content_length <= 0)
    {
        request->body.length = 0;
        request->parsed.body = 1; 
        return;
    }

    if (dbuf->offset >= g_global.cfg.http.max_request_body_size)
    {
        request->parsed.error = 1;
        request->parsed.error_type = HTTP_ERROR_PARSE_BODY;
        return;
    }

    if (dbuf->offset >= request->content_length)
    {
        request->body.length = request->content_length;
        data_buf_destroy(request->body.dbuf);
        request->body.dbuf = data_buf_create(request->content_length);
        data_buf_append(request->body.dbuf, dbuf->buf, request->content_length);
        request->parsed.body = 1;
        return;
    }
}

static struct http_header_st *http_header_create(const char *hdr_name, const char *hdr_value)
{
    struct http_header_st *hdr;

    hdr        = xmalloc(g_tsize.http_header_st);
    hdr->name  = (NULL == hdr_name) ? NULL : xstrdup(hdr_name);
    hdr->value = (NULL == hdr_value) ? NULL : xstrdup(hdr_value);

    return hdr;
}

struct http_header_st *http_build_header(const char * hdr_name, const char * hdr_value)
{
    return http_header_create(hdr_name, hdr_value);
}

static struct http_header_st *http_build_header_2(const char *start, const char *end)
{
    const char *t;
    const char *delim;
    struct http_header_st *hdr;

    while (start <= end && xisspace(*start) && ++start) ;
    if (start > end)
    {
        return NULL;
    }
    delim = memchr(start, ':', end - start + 1);
    if (NULL == delim || delim == start)
    {
        return NULL;
    }
    for (t = delim - 1; t >= start && xisspace(*t); --t) ;
    if (t < start)
    {
        return NULL;
    }
    hdr = http_header_create(NULL, NULL);
    hdr->name = xstrndup(start, t - start + 1);
    for (delim++; delim <= end && xisspace(*delim); ++delim) ;
    if (delim > end)
    {
        http_header_destroy(hdr);
        return NULL;
    }
    for (t = end; t > delim && xisspace(*t); --t) ;
    if (t < delim)
    {
        http_header_destroy(hdr);
        return NULL;
    }
    hdr->value = xstrndup(delim, t - delim + 1);

    return hdr;
}

void http_add_header(struct array_st *hdrs, struct http_header_st *hdr)
{
    array_append(hdrs, hdr, (FREE *)http_header_destroy);
}

struct http_header_st *http_find_header(struct array_st *hdrs, const char *hdr_name)
{
    int i;
    struct http_header_st *hdr;

    for (i = 0; i < hdrs->count; ++i)
    {
        hdr = ARRAY_ITEM_DATA(hdrs, i);
        if (0 == strcmp(hdr->name, hdr_name))
        {
            return hdr;
        }
    }

    return NULL;
}

ssize_t http_get_content_length(struct array_st *hdrs)
{
    ssize_t content_length;
    struct http_header_st *hdr;

    hdr = http_find_header(hdrs, "Content-Length");
    if (NULL == hdr)
    {
        return -1;
    }
    content_length = strtol(hdr->value, NULL, 10);

    return content_length;
}

void http_build_request_line(struct http_request_st *request, int method, char *uri, char *proto, int major, int minor)
{
    request->method = method;
    request->uri = xstrdup(uri);
    request->proto = xstrdup(proto);
    request->major = major,
    request->minor = minor;
}

void http_build_body(struct http_body_st *body, struct data_buf_st *dbuf)
{
    if (NULL == dbuf)
    {
        body->length = 0;
        body->dbuf = NULL;
        return;
    }
    body->dbuf->offset = 0;
    body->length = dbuf->offset;
    data_buf_append(body->dbuf, dbuf->buf, dbuf->offset);
}

static void http_pack_request_line(struct data_buf_st *dbuf, int method, char *uri, char *proto, int major, int minor)
{
    size_t len;
    char buf[SIZE_4KB];

    len = snprintf(buf, SIZE_4KB,
                   "%s %s %s/%d.%d\r\n",
                   http_method_convert2(method),
                   uri,
                   proto, major, minor);
    data_buf_append(dbuf, buf, len);
}

static void http_pack_header(struct data_buf_st *dbuf, struct array_st *hdrs)
{
    int i;
    char buf[SIZE_4KB];
    struct http_header_st *hdr;

    for (i = 0; i < hdrs->count; ++i)
    {
        hdr = ARRAY_ITEM_DATA(hdrs, i);
        if (NULL == hdr)
        {
            continue;
        }
        snprintf(buf, SIZE_4KB, "%s: %s\r\n", hdr->name, hdr->value);
        data_buf_append(dbuf, buf, strlen(buf));
    }
    data_buf_append(dbuf, "\r\n", 2);
}

static void http_pack_body(struct data_buf_st *dbuf, struct http_body_st *body)
{
    if (body->length > 0 && NULL != body->dbuf)
    {
        data_buf_append(dbuf, body->dbuf->buf, body->dbuf->offset);
    }
}

void http_pack_request(struct data_buf_st *dbuf, struct http_request_st *request)
{
    http_pack_request_line(dbuf,
                           request->method, request->uri,
                           request->proto, request->major, request->minor);
    http_pack_header(dbuf, &request->header);
    http_pack_body(dbuf, &request->body);
    request->packed = 1;
}

void http_build_reply_status_line(struct http_reply_st *reply, int status, int major, int minor)
{
    reply->status = status;
    reply->major = major,
    reply->minor = minor;
}

void http_build_reply_header(struct http_reply_st *reply)
{
    char hdr_value[SIZE_256];
    struct http_header_st *hdr;

    current_gmttime(hdr_value, SIZE_256);
    hdr = http_build_header("Date", hdr_value); 
    http_add_header(&reply->header, hdr);

    server_name(hdr_value, SIZE_256);
    hdr = http_build_header("Server", hdr_value);
    http_add_header(&reply->header, hdr);

    snprintf(hdr_value, SIZE_256, "%ld", (long)reply->body.length);
    hdr = http_build_header("Content-Length", hdr_value);
    http_add_header(&reply->header, hdr);

    hdr = http_build_header("X-CC-Preload-Reply", "ChinaCache");
    http_add_header(&reply->header, hdr);

    hdr = http_build_header("Connection", "close");
    http_add_header(&reply->header, hdr);
}

void http_build_reply(struct http_reply_st *reply, int status, int major, int minor, struct data_buf_st *body_dbuf)
{
    http_build_reply_status_line(reply, status, major, minor);
    http_build_body(&reply->body, body_dbuf);
    http_build_reply_header(reply);
}

static void http_pack_reply_line(struct data_buf_st *dbuf, int status, int major, int minor)
{
    int len;
    char buf[SIZE_4KB];

    len = snprintf(buf, SIZE_4KB,
                   "HTTP/%d.%d %03d %s\r\n",
                   major, minor,
                   status, http_status_convert(status));
    data_buf_append(dbuf, buf, len);
}

void http_pack_reply(struct data_buf_st *dbuf, struct http_reply_st *reply)
{
    http_pack_reply_line(dbuf, reply->status, reply->major, reply->minor);
    http_pack_header(dbuf, &reply->header);
    http_pack_body(dbuf, &reply->body);
    reply->packed = 1;
}

static int http_check_request_header_content_length(struct http_request_st *request)
{
    switch (request->method)
    {
        case HTTP_METHOD_PUT:
        case HTTP_METHOD_POST:
        return (request->content_length < 0 ? -1 : 0);
        case HTTP_METHOD_GET:
        case HTTP_METHOD_HEAD:
        return (request->content_length > 0 ? -1 : 0);
        default:
        return 0;
    }
}

static int http_check_request_header(struct http_request_st *request)
{
    if (http_check_request_header_content_length(request) < 0)
    {
        return -1;
    }

    /* FIXME: process Transfer-Encoding: chunked
    if (NULL != http_find_header(&request->header, "Transfer-Encoding"))
    {
        return -1;
    }
    */

    return 0;
}

void http_parse_request(struct http_request_st *request, struct data_buf_st *dbuf)
{
    if (0 == dbuf->offset)
    {
        return;
    }

    if (0 == request->parsed.line)
    {
        http_parse_request_line(request, dbuf);
        if (request->parsed.error)
        {
            LogError(1, "HTTP parsed request line error.");
            return;
        }
        if (0 == request->parsed.line)
        {
            return;
        }
    }

    if (0 == request->parsed.header)
    {
        http_parse_request_header(request, dbuf);
        if (request->parsed.error)
        {
            LogError(1, "HTTP parsed request header error.");
            return;
        }
        request->content_length = http_get_content_length(&request->header);
        if (http_check_request_header(request) < 0)
        {
            request->parsed.error = 1;
            LogError(1, "HTTP parsed request header error.");
            return;
        }
        if (0 == request->parsed.header)
        {
            return;
        }
    }

    if (0 == request->parsed.body)
    {
        http_parse_request_body(request, dbuf);
        if (request->parsed.error)
        {
            LogError(1, "HTTP parsed request body error.");
            return;
        }
        if (0 == request->parsed.body)
        {
            return;
        }
    }
request->parsed.done = 1;
}

void http_parse_reply_line(struct http_reply_st *reply, struct data_buf_st *dbuf)
{
    int found_flag;
    int major, minor;
    size_t line_len;
    char * status;
    const char * start, *end;
    const char * line_end;
    const char * line_end_ptr;

    /* HTTP reply line format: HTTP/x.x Status Desc */

    if (0 == dbuf->offset)
    {
        /* No data, return. */
        return;
    }

    /* HTTP reply line should be end with "\n" or "\r\n". */
    line_end_ptr = memchr(dbuf->buf, '\n', dbuf->offset);
    if (NULL == line_end_ptr)
    {
        if (dbuf->offset >= g_global.cfg.http.max_request_header_size)
        {
            reply->parsed.error = 1;
            reply->parsed.error_type = HTTP_ERROR_PARSE_LINE;
            return;
        }
        /* Not found the HTTP reply line boundary charactors,
        keep recving data for next parsing. */
        return;
    }

    line_len = line_end_ptr - dbuf->buf + 1;
    if (line_len >= g_global.cfg.http.max_request_header_size)
    {
        reply->parsed.error = 1;
        reply->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }

    line_end = line_end_ptr - 1;
    if (line_end < dbuf->buf)
    {
        reply->parsed.error = 1;
        reply->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    if ('\r' == *line_end)
    {
        line_end--;
    }
if (line_end < dbuf->buf)
{
    reply->parsed.error = 1;
    reply->parsed.error_type = HTTP_ERROR_PARSE_LINE;
    return;
}

    /* HTTP reply version: HTTP/major.minor, eg: HTTP/1.1 */
    for (start = dbuf->buf; start <= line_end && xisspace(*start); ++start) ;
    if (start > line_end)
    {
        reply->parsed.error = 1;
        reply->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    if (start + 4 > line_end)
    {
        reply->parsed.error = 1;
        reply->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    if (strncasecmp(start, "HTTP/", 5))
    {
        reply->parsed.error = 1;
        reply->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }

    /* HTTP protocol Major */
    major = 0;
    found_flag = 0;

    for (start += 5; start <= line_end && isdigit(*start) && major < 65535; ++start)
    {
        major *= 10;
        major += (*start) - '0';
        found_flag = 1;
    }
    if (start > line_end
        || start + 1 > line_end
        ||'.' != *start
        || 0 == found_flag
        || major >= 65535)
    {
        reply->parsed.error = 1;
        reply->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }

    /* HTTP protocol Minor */
    minor = 0;
    found_flag = 0;

    for (start += 1; start <= line_end && isdigit(*start) && minor < 65535; ++start)
    {
        minor *= 10;
        minor += *start - '0';
        found_flag = 1;
    }
    if (0 == found_flag || minor > 65535)
    {
        reply->parsed.error = 1;
        reply->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    reply->major = major;
    reply->minor = minor;

    /* HTTP reply status */
    for ( ; start <= line_end && xisspace(*start); ++start) ;
    if (start > line_end)
    {
        reply->parsed.error = 1;
        reply->parsed.error_type = HTTP_ERROR_PARSE_LINE;
        return;
    }
    for (end = start; end <= line_end && (0 == xisspace(*end)); ++end) ;
    status = xstrndup(start, end - start);
    reply->status = strtol(status, NULL, 10);
    safe_process(free, status);

    line_end_ptr += 1;
    dbuf->offset -= line_end_ptr - dbuf->buf;
    memmove(dbuf->buf, line_end_ptr, dbuf->offset);
    reply->parsed.line = 1;
    }

static void http_parse_reply_header(struct http_reply_st *reply, struct data_buf_st *dbuf)
{
    http_parse_header(&reply->header, dbuf, &reply->parsed);
}

static void http_parse_reply_body(struct http_reply_st *reply, struct data_buf_st *dbuf)
{
    size_t copy_len;

    /* FIXME: Content-Length, Transfer-Encoding: chunked */

    if (reply->content_length <= 0)
    {
        reply->parsed.body = 1; 
        reply->body.length = 0;
        return;
    }

    if (reply->body.length + dbuf->offset < reply->content_length)
    {
        copy_len = dbuf->offset;
    }
    else
    {
        reply->parsed.body = 1;
        copy_len = reply->content_length - reply->body.length;
    }

    data_buf_append(reply->body.dbuf, dbuf->buf, copy_len);
    reply->body.length += copy_len;
}

void http_parse_reply(struct http_reply_st *reply, struct data_buf_st *dbuf)
{
    if (0 == dbuf->offset)
    {
        return;
    }
    if (0 == reply->parsed.line)
    {
        http_parse_reply_line(reply, dbuf);
        if (reply->parsed.error)
        {
            LogError(1, "HTTP parsed reply line error.");
            return;
        }
        if (0 == reply->parsed.line)
        {
            return;
        }
    }
    if (0 == reply->parsed.header)
    {
        http_parse_reply_header(reply, dbuf);
        if (reply->parsed.error)
        {
            LogError(1, "HTTP parsed reply header error.");
            return;
        }
        reply->content_length = http_get_content_length(&reply->header);
        if (0 == reply->parsed.header)
        {
            return;
        }
    }
    if (0 == reply->parsed.body)
    {
        http_parse_reply_body(reply, dbuf);
        if (reply->parsed.error)
        {
            LogError(1, "HTTP parsed reply body error.");
            return;
        }
        if (0 == reply->parsed.body)
        {
            return;
        }
    }
    reply->parsed.done = 1;
}

/*
 cn->request: build, pack
 cn->reply: process


void http_read_request()
{

}

void http_read_reply(void *data)
{
    struct comm_connect_st *cn;

    cn  = data;
    fd  = cn->fd;
    rev = &g_commtable[fd].rev;


    for ( ; ; )
    {
        n = comm_recv(fd, rev->dbuf.buf + rev->dbuf.offset, rev->dbuf.size - rev->dbuf.offset);
        if (rev->error)
        {
            comm_close(fd);
            return;
        }
        if (rev->eof)
        {
            comm_close(fd);
            return;
        }
        if (n < 0 && 0 == rev->ready)
        {
            return;
        }
        rev->dbuf.offset += n;
        http_parse_reply(http->reply, &rev->dbuf);
        ret = cn->process(cn);
        //ret = preload_process_reply(task, http->reply);
        printf("process ret: %d", ret);
        if (-1 == ret || 0 == ret)
        {
            comm_set_io_event(fd, IO_EVENT_READ, NULL, NULL);
            comm_close(fd);
            return;
        }
        if (1 == ret)
        {
            if (rev->dbuf.offset == rev->dbuf.size)
            {
                rev->dbuf.size += SIZE_64KB;
                rev->dbuf.buf = xrealloc(rev->dbuf.buf, rev->dbuf.size);
            }
            continue;
        }
        data_buf_reset(&rev->dbuf);
    }
}

void http_send_request(void *data)
{
    struct comm_connect_st *cn;

    cn  = data;
    fd  = cn->fd;
    wev = &g_commtable[fd].wev;

    if (0 == cn->http->request->built)
    {
        preload_build_request(cn->http->request, cn->task);
        http_pack_request(&wev->dbuf, cn->http->request);
        printf("@Send Request ====> \n%s\n", wev->dbuf.buf);
    }

    for ( ; ; )
    {
        n = comm_send(cn->fd, wev->dbuf.buf, wev->dbuf.offset);
        if (wev->error)
        {
            comm_close(cn->fd);
            return;
        }
        if (n <= 0 && 0 == wev->ready)
        {
            return;
        }
        wev->dbuf.offset -= n;
        if (0 == wev->dbuf.offset)
        {
            comm_set_io_event(cn->fd, IO_EVENT_WRITE, NULL, NULL);
            if (preload_check_reply_readable(cn->task) < 0)
            {
                comm_close(cn->fd);
                return;
            }
            comm_set_io_event(cn->fd, IO_EVENT_READ, preload_read_reply, cn);
            return;
        }
        memmove(wev->dbuf.buf, wev->dbuf.buf + n, wev->dbuf.offset);
    }
}

void http_send_reply()
{

}
*/

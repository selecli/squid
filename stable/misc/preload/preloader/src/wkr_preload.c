#include "preloader.h"

static void digest_init(struct digest_st *digest)
{
    switch (digest->type)
    {
        case CHECK_TYPE_BASIC:
            break;
        case CHECK_TYPE_MD5:
            digest->context = xcalloc(1, g_tsize.md5_context_st);
            md5_init(digest->context);
            break;
        case CHECK_TYPE_SHA1:
            break;
        default:
            break;
    }
}

static const char *digest_convert(int type)
{
    const char *digest;

    switch (type)
    {
        case CHECK_TYPE_BASIC:
            digest = "BASIC";
            break;
        case CHECK_TYPE_MD5:
            digest = "MD5";
            break;
        case CHECK_TYPE_SHA1:
            digest = "SHA1";
            break;
        default:
            digest = "UNKNOWN";
            break;
    }

    return digest;
}

static void task_report_retry_reset(struct task_report_st *report)
{
    report->status = 0;
    report->start  = 0;
    report->done   = 0;
    report->error  = 0;
}

static struct data_buf_st *preload_build_refresh_request_body_data(struct task_st *task)
{
    int len;
    char buf[SIZE_4KB];
    char sessionid[SIZE_256];
    struct timeval tv;
    struct data_buf_st *dbuf;

    dbuf = data_buf_create(SIZE_8KB);

    len = snprintf(buf, SIZE_4KB, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    data_buf_append(dbuf, buf, len);

    gettimeofday(&tv, NULL);
    snprintf(sessionid, SIZE_256, "PRF%ld%06ld", tv.tv_sec, tv.tv_usec);
    len = snprintf(buf, SIZE_4KB, "<method name=\"url_purge\" sessionid=\"%s\">\n", sessionid);
    data_buf_append(dbuf, buf, len);

    len = snprintf(buf, SIZE_4KB, "<url_list>\n");
    data_buf_append(dbuf, buf, len);

    len = snprintf(buf, SIZE_4KB, "<url id=\"0\">%s</url>\n", task->task_url->url);
    data_buf_append(dbuf, buf, len);

    len = snprintf(buf, SIZE_4KB, "</url_list>\n");
    data_buf_append(dbuf, buf, len);

    len = snprintf(buf, SIZE_4KB, "</method>\n");
    data_buf_append(dbuf, buf, len);

    return dbuf;
}

static struct data_buf_st *preload_build_report_request_body_data(struct task_st *task)
{
    int len;
    char buf[SIZE_4KB];
    struct data_buf_st *dbuf;

    dbuf = data_buf_create(SIZE_8KB);

    len = sprintf(buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<preload_report sessionid=\"%s\">\n",
                  task->sessionid);
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<url_id>%s</url_id>\n",
                  task->task_url->id);
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<refresh_status>%s</refresh_status>\n",
                  task->refresh.status ? "success" : "failed");
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<preload_status>%03d</preload_status>\n",
                  task->preload.status);
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<download_mean_rate>%.3lf</download_mean_rate>\n",
                  task->preload.limit_rate.mean_rate);
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<check_result type=\"%s\">%s</check_result>\n",
                  digest_convert(task->preload.digest.type),
                  task->preload.digest.digest ? (char *)task->preload.digest.digest : "-");
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<cache_status>%s</cache_status>\n",
                  task->preload.cache_status ? task->preload.cache_status : "-");
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<http_status_code>%s</http_status_code>\n",
                  task->preload.http_status ? task->preload.http_status : "-");
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<date>%s</date>\n",
                  task->preload.date ? task->preload.date : "-");
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<last_modified>%s</last_modified>\n",
                  task->preload.last_modified ? task->preload.last_modified : "-");
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf,
                  "<content_length>%s</content_length>\n",
                  task->preload.content_length ? task->preload.content_length : "-");
    data_buf_append(dbuf, buf, len);

    len = sprintf(buf, "</preload_report>\n"); 
    data_buf_append(dbuf, buf, len);

    return dbuf;
}

static void preload_build_refresh_request(struct http_request_st *request, struct task_st *task)
{
    char body_len[SIZE_256];
    struct data_buf_st *dbuf;
    struct http_header_st *hdr;

    dbuf = preload_build_refresh_request_body_data(task);
    http_build_body(&request->body, dbuf);
    data_buf_destroy(dbuf);
    snprintf(body_len, SIZE_256, "%ld", (long)request->body.length);
    http_build_request_line(request, HTTP_METHOD_POST, "/", "http", 1, 1);
    hdr = http_build_header("Host", "www.refresh.com");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("Accept", "*/*");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("User-Agent", "ChinaCache");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("X-CC-Preload-Refresh", "ChinaCache");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("Content-Length", body_len);
    http_add_header(&request->header, hdr);
    hdr = http_build_header("Connection", "close");
    http_add_header(&request->header, hdr);
}

static void preload_build_preload_request(struct http_request_st *request, struct task_st *task)
{
    struct http_header_st *hdr;

    http_build_request_line(request,
                            HTTP_METHOD_GET,
                            task->task_url->uri,
                            "HTTP", 1, 1);
    hdr = http_build_header("Host", task->task_url->host);
    http_add_header(&request->header, hdr);
    hdr = http_build_header("Accept", "*/*");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("User-Agent", "ChinaCache");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("X-CC-Preload", "ChinaCache");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("Connection", "close");
    http_add_header(&request->header, hdr);
}

static void preload_build_report_request(struct http_request_st *request, struct task_st *task)
{
    char body_len[SIZE_256];
    struct data_buf_st *dbuf;
    struct http_header_st *hdr;

    dbuf = preload_build_report_request_body_data(task);
    http_build_body(&request->body, dbuf);
    data_buf_destroy(dbuf);
    snprintf(body_len, SIZE_256, "%ld", (long)request->body.length);
    http_build_request_line(request,
                            HTTP_METHOD_POST,
                            "/content/preload_report",
                            "HTTP", 1, 1);
    hdr = http_build_header("Host", "www.report.com");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("Accept", "*/*");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("User-Agent", "ChinaCache");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("X-CC-Preload-Report", "ChinaCache");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("Content-Length", body_len);
    http_add_header(&request->header, hdr);
    hdr = http_build_header("Connection", "close");
    http_add_header(&request->header, hdr);
}

static void preload_build_request(struct http_request_st *request, struct task_st *task)
{
    switch (task->state)
    {
        case TASK_STATE_REFRESH:
            preload_build_refresh_request(request, task);
            break;
        case TASK_STATE_PRELOAD:
            preload_build_preload_request(request, task);
            break;
        case TASK_STATE_REPORT:
            preload_build_report_request(request, task);
            break;
        default:
            break;
    }
    request->built = 1;
}

static void preload_switch_task_state(struct task_st *task)
{
    switch (task->state)
    {
        case TASK_STATE_READY:
        switch (task->action)
        {
            case ACTION_REFRESH:
            task->state = TASK_STATE_REFRESH;
            return;
            case ACTION_PRELOAD:
            task->state = TASK_STATE_PRELOAD;
            return;
            case ACTION_REFRESH_PRELOAD:
            task->state = TASK_STATE_REFRESH;
            return;
            default:
            task->state = TASK_STATE_ERROR;
            return;
        }
        break;
        case TASK_STATE_REFRESH:
        switch (task->action)
        {
            case ACTION_REFRESH:
            task->state = TASK_STATE_REPORT;
            return;
            case ACTION_REFRESH_PRELOAD:
            task->state = TASK_STATE_PRELOAD;
            return;
            default:
            task->state = TASK_STATE_ERROR;
            return;
        }
        break;
        case TASK_STATE_PRELOAD:
        switch (task->action)
        {
            case ACTION_PRELOAD:
            task->state = TASK_STATE_REPORT;
            return;
            case ACTION_REFRESH_PRELOAD:
            task->state = TASK_STATE_REPORT;
            return;
            default:
            task->state = TASK_STATE_ERROR;
            return;
        }
        return;
        case TASK_STATE_REPORT:
        task->state = TASK_STATE_DONE;
        return;
        case TASK_STATE_ERROR:
        task->state = TASK_STATE_DONE;
        return;
        default:
            task->state = TASK_STATE_ERROR;
            return;
    }
}

static void preload_process_error(struct task_st *task)
{
    switch (task->state)
    {
        case TASK_STATE_REFRESH:
            task->refresh.error = 1;
            task->refresh.status = 0;
            return;
        case TASK_STATE_PRELOAD:
            task->preload.error = 1;
            task->preload.status = PRELOAD_STATUS_ERROR;
            return;
        case TASK_STATE_REPORT:
            task->report.error = 1; 
            task->report.status = 0;
            return;
        default:
            LogAbort("no this task state.");
            return;
    }
}

static void preload_process_eof(struct task_st *task)
{
    preload_process_error(task);
}

static void preload_process_done(struct task_st *task)
{
    switch (task->state)
    {
        case TASK_STATE_REFRESH:
            task->refresh.done = 1;
            return;
        case TASK_STATE_PRELOAD:
            task->preload.done = 1;
            return;
        case TASK_STATE_REPORT:
            task->report.done = 1; 
            return;
        default:
            LogAbort("no this task state.");
            return;
    }
}

static int preload_process_refresh_reply_body(struct task_refresh_st *refresh, struct http_body_st *body)
{
    char *p;
    struct data_buf_st *dbuf;

    dbuf = body->dbuf;

    if (0 == dbuf->offset)
    {
        return -1;
    }

    p = memmem(dbuf->buf, dbuf->offset, "<url_ret", 8);
    if (NULL == p)
    {
        return -1;
    }

    p += 8;

    p = memchr(p, '>', dbuf->offset - (p - dbuf->buf));
    if (NULL == p)
    {
        return -1;
    }

    if (p + 3 >= (dbuf->buf + dbuf->offset))
    {
        return -1;
    }

    if (0 != memcmp(p + 1, "200", 3))
    {
        return -1;
    }

    return 0;
}

static int preload_process_refresh_reply(struct task_refresh_st *refresh, struct http_reply_st *reply)
{
    /* return value:
     * -1: error
     *  0: data done
     *  1: data more
     */

    if (reply->parsed.error)
    {
        refresh->status = 0;
        return -1;
    }

    if (reply->parsed.done)
    {
        if (preload_process_refresh_reply_body(refresh, &reply->body) < 0)
        {
            refresh->status = 0;
        }
        else
        {
            refresh->status = 1;
        }
        //refresh->done = 1;
        return 0;
    }

    if (reply->parsed.header && reply->body.length > refresh->max_reply_body_size)
    {
        refresh->status = 0;
        return -1;
    }

    return 1;
}

static void preload_process_preload_reply_body_digest(struct task_preload_st *preload, struct data_buf_st *dbuf)
{
    unsigned char buf[SIZE_16];
    unsigned char digest[SIZE_32 + 1];

    switch (preload->digest.type)
    {
        case CHECK_TYPE_BASIC:
            /* do nothing here, already done when processing reply header. */
            return;
        case CHECK_TYPE_MD5:
            md5_update(preload->digest.context, (unsigned char *)dbuf->buf, dbuf->offset);
            if (1 == preload->done)
            {
                md5_final(buf, preload->digest.context);
                snprintf((char *)digest,
                        SIZE_32 + 1,
                        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                        buf[0], buf[1], buf[2], buf[3], buf[4],
                        buf[5], buf[6], buf[7], buf[8], buf[9],
                        buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
                preload->digest.digest = (unsigned char *)xstrdup((char *)digest);
            }
            return;
        case CHECK_TYPE_SHA1:
            /* unspport now, do nothing. */
            return;
        default:
            /* do nothing. */
            return;
    }
}

static const char *m3u8_find_line_end(const char *start, const char *end)
{
    const char *pos;

    for (pos = start; pos <= end; ++pos)
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

static void preload_process_preload_reply_body_m3u8(struct task_preload_st *preload, struct data_buf_st *dbuf)
{
    char *filename;
    const char *pos;
    const char *p_start;
    const char *p_end;
    char url[SIZE_4KB];
    struct data_buf_st *new_dbuf;

    if (0 == preload->m3u8.enable)
    {
        return;
    }

    new_dbuf = data_buf_clone(&preload->m3u8.dbuf);
    data_buf_append(new_dbuf, dbuf->buf, dbuf->offset);
    data_buf_reset(&preload->m3u8.dbuf);
    p_start = new_dbuf->buf;

    for (pos = new_dbuf->buf; pos < new_dbuf->buf + new_dbuf->offset; ++pos)
    {
        if ('\n' == *pos)
        {
            p_end = m3u8_find_line_end(p_start, pos);
            if (p_end - p_start + 1 < 3)
            {
                p_start = pos + 1;
                continue;
            }
            filename = xstrndup(p_start, p_end - p_start + 1);
            snprintf(url, SIZE_4KB, "%s/%s", preload->m3u8.url_prefix, filename);
            task_submit_subtask(preload->m3u8.task, url);
            safe_process(free, filename);
        }
    }

    if (p_start < pos)
    {
        data_buf_append(&preload->m3u8.dbuf, p_start, pos - p_start);
    }

    data_buf_destroy(new_dbuf);
}

static void preload_process_preload_reply_body(struct task_preload_st *preload, struct http_body_st *body)
{
    preload_process_preload_reply_body_m3u8(preload, body->dbuf);
    preload_process_preload_reply_body_digest(preload, body->dbuf);
}

static void preload_process_preload_reply_hdr_date(struct array_st *hdrs, char **date)
{
    struct http_header_st *hdr;

    hdr = http_find_header(hdrs, "Date");
    if (NULL == hdr)
    {
        *date = xstrdup("-");
        return;
    }
    *date = xstrdup(hdr->value);
}

static void preload_process_preload_reply_hdr_last_modified(struct array_st *hdrs, char **last_modified)
{
    struct http_header_st *hdr;

    hdr = http_find_header(hdrs, "Last-Modified");
    if (NULL == hdr)
    {
        *last_modified = xstrdup("-");
        return;
    }
    *last_modified = xstrdup(hdr->value);
}

static void preload_process_preload_reply_hdr_content_length(struct array_st *hdrs, char **content_length)
{
    struct http_header_st *hdr;

    hdr = http_find_header(hdrs, "Content-Length");
    if (NULL == hdr)
    {
        *content_length = xstrdup("-");
        return;
    }
    *content_length = xstrdup(hdr->value);
}

static void preload_process_preload_reply_hdr_cache_status(struct array_st *hdrs, char **cache_status)
{
    char *token, *saveptr;
    struct http_header_st *hdr;

    hdr = http_find_header(hdrs, "Powered-By-ChinaCache");
    if (NULL == hdr)
    {
        *cache_status = xstrdup("-");
        return;
    }

    token = strtok_r(hdr->value, " \t", &saveptr);
    if (NULL == token)
    {
        *cache_status = xstrdup("-");
        return;
    }

    if (strcasecmp(token, "HIT"))
    {
        *cache_status = xstrdup("MISS");
        return;
    }

    *cache_status = xstrdup("HIT");
}

static void preload_process_preload_reply_header(struct task_preload_st *preload, struct array_st *hdrs)
{
    preload_process_preload_reply_hdr_date(hdrs, &preload->date);
    preload_process_preload_reply_hdr_last_modified(hdrs, &preload->last_modified);
    preload_process_preload_reply_hdr_content_length(hdrs, &preload->content_length);
    preload_process_preload_reply_hdr_cache_status(hdrs, &preload->cache_status);
}

static void preload_process_preload_reply_line(struct task_preload_st *preload, struct http_reply_st *reply)
{
    char status[SIZE_64];

    if (PRELOAD_STATUS_OK == reply->status)
    {
        preload->status = PRELOAD_STATUS_OK;
    }
    else
    {
        preload->status = PRELOAD_STATUS_ERROR;
    }
    snprintf(status, SIZE_64, "%03d", reply->status);
    preload->http_status = xstrdup(status);
}

static int preload_process_preload_reply(struct task_preload_st *preload, struct http_reply_st *reply)
{
    if (reply->parsed.error)
    {
        return -1;
    }

    if (1 == reply->parsed.line)
    {
        if (0 == preload->line_done)
        {
            preload_process_preload_reply_line(preload, reply);
            preload->line_done = 1;
        }
    }

    if (reply->parsed.header)
    {
        if (0 == preload->hdr_done)
        {
            preload_process_preload_reply_header(preload, &reply->header);
            preload->hdr_done = 1;
        }
        if (1 == reply->parsed.body)
        {
            preload->done = 1;
            preload_process_preload_reply_body(preload, &reply->body);
            data_buf_reset(reply->body.dbuf);
            LogDebug(9,
                     "preload recv data total length: %ld; body length: %ld.",
                     preload->limit_rate.total_len, reply->body.length);
            return 0;
        }
        preload_process_preload_reply_body(preload, &reply->body);
        data_buf_reset(reply->body.dbuf);
        return 1;
    }

    return 1;
}

static int preload_process_report_reply(struct task_report_st *report, struct http_reply_st *reply)
{
    if (reply->parsed.error)
    {
        return -1;
    }

    if (0 == reply->parsed.line)
    {
        return 1;
    }

    if (HTTP_STATUS_OK == reply->status)
    {
        report->done = 1;
        return 0;
    }

    LogDebug(8, "preload report reply status is '%d', retry later.", reply->status);

    report->error = 1;

    return -1;
}

static int preload_process_reply(struct task_st *task, struct http_reply_st *reply)
{
    int ret;

    /* return value:
     * -1: error.
     *  0: data done, recv needlessly.
     *  1: data more for refresh, check the dbuf size to enlarge it.
     *  2: data more for preload, reset the dbuf for recving.
     */

    switch (task->state)
    {
        case TASK_STATE_REFRESH:
            ret = preload_process_refresh_reply(&task->refresh, reply);
            if (-1 == ret)
            {
				LogError(9,
                     "preload task_state_refresh error");
                preload_process_error(task);
                return -1; 
            }
            if (0 == ret)
            {
                preload_process_done(task);
                return 0;
            }
            return 1;
        case TASK_STATE_PRELOAD:
            ret = preload_process_preload_reply(&task->preload, reply);
            if (-1 == ret)
            {
				LogError(9,
                     "preload task_state_preload error");				
                preload_process_error(task);
                return -1;
            }
            if (0 == ret)
            {
                preload_process_done(task);
                return 0;
            }
            return 2;
        case TASK_STATE_REPORT:
            ret = preload_process_report_reply(&task->report, reply);
            if (-1 == ret)
            {
				LogError(9,
                     "preload task_state_report error");
                preload_process_error(task);
                return -1;
            }
            if (0 == ret)
            {
                preload_process_done(task);
                return 0;
            }
            return 1;
        default:
            LogAbort("no this task state.");
            return -1;
    }
}

static void preload_adjust_before_recv(struct task_st *task, struct data_buf_st *dbuf, char **buf, size_t *len)
{
    time_t delta, curtime;
    ssize_t need_len, left_space;
    struct limit_rate_st *lr;

    if (TASK_STATE_PRELOAD != task->state)
    {
        *buf = dbuf->buf + dbuf->offset;
        *len = dbuf->size - dbuf->offset;
        return;
    }

    lr = &task->preload.limit_rate;
    time(&curtime);

    if (0 == lr->start)
    {
        lr->start = curtime;
    }

    if (0 == lr->rate)
    {
        /* no limit rate */
        *buf = dbuf->buf + dbuf->offset;
        *len = dbuf->size - dbuf->offset;
        return;
    }

    *buf = dbuf->buf + dbuf->offset;
    delta = curtime - lr->start + 1;
    need_len = lr->rate * delta - lr->total_len;
    if (need_len <= 0)
    {
        *len = 0;
        return;
    }
    left_space = dbuf->size - dbuf->offset;
    *len = need_len > left_space ? left_space : need_len;
}

static void preload_adjust_after_recv(struct task_st *task, size_t n)
{
    struct limit_rate_st *lr;

    if (TASK_STATE_PRELOAD == task->state)
    {
        lr = &task->preload.limit_rate;
        lr->total_len += n;
        return;
    }
}

static void preload_download_mean_rate(struct task_st *task)
{
    time_t delta;
    struct limit_rate_st *lr;

    if (TASK_STATE_PRELOAD != task->state)
    {
        return;
    }

    lr = &task->preload.limit_rate;
    delta = time(NULL) - lr->start + 1;
    lr->mean_rate = (lr->total_len / delta) * 8 / 1024.0;
}

static RCV_MTD *preload_recv_method(struct task_st *task)
{
    if (TASK_STATE_PRELOAD != task->state)
    {
        return comm_recv;
    }

    if (PROTOCOL_HTTP == task->task_url->protocol)
    {
        return comm_recv;
    }

    return comm_ssl_recv;
}

static void preload_read_reply(void *data)
{
    char *buf;
    size_t len;
    ssize_t n;
    int fd, ret;
    struct task_st *task;
    struct http_st *http;
    struct io_event_st *rev;
    struct comm_preload_st *pl;
    RCV_MTD *recv_method;

    pl   = data;
    fd   = pl->cn->fd;
    rev  = pl->cn->rev;
    task = pl->task;
    http = pl->http;

    recv_method = preload_recv_method(task);

    for ( ; ; )
    {
        preload_adjust_before_recv(task, &rev->dbuf, &buf, &len);
        if (0 == len)
        {
            return;
        }
        n = recv_method(fd, buf, len);
        if (1 == rev->error)
        {
			LogError(9,
                     "preload receive task error");	
            preload_process_error(task);
            comm_close(fd);
            return;
        }
        if (1 == rev->timedout)
        {
			LogError(9,
                     "preload receive task timeout");	
            preload_process_error(task);
            comm_close(fd);
            return;
        }
        if (1 == rev->eof)
        {
            /* FIXME: done ?? */
			LogError(9,
                     "preload receive task error,come in the function preload_process_eof");	
            preload_process_eof(task);
            comm_close(fd);
            return;
        }
        if (n < 0 && 0 == rev->ready)
        {
            /* EAGAIN */
            return;
        }
        rev->dbuf.offset += n;
        preload_adjust_after_recv(task, n);
        http_parse_reply(http->reply, &rev->dbuf);
        ret = preload_process_reply(task, http->reply);
        /* return value:
         * -1: error.
         *  0: data done, recv needlessly.
         *  1: data more for refresh or report, check the dbuf size to enlarge it.
         *  2: data more for preload, reset the dbuf for recving.
         */
        if (-1 == ret)
        {
            comm_close(fd);
            return;
        }
        if (0 == ret)
        {
            preload_download_mean_rate(task);
            comm_close(fd);
            return;
        }
        comm_set_timeout(fd, IO_EVENT_READ, g_global.cfg.http.recv_timeout);
        if (1 == ret)
        {
            if (rev->dbuf.offset == rev->dbuf.size)
            {
                data_buf_resize(&rev->dbuf, rev->dbuf.size + SIZE_64KB);
            }
            continue;
        }
        data_buf_reset(&rev->dbuf);
    }
}

static int preload_ssl_connect(struct comm_preload_st *pl)
{
    struct task_st *task;

    task = pl->task;

    if (TASK_STATE_PRELOAD != task->state)
    {
        return 0;
    }

    if (PROTOCOL_HTTP == task->task_url->protocol)
    {
        return 0;
    }

    /* HTTPS: SSL connect */
    if (comm_ssl_connect(pl->cn) < 0)
    {
        return -1;
    }

    return 0;
}

static SND_MTD *preload_send_method(struct task_st *task)
{
    if (TASK_STATE_PRELOAD != task->state)
    {
        return comm_send;
    }

    if (PROTOCOL_HTTP == task->task_url->protocol)
    {
        return comm_send;
    }

    return comm_ssl_send;
}

static void preload_send_request(void *data)
{
    int fd;
    ssize_t n;
    struct task_st *task;
    struct http_st *http;
    struct io_event_st *wev;
    struct comm_preload_st *pl;
    SND_MTD *send_method;

    pl   = data;
    fd   = pl->cn->fd;
    wev  = pl->cn->wev;
    task = pl->task;
    http = pl->http;

    if (0 == pl->cn->ssl_checked)
    {
        pl->cn->ssl_checked = 1;
        if (preload_ssl_connect(pl) < 0)
        {
            preload_process_error(task);
            comm_close(fd);
            return;
        }
    }

    if (0 == http->request->built)
    {
        preload_build_request(http->request, task);
        http_pack_request(&wev->dbuf, http->request);
        LogDebug(8, "preload send request data: \n%s", wev->dbuf.buf);
    }

    send_method = preload_send_method(task);

    for ( ; ; )
    {
        n = send_method(fd, wev->dbuf.buf, wev->dbuf.offset);
        if (wev->error)
        {
            preload_process_error(task);
            comm_close(fd);
            return;
        }
        if (wev->timedout)
        {
            preload_process_error(task);
            comm_close(fd);
            return;
        }
        if (n <= 0 && 0 == wev->ready)
        {
            /* EAGAIN */
            return;
        }
        wev->dbuf.offset -= n;
        if (0 == wev->dbuf.offset)
        {
            comm_set_timeout(fd, IO_EVENT_WRITE, -1);
            comm_set_io_event(fd, IO_EVENT_WRITE, NULL, NULL);
            comm_set_timeout(fd, IO_EVENT_READ, g_global.cfg.http.recv_timeout);
            comm_set_io_event(fd, IO_EVENT_READ, preload_read_reply, pl);
            return;
        }
        memmove(wev->dbuf.buf, wev->dbuf.buf + n, wev->dbuf.offset);
    }
}

static int preload_connect_peer(struct sockaddr_in *addr, struct task_st *task)
{
    int fd;
    struct comm_preload_st *pl;

    fd = comm_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
    {
        /* FIXME: process error */
        return -1;
    }
    if (comm_set_close_on_exec(fd) < 0)
    {
        close(fd);
        return -1;
    }
    if (comm_set_nonblock(fd) < 0)
    {
        close(fd);
        return -1;
    }
    if (comm_set_reuse_addr(fd) < 0)
    {
        close(fd);
        return -1;
    }
    if (comm_connect(fd, addr) < 0)
    {
        close(fd);
        return -1;
    }
    if (comm_open(fd) < 0)
    {
        close(fd);
        return -1;
    }
    pl = comm_preload_create(fd, task);
    comm_set_connection(fd, pl->cn);
    comm_set_close_handler(fd, comm_preload_destroy, pl);
    comm_set_timeout(fd, IO_EVENT_WRITE, g_global.cfg.http.send_timeout);
    comm_set_io_event(fd, IO_EVENT_WRITE, preload_send_request, pl);

    return 0;
}

static void preload_process_task_refresh(struct task_st *task)
{
    struct sockaddr_in refresh_addr;

    if (1 == task->refresh.done || 1 == task->refresh.error)
    {
        preload_switch_task_state(task);
        return;
    }

    if (1 == task->refresh.start)
    {
        comm_process_io_event(task->cn->fd);
        return;
    }

    refresh_addr.sin_family = AF_INET;
    refresh_addr.sin_port = htons(PORT_DEFAULT_REFRESH);
    refresh_addr.sin_addr = task->preload_addr.sin_addr;
    if (preload_connect_peer(&refresh_addr, task) < 0)
    {
        task->refresh.error = 1;
        return;
    }
    task->refresh.start = 1;
    task->refresh.status = 0;
}

static void preload_process_task_preload(struct task_st *task)
{
    time_t curtime;

    if (task->preload.done || task->preload.error)
    {
        preload_switch_task_state(task);
        return;
    }

    if (1 == task->preload.start)
    {
        comm_process_io_event(task->cn->fd);
        return;
    }

    curtime = time(NULL);
    task->preload.status = 0;
    task->preload.start_time = curtime;
    task->preload.finish_time = curtime;
    digest_init(&task->preload.digest);
    if (preload_connect_peer(&task->preload_addr, task) < 0)
    {
        task->preload.error = 1;
        return;
    }
    task->preload.start = 1;
}

static void preload_process_task_report(struct task_st *task)
{
    if (0 == task->need_report)
    {
        preload_switch_task_state(task);
        return;
    }

    if (1 == task->report.done)
    {
        preload_switch_task_state(task);
        return;
    }

    if (1 == task->report.error)
    {
        if (++task->report.retry >= 3)
        {
            preload_switch_task_state(task);
            return;
        }
        task_report_retry_reset(&task->report);
    }

    if (1 == task->report.start)
    {
        comm_process_io_event(task->cn->fd);
        return;
    }

    if (preload_connect_peer(&task->report_addr, task) < 0)
    {
        task->report.error = 1;
        return;
    }
    task->report.start = 1;
    task->report.status = 0;
}

static void preload_process_task_done(struct task_st *task)
{
    char buf[SIZE_8KB];

    snprintf(buf,
             SIZE_8KB,
             "%s;%s;%s;%s;%s",
             task->sessionid,
             task->task_url->id,
             task->task_url->url,
             task->refresh.status ? "success" : "failed",
             task->preload.status ? "success" : "failed");

    LogAccess("%s", buf);

    task->done = 1;
    task_queue_remove(RUNNING_QUEUE, task, is_matched);
}

static void preload_process_task_error(struct task_st *task)
{
    LogError(1, "preload error for task: %s.", task->task_url->url);
    preload_switch_task_state(task);
}

static void task_process_ready(struct task_st *task)
{
    preload_switch_task_state(task);
}

static void preload_process_task(struct task_st *task)
{
    switch (task->state)
    {
        case TASK_STATE_READY: 
            task_process_ready(task);
            break;
        case TASK_STATE_REFRESH:
            preload_process_task_refresh(task);
            break;
        case TASK_STATE_PRELOAD:
            preload_process_task_preload(task);
            break;
        case TASK_STATE_REPORT:
            preload_process_task_report(task);
            break;
        case TASK_STATE_DONE:
            preload_process_task_done(task);
            break;
        case TASK_STATE_ERROR:
            preload_process_task_error(task);
            break;
        default:
        /* error */
            break;
    }
}

static int preload_trylock_task(void *key, void *data)
{
    struct task_st *task = data;

    return pthread_mutex_trylock(&task->mutex);
}

static struct task_st *preload_lock_task(void)
{
    return task_queue_find(RUNNING_QUEUE, NULL, preload_trylock_task);
}

static void preload_sync_task(void)
{
    struct task_st *task;
    unsigned int i, concurrency;

    for (i = 0; i < g_global.cfg.task_concurrency; ++i)
    {
        concurrency = task_queue_number(RUNNING_QUEUE);
        if (concurrency >= g_global.cfg.task_concurrency)
        {
            return;
        }
        task = task_queue_fetch(WAITING_QUEUE, NULL, always_matched);
        if (NULL == task)
        {
            return;
        }
        task_queue_insert(RUNNING_QUEUE, task);
    }
}

static void preload_process(void)
{
    int should_free;
    struct task_st *task;

    for ( ; ; )
    {
        comm_select_io_event();
        task = preload_lock_task();
        if (NULL != task)
        {
            should_free = 0;
            preload_process_task(task);
            if (task->done)
            {
                should_free = 1;
            }
            pthread_mutex_unlock(&task->mutex);
            if (should_free)
            {
                task_free(task);
            }
        }
        preload_sync_task();
    }
}

static void preload_set_signal(void)
{
    sigset_t s;

    sigemptyset(&s);
    sigaddset(&s, SIGINT);
    sigaddset(&s, SIGALRM);
    sigaddset(&s, SIGTERM);
    sigaddset(&s, SIGHUP);
    sigaddset(&s, SIGPIPE);
    sigaddset(&s, SIGUSR1);
    sigaddset(&s, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &s, NULL);
}

static void *preload_start_routine(void *arg)
{
    pthread_detach(pthread_self());
    preload_set_signal();
    preload_process();

    return NULL;
}

void preload_worker_start(void)
{
    int i;
    pthread_t tid;

    for (i = 0; i < g_preload.cfg.worker_number; ++i)
    {
        if (pthread_create(&tid, NULL, preload_start_routine, NULL))
        {
            LogError(1, "pthread_create() error for preload worker: %s.", xerror());
            exit(1);
        }
    }
}

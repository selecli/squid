#include "preloader.h"

static void grab_clean_reset(struct comm_grab_st *grab)
{
    grab->error = 0;
    grab->start = 0;
    grab->done  = 0;
    grab->dns_query_ok = 0;
    grab->cn = NULL;
    grab->last_time = time(NULL);
    grab->state = GRAB_STATE_READY;
    memset(&grab->addr, 0, g_tsize.sockaddr_in);
}

static void grab_process_error(struct comm_grab_st *grab)
{
    grab->error = 1;
}

static void grab_process_eof(struct comm_grab_st *grab)
{
    grab_process_error(grab);
}

static void grab_process_done(struct comm_grab_st *grab)
{
    grab->done = 1;
}

static void grab_build_request(struct http_request_st *request)
{
    struct http_header_st *hdr;

    http_build_request_line(request,
                            HTTP_METHOD_GET, 
                            "/preload/error_task",
                            "HTTP", 1, 1);
    hdr = http_build_header("Host", "www.grab.com");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("Accept", "*/*");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("User-Agent", "ChinaCache");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("X-CC-Preload-Grab", "ChinaCache");
    http_add_header(&request->header, hdr);
    hdr = http_build_header("Connection", "close");
    http_add_header(&request->header, hdr);
    request->built = 1;
}

static void grab_process_reply(struct http_reply_st *reply)
{
    struct data_buf_st *dbuf;
    struct session_st *session;
    char *xml_begin, *xml_end;
    size_t xml_length, left_length;

    dbuf = reply->body.dbuf;
    xml_begin = dbuf->buf;
    left_length = dbuf->offset;

    for ( ; ; )
    {
        if (0 == left_length)
        {
            break;
        }

        xml_end = memchr(xml_begin, '\n', left_length);
        if (NULL == xml_end)
        {
            xml_length = left_length;
            left_length = 0;
        }
        else
        {
            *xml_end = '\0';
            xml_length = xml_end - xml_begin + 1;
            left_length = left_length - xml_length;
        }

        LogDebug(5, "xml_length: %ld; left_length: %ld.", xml_length, left_length);

        session = xml_parse(xml_begin, xml_length);
        if (NULL != session)
        {
            task_submit(session);
            session_free(session);
        }

        xml_begin = xml_end + 1;
    }
}

static void grab_read_reply(void *data)
{
    int fd;
    ssize_t n;
    struct comm_ip_st *ip;
    struct io_event_st *rev;
    struct http_st *http;
    struct comm_grab_st *grab;

    ip   = data;
    fd   = ip->cn->fd;
    rev  = ip->cn->rev;
    http = ip->http;
    grab = ip->grab;

    for ( ; ; )
    {
        n = comm_recv(fd,
                      rev->dbuf.buf + rev->dbuf.offset,
                      rev->dbuf.size - rev->dbuf.offset);
        if (rev->error)
        {
            grab_process_error(grab);
            comm_close(fd);
            return;
        }
        if (rev->timedout)
        {
            grab_process_error(grab);
            comm_close(fd);
            return;
        }
        if (rev->eof)
        {
            grab_process_eof(grab);
            comm_close(fd);
            return;
        }
        if (n < 0 && 0 == rev->ready)
        {
            /* EAGAIN */
            return;
        }

        rev->dbuf.offset += n;
        rev->dbuf.buf[rev->dbuf.offset] = '\0';
        LogDebug(9, "grab recv reply data: \n%s", rev->dbuf.buf);

        http_parse_reply(http->reply, &rev->dbuf);

        if (http->reply->parsed.error)
        {
            grab_process_error(grab);
            comm_close(fd);
            return;
        }
        if (http->reply->parsed.done)
        {
            grab_process_reply(http->reply);
            grab_process_done(grab);
            comm_close(fd);
            return;
        }
        comm_set_timeout(fd, IO_EVENT_READ, g_global.cfg.http.recv_timeout);
        if (rev->dbuf.offset == rev->dbuf.size)
        {
            data_buf_resize(&rev->dbuf, rev->dbuf.size + SIZE_64KB);
        }
    }
}

static void grab_send_request(void *data)
{
    int fd;
    ssize_t n;
    struct comm_ip_st *ip;
    struct http_st *http;
    struct io_event_st *wev;
    struct comm_grab_st *grab;

    ip   = data;
    fd   = ip->cn->fd;
    wev  = ip->cn->wev;
    http = ip->http;
    grab = ip->grab;

    if (0 == http->request->built)
    {
        grab_build_request(http->request);
        http_pack_request(&wev->dbuf, http->request);
        LogDebug(9, "grab send request data: \n%s", wev->dbuf.buf);
    }

    for ( ; ; )
    {
        n = comm_send(fd, wev->dbuf.buf, wev->dbuf.offset);
        if (wev->error)
        {
            grab_process_error(grab);
            comm_close(fd);
            return;
        }
        if (wev->timedout)
        {
            grab_process_error(grab);
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
            comm_set_io_event(fd, IO_EVENT_READ, grab_read_reply, ip);
            return;
        }
        memmove(wev->dbuf.buf, wev->dbuf.buf + n, wev->dbuf.offset);
    }
}

static int grab_connect_peer(struct sockaddr_in *addr, struct comm_grab_st *grab)
{
    int fd;
    struct comm_ip_st *ip;

    LogDebug(5, "grab connect to ip '%s'.", inet_ntoa(addr->sin_addr));

    fd = comm_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
    {
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
    ip = comm_ip_create(fd, grab);
    grab->cn = ip->cn;
    comm_set_connection(fd, ip->cn);
    comm_set_close_handler(fd, comm_ip_destroy, ip);
    comm_set_timeout(fd, IO_EVENT_WRITE, g_global.cfg.http.send_timeout);
    comm_set_io_event(fd, IO_EVENT_WRITE, grab_send_request, ip);

    return 0;
}

static unsigned short grab_dns_query_id(void)
{
    static unsigned short dns_query_id_seed = 1;

    return (dns_query_id_seed++);
}

static int grab_dns_parse_reply(struct in_addr *addr, char * buf, size_t sz)
{
    int k, n, na, ttl, parse_ok;
    struct rfc1035_rr_st *answers;
    struct rfc1035_message_st *message;

    na       = 0;
    ttl      = 0;
    parse_ok = 0;
    message  = NULL;

    n = rfc1035MessageUnpack(buf, sz,  &message);
    if (message == NULL)
    {    
        LogError(1, "malformed dns response.");
        return -1;
    }    
    if (n < 0)
    {
        LogError(1, "dns query error %s (%d).", rfc1035_error_message, rfc1035_errno);
        rfc1035MessageDestroy(message);
        return -1;
    }

    answers = message->answer;

    for (k = 0; k < n; k++)
    {
        if (answers[k].type != RFC1035_TYPE_A)
        {
            continue;
        }
        if (answers[k].class != RFC1035_CLASS_IN)
        {
            continue;
        }
        if (answers[k].rdlength != 4)
        {
            continue;
        }
        na++;
    }

    if (0 == na)
    {
        LogError(1, "dns query result: no address records in response.");
        rfc1035MessageDestroy(message);
        return -1;
    }

    for (k = 0; k < n; k++)
    {
        if (answers[k].class != RFC1035_CLASS_IN)
        {
            continue;
        }
        if (answers[k].type == RFC1035_TYPE_A)
        {
            if (answers[k].rdlength != 4)
            {
                continue;
            }
            parse_ok = 1;
            memcpy(addr, answers[k].rdata, 4);
            break;
        }
        else if (answers[k].type != RFC1035_TYPE_CNAME)
        {
            continue;
        }
        if (ttl == 0 || ttl > answers[k].ttl)
        {
            ttl = answers[k].ttl;
        }
    }

    rfc1035MessageDestroy(message);

    return (parse_ok ? 0 : -1);
}

static void grab_dns_recv_reply(void *data)
{
    int fd;
    ssize_t n;
    char buf[SIZE_8KB];
    struct in_addr *addr;
    struct comm_dns_st *dns;
    struct io_event_st *rev;
    struct sockaddr_in peer_addr;
    struct comm_grab_st *grab;
    socklen_t peer_addr_len;

    dns  = data;
    fd   = dns->cn->fd;
    rev  = dns->cn->rev;
    grab = dns->grab;
    addr = &dns->grab->addr.sin_addr;

    n = comm_recvfrom(fd,
                      buf, SIZE_8KB,
                      0,
                      (struct sockaddr *)&peer_addr,
                      &peer_addr_len);
    if (rev->error)
    {
        grab_process_error(grab);
        comm_close(fd);
        return;
    }
    if (rev->timedout)
    {
        grab_process_error(grab);
        comm_close(fd);
        return;
    }
    if (rev->eof)
    {
        grab_process_eof(grab);
        comm_close(fd);
        return;
    }
    if (n < 0 && 0 == rev->ready)
    {
        /* EAGAIN */
        return; 
    }
    if (grab_dns_parse_reply(addr, buf, n) < 0)
    {
        grab_process_error(grab);
        comm_close(fd);
        return;
    }
    grab->dns_query_ok = 1;
    grab_process_done(grab);
    comm_close(fd);
}

static struct dns_query_st *grab_dns_query_create(char *host)
{
    struct dns_query_st * q;

    q = xcalloc(1, g_tsize.dns_query_st);
    q->id = grab_dns_query_id();
    q->start_time = time(NULL);
    q->name = xstrdup(host);
    q->len = rfc1035BuildAQuery(q->name,
                                q->buf,
                                sizeof(q->buf),
                                q->id,
                                &q->query);

    if (q->len < 0)
    {
        safe_process(free, q->name);
        safe_process(free, q);
        return NULL;
    }

    return q;
}

int grab_dns_send_query(char *host, struct sockaddr_in *dns_addr, struct comm_grab_st *grab)
{
    int fd, ret;
    struct dns_query_st *q;
    struct comm_dns_st *dns;

    fd = comm_socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
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
    if (comm_open(fd) < 0)
    {
        close(fd);
        return -1;
    }
    q = grab_dns_query_create(host);
    if (NULL == q)
    {
        comm_close(fd);
        return -1;
    }
    ret = comm_sendto(fd,
                      q->buf, q->len,
                      0,
                      (struct sockaddr *)dns_addr,
                      g_tsize.sockaddr_in);
    safe_process(free, q->name);
    safe_process(free, q);
    if (ret < 0)
    {
        comm_close(fd);
        return -1;
    }
    dns = comm_dns_create(fd, grab);
    grab->cn = dns->cn;
    comm_set_connection(fd, dns->cn);
    comm_set_close_handler(fd, comm_dns_destroy, dns);
    comm_set_timeout(fd, IO_EVENT_READ, g_global.cfg.http.recv_timeout);
    comm_set_io_event(fd, IO_EVENT_READ, grab_dns_recv_reply, dns);

    return 0;
}

static void grab_by_ip(void)
{
    if (1 == g_grab.grab.done || 1 == g_grab.grab.error)
    {
        grab_clean_reset(&g_grab.grab);
        return;
    }

    if (1 == g_grab.grab.start)
    {
        if (NULL != g_grab.grab.cn)
        {
            comm_process_io_event(g_grab.grab.cn->fd);
        }
        return;
    }

    if (grab_connect_peer(&g_grab.grab.addr, &g_grab.grab) < 0)
    {
        grab_process_error(&g_grab.grab);
        return;
    }

    g_grab.grab.start = 1;
}

static void grab_by_dns(void)
{
    int ret;
    struct in_addr sin_addr;

    if (1 == g_grab.grab.error)
    {
        grab_clean_reset(&g_grab.grab);
        return;
    }

    if (1 == g_grab.grab.done)
    {
        if (0 == g_grab.grab.dns_query_ok)
        {
            grab_clean_reset(&g_grab.grab);
            return;
        }
        sin_addr = g_grab.grab.addr.sin_addr;
        grab_clean_reset(&g_grab.grab);
        g_grab.grab.addr.sin_family = AF_INET;
        g_grab.grab.addr.sin_port = g_grab.cfg.addr.sockaddr.dns.port;
        g_grab.grab.addr.sin_addr = sin_addr;
        g_grab.grab.state = GRAB_STATE_IP;
        return;
    }

    if (1 == g_grab.grab.start)
    {
        if (NULL != g_grab.grab.cn)
        {
            comm_process_io_event(g_grab.grab.cn->fd);
        }
        return;
    }

    ret = grab_dns_send_query(g_grab.cfg.addr.sockaddr.dns.host,
                              &g_global.cfg.dns_addr,
                              &g_grab.grab);
    if (ret < 0)
    {
        grab_process_error(&g_grab.grab);
        return;
    }

    g_grab.grab.start = 1;
}

static void grab_process_ready(void)
{
    time_t curtime;
    time_t checktime;

    curtime = time(NULL);
    checktime = g_grab.grab.last_time + g_grab.cfg.interval_time;
    if (checktime > curtime)
    {
        return;
    }

    switch (g_grab.cfg.addr.type)
    {
        case SOCKADDR_IP:
        g_grab.grab.addr = g_grab.cfg.addr.sockaddr.ip;
        g_grab.grab.state = GRAB_STATE_IP;
        break;
        case SOCKADDR_DNS:
        g_grab.grab.state = GRAB_STATE_DNS;
        break;
        default:
        LogAbort("no this grab address type.");
        break;
    }
}

static void grab_task(void)
{
    switch (g_grab.grab.state)
    {
        case GRAB_STATE_READY:
            grab_process_ready();
            break;
        case GRAB_STATE_IP:
            grab_by_ip();
            break;
        case GRAB_STATE_DNS:
            grab_by_dns();
            break;
        default:
            break;
    }
}

static void grab_process(void)
{
    for ( ; ; )
    {
        grab_task();
        comm_select_io_event();
    }
}

static void grab_set_signal(void)
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

static void *grab_start_routine(void *arg)
{
    pthread_detach(pthread_self());
    grab_set_signal();
    grab_process();

    return NULL;
}

void grab_worker_start(void)
{
    pthread_t tid;

    if (pthread_create(&tid, NULL, grab_start_routine, NULL))
    {
        LogError(1, "pthread_create() error for grab worker: %s.", xerror());
        exit(1);
    }
}

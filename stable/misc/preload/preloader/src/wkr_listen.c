#include "preloader.h"

static void listen_send_reply(void *data)
{
    int fd;
    ssize_t n;
    struct http_st *http;
    struct io_event_st *wev;
    struct comm_accept_st *ac;

    ac   = data;
    fd   = ac->cn->fd;
    wev  = ac->cn->wev;
    http = ac->http;

    if (0 == http->reply->packed)
    {
        data_buf_reset(&wev->dbuf);
        http_pack_reply(&wev->dbuf, http->reply);
    }

    for ( ; ; )
    {
        n = comm_send(fd, wev->dbuf.buf, wev->dbuf.offset);
        if (1 == wev->error)
        {
            comm_close(fd);
            return;
        }
        if (1 == wev->timedout)
        {
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
            comm_close(fd);
            return;
        }
        memmove(wev->dbuf.buf, wev->dbuf.buf + n, wev->dbuf.offset);
    }
}

static struct data_buf_st *listen_build_reply_body_data(struct session_st *session)
{
    size_t len;
    char buf[SIZE_4KB];
    struct data_buf_st *dbuf;
    struct task_url_st *task_url;
    struct list_st *taskurl_list;
    struct list_node_st *cur;

    dbuf = data_buf_create(SIZE_64KB);
    taskurl_list = session->taskurl_list;

    len = snprintf(buf, SIZE_64KB, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    data_buf_append(dbuf, buf, len);

    len = snprintf(buf, SIZE_64KB, "<preload_response sessionid=\"%s\">\n", session->id);
    data_buf_append(dbuf, buf, len);

    for (cur = taskurl_list->head; NULL != cur; cur = cur->next)
    {
        task_url = cur->data;
        len = snprintf(buf, SIZE_64KB, "<pre_ret id=\"%s\">200</pre_ret>\n", task_url->id);
        data_buf_append(dbuf, buf, len);
    }

    len = snprintf(buf, SIZE_64KB, "</preload_response>\n");
    data_buf_append(dbuf, buf, len);

    return dbuf;
}

static void listen_process_request(struct comm_accept_st *ac)
{
    int waiting_number;
    struct http_st *http;
    struct data_buf_st *dbuf;
    struct session_st *session;

    http = ac->http;
    waiting_number = task_queue_number(WAITING_QUEUE);
    if (waiting_number >= g_global.cfg.task_limit_number)
    {
        http_build_reply(http->reply,
                         HTTP_STATUS_FORBIDDEN,
                         http->request->major,
                         http->request->minor,
                         NULL);
        return;
    }
    dbuf = http->request->body.dbuf;
    session = xml_parse(dbuf->buf, dbuf->offset);
    if (NULL == session)
    {
        http_build_reply(http->reply,
                         HTTP_STATUS_BAD_REQUEST,
                         http->request->major,
                         http->request->minor,
                         NULL);
        return;
    }
    dbuf = listen_build_reply_body_data(session);
    http_build_reply(http->reply,
                     HTTP_STATUS_OK,
                     http->request->major,
                     http->request->minor,
                     dbuf);
    data_buf_destroy(dbuf);
    task_submit(session);
    session_free(session);
}

static void listen_read_request(void *data)
{
    int fd;
    ssize_t n;
    struct http_st *http;
    struct io_event_st *rev;
    struct comm_accept_st *ac;

    ac   = data;
    fd   = ac->cn->fd;
    rev  = ac->cn->rev;
    http = ac->http;

    for ( ; ; )
    {
        n = comm_recv(fd,
                      rev->dbuf.buf + rev->dbuf.offset,
                      rev->dbuf.size - rev->dbuf.offset);
        if (1 == rev->error)
        {
            comm_close(fd);
            return;
        }
        if (1 == rev->timedout)
        {
            comm_close(fd);
            return;
        }
        if (1 == rev->eof)
        {
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
        LogDebug(9, "listen recv request data: \n%s", rev->dbuf.buf);

        http_parse_request(http->request, &rev->dbuf);

        if (http->request->parsed.error)
        {
            http_build_reply(http->reply,
                             HTTP_STATUS_BAD_REQUEST,
                             http->request->major,
                             http->request->minor,
                             NULL);
            comm_set_timeout(fd, IO_EVENT_READ, -1);
            comm_set_io_event(fd, IO_EVENT_READ, NULL, NULL);
            comm_set_timeout(fd, IO_EVENT_WRITE, g_global.cfg.http.send_timeout);
            comm_set_io_event(fd, IO_EVENT_WRITE, listen_send_reply, ac);
            return;
        }

        if (http->request->parsed.done)
        {
            if (HTTP_METHOD_POST != http->request->method)
            {
                http_build_reply(http->reply,
                                 HTTP_STATUS_BAD_REQUEST,
                                 http->request->major,
                                 http->request->minor,
                                 NULL);
                comm_set_timeout(fd, IO_EVENT_READ, -1);
                comm_set_io_event(fd, IO_EVENT_READ, NULL, NULL);
                comm_set_timeout(fd, IO_EVENT_WRITE, g_global.cfg.http.send_timeout);
                comm_set_io_event(fd, IO_EVENT_WRITE, listen_send_reply, ac);
                return;
            }
            listen_process_request(ac);
            comm_set_timeout(fd, IO_EVENT_READ, -1);
            comm_set_io_event(fd, IO_EVENT_READ, NULL, NULL);
            comm_set_timeout(fd, IO_EVENT_WRITE, g_global.cfg.http.send_timeout);
            comm_set_io_event(fd, IO_EVENT_WRITE, listen_send_reply, ac);
            return;
        }

        if (rev->dbuf.offset == rev->dbuf.size)
        {
            data_buf_resize(&rev->dbuf, rev->dbuf.size + SIZE_64KB);
        }
    }
}

void listen_process_timeout(void *data)
{
}

static void listen_accept(void *data)
{
    int n, fd, lfd;
    struct io_event_st *rev;
    struct comm_accept_st *ac;
    struct comm_listen_st *ls;

    ls  = data;
    lfd = ls->cn->fd;
    rev = ls->cn->rev;

    for (n = 0; n < ls->n_accept_once; ++n)
    {
        fd = comm_accept(lfd);
        if (1 == rev->error)
        {
            comm_close(lfd);
            server_exit(1);
            return;
        }
        if (0 == rev->ready)
        {
            /* EAGAIN: no data now. */
            return;
        }
        if (comm_set_close_on_exec(fd) < 0)
        {
            close(fd);
            return;
        }
        if (comm_set_nonblock(fd) < 0)
        {
            close(fd);
            return;
        }
        if (comm_set_reuse_addr(fd) < 0)
        {
            close(fd);
            return;
        }
        if (comm_open(fd) < 0)
        {
            close(fd);
            return;
        }
        ac = comm_accept_create(fd, ls);
        comm_set_connection(fd, ac->cn);
        comm_set_close_handler(fd, comm_accept_close, ac);
        comm_set_timeout(fd, IO_EVENT_READ, g_global.cfg.http.recv_timeout);
        comm_set_io_event(fd, IO_EVENT_READ, listen_read_request, ac);
    }
}

static struct comm_listen_st *listen_open(struct sockaddr_in *addr)
{
    int fd;
    struct comm_listen_st *ls;

    fd = comm_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
    {
        server_exit(1);
    }
    if (comm_set_close_on_exec(fd) < 0)
    {
        server_exit(1);
    }
    if (comm_set_nonblock(fd) < 0)
    {
        server_exit(1);
    }
    if (comm_set_reuse_addr(fd) < 0)
    {
        server_exit(1);
    }
    if (comm_bind(fd, addr) < 0)
    {
        server_exit(1);
    }
    if (comm_listen(fd, 256) < 0)
    {
        server_exit(1);
    }
    if (comm_open(fd) < 0)
    {
        server_exit(1);
    }
    ls = comm_listen_create(fd);
    comm_set_connection(fd, ls->cn);
    comm_set_close_handler(fd, comm_listen_destroy, ls);
    comm_set_io_event(fd, IO_EVENT_READ, listen_accept, ls);

    return ls;
}

static int listen_check_accepted_request(void *data)
{
    struct comm_accept_st *ac = data;

    comm_process_io_event(ac->cn->fd);

    return 0;
}

static void listen_process_accepted_request(struct comm_listen_st *ls)
{
    list_travel(ls->accept_list, listen_check_accepted_request);
}

static int accept_check_done(void *key, void *data)
{
    struct comm_accept_st *ac = data;

    return (0 == ac->done);
}

static void listen_check_accepted_list(struct comm_listen_st *ls)
{
    struct comm_accpet_st *ac;

    for ( ; ; )
    {
        ac = list_fetch(ls->accept_list, NULL, accept_check_done);
        if (NULL == ac)
        {
            break;
        }
        comm_accept_destroy(ac);
    }
}

static void listen_process(void)
{
    struct comm_listen_st *ls;

    ls = listen_open(&g_listen.cfg.addr);

    for ( ; ; )
    {
        comm_select_io_event();
        comm_process_io_event(ls->cn->fd);
        listen_process_accepted_request(ls);
        listen_check_accepted_list(ls);
    }
}

static void listen_set_signal(void)
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

static void *listen_start_routine(void * arg)
{
    pthread_detach(pthread_self());
    listen_set_signal();
    listen_process();

    return NULL;
}

void listen_worker_start(void)
{
    pthread_t tid;

    if (pthread_create(&tid, NULL, listen_start_routine, NULL))
    {
        LogError(1, "pthread_create() error for listen worker: %s.", xerror());
        exit(1);
    }
}

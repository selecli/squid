#include "preloader.h"

struct data_buf_st *data_buf_create(size_t size)
{
    struct data_buf_st *dbuf;

    dbuf = xmalloc(g_tsize.data_buf_st);
    data_buf_init(dbuf, size);

    return dbuf;
}

void data_buf_init(struct data_buf_st *dbuf, size_t size)
{
    dbuf->buf    = xmalloc(size + 1);
    dbuf->size   = size;
    dbuf->offset = 0;
}

void data_buf_resize(struct data_buf_st *dbuf, size_t size)
{
    dbuf->buf  = xrealloc(dbuf->buf, size + 1);
    dbuf->size = size;
}

void data_buf_destroy(struct data_buf_st *dbuf)
{
    safe_process(free, dbuf->buf);
    safe_process(free, dbuf);
}

void data_buf_append(struct data_buf_st *dbuf, const char *buf, size_t len)
{
    xassert(NULL != buf);

    if (dbuf->size < dbuf->offset + len)
    {
        data_buf_resize(dbuf, dbuf->offset + len);
    }
    memcpy(dbuf->buf + dbuf->offset, buf, len);
    dbuf->offset += len;
    dbuf->buf[dbuf->offset] = '\0';
}

void data_buf_reset(struct data_buf_st *dbuf)
{
    dbuf->offset = 0;
}

void data_buf_clean(struct data_buf_st *dbuf)
{
    dbuf->size   = 0;
    dbuf->offset = 0;
    safe_process(free, dbuf->buf);
}

struct data_buf_st *data_buf_clone(struct data_buf_st *dbuf)
{
    struct data_buf_st *new_dbuf;

    new_dbuf = xmalloc(dbuf->offset + 1);
    new_dbuf->offset = dbuf->offset;
    memcpy(new_dbuf->buf, dbuf->buf, dbuf->offset);
    new_dbuf->buf[new_dbuf->offset] = '\0';

    return new_dbuf;
}

void comm_ssl_destroy(struct comm_ssl_st *ssl)
{
    safe_process(SSL_free, ssl->ssl);
    safe_process(SSL_CTX_free, ssl->ctx);
    safe_process(free, ssl);
}

void comm_connect_destroy(void *data)
{
    struct comm_connect_st *cn = data;

    safe_process(comm_ssl_destroy, cn->ssl);
    safe_process(comm_io_event_destroy, cn->rev);
    safe_process(comm_io_event_destroy, cn->wev);
    safe_process(free, cn);
}

void comm_listen_destroy(void *data)
{
    struct comm_listen_st *ls = data;

    comm_connect_destroy(ls->cn);
    list_destroy(ls->accept_list);
    safe_process(free, ls);
}

void comm_io_event_destroy(struct io_event_st *ev)
{
    pthread_mutex_lock(&ev->mutex);
    data_buf_clean(&ev->dbuf);
    if (ev->timer_set)
    {
        rbtree_remove(g_rbtree, &ev->timer);
    }
    pthread_mutex_unlock(&ev->mutex);
    pthread_mutex_destroy(&ev->mutex);
    safe_process(free, ev);
}

static void comm_event_timer_init(struct rbtree_node_st *timer)
{
    timer->key    = 0;
    timer->color  = RBTREE_RED;
    timer->parent = NULL;
    timer->left   = NULL;
    timer->right  = NULL;
}

static void comm_io_event_init(struct io_event_st *ev)
{
    memset(ev, 0, g_tsize.io_event_st);
    data_buf_init(&ev->dbuf, SIZE_32KB);
    comm_event_timer_init(&ev->timer);
    pthread_mutex_init(&ev->mutex, NULL);
}

struct io_event_st *comm_io_event_create(void)
{
    struct io_event_st *ev;

    ev = xcalloc(1, g_tsize.io_event_st);
    comm_io_event_init(ev);

    return ev;
}

struct comm_listen_st *comm_listen_create(int fd)
{
    struct comm_listen_st *ls;

    ls                = xmalloc(g_tsize.comm_listen_st);
    ls->n_accept_once = 5;
    ls->cn            = comm_connect_create(fd);
    ls->accept_list   = list_create();

    return ls;
}

struct comm_accept_st *comm_accept_create(int fd, struct comm_listen_st *ls)
{
    struct comm_accept_st *ac;

    ac       = xmalloc(g_tsize.comm_accept_st);
    ac->ls   = ls;
    ac->done = 0;
    ac->http = http_create();
    ac->cn   = comm_connect_create(fd);

    list_insert_tail(ls->accept_list, ac);

    return ac;
}

struct comm_connect_st *comm_connect_create(int fd)
{
    struct comm_connect_st *cn;

    cn              = xmalloc(g_tsize.comm_connect_st);
    cn->fd          = fd;
    cn->link        = 0;
    cn->ssl_checked = 0;
    cn->ssl         = NULL;
    cn->rev         = comm_io_event_create();
    cn->wev         = comm_io_event_create();

    return cn;
}

struct comm_connect_st *comm_connect_link(struct comm_connect_st *cn)
{
    cn->link++;

    return cn;
}

void comm_connect_unlink(struct comm_connect_st *cn)
{
    cn->link--;
}

struct comm_ssl_st *comm_ssl_create(SSL *ssl, SSL_CTX *ctx)
{
    struct comm_ssl_st *new_ssl;

    new_ssl          = xmalloc(g_tsize.comm_ssl_st);
    new_ssl->version = SSL_V23; /* default: SSLv23 */
    new_ssl->ssl     = ssl;
    new_ssl->ctx     = ctx;

    return new_ssl;
}

struct comm_preload_st *comm_preload_create(int fd, struct task_st *task)
{
    struct comm_preload_st *pl;

    pl       = xmalloc(g_tsize.comm_preload_st);
    pl->cn   = comm_connect_create(fd);
    pl->http = http_create();
    pl->task = task;
    task->cn = comm_connect_link(pl->cn);

    return pl;
}

struct comm_dns_st *comm_dns_create(int fd, struct comm_grab_st *grab)
{
    struct comm_dns_st *dns;

    dns       = xmalloc(g_tsize.comm_dns_st);
    dns->cn   = comm_connect_create(fd);
    dns->grab = grab;

    return dns;
}

struct comm_ip_st *comm_ip_create(int fd, struct comm_grab_st *grab)
{
    struct comm_ip_st *ip;

    ip       = xmalloc(g_tsize.comm_ip_st);
    ip->cn   = comm_connect_create(fd);
    ip->http = http_create();
    ip->grab = grab;

    return ip;
}

void comm_dns_destroy(void *data)
{
    struct comm_dns_st *dns = data;

    safe_process(comm_connect_destroy, dns->cn);
    safe_process(free, dns);
}

void comm_ip_destroy(void *data)
{
    struct comm_ip_st *ip = data;

    safe_process(http_destroy, ip->http);
    safe_process(comm_connect_destroy, ip->cn);
    safe_process(free, ip);
}

void comm_preload_destroy(void *data)
{
    struct comm_preload_st *pl = data;

    safe_process(http_destroy, pl->http);
    safe_process(comm_connect_destroy, pl->cn);
    safe_process(free, pl);
}

void comm_accept_close(void *data)
{
    struct comm_accept_st *ac = data;

    ac->done = 1;
}

void comm_accept_destroy(void *data)
{
    struct comm_accept_st *ac = data;

    safe_process(http_destroy, ac->http);
    safe_process(comm_connect_destroy, ac->cn);
    safe_process(free, ac);
}

int comm_set_block(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
    {
        LogError(1, "fcntl(F_GETFL) error: %s.", xerror());
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags&(~O_NONBLOCK)) < 0)
    {
        LogError(1, "fcntl(F_SETFL) error: %s.", xerror());
        return -1;
    }

    return 0;
}

int comm_set_nonblock(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
    {
        LogError(1, "fcntl(F_GETFL) error: %s.", xerror());
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags|O_NONBLOCK) < 0)
    {
        LogError(1, "fcntl(F_SETFL) error: %s.", xerror());
        return -1;
    }

    return 0;
}

int comm_set_reuse_addr(int fd)
{
    int v = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0)
    {
        LogError(1, "setsockopt(SO_REUSEADDR) error: %s.", xerror());
        return -1;
    }

    return 0;
}

int comm_set_close_on_exec(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFD);
    if (flags < 0)
    {
        LogError(1, "fcntl(F_GETFD) error: %s.", xerror());
        return -1;
    }

    if (fcntl(fd, F_SETFD, flags|FD_CLOEXEC) < 0)
    {
        LogError(1, "fcntl(F_SETFD) error: %s.", xerror());
        return -1;
    }

    return 0;
}

void comm_set_connection(int fd, struct comm_connect_st *cn)
{
    struct comm_st *comm;

    comm = &g_commtable[fd];
    pthread_mutex_lock(&comm->mutex);
    comm->cn = cn;
    pthread_mutex_unlock(&comm->mutex);
}

void comm_set_close_handler(int fd, CB_HDL *handler, void *handler_data)
{
    struct comm_st           *comm;
    struct close_handler_st  *ch;

    comm = &g_commtable[fd];
    pthread_mutex_lock(&comm->mutex);
    ch                  = xmalloc(g_tsize.close_handler_st);
    ch->handler         = handler;
    ch->data            = handler_data;
    ch->next            = comm->close_handler;
    comm->close_handler = ch;
    pthread_mutex_unlock(&comm->mutex);
}

static void comm_remove_event_timeout(struct io_event_st *ev)
{
    pthread_mutex_lock(&ev->mutex);
    if (ev->timer_set)
    {
        rbtree_remove(g_rbtree, &ev->timer);
        ev->timer_set = 0;
    }
    pthread_mutex_unlock(&ev->mutex);
}

static void comm_add_event_timeout(struct io_event_st *ev, time_t timeout)
{
    pthread_mutex_lock(&ev->mutex);
    ev->timer.key = time(NULL) + timeout;
    ev->timer_set = 1;
    rbtree_insert(g_rbtree, &ev->timer);
    pthread_mutex_unlock(&ev->mutex);
}

void comm_set_timeout(int fd, int ev_type, time_t timeout)
{
    struct comm_st *comm;

    comm = &g_commtable[fd];

    pthread_mutex_lock(&comm->mutex);
    switch (ev_type)
    {
        case IO_EVENT_READ:
            comm_remove_event_timeout(comm->cn->rev);
            if (timeout > 0)
            {
                comm_add_event_timeout(comm->cn->rev, timeout);
            }
            break;
        case IO_EVENT_WRITE:
            comm_remove_event_timeout(comm->cn->wev);
            if (timeout > 0)
            {
                comm_add_event_timeout(comm->cn->wev, timeout);
            }
            break;
        default:
            LogAbort("no this io event type");
            break;
    }
    pthread_mutex_unlock(&comm->mutex);
}

void comm_set_io_event(int fd, int ev_type, CB_HDL *handler, void *handler_data)
{
    struct epoll_event   ev;
    struct io_event_st  *rev;
    struct io_event_st  *wev;
    struct comm_st      *comm;

    comm = &g_commtable[fd];
    rev  = comm->cn->rev;
    wev  = comm->cn->wev;

    if (IO_EVENT_READ == ev_type)
    {
        pthread_mutex_lock(&comm->mutex);
        memset(&ev, 0, g_tsize.epoll_event);
        ev.data.fd = fd;
        if (NULL == handler)
        {
            if (0 == rev->active)
            {
                pthread_mutex_unlock(&comm->mutex);
                return;
            }
            if (0 == wev->active)
            {
                ev.events = EPOLLIN|EPOLLHUP|EPOLLERR|EPOLLET;
                epoll_ctl(g_epoll.fd, EPOLL_CTL_DEL, fd, &ev);
                rev->active = 0;
                pthread_mutex_unlock(&comm->mutex);
                return;
            }
            ev.events = EPOLLOUT|EPOLLHUP|EPOLLERR|EPOLLET;
            epoll_ctl(g_epoll.fd, EPOLL_CTL_MOD, fd, &ev);
            rev->active = 0;
            pthread_mutex_unlock(&comm->mutex);
            return;
        }
        if (0 == rev->active)
        {
            ev.events = EPOLLIN|EPOLLHUP|EPOLLERR|EPOLLET;
            if (wev->active)
            {
                ev.events |= EPOLLOUT;
            }
            epoll_ctl(g_epoll.fd, EPOLL_CTL_ADD, fd, &ev);
            rev->active = 1;
        }
        rev->handler = handler;
        rev->handler_data = handler_data;
        pthread_mutex_unlock(&comm->mutex);
        return;
    }

    if (IO_EVENT_WRITE == ev_type)
    {
        pthread_mutex_lock(&comm->mutex);
        memset(&ev, 0, g_tsize.epoll_event);
        ev.data.fd = fd;
        if (NULL == handler)
        {
            if (0 == wev->active)
            {
                pthread_mutex_unlock(&comm->mutex);
                return;
            }
            if (0 == rev->active)
            {
                ev.events = EPOLLOUT|EPOLLHUP|EPOLLERR|EPOLLET;
                epoll_ctl(g_epoll.fd, EPOLL_CTL_DEL, fd, &ev);
                wev->active = 0;
                pthread_mutex_unlock(&comm->mutex);
                return;
            }
            ev.events = EPOLLIN|EPOLLHUP|EPOLLERR|EPOLLET;
            epoll_ctl(g_epoll.fd, EPOLL_CTL_MOD, fd, &ev);
            wev->active = 0;
            pthread_mutex_unlock(&comm->mutex);
            return;
        }
        if (0 == wev->active)
        {
            ev.events = EPOLLOUT|EPOLLHUP|EPOLLERR|EPOLLET;
            if (rev->active)
            {
                ev.events |= EPOLLIN;
            }
            epoll_ctl(g_epoll.fd, EPOLL_CTL_ADD, fd, &ev);
            wev->active = 1;
        }
        wev->handler = handler;
        wev->handler_data = handler_data;
        pthread_mutex_unlock(&comm->mutex);
        return;
    }
}

static void comm_remove_io_event(int fd)
{
    comm_set_io_event(fd, IO_EVENT_READ, NULL, NULL);
    comm_set_io_event(fd, IO_EVENT_WRITE, NULL, NULL);
}

void comm_select_io_event(void)
{
    int                  i;
    int                  n;
    struct epoll_event  *ev;
    struct io_event_st  *rev;
    struct io_event_st  *wev;
    struct comm_st      *comm;

    if (pthread_mutex_trylock(&g_epoll.mutex))
    {
        usleep(25000);
        return;
    }

    n = epoll_wait(g_epoll.fd, g_epoll.events, g_epoll.maxevents, g_epoll.timeout);

    if (n < 0)
    {
        LogError(1, "epoll_wait() error: %s.", xerror());
        pthread_mutex_unlock(&g_epoll.mutex);
        return;
    }

    if (n == 0)
    {
        pthread_mutex_unlock(&g_epoll.mutex);
        return;
    }

    for (i = 0; i < n; ++i)
    {
        ev   = &g_epoll.events[i];
        comm = &g_commtable[ev->data.fd];
        rev  = comm->cn->rev;
        wev  = comm->cn->wev;

        if (ev->events & EPOLLIN)
        {
            pthread_mutex_lock(&rev->mutex);
            rev->posted_ready = 1;
            pthread_mutex_unlock(&rev->mutex);
        }

        if (ev->events & EPOLLOUT)
        {
            pthread_mutex_lock(&wev->mutex);
            wev->posted_ready = 1;
            pthread_mutex_unlock(&wev->mutex);
        }

        if (ev->events & (EPOLLHUP|EPOLLERR))
        {
            pthread_mutex_lock(&rev->mutex);
            rev->posted_error = 1;
            pthread_mutex_unlock(&rev->mutex);
            pthread_mutex_lock(&wev->mutex);
            wev->posted_error = 1;
            pthread_mutex_unlock(&wev->mutex);
        }
    }

    pthread_mutex_unlock(&g_epoll.mutex);
}

void comm_process_posted_event(struct io_event_st *ev)
{
    pthread_mutex_lock(&ev->mutex);
    if (ev->active)
    {
        ev->ready          |= ev->posted_ready; 
        ev->timedout       |= ev->posted_timedout;
        ev->error          |= ev->posted_error;
        ev->posted_ready    = 0;
        ev->posted_timedout = 0;
        ev->posted_error    = 0;
    }
    pthread_mutex_unlock(&ev->mutex);
}

void comm_process_io_event(int fd)
{
    struct io_event_st  *ev;
    struct comm_st      *comm;

    comm = &g_commtable[fd];
    if (pthread_mutex_trylock(&comm->mutex))
    {
        return;
    }
    if (0 == comm->inuse)
    {
        pthread_mutex_unlock(&comm->mutex);
        return;
    }
    ev = comm->cn->rev;
    if (ev->active)
    {
        comm_process_posted_event(ev);
        ev->handler(ev->handler_data);
    }
    /* maybe closed by the last event handler. */
    if (0 == comm->inuse)
    {
        pthread_mutex_unlock(&comm->mutex);
        return;
    }
    ev = comm->cn->wev;
    if (ev->active)
    {
        comm_process_posted_event(ev);
        ev->handler(ev->handler_data);
    }
    pthread_mutex_unlock(&comm->mutex);
}

int comm_open(int fd)
{
    struct comm_st *comm;

    xassert(fd >= 0);

    comm = &g_commtable[fd];
    if (pthread_mutex_trylock(&comm->mutex))
    {
        return -1;
    }
    comm->inuse         = 1;
    comm->cn            = NULL;
    comm->close_handler = NULL;
    pthread_mutex_unlock(&comm->mutex);

    return 0;
}

void comm_ssl_close(struct comm_ssl_st *ssl)
{
    SSL_shutdown(ssl->ssl);
    ERR_free_strings();
}

static void comm_call_close_handler(struct close_handler_st *close_handler)
{
    struct close_handler_st  *ch;
    struct close_handler_st  *ch_next;

    for (ch = close_handler; NULL != ch; ch = ch_next)
    {
        ch_next = ch->next;
        ch->handler(ch->data);
        safe_process(free, ch);
    }
}

void comm_close(int fd)
{
    struct comm_st *comm;

    xassert(fd >= 0);

    comm = &g_commtable[fd];
    comm_remove_io_event(fd);
    pthread_mutex_lock(&comm->mutex);
    safe_process2(comm_ssl_close, comm->cn->ssl);
    close(fd);
    safe_process(comm_connect_unlink, comm->cn);
    safe_process(comm_call_close_handler, comm->close_handler);
    comm->inuse = 0;
    pthread_mutex_unlock(&comm->mutex);
}

int comm_connect(int fd, struct sockaddr_in *addr)
{
    int retry;

    /* return value:
     * -1: connect failed
     *  0: connection is in progressing
     *  1: connect success
     */

    for (retry = 0; retry < 3; ++retry)
    {
        if (connect(fd, (struct sockaddr *)addr, g_tsize.sockaddr_in) < 0)
        {
            if (EINPROGRESS == errno)
            {
                return 0;
            }
            if (EINTR == errno)
            {
                usleep(1);
                continue;
            }
            return -1;
        }
        return 1;
    }

    return -1;
}

int comm_ssl_connect(struct comm_connect_st *cn)
{
    int          fd;
    uint16_t     rnd;
    SSL         *ssl;
    SSL_CTX     *ctx;
    SSL_METHOD  *method;

    fd = cn->fd;
    comm_set_block(fd);
    SSL_load_error_strings();
    SSL_library_init();
    method = SSLv23_client_method();

    ctx = SSL_CTX_new(method);
    if (NULL == ctx)
    {
        LogError(1,
                 "SSL_CTX_new() error: %s.\n",
                 ERR_reason_error_string(ERR_get_error()));
        ERR_free_strings();
        return -1;
    }

    ssl = SSL_new(ctx);
    if (NULL == ssl)
    {
        LogError(1,
                 "SSL_new() error: %s.\n",
                 ERR_reason_error_string(ERR_get_error()));
        SSL_CTX_free(ctx);
        ERR_free_strings();
        return -1;
    }

    if (0 == SSL_set_fd(ssl, fd))
    {
        LogError(1,
                 "SSL_set_fd() error: %s.\n",
                 ERR_reason_error_string(ERR_get_error()));
        SSL_CTX_free(ctx);
        SSL_free(ssl);
        ERR_free_strings();
        return -1;
    }

    RAND_poll();
    while (0 == RAND_status())
    {
        rnd = rand() % 65536;
        RAND_seed(&rnd, sizeof(rnd));
    }

    /* FIXME: maybe can non-blocking here ? */
    if (SSL_connect(ssl) <= 0)
    {
        LogError(1,
                 "SSL_connect() error: %s.\n",
                 ERR_reason_error_string(ERR_get_error()));
        SSL_CTX_free(ctx);
        SSL_free(ssl);
        ERR_free_strings();
        return -1;
    }

    cn->ssl = comm_ssl_create(ssl, ctx);
    comm_set_nonblock(cn->fd);

    return 0;
}

int comm_accept(int listen_fd)
{
    int                  fd;
    int                  retry;
    struct io_event_st  *rev;

    rev = g_commtable[listen_fd].cn->rev;

    if (1 == rev->error || 0 == rev->ready)
    {
        return -1;
    }

    for (retry = 0; retry < 3; ++retry)
    {
        fd = accept(listen_fd, NULL, NULL);
        if (fd < 0)
        {
            if (EAGAIN == errno)
            {
                rev->ready = 0;
                break;
            }
            if (EINTR == errno)
            {
                usleep(1);
                continue;
            }
            rev->error = 1;
            rev->ready = 0;
            break;
        }
        break;
    }

    return fd;
}

int comm_socket(int domain, int type, int protocol)
{
    int fd;

    fd = socket(domain, type, protocol);

    if (fd < 0)
    {
        LogError(1, "socket() error: %s.", xerror());
        return -1;
    }

    return fd;
}

int comm_bind(int fd, struct sockaddr_in *addr)
{
    if (bind(fd, (const struct sockaddr *)addr, g_tsize.sockaddr_in) < 0)
    {
        LogError(1,
                 "bind(%s:%u) error: %s.",
                 inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), xerror());
        return -1;
    }

    return 0;
}

int comm_listen(int fd, int backlog)
{
    if (listen(fd, backlog) < 0)
    {
        LogError(1, "listen() error: %s.", xerror());
        return -1;
    }

    return 0;
}

ssize_t comm_recv(int fd, char *buf, size_t len)
{
    int                  retry;
    ssize_t              n;
    struct io_event_st  *rev;

    rev = g_commtable[fd].cn->rev;

    if (1 == rev->error || 1 == rev->timedout || 0 == rev->ready)
    {
        return -1;
    }

    for (retry = 0; retry < 3; ++retry)
    {
        n = recv(fd, buf, len, 0);
        if (n < 0)
        {
            if (EAGAIN == errno)
            {
                rev->ready = 0;
                break;
            }
            if (EINTR == errno)
            {
                usleep(1);
                continue;
            }
            rev->ready = 0;
            rev->error = 1;
            break;
        }
        if (n == 0)
        {
            rev->eof = 1;
            rev->ready = 0;
            break;
        }
        if (n < len)
        {
            rev->ready = 0;
            break;
        }
        break;
    }

    return n;
}

ssize_t comm_ssl_recv(int fd, char *buf, size_t len)
{
    int                  error;
    ssize_t              n;
    struct comm_ssl_st  *ssl;
    struct io_event_st  *rev;

    ssl = g_commtable[fd].cn->ssl;
    rev = g_commtable[fd].cn->rev;

    if (1 == rev->error || 1 == rev->timedout || 0 == rev->ready)
    {
        return -1;
    }

    n = SSL_read(ssl->ssl, buf, len);
    if (n <= 0)
    {
        error = SSL_get_error(ssl->ssl, n);
        switch (error)
        {
            case SSL_ERROR_NONE:
            /* go on */
            case SSL_ERROR_ZERO_RETURN:
            /* connection shutdown by peer */
            rev->eof   = 1;
            rev->ready = 0;
            break;

            case SSL_ERROR_WANT_READ:
            /* EAGAIN */
            rev->ready = 0;
            break;

            case SSL_ERROR_SYSCALL:
            if (0 == error)
            {
                break;
            }
            rev->ready = 0;
            rev->error = 1;
            break;

            default:
            rev->ready = 0;
            rev->error = 1;
            break;
        }
    }

    return n;
}

ssize_t comm_send(int fd, char *buf, size_t len)
{
    int                  retry;
    ssize_t              n;
    struct io_event_st  *wev;

    wev = g_commtable[fd].cn->wev;

    if (1 == wev->error || 1 == wev->timedout || 0 == wev->ready)
    {
        return -1;
    }

    for (retry = 0; retry < 3; ++retry)
    {
        n = send(fd, buf, len, 0);
        if (n < 0)
        {
            if (EAGAIN == errno)
            {
                wev->ready = 0;
                break;
            }
            if (EINTR == errno)
            {
                usleep(1);
                continue;
            }
            wev->ready = 0;
            wev->error = 1;
            break;
        }
        if (n == 0)
        {
            wev->ready = 0;
            break;
        }
        if (n < len)
        {
            wev->ready = 0;
            break;
        }
        break;
    }

    return n;
}

ssize_t comm_ssl_send(int fd, char *buf, size_t len)
{
    int                  error;
    ssize_t              n;
    struct comm_ssl_st  *ssl;
    struct io_event_st  *wev;

    ssl = g_commtable[fd].cn->ssl;
    wev = g_commtable[fd].cn->wev;

    if (1 == wev->error || 1 == wev->timedout || 0 == wev->ready)
    {
        return -1;
    }

    n = SSL_write(ssl->ssl, buf, len);
    if (n <= 0)
    {
        error = SSL_get_error(ssl->ssl, n);
        switch (error)
        {
            case SSL_ERROR_NONE:
            /* go on */
            case SSL_ERROR_ZERO_RETURN:
            wev->ready = 0;
            wev->error = 1;
            break;

            case SSL_ERROR_WANT_WRITE:
            wev->ready = 0;
            break;

            case SSL_ERROR_SYSCALL:
            if (0 == error)
            {
                break;
            }
            wev->ready = 0;
            wev->error = 1;
            break;

            default:
            wev->ready = 0;
            wev->error = 1;
            break;
        }
    }

    return n;
}

int comm_sendto(int fd, char * buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    return sendto(fd, buf, len, flags, to, tolen);
}

ssize_t comm_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
    int                  retry;
    ssize_t              n;
    struct io_event_st  *rev;

    rev = g_commtable[fd].cn->rev;

    if (1 == rev->error || 1 == rev->timedout || 0 == rev->ready)
    {
        return -1;
    }

    for (retry = 0; retry < 3; ++retry)
    {
        n = recvfrom(fd, buf, len, flags, from, fromlen);
        if (n < 0)
        {
            if (EAGAIN == errno)
            {
                rev->ready = 0;
                break;
            }
            if (EINTR == errno)
            {
                usleep(1);
                continue;
            }
            rev->ready = 0;
            rev->error = 1;
            break;
        }

        if (0 == n)
        {
            rev->eof = 1;
            rev->ready = 0;
            break;
        }

        if (n < len)
        {
            rev->ready = 0;
            break;
        }

        break;
    }

    return n;
}

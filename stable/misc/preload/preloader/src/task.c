#include "preloader.h"

void task_url_free(struct task_url_st *task_url)
{
    safe_process(free, task_url->id);
    safe_process(free, task_url->url);
    safe_process(free, task_url->uri);
    safe_process(free, task_url->host);
    safe_process(free, task_url);
}

struct task_url_st *task_url_clone(struct task_url_st *task_url)
{
    struct task_url_st *new_task_url;

    new_task_url           = xmalloc(g_tsize.task_url_st);
    new_task_url->id       = xstrdup(task_url->id);
    new_task_url->url      = xstrdup(task_url->url);
    new_task_url->uri      = xstrdup(task_url->uri);
    new_task_url->host     = xstrdup(task_url->host);
    new_task_url->port     = task_url->port;
    new_task_url->protocol = task_url->protocol;

    return new_task_url;
}

struct task_url_st *task_url_create(char *id, char *url)
{
    size_t               len;
    long                 port;
    char                *port_start;
    char                *host_start;
    char                *host_end;
    struct task_url_st  *task_url;

    task_url = xcalloc(1, g_tsize.task_url_st);
    task_url->id = xstrdup((char *)id);
    task_url->url = xstrdup((char *)url);

    host_start = strstr(url, "://");
    if (NULL == host_start)
    {
        task_url_free(task_url);
        return NULL;
    }

    len = host_start - url;

    if ((4 == len) && (0 == strncmp(url, "http", 4)))
    {
        task_url->protocol = PROTOCOL_HTTP;
    }
    else if ((5 == len) && (0 == strncmp(url, "https", 5)))
    {
        task_url->protocol = PROTOCOL_HTTPS;
    }
    else
    {
        task_url_free(task_url);
        return NULL;
    }

    host_start += 3;

    if ('\0' == *host_start)
    {
        task_url_free(task_url);
        return NULL;
    }

    host_end = strchr(host_start, '/');
    if (NULL == host_end)
    {
        task_url->host = xstrdup(host_start);
        task_url->uri = xstrdup("/");
    }
    else
    {
        task_url->host = xstrndup(host_start, host_end - host_start);
        task_url->uri = xstrdup(host_end);
    }

    port_start = strchr(task_url->host, ':');
    if (NULL == port_start)
    {
        task_url->port = PORT_DEFAULT_PRELOAD;
    }
    else
    {
        *port_start = '\0';
        port = strtol(port_start + 1, NULL, 10);
        if (port < 0 || port > 65535)
        {
            task_url->port = PORT_DEFAULT_PRELOAD;
        }
        task_url->port = port;
    }

    return task_url;
}

void session_free(struct session_st *session)
{
    list_destroy(session->taskurl_list);
    pthread_mutex_destroy(&session->mutex);
    safe_process(free, session->id);
    safe_process(free, session);
}

void task_submit(struct session_st *session)
{
    char *ptr;
    struct task_st *task;
    struct task_url_st *task_url;
    pthread_mutexattr_t attr;

    for ( ; ; )
    {
        task_url = list_fetch(session->taskurl_list, NULL, always_matched);
        if (NULL == task_url)
        {
            return;
        }

        task                              = xcalloc(1, g_tsize.task_st);
        task->state                       = TASK_STATE_READY;
        task->task_url                    = task_url_clone(task_url);
        task->sessionid                   = xstrdup(session->id);
        task->action                      = session->action;
        task->priority                    = session->priority;
        task->need_report                 = session->need_report;
        task->is_override                 = session->is_override;
        task->preload_addr                = session->preload_addr;
        task->report_addr                 = session->report_addr;
        task->nest_track_level            = session->nest_track_level;
        task->refresh.max_reply_body_size = SIZE_64KB;
        task->preload.limit_rate.rate     = session->limit_rate;
        task->preload.digest.type         = session->check_type ? session->check_type : CHECK_TYPE_BASIC;

        if (session->nest_track_level && g_preload.cfg.m3u8_engine_enabled)
        {
            ptr = strstr(task_url->url, ".m3u8");
            if (NULL != ptr)
            {
                task->preload.m3u8.id         = 0;
                task->preload.m3u8.enable     = 1;
                task->preload.m3u8.task       = task;
                task->preload.m3u8.url_prefix = xstrndup(task_url->url, ptr - task_url->url);
            }
        }

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&task->mutex, &attr);
        pthread_mutexattr_destroy(&attr);

        task_url_free(task_url);

        task_queue_insert(WAITING_QUEUE, task);
    }
}

void task_submit_subtask(struct task_st *task, char *url)
{
    char *ptr;
    char id[SIZE_64];
    struct task_st *subtask;
    struct task_url_st *task_url;
    pthread_mutexattr_t attr;

    task->preload.m3u8.id++;

    snprintf(id, SIZE_64, "%s-subtask%d", task->task_url->id, task->preload.m3u8.id);

    task_url = task_url_create(id, url);
    if (NULL == task_url)
    {
        return;
    }

    subtask                              = xcalloc(1, g_tsize.task_st);
    subtask->task_url                    = task_url;
    subtask->sessionid                   = xstrdup(task->sessionid);
    subtask->action                      = task->action;
    subtask->priority                    = task->priority;
    subtask->need_report                 = task->need_report;
    subtask->is_override                 = task->is_override;
    subtask->preload_addr                = task->preload_addr;
    subtask->report_addr                 = task->report_addr;
    subtask->nest_track_level            = task->nest_track_level;
    subtask->state                       = TASK_STATE_READY;
    subtask->refresh.max_reply_body_size = SIZE_64KB;
    subtask->preload.digest.type         = task->preload.digest.type;
    subtask->preload.limit_rate.rate     = task->preload.limit_rate.rate;

    if (subtask->nest_track_level && g_preload.cfg.m3u8_engine_enabled)
    {
        ptr = strstr(task_url->url, ".m3u8");
        if (NULL != ptr)
        {
            subtask->preload.m3u8.id         = 0;
            subtask->preload.m3u8.enable     = 1;
            subtask->preload.m3u8.task       = subtask;
            subtask->preload.m3u8.url_prefix = xstrdup(task->preload.m3u8.url_prefix);
        }
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&subtask->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    task_queue_insert(WAITING_QUEUE, subtask);
}

void digest_free(struct digest_st *digest)
{
    safe_process(free, digest->context);
    safe_process(free, digest->digest);
}

void task_preload_free(struct task_preload_st *preload)
{
    safe_process(free, preload->date);
    safe_process(free, preload->http_status);
    safe_process(free, preload->content_length);
    safe_process(free, preload->cache_status);
    safe_process(free, preload->last_modified);
    digest_free(&preload->digest);
    data_buf_clean(&preload->m3u8.dbuf);
}

void task_free(struct task_st *task)
{
    pthread_mutex_destroy(&task->mutex);
    task_url_free(task->task_url);
    task_preload_free(&task->preload);
    safe_process(free, task->sessionid);
    safe_process(free, task);
}

void task_queue_insert(int type, struct task_st *task)
{
    switch (type)
    {
        case WAITING_QUEUE:
        list_insert_tail(g_global.waiting_list, task);
        break;
        case RUNNING_QUEUE:
        list_insert_tail(g_global.running_list, task);
        break;
        default:
        LogAbort("no this queue type.");
        break;
    }
}

int task_url_compare(const void * key, const void * data)
{
    const struct task_st *task1 = key;
    const struct task_st *task2 = data;

    return strcmp(task1->task_url->url, task2->task_url->url);
}

void task_process_remove(void *key, CMP_HDL *cmp)
{

}

void task_queue_remove(int type, void *key, CMP_HDL *cmp)
{
    switch (type)
    {
        case WAITING_QUEUE:
        list_remove(g_global.waiting_list, key, cmp);
        break;
        case RUNNING_QUEUE:
        list_remove(g_global.running_list, key, cmp);
        break;
        default:
        LogAbort("no this queue type.");
        break;
    }
}

struct task_st *task_queue_find(int type, void *key, CMP_HDL *cmp)
{
    struct task_st *task = NULL;

    switch (type)
    {
        case WAITING_QUEUE:
        task = list_find(g_global.waiting_list, key, cmp);
        break;
        case RUNNING_QUEUE:
        task = list_find(g_global.running_list, key, cmp);
        break;
        default:
        LogAbort("no this queue type.");
        break;
    }

    return task;
}

struct task_st *task_queue_fetch(int type, void *key, CMP_HDL *cmp)
{
    struct task_st *task = NULL;

    switch (type)
    {
        case WAITING_QUEUE:
        task = list_fetch(g_global.waiting_list, key, cmp);
        break;
        case RUNNING_QUEUE:
        task = list_fetch(g_global.running_list, key, cmp);
        break;
        default:
        LogAbort("no this queue type.");
        break;
    }

    return task;
}

int task_queue_number(int type)
{
    int number = 0;

    switch (type)
    {
        case WAITING_QUEUE:
        number = list_member_number(g_global.waiting_list);
        break;
        case RUNNING_QUEUE:
        number = list_member_number(g_global.running_list);
        break;
        default:
        LogAbort("no this queue type.");
        break;
    }

    return number;
}

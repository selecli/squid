#include "preloader.h"

static void help_check_log_rotate(void)
{
    log_rotate(g_log.access);
    log_rotate(g_log.debug);
    log_rotate(g_log.error);
}

static void help_check_event_timeout(void)
{
    rbtree_process_event_timeout(g_rbtree);
}

static void help_process(void)
{
    for( ; ; )
    {
        help_check_log_rotate();
        help_check_event_timeout();
        usleep(1000000);
    }
}

static void help_set_signal(void)
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

static void *help_start_routine(void *arg)
{
    pthread_detach(pthread_self());
    help_set_signal();
    help_process();

    return NULL;
}

void help_worker_start(void)
{
    pthread_t tid;

    if (pthread_create(&tid, NULL, help_start_routine, NULL))
    {
        LogError(1, "pthread_create() error for help worker: %s.", xerror());
        exit(1);
    }
}

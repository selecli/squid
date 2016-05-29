#include "preloader.h"

static void show_version(void)
{
    fprintf(stdout,
            "\033[1m%s %s\033[0m\n\
            \rBuildTime: %s %s\n\
            \rCopyright (C) 2013 ChinaCache\n",
            APP_NAME, APP_VERSION,
            __DATE__, __TIME__);

    fflush(stdout);
}

static void show_usage(void)
{
    const char  **h;
    const char   *usage[] = {
        "Usage: preloader [OPTION] <argument>",
        "-f file        Specify the config file.",
        "               default is: /usr/local/preloader/conf/preloader.conf",
        "-F             Set the program running in the foreground.",
        "-h/--help      Show help message.",
        "-k parse|reconfigure|reload|rotate|shutdown|kill",
        "               Send signal to the running copy and exit.",
        "-v/--version   Print version.",
        NULL
    };

    for (h = usage; NULL != *h; ++h)
    {
        fprintf(stdout, "%s\n", *h);
    }

    fflush(stdout);
}

static pid_t read_pid_file(void)
{
    pid_t   pid;
    FILE   *fp;
    char   *pidstr;
    char   *saveptr;
    char    buffer[SIZE_1KB];

    pid = 0; /* pid must be inited with 0 */

    fp = xfopen2(APP_PID_FILE, "r");

    while (0 == feof(fp))
    {   
        xmemzero(buffer, SIZE_1KB);

        if (NULL == fgets(buffer, SIZE_1KB, fp))
        {
            continue;
        }

        pidstr = strtok_r(buffer, " \t\r\n", &saveptr);

        if (NULL != pidstr)
        {
            pid = (pid_t)strtol(pidstr, NULL, 10);
            break;
        }
    };  

    xfclose(fp);

    return pid;
}

static void server_reload(void)
{
    struct stat st; 

    if (stat(g_cfg_file.path, &st) < 0)
    {   
        LogError(1, "stat(%s) error: %s.", g_cfg_file.path, xerror());
        return;
    }   

    if (st.st_size == g_cfg_file.size && st.st_mtime == g_cfg_file.mtime)
    {   
        LogError(1, "preloader config file unchanged, reload needlessly.");
        return;
    }   

    g_cfg_file.size = st.st_size;
    g_cfg_file.mtime = st.st_mtime;

    LogError(1, "preloader config file changed, reload now.");
    exit(10);
}

static void server_shutdown(const char * reason)
{
    LogError(1, "preloader shutdown: %s.", reason);
    exit(0);
}

void server_exit(int status)
{
    exit(status);
}

static void do_send_signal(int signum)
{
    pid_t pid;

    if (signum < 0)
    {
        fprintf(stderr, "error: signum(%d) < 0\n", signum);
        exit(1);
    }

    pid = read_pid_file();
    if (pid <= 0)
    {
        fprintf(stderr,
                "%s (version %s): no running copy\n",
                APP_NAME, APP_VERSION);
        exit(1);
    }

    if (kill(pid, signum) < 0)
    {
        if (0 == signum)
        {
            fprintf(stderr,
                    "%s (version %s): no running copy\n",
                    APP_NAME, APP_VERSION);
        }
        else
        {
            fprintf(stderr,
                    "%s (version %s): can't send signal(%d) to process(%d): %s\n",
                    APP_NAME, APP_VERSION, signum, pid, xerror());
        }

        exit(1);
    }

    if (0 == signum)
    {
        fprintf(stdout, "%s (pid %d) is running...\n", APP_NAME, pid);
        fflush(stdout);
    }

    exit(0);
}

static void send_signal(const char * optarg)
{
    int     signum;
    size_t  length;

    if (NULL == optarg)
    {
        show_usage();
        exit(1);
    }

    length = strlen(optarg);

    if (0 == strncmp(optarg, "check", length))
    {
        /* SIGNULL: only check the process running state */
        signum = 0;
    }
    else if (0 == strncmp(optarg, "reload", length))
    {
        signum = SIGHUP;
    }
    else if (0 == strncmp(optarg, "rotate", length))
    {
        signum = SIGUSR1;
    }
    else if (0 == strncmp(optarg, "debug", length))
    {
        signum = SIGUSR2;
    }
    else if (0 == strncmp(optarg, "kill", length))
    {
        signum = SIGKILL;
    }
    else if (0 == strncmp(optarg, "shutdown", length))
    {
        signum = SIGTERM;
    }
    else
    {
        show_usage();
        exit(1);
    }

    do_send_signal(signum);

    exit(1);
}

static void parse_options(int argc, char * argv[])
{
    int             opt;
    int             index;
    const char     *optstr = ":f:Fhk:v";
    struct option   options[] = {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {NULL,      0, NULL,  0 },
    };

    do {
        opt = getopt_long(argc, argv, optstr, options, &index);	
        switch (opt)
        {
            case 'f':
            safe_process(free, g_cfg_file.path);
            g_cfg_file.path = xstrdup(optarg);
            break;

            case 'F':
            g_run_mode = RUN_FOREGROUND;
            break;

            case 'h':
            show_usage();
            exit(0);
            break;

            case 'k':
            if (0 == strcmp(optarg, "parse"))
            {
                parse_conf();
                exit(0);
            }
            send_signal(optarg);
            break;

            case 'v':
            /* print version */
            show_version();
            /* exit after print version */
            exit(0);
            break;

            case ':':
            /* 1. option needs a value
            * 2. stderr doesn't need fflush(stderr), it usually be defined as
            *    line buffered or no buffered, but all buffered 
                 */
            fprintf(stderr, "Error: option [-%c] requires an argument.\n", optopt);
            show_usage();
            exit(1);
            break;

            case '?':
            /* option unknown */
            fprintf(stderr, "Error: unknown option [-%c].\n", optopt);
            show_usage();
            exit(1);
            break;

            case -1:
            /* all command-line options parsed OK */
            break;

            default:
                fprintf(stderr, "Error: invalid option [-%c].\n", optopt);
                show_usage();
                exit(1);
                break;
        }
        } while (-1 != opt);
        }

static void check_start_up(void)
{
    int           n;
    int           fd;
    int           fd_flags;
    char          buf[SIZE_512];
    pid_t         pid;
    struct flock  lock;

    /* using the file write lock to ensure that
       only one copy can run at the same time, other must exit immediately */

    fd = xopen2(APP_PID_FILE, O_WRONLY|O_CREAT|O_APPEND, 0644);
    if (fd < 0)
    {
        /* pid file open failed, whatever, exit here. */
        exit(1);
    }

    /* setting the file write locks */

    lock.l_len     = 0;
    lock.l_start   = 0;
    lock.l_whence  = SEEK_SET;
    lock.l_type    = F_WRLCK;

    if (fcntl(fd, F_SETLK, &lock) < 0)
    {
        pid = read_pid_file();
        fprintf(stderr, "%s (pid %d) is already running.\n", APP_NAME, pid);
        exit(1);
    }   

    /* truncate the pid file to 0. */
    if (ftruncate(fd, 0) < 0)
    {
        LogError(1, "ftruncate(%s, 0) error: %s.", APP_PID_FILE, xerror());
        exit(1);
    }

    /* the file offset is not changed with ftruncate(2),
       so seek it to the beginning of the file. */
    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        LogError(1, "lseek(%s, 0, SEEK_SET) error: %s.", APP_PID_FILE, xerror());
        exit(1);
    }

    xmemzero(buf, SIZE_512);
    n = snprintf(buf, SIZE_512, "%d\n", getpid());

    if(write(fd, buf, n) != n)
    {
        LogError(1, "write(%d, %s, %d) error: %s.", fd, buf, n, xerror());
        exit(1);
    }

    /* we don't want the pid file FD inherit opening by the child process,
       so set the file descriptor flags FD_CLOEXEC here. */

    fd_flags = fcntl(fd, F_GETFD);
    if(fd_flags < 0) 
    {
        LogError(1, "fcntl(%s) error: %s.", APP_PID_FILE, xerror());
        exit(1);
    }

    /* set fd flag: close-on-exec */
    if(fcntl(fd, F_SETFD, fd_flags | FD_CLOEXEC) < 0)
    {
        LogError(1, "fcntl(%s) error: %s.", APP_PID_FILE, xerror());
        exit(1);
    }

    /* we will keep the pid file FD until process exiting,
       so close it when process closing */
       }

static void switch_to_daemon(char * argv[])
{
    int      fd;
    int      child_status; 
    int      exit_status;
    int      child_stop_count;
    time_t   child_start;
    time_t   child_stop;
    ssize_t  length;
    pid_t    pid;
    pid_t    ppid; 
    char     program[SIZE_512];

    if (RUN_FOREGROUND == g_run_mode)
    {
        /* process will running in the forground. */
        return;
    }

    if (0 == strcmp(argv[0], "preloaderd"))
    {
        /* already daemon. */
        return;
    }

    /* in order to avoid the influence of forward cache buffer data,
     * we must flush all the cache buffer of Standard I/O streams here.
    */
    fflush(NULL);

    /* fork() for deamon creating:
     * in order to ensure that the deamon process is not the current
     * process group leader(because we will call setsid() following),
     * we will fork a new process here and then change it to a deamon process.
    */
    pid = fork(); 

    if(pid < 0)  
    {    
        /* fork() error, exit now. */
        LogError(1, "fork() error: %s.", xerror());
        exit(1);
    }

    if(pid > 0)
    {
        /* parent process exit now. */
        exit(0);
    }

    /* child process, backgroud work. */

    fd = open("/dev/null", O_RDWR);

    if (fd < 0)
    {
        /* fd < 0: 
         * Here, we can't redirect STDXXX_FILENO to fd("/dev/null")
         * So, close the standard I/O stream: stdin, stdout, stderr
         * Because we don't want to print any character
        */
        fclose(stdin);
        fclose(stdout);
        fclose(stderr);

        LogError(1, "open(\"/dev/null\") error: %s.", xerror());
    }
    else
    {
        /* redirect stdin, stdout, stderr to "/dev/null"
         * Unix/Linux programs usually use 0, 1, 2 instead of
         * STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO
         * Be careful to use 0, 1, 2, use STDXXX_FILENO
        */
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);

        if (STDIN_FILENO != fd && STDOUT_FILENO != fd && STDERR_FILENO != fd)
        {
            /* close any fd, but STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO */
            close(fd);
        }
    }

    /* set file mode creation mask to 0 */
    umask(0);

    /* change working directory to "/" */
    chdir("/");

    /* create a new session for deamon
     * Who called setsid() must not the process group leader
     * setsid() will make the process from current session and terminations.
     * Then it will be a real deamon process.
     */
     if (setsid() < 0)
     {
         LogError(1, "setsid() error: %s.", xerror());
         exit(1);
     }

    /* all the above operation was successful,
     * the new process is running in background now.
     * we must watch the master process by its parent, and then
     * we can deal with some of the common anomaly.
     */

    child_stop_count = 0;

    do {
        pid = fork(); 

        if(pid < 0)  
        {    
            LogError(1, "fork() error: %s.", xerror());
            exit(1);
        }

        if (0 == pid)
        {
            /* the child process: 
             * get the program's absolute path (portability ??).
            */
            length = readlink("/proc/self/exe", program, SIZE_512);

            if (length < 0)
            {
                LogError(1, "readlink() error: %s.", xerror());
                exit(1);
            }

            program[length] = '\0';
            argv[0] = xstrdup("preloaderd");
            /* replace current process image with a new process image by execvp(). */
            execvp(program, argv);
            LogError(1, "execvp(%s, %s) error: %s.", program, argv[0], xerror());
            exit(1);
        }

        /* pid > 0
         * the parent process,
         * watching the child process, and dealing with the common anomaly.
        */

        ppid = getpid();

        LogDebug(1, "Parent(%d): watching at child process(%d).", ppid, pid);

        time(&child_start);

        pid = waitpid(-1, &child_status, 0);

        time(&child_stop);

        if (WIFEXITED(child_status))
        {
            /* child process terminated by exit() or _exit(). */
            exit_status = WEXITSTATUS(child_status);

            if (10 == exit_status)
            {
                /* for reloading */
                usleep(500000);
                continue;
            }

            LogError(1,
                     "Parent(%d): caught child(%d) exited with status(%d).",
                     ppid, pid, exit_status);

            if (0 == exit_status)
            {
                /* terminated normally */
                exit(0);
            }
        }
        else if (WIFSIGNALED(child_status))
        {
            /* child process terminated by a signal. */
            exit_status = WTERMSIG(child_status);

            LogError(1,
                     "Parent(%d): caught child(%d) exited with signal(%d).",
                     ppid, pid, exit_status);	

            switch (exit_status)
            {
                case SIGKILL:
                exit(0);
                case SIGINT:
                case SIGTERM:
                exit(1);
                default:
                break;
            }
        }
        else
        {
            LogError(1,
                     "Parent(%d): caught child(%d) exited with unknown reasons.",
                     ppid, pid);
        }

        if (child_stop - child_start < 5)
        {
            child_stop_count++;
        }
        else
        {
            /* reset the counter. */
            child_stop_count = 0;
        }

        if (child_stop_count > 60)
        {
            LogError(1, "Parent(%d): child exited too rapidly, exit now.");
            /* if the child process exited more than 3 times in 5 seconds,
               parent will give up start it, and self exit immediately. */
            exit(1);
        }

        usleep(500000);

    } while (1);
}

static void do_signal_debug(int signum)
{
}

static void do_signal_rotate(int signum)
{
}

static void do_signal_reload(int signum)
{
    server_reload();
}

static void do_signal_shutdown(int signum)
{
    char reason[SIZE_128];

    snprintf(reason, SIZE_128, "shutdown by signal(%d)", signum);
    server_shutdown(reason);
    exit(0);
}

static int set_signal_action(int signum, SIG_HDL * handler, int flags)
{
    struct sigaction act;

    act.sa_flags   = flags;
    act.sa_handler = handler;

    sigemptyset(&act.sa_mask);

    if (sigaction(signum, &act, NULL) < 0)
    {
        LogError(1, "sigaction(%d) error: %s.", signum, xerror());	
        return -1;
    }

    return 0;
}

static void set_signal_behavior(void)
{
    /* flags(SA_XXX):
    *   SA_RESTART:   provide behaviour compatible with BSD signal semantics
     *                 by making certain systerm calls restartable accross signals.
     *   SA_NODEFER:   don't prevent the signal from being received from within its own handler.
     *   SA_RESETHAND: restore the signal action to the default state once
     *                 the signal handler has been called.
    */

    set_signal_action(SIGUSR1, do_signal_rotate,   SA_RESTART);
    set_signal_action(SIGUSR2, do_signal_debug,    SA_RESTART);
    set_signal_action(SIGHUP,  do_signal_reload,   SA_RESTART);
    set_signal_action(SIGINT,  do_signal_shutdown, SA_RESTART|SA_NODEFER|SA_RESETHAND);
    set_signal_action(SIGTERM, do_signal_shutdown, SA_RESTART|SA_NODEFER|SA_RESETHAND);
}

static void set_max_open_file_number(void)
{
    struct rlimit rlim;

    /* set maximum file descriptor number that can be opened.
    if we want to change the hard limit, try to run as the super user(Root). */

    if (getrlimit(RLIMIT_NOFILE, &rlim) < 0)
    {
        LogError(1, "getrlimit(RLIMIT_NOFILE) error: %s.", xerror());
        exit(1);
    }
    if (rlim.rlim_cur == MAX_OPEN_FD)
    {
        return;
    }
    if (rlim.rlim_max < MAX_OPEN_FD)
    {
        rlim.rlim_max = MAX_OPEN_FD;
    }
    rlim.rlim_cur = MAX_OPEN_FD;
    if (setrlimit(RLIMIT_NOFILE, &rlim) < 0)
    {
        LogError(1, "setrlimit(RLIMIT_NOFILE) error: %s.", xerror());
        exit(1);
    }
}

static void set_resource_limits(void)
{
    set_max_open_file_number();
}

static void type_size_init(void)
{
    g_tsize.pointer          = sizeof(void *);
    g_tsize.log_st           = sizeof(struct log_st);
    g_tsize.array_st         = sizeof(struct array_st);
    g_tsize.array_item_st    = sizeof(struct array_item_st);
    g_tsize.in_addr          = sizeof(struct in_addr);
    g_tsize.sockaddr_in      = sizeof(struct sockaddr_in);
    g_tsize.epoll_event      = sizeof(struct epoll_event);
    g_tsize.io_event_st      = sizeof(struct io_event_st);
    g_tsize.directive_st     = sizeof(struct directive_st);
    g_tsize.config_addr_st   = sizeof(struct config_addr_st);
    g_tsize.hash_link_st     = sizeof(struct hash_link_st);
    g_tsize.hash_table_st    = sizeof(struct hash_table_st);
    g_tsize.list_st          = sizeof(struct list_st);
    g_tsize.list_node_st     = sizeof(struct list_node_st);
    g_tsize.rbtree_st        = sizeof(struct rbtree_st);
    g_tsize.rbtree_node_st   = sizeof(struct rbtree_node_st);
    g_tsize.session_st       = sizeof(struct session_st);
    g_tsize.task_st          = sizeof(struct task_st);
    g_tsize.task_url_st      = sizeof(struct task_url_st);
    g_tsize.comm_st          = sizeof(struct comm_st);
    g_tsize.comm_data_st     = sizeof(struct comm_data_st);
    g_tsize.comm_ssl_st      = sizeof(struct comm_ssl_st);
    g_tsize.comm_connect_st  = sizeof(struct comm_connect_st);
    g_tsize.comm_accept_st   = sizeof(struct comm_accept_st);
    g_tsize.comm_listen_st   = sizeof(struct comm_listen_st);
    g_tsize.comm_preload_st  = sizeof(struct comm_preload_st);
    g_tsize.comm_dns_st      = sizeof(struct comm_dns_st);
    g_tsize.comm_ip_st       = sizeof(struct comm_ip_st);
    g_tsize.http_st          = sizeof(struct http_st);
    g_tsize.http_header_st   = sizeof(struct http_header_st);
    g_tsize.http_request_st  = sizeof(struct http_request_st);
    g_tsize.http_reply_st    = sizeof(struct http_reply_st);
    g_tsize.data_buf_st      = sizeof(struct data_buf_st);
    g_tsize.close_handler_st = sizeof(struct close_handler_st);
    g_tsize.md5_context_st   = sizeof(struct md5_context_st);
    g_tsize.dns_query_st     = sizeof(struct dns_query_st);
}

static void log_file_open(void)
{
    log_open(g_log.access);
    log_open(g_log.debug);
    log_open(g_log.error);
}

static void global_commtable_create(void)
{
    int                  i;
    struct comm_st      *comm;
    pthread_mutexattr_t  mutexattr;

    g_commtable = xmalloc(MAX_OPEN_FD * g_tsize.comm_st);

    for (i = 0; i < MAX_OPEN_FD; ++i)
    {
        comm = &g_commtable[i];
        comm->inuse = 0;
        comm->cn = NULL;
        comm->close_handler = NULL;
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&comm->mutex, &mutexattr);
        pthread_mutexattr_destroy(&mutexattr);
    }
}

static void global_epoll_create(void)
{
    g_epoll.timeout   = 25;
    g_epoll.maxevents = MAX_OPEN_FD;
    g_epoll.fd        = epoll_create(MAX_OPEN_FD);
    g_epoll.events    = xcalloc(MAX_OPEN_FD, g_tsize.epoll_event);
    pthread_mutex_init(&g_epoll.mutex, NULL);
    global_commtable_create();
}

static void global_log_init(void)
{
    g_log.access = log_create(LOG_ACCESS_FILE, 1, 5, 204800000);
    g_log.debug  = log_create(LOG_DEBUG_FILE, 1, 5, 204800000);
    g_log.error  = log_create(LOG_ERROR_FILE, 1, 5, 204800000);
}

static void global_config_init(void)
{
    struct stat st; 

    g_cfg_file.path = xstrdup(APP_CFG_FILE);
    if (stat(g_cfg_file.path, &st) < 0)
    {   
        g_cfg_file.size  = 0;
        g_cfg_file.mtime = 0;
    }   
    else
    {
        g_cfg_file.size  = st.st_size;
        g_cfg_file.mtime = st.st_mtime;
    }

    g_global.cfg.sched_policy                 = SCHED_POLICY_FIFO;
    g_global.cfg.task_limit_number            = 1024;
    g_global.cfg.task_concurrency             = 5;
    g_global.cfg.http.send_timeout            = 60;
    g_global.cfg.http.recv_timeout            = 60;
    g_global.cfg.http.connect_timeout         = 120;
    g_global.cfg.http.keepalive_timeout       = 600;
    g_global.cfg.http.max_request_header_size = 20480;
    g_global.cfg.http.max_request_body_size   = 1024000;
    g_global.cfg.access_log                   = log_create(LOG_ACCESS_FILE, 1, 5, 204800000);
    g_global.cfg.debug_log                    = log_create(LOG_DEBUG_FILE, 1, 5, 204800000);
    g_global.cfg.error_log                    = log_create(LOG_ERROR_FILE, 1, 5, 204800000);
    g_global.waiting_list                     = list_create();
    g_global.running_list                     = list_create();

    g_listen.cfg.addr.sin_family = AF_INET;
    g_listen.cfg.addr.sin_port = htons(31108);
    if (inet_pton(AF_INET, "0.0.0.0", (void *)&g_listen.cfg.addr.sin_addr) < 0)
    {
        fprintf(stderr, "inet_pton(0.0.0.0) error: %s\n", xerror());
        exit(1);
    }

    g_preload.cfg.worker_number       = 5;
    g_preload.cfg.m3u8_engine_enabled = 1;
    g_preload.cfg.addr.sin_port       = htons(80);
    if (inet_pton(AF_INET, "127.0.0.1", (void *)&g_preload.cfg.addr.sin_addr) < 0)
    {
        fprintf(stderr, "inet_pton(127.0.0.1) error: %s\n", xerror());
        exit(1);
    }

    g_grab.grab.start        = 0;
    g_grab.grab.last_time    = time(NULL);
    g_grab.grab.state        = GRAB_STATE_READY;
    g_grab.grab.cn           = NULL;
    g_grab.cfg.interval_time = 360;
    g_grab.cfg.addr.type     = SOCKADDR_IP;
    g_grab.cfg.addr.sockaddr.ip.sin_family = AF_INET;
    g_grab.cfg.addr.sockaddr.ip.sin_port = htons(80);
    if (inet_pton(AF_INET, "255.255.255.255", (void *)&g_grab.cfg.addr.sockaddr.ip.sin_addr) < 0)
    {
        fprintf(stderr, "inet_pton(255.255.255.255) error: %s\n", xerror());
        exit(1);
    }
}

static void global_variable_init(void)
{
    global_log_init();
    global_config_init();
    g_run_mode = RUN_BACKGROUND;
    g_rbtree   = rbtree_create((void *)rbtree_insert_event_timer);
}

static void master_running(void)
{
    sigset_t sigset;

    listen_worker_start();
    preload_worker_start();
    grab_worker_start();
    help_worker_start();

    for ( ; ; )
    {
        /* process suspend until signals comming in. */
        sigemptyset(&sigset);
        sigsuspend(&sigset);
    }
}

static void main_init(void)
{
    type_size_init();
    global_variable_init();
    log_file_open();
}

static void main_parse(int argc, char *argv[])
{
    parse_options(argc, argv);
    parse_conf();
}

static void main_start(char *argv[])
{
    check_start_up();
    switch_to_daemon(argv);
    set_resource_limits();
    set_signal_behavior();
    global_epoll_create();
    master_running();
}

int main(int argc, char *argv[])
{
    /******************* preloader *******************
    **************** Preload Server *****************
    *************************************************/

    main_init();
    main_parse(argc, argv);
    main_start(argv);

    /***************** Program Exit ******************
     ************ exit(x) better than return x *******
     ************ 0 - normal; !0 - abnormal **********
     *************************************************/

    exit(0);
}

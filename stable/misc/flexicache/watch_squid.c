#include "flexicache.h"

static int squidWatchSendMsg(int fd, void *data, int nbytes)
{
	struct msghdr msg;
	struct iovec vec[1];
	union
	{
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	}cmsg;
	int sendfd = 0;
	
	msg.msg_control 	= (caddr_t)&cmsg;
	msg.msg_controllen 	= sizeof(cmsg);

	cmsg.cm.cmsg_len 	= CMSG_LEN(sizeof(int));
	cmsg.cm.cmsg_level 	= SOL_SOCKET;
	cmsg.cm.cmsg_type	= SCM_RIGHTS;

	*((int *)CMSG_DATA(&cmsg.cm)) = sendfd;
	
	vec[0].iov_base = data;
	vec[0].iov_len 	= nbytes;
	msg.msg_iov 	= vec;
	msg.msg_iovlen 	= 1;
	
	msg.msg_name 	= NULL;
	msg.msg_namelen = 0;

	return sendmsg(fd, &msg, 0);
}

static int squidWatchReceiveMsg(int fd, void *data, int nbytes)
{
	struct msghdr msg;
	struct iovec vec[1];
	union
	{
		struct cmsghdr cm;	
		char control[CMSG_SPACE(sizeof(int))];
	}cmsg;

	msg.msg_control = (caddr_t)&cmsg;
	msg.msg_controllen = sizeof(cmsg);
	
	vec[0].iov_base = data;
	vec[0].iov_len  = nbytes;
	msg.msg_iov     = vec;
	msg.msg_iovlen  = 1;
	
    msg.msg_name    = NULL;
	msg.msg_namelen = 0;
	
	if (recvmsg(fd, &msg, 0) <= 0)
	{
		debug(0, "cacheWatchReceiveMsg failed! fd: %d\n", fd);
		return -1;
	}

	if (cmsg.cm.cmsg_len == CMSG_LEN(sizeof(int)))
	{
		if (cmsg.cm.cmsg_level != SOL_SOCKET || cmsg.cm.cmsg_type != SCM_RIGHTS)
		{
			debug(0, "cacheWatchReceiveMsg failed! fd: %d\n", fd);
			return -1;
		}

		return *((int *)CMSG_DATA(&cmsg.cm));
	}
	else
	{
		debug(0, "cacheWatchReceiveMsg failed! fd: %d\n", fd);
		return -1;
	}
}

static int squidWatchOpen()
{
    int fd;
    int retry = 0;
    
    while (retry < CACHE_WATCH_FAIL_RETRY)
    {
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
	    if (-1 == fd)
	    {
		    debug(0, "%s call socket failed! error: %s\n", __FUNCTION__, strerror(errno));
            retry++;
            continue;
	    }
        
        break;
    }

    return fd;	
}

static int squidWatchEventCreate()
{
    return epoll_create(CACHE_WATCH_EVENTS);
}

static int squidWatchEventAdd(int efd, int fd)
{
    struct epoll_event ev;

	ev.data.fd  = fd;
    ev.events   = EPOLLOUT;
   
    return epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
}

static int squidWatchConnect(int efd, char *server_name)
{
	struct sockaddr_un addr;
	int fd;
	int retry = 0;

    fd = squidWatchOpen();
    if (-1 == fd)	
    {
		debug(0, "%s failed! error: %s\n", __FUNCTION__, strerror(errno));
        return -1;
    }
    
    squidWatchEventAdd(efd, fd);

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, server_name);
	
    while (1)
	{
		if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		{
			if (ECONNREFUSED == errno || ENOENT == errno)
			{
				debug(0, "%s failed! error: %s\n", __FUNCTION__, strerror(errno));
				return -1;
			}
			else if (retry < CACHE_WATCH_FAIL_RETRY)
			{
				retry++;
				sleep(1);

				debug(0, "%s reconnect %d, fd: %d\n", __FUNCTION__, retry, fd);
				continue;
			}

			return -1;
		}

		return fd;
	}
}

static int squidWatchEventSend(int efd, struct epoll_event *ev)
{
    if (squidWatchSendMsg(ev->data.fd, CACHE_WATCH_SEND_MSG, CACHE_WATCH_SEND_MSG_LENGTH) == -1)
    {
        debug(0, "send msg failed! %s\n", strerror(errno));
        return -1;
    }
    else
    {
        debug(1, "squid watch %d send msg successfully\n", getpid()); 
        
        ev->events = EPOLLIN;
        epoll_ctl(efd, EPOLL_CTL_MOD, ev->data.fd, ev);

        return 0;
    }
}

static int squidWatchEventRecv(int efd, struct epoll_event *ev)
{
    char msg[CACHE_WATCH_RECV_MSG_LENGTH];
    
    if (squidWatchReceiveMsg(ev->data.fd, msg, CACHE_WATCH_RECV_MSG_LENGTH) != -1)
    {
        debug(0, "squid watch %d receive msg: %s, msg length: %d\n", getpid(), msg, CACHE_WATCH_RECV_MSG_LENGTH);

        epoll_ctl(efd, EPOLL_CTL_DEL, ev->data.fd, ev);
        close(ev->data.fd);

        return 0;
    }
    else
    {
        return -1;
    }
}

static int squidWatchEventDispatch(int efd)
{
    struct epoll_event events[CACHE_WATCH_EVENTS];
    int i, num;
 
    while (1)
    {
        num = epoll_wait(efd, events, CACHE_WATCH_EVENTS, g_config.watch_timeout);
        
        if (num < 0)
        {
            debug(0, "epoll_wait failed! %s\n", strerror(errno));
            return -1;
        }
        else if (0 == num)
        {
            debug(0, "epoll_wait timeout!\n");
            return -1;
        } 
       
        for (i = 0; i < num; i++)
        {
            if (events[i].events & EPOLLOUT)
            {
                if (squidWatchEventSend(efd, &events[i]) == -1)
                {
                    return -1;
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                return squidWatchEventRecv(efd, &events[i]);
            }
        }
    }
}

static void squidWatchCreate(int id)
{
    int efd, sockfd;
	char watch_squid[256];

	snprintf(watch_squid, 256, "/tmp/xiaosi%d", id);
	debug(1, "%s arg: %s\n", __FUNCTION__, watch_squid);

    efd = squidWatchEventCreate();
    if (-1 == efd)
    {
        return;
    }
    
    while (1)
    {
        sockfd = squidWatchConnect(efd, watch_squid);
        if (-1 == sockfd)
        {
            break;
        }
        
        if (squidWatchEventDispatch(efd) == 0)
        {
            sleep(g_config.watch_interval);
        }
        else
        {
            close(sockfd);
			//fatalf("The squid %d responds slowly! Response Timeout: %dms\n", id, g_config.watch_timeout);
			debug(0,"The squid %d responds slowly! Response Timeout: %dms\n", id, g_config.watch_timeout);
        }
    }	
}

static int squidWatchSetProcName(int id)
{
    /* The length indicate the range of watch_name is squid_watch1 to squid_watch99. */
	char watch_name[14];
   
    snprintf(watch_name, 14, "squid_watch%d", id);
    strncpy(g_watch_argv[0], watch_name, 14);
    
    return 0;
}

static void squidWatchProcessStart(int id)
{
	int nullfd = open("/dev/null", O_RDWR, 644);

	dup2(nullfd, 0);
	dup2(nullfd, 1);
	dup2(nullfd, 2);

    squidWatchSetProcName(id);
    squidWatchCreate(id);
}

static int squidWatchStart()
{
	pid_t pid;
	int i;

    for (i = 0; i < g_config.cache_processes; i++)
	{
		if ((pid = fork()) < 0)
		{
			fatalf("error when starting server watch! error: %s\n", strerror(errno));
		}
		else if (0 == pid)
		{
			squidWatchProcessStart(i + 1);
			exit(1);
		}
        else
        {
            g_watch_manager.pids[i] = pid;
            g_watch_manager.num_pids++;
        }
	}

    return 0;
}

static int squidWatchTerminate()
{
    int i;
    int watch_procs = g_watch_manager.num_pids;

    for (i = 0; i < watch_procs; i++)
    {
        if (kill(g_watch_manager.pids[i], SIGTERM) < 0)
        {
            debug(1, "Killing squid watch %d failed! %s\n", g_watch_manager.pids[i], strerror(errno));
            
            return -1;
        }
        else
        {
            g_cache_manager.num_pids--;
            debug(1, "Killed squid watch %d\n", g_watch_manager.pids[i]);
        }   
    }

    return 0;
}

static int squidWatchCheckRestart(pid_t pid)
{
    pid_t new_pid;
    int i;
    int watch_procs = g_watch_manager.num_pids;
    
    for (i = 0; i < watch_procs; i++)
    {
        if (pid == g_watch_manager.pids[i])
        {
            debug(1, "restart squid watch %d\n", i + 1);
            if ((new_pid = fork()) < 0)
            {
                g_watch_manager.num_pids--;
                fatalf("restart squid watch %d failed, %s\n", i + 1, strerror(errno));
            }
            else if (0 == new_pid)
            {
			    squidWatchProcessStart(i + 1);
			    exit(1);
            }
            else
            {
                g_watch_manager.pids[i] = new_pid;
            }            
        }
    }

    return 0;
}

void squidWatchInit(WatchManager *w)
{
	w->starter          = squidWatchStart;
	w->terminator       = squidWatchTerminate;
    w->checkRestarter   = squidWatchCheckRestart;

	debug(1, "squid Watch init ok, type is squid\n");
}

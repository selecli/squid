
/*
 * $Id: squidclient.c,v 1.9.2.1 2008/06/04 20:32:50 hno Exp $
 *
 * DEBUG: section 0     WWW Client
 * AUTHOR: Harvest Derived
 *
 * SQUID Web Proxy Cache          http://www.squid-cache.org/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from
 *  the Internet community; see the CONTRIBUTORS file for full
 *  details.   Many organizations have provided support for Squid's
 *  development; see the SPONSORS file for full details.  Squid is
 *  Copyrighted (C) 2001 by the Regents of the University of
 *  California; see the COPYRIGHT file for full details.  Squid
 *  incorporates software developed and/or copyrighted by other
 *  sources; see the CREDITS file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

#include "config.h"

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef _SQUID_WIN32_
#include <io.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_NETDB_H && !defined(_SQUID_NETDB_H_)	/* protect NEXTSTEP */
#define _SQUID_NETDB_H_
#include <netdb.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <sys/un.h>

#include "util.h"

#ifndef BUFSIZ
#define BUFSIZ 8192
#endif

#ifndef PRId64
#ifdef _SQUID_MSWIN_		/* Windows native port using MSVCRT */
#define PRId64 "I64d"
#elif SIZEOF_INT64_T > SIZEOF_LONG
#define PRId64 "lld"
#else
#define PRId64 "ld"
#endif
#endif

#if SIZEOF_INT64_T > SIZEOF_LONG && HAVE_STRTOLL
typedef int64_t squid_off_t;
#define SIZEOF_SQUID_OFF_T SIZEOF_INT64_T
#define PRINTF_OFF_T PRId64
#define strto_off_t (int64_t)strtoll
#else
typedef long squid_off_t;
#define SIZEOF_SQUID_OFF_T SIZEOF_LONG
#define PRINTF_OFF_T "ld"
#define strto_off_t strtol
#endif

#define CACHE_SELECTOR_HOSTNAME ("localhost")
static int g_cache_selector_port;

typedef void SIGHDLR(int sig);

/* Local functions */
static int client_comm_bind(int, const char *);
static int client_comm_connect(int, const char *, u_short, struct timeval *);
static void usage(const char *progname);
static int Now(struct timeval *);
static SIGHDLR catchSignal;
static SIGHDLR pipe_handler;
static void set_our_signal(void);
static int clientConnect(const char *hostname, const char *localhost, int port, int ping, int opt_verbose, struct timeval *tv);
static int clientConnectCacheByID(int id, int ping, int opt_verbose, struct timeval *tv);
static ssize_t myread(int fd, void *buf, size_t len);
static ssize_t mywrite(int fd, void *buf, size_t len);
static int put_fd;
static char *put_file = NULL;
static struct stat sb;
int total_bytes = 0;
int io_timeout = 120;

#ifdef _SQUID_MSWIN_
void
Win32SockCleanup(void)
{
    WSACleanup();
    return;
}
#endif /* ifdef _SQUID_MSWIN_ */

static void
usage(const char *progname)
{
    fprintf(stderr,
        "Usage: %s [-arsv] [-i IMS] [-h remote host] [-l local host] [-p port] [-m method] [-t count] [-I ping-interval] [-H 'strings'] [-T timeout] [-j 'hostheader'] url\n"
        "Options:\n"
        "    -P file      PUT request.\n"
        "    -a           Do NOT include Accept: header.\n"
        "    -r           Force cache to reload URL.\n"
        "    -s           Silent.  Do not print data to stdout.\n"
        "    -v           Verbose. Print outgoing message to stderr.\n"
        "    -i IMS       If-Modified-Since time (in Epoch seconds).\n"
        "    -h host      Retrieve URL from cache on hostname.  Default is localhost.\n"
        "    -l host      Specify a local IP address to bind to.  Default is none.\n"
        "    -j hosthdr   Host header content\n"
        "    -p port      Port number of cache.  Default is %d.\n"
        "    -c id        ID number of cache.  Only for Multiple Squid.  Default is 1.\n"
        "    -m method    Request method, default is GET.\n"
        "    -t count     Trace count cache-hops\n"
        "    -g count     Ping mode, \"count\" iterations (0 to loop until interrupted).\n"
        "    -I interval  Ping interval in seconds (default 1 second).\n"
        "    -H 'string'  Extra headers to send. Use '\\n' for new lines.\n"
        "    -T timeout   Timeout value (seconds) for read/write operations.\n"
        "    -u user      Proxy authentication username\n"
        "    -w password  Proxy authentication password\n"
        "    -U user      WWW authentication username\n"
        "    -W password  WWW authentication password\n"
        "    -V version   HTTP Version\n",
	    progname, CACHE_HTTP_PORT);

    exit(1);
}

static int interrupted = 0;

int
main(int argc, char *argv[])
{
    int conn, c, len, bytesWritten;
    int port, to_stdout, reload;
    int cache_id = 0;
    int ping, pcount;
    int keep_alive = 0;
    int opt_noaccept = 0;
    int opt_verbose = 0;
    const char *hostname, *localhost;
    char url[BUFSIZ], msg[BUFSIZ], buf[BUFSIZ];
    char extra_hdrs[BUFSIZ];
    const char *method = "GET";
    extern char *optarg;
    time_t ims = 0;
    int max_forwards = -1;
    struct timeval tv1, tv2;
    int i = 0, loops;
    long ping_int;
    long ping_min = 0, ping_max = 0, ping_sum = 0, ping_mean = 0;
    const char *proxy_user = NULL;
    const char *proxy_password = NULL;
    const char *www_user = NULL;
    const char *www_password = NULL;
    const char *host = NULL;
    const char *version = "1.1";

    /* set the defaults */
    hostname = "localhost";
    localhost = NULL;
    extra_hdrs[0] = '\0';
    port = CACHE_HTTP_PORT;
    to_stdout = 1;
    reload = 0;
    ping = 0;
    pcount = 0;
    ping_int = 1 * 1000;

    if (argc < 2) {
	    usage(argv[0]);		/* need URL */
    } else if (argc >= 2) {
	    strncpy(url, argv[argc - 1], BUFSIZ);
	    url[BUFSIZ - 1] = '\0';
	    if (url[0] == '-')
	        usage(argv[0]);

        while ((c = getopt(argc, argv, "ah:j:V:l:P:i:km:p:rsvt:g:p:c:I:H:T:u:U:w:W:?")) != -1) {
            switch (c) {
                case 'a':
                opt_noaccept = 1;
                break;
                case 'h':		/* remote host */
                hostname = optarg;
                break;
                case 'j':
                host = optarg;
                break;
                case 'V':
                version = optarg;
                break;
                case 'l':		/* local host */
                localhost = optarg;
                break;
                case 's':		/* silent */
                to_stdout = 0;
                break;
                case 'k':		/* backward compat */
                keep_alive = 1;
                break;
                case 'r':		/* reload */
                reload = 1;
                break;
                case 'p':		/* port number */
                sscanf(optarg, "%d", &port);
                if (port < 1)
                    port = CACHE_HTTP_PORT;	/* default */
                break;
                case 'c':		/* ID number */
                sscanf(optarg, "%d", &cache_id);
                if (cache_id < 1)
                    cache_id = 1;	/* default */
                break;
                case 'P':
                put_file = xstrdup(optarg);
                break;
                case 'i':		/* IMS */
                ims = (time_t) atoi(optarg);
                break;
                case 'm':
                method = xstrdup(optarg);
                break;
                case 't':
                method = xstrdup("TRACE");
                max_forwards = atoi(optarg);
                break;
                case 'g':
                ping = 1;
                pcount = atoi(optarg);
                to_stdout = 0;
                break;
                case 'I':
                if ((ping_int = atoi(optarg) * 1000) <= 0)
                    usage(argv[0]);
                break;
                case 'H':
                if (strlen(optarg)) {
                    char *t;
                    strncpy(extra_hdrs, optarg, sizeof(extra_hdrs));
                    while ((t = strstr(extra_hdrs, "\\n")))
                    *t = '\r', *(t + 1) = '\n';
                }
                break;
                case 'T':
                io_timeout = atoi(optarg);
                break;
                case 'u':
                proxy_user = optarg;
                break;
                case 'w':
                proxy_password = optarg;
                break;
                case 'U':
                www_user = optarg;
                break;
                case 'W':
                www_password = optarg;
                break;
                case 'v':
                /* undocumented: may increase verb-level by giving more -v's */
                opt_verbose++;
                break;
                case '?':		/* usage */
                default:
                usage(argv[0]);
                break;
            }
        }
    }
#ifdef _SQUID_MSWIN_
    {
        WSADATA wsaData;
        WSAStartup(2, &wsaData);
        atexit(Win32SockCleanup);
    }
#endif
    /* Build the HTTP request */
    if (strncmp(url, "mgr:", 4) == 0) {
        char *t = xstrdup(url + 4);
        snprintf(url, BUFSIZ, "cache_object://%s/%s", hostname, t);
        xfree(t);
    }
    if (put_file) {
        put_fd = open(put_file, O_RDONLY);
        set_our_signal();
        if (put_fd < 0) {
            fprintf(stderr, "%s: can't open file (%s)\n", argv[0],
            xstrerror());
            exit(-1);
	    }
#ifdef _SQUID_WIN32_
	    setmode(put_fd, O_BINARY);
#endif
	    fstat(put_fd, &sb);
    }
    if (!host) {
        char *newhost = strstr(url, "://");
        if (newhost) {
            char *t;
            newhost += 3;
            newhost = strdup(newhost);
            t = newhost + strcspn(newhost, "@/?");
            if (*t == '@') {
                newhost = t + 1;
                t = newhost + strcspn(newhost, "@/?");
            }
            *t = '\0';
            host = newhost;
        }
    }
    if (host)
	    snprintf(msg, BUFSIZ, "%s %s HTTP/%s\r\nHost: %s\r\n", method, url, version, host);
    else
	    snprintf(msg, BUFSIZ, "%s %s HTTP/%s\r\n", method, url, version);

    if (reload) {
        snprintf(buf, BUFSIZ, "Pragma: no-cache\r\n");
        strcat(msg, buf);
    }
    if (put_fd > 0) {
        snprintf(buf, BUFSIZ, "Content-length: %d\r\n", (int) sb.st_size);
        strcat(msg, buf);
    }
    if (opt_noaccept == 0) {
        snprintf(buf, BUFSIZ, "Accept: */*\r\n");
        strcat(msg, buf);
    }
    if (ims) {
        snprintf(buf, BUFSIZ, "If-Modified-Since: %s\r\n", mkrfc1123(ims));
        strcat(msg, buf);
    }
    if (max_forwards > -1) {
        snprintf(buf, BUFSIZ, "Max-Forwards: %d\r\n", max_forwards);
        strcat(msg, buf);
    }
    if (proxy_user) {
        const char *user = proxy_user;
        const char *password = proxy_password;
#if HAVE_GETPASS
        if (!password)
            password = getpass("Proxy password: ");
#endif
        if (!password) {
            fprintf(stderr, "ERROR: Proxy password missing\n");
            exit(1);
        }
        snprintf(buf, BUFSIZ, "%s:%s", user, password);
        snprintf(buf, BUFSIZ, "Proxy-Authorization: Basic %s\r\n", base64_encode(buf));
        strcat(msg, buf);
    }
    if (www_user) {
        const char *user = www_user;
        const char *password = www_password;
    #if HAVE_GETPASS
        if (!password)
            password = getpass("WWW password: ");
    #endif
        if (!password) {
            fprintf(stderr, "ERROR: WWW password missing\n");
            exit(1);
        }
        snprintf(buf, BUFSIZ, "%s:%s", user, password);
        snprintf(buf, BUFSIZ, "Authorization: Basic %s\r\n", base64_encode(buf));
        strcat(msg, buf);
    }
    if (strcmp(version, "1.0") == 0) {
        if (keep_alive) {
            if (strstr(url, "://"))
                strcat(msg, "Proxy-Connection: keep-alive\r\n");
            else
                strcat(msg, "Connection: keep-alive\r\n");
        }
    } else {
	    if (!keep_alive)
	        strcat(msg, "Connection: close\r\n");
    }

    strcat(msg, extra_hdrs);
    strcat(msg, "\r\n");

    if (opt_verbose)
	    fprintf(stderr, "headers: '%s'\n", msg);

    if (ping) {
#if HAVE_SIGACTION
        struct sigaction sa, osa;
        if (sigaction(SIGINT, NULL, &osa) == 0 && osa.sa_handler == SIG_DFL) {
            sa.sa_handler = catchSignal;
            sa.sa_flags = 0;
            sigemptyset(&sa.sa_mask);
            (void) sigaction(SIGINT, &sa, NULL);
        }
#else
        void (*osig) ();
        if ((osig = signal(SIGINT, catchSignal)) != SIG_DFL)
            (void) signal(SIGINT, osig);
#endif
    }

    loops = ping ? pcount : 1;
    for (i = 0; loops == 0 || i < loops; i++) {
        squid_off_t fsize = 0;

        if (cache_id > 0) {
            conn = clientConnectCacheByID(cache_id, ping, opt_verbose, &tv1);
        } else {
            conn = clientConnect(hostname, localhost, port, ping, opt_verbose, &tv1); 
        }
    
        /* Send the HTTP request */
    	bytesWritten = mywrite(conn, msg, strlen(msg));
        if (bytesWritten < 0) {
            perror("client: ERROR: write");
            exit(1);
        } else if (bytesWritten != strlen(msg)) {
            fprintf(stderr, "client: ERROR: Cannot send request?: %s\n", msg);
            exit(1);
        }

        if (put_file) {
            int x;
            lseek(put_fd, 0, SEEK_SET);
#ifdef _SQUID_MSWIN_
    	    while ((x = read(put_fd, buf, sizeof(buf))) > 0) {
#else
            while ((x = myread(put_fd, buf, sizeof(buf))) > 0) {
#endif
                x = mywrite(conn, buf, x);
                total_bytes += x;
                if (x <= 0)
                    break;
            }
            if (x != 0)
            fprintf(stderr, "client: ERROR: Cannot send file.\n");
        }// end if (put_file)

        /* Read the data */

#ifdef _SQUID_MSWIN_
	    setmode(1, O_BINARY);
#endif
	    while ((len = myread(conn, buf, sizeof(buf))) > 0) {
	        fsize += len;
	        if (to_stdout) {
		        fwrite(buf, len, 1, stdout);
            }
	    }

#ifdef _SQUID_MSWIN_
        setmode(1, O_TEXT);
#endif
	    (void) close(conn);	/* done with socket */

	    if (interrupted)
	        break;

	    if (ping || opt_verbose) {
            struct tm *tmp;
            time_t t2s;
            long elapsed_msec;

            (void) Now(&tv2);
            elapsed_msec = tvSubMsec(tv1, tv2);
            t2s = tv2.tv_sec;
            tmp = localtime(&t2s);
            fprintf(stderr, "%d-%02d-%02d %02d:%02d:%02d [%d]: %ld.%03ld secs, %f KB/s (%" PRINTF_OFF_T "KB)\n",
                tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday,
                tmp->tm_hour, tmp->tm_min, tmp->tm_sec, i + 1,
                elapsed_msec / 1000, elapsed_msec % 1000,
                elapsed_msec ? (double) fsize / elapsed_msec * 1000 / 1024 : -1.0,
                (fsize + 1023) / 1024);

            if (i == 0 || elapsed_msec < ping_min)
                ping_min = elapsed_msec;
            if (i == 0 || elapsed_msec > ping_max)
                ping_max = elapsed_msec;
            ping_sum += elapsed_msec;

            /* Delay until next "ping_int" boundary */
            if (ping && (loops == 0 || i + 1 < loops) && elapsed_msec < ping_int) {
                struct timeval tvs;
                long msec_left = ping_int - elapsed_msec;

                tvs.tv_sec = msec_left / 1000;
                tvs.tv_usec = (msec_left % 1000) * 1000;
                select(0, NULL, NULL, NULL, &tvs);
            }
	    }
    }// end for (i = 0; loops == 0 || i < loops; i++)

    if (ping && i) {
	    ping_mean = ping_sum / i;
	    fprintf(stderr, "%d requests, round-trip (secs) min/avg/max = "
	        "%ld.%03ld/%ld.%03ld/%ld.%03ld\n", i,
	        ping_min / 1000, ping_min % 1000, ping_mean / 1000, ping_mean % 1000,
	        ping_max / 1000, ping_max % 1000);
    }

    exit(0);
    /*NOTREACHED */
    return 0;
}// end main

static int
client_comm_bind(int sock, const char *local_host)
{
    static const struct hostent *hp = NULL;
    struct sockaddr_in from_addr;

    /* Set up the source socket address from which to send. */
    if (NULL == hp) {
        if ((hp = gethostbyname(local_host)) == 0) {
            return (-1);
	    }
    }

    from_addr.sin_family = AF_INET;
    from_addr.sin_port = 0;
    xmemcpy(&from_addr.sin_addr, hp->h_addr, hp->h_length);
    
    return bind(sock, (struct sockaddr *) &from_addr, sizeof(struct sockaddr_in));
}

static int
client_comm_connect(int sock, const char *dest_host, u_short dest_port, struct timeval *tvp)
{
    static const struct hostent *hp = NULL;
    struct sockaddr_in to_addr;

    /* Set up the destination socket address for message to send to. */
    if (NULL == hp) {
        if ((hp = gethostbyname(dest_host)) == 0) {
            return (-1);
        }
    }

    to_addr.sin_family = AF_INET;
    to_addr.sin_port = htons(dest_port);
    xmemcpy(&to_addr.sin_addr, hp->h_addr, hp->h_length);

    if (tvp)
	    (void) Now(tvp);
    
    return connect(sock, (struct sockaddr *) &to_addr, sizeof(struct sockaddr_in));
}

static int
Now(struct timeval *tp)
{
#if GETTIMEOFDAY_NO_TZP
    return gettimeofday(tp);
#else
    return gettimeofday(tp, NULL);
#endif
}				/* ARGSUSED */

static void
catchSignal(int sig)
{
    interrupted = 1;
    fprintf(stderr, "Interrupted.\n");
}

static void
pipe_handler(int sig)
{
    fprintf(stderr, "SIGPIPE received.\n");
}

static void
set_our_signal(void)
{
#if HAVE_SIGACTION
    struct sigaction sa;
    sa.sa_handler = pipe_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
	    fprintf(stderr, "Cannot set PIPE signal.\n");
	    exit(-1);
    }
#else
    signal(SIGPIPE, pipe_handler);
#endif

}

static ssize_t
myread(int fd, void *buf, size_t len)
{
#ifndef _SQUID_MSWIN_
    alarm(io_timeout);
    return read(fd, buf, len);
#else
    return recv(fd, buf, len, 0);
#endif
}

static ssize_t
mywrite(int fd, void *buf, size_t len)
{
#ifndef _SQUID_MSWIN_
    alarm(io_timeout);
    return write(fd, buf, len);
#else
    return send(fd, buf, len, 0);
#endif
}

static int
clientConnect(const char *hostname, const char *localhost, int port, int ping, int opt_verbose, struct timeval *tv)
{
    int conn = -1;

    /* Connect to the server */
    if ((conn = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("client: socket");
        exit(1);
    }

    if (localhost && client_comm_bind(conn, localhost) < 0) {
        perror("client: bind");
        exit(1);
    }

    if (client_comm_connect(conn, hostname, port, (ping || opt_verbose) ? tv : NULL) < 0) {
        if (0 == errno) {
            fprintf(stderr, "client: ERROR: Cannot connect to %s:%d: Host unknown.\n", hostname, port);
        } else {
            char tbuf[BUFSIZ];
            snprintf(tbuf, BUFSIZ, "client: ERROR: Cannot connect to %s:%d", hostname, port);
            perror(tbuf);
        }

        exit(1);
    }

    return conn;
}

static int 
clientSelectorSendFD(int fd, int sendfd)
{
	struct msghdr msg;
	struct iovec vec[1];
	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	}cmsg;
    char data[1];

    if (sendfd < 0) {
        return -1;
    }
	
	msg.msg_control 	= (caddr_t)&cmsg;
	msg.msg_controllen 	= sizeof(cmsg);

	cmsg.cm.cmsg_len 	= CMSG_LEN(sizeof(int));
	cmsg.cm.cmsg_level 	= SOL_SOCKET;
	cmsg.cm.cmsg_type	= SCM_RIGHTS;

	*((int *)CMSG_DATA(&cmsg.cm)) = sendfd;
	
	vec[0].iov_base = data;
	vec[0].iov_len 	= 1;
	msg.msg_iov 	= vec;
	msg.msg_iovlen 	= 1;
	
	msg.msg_name 	= NULL;
	msg.msg_namelen = 0;

	return sendmsg(fd, &msg, 0);
}

static int 
clientSelectorSelectCacheByID(int id)
{
    int fd;
    struct sockaddr_un addr;
    char path[256];

    snprintf(path, 256, "/tmp/xiaosi%d", id);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == fd) {
        perror("cacheSelect socket");
        return -1;
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("cacheSelect connect");
        return -1;
    }

    return fd;
}

static int 
clientSelectorStart(int *id)
{
    struct sockaddr_in selector_addr, clientaddr;
    static const struct hostent *hp = NULL;
    int listenfd, sendfd, cachefd;
    int cache_id = *id;
    int addr_size;
    int ret = 0;

    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (-1 == listenfd) {
        perror("clientSelectorStart socket");
        return -1;
    }

    bzero(&selector_addr, sizeof(selector_addr));
    selector_addr.sin_family    = AF_INET;
    selector_addr.sin_port      = 0;

    if (NULL == hp) {
        hp = gethostbyname(CACHE_SELECTOR_HOSTNAME);
        if (NULL == hp) {
            perror("clientSelectorStart get host");
            close(listenfd);
            
            return -1;
        }
    }

    memcpy(&selector_addr.sin_addr, hp->h_addr, hp->h_length);
 
    if (bind(listenfd, (struct sockaddr *)&selector_addr, sizeof(selector_addr)) == -1) {
        perror("clientSelectorStart bind");
        close(listenfd);
        
        return -1;
    }
    
    if (listen(listenfd, 128) == -1) {
        perror("clientSelectorStart listen");
        close(listenfd);
        
        return -1;
    }

    addr_size = sizeof(struct sockaddr_in);
    
    getsockname(listenfd, (struct sockaddr *)&selector_addr, &addr_size);
    g_cache_selector_port = ntohs(selector_addr.sin_port);

    while (1) {
        sendfd = accept(listenfd, (struct sockaddr *)(&clientaddr), &addr_size);
        if (-1 == sendfd) {
            perror("clientSelectorStart accept");
            ret = -1;
            break;
        }

        cachefd = clientSelectorSelectCacheByID(cache_id); 
        if (cachefd < 0) {
            ret = -1;
            break;
        }
        
        if (clientSelectorSendFD(cachefd, sendfd) == -1) {
            perror("clientSelectorStart sendmsg");
            ret = -1;
        }

        break; 
    }

    close(listenfd);
    close(cachefd);
    close(sendfd);

    return ret;
}

static int 
clientConnectCacheByID(int id, int ping, int opt_verbose, struct timeval *tv)
{
    pthread_t tid;
    void *ret;
    int conn;
    int retry = 3;

    if (pthread_create(&tid, NULL, clientSelectorStart, &id) != 0) {
        return -1;
    }

    while (retry--) {
        usleep(100);

        if (0 < g_cache_selector_port) {
            conn = clientConnect(CACHE_SELECTOR_HOSTNAME, NULL, g_cache_selector_port, ping, opt_verbose, tv);
            if (conn < 0) {
                fprintf(stderr, "client connect cache %d error!\n", id);
            }        
            
            g_cache_selector_port = 0;

            break;
        }
    }

    if (pthread_join(tid, &ret) != 0) {
        return -1;
    }

    if (-1 == (int)ret) {
        return -1;
    }

    return conn;
}

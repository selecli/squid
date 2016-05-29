#include "cc_framework_api.h"

#ifdef CC_FRAMEWORK
#ifdef CC_MULTI_CORE

#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

static cc_module* mod = NULL;
void connStateFree(int fd, void *data);
void requestTimeout(int fd, void *data);
void clientReadRequest(int fd, void *data);
int clientReadDefer(int fd, void *data);
int httpAcceptDefer(int fd, void *dataunused);

#ifdef TCP_NODELAY
static void
commSetTcpNoDelay(int fd) 
{
	if (0 == fd_table[fd].flags.nodelay)
	{   
		int on = 1;
		if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &on, sizeof(on)) < 0)
			debug(191, 1) ("commSetTcpNoDelay: multisquid_lscs: FD %d: %s\n", fd, xstrerror());
		fd_table[fd].flags.nodelay = 1;
	}   
}
#endif

static void mod_commSetReuseAddr(int fd)
{
        int on = 1; 
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
                debug(191, 5) ("mod_commSetReuseAddr: FD %d: %s\n", fd, xstrerror());
}

static int
comm_open_un(int sock_type,
		  int proto,
		  char *path,
		  int flags,
		  const char *note)
{
	int new_socket;

	/* Create socket for accepting new connections. */
	statCounter.syscalls.sock.sockets++;
	if ((new_socket = socket(AF_UNIX, sock_type, proto)) < 0)
	{
		/* Increase the number of reserved fd's if calls to socket()
		 * are failing because the open file table is full.  This
		 * limits the number of simultaneous clients */
		switch (errno)
		{
		case ENFILE:
		case EMFILE:
			debug(191, 3) ("comm_open: socket failure: %s\n", xstrerror());
			fdAdjustReserved();
			break;
		default:
			debug(191, 0) ("comm_open: socket failure: %s\n", xstrerror());
		}
		return -1;
	}
	debug(191, 5) ("comm_open_un: FD %d is a new UNIX socket\n", new_socket);
	fde *F = NULL;
	fd_open(new_socket, FD_SOCKET, note);
	F = &fd_table[new_socket];
	//F->local_addr = addr;
	//F->tos = tos;
	if (!(flags & COMM_NOCLOEXEC))
		commSetCloseOnExec(new_socket);
	if ((flags & COMM_REUSEADDR))
		mod_commSetReuseAddr(new_socket);
	struct sockaddr_un S;

	memset(&S, '\0', sizeof(S));
	S.sun_family = AF_UNIX;
	strcpy(S.sun_path, path);
	statCounter.syscalls.sock.binds++;
	if (bind(new_socket, (struct sockaddr *) &S, sizeof(S)) != 0)
	{
		debug(191, 5) ("commBind: Cannot bind socket FD %d to %s: %s\n",
				new_socket,
				path,
				xstrerror());

		fatal("comm_open_un can not bind socket\n");
	}

	//F->local_port = port;

	if (flags & COMM_NONBLOCKING)
		if (commSetNonBlocking(new_socket) == COMM_ERROR)
			return -1;

	return new_socket;


}

static int
comm_accept_un(int fd)
{
	int sock;
	struct sockaddr_un P;
	socklen_t Slen;
	Slen = sizeof(P);
	statCounter.syscalls.sock.accepts++;
	if ((sock = accept(fd, (struct sockaddr *) &P, &Slen)) < 0)
	{
		if (ignoreErrno(errno) || errno == ECONNREFUSED || errno == ECONNABORTED)
		{
			debug(191, 5) ("comm_accept_un: FD %d: %s\n", fd, xstrerror());
			return COMM_NOMESSAGE;
		}
		else if (ENFILE == errno || EMFILE == errno)
		{
			debug(191, 3) ("comm_accept_un: FD %d: %s\n", fd, xstrerror());
			return COMM_ERROR;
		}
		else
		{
			debug(191, 1) ("comm_accept_un: FD %d: %s\n", fd, xstrerror());
			return COMM_ERROR;
		}
	}
	commSetCloseOnExec(sock);
	/* fdstat update */
	fd_open(sock, FD_SOCKET, "LSCS Request");
	commSetNonBlocking(sock);
	return sock;
}

#define CACHE_WATCH_RECV_MSG            "watch_squid_request"
#define CACHE_WATCH_RECV_MSG_LENGTH      sizeof(CACHE_WATCH_RECV_MSG)
#define CACHE_WATCH_SEND_MSG            "watch_squid_response"
#define CACHE_WATCH_SEND_MSG_LENGTH      sizeof(CACHE_WATCH_SEND_MSG)

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

static int squidWatchReply(int fd, struct msghdr *msg, char *buf)
{
    if (strcmp(buf, CACHE_WATCH_RECV_MSG))
    {
        return -1;
    }
    
    return squidWatchSendMsg(fd, CACHE_WATCH_SEND_MSG, CACHE_WATCH_SEND_MSG_LENGTH);
}

static void clientRecvFd(int sock, void *data)
{
	int retval;     
	struct msghdr msg;
	struct iovec vec;       
	char cmsgbuf[CMSG_SPACE(sizeof(int))];
	struct cmsghdr *p_cmsg;
	char iov_buf[CACHE_WATCH_RECV_MSG_LENGTH];

	vec.iov_base = iov_buf;
	vec.iov_len = CACHE_WATCH_RECV_MSG_LENGTH;
	msg.msg_name = NULL;    
	msg.msg_namelen = 0;
	msg.msg_iov = &vec; 
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);   
	msg.msg_flags = 0;      /* In case something goes wrong, set the fd to -1 before the syscall */
	retval = recvmsg(sock, &msg, 0);

	if( retval <= 0 )
	{       
		comm_close(sock);
		return; 
	}       

	if( (p_cmsg = CMSG_FIRSTHDR(&msg)) == NULL )
	{       
		comm_close(sock);
		return; 
	}       

    if (squidWatchReply(sock, &msg, iov_buf) != -1)
    {
        comm_close(sock);
        return;
    }

	int fd = *((int*)CMSG_DATA(p_cmsg));
	// for keep alive
	commSetSelect(sock, COMM_SELECT_READ, clientRecvFd, NULL, 0);
	// omm_close(sock);
	if(fd < 0)
	{
		return;
	}
	//comm_accept's jobs
	struct sockaddr_in peername;
	struct sockaddr_in sockname;
	socklen_t socklen;
	socklen = sizeof(struct sockaddr_in);
	memset(&peername, '\0', socklen);
	memset(&sockname, '\0', socklen);
	getpeername(fd, (struct sockaddr *)&peername, &socklen);
	getsockname(fd, (struct sockaddr *)&sockname, &socklen);
	commSetCloseOnExec(fd);
	fd_open(fd, FD_SOCKET, "HTTP Request");
	fde *F = &fd_table[fd];
	xstrncpy(F->ipaddr, xinet_ntoa(peername.sin_addr), 16);
	F->remote_port = htons(peername.sin_port);
	F->local_port = htons(sockname.sin_port);
	commSetNonBlocking(fd);
	//rest of httpAccept's jobs
    //
    //int on = 1;
    //if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &on, sizeof(on)) < 0)
    //    debug(191, 0) ("commSetTcpNoDelay: FD %d: %s\n", fd, xstrerror());
    //fd_table[fd].flags.nodelay = 1;
#ifdef TCP_NODELAY
	commSetTcpNoDelay(fd); /*Fix tcp use Negale bug while send packet*/
#endif

#ifdef CC_FRAMEWORK
	cc_call_hook_func_private_http_accept(fd);
#endif

	debug(191, 4) ("clientRecvFd: FD %d: accepted port %d client %s:%d\n", fd, F->local_port, F->ipaddr, F->remote_port);
	fd_note_static(fd, "client http connect");
	ConnStateData * connState = cbdataAlloc(ConnStateData);
	assert(Config.Sockaddr.http);
	connState->port = Config.Sockaddr.http;
	cbdataLock(connState->port);
	connState->peer = peername; 
	connState->log_addr = peername.sin_addr;
	connState->log_addr.s_addr &= Config.Addrs.client_netmask.s_addr;
	connState->me = sockname;
	connState->fd = fd;
	connState->pinning.fd = -1;
	connState->in.buf = memAllocBuf(CLIENT_REQ_BUF_SZ, &connState->in.size);
	comm_add_close_handler(fd, connStateFree, connState);
	if (Config.onoff.log_fqdn)
		fqdncache_gethostbyaddr(peername.sin_addr, FQDN_LOOKUP_IF_MISS);
	commSetTimeout(fd, Config.Timeout.request, requestTimeout, connState);
#if USE_IDENT
	static aclCheck_t identChecklist;
	identChecklist.src_addr = peername.sin_addr;
	identChecklist.my_addr = sockname.sin_addr;
	identChecklist.my_port = ntohs(sockname.sin_port);
	if (aclCheckFast(Config.accessList.identLookup, &identChecklist))
		identStart(&sockname, &peername, clientIdentDone, connState);
#endif
	commSetSelect(fd, COMM_SELECT_READ, clientReadRequest, connState, 0);
	commSetDefer(fd, clientReadDefer, connState);
	if (connState->port->tcp_keepalive.enabled)
	{       
		commSetTcpKeepalive(fd, connState->port->tcp_keepalive.idle, connState->port->tcp_keepalive.interval, connState->port->tcp_keepalive.timeout);
	}       

	clientdbEstablished(peername.sin_addr, 1);
	incoming_sockets_accepted++;
}

static void lscsAccept(int sock, void *data)
{
        int fd = -1;
//        fde *F;
        ConnStateData *connState = NULL;
        int max = INCOMING_HTTP_MAX;
        commSetSelect(sock, COMM_SELECT_READ, lscsAccept, data, 0);
        while (max-- && !httpAcceptDefer(sock, NULL))
        {
                if ((fd = comm_accept_un(sock)) < 0)
                {
                        if (!ignoreErrno(errno))
                                debug(191, 1) ("lscsAccept: FD %d: accept failure: %s\n",
                                                          sock, xstrerror());
                        break;
                }



                commSetSelect(fd, COMM_SELECT_READ, clientRecvFd, connState, 0);
        }
}

void clientLscsConnectionsOpen(void)
{
        int fd; 
        enter_suid();
        struct sockaddr_un addr_UNIX;
        addr_UNIX.sun_family = AF_LOCAL;
        char path[4096];
        snprintf(path, 4096, "/tmp/xiaosi%d", opt_squid_id);
        strcpy(addr_UNIX.sun_path,path);
        unlink(path);
        fd = comm_open_un(SOCK_STREAM,
                        PF_LOCAL,
                        path,   
                        COMM_NONBLOCKING,
                        "LSCS Socket");
        leave_suid();
        comm_listen(fd);
        commSetSelect(fd, COMM_SELECT_READ, lscsAccept, NULL, 0);
        /*      
         * We need to set a defer handler here so that we don't
         * peg the CPU with select() when we hit the FD limit.
         */
        commSetDefer(fd, httpAcceptDefer, NULL);
        debug(191, 1) ("Accepting LSCS connections at %s, FD %d.\n",
                        path,   
                        fd);    
}

// module init 
int mod_register(cc_module *module)
{
	debug(191, 1)("(mod_multisquid_lscs) ->  init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	
	//module->hook_func_clientLscsConnectionsOpen= clientLscsConnectionsOpen;
	cc_register_hook_handler(HPIDX_hook_func_clientLscsConnectionsOpen,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_clientLscsConnectionsOpen), 
			clientLscsConnectionsOpen);
	
	mod = module;
	return 0;
}
#endif
#endif

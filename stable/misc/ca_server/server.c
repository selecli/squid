//
#include "server_header.h"

//static char *config_file = "./server.conf";
static char *config_file = "/usr/local/squid/etc/ca_server.conf";
static int daemon_mode = 0;
int listen_fd;
time_t now;
fd_struct *fd_table;
bool server_shutdown = false;
static bool server_reconfig = false;
static bool server_log_rotate = false;
static bool server_sig_child  = false;
static char *version = "1.0";
int FD_MAX = 1024*100;

extern void backtrace_info();

extern uint64_t cc_malloc_count ;
extern uint64_t cc_malloc_times ;
extern uint64_t cc_free_times ;

extern void    
httpHeaderAddEntry(HttpHeader * hdr, HttpHeaderEntry * e);

static void usage(void)
{
	printf("unknown args\n");
}

void sig_chld()
{
        pid_t   pid;
        int     stat;

        while((pid = waitpid(-1, &stat, WNOHANG)) > 0);
}

static void signals_handle(int sig)
{
        switch(sig) {
                case SIGSEGV:
                        sig_log(sig);
                        backtrace_info();
                        exit(-1);
                        server_shutdown = true;
                case SIGABRT:
                        signal(SIGABRT, SIG_DFL);
                        sig_log(sig);
                        backtrace_info();
                        exit(-1);
                        server_shutdown = true;
                case SIGINT:
                case SIGTERM:
                case SIGHUP:
                        sig_log(sig);
                        backtrace_info();
                        server_shutdown = true;
                        break;
                case SIGUSR1:
                        server_reconfig = true;
                        break;
                case SIGCHLD:
			server_sig_child = true;
		case SIGQUIT:
			server_log_rotate = true;
                        break;
                default:
                        sig_log(sig);
                        backtrace_info();

                        break;
        }
}

static void signal_init(void)
{
        if( (signal(SIGPIPE, signals_handle) == SIG_ERR) ||
                (signal(SIGINT, signals_handle) == SIG_ERR) ||
                (signal(SIGTERM, signals_handle) == SIG_ERR) ||
                (signal(SIGUSR1, signals_handle) == SIG_ERR) ||
                (signal(SIGUSR2, signals_handle) == SIG_ERR) ||
                (signal(SIGCHLD, signals_handle) == SIG_ERR) ||
                (signal(SIGQUIT, signals_handle) == SIG_ERR) ||
                (signal(SIGSEGV, signals_handle) == SIG_ERR) ||
                (signal(SIGABRT, signals_handle) == SIG_ERR) ||
                (signal(SIGHUP, signals_handle) == SIG_ERR) ) {
                cclog(0,"set signal handler failed!");
        }
}

static int options_send_cmd(const char *cmd){
        if( cmd == NULL || strlen(optarg) == 0 ) {
                usage();
                return -1;
        }

        pid_t pid = get_running_pid();
        if(pid <= 0) {
                fprintf(stderr, "no server is running\n");
                return -2;
        }

        int sig = 0;

        if(strcasecmp(cmd,"shutdown") == 0){
                sig = SIGHUP;
        }
        else if(strcasecmp(cmd,"reconfig") == 0){
                sig = SIGUSR1;
        }
        else if(strcasecmp(cmd,"log_rotate") == 0){
                sig = SIGQUIT;
        }
        else{
                usage();
                return -1;
        }
	cclog(0,"send %s command to server(%d)",cmd,pid);

        if(kill(pid, sig) != 0){
                cclog(0,"send command failed!!");
        }

        return 0;
}

static void options_parse(int argc, char **argv)
{
        int c;
        while(-1 != (c = getopt(argc, argv, "hvdf:k:"))) {
                switch(c) {
                        case 'f':
                                if(optarg){
                                        config_file = optarg;
                                }
                                break;
                        case 'k':
                                options_send_cmd(optarg);
                                exit(1);
                        case 'd':
                                daemon_mode = 1;
                                break;
                        case 'h':
                                usage();
                                exit(-1);
                        case 'v':
                                printf("server version V%s\n",version);
                                exit(-1);
                        default:
                                fprintf(stderr, "Unknown arg: %c\n", c);
                                usage();
                                exit(-1);
                }
        }
}

static void listen_start(int port)
{
        int m_socket;
        if( (m_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
                cclog(0,"socket fail: %s\n", strerror(errno));
                server_shutdown = true;
                return;
        }

        struct  sockaddr_in     server_address;
        memset(&(server_address), 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);

        int option = 1;
        if( setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0 ) {
                cclog(0,"setsockopt fail: %s\n", strerror(errno));
                server_shutdown = true;
                return;
        }

        int bind_times = 10;
	while( bind(m_socket,(struct sockaddr*)&server_address, sizeof(struct sockaddr)) < 0 ) {
		cclog(0,"bind fail. retry: %s\n", strerror(errno));
		sleep(1);
		if( server_shutdown )
			return;
		if((bind_times--) == 0){
			server_shutdown = true;
			return;
		}
	}

	if( listen(m_socket, 10) < 0 ) {
		cclog(0,"listen fail:  %s\n", strerror(errno));
		server_shutdown = true;
		return;
	}

	server_epoll_add(m_socket,EPOLLIN | EPOLLHUP | EPOLLERR);
        fd_close_onexec(m_socket);
        listen_fd = m_socket;
}

int fd_init(int fd){
        if(fd > FD_MAX){
                cclog(0,"fatal ! fd(%d) > FD_MAX(%d) !!",fd,FD_MAX);
                assert(fd < FD_MAX);
        }

        fd_struct *fd_s = &fd_table[fd];
        if(fd_s->inuse){
                cclog(1,"fd in use : fd(%d),ip(%s),type(%d)",fd,get_peer_ip(fd),fd_s->type);
                goto OUT;
        }

	memset(fd_s, 0, sizeof(fd_struct));

        fd_s->inuse = 1;
        fd_s->setup_time = now;
        fd_s->fd = fd;
        fd_s->type = UNKNOW;
	fd_s->conn_status = CONN_DEF;

        assert(fd_s->read_buff == NULL);
        assert(fd_s->write_buff == NULL);

        fd_s->read_buff = cc_malloc(BUFFER_SIZE);
//        fd_s->write_buff = cc_malloc(BUFFER_SIZE);
	fd_s->read_buff_size = BUFFER_SIZE;

	fd_s->header.data = fd_s;

        cclog(3,"fd new connect : fd(%d),ip(%s),type(%d)",fd,get_peer_ip(fd),fd_s->type);
OUT:
        return OK;
}

static void handle_accept(int listen_fd)
{
        struct  sockaddr_in     client_addr;
        socklen_t len = sizeof(client_addr);
        int client_socket = 0;

        if( (client_socket = accept(listen_fd, (struct sockaddr*)&client_addr, &len)) < 0 ) {
                cclog(0,"accept fail: %s", strerror(errno));
                return;
        }

//????        cclog(3,"accept ok!client ip = %s", get_peer_ip(client_socket));

        int opts;
        if( (opts = fcntl(client_socket, F_GETFL)) < 0 ) {
                goto failed;
        }

        opts = opts|O_NONBLOCK|FD_CLOEXEC;

        if( fcntl(client_socket, F_SETFL, opts) < 0 ) {
                goto failed;
        }

        int ret = server_epoll_add(client_socket,EPOLLIN | EPOLLHUP | EPOLLERR);
        if(ret != 0){
                goto failed;
        }

	fd_init(client_socket);

	fd_struct *fd_s = &fd_table[client_socket];

	fd_s->type = FC_2_S;
	fd_s->data = cc_malloc(sizeof(session_fds));
	fd_s->conn_status = CONN_OK;

	((session_fds *)fd_s->data)->fc_2_s_fd = client_socket;


        return;
failed:
        cclog(0,"unresolved socket : %s\n",get_peer_ip(client_socket));
        close(client_socket);
}

// close_type: 1, close always; 2, close options...
void disconnect_fd(int fd, int close_type){
	if(fd < 0)
	{
		cclog(3, "in disconnect_fd func, fd is < 0");
		return;
	}

	fd_struct *fd_s = &fd_table[fd];

	if(!fd_s->inuse)
	{
		cclog(3, "in disconnect_fd func, fd_s is not inuse");
		return;
	}

	fd_struct *tmp_s;
	session_fds *fds = fd_s->data;
	uint32_t tmp_fd;  
	int i;

	assert(fds != NULL);

	if((fd_s->type == S_2_WEB) && (fd_s->close_delay == 1))
	{
		server_epoll_remove(fd);
		if(close_type == 0)
		{
			cclog(3, "peer close fd: %d, but fd type is S_2_WEB, so we close all fd later\n", fd);
			return;
		}
	}

	if((fd_s->type == S_2_CA) && (fd_s->close_delay == 1))
	{
		server_epoll_remove(fd);
		if(close_type == 0)
		{
			cclog(3, "ca close fd: %d", fd);
			return;
		}
	}


	for(i = 0; i < 3; i++)
	{
		tmp_fd = ((uint32_t *)fds)[i];

		if(tmp_fd > 0)
		{
			tmp_s = &fd_table[tmp_fd];
			if(tmp_s->inuse)
			{
				if(tmp_s->read_buff)
				{
					cc_free(tmp_s->read_buff);
					tmp_s->read_buff = NULL;
				}

				if(tmp_s->write_buff)
				{
					//	cc_free(tmp_s->write_buff);
					tmp_s->write_buff = NULL;
				}

				if(tmp_s->epoll_flag)
				{
					if(server_epoll_remove(tmp_fd) != 0)
						cclog(3, "epoll remove failed: %s", strerror(errno));
				}

				free_http_header(&tmp_s->header);
				memset(&tmp_s->header, 0, sizeof(HttpHeader));

			//	memset(tmp_s,0,sizeof(fd_struct));
				close(tmp_fd);

				memset(tmp_s,0,sizeof(fd_struct));
				cclog(3, "close fd: %d", tmp_fd);
			}
		}
	}

	cc_free(fds);
	fds = NULL;

	/*
	//set all members to zero
	memset(fd_s,0,sizeof(fd_struct));

	server_epoll_remove(fd);
	close(fd);
	 */
}

const char *xstrerror(void)
{
        static char xstrerror_buf[BUFSIZ];
        const char *errmsg;
                        
        errmsg = strerror(errno);
        
        if (!errmsg || !*errmsg)
                errmsg = "Unknown error";

        snprintf(xstrerror_buf, BUFSIZ, "(%d) %s", errno, errmsg);
        return xstrerror_buf;
}       

static int connect_to_s(int fd_in)
{
	char *ip;
	unsigned short port;
//	struct in_addr addr;
	char buf[64];

        struct sockaddr_in  maddress;
        memset(&maddress, 0, sizeof(struct sockaddr_in));
	
	memset(buf, 0, sizeof(buf));

	fd_struct *fd_s = &fd_table[fd_in];
	fd_struct *fd_s2;

	session_fds *fds = fd_s->data;


        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if( fd < 0 ) {
                return -1;
        }

	if(fd_s->type == FC_2_S)
	{
		//fd is FC_2_S, so we connect to web server
		strcpy(buf, (char *)strBuf(fd_s->header.ip));
		
		char *tmp;
		char *_port;

		if((tmp = strstr(buf, ":")) == NULL)
		{
			cclog(2, "not find : in request host ip:port header");
			close(fd);
			return -1;
		}
		ip = buf;
		_port = tmp + 1;

		*tmp = '\0';

		if(inet_aton(ip,&maddress.sin_addr) == 0){
			cclog(2,"ip -> %s is invalid!!(%s), connect to web server failed",ip,strerror(errno));
			close(fd);
			return -1;
		}
		port = atoi(_port);
	}
	else if(fd_s->type == S_2_WEB)
	{
		//fd is S_2_WEB, so we connect to CA
		ip = config.ca_host;
		port = config.ca_port;

		// already got ca_addr in process_rep_header_buf
		fd_s2 = &fd_table[fds->fc_2_s_fd];
		maddress.sin_addr = fd_s2->ca_addr;

		fd_s->ca_addr = maddress.sin_addr;

		/*
		fd_s2 = &fd_table[fds->fc_2_s_fd];

		if(get_ca_ip(ip, ca_host_type,strBuf(fd_s2->header.uri), &maddress.sin_addr) != 0){
			cclog(2,"ca host -> %s is invalid!!(%s), connect to ca failed",ip,strerror(errno));
			close(fd);
			return -1;
		}

		fd_s->ca_addr = maddress.sin_addr;

		//append request header Host: ca_ip:ca_port for CA
		HttpHeaderEntry *e;
		e = cc_malloc(sizeof(HttpHeaderEntry));

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%s:%d", inet_ntoa(maddress.sin_addr), port);
		
		stringInit(&e->name, "Host");
		stringInit(&e->value, buf);

		httpHeaderAddEntry(&fd_s->header, e);
		*/
	}
	else
	{
		cclog(1, "fd %d type %d error!\n", fd, fd_s->type);
		return -1;
	}
		
        cclog(3,"connect host : %s with ip : %s", ip, inet_ntoa(maddress.sin_addr));

        maddress.sin_family = AF_INET;
        maddress.sin_port   = htons(port);

        int ret = fd_setnonblocking(fd);
        if(ret != 0){
                close(fd);
                return ret;
        }


        int n = 0;
	if( (n = connect(fd , (struct sockaddr *)&maddress, sizeof(struct sockaddr))) < 0) {
		if( errno != EINPROGRESS ) {
			cclog(2, "connect fd: %d host: %s ip: %s failed: %s", fd, ip, inet_ntoa(maddress.sin_addr), strerror(errno));
			if(fd_s->type == S_2_WEB)
				mark_bad_ip(maddress.sin_addr);
			close(fd);
			return -1;
		}
	}

        fd_init(fd);
      	fd_s2 = &fd_table[fd];
	fd_s2->data = fd_s->data;

	if(n == 0)
	{
		cclog(7, "fd %d conn status: %d", fd, CONN_OK);
		fd_s2->conn_status = CONN_OK;
	}
	else
	{
		cclog(7, "fd %d conn status: %d", fd, CONNING);
		fd_s2->conn_status = CONNING;
	}
	

	if(fd_s->type == FC_2_S)
	{
      		fd_s2->type = S_2_WEB;
		fd_s2->write_buff = fd_s->read_buff;
		fd_s2->write_buff_pos = 0;
		fds->s_2_w_fd = fd;

	}
	else if(fd_s->type == S_2_WEB)
	{
		fd_s2->type = S_2_CA;
		fd_s2->write_buff = fd_s->read_buff;
		fd_s2->write_buff_pos = 0;
		fds->s_2_ca_fd = fd;
	}

	if(n == 0)
		server_epoll_add(fd,EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR);
	else
		server_epoll_add(fd,EPOLLOUT|EPOLLHUP|EPOLLERR);

        return 0;
}

int process_header(fd)
{
	fd_struct *fd_s = &fd_table[fd];

	//read from ca ? don't need to parse, send to FC direct
	if(fd_s->type == S_2_CA)
	{
		fd_s->header.header_ok = 1;
		return 0;
	}

	if(fd_s->type == FC_2_S)
		fd_s->header.owner = hoRequest;
	else 
		fd_s->header.owner = hoReply;
	
	return process_header_real(&fd_s->header, fd_s->read_buff, fd_s->read_buff_pos);

/*
	fd_s->header_ok = 1;
	return 0;
*/
}



//read from fd FC_2_S, and fd S_2_WEB use the read_buff to write
int process_read(int fd, char *buf, int len)
{		
	fd_struct *fd_s = &fd_table[fd];
	fd_struct *fd_s2;
	char fd_type = fd_s->type;

	session_fds *fds = fd_s->data;

	cclog(9, "read_pos: %d read_len: %d read_buff_size %d", fd_s->read_buff_pos, len, fd_s->read_buff_size);

	//need MAX_HEADER_SIZE space to store new packed header when before send to CA
	assert(fd_s->read_buff_pos + len <= fd_s->read_buff_size - MAX_HEADER_SIZE);

	memcpy(fd_s->read_buff + fd_s->read_buff_pos, buf, len);
	fd_s->read_buff_pos += len;

	//have space to read or not?
	if(fd_s->read_buff_pos + RECV_BUFF_SIZE + MAX_HEADER_SIZE > fd_s->read_buff_size)
		fd_s->read_delay = 1;

	//parse header finish ?
	if(!fd_s->header.header_ok)
	{
		if(process_header(fd) != 0)
			return ERR;

		//if !header_ok return OK and wait for more data
		if(!fd_s->header.header_ok)
			return OK;
	}

	//at here, we have process the header success, and ready to send the data.
	if(fd_type == FC_2_S)
	{
		fd_s2 = &fd_table[fds->s_2_w_fd];

		if((fds->s_2_w_fd <= 0) || (!fd_s2->inuse) || (fd_s->conn_status == CONN_DEF))
		{
			return connect_to_s(fd);
		}
		else
		{
			assert((fd_s2->inuse) && (fd_s2->type == S_2_WEB));

			if(fd_s2->write_delay == 1)
			{
				server_epoll_mod(fd_s2->fd,EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR);
				fd_s2->write_delay = 0;
			}
		}

		/*
		//delay read
		if((fd_s->read_buff_size - fd_s->read_buff_pos < RECV_BUFF_SIZE) && (fd_s2->write_buff_pos != fd_s->read_buff_pos))
		{
			fd_s->read_delay = 1;
		}
		*/

	}
	else if(fd_type == S_2_WEB)
	{
		//whether need to POST the data recv from web serfer to CA, if don't, write to FC directly
		if(fd_s->need_post == NOT_POST)
		{
			server_epoll_mod(fds->fc_2_s_fd,EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR);
                                fd_s2 = &fd_table[fds->fc_2_s_fd];
                                fd_s2->write_buff = fd_s->read_buff;

                                if(fd_s2->write_delay == 1)
				{
					server_epoll_mod(fd_s2->fd,EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR);
					fd_s2->write_delay = 0;
				}

				return OK;
		}


		//connect and post to CA
		fd_s2 = &fd_table[fds->s_2_ca_fd];

		if((fds->s_2_ca_fd <= 0) || (!fd_s2->inuse) || (fd_s->conn_status == CONN_DEF))
		{
			return connect_to_s(fd);
		}
		else
		{
			assert((fd_s2->inuse) && (fd_s2->type == S_2_CA));

			if(fd_s2->write_delay == 1)
			{
				server_epoll_mod(fd_s2->fd,EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR);
				fd_s2->write_delay = 0;
			}
		}

		/*
		//delay read 
		if((fd_s->read_buff_size - fd_s->read_buff_pos < RECV_BUFF_SIZE) && (fd_s2->write_buff_pos != fd_s->read_buff_pos))
		{
			fd_s->read_delay = 1;
		}
		*/
	}
	else if(fd_type == S_2_CA)
	{
		fd_s2 = &fd_table[fds->fc_2_s_fd];

		assert((fd_s2->inuse) && (fd_s2->conn_status == CONN_OK) && (fd_s2->type == FC_2_S));

		if(fd_s2->write_delay == 1)
		{
			server_epoll_mod(fd_s2->fd,EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR);
			fd_s2->write_delay = 0;
		}

		/*
		if((fd_s->read_buff_size - fd_s->read_buff_pos < RECV_BUFF_SIZE) && (fd_s2->write_buff_pos != fd_s->read_buff_pos))
		{
			fd_s->read_delay = 1;
		}
		*/
	}

	return OK;
}

/*
int process_2_read(int fd, char *buf, int len)
{
	return 0;
}

int process_3_read(int fd, char *buf, int len)
{
	return 0;
}
*/

int handle_read_event(int fd) {
	static char buf[RECV_BUFF_SIZE];

	int len;
	fd_struct *fd_s = &fd_table[fd];
	fd_struct *fd_s2;

	session_fds *fds = fd_s->data;

	if(!(fd_s->inuse))
        {               
                cclog(3, "read event, fd: %d, is not in_use, return", fd);
                return OK;
        }      

	//read delay ?
	if(fd_s->read_delay == 1)
	{
		cclog(8, "read : FD %d delay!\n", fd);
		return OK;
	}
	
	len = read(fd, buf, RECV_BUFF_SIZE);
	if (len < 0)
        {
                cclog(2, "read: FD %d: read failure: %s.\n",
                                          fd, xstrerror());
                if (ignore_errno(errno))
                {
                        return OK;
                }
                else
                {
                        return ERR;
                }
        }
	else if(len == 0)
	{
		//fc close the session? close all...
		if(fd_s->type == FC_2_S)
		{
			cclog(2, "in read event handle, fc close the session");
			return ERR;
		}
		//web close the session ? close later ?.
		else if(fd_s->type == S_2_WEB)
		{
			if(fd_s->need_post == NOT_POST)
			{
				fd_s2 = &fd_table[fds->fc_2_s_fd];

				if(!fd_s2->inuse)
				{
					cclog(3, "in read event handle, fd_s2 is not inuse");
					return OK;
				}

				cclog(6, "%d, %d", fd_s->read_buff_pos, fd_s2->write_buff_pos);
				if(fd_s->read_buff_pos != fd_s2->write_buff_pos)
					fd_s->close_delay = 1;
				else
					fd_s->close_delay = 0;
			}
			else
			{
				//don't close until all data from ca has been sent to FC
				fd_s->close_delay = 1;
			}
		}
		//ca close and all data has sent to FC?
		else if(fd_s->type == S_2_CA)
		{
			fd_s2 = &fd_table[fds->fc_2_s_fd];

			if(!fd_s2->inuse)
			{
				cclog(3, "in read event handle, fd_s2 is not inuse");
				return OK;
			}

			cclog(6, "%d, %d", fd_s->read_buff_pos, fd_s2->write_buff_pos);
			if(fd_s->read_buff_pos != fd_s2->write_buff_pos)
				fd_s->close_delay = 1;
			else
				fd_s->close_delay = 0;
		}

		return ERR_PEER_CLOSE;
	}

	fd_s->read_count += len;

	cclog(8, "read %d bytes from fd %d type %d\n", len, fd, fd_s->type);

	return process_read(fd, buf, len);
/*
	switch(fd_s->type)
	{
		case FC_2_S:
			return process_1_read(fd, buf, len);
		case S_2_WEB:
			return process_2_read(fd, buf, len);
		case S_2_CA:
			return process_3_read(fd, buf, len);
			break;
		default:
			break;
	}
*/

	return OK;

}

int write_method(int fd, char *buf, int size)
{
	int len;
	len = write(fd, buf, size);

	if (len < 0)
	{
		cclog(2, "write: FD %d: write failure: %s.\n",
                                          fd, xstrerror());
                if (ignore_errno(errno))
                {
                        return 0;
                }
                else
                {
                        return -1;
                }
        }
	else
		return len;
}

int process_write(fd)
{
	int len;
	int left;
	int tmp_fd;
	fd_struct *fd_s = &fd_table[fd];
	fd_struct *fd_s2;
	session_fds *fds = fd_s->data;

	char fd_type = fd_s->type;

	if(fd_s->conn_status == CONNING)
	{
		if(is_socket_alive(fd))
		{
			fd_s->conn_status = CONN_OK;
			server_epoll_mod(fd,EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR);

			if(fd_type == S_2_CA)
			{
				server_epoll_mod(fds->fc_2_s_fd,EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLERR);
				fd_s2 = &fd_table[fds->fc_2_s_fd];

				if(!fd_s2->inuse)
				{
					cclog(3, "in process 1 write handle of conn, fd_s2 is not inuse");
					return OK;
				}

				fd_s2->write_buff = fd_s->read_buff;
				if(fd_s2->write_buff_pos == fd_s2->read_buff_pos)
				{
					server_epoll_mod(fd_s2->fd,EPOLLIN|EPOLLHUP|EPOLLERR);
					fd_s2->write_delay = 1;
				}
			}
		}
		else
		{
			cclog(2, "in process 1 write handle, fd: %d failed %s", fd, strerror(errno));

			if(fd_type == S_2_CA)
				mark_bad_ip(fd_s->ca_addr);
			
			return ERR;
		}
	}

	if(fd_s->write_delay)
	{
		cclog(8, "in process fd: %d write handle, waiting for more data arrive!", fd);
		return OK;
	}

	if(fd_type == S_2_WEB)
	{
		if((tmp_fd = fds->fc_2_s_fd) <= 0)
		{
			cclog(3, "in process write handle, fds->fc_2_s_fd is not > 0");
			return OK;
		}

		fd_s2 = &fd_table[tmp_fd];

		if(!fd_s2->inuse)
		{
			cclog(3, "in process write handle, fd_s2 is not inuse");
			return OK;
		}
		assert(fd_s2->type == FC_2_S);
	}
	else if(fd_type == S_2_CA)
	{
		if((tmp_fd = fds->s_2_w_fd) <= 0)
		{
			cclog(3, "in process write handle, fds->s_2_w_fd is not > 0");
			return OK;
		}
		
		fd_s2 = &fd_table[tmp_fd];

		if(!fd_s2->inuse)
		{
			cclog(3, "in process write handle, fd_s2 is not inuse");
			return OK;
		}
		assert(fd_s2->type == S_2_WEB);
	}
	else if(fd_type == FC_2_S)
	{
		//data don't POST to ca ???
		if((tmp_fd = fds->s_2_w_fd) <= 0)
		{
			cclog(3, "in process write handle, fds->s_2_w_fd in not > 0");
			return OK;
		}

		fd_s2 = &fd_table[tmp_fd];

		if(!fd_s2->inuse)
		{
			cclog(3, "in process write handle, fd_s2 is not inuse");
			return OK;
		}

		if(fd_s2->need_post == NOT_POST)
		{
			//read_buff is S_2_WEB
			(void)0;
		}
		else
		{
			//read_buf is S_2_CA
			if((tmp_fd = fds->s_2_ca_fd) <= 0)
			{
				cclog(3, "in process write handle, fds->s_2_w_fd in not > 0");
				return OK;
			}

			fd_s2 = &fd_table[tmp_fd];

			if(!fd_s2->inuse)
			{
				cclog(3, "in process write handle, fd_s2 is not inuse");
				return OK;
			}
			assert(fd_s2->type == S_2_CA);
		}
	}
	else
	{
		cclog(1, "unknown fd %d type: %d", fd, fd_type);
		return ERR;
	}

	assert(fd_s->write_buff && ((left = fd_s2->read_buff_pos -fd_s->write_buff_pos) > 0));

	len = write_method(fd, fd_s->write_buff + fd_s->write_buff_pos, left);

	if(len < 0)
	{
		cclog(2, "in process  write handle, fd: %d failed2 %s", fd, strerror(errno));
		return ERR;
	}

	fd_s->write_buff_pos += len;

	//all data in buf is sent, should we close the conn or reset the buff pos and wait more data ?
	if(fd_s2->read_buff_pos == fd_s->write_buff_pos)
	{
		//don't need POST to ca, and WEB already closed the conn to S
		if((fd_s2->type == S_2_WEB) && (fd_s2->close_delay == 1))
		{
			fd_s2->close_delay = 0;
			return CLOSE;
		}

		//CA already closed the conn to S
		if((fd_s2->type == S_2_CA) && (fd_s2->close_delay == 1))
		{
			fd_s2->close_delay = 0;
			return CLOSE;
		}

		cclog(5, "reset the buff pos");
		fd_s->write_buff_pos = fd_s2->read_buff_pos = 0;
		if(fd_s2->read_delay)
			fd_s2->read_delay = 0;

		server_epoll_mod(fd_s->fd,EPOLLIN|EPOLLHUP|EPOLLERR);
		fd_s->write_delay = 1;
	}

	return OK;
}

int handle_write_event(int fd) {

	fd_struct *fd_s = &fd_table[fd];
        cclog(6,"write event , fd: %d, type: %d",fd, fd_s->type);

	if(!(fd_s->inuse))
	{
		cclog(8, "write event, fd: %d, is not in_use, return", fd);
		return OK;
	}

	assert(fd_s->conn_status != CONN_DEF);

	switch(fd_s->type)
	{
		case FC_2_S:
		case S_2_WEB:
		case S_2_CA:
			return process_write(fd);
		default:
			return ERR;
	}
}

static void set_max_fd()
{
	struct rlimit rl;
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
		cclog(0, "setrlimit: RLIMIT_NOFILE: %s %d %d\n", xstrerror(),rl.rlim_cur,rl.rlim_max);
		abort();
	}
	else if (FD_MAX > 0)
	{
		rl.rlim_cur = FD_MAX;
		if (rl.rlim_cur > rl.rlim_max)
			rl.rlim_max = rl.rlim_cur;
		cclog(0, "rl.rlim_cur = %d rl.rlim_max = %d\n",rl.rlim_cur,rl.rlim_max);
		if (setrlimit(RLIMIT_NOFILE, &rl))
		{
			cclog(0, "setrlimit: RLIMIT_NOFILE: %s %d %d\n", xstrerror(),rl.rlim_cur,rl.rlim_max);
			getrlimit(RLIMIT_NOFILE, &rl);
			cclog(0, "rl.rlim_cur = %d rl.rlim_max = %d\n",rl.rlim_cur,rl.rlim_max);
			rl.rlim_cur = rl.rlim_max;
			if (setrlimit(RLIMIT_NOFILE, &rl))
			{
				cclog(0, "setrlimit: RLIMIT_NOFILE: %s %d %d\n", xstrerror(),rl.rlim_cur,rl.rlim_max);
				abort();
			}
		}
	}
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
		cclog(0,"setrlimit: RLIMIT_NOFILE: %s\n", xstrerror());
		abort();
	}
	else
	{
		cclog(0, "rl.rlim_cur = %d rl.rlim_max = %d\n",rl.rlim_cur,rl.rlim_max);
		FD_MAX = rl.rlim_cur;
	}
}

static void fd_table_init(){
	set_max_fd();
	
        fd_table = NULL;
        fd_table = cc_malloc(sizeof(fd_struct) * FD_MAX);
        assert(fd_table != NULL);

        memset(fd_table,0,sizeof(fd_struct) * FD_MAX);
}

static void fd_table_dest(){
        int index = 0;
        for(;index < FD_MAX;index++){
                if(fd_table[index].inuse == 1)
                        disconnect_fd(index, 1);
        }

        cc_free(fd_table);
}

static void system_init()
{
        setuid(0);
        signal_init();
        server_epoll_init();
        fd_table_init();

        listen_start(config.port);
}

static void exit_handle()
{
        if(listen_fd != -1){
                close(listen_fd);
                listen_fd = -1;
        }

        server_epoll_dest();
        fd_table_dest();
//	server_config_dest();

        unlink(PIDFILE);
}

void mem_check(){
        cclog(3,"malloc count   : %"PRINTF_UINT64_T,cc_malloc_count);
        cclog(3,"malloc times   : %"PRINTF_UINT64_T,cc_malloc_times);
        cclog(3,"free times     : %"PRINTF_UINT64_T,cc_free_times);
}

static int server_signal_check(){
        if (server_shutdown) {
                cclog(0,"server_shutdown!!\n");
                return -1;
        }
        else if(server_reconfig){
                int ret = config_parse(config_file);
                if(ret != OK){
                        cclog(0,"parse config file error! %s",config_file);
                        exit(-1);
                }

	server_reconfig = false;
	}
        else if(server_log_rotate){
                scs_log_rotate();
                server_log_rotate = false;
        }
        else if(server_sig_child){
                sig_chld();
                server_sig_child = false;
        }

        return 0;
}


int main(int argc, char ** argv) {
        now = time(NULL);
                
        options_parse(argc, argv);
        int ret = config_parse(config_file);
        if(ret != OK){
                fprintf(stderr,"parse config file error ! %s\n",config_file);
                exit(-1);
        }

	if(daemon_mode)
                daemonize();

        if(check_running_pid())
                exit(-1);

        cclog(0,"server start!");
        system_init();

        while(true) {

		cclog(8, "----------------------------------------------------->");
                int ret = server_signal_check();
                if(ret == -1){
                        break;
                }

                now = time(NULL);

                server_epoll_wait(handle_accept,handle_read_event,handle_write_event);
        }

        exit_handle();
        mem_check();
        cclog(0,"server exited!");
        return OK;
}

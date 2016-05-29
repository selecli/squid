/*
 * $Id$
 * author : liwei(liwei@chinacache.com)
 * desc   : refreshd main
 */

#include "refresh_header.h"

static char *config_file = "/usr/local/squid/etc/refreshd.conf";
static int daemon_mode = 0;
rf_client *fd_table;
rf_client *rfc_current;
rf_client *rfc_tmp;
int listen_fd;
int db_num;
static int last_db_num = 0;
pid_t hash_pid;
time_t now;
bool rf_shutdown = false;
bool change = false;
static bool rf_reconfig = false;
static bool rf_url_verify = false;
static bool rf_log_rotate = false;
static bool rf_sig_child  = false;
static char *version = "3.2";

bool enable_dir_purge = false;

//for dir refresh thread
bool stop;

hash_dir_sess_stack hash_dir_sess_stacks[DB_NUM];

extern void backtrace_info();

extern uint64_t cc_malloc_count ;
extern uint64_t cc_malloc_times ;
extern uint64_t cc_free_times ;

static void usage(void)
{
	fprintf(stderr, "USAGE: refreshd [options]\n"
			" -f			config file\n"
			" -d 			toggle daemon mode\n"
			" -k 			\n"
			"\t\tshutdown\n"
			"\t\treconfig\n"
			"\t\turl_verify\n"
			"\t\tlog_rotate\n"
			" -h			print help information\n"
			" -v			print version information\n"
		   );
}

/*
 *	sig_chld
 *	回收子进程
 */
void sig_chld()
{
	pid_t   pid;
	int     stat;

	while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
	{
		//if hash db return
		if(pid == hash_pid)
		{
			cclog(0, "hash db return\n");
			change = false;
		}
	}
}

static void signals_handle(int sig)
{
	switch(sig) {
		case SIGSEGV:
			signal(SIGSEGV, SIG_DFL);
			sig_log(sig);
			backtrace_info();
			rf_shutdown = true;
			exit(-1);
		case SIGABRT:
			signal(SIGABRT, SIG_DFL);
			sig_log(sig);
			rf_shutdown = true;
			exit(-1);
		case SIGINT:
		case SIGTERM:
		case SIGHUP:
			sig_log(sig);
			backtrace_info();
			rf_shutdown = true;
			break;
		case SIGUSR1:
			rf_reconfig = true;
			break;
		case SIGUSR2:
			rf_url_verify = true;
			break;
		case SIGCHLD:
			rf_sig_child = true;
			break;
		case SIGQUIT:
			rf_log_rotate = true;
			break;
		default:
			sig_log(sig);
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
		fprintf(stderr, "no refreshd is running\n");
		return -2;
	}

	int sig = 0;

	if(strcasecmp(cmd,"shutdown") == 0){
		sig = SIGHUP;
	}
	else if(strcasecmp(cmd,"reconfig") == 0){
		sig = SIGUSR1;
	}
	else if(strcasecmp(cmd,"url_verify") == 0){
		sig = SIGUSR2;
	}
	else if(strcasecmp(cmd,"log_rotate") == 0){
		sig = SIGQUIT;
	}
	else{
		usage();
		return -1;
	}

	cclog(0,"send %s command to refreshd(%d)",cmd,pid);

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
				printf("refreshd version V%s\n",version);
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
		rf_shutdown = true;
		return;
	}

	struct	sockaddr_in	server_address;
	memset(&(server_address), 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port	= htons(port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	int option = 1;
	if( setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0 ) {
		cclog(0,"setsockopt fail: %s\n", strerror(errno));
		rf_shutdown = true;
		return;
	}

	int bind_times = 10;
	while( bind(m_socket,(struct sockaddr*)&server_address, sizeof(struct sockaddr)) < 0 ) {
		cclog(0,"bind fail. retry: %s\n", strerror(errno));
		sleep(1);
		if( rf_shutdown )
			return;
		if((bind_times--) == 0){
			rf_shutdown = true;
			return;
		}
	}

	if( listen(m_socket, 10) < 0 ) {
		cclog(0,"listen fail:  %s\n", strerror(errno));
		rf_shutdown = true;
		return;
	}

	rf_epoll_add(m_socket,EPOLLIN | EPOLLHUP | EPOLLERR);
	fd_close_onexec(m_socket);
	listen_fd = m_socket;
}

int rf_fd_init(int fd){
	if(fd > FD_MAX){
		cclog(0,"fatal ! fd(%d) > FD_MAX(%d) !!",fd,FD_MAX);
		assert(fd < FD_MAX);
	}

	rf_client *rfc = &fd_table[fd];
	if(rfc->inuse){
		cclog(3,"fd in use : fd(%d),ip(%s),type(%d)",fd,get_peer_ip(fd),rfc->type);
		goto OUT;
	}


	rfc->inuse = 1;
	rfc->setup_time = now;
	rfc->fd = fd;
	rfc->type = RF_CLIENT_UNKNOW;

	assert(rfc->read_buff == NULL);

	//rfc->read_buff = cc_malloc(BUFFER_SIZE);
	cclog(3,"fd new connect : fd(%d),ip(%s),type(%d)",fd,get_peer_ip(fd),rfc->type);
OUT:
	return RF_OK;
}

/*
 *	rf_fd_filter
 *	向refreshd发送连接请求的可能会有FC和刷新系统,refreshd需要对两者进行区分
 *	该函数预读tcp buffer，如果发现有http则认为是client(刷新系统)发送过来的连接
 *	否则认为是FC
 */
int rf_fd_filter(int fd){
	rf_client *rfc = &fd_table[fd];
	static char buffer[1024];
	memset(buffer,0,1024);

	int ret = recv(fd,buffer,1024,MSG_PEEK);
	if(ret == 0)
		return RF_ERR_PEER_CLOSE;
	else if(ret < 0){
		cclog(3,"recv error from %d,%s,error reason: %s",fd,get_peer_ip(fd),strerror(errno));
		return RF_ERR_RECV;
	}

	//FIXME: Possible "ht" or "htt",but it is a part of "http"
	if(strstr(buffer,"http") || strstr(buffer,"HTTP"))
		rfc->type = RF_CLIENT_CLI;
	else
		rfc->type = RF_CLIENT_FC;

	return RF_OK;
}

/*
 *	handle_accept
 *	接受连接，并加入到epoll
 */
static void handle_accept(int listen_fd)
{
	struct	sockaddr_in	client_addr;
	socklen_t len = sizeof(client_addr);
	int client_socket = 0;

	if( (client_socket = accept(listen_fd, (struct sockaddr*)&client_addr, &len)) < 0 ) {
		cclog(0,"accept fail: %s", strerror(errno));
		return;
	}

	cclog(3,"accept ok!client ip = %s", get_peer_ip(client_socket));

	int opts;
	if( (opts = fcntl(client_socket, F_GETFL)) < 0 ) {
		goto failed;
	}

	opts = opts|O_NONBLOCK|FD_CLOEXEC;

	if( fcntl(client_socket, F_SETFL, opts) < 0 ) {
		goto failed;
	}

	int ret = rf_epoll_add(client_socket,EPOLLIN | EPOLLHUP | EPOLLERR);
	if(ret != 0){
		goto failed;
	}

	rf_fd_init(client_socket);

	return;
failed:
	cclog(0,"unresolved socket : %s\n",get_peer_ip(client_socket));
	close(client_socket);
}

void disconnect_fd(int fd){
	rf_client *rfc = &fd_table[fd];

	int i;

	for(i = 0; i < DB_NUM; i++)
	{
		if(rfc->db_handlers[i] != NULL)
			rf_db_close(rfc->db_handlers[i]);

		if(rfc->db_bak_handlers[i] != NULL)
			rf_db_close(rfc->db_bak_handlers[i]);

		if(rfc->db_files[i] != NULL)
			cc_free(rfc->db_files[i]);

		if(rfc->db_bak_files[i] != NULL)
			cc_free(rfc->db_bak_files[i]);
	}

	if(rfc->ident != NULL){
		cc_free(rfc->ident);
	}

	if(rfc->read_buff){
		cc_free(rfc->read_buff);
	}

	if(rfc->wl){
		rf_wl_free(rfc->wl);
		cc_free(rfc->wl);
	}

	//set all members to zero
	memset(rfc,0,sizeof(rf_client));

	rf_epoll_remove(fd);
	close(fd);
}

/* added-begin by xin.yao: alloc read buffer by 'Content-Length' */
typedef struct peerk_data_st {
	size_t hdrlen; 
	size_t contlen;
} peek_data_t;

/* if peek no 'Content-Length', set read buffer size 800KB */
#define MAX_READ_BUF_SIZE 819200

static ssize_t parsePeekMsgHeaders(char *buffer, peek_data_t *peek)
{
	size_t contlen;
	char *pos, *token;

	memset(peek, 0, sizeof(peek_data_t));

	pos = strstr(buffer, "\r\n\r\n");
	cclog(0, "header: %s\n", buffer);
	if (NULL == pos) 
	{    
		cclog(2, "recv(MSG_PEEK): message has no headers");
		return -1;
	}    
	peek->hdrlen = pos - buffer + strlen("\r\n\r\n");
	contlen = strlen("Content-Length");
	token = strtok(buffer, "\r\n");
	while (NULL != token)
	{    
		if (!strncasecmp(token, "Content-Length", contlen))
		{    
			pos = strchr(token, ':');
			if (NULL == pos) 
			{    
				cclog(0, "header 'Content-Length' has wrong format");
				return -1;
			}
			peek->contlen = atol(++pos);
			cclog(2, "peek Content-Length: %ld", peek->contlen);
			return (peek->contlen < 0) ? -1 : 0;
		}
		token = strtok(NULL, "\r\n");
	}

	return -1;
}

static ssize_t readBufferSizeByPeekMsgHeaders(int sockfd, peek_data_t *peek)
{
	ssize_t ret;
	char buffer[8192];

	memset(buffer, 0, 8192);
	/* only use massage peek for parse header */
	ret = recv(sockfd, buffer, 8192, MSG_PEEK);
	if (ret < 0)
	{
		cclog(0, "recv(MSG_PEEK) failed: %s", strerror(errno));
		return -1;
	}

	return parsePeekMsgHeaders(buffer, peek);
}

static int allocReadBufferByHeaderContentLength(const int sockfd)
{
	peek_data_t peek;
	rf_client *rfc = &fd_table[sockfd];

	if (readBufferSizeByPeekMsgHeaders(sockfd, &peek) < 0)
	{
		/* if MSG_PEEK failed or no 'Content-Length'
		 * set read_buff_size 800KB
		 */
		rfc->read_buff_size = MAX_READ_BUF_SIZE;
	}
	else
	{
		rfc->read_buff_size = peek.hdrlen + peek.contlen + 1;
		/* if peek size bigger than 800KB, only recv 800KB */
		if (rfc->read_buff_size > MAX_READ_BUF_SIZE)
			rfc->read_buff_size = MAX_READ_BUF_SIZE;
		cclog(3, "peek.hdrlen: %ld, peek.contlen: %ld\n", peek.hdrlen, peek.contlen);
	}
	/* cc_malloc() called calloc() for allocating */
	rfc->read_buff = cc_malloc(rfc->read_buff_size);
	if (NULL == rfc->read_buff)
	{
		cclog(0, "rfc->read_buff calloc() failed: %s\n", strerror(errno));
		return -1;
	}
	cclog(3, "rfc->read_buff_size: %d\n", rfc->read_buff_size);
	return 0;
}
/* added-end by xin.yao: alloc read buffer by Content-Length */

/*
 *	rf_fd_recv
 *	接收发送过来的数据,注意前后buffer的数据拼接
 */
int rf_fd_recv(int fd){
	rf_client *rfc = &fd_table[fd];
	assert(rfc);

	/* added by xin.yao: 
	 * alloc read buffer by header 'Content-Lenth'
	 */
	if (NULL == rfc->read_buff)
	{
		if (allocReadBufferByHeaderContentLength(fd) < 0)
			return RF_ERR_RECV; 
	}
	assert(rfc->read_buff);
	memset(rfc->read_buff + rfc->read_buff_len, 0, rfc->read_buff_size - rfc->read_buff_len);

	int ret = 0;
	if(rfc->type == RF_CLIENT_UNKNOW){
		ret = rf_fd_filter(fd);
		if(ret != RF_OK)
			return ret;
	}

	ret = recv(fd,rfc->read_buff + rfc->read_buff_len,BUFFER_SIZE - rfc->read_buff_len,0);
	if(ret == 0)
		return RF_ERR_PEER_CLOSE;
	else if(ret < 0)
	{
		cclog(0, "Recv from fd: [%d] error: %s", fd, strerror(errno));
		return RF_ERR_RECV;
	}

	rfc->read_buff_len += ret;

	if(rfc->type == RF_CLIENT_REPORT)
	{
		cclog(3, "recv from report ip, now close the report fd"); 
		return RF_CLOSE_REPORT;
	}

	return RF_OK;
}

//处理读事件
int handle_read_event(int fd) {
	int ret  = rf_fd_recv(fd);
	if(ret != RF_OK)
		goto failed;

	rf_client *rfc = &fd_table[fd];
	rfc_current = rfc;

	if(rfc->type == RF_CLIENT_FC){
		ret = rf_fc_process(fd);
		if(ret != RF_OK)
			cclog(0,"rf_fc_process error!!");

		//do not want close connection with squid!
		return RF_OK;
	}
	else if(rfc->type == RF_CLIENT_CLI){
		ret = rf_cli_process(fd);
	}
	else{
		cclog(2,"unknown client !!");
		ret = RF_ERR_UNKNOWN_CLIENT;
	}

failed:
	return ret;
}

//处理写事件
int handle_write_event(int fd) {
	cclog(6,"toggle write event , fd: %d",fd);

	return rf_wl_xsend(fd);
}

static void fd_table_init(){
	fd_table = NULL;
	fd_table = cc_malloc(sizeof(rf_client) * FD_MAX);
	assert(fd_table != NULL);

	memset(fd_table,0,sizeof(rf_client) * FD_MAX);
}

static void fd_table_dest(){
	int index = 0;
	for(;index < FD_MAX;index++){
		if(fd_table[index].inuse == 1)
			disconnect_fd(index);
	}

	cc_free(fd_table);
}

static void system_init()
{
	setuid(0);
	signal_init();
	rf_epoll_init();
	fd_table_init();

	int ret = rf_store_init();
	if(ret != RF_OK)
		abort();

	rf_session_init();
	listen_start(config.port);
}

static void exit_handle()
{
	if(listen_fd != -1){
		close(listen_fd);
		listen_fd = -1;
	}

	rf_epoll_dest();
	fd_table_dest();
	rf_session_dest();
	rf_store_dest();

	int i;
	for(i = 0; i < config.compress_formats_count; i++)
	{
		if(config.compress_formats[i] != NULL)
			cc_free(config.compress_formats[i]);
	}

	unlink(PIDFILE);
}

void mem_check(){
	cclog(3,"malloc count	: %"PRINTF_UINT64_T,cc_malloc_count);
	cclog(3,"malloc times	: %"PRINTF_UINT64_T,cc_malloc_times);
	cclog(3,"free times	: %"PRINTF_UINT64_T,cc_free_times);
}

static int rf_signal_check(){
	if (rf_shutdown) {
		cclog(0,"rf_shutdown!!\n");
		return -1;
	}
	else if(rf_reconfig){
		int ret = config_parse(config_file);
		if(ret != RF_OK){
			cclog(0,"parse config file error! %s",config_file);
			exit(-1);
		}

		if(config.if_hash_db)
			db_num = 5;
		else
			db_num = 1;

		cclog(2, "last_db_num: %d, db_num: %d", last_db_num, db_num);

		if((last_db_num == 5) && (db_num == 1))		//can't do that, ignore it
			db_num = 5;

		if((last_db_num == 1) && (db_num == 5))		//change mode to hash db
		{	
			last_db_num = 5;
			change = true;
		}

		rf_reconfig = false;
	}else if(rf_url_verify){
		rf_notify_squid_verify();
		rf_url_verify = false;
	}
	else if(rf_log_rotate){
		scs_log_rotate();
		rf_log_rotate = false;
	}
	else if(rf_sig_child){
		sig_chld();
		rf_sig_child = false;
	}

	return 0;
}

int main(int argc, char ** argv) {
	now = time(NULL);

	last_db_num = 1;	//default
	db_num = 1;

	stop = false;

	options_parse(argc, argv);
	int ret = config_parse(config_file);
	if(ret != RF_OK){
		fprintf(stderr,"parse config file error ! %s\n",config_file);
		exit(-1);
	}

	if(config.if_hash_db)
		db_num = 5;
	else
		db_num = 1;

	last_db_num = db_num;

	if(daemon_mode)
		daemonize();

	if(check_running_pid())
		exit(RF_ERR_HAVE_INSTANCE);

	cclog(0,"refreshd start!");
	system_init();

	while(true) {
		int ret = rf_signal_check();
		if(ret == -1){
			break;
		}

		//if change to hash db mode, close all connected FC dbs stored in the fd_table, then we can delete them later. 
		if(change)
		{
			cclog(0, "change to hash db mode\n");

			now = time(NULL);

			int fc_array[128];
			int count = 0;
			int i;

			char db_name[30][256];
			char *_db_name[30];

			memset(db_name, 0, sizeof(db_name));

			//for exec args
			strcpy(db_name[0], "hash_db");
			_db_name[0] = db_name[0];

			rf_get_fc_alive(fc_array,&count);

			if(count > 0)
			{
				for(i = 0; i < count; i++)
				{
					rfc_tmp = &fd_table[fc_array[i]];	

					if(rfc_tmp->db_handlers[0] != NULL)
						fclose(rfc_tmp->db_handlers[0]);
					rfc_tmp->db_handlers[0] = NULL;

					strcpy(db_name[i + 1], rfc_tmp->db_files[0]);
					_db_name[i + 1] = db_name[i + 1];
					cclog(0, "original db name: %s\n", db_name[i + 1]);

					cc_free(rfc_tmp->db_files[0]);
					rfc_tmp->db_files[0] = NULL;
				}

				_db_name[i + 1] = (char*)NULL;

				//call hash db function

				if((hash_pid = fork()) == 0)
				{
					execv(REFRESH_HASH_DB_PATH, _db_name);

					cclog(0, "exec change to hash db mode failed: %s!!!", strerror(errno));
					cclog(0, "refreshd change mode failed, exit now!!!");
					kill(getppid(), 9);
					exit(-1);
				}
			}
			else
			{
				cclog(0, "can't find connect FC, please retry to change db mode later\n");
				db_num = 1;
				last_db_num = 1;
				change = false;

				goto hash_out;
			}

			while(change != false)
			{
				if((time(NULL) - now) > 300)
				{
					cclog(0, "hash db process time out, please retry later\n");
					change = false;
					db_num = 1;
					last_db_num = 1;

					goto hash_out;
				}

				//wait for hash db return
				usleep(300);
				if(-1 == rf_signal_check())
				{
					cclog(0, "hash db process time out 2, please retry later\n");
					change = false;
					db_num = 1;
					last_db_num = 1;

					goto hash_out;
				}

				continue;
			}

			//hash return, now remove the ori db files

			i = 0;

			while(_db_name[i + 1])
			{
				if(unlink(_db_name[1 + (i++)]) != 0)
					cclog(0, "delete original db: %s failed\n", _db_name[i]);
				else
					cclog(0, "delete original db: %s success\n", _db_name[i]);
			}
		}

hash_out:

		now = time(NULL);

		rf_epoll_wait(handle_accept,handle_read_event,handle_write_event);
		rf_session_check();
		rf_db_check();
	}

	exit_handle();
	mem_check();
	cclog(0,"refreshd exited!");
	return RF_OK;
}

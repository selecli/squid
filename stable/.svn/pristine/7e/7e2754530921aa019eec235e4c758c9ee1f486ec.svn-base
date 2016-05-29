#include "server_header.h"
#include "log.h"
#include <execinfo.h>

#define inaddrr(x) (*(struct in_addr *) &ifr->x[sizeof sa.sin_port])
#define IFRSIZE   ((int)(size * sizeof (struct ifreq)))

extern FILE* debug_log;

char* get_peer_ip(int socket_fd) {
//	return "noting";
	errno = 0;

	struct sockaddr_in cli_addr;
	socklen_t cli_len = sizeof(struct sockaddr_in);

	if(getpeername(socket_fd,(struct sockaddr *)&cli_addr,&cli_len) == -1) {
		//perhaps failed when connecting to peer....
		cclog(5, "getpeername failed: %s",strerror(errno));
		return NULL;
	}

	return inet_ntoa(cli_addr.sin_addr);
}

void daemonize(void) {

	pid_t pid;

	pid = fork();

	if( pid < 0 ) {
		fprintf(stderr, "fork, fail.\n");
		exit(1);
	}

	if( pid > 0 ) {
		exit(0);
	}

	setsid();

	close(0);
	close(1);
	close(2);

	chdir("/");
}

pid_t get_running_pid(void) {
	FILE *fd;
	char pid[128];
	char buff[128];
	FILE  *procfd;
	int pidnum;
	pid_t ret = -1;

	if ((fd = fopen(PIDFILE, "r")) != NULL) {
		fgets(pid, sizeof(pid), fd);
		pidnum = atoi(pid);
		snprintf(buff, sizeof(buff), "/proc/%d/status", pidnum);
		if ((procfd = fopen(buff,"r")) != NULL) {
			fgets(pid, sizeof(pid), procfd);
			if (strstr(pid, "server")!= NULL) {
				ret = pidnum;
			}

			fclose(procfd);
		}

		fclose(fd);
	}
	return ret;
}

int check_running_pid(void) {
	int ret = 1;
	pid_t pid = get_running_pid();
	if(pid == -1)
	{
		FILE * fd;
		if ((fd = fopen(PIDFILE, "w+")) != NULL)
		{
			fprintf(fd, "%d", getpid());
			fclose(fd);
			ret = 0;
		}
		else
			ret = -1;

	}
	else
		printf("server is running, process id:%d\n",pid);

	return ret;
}

char *signal_itoa(int sig){
	switch(sig) {
		case SIGINT:
			return "SIGINT";
		case SIGTERM:
			return "SIGTERM";
		case SIGHUP:
			return "SIGHUP";
		case SIGSEGV:
			return "SIGSEGV";
		case SIGABRT:
			return "SIGABRT";
		case SIGUSR1:
			return "SIGUSR1";
		case SIGUSR2:
			return "SIGUSR2";
		case SIGCHLD:
			return "SIGCHLD";
		case SIGQUIT:
			return "SIGQUIT";
		default:
			return "UNKNOWN";
	}
}

int *strto_mi(char *buffer,char token){
	assert(buffer != NULL);

	static int mi[10];
	int i,s,t;
	i = s = t = 0;
	char tmp_buff[32];

	while(buffer[i++] != '\0'){
		if(buffer[i] == token){
			memset(tmp_buff,0,32);
			memcpy(tmp_buff,buffer + s,i - s);
			mi[t++] = atoi(tmp_buff);

			while(buffer[i] == token) i++;
			s = i;
		}
	}

	mi[t++] = atoi(buffer + s);
	return mi;
}

int get_local_ip(uint32_t *ip_list,uint32_t *ip_number){
	int                sockfd, size  = 1;
	struct ifreq       *ifr;
	struct ifconf      ifc;
	struct sockaddr_in sa;
	int32_t result = -1;

	if (0 > (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP))) {
		return -1;
	}

	ifc.ifc_len = IFRSIZE;
	ifc.ifc_req = NULL;

	do {
		++size;
		if (NULL == (ifc.ifc_req = realloc(ifc.ifc_req, IFRSIZE))) {
			result = -1;
			goto OUT;
		}
		ifc.ifc_len = IFRSIZE;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc)) {
			result = -1;
			goto OUT;
		}
	} while  (IFRSIZE <= ifc.ifc_len);

	ifr = ifc.ifc_req;
	int i = 0;
	for (;(char *) ifr < (char *) ifc.ifc_req + ifc.ifc_len; ++ifr) {
		if (ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data) {
			continue;
		}

		if (ioctl(sockfd, SIOCGIFFLAGS, ifr)) {
			continue;
		}

		struct in_addr local_in = inaddrr(ifr_addr.sa_data);
		ip_list[i++] = (uint32_t)local_in.s_addr;
	}

	*ip_number = i;
	result = 0;

OUT:
	close(sockfd);
	free(ifc.ifc_req);
	return result;
}

bool is_local_ip(uint32_t ip){
	static uint32_t ip_list[128];
	static uint32_t ip_number = 0;

	if(ip_number == 0){
		get_local_ip(ip_list ,&ip_number);
	}

	uint32_t i = 0;
	for(; i < ip_number; i++){
		if(ip_list[i] == ip)
			return true;
	}

	return false;
}

bool is_local_socket(int socket_fd){
	struct sockaddr_in cli_addr;
	socklen_t cli_len = sizeof(struct sockaddr_in);

	if(getpeername(socket_fd,(struct sockaddr *)&cli_addr,&cli_len) == -1) {
		return false;
	}

	return is_local_ip(cli_addr.sin_addr.s_addr);
}

int ignore_errno(int ierrno)
{
	switch (ierrno)
	{
		case EINPROGRESS:
		case EWOULDBLOCK:
#if EAGAIN != EWOULDBLOCK
		case EAGAIN:
#endif
		case EALREADY:
		case EINTR:
#ifdef ERESTART
		case ERESTART:
#endif
			return 1;
		default:
			return 0;
	}
}

int is_socket_alive(int sock_fd){
	int err = 0;
	errno = 0;
	socklen_t e_len = sizeof(err);
	int x = getsockopt(sock_fd,SOL_SOCKET,SO_ERROR,&err,&e_len);
	if(x == 0)
		errno = err;
/*
	if (errno == 0 || errno == EISCONN)
		return true;;
*/
	if(errno == 0)
		return 1;

	if(errno == EISCONN)
		return EISCONN;

	return 0;
}

static void
fd_set_reuse_addr(int fd)
{
        int on = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
                cclog(1, "fd_set_reuseaddr: FD %d: %s\n", fd, strerror(errno));
}               

int fd_setnonblocking(int fd) {
	int flags;
	int dummy = 0;

	fd_set_reuse_addr(fd);

	if ((flags = fcntl(fd, F_GETFL, dummy)) < 0) {
		return -1;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		return -1;
	}
	return 0;
}

char *get_ip_by_ethname(const char *ethname){
	int   rec;  
	struct   ifreq   if_data;  
	uint32_t  ip;  
	struct in_addr in;  

	if (0 > (rec = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP))) {
		return NULL;
	}

	strcpy(if_data.ifr_name,ethname);  
	if(ioctl(rec,SIOCGIFADDR,&if_data) < 0){  
		return NULL;
	}  

	memcpy((void *)&ip, (void *)&if_data.ifr_addr.sa_data + 2,4);  
	in.s_addr=ip;  

	return inet_ntoa(in);
}

int fd_close_onexec(int fd)
{
	int flags;
	int dummy = 0;
	if ((flags = fcntl(fd, F_GETFL, dummy)) < 0) {
		return -1;
	}

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0)
		return -1;

	return 0;
}

int convert_G_B(char *s,uint32_t *gb,uint32_t *byte){
	char *a,*b;
	char di[128];
	memset(di,0,128);

	a = b = s;
	while(*b && isdigit(*b)) b++;
	memcpy(di,a,b-a);

	uint32_t num = strtol((const char *)s, (char **)NULL, 10);

	*gb = *byte = 0;
	switch(*b){
		case 'G':
		case 'g':
			*gb = num;
			break;
		case 'M':
		case 'm':
			*byte = num * 1024 * 1024;
			break;
		case 'K':
		case 'k':
			*byte = num * 1024;
			break;
		case 'B':
		case '\0':
			*byte = num;
			break;
		default:
			break;
	};

	return 0;
}

void backtrace_info()
{
	static void *(array[30]);
//	char **info;

	int n;

	n = backtrace(array, 30);
	backtrace_symbols_fd(array, n, fileno(debug_log));

//	info = backtrace_symbols(array, n);

//	for(i = 0; i < n; i++)
//		cclog(0, "%s\n", info[i]);

//	free(info);

	return;
}

unsigned long ELFhash(const char *key)
{
	unsigned long h = 0;
	
	while(*key)
	{
		h = (h<<4) + *key++;

		unsigned long g = h & 0Xf0000000L;

		if(g)
			h ^= g >> 24;
		h &= ~g;
	}

	return h;
}

/*
 *  xstrncpy() - similar to strncpy(3) but terminates string
 *  always with '\0' if (n != 0 and dst != NULL),
 *  and doesn't do padding
 */     
char *  
xstrncpy(char *dst, const char *src, size_t n)
{       
        char *r = dst;
        if (!n || !dst)
                return dst;
        if (src)
                while (--n != 0 && *src != '\0')
                        *dst++ = *src++;
        *dst = '\0';
        return r;
}       

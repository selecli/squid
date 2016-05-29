/*
 * $Id$
 * author : liwei(liwei@chinacache.com)
 * desc   : some utils
 */

#include "refresh_header.h"
#include "log.h"
#include "squid_enums.h"
#include <execinfo.h>

#define inaddrr(x) (*(struct in_addr *) &ifr->x[sizeof sa.sin_port])
#define IFRSIZE   ((int)(size * sizeof (struct ifreq)))

extern FILE* debug_log;

char* get_peer_ip(int socket_fd) {
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
			if (strstr(pid, "refreshd")!= NULL) {
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
		printf("refreshd is running, process id:%d\n",pid);

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

int action_atoi(char *action){
	if(strcasecmp(action,"url_purge") == 0)
		return RF_FC_ACTION_URL_PURGE;
	if(strcasecmp(action,"url_expire") == 0)
		return RF_FC_ACTION_URL_EXPIRE;
	if(strcasecmp(action,"dir_purge") == 0)
		return RF_FC_ACTION_DIR_PURGE;
	if(strcasecmp(action,"dir_expire") == 0)
		return RF_FC_ACTION_DIR_EXPIRE;

	return RF_FC_ACTION_UNKNOWN;
}

char *action_itoa(int action){
	static char *action_desc[] = {
		"unknow",
		"squid's info",
		"add",
		"verify_start",
		"verify_finish",
		"url_purge",
		"url_expire",
		"dir_purge",
		"dir_expire",
		"url_purge_return",
		"url_expire_return",
		"dir_purge_return",
		"dir_expire_return",
		"verify_return",
		"dir_expire_refresh_prefix_start",
		"dir_expire_refresh_wildcart_start",
		"dir_purge_refresh_prefix_start",
		"dir_purge_refresh_wildcard_start",
		"dir_refresh_finish",
		"dir_refresh_result",
	};

	return action_desc[action];
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

int rf_fc_gen_buffer(char *buffer,int action,char *url,char *inte0,int *inte1){
	int len = 0;
	len += sprintf(buffer + len,"%d",action);	
	buffer[len++] = 0;

	if(url != NULL)
		len += sprintf(buffer + len,"%s",url);
	buffer[len++] = 0;

	if(inte0 != NULL)
		len += sprintf(buffer + len,"%s",inte0);
	buffer[len++] = 0;

	if(inte1 != NULL)
		len += sprintf(buffer + len,"%d",*inte1);
	buffer[len++] = 0;
	len += sprintf(buffer + len,"\n");

	return len;
}

char *rf_http_header(int len){
	static char buffer[256];
	memset(buffer,0,256);

	sprintf(buffer,"HTTP/1.0 200 OK\nContent-Length: %d\r\n\r\n",len);

	return buffer;
}

char *rf_http_header_post(int len){
	static char buffer[256];
	memset(buffer,0,256);
	int b_len = 0;

	b_len +=sprintf(buffer + b_len,"POST /receiveService HTTP/1.1\r\n");
	b_len +=sprintf(buffer + b_len,"Connection: Keep-Alive\r\n");
	b_len +=sprintf(buffer + b_len,"Host:localhost\r\n");
	b_len +=sprintf(buffer + b_len,"Content-Length: %d\r\n\r\n",len);

	return buffer;
}

int rf_url_match_prefix(char *token,char *url){
	return strncasecmp(token,url,strlen(token));
}

int rf_url_match_regex(char *token,char *url){
	return fnmatch(token, url, 0);
}
/*
char *rf_get_db_file(char *squid_id){
	char *ident = squid_id;
	if(ident == NULL)
		ident = "refreshd";

	static char db_file[256];
	memset(db_file,0,256);

	sprintf(db_file,"%srf_%s.db",REFRESH_DB_PATH,ident);
	return db_file;
}
*/
char *rf_get_db_file(char *squid_id, int index)
{
	char *ident = squid_id;
	if(ident == NULL)
		ident = "refreshd";

	static char db_file[256];
	memset(db_file, 0, 256);

	if(db_num != 1)
		sprintf(db_file, "%srf_%s_%02d.db", REFRESH_DB_PATH, ident, index);
	else
		sprintf(db_file,"%srf_%s.db",REFRESH_DB_PATH,ident);

	return db_file;
}

char *rf_get_db_bak_file(char *squid_id, int index)
{
	char *ident = squid_id;
	if(ident == NULL)
		ident = "refreshd";

	static char db_file[256];
	memset(db_file, 0, 256);

	if(db_num != 1)
		sprintf(db_file, "%srf_%s_%02d.db.bak", REFRESH_DB_PATH, ident, index);
	else
		sprintf(db_file,"%srf_%s.db.bak",REFRESH_DB_PATH,ident);
		
	return db_file;
}
/*
char *rf_get_db_filebak(char *squid_id){
	char *ident = squid_id;
	if(ident == NULL)
		ident = "refreshd";

	static char db_file[256];
	memset(db_file,0,256);

	sprintf(db_file,"%srf_%s.db.bak",REFRESH_DB_PATH,ident);
	return db_file;
}
*/

void check_db_handlers(rf_client *rfc, char* squid_id)
{
	int i;

	for(i = 0; i < db_num; i++)
	{
		if(rfc->db_handlers[i] == NULL)
		{
			if(rfc->db_files[i] == NULL)
			{
				char *db_file = rf_get_db_file(squid_id, i);
				rfc->db_files[i] = cc_malloc(strlen(db_file) + 1);
				strcpy(rfc->db_files[i], db_file);
        
                        	cclog(3,"fd(%d),db_file : %s",rfc->fd,rfc->db_files[i]);
                	}
        		
			rfc->db_handlers[i] = get_db_handler(rfc->db_files[i]);
        	}
	}
}

void check_db_bak_handlers(rf_client *rfc, char* squid_id)
{
        int i;

        for(i = 0; i < db_num; i++)
        {
                if(rfc->db_bak_handlers[i] == NULL)
                {
                        if(rfc->db_bak_files[i] == NULL)
                        {
                                char *db_bak_file = rf_get_db_bak_file(squid_id, i);
                                rfc->db_bak_files[i] = cc_malloc(strlen(db_bak_file) + 1);
                                strcpy(rfc->db_bak_files[i], db_bak_file);

                                cclog(3,"fd(%d),db_bak_file : %s",rfc->fd,rfc->db_bak_files[i]);
                        }

                        rfc->db_bak_handlers[i] = get_db_handler(rfc->db_bak_files[i]);
                }
        }
}

int fd_setnonblocking(int fd) {
	int flags;
	int dummy = 0;
	if ((flags = fcntl(fd, F_GETFL, dummy)) < 0) {
		return -1;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		return -1;
	}
	return 0;
}

int string_split(const char *string,const char *delimiter,char *prefix,char *suffix){
	//string == http://www.sohu.com/a.txt@#@gzip --> 0
	//string == http://www.sohu.com/a.txt@#@ --> 1
	//string == http://www.sohu.com/a.txt --> -1
	char *begin = NULL;
	if((begin = strstr(string,delimiter)) == NULL){
		return -1;
	}

	memcpy(prefix,string,begin - string);
	if(strlen(begin + strlen(delimiter)) > 0){
		strcpy(suffix,begin + strlen(delimiter));
		return 0;
	}

	//suffix is empty !
	return 1;
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

unsigned long ELFhash(char *key)
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

int get_url_filenum(char *url)
{
	if(url == NULL)
		goto err;

	static char hostname[1024];
	if(rf_url_hostname(url, hostname) == NULL)
		goto err;

	return (int)(ELFhash(hostname) % DB_NUM);

err:
	cclog(0, "input url: %s format error, please use format like: http://hostname/.... \n", url);
	return -1;
}

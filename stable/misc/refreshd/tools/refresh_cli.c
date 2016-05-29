#include "refresh_header.h"
#include <netdb.h>

#define L_BUFFER_LEN 204800
#define M_BUFFER_LEN 2048
#define S_BUFFER_LEN 128
#define M_LIST_SIZE	128

//#define DEBUG

struct _spe_char{
	char ch;
	char *spec;
};

struct _spe_char sclist[] = {
	{'&',	"&amp;"},
	{0,NULL}
};

static int port = 21108;
static char ip[S_BUFFER_LEN] = "127.0.0.1";
static char report_address[S_BUFFER_LEN] = "";
static char method[S_BUFFER_LEN];
static char url[L_BUFFER_LEN];
static char params[S_BUFFER_LEN] = "0";
static int fd = -1;
static bool keep_alive = false;
static int timeout = 15; //default

void usage(){
	/**
	 * modify by sw
	 fprintf(stdout,"usage : refresh_cli\n"
	 "\t-h [ip or hostname] \n"
	 "\t-p [port] \n"
	 "\t-m [method] \n"
	 "\t\turl_purge : purge url\n"
	 "\t\turl_expire: expire url\n"
	 "\t\tdir_purge : dir purge \n"
	 "\t\tdir_expire: dir expire\n"
	 "\t-k 	keep alive\n"
	 "\t-s [params]\n"
	 "\t\turl : 1 - recursion , 0 - no recurse\n"
	 "\t\tdir : 1 - glob ,      0 - dir prefix\n"
	 "\t-r [url]\n"
	 "\t-t [timeout,second]\n"
	 );
	 */
	fprintf(stdout,"usage : refresh_cli\n"
			"\t-h [ip or hostname] \n"
			"\t-p [port] \n"
			"\t-f  - purge one url\n"
			"\t-e  - expire one url\n"
			"\t-d -f  - purge one dir\n"
			"\t-d  - expire one dir\n"
			"\t-k 	keep alive\n"
			"\t-s [params]\n"
			"\t\tdir : 1 - glob ,      0 - dir prefix\n"
			"\t-t [timeout,second]\n"
			"\t-r [report_address,ip]\n"
			"\t url\n"
		   );


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

int check_socket_valid(const int sockfd)
{
	int sock_errno = 0;
	socklen_t sockerr_len = sizeof(sock_errno);

	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sock_errno, &sockerr_len) < 0)
	{   
		perror("getsockopt():");
		return 0;
	}       

	return (0 == sock_errno || EISCONN == sock_errno);
}

struct in_addr * atoaddr(char *address) {
	struct hostent *host;
	static struct in_addr saddr;

	saddr.s_addr = inet_addr(address);
	if (saddr.s_addr != -1) {
		return &saddr;
	}
	host = gethostbyname(address);
	if (host != NULL) {
		return (struct in_addr *) *host->h_addr_list;
	}
	return NULL;
}

static void options_parse(int argc, char **argv)
{
	int dir_expire = 0;
	int purge = 0;

	if(argc < 2) {
		usage();
		exit(-1);
	}

	int c;
	//there must be a method, set to empty first
	memset(method,0,S_BUFFER_LEN);

	while(-1 != (c = getopt(argc, argv, "kp:h:s:t:r:def"))) {
		switch(c) {
			case 'p':
				if(optarg) {
					port = atoi(optarg);
				}
				else{
					usage();
					exit(-1);
				}
				break;
			case 'h':
				if(optarg){
					memset(ip,0,S_BUFFER_LEN);
					strcpy(ip,optarg);
				}
				else{
					usage();
					exit(-1);
				}

				break;
			case 'k':
				keep_alive = true;
				break;
				/**
				 * deleted by sw
				 case 'm':
				 if(optarg){
				 memset(method,0,S_BUFFER_LEN);
				 strcpy(method,optarg);
				 }
				 else{
				 usage();
				 exit(-1);
				 }

				 if(strcmp(method,"url_purge") && strcmp(method ,"url_expire") && strcmp(method,"dir_purge") && strcmp(method,"dir_expire")){
				 fprintf(stderr,"method error!\n");
				 usage();
				 exit(-1);
				 }
				 break;
				 */
			case 's':
				if(optarg){
					memset(params,0,S_BUFFER_LEN);
					strcpy(params,optarg);
				}
				else{
					usage();
					exit(-1);
				}
				break;
				/**
				 * deleted by sw
				 case 'r':
				 if(optarg){
				 memset(url,0,L_BUFFER_LEN);
				 strcpy(url,optarg);
				 }
				 else{
				 usage();
				 exit(-1);
				 }
				 break;
				 */

			case 't':
				if(optarg){
					int tout = strtol(optarg, (char **)NULL, 10);
					if(tout > 0)
						timeout = tout;
				}

				break;
				/** 
				 * add by sw
				 * expire one dir
				 */
			case 'd':
				//	strcpy(method,"dir_expire");
				dir_expire = 1;
				break;
				/**
				 * add by sw
				 * expire one url
				 */
			case 'e':
				strcpy(method,"url_expire");
				break;
				/**
				 * add by sw
				 * purge one url
				 */
			case 'f' :
				//	strcpy(method,"url_purge");
				purge = 1;
				break;
			case 'r' :
				if(optarg){
					strcpy(report_address,optarg);
				}

				break;
			default:
				fprintf(stderr, "Unknown arg: %c\n", c);
				usage();
				exit(2);
				break;
		}
	}

	if((dir_expire == 1) && (purge == 1))
	{
		strcpy(method, "dir_purge");
	}
	else if(dir_expire == 1)
	{
		strcpy(method, "dir_expire");
	}
	else if(purge == 1)
	{
		strcpy(method, "url_purge");
	}

	if(! *method){
		usage();
		exit(-1);
	}
	/**
	 * add by sw
	 * get the last param which is the url or dir(equals -r)
	 */
	memset(url,0,L_BUFFER_LEN);
	strcpy(url,argv[argc - 1]);

}
int rf_write(int fd,char *buffer,uint32_t len){
	int s_len = 0;
	int r_len = len;

	while(r_len > 0){
		s_len = write(fd,buffer,r_len);
		if(s_len > 0 ){
			r_len -= s_len;
			continue;
		}

		if(s_len == -1){
			fprintf(stderr,"write error : %s\n",strerror(errno));
			return s_len;
		}
	}

	return r_len;
}

int rf_read(int fd)
{
	int len = 0;
	int count = 0;
	static char buffer[L_BUFFER_LEN];
	memset(buffer,0,L_BUFFER_LEN);

	printf("----------------------------------------\n");

	if (!check_socket_valid(fd))
	{
		fprintf(stderr, "socket fd invalid before recv");
		return 0;
	}
	while((len = recv(fd,buffer,L_BUFFER_LEN,0)) != 0){
		if(len > 0) {
			write(2, buffer, len);
		} else if (len == 0) {
			//over, break
			break;
		} else {
			if(ignore_errno(errno))
			{
				//300s: 6000 * 50000
				if (++count > 6000)
				{
					fprintf(stderr, "recv error!");
					break;
				}
				usleep(50000);
				continue;
			}
			fprintf(stderr, "recv error!");
			break;
		}
	}
	printf("\n");

	return 0;
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

int fd_connect(){
	struct sockaddr_in  maddress;
	memset(&maddress, 0, sizeof(struct sockaddr_in));
	struct in_addr *ina = atoaddr(ip);
	if(ina == NULL)
		return -1;

	maddress.sin_addr = *ina;
	maddress.sin_family = AF_INET;
	maddress.sin_port   = htons(port);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if( fd < 0 ) {
		return -1;
	}

	int ret = fd_setnonblocking(fd);
	if(ret != 0){
		close(fd);
		return ret;
	}

	int n = 0;
	if( (n = connect(fd, (struct sockaddr *)&maddress, sizeof(struct sockaddr))) != 0) {
		if(errno != EINPROGRESS){
			perror("connect:");
			return -1;
		}
	}

	return 0;
}

int get_pos_sclist(char x){
	int i= 0;
	for(;sclist[i].ch; i++){
		if(x == sclist[i].ch){
			return i;
		}
	}

	return -1;
}

int main(int argc,char *argv[]){
	options_parse(argc,argv);
	fd_connect();

	fd_set rfds;
	fd_set wfds;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);

	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	int ret = select(fd + 1, NULL,&wfds, NULL, &tv);
	if(ret < 0){
		perror("select");
		exit(-1);
	}
	else if(ret == 0){
		printf("connect timeout : %d\n",timeout);
		exit(-1);
	}

	char buffer[L_BUFFER_LEN];
	char xml_buffer[L_BUFFER_LEN];
	memset(buffer,0,L_BUFFER_LEN);
	memset(xml_buffer,0,L_BUFFER_LEN);
	int len = 0;
	int http_len = 0;

	time_t ti = time(NULL);

	len += sprintf(xml_buffer + len,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<method name=\"%s\" sessionid=\"%ld\">\n",method,ti);

	if((strcmp(method,"url_expire") == 0) || (strcmp(method,"url_purge") == 0)){
		len += sprintf(xml_buffer + len,"<recursion>%s</recursion>\n",params);
		len += sprintf(xml_buffer + len,"<url_list>\n");

		int id = 0;
		int in = 0;
		int a_in = 0;
		char tmp_buf[M_BUFFER_LEN];
		int url_number = 0;

		while(url[in++]){
			if(url[in] == '&'){
				int pos = get_pos_sclist('&');
				assert(pos != -1);

				if(strncasecmp(sclist[pos].spec,url + in ,strlen(sclist[pos].spec))){
					memset(tmp_buf,0,M_BUFFER_LEN);
					strcpy(tmp_buf,url + in + 1);
					memcpy(url + in,sclist[pos].spec,strlen(sclist[pos].spec));
					memcpy(url + in + strlen(sclist[pos].spec),tmp_buf,strlen(tmp_buf));
				}

				in += strlen(sclist[pos].spec) -1;
				continue;
			}

			if(url[in] == ';' || url[in] == '\0'){
				memset(tmp_buf,0,M_BUFFER_LEN);
				memcpy(tmp_buf,url + a_in,in - a_in);

				if((len + strlen(tmp_buf)) > L_BUFFER_LEN){
					fprintf(stderr,"url number is too huge!\n\n");
					exit(-1);
				}

				printf("%s\n",tmp_buf);

				len += sprintf(xml_buffer + len,"<url id=\"%d\">%s</url>\n",id++,tmp_buf);
				url_number++;

				while(url[in] == ';') in++;		//skip ';'
				a_in = in;

			}

			if(url[in] == '\0')
				break;
		}

		if(url_number == 0){
			fprintf(stderr,"error -> do not find url\n");
			exit(-1);
		}

		len += sprintf(xml_buffer + len,"</url_list>\n");
	}
	else{
		len += sprintf(xml_buffer + len ,"<action>%s</action>\n",params);
		len += sprintf(xml_buffer + len ,"<dir>%s</dir>\n",url);
		if(strlen(report_address) > 0){
			len += sprintf(xml_buffer + len ,"<report_address>%s</report_address>\n",report_address);
		}
	}
	len += sprintf(xml_buffer + len,"</method>");

	http_len += sprintf(buffer + http_len,"POST http://www.test.com.cn http/1.1\r\n");
	http_len += sprintf(buffer + http_len,"Content-Length :%d\r\n",len);
	if(keep_alive){
		http_len += sprintf(buffer + http_len,"Connection: Keep-Alive\r\n");
	}
	http_len += sprintf(buffer + http_len,"\r\n");
	http_len += sprintf(buffer + http_len,"%s",xml_buffer);

#ifdef DEBUG
	write(2,buffer,http_len);
#else
	rf_write(fd,buffer,http_len);

	FD_SET(fd, &rfds);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	ret = select(fd + 1, &rfds,NULL, NULL, &tv);
	if(ret < 0){
		perror("select");
		exit(-1);
	}
	else if(ret == 0){
		printf("read timeout : %d\n",timeout);
		exit(-1);
	}

	rf_read(fd);
#endif

	return 0;
}

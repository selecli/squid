#include "request.h"
#include "display.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>  
#include <fcntl.h>
#include <stdarg.h>

#ifdef HTTPS
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#define debug(a, s, args...)	_debug((a), (char *)__FILE__, (int)__LINE__, (char *)__func__, s, ## args)
void _debug(int level, char * file, int line, char * func, char * format, ...);

static struct Request g_stRequest;
#define BUFFER_SIZE 4095
static char g_szBuffer[BUFFER_SIZE+1];

static char g_szFilename[256];

int set_no_blocking(int fd) 
{
	int flags;
	int dummy = 0;

	if ((flags = fcntl(fd, F_GETFL, dummy)) < 0) {
		debug(1, "set_no_blocking: %d fcntl F_GETFL: %s\n", fd, strerror(errno));
		return -1; 
	}   

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		debug(1, "set_no_blocking: FD %d: %s\n", fd, strerror(errno));
		return -1; 
	}   
	return 1;
}

int set_blocking(int fd) 
{
	int flags;
	int dummy = 0;
	
	if ((flags = fcntl(fd, F_GETFL, dummy)) < 0) 
	{ 
		debug(1, "set_blocking: %d fcntl F_GETFL: %s\n", fd, strerror(errno));
		return -1; 
	}


	if (fcntl(fd, F_SETFL, flags & (~O_NONBLOCK)) < 0) 
	{ 
		debug(1, "set_blocking: FD %d: %s\n", fd, strerror(errno));
		return -1; 
	}

	return 1;
}

int safe_connect(int sockfd, const struct sockaddr *address, int address_len)
{

	int retry_times = 0;
	int ret = 0;


	while(retry_times < 5)
	{
		ret = connect(sockfd, address, address_len);
		 
        if( -1 == ret)
		{
			usleep(2000);
			retry_times++;		
		}	
		return 1;
		
	}
	debug(1,"retry 5 times ,each 2ms, connect timout !!");
	return -1;
}

int send_data(int fd, void *buffer,int length) 
{
	int bytes_left = 0;
	int written_bytes = 0;
	char *ptr=NULL;
	int retry_times = 0;

	ptr=buffer;
	bytes_left = length; 

	if (length == 0)
		return 0;   

	while (bytes_left > 0) {
		written_bytes = write(fd, ptr, bytes_left);

		if (written_bytes == 0)
			return length - bytes_left;

		if(written_bytes < 0) {       
			if((errno == EINTR) || (errno == EAGAIN)) {
				usleep(50000);
				retry_times++;

				//timeout 1s
				if(retry_times > 20){
					debug(1,"retry 20,write timout !!");
					return -1;
				}
			} else {
				debug(1,"write to %d error %d,%s\n",fd,errno,strerror(errno));
				return -1;
			}
		}
		else{
			bytes_left -= written_bytes;
			ptr += written_bytes;
		}
	}

	return length;
}

int safe_read(int fd,void *buffer,int length)
{
	int bytes_left = length;
	int bytes_read = 0;
	char * ptr = buffer;
	int retry_times = 0;

	while (bytes_left > 0) {
		bytes_read = recv(fd, ptr, bytes_left, 0);
//		bytes_read = read(fd, ptr, bytes_left);
		if (bytes_read < 0) {
			if((errno == EINTR) || (errno == EAGAIN)) {
				usleep(50000);
				retry_times++;

				//timeout 5s
				if(retry_times > 100)
					return -1;
			} else 
				return -1;
		} else if (bytes_read == 0){
			//peer close the socket
			return 0;
		}
		else{
			bytes_left -= bytes_read;
			ptr += bytes_read;
		}
	}

	return length - bytes_left;
}

void _debug(int level, char * file, int line, char * func, char * format, ...)
{
	va_list list;
	char infoLogFileName[128];
	memset(infoLogFileName, 0, sizeof(infoLogFileName));
	strcpy(infoLogFileName, "/var/log/chinacache/detectorigin_info.log");

	FILE* fp = fopen(infoLogFileName, "a+");
	if(NULL == fp)
	{

	}
	{
		fprintf(fp, "\n[%s][%d][%s]\n", file, line, func);
		va_start(list,format);
		vfprintf(fp, format, list);
		va_end(list);
	}
	fflush(fp);
	fclose(fp);
}

ssize_t ReadSocket(int SocketFD, void *ReadBuff, size_t ReadLen)
{

        ssize_t nread;
        size_t nleft = ReadLen;
        unsigned char*  des = (unsigned char*)ReadBuff;

        while (nleft > 0) {
                nread = read(SocketFD, des, nleft);
                if (nread < 0) {
                        if (errno == EINTR)
                                continue;
                        return -13;
                } else if(nread == 0)
                        break;

                des += nread;
                nleft -= nread;
        }

        return (ReadLen - nleft);
}
#ifdef HTTPS
ssize_t ReadHttpsSocket(BIO * bio, void *ReadBuff, size_t ReadLen)
{

        ssize_t nread;
        size_t nleft = ReadLen;
        unsigned char*  des = (unsigned char*)ReadBuff;

        while (nleft > 0) {
   //             nread = read(SocketFD, des, nleft);
				nread = BIO_read(bio, des, nleft);
                if (nread < 0) {
                        if (errno == EINTR)
                                continue;
                        return -13;
                } else if(nread == 0)
                        break;

                des += nread;
                nleft -= nread;
        }

        return (ReadLen - nleft);
}
#endif

ssize_t WriteSocket(int SocketFD, void *WirteBuff, size_t WriteLen)
{
        size_t Left = WriteLen;
        unsigned char *des = (unsigned char*)WirteBuff;
        size_t nwrite;

        while( Left > 0 ) {
                nwrite = write(SocketFD, des, Left);
                if(nwrite < 0) {
                        if( errno == EINTR )
                                continue;
                        return -12;
                } else if(nwrite == 0)
                        break;

                des += nwrite;
                Left -= nwrite;
        }

        return (WriteLen - Left);
}

void signalAlarm(int sig)
{
//	debug(1,"signalAlarm======");
}
//登记信号
void registerSignal()
{
	struct sigaction act;

	act.sa_handler = signalAlarm;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction(SIGALRM, &act, 0);
}

/*由主机名得到一个ip地址
 *输入:
 *    host----主机名
 *返回值:
 *    0----失败
 *    其它----对应的ip地址
*/
unsigned int getIpFromHost(char* host)
{
	struct hostent *hptr = gethostbyname(host);
	if(NULL == hptr)
	{
		return 0;
	}
	if(AF_INET != hptr->h_addrtype)
	{
		return 0;
	}
	if(NULL == *(hptr->h_addr_list))
	{
		return 0;
	}
	return *(unsigned int*)(*(hptr->h_addr_list));

}

/*从一个配置行得到一个请求
 *输入：
 *    line----配置行。每一项以\t分隔，可以多个。各项依次是ip、port、类型（get,post）、文件名、主机、头像（0到多个）、post的内容（get时没有）
 *输出：
 *    pstRequest----请求的分析结果
 *返回值：
 *    0----合格请求
 *    -1----不合格请求
*/
int getOneRequestFromInput(char* line, struct Request* pstRequest)
{
	char* str;
	int ret;

	//ip
	if( (str = strtok(line, "\t")) == NULL ) {
		printf("get ip error.\n");
		return -1;
	}
	if(0 != atoi(str)) {//有ip
		if( inet_pton(AF_INET, str, &(pstRequest->ip)) <= 0 ) {
			printf("get ip error.\n");
			return -1;
		}
	}


	//port
	if( (str = strtok(NULL, "\t")) == NULL ) {
		printf("get port error.\n");
		return -1;
	}
	pstRequest->port = htons(atoi(str));


	//get type
	if( (str = strtok(NULL, "\t")) == NULL ) {
		printf("get type error.");
		return -1;
	}
	if(0 == strcasecmp(str, "GET"))
	{
		pstRequest->type = 1;
	} else if(0 == strcasecmp(str, "POST"))
	{
		pstRequest->type = 2;
	} else if(0 == strcasecmp(str, "HEAD"))
	{
		pstRequest->type = 3;
	} else {
		printf("get type error.");
		return -1;
	}


	//get filename
	if( (str = strtok(NULL, "\t")) == NULL ) {
		printf("get filename error.");
		return -1;
	}
	snprintf(pstRequest->filename, 512, "%s", str);


	//get host
	if( (str = strtok(NULL, "\t")) == NULL ) {
		printf("get host error.");
		return -1;
	}
	snprintf(pstRequest->host, 128, "%s", str);

	//get length
	if( (str = strtok(NULL, "\t")) == NULL ) {
		printf("get host error.");
		return -1;
	}
	pstRequest->length = atoi(str);

	//get header
	str = strtok(NULL, "\t\r\n");
	if(NULL == str)
	{
		if((1==pstRequest->type) || (3==pstRequest->type))
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	ret = 0;
	char* str2;
	while(1)
	{
		str2 = strtok(NULL, "\t\r\n");
		if(NULL == str2)
		{
			break;
		}
		ret += snprintf(pstRequest->header+ret, MAX_HEAD_LENGTH-ret, "%s\r\n", str);
		str = str2;
	}
	if(2 == pstRequest->type)
	{
		snprintf(pstRequest->content, MAX_CONTENT_LENGTH, "%s", str);
	}
	else
	{
		ret += snprintf(pstRequest->header+ret, MAX_HEAD_LENGTH-ret, "%s\r\n", str);
	}

	return 0;
}

/*处理一个请求
 *输入：
 *    pstRequest----请求内容
 *    fgHeader----是否打印头到标准输出。0--不打印
 *    filename----响应保存到的文件名
 *    length----保存的最大长度
 *    tv----网络传输最大延迟
 *输出：
 *    responseFirstLine----响应的第一行
 *返回值：
 *    -1----失败
 *    整数----保存的文件的大小
*/
int dealwithOneRequest(struct Request* pstRequest, int fgHeader, char* filename, int length, const struct timeval* tv, unsigned int inspect, char* responseFirstLine)
{
	int ret;
	struct timeval stTimeval1;
	struct timeval stTimeval2;
	double usedtime;
	int sockfd;

	registerSignal();
	//如果没有指定ip，gethostbyname解析
	if (0 == pstRequest->ip) {

		gettimeofday(&stTimeval1, NULL);
		if( (pstRequest->ip = getIpFromHost(pstRequest->host)) == 0 )
			return -1;
		gettimeofday(&stTimeval2, NULL);

		if( inspect & DNS_INSPECT ) {
			usedtime = stTimeval2.tv_sec-stTimeval1.tv_sec + (stTimeval2.tv_usec-stTimeval1.tv_usec)/1000000.0;
			fprintf(stdout, "DNS Lookiup: %.6f\n", usedtime);
		}
	}

	if( ( sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		return -1;
/*
	int nNetTimeout = 3000;//3e秒
	//发送时限
	ret = setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,(char *)&nNetTimeout,sizeof(int));
	if(ret < 0 ){
		debug(1,"setsockopt error!");
		debug(1,strerror(errno));
	}
	////接收时限
	ret = setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char *)&nNetTimeout,sizeof(int));
	if(ret < 0 ){
		debug(1,"setsockopt error!");
		debug(1,strerror(errno));
	}
*/
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = pstRequest->port;
	servaddr.sin_addr.s_addr = pstRequest->ip;

	//起动定时器
	struct itimerval itimer;
	memset(&itimer, 0, sizeof(itimer));
	itimer.it_value.tv_sec = tv->tv_sec;
	itimer.it_value.tv_usec = tv->tv_usec;

	setitimer(ITIMER_REAL, &itimer, NULL);

	gettimeofday(&stTimeval1, NULL);
#ifdef HTTPS
	if ( 443 == ntohs(pstRequest->port))
	{
		/* Initializing OpenSSL */
		SSL_load_error_strings();
		ERR_load_BIO_strings();
		OpenSSL_add_all_algorithms();
		
		SSL_CTX * ctx = SSL_CTX_new(SSLv23_client_method());
		SSL * ssl;


		/* then call this from within the application */
		if(! SSL_CTX_load_verify_locations(ctx, NULL, "/usr/local/squid/etc/key.perm"))
		{
				    /* Handle error here */
			fprintf(stderr,"SSL_CTX_load_verify_locations error\n");
		}
		BIO * bio;
		bio = BIO_new_ssl_connect(ctx);
		BIO_get_ssl(bio, & ssl);
		SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	  	if(bio == NULL) 
		{
			fprintf(stderr,"BIO is null\n"); 
			return -1; 
		}
		/* Attempt to connect */
		char host_port[1024];
		snprintf(host_port,1024,"%s:%d",pstRequest->host,ntohs(pstRequest->port));
		BIO_set_conn_hostname(bio, host_port);
		/* Verify the connection opened and perform the handshake */

	    if(BIO_do_connect(bio) <= 0)

		{
			fprintf(stderr,"BIO connect error\n"); 
			BIO_free_all(bio);
			return -1;
		}

		if(SSL_get_verify_result(ssl) != X509_V_OK)
		{
		/* Handle the failed verification */
	//		fprintf(stderr,"SSL_get_verify_result error\n");
		}

		gettimeofday(&stTimeval2, NULL);

		if( inspect & CONNECT_INSPECT ) {
			usedtime = stTimeval2.tv_sec-stTimeval1.tv_sec + (stTimeval2.tv_usec-stTimeval1.tv_usec)/1000000.0;
			fprintf(stdout, "Initial Connection Time: %.6f\n", usedtime);
		}

		//得到请求，假定请求长度小于BUFFER_SIZE

		char *type[] = {"GET ", "POST ", "HEAD ", ""};
		if( pstRequest->type < 1 || pstRequest->type > 3 ) {
			setitimer(ITIMER_REAL, NULL, NULL);
			BIO_free_all(bio);
			return -1;
		}
		strcpy(g_szBuffer, type[pstRequest->type-1]);


		//不判断host和filename是否为NULL
		snprintf(g_szBuffer + strlen(g_szBuffer), BUFFER_SIZE - strlen(g_szBuffer), "%s HTTP/1.1\r\nHost: %s\r\n", pstRequest->filename, pstRequest->host);
		snprintf(g_szBuffer + strlen(g_szBuffer), BUFFER_SIZE - strlen(g_szBuffer), "%s", pstRequest->header);


		if( strcmp(type[pstRequest->type-1], "POST ") == 0 ) {
			if(NULL == pstRequest->content)
				snprintf(g_szBuffer + strlen(g_szBuffer), BUFFER_SIZE - strlen(g_szBuffer), "Content-Length: 0\r\n\r\n");
			else
				snprintf(g_szBuffer + strlen(g_szBuffer), BUFFER_SIZE - strlen(g_szBuffer), "Content-Length: %d\r\n\r\n%s", (int32_t)(strlen(pstRequest->content)), pstRequest->content);
		} else {
			snprintf(g_szBuffer + strlen(g_szBuffer), BUFFER_SIZE - strlen(g_szBuffer), "\r\n");
		}

		gettimeofday(&stTimeval1, NULL);
		if( BIO_write(bio, g_szBuffer, strlen(g_szBuffer)) < 0) {
			setitimer(ITIMER_REAL, NULL, NULL);
            BIO_free_all(bio);  
			return -1;
		}
		gettimeofday(&stTimeval2, NULL);

		if( inspect & WRITE_INSPECT ) {
			usedtime = stTimeval2.tv_sec-stTimeval1.tv_sec + (stTimeval2.tv_usec-stTimeval1.tv_usec)/1000000.0;
			fprintf(stdout, "Send Request: %.6f\n", usedtime);
		}


		//不需要结果
		if((0==fgHeader) && (0==length)) {
			setitimer(ITIMER_REAL, NULL, NULL);
			close(sockfd);
			return -1;
		}


		//=======================================================================================

		gettimeofday(&stTimeval1, NULL);
		if( ReadHttpsSocket(bio, g_szBuffer, 1024) < 0 ) {

			setitimer(ITIMER_REAL, NULL, NULL);
            BIO_free_all(bio);  
			close(sockfd);
			return -1;
		}
    	BIO_free_all(bio);  
	}
	else
	{
#endif

		set_no_blocking(sockfd);
		if( (ret = safe_connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr))) < 0 ) {
			setitimer(ITIMER_REAL, NULL, NULL);
			close(sockfd);
			return -1;
		}

		gettimeofday(&stTimeval2, NULL);

		if( inspect & CONNECT_INSPECT ) {
			usedtime = stTimeval2.tv_sec-stTimeval1.tv_sec + (stTimeval2.tv_usec-stTimeval1.tv_usec)/1000000.0;
			fprintf(stdout, "Initial Connection Time: %.6f\n", usedtime);
		}

		//得到请求，假定请求长度小于BUFFER_SIZE

		char *type[] = {"GET ", "POST ", "HEAD ", ""};
		if( pstRequest->type < 1 || pstRequest->type > 3 ) {
			setitimer(ITIMER_REAL, NULL, NULL);
			close(sockfd);
			return -1;
		}
		strcpy(g_szBuffer, type[pstRequest->type-1]);


		//不判断host和filename是否为NULL
		snprintf(g_szBuffer + strlen(g_szBuffer), BUFFER_SIZE - strlen(g_szBuffer), "%s HTTP/1.1\r\nHost: %s\r\n", pstRequest->filename, pstRequest->host);
		snprintf(g_szBuffer + strlen(g_szBuffer), BUFFER_SIZE - strlen(g_szBuffer), "%s", pstRequest->header);


		if( strcmp(type[pstRequest->type-1], "POST ") == 0 ) {
			if(NULL == pstRequest->content)
				snprintf(g_szBuffer + strlen(g_szBuffer), BUFFER_SIZE - strlen(g_szBuffer), "Content-Length: 0\r\n\r\n");
			else
				snprintf(g_szBuffer + strlen(g_szBuffer), BUFFER_SIZE - strlen(g_szBuffer), "Content-Length: %d\r\n\r\n%s", (int32_t)(strlen(pstRequest->content)), pstRequest->content);
		} else {
			snprintf(g_szBuffer + strlen(g_szBuffer), BUFFER_SIZE - strlen(g_szBuffer), "\r\n");
		}

		gettimeofday(&stTimeval1, NULL);
		if( send_data(sockfd, g_szBuffer, strlen(g_szBuffer)) < 0 ) {
			setitimer(ITIMER_REAL, NULL, NULL);
			close(sockfd);
			return -1;
		}
		gettimeofday(&stTimeval2, NULL);

		if( inspect & WRITE_INSPECT ) {
			usedtime = stTimeval2.tv_sec-stTimeval1.tv_sec + (stTimeval2.tv_usec-stTimeval1.tv_usec)/1000000.0;
			fprintf(stdout, "Send Request: %.6f\n", usedtime);
		}


		//不需要结果
		if((0==fgHeader) && (0==length)) {
			setitimer(ITIMER_REAL, NULL, NULL);
			close(sockfd);
			return -1;
		}


		//=======================================================================================
		gettimeofday(&stTimeval1, NULL);
		set_blocking(sockfd);
		if(recv(sockfd,g_szBuffer,1,MSG_PEEK) < 0)
		{
			setitimer(ITIMER_REAL, NULL, NULL);
			close(sockfd);
			return -1;
		}		
		gettimeofday(&stTimeval2, NULL); 
		if( inspect & READ_INSPECT ) 
		{
			usedtime = stTimeval2.tv_sec-stTimeval1.tv_sec+(stTimeval2.tv_usec-stTimeval1.tv_usec)/1000000.0;
			fprintf(stdout, "FirstByte: %.6f\n", usedtime);
		}

		gettimeofday(&stTimeval1, NULL);
		set_no_blocking(sockfd);
		if( safe_read(sockfd, g_szBuffer, 1024) < 0 ) {
			setitimer(ITIMER_REAL, NULL, NULL);
			close(sockfd);
			return -1;
		}
#ifdef HTTPS
	}
#endif
	gettimeofday(&stTimeval2, NULL);

	if( inspect & READ_INSPECT ) {
		usedtime = stTimeval2.tv_sec-stTimeval1.tv_sec + (stTimeval2.tv_usec-stTimeval1.tv_usec)/1000000.0;
		fprintf(stdout, "Receive Request: %.6f\n", usedtime);
	}

	close(sockfd);
	setitimer(ITIMER_REAL, NULL, NULL);

	char* str = strchr(g_szBuffer, '\n');
	if(NULL == str) {
		return -1;
	}
	str++;
	if(NULL != responseFirstLine)
	{
		memcpy(responseFirstLine, g_szBuffer, str-g_szBuffer);
		responseFirstLine[str-g_szBuffer] = 0;
	}

	return 0;
}









/*处理一个输入行
 *输入：
 *    line----输入行
 *    fgHeader----是否打印头到标准输出。0--不打印
 *    filename----响应保存到的文件名，如果为NULL，取请求对应的文件名
 *    length----保存的最大长度
 *    tv----最大时间限制
 *返回值：
 *    -1----失败
 *    整数----保存的文件的大小
*/
int dealwithOneLine(char* line, int fgHeader, char* filename, int length, const struct timeval* tv, unsigned int inspect)
{
	memset(&g_stRequest, 0, sizeof(g_stRequest));
	int ret = getOneRequestFromInput(line, &g_stRequest);
	if(-1 == ret)
	{
		fprintf(stdout, "\n");
		fflush(stdout);
		return -1;
	}

	if(-1 != g_stRequest.length)
	{
		length = g_stRequest.length;
	}
	//确保有文件名
	if(length > 0)
	{
		if(NULL == filename)
		{
			filename = strrchr(g_stRequest.filename, '/');
			if(NULL == filename)
			{
				fprintf(stdout, "\n");
				fflush(stdout);
				return -1;
			}
			filename++;
			if(0 == *filename)
			{
				strcpy(g_szFilename, "index.html");
			}
			else
			{
				strcpy(g_szFilename, filename);
			}
			filename = g_szFilename;
		}
		char* str = strchr(filename, '?');
		if(NULL != str)
		{
			*str = 0;
		}
	}
	char responseFirstLine[128];
	struct timeval stTimeval1;
	gettimeofday(&stTimeval1, NULL);
	ret = dealwithOneRequest(&g_stRequest, fgHeader, filename, length, tv, inspect, responseFirstLine);
	struct timeval stTimeval2;
	gettimeofday(&stTimeval2, NULL);
	double usedtime = stTimeval2.tv_sec-stTimeval1.tv_sec + (stTimeval2.tv_usec-stTimeval1.tv_usec)/1000000.0;
	if((ret>=0) && (fgHeader))
	{
		fprintf(stdout, "%.6f %s", usedtime, responseFirstLine);
		fflush(stdout);
	}
	else
	{
		fprintf(stdout, "%.6f\n", usedtime);
		fflush(stdout);
	}
	return ret;
}


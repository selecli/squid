#include "request.h"
//#include "display.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>

static const char helpStr[] =   "Usage:\n"
				"    -v, --version   display the version and exit\n"
				"    -h, --help      print the help\n"
				"\n"
				"There are two modes to run this process: one is command mode, \n"
				"request is gotton from command line, \n"
				"only one request is dealed in one process, \n"
				"the other mode is stdin mode, request is gotton by stdin, \n"
				"thus one process may deal many request.\n"
				"\n"
				"The command mode:\n"
				"   -d display_header   optional, print http response first line,\n"
				"                       2--display all head,1--display,0--not,default is 0\n"
				"   -w filename         optional, save the result, \n"
				"                       default the request's filename\n"
				"   -l length           optional, get response's max length,\n"
				"                       default is 1000M\n"
				"   --length length     optional, is same the previous\n"
				"   -t timer            optional, connect's max time, \n"
				"                       default is 3.000001s\n"
				"   --timer timer       optional, is same the previous\n"
				"   --ip ip             optional, the server's ip, \n"
				"                       default is gotton from hostname\n"
				"   --port port         optional, the server's port, \n"
				"                       default is 80\n"
				"   --type request      optional, type	the request type,\n"
				"                       default is get\n"
				"   --host hostname     optional, the server's domain, \n"
				"                       default is gotton from url\n"
				"   --file filename     the request's target file\n"
				"   --header Header     optional, http's header\n"
				"   --content content   the post's content, \n"
				"                       there is none if type is get\n"
				"   --url url           optional, request's url\n"
				"   --inspect argv      optional, inspect time. argv may be dns, connect, send,\n"
				"                       receive_first,receive,all. default inspect total time.\n"
				"                       one appear, it is inspected.\n"
				"   -i argv             optional, is same the previous\n"
				"\n"
				"The stdin mode:\n"
				"   --url stdin         know stdin mode\n"
				"   -d display_header   optional, print http response first line,\n"
				"                       2--display all head,1--display,0--not,default is 0\n"
				"   -w filename         optional, save the result, \n"
				"                       default the request's filename\n"
				"   -l length           optional, get response's max length, \n"
				"                       default is 1000M\n"
				"   --length length     optional, is same the previous\n"
				"   -t timer            optional, connect's max time, \n"
				"                       default is 3.000001s\n"
				"   --timer timer       optional, is same the previous\n"
				"   --inspect argv      optional, inspect time. argv may be dns, connect, write,\n"
				"                       read, all. default inspect total time.\n"
				"                       one appear, it is inspected.\n"
				"   -i argv             optional, is same the previous\n"
				"for exemple:\n"
				"./request --url  http://news.sina.com.cn/index.htm -d 2  -w /dev/null -t 5 -i all\n\n";


static int g_fgDisplayHeader = 0;
static int g_iResponseLength = 1024000000;
static struct timeval g_stTimer;
static char g_szResultFilename[256];
static int g_fgStdin = 0;
static unsigned int g_uiInspect = 0;

static int dealwithCommandLine(int argc, char* argv[])
{
	struct Request stRequest;
	memset(&stRequest, 0, sizeof(stRequest));
	memset(g_szResultFilename, 0, sizeof(g_szResultFilename));
	memset(&g_uiInspect, 0, sizeof(g_uiInspect));

	g_stTimer.tv_sec = 3;
	g_stTimer.tv_usec = 1;
	static struct option long_options[] = {
			{"ip",		1, 0, 0},
			{"port",	1, 0, 0},
			{"type",	1, 0, 0},
			{"host",	1, 0, 0},
			{"file",	1, 0, 0},
			{"header",	1, 0, 0},
			{"content",	1, 0, 0},
			{"url",		1, 0, 0},
			{"length",	1, 0, 'l'},
			{"version",	0, 0, 'v'},
			{"help",	0, 0, 'h'},
			{"inspect",	1, 0, 'i'},
			{"timer",	1, 0, 't'},
			{0, 0, 0, 0}
	};

	int opt;
	int option_index;
	int ret;
	double timer;


	while(1) {

		opt = getopt_long(argc, argv, "hvw:l:d:t:i:", long_options, &option_index);
		if(-1 == opt)
			break;

		switch(opt) {
			case 0:
				if(0 == option_index) {	//ip
					ret = inet_pton(AF_INET, optarg, &(stRequest.ip));
					if(ret <= 0) {
						fprintf(stderr, "ip error\n");
						return -1;
					}
				}

				if(1 == option_index) { // port
					stRequest.port = htons(atoi(optarg));
				}

				if(2 == option_index) { //type
					if(0 == strcasecmp(optarg, "GET")) {
						stRequest.type = 1;
					} else if(0 == strcasecmp(optarg, "POST")) {
						stRequest.type = 2;
					} else if(0 == strcasecmp(optarg, "HEAD")) {
						stRequest.type = 3;
					} else {
						fprintf(stderr, "request error\n");
						return -1;
					}
				}

				if(3 == option_index) //host
					snprintf(stRequest.host, 128, "%s", optarg);

				if(4 == option_index) //file
					snprintf(stRequest.filename, 512, "%s", optarg);


				if(5 == option_index) { //header
					strcat(stRequest.header, optarg);
					strcat(stRequest.header, "\r\n");
				}

				if(6 == option_index) //content
					snprintf(stRequest.content, 2048, "%s", optarg);

				if(7 == option_index) { //url

					if(0 == strcmp(optarg, "stdin")) {
						g_fgStdin = 1;
						break;
					}
					/*
					if(0 != memcmp(optarg, "http://", 7)) {
						fprintf(stderr, "url error: %s\n", optarg);
						return -1;
					}

					char* str = strchr(optarg+7, '/');

					if(NULL == str) {
						fprintf(stderr, "url error\n");
						return -1;
					}

					if(0 == stRequest.host[0])
					{
						memcpy(stRequest.host, optarg+7, str-optarg-7);
					}
					*/
					if ( 0 == memcmp(optarg, "http://", 7))
					{
						char* str = strchr(optarg+7, '/');
						if(NULL == str) {
							fprintf(stderr, "url error\n");
							return -1;
						}

						if(0 == stRequest.host[0])
						{
							memcpy(stRequest.host, optarg+7, str-optarg-7);
						}
						if(0 == stRequest.filename[0])
						{
							strcpy(stRequest.filename, str);
						}

						snprintf(stRequest.content, 2048, "%s", optarg);

					}
					else if (0 == memcmp(optarg, "https://", 8))
					{

						char* str = strchr(optarg+8, '/');
						if(NULL == str) {
							fprintf(stderr, "url error\n");
							return -1;
						}

						if(0 == stRequest.host[0])
						{
							memcpy(stRequest.host, optarg+8, str-optarg-8);
						}
						if(0 == stRequest.filename[0])
						{
							strcpy(stRequest.filename, str);
						}

					snprintf(stRequest.content, 2048, "%s", optarg);
					stRequest.port = htons(443);
					}
					else
					{
						fprintf(stderr, "url error: %s\n", optarg);
						return -1; 
					}
				}
				break;
			case 'h':
				printf("%s\n", helpStr);
				return 0;
			case 'v':
				printf("version: 0.51\n");
				return 0;
			case 'w':
				strcpy(g_szResultFilename, optarg);
				break;
			case 'l':
				g_iResponseLength = atoi(optarg);
				break;
			case 'd':
				g_fgDisplayHeader = atoi(optarg);
				break;
			case 't':
				timer = atof(optarg);
				g_stTimer.tv_sec = (int)timer;
				g_stTimer.tv_usec = (int)((timer-g_stTimer.tv_sec)*1000000);
				break;
			case 'i':
				if(0 == strcmp(optarg, "dns")) {
					g_uiInspect |= DNS_INSPECT;
				} else if(0 == strcmp(optarg, "connect")) {
					g_uiInspect |= CONNECT_INSPECT;
				} else if(0 == strcmp(optarg, "write")) {
					g_uiInspect |= WRITE_INSPECT;
				} else if(0 == strcmp(optarg, "read")) {
					g_uiInspect |= READ_INSPECT;
				} else if(0 == strcmp(optarg, "all")) {
					g_uiInspect |= 0xffffffff;
				} else {
					printf("--inspect argv=[%s] is error\n", optarg);
					return -1;
				}
				break;
			default:
				fprintf(stderr, "excess error\n");
				return -1;
		}
	}

	if(g_fgStdin)
		return 1;


	if(0 == stRequest.port)
	{
		stRequest.port = htons(80);
	}
	if(0 == stRequest.filename[0])
	{
		stRequest.filename[0] = '/';
	}
	if(0 == stRequest.type)
	{
		stRequest.type = 1;
	}
	if(0 == stRequest.host[0])
	{
		fprintf(stderr, "host error\n");
		return -1;
	}
	if(0 == stRequest.header[0])
	{
		strcpy(stRequest.header, "Connection:close\r\n");
	}
	if(0 == g_szResultFilename[0])
	{
		char* str = strrchr(stRequest.filename, '/');
		if(NULL == str)
		{
			fprintf(stderr, "filenme error\n");
			return -1;
		}
		str++;
		if(0 == *str)
		{
			strcpy(g_szResultFilename, "index.html");
		}
		else
		{
			strcpy(g_szResultFilename, str);
		}
	}
	char* str = strchr(g_szResultFilename, '?');
	if(NULL != str)
	{
		*str = 0;
	}

	char responseFirstLine[128];
	struct timeval stTimeval1;
	gettimeofday(&stTimeval1, NULL);
	ret = dealwithOneRequest(&stRequest, g_fgDisplayHeader, g_szResultFilename, g_iResponseLength, &g_stTimer, g_uiInspect, responseFirstLine);
	struct timeval stTimeval2;
	gettimeofday(&stTimeval2, NULL);

	double usedtime = stTimeval2.tv_sec-stTimeval1.tv_sec + (stTimeval2.tv_usec-stTimeval1.tv_usec)/1000000.0;

	if((ret>=0) && (g_fgDisplayHeader))
	{
		fprintf(stdout, "%.6f %s", usedtime, responseFirstLine);
		fflush(stdout);
	}
	else
	{
		fprintf(stdout, "%.6f\n", usedtime);
		fflush(stdout);
	}
	if(-1 == ret)
	{
		return -1;
	}
	return 0;
}


/*
void signalAlarm(int sig)
{
	return;
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
*/
int main(int argc, char* argv[])
{

	if (1 == argc) {
		printf("%s", helpStr);
		return -1;
	}
//	registerSignal();

	int ret = dealwithCommandLine(argc, argv);

	if(0 == g_fgStdin)     //去掉stdin的配置
		return ret;

	if(-1 == ret)
		return -1;

	int g_iTooLongFlag = 0;
	char line[1024];
	char* str;

	while(1) {
		str = fgets(line, 1024, stdin);

		if(NULL == str) {
			if(0 == feof(stdin)) {
				continue;
			}
			break;
		}

		//如果当前串末不为'\n'，放弃当前串，继续读
		if('\n' != line[strlen(line)-1]) {
			g_iTooLongFlag = 1;
			continue;
		}

		//上次未读到'\n'，这次读到，但行过长，输出空行
		if(1 == g_iTooLongFlag) {
			fprintf(stdout, "\n");
			fflush(stdout);
			g_iTooLongFlag = 0;
			continue;
		}

		//空行或注释行
		if(('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0])) {
			fprintf(stdout, "\n");
			fflush(stdout);
			continue;
		}

		if(0 != g_szResultFilename[0])
			dealwithOneLine(line, g_fgDisplayHeader, g_szResultFilename, g_iResponseLength, &g_stTimer, g_uiInspect);
		else
			dealwithOneLine(line, g_fgDisplayHeader, NULL, g_iResponseLength, &g_stTimer, g_uiInspect);

	}

	return 0;
}

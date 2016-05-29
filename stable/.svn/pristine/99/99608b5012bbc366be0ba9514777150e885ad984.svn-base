#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <ctype.h>
#include "redirect.h"
#include "redirect_conf.h"
#include "display.h"


#define MAX_CONF_LINE 1024			//redirect.conf 配置行的最大个数
#define REDIRECT_START 0
#define REDIRECT_END 1

static char* g_pcConfFilename=NULL;		//配置文件名
static int g_iTooLongFlag = 0;		//对于太长的请求（目前设上限1023），忽略（返回空行）

int g_fdDebug = 0;
struct redirect_conf* g_pstRedirectConf = NULL;
int g_iRedirectConfNumber=0;
int g_iRedirectConfCheckFlag = 0;       //Just for check redirect.conf
static const char g_szLogFile[] = "/var/log/chinacache/redirect.log";
FILE* g_fpLog=NULL;
char* g_szDelimit = NULL;
void kill_redirect();
char *reloadMark = "CC_REDIRECT_RELOAD";


/* free some resource */
static void closeRedirect()
{
	extern int QQTopStreamReleaseModule();
	releaseDomainAndFromtagDb();
	QQTopStreamReleaseModule();
	return;
}

static void get_delimit()
{
	FILE* fp = fopen("/usr/local/squid/etc/squid.conf", "r");
	if(NULL == fp)
	{
		g_szDelimit = malloc(4);
		strcpy(g_szDelimit, "@#@");
		return;
	}
	char buffer[4096];
	while(fgets(buffer, 4095, fp))
	{
		xstrtok(buffer, " \t");
		if(0 == strcmp(buffer, "cc_compress_delimiter"))
		{
            char* str = xstrtok(NULL, " \t\r\n");
			if(NULL != str)
			{
				g_szDelimit = malloc(strlen(str)+1);
				strcpy(g_szDelimit, str);
				return;
			}
		}
	}
	g_szDelimit = malloc(4);
	strcpy(g_szDelimit, "@#@");
}

static void options_parse(int argc, char **argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "argv:\tconf_filename [5]\n");
		exit(-1);
	}
	if(0 == strcmp(argv[1], "-v"))
	{
		printf("redirect version: 2012/11/08-FC7.0 \n");
		exit(-1);
	}
	else if(0 == strcmp(argv[1], "-h"))
	{
		printf("Usage:\tredirect conf_filename [5]\n");
		exit(-1);
	}
	else if (0 == strcmp(argv[1], "-r"))
	{
		kill_redirect();	
		exit(0);
	}
	return;
}

static void init_log_file()
{
	g_fpLog = fopen(g_szLogFile, "a");
	if(NULL == g_fpLog)
	{
		fprintf(stderr, "cannot open log file=[%s]\n", g_szLogFile);
		return ;
	}

	chmod(g_szLogFile, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	
}

static void get_now_time( int now )
{
	time_t t1 = time(NULL);
	char* str = ctime(&t1);
	if (0 == now)
		fprintf(g_fpLog, "***********redirect [pid=%d] start at [%.*s]*********\n",getpid(), (int)strlen(str)-1, str);
	else
		fprintf(g_fpLog, "***********redirect [pid=%d] endup at [%.*s]*********\n",getpid(), (int)strlen(str)-1, str);

	fflush(g_fpLog);
}

/* add for reload, by chenqi */
#define PID_FILE "/var/run/cc_redirect.pid" 
#define PID_FILE_CLEAN "/var/run/cc_redirect.pid.clean" 
void kill_redirect()
{
	FILE *fd, *fp, *procfd;
	char pid[128];
	char buff[128];
	int pidnum;

	if (NULL == (fp = fopen(PID_FILE_CLEAN, "w"))) 
		return;

	if ((fd = fopen(PID_FILE, "r"))) {
		while (fgets(pid, sizeof(pid), fd)) { 
			pidnum = atoi(pid);
			if (kill(pidnum, 0))
				continue;
			snprintf(buff, sizeof(buff), "/proc/%d/status", pidnum);
			if ((procfd = fopen(buff,"r")) != NULL) {
				fgets(pid, sizeof(pid), procfd);
				fclose(procfd);
				if (strstr(pid, "redirect")) {
					if (0 == kill(pidnum, SIGHUP)) {
						fprintf(fp, "%d\n", pidnum); 
						printf("Send SIGHUP to redirect [pid=%d]\n", pidnum);
					}
				}   
			}    
		}   
		fclose(fd);
	}    
	fclose(fp);
	rename(PID_FILE_CLEAN, PID_FILE);
}

static int write_pid_file(void)
{
	FILE * fd;
	if (NULL != (fd = fopen(PID_FILE, "a"))) {    
		fprintf(fd, "%d\n", getpid());
		fclose(fd);
		fprintf(g_fpLog, "redirect write [pid=%d] to pidfile\n", getpid());
		return 0;
	}    
	fprintf(g_fpLog, "redirect can not write [pid=%d] to pidfile: %s\n", getpid(), strerror(errno));
	return -1;
}

static void InitArgv(int argc, char **argv)
{
	//get conf filename
	if (NULL != g_pcConfFilename) {
		free(g_pcConfFilename);
		g_pcConfFilename = NULL;
	}
	g_pcConfFilename = (char*)malloc(1+strlen(argv[1]));
	if (NULL == g_pcConfFilename) {
		fprintf(g_fpLog, "malloc failure in file=[%s] in line=[%d]\n", __FILE__, __LINE__);
		exit(-1);
	}
	strcpy(g_pcConfFilename, argv[1]);

	//get debug level
	g_fdDebug = 0;
	if (argc > 2) {
		if(!strcmp(argv[2], "check"))
		{
			g_iRedirectConfCheckFlag = 1;
		}
		else if(isdigit(argv[2][0]))
		{
			g_fdDebug = atoi(argv[2]);
			if(0 == g_fdDebug)
				g_fdDebug = 1;
		}
		
	}
}

/* 
   WARNING:
   Although i added reconfigure function here, but it doesn't work.
   The reason is that some specified anti-hijack do not analyse configure in proper method.
   This make the global variable "g_pstRedirectConf" canot get the right value after reconfiguring.
*/
void redirect_reconfigure(int signo)
{
	assert(SIGHUP == signo);
	/* free old configure */
	freeSomeRedirectConf(g_pstRedirectConf, g_iRedirectConfNumber);
	free(g_pstRedirectConf);
	g_pstRedirectConf = NULL;
	/* reconfigure */
	int ret = init_redirect_conf(g_pcConfFilename, MAX_CONF_LINE);
	if (-1 == ret) {   
		fprintf(g_fpLog, "redirect[pid=%d]\t Reconfigure failed\n", getpid());
		exit(-1); 
	}   
	fprintf(g_fpLog, "redirect[pid=%d]\t Reconfigure success\n", getpid());
	fflush(g_fpLog);
}

/* add for reload */

/*
 *  @@ Func: When redirectors deal with a 'redirect reload' task, we need reload config file.
 */    
void redirect_reload(void)
{
	// free old configure
	freeSomeRedirectConf(g_pstRedirectConf, g_iRedirectConfNumber);
	free( g_pstRedirectConf );
	g_pstRedirectConf = NULL;
	// reload config file
	int ret = init_redirect_conf(g_pcConfFilename, MAX_CONF_LINE);
	if (-1 == ret) {
		fprintf(g_fpLog, "*********Redirector[pid = %d]\treconfigure failed\n", getpid());
		printf("CC_RED_RELOAD_FAIL\n");                             //notify squid
	}
	else {
		fprintf(g_fpLog, "***********Redirector[pid = %d]\treconfigure successed\n", getpid());
		printf("CC_RED_RELOAD_OK\n");                                   //nofify squid
	}
	fflush(stdout);
	fflush(g_fpLog);
}



int main(int argc, char* argv[])
{
	options_parse(argc, argv);
	init_log_file();
    cc_redirect_check_clean();
	/* add for reload, by chenqi */
	// write_pid_file();		// cancel reconfigure function !!!
	struct sigaction act; 
	act.sa_handler = redirect_reconfigure;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGHUP);
	act.sa_flags = 0;
	if (sigaction(SIGHUP, &act, NULL) < 0) {
		fprintf(g_fpLog, "FATAL: sigaction called failed\n");
        cc_redirect_check_start(0, 0, "FATAL: sigaction called failed");
        cc_redirect_check_close();
		return -1;
	}
	/* add for reload */

	get_delimit();
	get_now_time(REDIRECT_START); //0- redirect start time
	InitArgv(argc, argv);

	//初始化,根据redirect.conf解析配置各个配置行含义，动态申请空间
	int ret = init_redirect_conf(g_pcConfFilename, MAX_CONF_LINE);
	if(-1 == ret) {
        cc_redirect_check_close();
		fprintf(g_fpLog, "initDomainCult failure\n");
		return -1;
	}

	//FIX:QQ MUSIC功能限制线上还在使用么
	ret = initDomainAndFromtagDb(g_pcConfFilename);
	if(-1 == ret) {
        cc_redirect_check_close();
		fprintf(g_fpLog, "initDomain Fromtag failure\n");
		return -1;
	}

	//Just for check redirect.conf
	if(g_iRedirectConfCheckFlag)
	{
		fprintf(g_fpLog, "check redirect conf file success\n");
        cc_redirect_check_close();
		g_iRedirectConfCheckFlag = 0;
		return 0;
	}

	char line[20488];	//line的长度多出一块，防止后面的处理越界
	memset(line, 0, sizeof(line));
	while(!feof(stdin)) {
		if(NULL == fgets(line, 20480, stdin))
			continue;

		//判断一行是否过长
		if('\n' != line[strlen(line)-1]) {
			if(g_fdDebug) {
				if(0 == g_iTooLongFlag)		//过长行的开始
					fprintf(g_fpLog, "input:");
				fprintf(g_fpLog, "%s", line);
			}
			printf("%s", line);
			g_iTooLongFlag = 1;
			continue;
		}
		if(g_fdDebug) {
			if(0 == g_iTooLongFlag)
				fprintf(g_fpLog, "input:%s", line);
			else
				fprintf(g_fpLog, "%s", line);
		}
		//deal with redirect reload task    
		int retTmp = strncmp(line, reloadMark, strlen(reloadMark));
		if (0 == retTmp) {
			fprintf(g_fpLog, "*******Received a redirect reload task,begin do reload\n");
			fflush(g_fpLog);
			redirect_reload();
			continue;
		}
		//如果一行过长，日志输出空行，redirect返回主进程的是原字符串
		if(1 == g_iTooLongFlag) {
			if(g_fdDebug) {
				fprintf(g_fpLog, "output:\n");
				fflush(g_fpLog);
			}
			printf("%s", line);
			fflush(stdout);
			g_iTooLongFlag = 0;
			continue;
		}
		//处理请求行
		dealwithRequest(line);
	}
	closeRedirect();
	get_now_time(REDIRECT_END); //1- redirect start time
	return 0;
}

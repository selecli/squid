#include    <unistd.h>
#include    <stdint.h>
#include    <stdlib.h>
#include    <pthread.h>
#include    <string.h>
#include    <sys/stat.h>
#include    <sys/types.h>
#include    <stdarg.h>
#include    <fcntl.h>
#include    <sys/socket.h>
#include    <netdb.h>
#include    <stdio.h>
#include    <sys/time.h>
#include    <sys/wait.h>
#include    <netinet/in.h>
#include    <arpa/inet.h>
#include    <errno.h>
#include    <assert.h>
#include    <regex.h>
#include    <signal.h>
#include    <stdbool.h>
#include    <sys/epoll.h>
//add by cwg for getpwnam
#include	<pwd.h>
#include    "config.h"
#include    "mod_billing_hashtable.h"
#include    "mod_billing_mempool.h"
#define ASSERT(f)	assert(f)


//#define VER "1.5.6.build2013-03-14-FC7.0"
#define VER "1.6.0.build2013-07-30-FC7.0"
//#define VER "1.6.0.build2013-06-21-FC7.0"
#define LOG_ROTATE_NUMBER 5
#define LOG_ROTATE_SIZE 100000000
#define DEBUG_LOG_FILE	"/var/log/chinacache/billingd_info.log"

#define MAX_EVENTS 256
#define EPOLL_TIMEOUT 1000
#define MAX_HOST_NAME 100
#define FD_MAX 65535  //fd_table max

bool shutdown_flag = false;
bool reload_flag = false;
bool daemon_mode = false;
bool flag_cut_point = false;

unsigned short Port = 8877;
static char devid[200];

//pthread_mutex_t hashtable_mutex = PTHREAD_MUTEX_INITIALIZER;

static int debug_level = 0;
static FILE* fp = NULL;
static int file_size = 0;

static time_t last_rcv_ok = 0;
static time_t last_billing_out_time = 0;
static time_t last_up_time = 0;
static int delta_rcv_time = 12;

struct epoll_event events[MAX_EVENTS];
static int kdpfd = -1;
static int listen_fd = 0;

struct _fde 
{
	int fd;
	// add by cwg at 2013-07-30
	char *billing_buff;
	int left_len;
};
typedef struct _fde fde;

fde *fd_table;

#define debug(a, s, args...)    _debug((a), (char *)__FILE__, (int)__LINE__, (char *)__func__, s, ## args)
void _debug(int level, char * file, int line, char * func, char * format, ...);
int addInfoLog(int type, const char* str);

typedef int epoll_handle_read_event(int fd); 
int billingd_epoll_add(int fd,int flag);
int billingd_epoll_remove(int fd);
void disconnect_fd(int fd);
#define CHECK_VALUE(VAL, MAX)                    \
	if (VAL/delta_rcv_time > MAX) {                                                         \
		snprintf(error_info,1024,"[%s] [%llu]one line error happen",node->host,VAL); \
		addInfoLog(0,error_info);                                        \
		VAL = MAX*delta_rcv_time;                                 \
	}


void log_close()
{
	if(fp)
		fclose(fp);
	fp = NULL;
	file_size = 0;
}

void log_open()
{
	struct stat buf;
	/*judge the logFile exist*/
	int logFileIsExist = -1;
	int flage = 0;
	if (fp != NULL)
	{
		logFileIsExist = access(DEBUG_LOG_FILE,F_OK);
		if (0 != logFileIsExist)
		{
			log_close();
			flage = 1;
		}
	}
	if(NULL == fp)
	{
		fp = fopen(DEBUG_LOG_FILE, "a+");
		{
			stat(DEBUG_LOG_FILE,&buf);
			file_size = buf.st_size;
			if(fp == NULL)
				return;
			if (flage)
			{
				addInfoLog(0, "WARING: /var/log/chinacache/billing_info.log is not exist\n");
				struct passwd *pwd;
				int pwd_ret = 0;
				pwd = getpwnam("squid");
				pwd_ret = chown(DEBUG_LOG_FILE, pwd->pw_uid, pwd->pw_uid);
				if (pwd_ret != 0)
				{
					addInfoLog(0, "WARING: chown squid:squid /var/log/chinacache/billing_info.log error\n");
				}
				flage = 0;
			}
		}
	}
}

struct init_time
{
	long uptime_init;
	long write_init;
}start_time;

//从配置文件中解析出来的正则表达式链表
struct billingConfNode
{
	char regDomain[DOMAIN_LENTH];
	regex_t* regComp;
	struct billing_attr lbAttr;
	struct billing_attr rbAttr;
	struct billing_attr fbAttr;
	struct billingConfNode* next;
};

//带有流量信息的链
struct billingNode
{
	char regDomain[DOMAIN_LENTH];
	struct billing_attr lbAttr;
	struct billing_attr rbAttr;
	struct billing_attr fbAttr;
	struct billingNode* next;
};

//整体配置项读到的配置文件属性
struct billingConfig
{
	int interval;
	int on_off;
	char path[MAX_PATH_LENTH];
	int port;
	//是否不输出本地流量 0为不输出 1为输出
	int deny_local;
	//是否不输出下层fc流量 0为不输出, 1为输出
	int deny_chinacache;
	//debug level 等级
	int debug_level;
	//头指针: 正则链
	struct billingConfNode* confHead;
};

static struct billingConfig conf = {
	60,
	0,
	"/data/proclog/log/squid/billing/",
	8877,
	1,
	1,
	0,
	NULL
};

struct wbilling
{
	char* filename;
	char* content;
};

static long readuptime(void)
{
	char result[100];
	char up[64] = "0";
	FILE *fp = fopen("/proc/uptime", "r");
	if (NULL == fp){
		snprintf(result,100,"cannot open /proc/uptime");
		addInfoLog(0,result);
		exit(-1);
	}
	fgets(up, sizeof(up), fp);
	fclose(fp);
	char *token = strtok(up, " \t\n");
	if (NULL == token)
	{
		snprintf(result,100,"cannot read /proc/uptime");
		addInfoLog(0,result);
		exit(-1);
	}
	return atol(token);	
}

static time_t mytime(time_t *t)
{
	time_t now = start_time.write_init + readuptime() - start_time.uptime_init;
	if (t)
		*t = now;
	return now;
}

void _debug(int level, char * file, int line, char * func, char * format, ...)
{                         
	va_list list;         

	if (level >= debug_level) {    
		fprintf(stderr, "\n[%s][%d][%s]", file, line, func);
		va_start(list,format);
		vfprintf(stderr, format, list);
		va_end(list);     
	}
} 

void str_rtrim(char* str, const char* tokens)
{
	if (str == NULL)
	{
		return;
	}
	char* p = str + strlen(str) - 1;
	while ((p >= str) && (strchr(tokens, *p)))
	{
		p--;
	}
	*++p = '\0';
	return;
}
/* Log Rotate
 */
void log_rotate()
{
	static char from[256];
	static char to[256];

	memset(from,0,256);
	memset(to,0,256);

	int i = LOG_ROTATE_NUMBER;
	while(i){
		snprintf(from, 256, "%s.%d", DEBUG_LOG_FILE, i - 1);
		snprintf(to, 256, "%s.%d", DEBUG_LOG_FILE, i);
		rename(from, to);
		i--;
	}
	log_close();
	assert(fp == NULL);
	rename(DEBUG_LOG_FILE, from);
	log_open();
}

//Add by cwg at 2013-05-31
static char * logTime(time_t t)
{
	struct tm *tm;
	static char buf[128];
	static time_t last_t = 0;
	if (t != last_t)
	{
		tm = localtime(&t);
		strftime(buf, 127, "%a %B %d %H:%M:%S %Y", tm); 
		last_t = t;
	}
	return buf;
}

/*
 * 现在squid.conf中可以配置的debug_level等级只有连个
 * 0：显示关键的信息
 * 1：显示所有信息
 * */
int addInfoLog(int type, const char* str) 
{
	time_t t;
	t = time(NULL);
	char timeStr[128];
	snprintf(timeStr, sizeof(timeStr), "%s", logTime(t));
	str_rtrim(timeStr, "\n");

	log_open();

	if(0 == type )
	{
		file_size += fprintf(fp, "[%s]:%s\n", timeStr, str);
	}
	else if(1 == type && 1 == conf.debug_level)
	{
		file_size += fprintf(fp, "[%s]:%s\n", timeStr, str);
	}
	else if (2 == type) 
	{
		file_size += fprintf(fp, "Error[%s]:%s\n", timeStr, str);
	}    
	fflush(fp);


	if(file_size > LOG_ROTATE_SIZE)
		log_rotate();

	//	fclose(fp);
	return 1;
}

void sigproc(int signu)
{
	//	char result[100];
	ASSERT(signu == SIGTERM || signu == SIGHUP);

	if( signu == SIGTERM )
		shutdown_flag = true;

	if( signu == SIGHUP)
		reload_flag = true;
	//	snprintf(result,100,"shutdown_flag=%d;reloadflag=%d",shutdown_flag,reload_flag);
}

static int  dealwith_folder(void)
{
	int len  = 0 ;
	FILE *fp = NULL;
	char *ptr = NULL;
	char buf[MAX_HOST_NAME] = "0";
	char hostname[MAX_HOST_NAME] = "0";
	char sn_name[MAX_HOST_NAME] = "0";

	// got hostname
	if(gethostname(hostname, MAX_HOST_NAME)<0)
	{
		addInfoLog(0, "gethostname error\n");
		strcpy(hostname,"CCC-CC-0-000");
	}
	if (NULL == (ptr = strrchr(hostname,'-')))
	{
		addInfoLog(1,"NULL find  - in hostname %s\n");
	}
	else
	{
		*ptr = 0;
	}


	// got sn_name
	if (NULL == (fp = fopen("/sn.txt","r")))
	{
		addInfoLog(0,"fopen sn.txt error\n");
		strcpy(sn_name,"0100000300");
	}
	else
	{
		while(fgets(buf,MAX_HOST_NAME,fp))
		{
			if (sn_name[0] == '#') continue;
			strcpy(sn_name,buf);
			len = strlen(sn_name);
			if(sn_name[len-1] == '\n')
				sn_name[len-1] = 0;
		}
		fclose(fp);
	}

	// got folder
	int ret = 0;
	struct stat info;

	// fixed folder, added by chenqi
	strcpy(conf.path,"/data/proclog/log/squid/billing/");
	// fixed folder,  delete it when need

	if (-1 == stat(conf.path,&info))
	{
		addInfoLog(0, "WARING:/data/prolog/log/squid/billing/  path no exist\n");
		ret = mkdir(conf.path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(ret ==  -1 && errno!= EEXIST)
		{
			addInfoLog(0,"FATAL ERROR:/data/proclog/log/squid/billing/ path creat error\n");
			exit(-1);
		}
		if (-1 == stat(conf.path,&info))
		{
			addInfoLog(0, "FATAL ERROR: create 1 times, but is fail\n");
			exit(-1);
		}
		//chown(conf.path, 501, 501);
		//chown(conf.path, getuid(), getgid());
		//system("chown squid:squid /data/proclog/log/squid/billing/");
		struct passwd *pwd;
		int pwd_ret = 0;
		pwd = getpwnam("squid");
		pwd_ret = chown(conf.path, pwd->pw_uid, pwd->pw_uid);
		if (pwd_ret != 0)
		{
			addInfoLog(0, "WARING: chown squid:squid /data/proclog/log/squid/billing/ error\n");
		}
	}

	if(!S_ISDIR(info.st_mode))
	{
		ret = mkdir(conf.path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(ret ==  -1 && errno!= EEXIST)
		{
			strcpy(conf.path,"/data/proclog/log/squid/billing/");
			addInfoLog(0,"FATAL ERROR:/data/proclog/log/squid/billing/ path creat error\n");
		}
	}

	ret = strlen(conf.path);
	if (conf.path[ret-1] != '/')
		strcat(conf.path,"/");

	snprintf(devid,200,"%s%s_%s",conf.path, sn_name, hostname);
	return 0;
}

int dealwith_filename(char *s)
{
	struct stat info;
	static time_t sn_lastModifiedTime;
	int len = 0;
	FILE *fp = NULL;
	char *ptr = NULL;
	char sn_name[MAX_HOST_NAME] = "0";
	char hostname[MAX_HOST_NAME] = "0";
	char buf[MAX_HOST_NAME] = "0";
	struct timeval  now;
	struct tm* t;
	t = (struct tm*)localtime(&last_billing_out_time);
	gettimeofday(&now, NULL);

	// add by cwg at 2013-07-22 to read sn.txt when it changes 
	if (-1 == stat("/sn.txt", &info))
	{}
	else
	{
		if (sn_lastModifiedTime != info.st_mtime)
		{
			sn_lastModifiedTime = info.st_mtime;
			// got hostname
			if(gethostname(hostname, MAX_HOST_NAME)<0)
			{
				addInfoLog(0, "gethostname error\n");
				strcpy(hostname,"CCC-CC-0-000");
			}
			if (NULL == (ptr = strrchr(hostname,'-')))
			{
				addInfoLog(1,"NULL find  - in hostname %s\n");
			}
			else
			{
				*ptr = 0;
			}
			// got sn_name
			if (NULL == (fp = fopen("/sn.txt", "r")))
			{
				addInfoLog(0,"fopen sn.txt error\n");
				strcpy(sn_name,"0100000300");
			}
			else
			{
				while(fgets(buf, MAX_HOST_NAME,fp))
				{
					if (sn_name[0] == '#') continue;
					strcpy(sn_name,buf); 
					len = strlen(sn_name);
					if(sn_name[len-1] == '\n')
						sn_name[len-1] = 0;
				}
				fclose(fp);
			}
			memset(devid, '\0', 200);
			snprintf(devid, 200, "%s%s_%s", conf.path, sn_name, hostname);
		}
	}

	sprintf(s,"%s.billing%02d%02d%02d%02d%02d.%09ld.log.tmp", devid, (t->tm_mon+1), t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,now.tv_usec);
	addInfoLog(0,s);
	return 1;
}


//解析squid传来的每行
static void billing_line(char* line)
{
	if (NULL == line || delta_rcv_time <= 0)
		goto line_error;

	struct hash_entry* node = NULL;
	char error_info[1024];
	memset(error_info,0,1024);

	uint64_t FLOW_MAX = 1;
	FLOW_MAX <<= 30;    // MAX = 1G bytes/seconds
	long long int CONN_MAX = 10000000U;

	char* token = strtok(line, white_space);
	if( NULL == token)
		goto line_error;

	if ( (node = hashtable_get(token)) == NULL )
	{
		node = hashtable_create(token);
		strcpy(node->host, token);
	}

	uint64_t temp1;
	long long int temp2;
	//local
	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->local.client.read_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->local.client.write_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->local.source.read_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->local.source.write_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp2 = atoll(token);
	CHECK_VALUE(temp2,CONN_MAX)
		node->local.client.connection_times += temp2;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp2 = atoll(token);
	CHECK_VALUE(temp2,CONN_MAX)
		node->local.source.connection_times += temp2;

	//remote
	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->remote.client.read_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->remote.client.write_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->remote.source.read_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->remote.source.write_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp2 = atoll(token);
	CHECK_VALUE(temp2,CONN_MAX)
		node->remote.client.connection_times += temp2;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp2 = atoll(token);
	CHECK_VALUE(temp2,CONN_MAX)
		node->remote.source.connection_times += temp2;

	//fc
	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->fc.client.read_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->fc.client.write_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->fc.source.read_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp1 = atoll(token);
	CHECK_VALUE(temp1,FLOW_MAX)
		node->fc.source.write_size += temp1;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp2 = atoll(token);
	CHECK_VALUE(temp2,CONN_MAX)
		node->fc.client.connection_times += temp2;

	token = strtok(NULL, white_space);
	if( NULL == token)
		goto line_error;
	temp2 = atoll(token);
	CHECK_VALUE(temp2,CONN_MAX)
		node->fc.source.connection_times += temp2;

	return;

line_error:
	snprintf(error_info,1024,"receive one error line [%s]",line?line:"<NULL>");
	addInfoLog(0,error_info);
	return;
}

pid_t get_running_pid()
{
	FILE *fd;
	char pid[128];
	char buff[128];
	FILE  *procfd;
	int pidnum;
	pid_t ret = -1;
	static int flage = 0;

	if ((fd = fopen(PIDFILE, "r")) != NULL) 
	{
		fgets(pid, sizeof(pid), fd);
		fclose(fd);
		pidnum = atoi(pid);
		if (pidnum < 0)
		{
			addInfoLog(0, "WARING: atoi(pid) from cc_billingd.pid is wrong!\n");	
		}
/*		if (g_pid != pidnum)
		{
			char pid_msg[1024];
			memset(pid_msg, '\0', 1024);
			snprintf(pid_msg, 1024, "WARING: the pid[%d] of cc_billingd.pid in not equal g_pid[%d]!\n", pidnum, g_pid);
			addInfoLog(0, pid_msg);
		}*/
		snprintf(buff, sizeof(buff), "/proc/%d/status", pidnum);
		if ((procfd = fopen(buff,"r")) != NULL) 
		{
			fgets(pid, sizeof(pid), procfd);
			fclose(procfd);

			if (strstr(pid, "billingd")!= NULL) 
			{
				flage = 1;
				ret = pidnum;
			}
			else
			{
				char line_err[1024];
				memset(line_err, '\0', 1024);
				snprintf(line_err, 124, "WARING: No have 'billingd' in this line of [%s] \n", buff);
				addInfoLog(0, line_err);
			}

		}
		else
		{
			if (flage)
			{
				char line[1024];
				memset(line, '\0', 1024);
				snprintf(line, 1024, "WARING: open [%s] error!\n", buff);
				addInfoLog(0, line);
			}
		}
	}
	else
	{
		addInfoLog(0, "WARING: open /var/run/cc_billingd.pid error!\n");
	}

	return ret;
}

static int check_running_pid(void)
{
	int ret = 1;
	pid_t pid = get_running_pid();
	if(pid == -1)//  no running copy, write self pid into pidfile
	{
		FILE * fd;
		if ((fd = fopen(PIDFILE, "w+")) != NULL)
		{
			fprintf(fd, "%d", getpid());
			fclose(fd);
			ret = 0;
		}
		else
		{
			addInfoLog(0, "WARING: write cc_billingd.pid is wrong!\n");
			ret = -1;
		}

	}
	else
	{
		fprintf(stderr, "billingd is running, process id:%d\n",pid);
		addInfoLog(0,"billingd is running");
	}

	return ret;
}
//how to use
static void usage (void)
{
	fprintf(stderr, "USAGE: billingd [options]\n"
			"OPTIONS:\n"
			"  -v   vision\n"
			"  -h   help info\n"
			"  -k   reconfigure|shutdown    reload|close\n"
			"  -D   daemon mode\n"
			"  -d   debug level (0,1,2)\n");
}

static int read_billing_info( int sock_fd )
{
	uint32_t billing_count = 0;
	char msgError[1024];
	fde *F = &fd_table[sock_fd];
	//static char billing_buff[102400];
	//static int32_t left_len = 0;
	
	// add by cwg at 2013-07-23
	char tmp_buff[102400];
	memset(tmp_buff, '\0', 102400);
	if (F->left_len > 0)
	{
		if (F->billing_buff)
		{
			memcpy(tmp_buff, F->billing_buff, F->left_len);
			free(F->billing_buff);
			F->billing_buff = NULL;
		}
		else
		{
			addInfoLog(0, "WARING: the left_len large 0,but billing_buff is NULL!");
		}
	}
	else
	{
		if (F->billing_buff)
		{
			memset(msgError, '\0', 1024);
			snprintf(msgError, 1024, "WARING: the left_len == 0, but billing_buff is not NULL! billing_buff:%s", F->billing_buff);
			addInfoLog(0, msgError);
			free(F->billing_buff);
			F->billing_buff = NULL;
		}
	}
	ssize_t len = read(sock_fd, tmp_buff + F->left_len, 10240);
	if( len == 0)
	{
		addInfoLog(0,"in read_billing_info, read_len ==0\n");
		goto fail;
	}
	else if( (len < 0) && (errno == EINTR) )
	{
		addInfoLog(0,"in read_billing_info, errno == EINTR\n");
		return 0;
	}
	else if( len < 0 )
	{
		addInfoLog(0,"in read_billing_info, read_len < 0\n");
		goto fail;
	}

	F->left_len += len;
	//F->billing_buff[F->left_len] = 0;
	// add by cwg at 2013-07-23
	tmp_buff[F->left_len] = 0;

	time_t current_time = time(NULL);
	delta_rcv_time = (int) difftime(current_time, last_rcv_ok);
	if (delta_rcv_time > 300) {
		addInfoLog(0, "Warning: billingd not received data over 300 seconds\n");
	}
	else if (delta_rcv_time < 0) {
		addInfoLog(0, "Warning: the interval of billingd received data is abnormal\n");
	}

	/* i do not find a good resulution for multi-squid */
	delta_rcv_time = 12;		// fixed me, this is bad !!

	last_rcv_ok = current_time;
	addInfoLog(1,tmp_buff);

	char *tmp;
	while( (tmp = memchr(tmp_buff, '\n', F->left_len)) != NULL) {
		char line[1024];
		memset(line, 0, 1024);
	
		assert((tmp - tmp_buff) < 1024);
		memcpy(line, tmp_buff, (tmp - tmp_buff));

		billing_count++;
		billing_line(line);

		tmp++;
		int cha = tmp - tmp_buff;
		if(cha < 1)
		{
			addInfoLog(0,"in read_billing_info, cha < 1\n");
			goto fail;
		}

		F->left_len -= cha;

		memmove(tmp_buff, tmp, F->left_len);
		tmp_buff[F->left_len] = 0;

	}
	// add by cwg at 2013-07-30  不为NULL 代表1 malloc成功了；2 被赋值过
	if (F->billing_buff) 
	{
		free(F->billing_buff);
	 	F->billing_buff = NULL;
	}
	if (F->left_len > 0)
	{
		// add by cwg at 2013-07-29
		F->billing_buff = malloc(F->left_len + 1);
		if (!F->billing_buff)
		{
			addInfoLog(0, "error: malloc() for billing_buff fail! so exit(-1)!!");
			exit(-1);
		}
		memset(F->billing_buff, '\0', F->left_len + 1);
		memcpy(F->billing_buff, tmp_buff, F->left_len);
	}

	if( F->left_len < 0 ) {
		addInfoLog(2,"[read_socket] left_len < 0");
		exit(0);
	}

	return 0;

fail:
	addInfoLog(0, "[read_socket] error\n");
	disconnect_fd(sock_fd);
	return -1;
}

static void options_parse(int argc, char **argv)
{
	if(argc < 2) return;

	char* v = VER;

	int c;
	while( (c = getopt(argc, argv, "k:vhDd:")) != -1 ) {

		switch(c) {

			case 'h':
				usage();
				exit(EXIT_SUCCESS);

			case 'v':
				fprintf(stderr, "Version: %s\n", v);
				exit(EXIT_SUCCESS);

			case 'D':
				daemon_mode = true;
				break;
			case 'd':
				debug_level = atoi(optarg);;
				break;

			case 'k':
				if( (strcmp(optarg, "reconfigure") != 0) &&
						(strcmp(optarg, "shutdown") != 0) )
				{
					exit(EXIT_FAILURE);
				}

				pid_t nowpid;
				nowpid = get_running_pid();

				if ( -1 == nowpid )
				{
					exit(EXIT_FAILURE);
				}

				if ( strcmp(optarg, "reconfigure") == 0 )
				{
					kill( nowpid, SIGHUP );
				}
				else
					kill( nowpid, SIGTERM);

				exit(EXIT_SUCCESS);

			default:
				usage();
				exit(1);
		}
	}
}

static void daemonize(void)
{

	pid_t pid;

	pid = fork();

	if( pid < 0 ) {
		addInfoLog(0,"[daemonize] fork error\n");
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

bool getBillingConfig(char* config)
{
	assert(config);
	FILE* cf;
	char buf[MAX_CONFIG_LENTH] = "";
	char buf_stay[MAX_CONFIG_LENTH] = "";

	if ( NULL == config)
	{
		addInfoLog(2,"failed to open config files name is empty");
		exit(0);
	}

	if ((cf = fopen(config, "r")) == NULL)
	{
		addInfoLog(2,"failed to open config files");
		exit(0);
	}

	struct billingConfNode *q = NULL;
	q = conf.confHead;
	uint32_t len;

	//get a line of info
	memset(buf,0,MAX_CONFIG_LENTH);
	while (fgets(buf, sizeof(buf) - 1, cf) )
	{
		//remove last char '\n'
		strcpy(buf_stay,buf);
		len = strlen(buf);
		if(len == 0 || buf[0] == '#')
			continue;
		*(buf + len - 1) = '\0';
		//finish check config file

		//token start
		char *token = strtok(buf, white_space);
		if (!token || strcasecmp(token, CONFIG_FIRST_FILED))
			continue;

		//the 2ND zone
		if (NULL == (token = strtok(NULL, white_space)))
			return false;

		if (strncmp(token, CONFIG_SECOND_FILED, sizeof(CONFIG_SECOND_FILED)))
		{
			char *tail = buf_stay + len - 3; 
			if (!strncmp(tail,"on",2))
			{
				conf.on_off = 1;
			}
			continue;
		}

		//the 3rd zone
		if (NULL == (token = strtok(NULL, white_space)))
			return false;

		if (strncmp(token, CONFIG_THIRD_FILED, sizeof(CONFIG_THIRD_FILED)))
			continue;

		//now finish check the first 3 zone like "cc_mod_billing sub_mod billingd"

		//now 4th zone
		if (NULL == (token = strtok(NULL, white_space)))
			return false;

		//get expect local on or off
		if( strcasecmp(token, "except_localhost") == 0 )
		{
			if (NULL == (token = strtok(NULL, white_space)))
				return false;

			if(!strcasecmp(token, "on"))
				conf.deny_local = 0;
			else if(!strcasecmp(token, "off"))
				conf.deny_local = 1;
		}

		//get expect chinache on or off
		if( strcasecmp(token, "except_chinacache") == 0 )
		{
			if (NULL == (token = strtok(NULL, white_space)))
				return false;

			if(!strcasecmp(token, "on"))
				conf.deny_chinacache = 0;
			else if(!strcasecmp(token, "off"))
				conf.deny_chinacache = 1;
		}
		//get path to write billing
		if( strcasecmp(token, "path") == 0 )
		{
			if (NULL == (token = strtok(NULL, white_space)))
				return false;

			strncpy(conf.path, token, MAX_PATH_LENTH);
			conf.path[MAX_PATH_LENTH] = '\0';
		}

		//get interval

		if( strcasecmp(token, "interval") == 0 )
		{
			if (NULL == (token = strtok(NULL, white_space)))
				return false;

			conf.interval = atoi(token);
		}

		//get regex
		if( strcasecmp(token, "regex") == 0 )
		{
			if (NULL == (token = strtok(NULL, white_space)))
				return false;


			//printf("---->regex = %s\n", token);

			struct billingConfNode *p = NULL;
			p = cc_malloc(sizeof(struct billingConfNode));
			if(!p)
			{
				//				printf("Error: malloc faild\n");
				return false;
			}


			strcpy(p->regDomain, token);

			//	printf("---->regex2 = %s\n", p->regDomain);

			p->regComp = (regex_t*)cc_malloc(sizeof(regex_t));

			if( regcomp(p->regComp, p->regDomain, REG_ICASE) != 0 )
			{
				addInfoLog(2,"regcomp error and exit!");
				exit(0);
			}


			//set new nod to 0.
			p->lbAttr.client.read_size = 0;
			p->lbAttr.client.write_size = 0;
			p->lbAttr.source.read_size = 0;
			p->lbAttr.source.write_size = 0;
			p->lbAttr.client.connection_times = 0;
			p->lbAttr.source.connection_times = 0;

			p->rbAttr.client.read_size = 0;
			p->rbAttr.client.write_size = 0;
			p->rbAttr.source.read_size = 0;
			p->rbAttr.source.write_size = 0;
			p->rbAttr.client.connection_times = 0;
			p->rbAttr.source.connection_times = 0;

			p->fbAttr.client.read_size = 0;
			p->fbAttr.client.write_size = 0;
			p->fbAttr.source.read_size = 0;
			p->fbAttr.source.write_size = 0;
			p->fbAttr.client.connection_times = 0;
			p->fbAttr.source.connection_times = 0;


			if(conf.confHead == NULL)
			{
				conf.confHead = p;
				q = p;
			}
			else
			{
				q->next = p;
				q = q->next;
			}
		}

		//get port
		if( strcasecmp(token, "port") == 0 )
		{
			token = strtok(NULL, white_space);

			if(!token)
				return false;

			uint32_t temp = atoi(token);
			if( (temp < 0) || (temp > 65336))
				return false;

			conf.port = temp;
		}
		if( strcasecmp(token, "debug_level") == 0 )
		{
			token = strtok(NULL, white_space);

			if(!token)
				return false;

			uint32_t temp = atoi(token);
			if( (temp < 0) || (temp > 3))
				return false;

			conf.debug_level = temp;
		}

		memset(buf,0,MAX_CONFIG_LENTH);
	}
	fclose(cf); //close config file
	char config_info[1024];
	addInfoLog(0,"we got these from config file squid.conf:");
	snprintf(config_info,1024,"\n\t\t conf.port =%d \n \t\t conf.interval= %d  \n\t\t conf.deny_local=%d \n\t\t conf.deny_chinacache=%d \n\t\t conf.debug_level=%d \n\t\t conf.path=%s",
			conf.port,conf.interval,conf.deny_local,conf.deny_chinacache,conf.debug_level,conf.path);
	addInfoLog(0,config_info);

	return true;
}

//正则匹配
int regCheck(regex_t* reg,  char* host)
{
	const size_t nmatch = 2;
	regmatch_t pm[2];

	int ret = regexec(reg, host, nmatch, pm, 0);

	return ret;
}
//流量加
void flowSum(struct billing_attr* sum, struct billing_attr* b)
{
	sum->client.read_size += b->client.read_size;
	sum->client.write_size += b->client.write_size;
	sum->source.read_size += b->source.read_size;
	sum->source.write_size += b->source.write_size;
	sum->client.connection_times += b->client.connection_times;
	sum->source.connection_times += b->source.connection_times;
}

void* writeFile(void* arg)
{
	struct wbilling* pwb = (struct wbilling*) arg;
	FILE* billing_file;
	char billing_name[1024];
	memcpy(billing_name,pwb->filename,strlen(pwb->filename)-4);
	billing_name[strlen(pwb->filename)-4] = 0;

	pthread_detach(pthread_self());

	if( (billing_file = fopen(pwb->filename, "w")) == NULL ) {
		exit(1);
	}

	fprintf(billing_file, "%s", pwb->content);
	fflush(billing_file);

	if(-1 == rename(pwb->filename, billing_name)) {
		exit(1);
	}

	cc_free(pwb->filename); 
	cc_free(pwb->content); 
	cc_free(pwb); 
	fclose(billing_file);
	return 0;

}

static void set2zero(struct billing_attr* attr)
{
	attr->client.read_size = 0;
	attr->client.write_size = 0;
	attr->client.connection_times = 0;
	attr->source.read_size = 0;
	attr->source.write_size = 0;
	attr->source.connection_times = 0;
}

static void  removeFlow(struct hash_entry** left_entry, uint32_t left_entry_count)
{
	struct billingConfNode* qRegex = conf.confHead;
	struct billingConfNode* rLoop = NULL;
	int i;

	if(0 == conf.deny_local)
	{
		for( rLoop = qRegex; rLoop; rLoop = rLoop->next)
			set2zero(&(rLoop->lbAttr));

		for( i = 0 ; i < left_entry_count ; i++ ) 
			set2zero(&(left_entry[i]->local));
	}

	if(0 == conf.deny_chinacache)
	{
		for( rLoop = qRegex; rLoop; rLoop = rLoop->next)
			set2zero(&(rLoop->fbAttr));

		for( i = 0 ; i < left_entry_count ; i++ ) 
			set2zero(&(left_entry[i]->fc));
	}

}

bool writelog_body(char *bill,uint32_t len,uint32_t file_recap_num ,uint32_t left_entry_count,struct hash_entry** left_entry, uint32_t *real_data)
{
	struct billingConfNode* qRegex = conf.confHead;
	struct billingConfNode* rLoop = NULL;
	int i = 0;
	//先打印正则部分的 
	for( rLoop = qRegex; rLoop; rLoop = rLoop->next)
	{
		/*remove Excess billing end*/
		len += sprintf(bill+len, "http://%s/\t", rLoop->regDomain); //主机名
		len += sprintf(bill+len, "%llu\t", (unsigned long long)((rLoop->lbAttr.client.read_size + rLoop->rbAttr.client.read_size + rLoop->fbAttr.client.read_size)/file_recap_num));
		len += sprintf(bill+len, "%llu\t", (unsigned long long)((rLoop->lbAttr.client.write_size + rLoop->rbAttr.client.write_size + rLoop->fbAttr.client.write_size)/1000000000/file_recap_num));
		len += sprintf(bill+len, "%llu\t", (unsigned long long)(((rLoop->lbAttr.client.write_size + rLoop->rbAttr.client.write_size + rLoop->fbAttr.client.write_size)/file_recap_num)%1000000000));
		len += sprintf(bill+len, "%u\n", (rLoop->lbAttr.client.connection_times + rLoop->rbAttr.client.connection_times + rLoop->fbAttr.client.connection_times)/file_recap_num);
		*real_data = 1;
	}

	//打印非正则部分
	for( i = 0 ; i < left_entry_count ; i++ ) {
		len += sprintf(bill+len, "http://%s/\t", left_entry[i]->host); //主机名
		len += sprintf(bill+len, "%llu\t", (unsigned long long)((left_entry[i]->local.client.read_size + left_entry[i]->remote.client.read_size + left_entry[i]->fc.client.read_size)/file_recap_num));
		len += sprintf(bill+len, "%llu\t", (unsigned long long)((left_entry[i]->local.client.write_size + left_entry[i]->remote.client.write_size + left_entry[i]->fc.client.write_size)/1000000000/file_recap_num));
		len += sprintf(bill+len, "%llu\t", (unsigned long long)(((left_entry[i]->local.client.write_size + left_entry[i]->remote.client.write_size + left_entry[i]->fc.client.write_size)/file_recap_num)%1000000000));
		len += sprintf(bill+len, "%u\n", (left_entry[i]->local.client.connection_times + left_entry[i]->remote.client.connection_times + left_entry[i]->fc.client.connection_times)/file_recap_num);
		*real_data = 1;

	}


	//中缝线
	len += sprintf(bill+len, "-------original flowstat-------\n");

	//后面一半儿
	qRegex = conf.confHead;
	rLoop = NULL;
	for( rLoop = qRegex; rLoop; rLoop = rLoop->next)
	{
		len += sprintf(bill+len, "http://%s/\t", rLoop->regDomain); //主机名
		len += sprintf(bill+len, "%llu\t", (unsigned long long)((rLoop->lbAttr.source.write_size + rLoop->rbAttr.source.write_size + rLoop->fbAttr.source.write_size)/file_recap_num));
		len += sprintf(bill+len, "%llu\t", (unsigned long long)((rLoop->lbAttr.source.read_size + rLoop->rbAttr.source.read_size + rLoop->fbAttr.source.read_size)/(1000000000*file_recap_num)));
		len += sprintf(bill+len, "%llu\t", (unsigned long long)(((rLoop->lbAttr.source.read_size + rLoop->rbAttr.source.read_size + rLoop->fbAttr.source.read_size)/file_recap_num)%1000000000));
		len += sprintf(bill+len, "%u\n", (rLoop->lbAttr.source.connection_times + rLoop->rbAttr.source.connection_times + rLoop->fbAttr.source.connection_times)/file_recap_num);
		*real_data = 1;

		//正则清空数据

		rLoop->lbAttr.client.read_size = 0;
		rLoop->lbAttr.client.write_size = 0;
		rLoop->lbAttr.source.read_size = 0;
		rLoop->lbAttr.source.write_size = 0;
		rLoop->lbAttr.client.connection_times = 0;
		rLoop->lbAttr.source.connection_times = 0;

		rLoop->rbAttr.client.read_size = 0;
		rLoop->rbAttr.client.write_size = 0;
		rLoop->rbAttr.source.read_size = 0;
		rLoop->rbAttr.source.write_size = 0;
		rLoop->rbAttr.client.connection_times = 0;
		rLoop->rbAttr.source.connection_times = 0;

		rLoop->fbAttr.client.read_size = 0;
		rLoop->fbAttr.client.write_size = 0;
		rLoop->fbAttr.source.read_size = 0;
		rLoop->fbAttr.source.write_size = 0;
		rLoop->fbAttr.client.connection_times = 0;
		rLoop->fbAttr.source.connection_times = 0;

	}

	for( i = 0; i < left_entry_count ; i++ ) {
		len += sprintf(bill+len, "http://%s/\t", left_entry[i]->host); //主机名
		len += sprintf(bill+len, "%llu\t", (unsigned long long)((left_entry[i]->local.source.write_size + left_entry[i]->remote.source.write_size + left_entry[i]->fc.source.write_size)/file_recap_num));
		len += sprintf(bill+len, "%llu\t", (unsigned long long)((left_entry[i]->local.source.read_size + left_entry[i]->remote.source.read_size + left_entry[i]->fc.source.read_size)/(1000000000*file_recap_num)));
		len += sprintf(bill+len, "%llu\t", (unsigned long long)(((left_entry[i]->local.source.read_size + left_entry[i]->remote.source.read_size + left_entry[i]->fc.source.read_size)/file_recap_num)%1000000000));
		len += sprintf(bill+len, "%u\n", (left_entry[i]->local.source.connection_times + left_entry[i]->remote.source.connection_times + left_entry[i]->fc.source.connection_times)/file_recap_num);
		*real_data = 1;
	}

	return true;

}

bool writelog()
{
	char entryInfo[1024];
	time_t oldtime = last_billing_out_time;
	time_t actual_time = last_billing_out_time;//实际写日志的时间，用来消除尖峰用的。
	last_billing_out_time = time(NULL);
	time_t this_up_time = mytime(NULL);

	//hashtable中的节点总数
	uint32_t entry_count = hashtable_entry_count;

	//全部的hashtable节点指针
	struct hash_entry** entry_array = cc_malloc(sizeof(struct hash_entry*) * entry_count);
	memset(entry_array, 0, sizeof(struct hash_entry*) * entry_count);

	//复制hashtable to an array
	get_all_entry(entry_array);


	struct wbilling* wb = cc_malloc(sizeof(struct wbilling));
	memset(wb, 0, sizeof(struct wbilling));

	pthread_t wptid;

	//此时已经完成数据的收集.
	//按照标准打印机可.

	char billing_name[MAX_PATH_LENTH];
	char filename[MAX_PATH_LENTH] ;
	memset(filename,0,MAX_PATH_LENTH);
	memset(billing_name,0,MAX_PATH_LENTH);

	char billing_file[5000000];
	memset(billing_file,0,5000000);
	char* bill = billing_file;
	uint32_t len = 0;

	//匹配到的放到正则中,匹配不到的话,放到这里
	struct hash_entry** left_entry = cc_malloc(sizeof(struct hash_entry*) * entry_count);
	memset(left_entry, 0, sizeof(struct hash_entry*) * entry_count);

	uint32_t left_entry_count = 0;
	uint32_t i = 0;
	uint32_t real_data = 0;

	struct billingConfNode* qRegex = conf.confHead;
	struct billingConfNode* rLoop = NULL;

	FILE * fd_billing_file;

	dealwith_filename(filename);

	for( i = 0 ; i < entry_count ; i++ ) {

		//检查一下是不是正则串里面的
		qRegex = conf.confHead;
		rLoop = NULL;
		for( rLoop = qRegex; rLoop; rLoop = rLoop->next) {

			if( regCheck(rLoop->regComp, entry_array[i]->host) == 0 ) {
				flowSum(&(rLoop->rbAttr), &(entry_array[i]->remote));
				flowSum(&(rLoop->lbAttr), &(entry_array[i]->local));
				flowSum(&(rLoop->fbAttr), &(entry_array[i]->fc));
				break;
			}
		}

		if( rLoop != NULL )
			continue;

		left_entry[left_entry_count] = entry_array[i];
		left_entry_count++;
	}

	//this used to cut the point value --start
	if (true == flag_cut_point) {
		//uint32_t file_recap_num = (last_billing_out_time - oldtime)/(conf.interval); 
		uint32_t file_recap_num = (this_up_time - last_up_time)/(conf.interval); 
		if (file_recap_num < 2)
		{
			addInfoLog(2,"something wrong happend \n");
			flag_cut_point = false;
			return false;
		}

		uint32_t time_split = (this_up_time - last_up_time)%(conf.interval);
		//判断时间间隔长短，采取四舍五入；
		if(time_split *2 > conf.interval)
		{
			file_recap_num ++;
		}

		snprintf(entryInfo,1024,"%d billing files will be generated, left_entry_count is %d\n", file_recap_num, left_entry_count);
		addInfoLog(0,entryInfo);
		uint32_t count = 0;  //将积累的流量写到多个文件。
		if (entry_count > 0)
		{
			for (count = 0; count<file_recap_num; count++)
			{
				memset(billing_file,0,5000000);
				len =0;
				actual_time += conf.interval;

				removeFlow(left_entry, left_entry_count);
				//	if (0 == left_entry_count)
				//		continue;

				snprintf(entryInfo,1024,"entry_count=%d,left_entry_count=%d recap_num=%d\n",entry_count,left_entry_count,file_recap_num);
				addInfoLog(0,entryInfo);
				//打印表头时间

				if(count < file_recap_num-1)
					len += sprintf(bill+len, "start in %u, to %u\n", (unsigned int)oldtime, (unsigned int)actual_time);
				else
					len += sprintf(bill+len, "start in %u, to %u\n", (unsigned int)oldtime, (unsigned int)last_billing_out_time);

				writelog_body(bill,len,file_recap_num,left_entry_count,left_entry,&real_data);

				if(strlen(billing_file) > 5000000)
				{
					printf("too many billing lines, quit\n");
					addInfoLog(2,"too many billing lines, quit");
					exit(1);
				}

				dealwith_filename(filename);
				memcpy(billing_name,filename,strlen(filename)-4);
				billing_name[strlen(filename)-4+1] = 0; // rm .tmp


				if( (fd_billing_file = fopen(filename, "w")) == NULL ) {
					snprintf(entryInfo,1024,"billing_file %s open fail",filename);  
					addInfoLog(2,entryInfo);
					exit(1);
				}       

				fprintf(fd_billing_file, "%s", billing_file);
				fflush(fd_billing_file);

				if(-1 == rename(filename, billing_name)) {
					snprintf(entryInfo,1024, "copy temp file to billing file error");
					addInfoLog(2,entryInfo);
					exit(1);
				}       

				fclose(fd_billing_file);
				oldtime = actual_time;

			}
		}

		flag_cut_point = false;
		for(i = 0; i < entry_count; i++)
			cc_free(entry_array[i]);

		cc_free(entry_array);
		cc_free(left_entry);
	}
	//this used to cut the point value --end
	else {
		snprintf(entryInfo,1024,"entry_count=%d,left_entry_count=%d\n",entry_count,left_entry_count);
		addInfoLog(0,entryInfo);


		//remove flow stat if set expect local or FC
		removeFlow(left_entry, left_entry_count);

		//打印表头时间
		len += sprintf(bill+len, "start in %u, to %u\n", (unsigned int)oldtime, (unsigned int)last_billing_out_time);

		writelog_body(bill,len,1,left_entry_count,left_entry,&real_data);

		for(i = 0; i < entry_count; i++)
			cc_free(entry_array[i]);

		cc_free(entry_array);
		cc_free(left_entry);

		wb->filename = cc_malloc(strlen(filename)+1);
		wb->content = cc_malloc(strlen(billing_file)+1);	
		memcpy(wb->filename, filename, strlen(filename)+1);
		memcpy(wb->content, billing_file, strlen(billing_file)+1);

		if(1 == real_data)
		{
			if(pthread_create(&wptid, NULL, writeFile, wb) != 0)
			{       
				addInfoLog(2,"A billing file write thread create failed. This file missed");
			}       
			pthread_detach(wptid);
		}
		else
		{
			snprintf(entryInfo,1024,"file[%s] has no data, discard it \n",wb->filename);
			addInfoLog(0,entryInfo);
		}
	}
	last_up_time = mytime(NULL);
	return true;
}


void handle_accept(int listen_fd)
{
	struct	sockaddr_in	client_addr;
	socklen_t len = sizeof(client_addr);
	int client_socket = 0;

	if( (client_socket = accept(listen_fd, (struct sockaddr*)&client_addr, &len)) < 0 ) {
		addInfoLog(0,"accept error\n");
		return;
	}


	int opts;
	if( (opts = fcntl(client_socket, F_GETFL)) < 0 ) {
		goto failed;
	}

	opts = opts|O_NONBLOCK|FD_CLOEXEC;

	if( fcntl(client_socket, F_SETFL, opts) < 0 ) {
		goto failed;
	}

	if (billingd_epoll_add(client_socket,EPOLLIN | EPOLLHUP | EPOLLERR))
		goto failed;

	fde *F = &fd_table[client_socket];
	F->fd = client_socket; //add at 06-19 by cwg
	//add by cwg at 2013-07-30
	if (F->billing_buff)
		free(F->billing_buff);
	F->billing_buff = NULL;
	F->left_len = 0;

	return;
failed:
	addInfoLog(0, "accept error,close client_sock\n");
	close(client_socket);
}

void billingd_epoll_init(void)
{
	if( (kdpfd = epoll_create(MAX_EVENTS)) < 0 ) {
		addInfoLog(0, "epoll creat error\n");
	}
	memset(events, 0, sizeof(events));

}

int billingd_epoll_remove(int fd )
{
	return epoll_ctl(kdpfd, EPOLL_CTL_DEL, fd, NULL);
}

void disconnect_fd(fd)
{
	billingd_epoll_remove(fd);
	fde *F = &fd_table[fd];
	F->fd = fd;
	//add by cwg at 2013-07-30
	if (F->billing_buff)
	{
		free(F->billing_buff);
		F->billing_buff = NULL;
	}
	F->left_len = 0;
	F->fd = -1;
	memset(F, '\0',sizeof(fde));// add at 06-19 by cwg
	close(fd);
}

int billingd_epoll_add(int fd,int flag)
{ 

	struct epoll_event cevent;
	memset(&cevent, 0, sizeof(struct epoll_event));
	cevent.events = flag;
	cevent.data.fd = fd;

	if( epoll_ctl(kdpfd, EPOLL_CTL_ADD, fd, &cevent) == -1 )
	{
		addInfoLog(2,"[epoll_add] error\n");
		return -1;
	}

	return 0;
}

int billing_epoll_wait()
{
	int nfds;
	int i;
	int ret=0 ;

	nfds = epoll_wait(kdpfd, events, MAX_EVENTS, EPOLL_TIMEOUT);
	if(nfds == -1) {
		if( errno != EINTR ) {
			return -1;
		}

	} else if( 0 == nfds) {
		return 0;
	}


	for (i = 0; i < nfds; i++) {
		if (listen_fd == events[i].data.fd){
			handle_accept(listen_fd);
		}
		else if (events[i].events & EPOLLIN){
			read_billing_info(events[i].data.fd);
		}
		else if (events[i].events & (EPOLLHUP|EPOLLERR)){
			disconnect_fd(events[i].data.fd);
		}
	}
	return ret;
}

static void syn_time()
{
	pid_t pid = fork();
	if (pid < 0){
		addInfoLog(0, "billingd syn_time error");
		exit(-1);
	}
	else if (0 == pid) {	// child
		if (execl("/usr/local/squid/bin/syntime.sh", "syntime.sh", (char*)0)) {
			addInfoLog(0,"/usr/local/squid/bin/syntime.sh execute error");
			exit(-1);
		}
	}
	else {	// parent
		wait(NULL);
		addInfoLog(0, "billingd syn_time successfully");
	}
}

static void fd_table_init(void)
{
	int size;
	size = sizeof(struct _fde) * FD_MAX;
	fd_table = NULL;
	fd_table = cc_malloc(size);
	assert(fd_table != NULL);
	memset(fd_table, 0, size);
}

static void fd_table_dest(void)
{

	if(NULL != fd_table) {
		cc_free(fd_table);
	}
}


//main.c
int main(int argc, char** argv)
{
	addInfoLog(0,"==========billingd start==============");
	addInfoLog(0,VER);

	//register the sig
	signal(SIGTERM, sigproc);
	signal(SIGHUP, sigproc);

	options_parse(argc, argv);

	if( daemon_mode )
		daemonize();

	if(check_running_pid())
		exit(3);

	syn_time();
	fd_table_init();
	last_billing_out_time = time(NULL);		// time() initialize
	last_rcv_ok = last_billing_out_time;
	last_up_time = last_billing_out_time;

	start_time.uptime_init = 0;
	start_time.write_init = 0;
	start_time.uptime_init = readuptime();
	start_time.write_init = last_billing_out_time;


	//parse config file
	if ( !getBillingConfig(CONFIG_FILE) )
	{
		addInfoLog(2,"Erorr when parsing config file");
		exit(0);
	}


	if(conf.on_off == 0)
	{
		addInfoLog(0,"mod billing set off");
		exit(0);
	}

	dealwith_folder();
	hashtable_init();
	billingd_epoll_init();

	//fixme: 先写一次日志
	addInfoLog(0,"first write billing data\n");
	if( !writelog() )
	{
		char errMessage[1024];
		snprintf(errMessage, 1024, "[%s][%d]\t%s\n",(char *)__FILE__, __LINE__,	"writelog Error");
		addInfoLog(2,errMessage);
		exit(3);
	}
	//creat socket
	int SockFd;
	if( (SockFd = socket(AF_INET,SOCK_STREAM,0)) < 0 ) {
		addInfoLog(2,"socket creat error,exit");
		exit(0);
	}


	struct  sockaddr_in ServerAddress;
	memset(&(ServerAddress), 0, sizeof(ServerAddress));
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port  = htons(Port);
	ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	int option = 1;
	if( setsockopt(SockFd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0 ) {
		addInfoLog(2,"setsockopt error,exit");
		exit(0);
	}

	while( bind(SockFd,(struct sockaddr*)&ServerAddress, sizeof(struct sockaddr)) < 0 ) {
		addInfoLog(2,"socket bind error");
		sleep(1);
	}

	if( listen(SockFd, 10) < 0 ) {
		addInfoLog(2,"socket listen error,exit");
		exit(0);
	}

	// add the socket to epoll to listen new connections
	billingd_epoll_add(SockFd,EPOLLIN | EPOLLHUP | EPOLLERR);
	listen_fd = SockFd;

	time_t checktime;
	char checkMessage[1024];
	uint32_t wait_count = 0;


	while(1) {

		checktime = time(NULL);

		//================= write the billing file according to the system time start========
		if (checktime - last_billing_out_time > 599) {
			addInfoLog(2,"billingd time now has surpass last_write_ok override 600 seconds\n");
			if (wait_count < 60 ) { //ensure all data come,wait 60s
				snprintf(checkMessage, 1024, "data comes,wait all data come[%d][%d]continue",wait_count,conf.interval);
				addInfoLog(0,checkMessage);
				wait_count++;
			} else {
				addInfoLog(0,"the last data comes ,cut it\n");
				wait_count = 0; // count recover
				flag_cut_point = true;
				if(!writelog()) {
					char errMessage_1[1024];
					snprintf(errMessage_1, 1024, "[%s][%d]\t%s\n",(char *)__FILE__, __LINE__,	"writelog Error");
					addInfoLog(2,errMessage_1);
					exit(3);
				}
			}
		} else if ( checktime - last_billing_out_time < 0) {
			snprintf(checkMessage, 1024, "[%s][%d]\tchecktime:%d < last_billing_out_time:%d\n",(char *)__FILE__, __LINE__, (int)checktime, (int)last_billing_out_time);
			addInfoLog(0,checkMessage);
			syn_time();		// adjust clock
			if(!writelog()) {
				char errMessage_1[1024];
				snprintf(errMessage_1, 1024, "[%s][%d]\t%s\n",(char *)__FILE__, __LINE__,	"writelog Error");
				addInfoLog(2,errMessage_1);
				exit(3);
			}
		}
		//如果现在时间比上次写时间超过interval了， 就写个文件
		else if (checktime - last_billing_out_time  >= conf.interval ) {
			char result[200];
			snprintf(result,200,"checktime:%d - last_billing_out_time:%d = %d;internal=%d",(int)checktime,(int)last_billing_out_time,(int)(checktime-last_billing_out_time),conf.interval);
			addInfoLog(1,result);
			addInfoLog(1,"checktime >= last_billing_out_time + conf.interval");
			if(!writelog()) {
				char errMessage_2[1024];
				snprintf(errMessage_2, 1024, "[%s][%d]\t%s\n",(char *)__FILE__, __LINE__,	"writelog Error");
				addInfoLog(2,errMessage_2);
				exit(3);
			}
		}
		else {
			// checktime - last_billing_out_time  < conf.interval
			// do nothing
		}
		//================== write the billing file according to the system time end==========

		//如果需要关闭了， 也先写个文件， 然后退出
		if(shutdown_flag == true)
		{
			writelog();
			log_close();
			fd_table_dest();

			exit(1);
		}


		//如果需要reload, 写个文件, 然后清空配置(regex)链表, 重新load configure, 然后继续
		if(reload_flag == true)
		{
			if(!writelog())
			{
				char errMessage_3[1024];
				snprintf(errMessage_3, 1024, "[%s][%d]\t%s\n",(char *)__FILE__, __LINE__,	"writelog Error");
				addInfoLog(2,errMessage_3);
				exit(3);
			}
			//释放掉正则连结构

			struct billingConfNode* qstart = conf.confHead;
			struct billingConfNode* ploop = NULL;
			struct billingConfNode* ptemp = NULL;
			for( ploop = qstart; ploop; )
			{
				ptemp = ploop->next;
				cc_free(ploop->regComp);
				cc_free(ploop);
				ploop = ptemp;
			}

			conf.confHead = NULL;

			//重新装载配置
			if ( !getBillingConfig(CONFIG_FILE) )
			{
				addInfoLog(2,"erorr happens when parsing config file");
				exit(0);
			}
			reload_flag = false;
		}

		/* epoll wait when billing data arrive and new connection come
		   this is a simple epoll,we only focus on read */
		billing_epoll_wait();
	}
}

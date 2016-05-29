#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/stat.h>
#include "process.h"
#include "preloadmgr.h"

#define debug(s, args...) \
{   \
	if (config.verbose == 1) \
	printf(s, ## args);  \
}

#define MAX_PATH_LEN 1024
#define MAX_PRELOAD_LOGFILE_SIZE (100 * 1024 * 1024)
#define PID_FILE_PATH "/var/run/preloadmgr.pid"
#define LOG_FILE_PATH "/var/log/chinacache/preloadmgr.log"
#define PRELOAD_CONF_FILE "/usr/local/squid/etc/fcexternal.conf"
#define STORE_DIR_PREFIX "/tmp/preload"

FILE *log_fp = NULL;
off_t total_size = 0;
time_t start_time = -1;
struct config_st config;
pthread_cond_t alloc_cond;
pthread_mutex_t threadlock;
pthread_mutex_t alloc_mutex;
pthread_mutex_t count_mutex;
static int global_init_flag = 0;
struct tasklist_sync_st tasklist_sync;
static char short_options[] = "vhVn:r:m:s:T:";
char store_dir[MAX_PATH_LEN];
char tasklist_prev[MAX_PATH_LEN];
char tasklist_task[MAX_PATH_LEN];
char devid_buf[MAX_COMM_BUF_SZIE];
char back_url[MAX_URL_BUF_SIZE * 4];
char report_task_url[MAX_URL_BUF_SIZE];
char get_task_url_pre[MAX_URL_BUF_SIZE];
char get_task_url[MAX_URL_BUF_SIZE + MAX_COMM_BUF_SZIE];

static struct option long_options[] = {
	{"version", 0,  (int *) NULL,   'v'},   
	{"help",    0,  NULL,   'h'},   
	{"retry",   1,  (int *) &(config.retry), 0},        
	{"number", 1,  (int *) &(config.number), 0},
	{"listname", 1, (int *)(void *)&(config.listname),   0},
	{"command",  1, (int *)(void *)&(config.command),   0},
	{"verbose", 0,  (int *)&(config.verbose), 'V'},
	{"subthreads", 1, (int *)&(config.subthreads), 0},
	{"max_flex_count", 1, (int *)&(config.count), 0}
};

static void rmLeftGbg(void)
{
	char cmd[MAX_COMM_BUF_SZIE];

	memset(cmd, 0, MAX_COMM_BUF_SZIE);
	snprintf(cmd, MAX_COMM_BUF_SZIE, "rm -rf %s/*", STORE_DIR_PREFIX);
	system(cmd);
}

/* log level: 0: common write; 1: WARNING; 2: ERROR; 3: FATAL */
void writeLogInfo(const int type, char *format, ...)
{
	va_list ap; 
	char time_buf[256];
	char buf[MAX_URL_BUF_SIZE];
	char log_info[MAX_URL_BUF_SIZE];
	time_t cur_time;
	struct tm *local_time;

	va_start(ap, format);
	memset(buf, 0, MAX_URL_BUF_SIZE);
	memset(log_info, 0, MAX_URL_BUF_SIZE);

	while (*format) {
		if ('%' != *format) {
			snprintf(buf, MAX_URL_BUF_SIZE, "%c", *format);
			strcat(log_info, buf);
			++format;
			continue;
		}   
		switch (*(++format)) {
			case 'c': //char
				snprintf(buf, MAX_URL_BUF_SIZE, "%c", (char)va_arg(ap, int)); 
				strcat(log_info, buf);
				break;
			case 'd': //integer
				snprintf(buf, MAX_URL_BUF_SIZE, "%d", va_arg(ap, int)); 
				strcat(log_info, buf);
				break;
			case 'l': //char
				switch(*(++format)) {
					case 'd': //long integer
						snprintf(buf, MAX_URL_BUF_SIZE, "%ld", va_arg(ap, long)); 
						strcat(log_info, buf);
						break;
					default: //default
						snprintf(buf, MAX_URL_BUF_SIZE, "no parse for param[%c]", *format); 
						break;
				}
				break;
			case 's': //string
				snprintf(buf, MAX_URL_BUF_SIZE, "%s", va_arg(ap, char *)); 
				strcat(log_info, buf);
				break;
			default: //default
				snprintf(buf, MAX_URL_BUF_SIZE, "no parse for param[%c]", *format); 
				break;
		}
		++format;
	}
	va_end(ap);

	cur_time = time(NULL);
	local_time = localtime(&cur_time);
	strftime(time_buf, 256, "%Y/%m/%d %H:%M:%S", local_time);	

	switch(type) {
		case 0: //commen
			fprintf(log_fp, "%s| %s\n", time_buf, log_info);
			fflush(log_fp);
			break;
		case 1: //warning
			fprintf(log_fp, "%s| WARNING: %s\n", time_buf, log_info);
			fflush(log_fp);
			break;
		case 2: //error
			fprintf(log_fp, "%s| ERROR: %s\n", time_buf, log_info);
			fflush(log_fp);
			break;
		case 3: //fatal
			fprintf(log_fp, "%s| FATAL: %s\n", time_buf, log_info);
			fflush(log_fp);
			break;
		default: //default
			fprintf(log_fp, "%s| WARNING: [no this log level type, check it]\n", time_buf);
			fflush(log_fp);
			break;
	}
}

static void configMemberInit(struct config_st * config)
{
	config->help = 0;
	config->retry = 3;
	config->number = 3;
	config->count = 10;
	config->verbose = 0;
	config->version = 0;
	config->subthreads = 5;
	config->gettaskCounts = 3;
	config->command = NULL;
	config->listname = NULL;
}

static void openForTask(void)
{
	tasklist_sync.fp = fopen(tasklist_task, "w+");
	if (NULL == tasklist_sync.fp) {
		writeLogInfo(2, "open tasklist_task failed[%s]", strerror(errno));
		exit(1);
	}
}

static void  getPreloadConfBackup(void)
{
	char pre_file[MAX_PATH_LEN];
	char post_file[MAX_PATH_LEN];
	char sys_buf[MAX_PATH_LEN * 2];

	memset(pre_file, 0, MAX_PATH_LEN);
	memset(post_file, 0, MAX_PATH_LEN);
	memset(sys_buf, 0, MAX_PATH_LEN * 2);
	memset(get_task_url_pre, 0, MAX_URL_BUF_SIZE);
	memset(report_task_url, 0, MAX_URL_BUF_SIZE);
	memset(get_task_url, 0, MAX_URL_BUF_SIZE + MAX_COMM_BUF_SZIE);
	strcpy(get_task_url_pre, "http://centre.fds.ccgslb.net:8080/fds/fc/fctask.do");
	strcpy(report_task_url, "http://centre.fds.ccgslb.net:8080/fds/fc/fctaskstatus.do");
	strcpy(back_url, "http://centre.fds.ccgslb.net:8080/fds/fc/fctaskstatus.do");
	strcpy(get_task_url, get_task_url_pre);
	snprintf(get_task_url, MAX_URL_BUF_SIZE + MAX_COMM_BUF_SZIE, "%s?op=preload&devid=%s", get_task_url_pre, devid_buf);
}

static void getPreloadConf(void)
{
	FILE *fp = NULL;
	int count = 0;
	int get_conf_flag = 0;
	int back_conf_flag = 0;
	int report_conf_flag = 0;
	char *ok, *str, *temp;
	char tembuf[MAX_COMM_BUF_SZIE];
	char tempbuf[MAX_URL_BUF_SIZE];
	char buf[MAX_URL_BUF_SIZE * 2];

	while (NULL == (fp = fopen(PRELOAD_CONF_FILE, "r+")) && ++count < 3) usleep(500);
	if (NULL == fp) {
		writeLogInfo(2, "open fcexternal.conf failed[%s]", strerror(errno));
		getPreloadConfBackup();
		return;
	}
	while (!feof(fp)) {
		if (get_conf_flag && report_conf_flag && back_conf_flag) {
			break;
		}
		memset(buf, 0, MAX_URL_BUF_SIZE * 2);
		fgets(buf, MAX_COMM_BUF_SZIE, fp);
		temp = buf;
		if ('\0' == *temp || '#' == *temp) {
			continue;
		}
		while (' ' == *temp || '\t' == *temp) {++temp;}
		if ('#' == *temp || '\r' == *temp || '\n' == *temp) {
			continue;
		}
		temp[strlen(temp) - 1] = '\0';
		if (NULL == strstr(temp, "PreloadGetUrl") && NULL == strstr(temp, "PreloadReportUrl") && NULL == strstr(temp, "PreloadBackUrl")) {
			continue;
		}
		str = strtok_r(temp, " \t\n", &ok);
		if (NULL == str) {
			writeLogInfo(2, "fcexternal.conf configure error[%s]", buf);
			exit(-1);
		}
		strcpy(tembuf, str);
		str = strtok_r(NULL, " \t\n", &ok);
		if (NULL == str) {
			writeLogInfo(2, "fcexternal.conf configure error[%s]", buf);
			exit(-1);
		}
		strcpy(tempbuf, str);
		if (!strncmp(tembuf, "PreloadGetUrl", 13) && !get_conf_flag) {
			memset(get_task_url_pre, 0, MAX_URL_BUF_SIZE);
			if(!strncmp(tempbuf, "http://", 7)) {
				strcpy(get_task_url_pre, tempbuf);
				get_conf_flag = 1;
			}
		} else if (!strncmp(tembuf, "PreloadReportUrl", 16) && !report_conf_flag) {
			memset(report_task_url, 0, MAX_URL_BUF_SIZE);
			if(!strncmp(tempbuf, "http://", 7)) {
				strcpy(report_task_url, tempbuf);
				report_conf_flag = 1;
			}
		} else if (!strncmp(tembuf, "PreloadBackUrl", 14) && !back_conf_flag) {
			memset(back_url, 0, MAX_URL_BUF_SIZE * 4);
			if(!strncmp(tempbuf, "http://", 7)) {
				strcpy(back_url, tempbuf);
				back_conf_flag = 1;
			}
		}
	}
	if (get_conf_flag) {
		snprintf(get_task_url, MAX_URL_BUF_SIZE + MAX_COMM_BUF_SZIE, "%s?op=preload&devid=%s", get_task_url_pre, devid_buf);
	}
}

static void getDevID(void)
{
	FILE *fp = NULL;

	fp = fopen("/sn.txt", "r");
	if (NULL == fp) {
		writeLogInfo(2, "open /sn.txt failed[%s]", strerror(errno));
		exit(1);
	}
	fgets(devid_buf, MAX_COMM_BUF_SZIE, fp);
	devid_buf[strlen(devid_buf) - 1] = '\0';
	fclose(fp);
}

static void storeDirInit(void)
{
	char mkcmd[MAX_PATH_LEN];

	memset(mkcmd, 0, MAX_PATH_LEN);
	memset(store_dir, 0, MAX_PATH_LEN);
	snprintf(store_dir, MAX_PATH_LEN, "%s/preload_store_%d", STORE_DIR_PREFIX, getpid());
	snprintf(mkcmd, MAX_PATH_LEN, "mkdir -p %s", store_dir);
	system(mkcmd);
}

static void globalInit(void)
{
	storeDirInit();
	memset(tasklist_prev, 0, MAX_PATH_LEN);
	memset(tasklist_task, 0, MAX_PATH_LEN);
	snprintf(tasklist_prev, MAX_PATH_LEN, "%s/preloadlist.pre", store_dir);
	snprintf(tasklist_task, MAX_PATH_LEN, "%s/preloadlist.task", store_dir);
	memset(&tasklist_sync, 0, sizeof(tasklist_sync));
	pthread_mutex_init(&tasklist_sync.prepare.mutex, NULL);
	pthread_mutex_init(&tasklist_sync.handle.mutex, NULL);
	pthread_mutex_init(&threadlock, NULL);  
	pthread_mutex_init(&alloc_mutex, NULL);  
	pthread_mutex_init(&count_mutex, NULL);  
	pthread_mutex_init(&tasklist_sync.mutex, NULL);
	pthread_cond_init(&alloc_cond, NULL);  
	pthread_cond_init(&tasklist_sync.cond, NULL);
	pthread_cond_init(&tasklist_sync.handle.cond, NULL);
	pthread_cond_init(&tasklist_sync.prepare.cond, NULL);
	getDevID();
	getPreloadConf();
	openForTask();
	tasklist_sync.status = TASKLIST_NULL;
	global_init_flag = 1;
}

static void versionInfoPrint(void)
{
	char version[] = "\rPreload 5.6.1\n\
					  \rCopyright (C) 2011 ChinaCache\n\
					  \rDescription: preload specified files\n";

	fprintf(stdout, version);
}

static void helpInfoPrint(void)
{
	int i = 0;
	static const char *helpstr[] = {
		"\n",
		"Usage: preloadmgr [options] [arguments]\n",
		"  -v,  --version               print version, subversion\n",
		"  -h,  --help                  print this help message\n",
		"  -V,  --verbose               enable verbose print, default isn't verbose\n",
		"  -n,                          set thread number, default is 3\n",
		"  -r                           set retry times when ran failed\n",
		"  -T                           set retry times when obtained task list failed\n",
		"  -m   --subthreads            assign how many threads that subordinate task should run, default is 3\n",
		NULL
	};

	while (NULL != helpstr[i]) {
		fprintf(stdout, helpstr[i++]);
	}
}

static void parseArgv(int argc, char ** argv)
{
	int ret;
	int *flag;
	int longindex=0;

	while ((ret = getopt_long (argc, argv, short_options, long_options, &longindex)) != -1) {
		switch(ret) {
			case 0:
				flag = (int *)long_options[longindex].flag;
				if (flag == (int *)&config.retry) {
					config.retry = atoi(optarg);
					debug("%d\n",config.retry);
				} else if (flag == (int *)(void *)&config.listname) {
					config.listname=(char *)malloc(strlen(optarg)+1);
					strcpy(config.listname, (const char *)optarg);
					debug("%s\n",config.listname);
				} else if (flag == (int *)(void *)&config.number){
					config.number = atoi(optarg);
					debug("%d\n", config.number);
				} else if (flag == (int *)(void *)&config.command){
					config.command=(char *)malloc(strlen(optarg)+1);
					strcpy(config.command, (const char *)optarg);
					debug("%s\n",config.command);
				} else if (flag == (int *)&config.subthreads){
					config.subthreads = atoi(optarg);
					debug("%d\n",config.subthreads);
				}
				break;
			case 'h':
				config.help = 1;
				break;
			case 'v':
				config.version = 1;
				break;
			case 'V':
				config.verbose = 1;
				break;
			case 'r':
				config.retry = atoi((const char *)optarg);
				debug("%d\n",config.retry);
				break;
			case 'c':
				config.command=(char *)malloc(strlen(optarg)+1);
				strcpy(config.command, (const char *)optarg);
				debug("%s\n",config.command);
				break;
			case 'l':
				config.listname=(char *)malloc(strlen(optarg)+1);
				strcpy(config.listname, (const char *)optarg);
				debug("%s\n",config.listname);
				break;
			case 'n':
				config.number = atoi((const char *)optarg);
				debug("%d\n", config.number);
				break;
			case 'm':
				config.subthreads = atoi(optarg);
				debug("%d\n",config.subthreads);
				break;
			case 's':
				config.count = atoi(optarg);
				debug("%d\n", config.count);
				break;
			case 'T':
				config.gettaskCounts = atoi(optarg);
				debug("gettaskCounts: %d\n", config.gettaskCounts);
				break;
			default:
				printf("Try `--help' for more options.\n");
				exit(-1);
		}
	}
}


int writepidFile(void)
{
	int fd;
	int val;
	char buf[256];
	struct flock lock;

	writeLogInfo(0, "-------------------------------------- start -------------------------------------------");

	//写打开标识文件，文件权限为读、写
	fd = open(PID_FILE_PATH, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
	if (fd < 0) {
		writeLogInfo(1, "open(pidfile) failed[%s]", strerror(errno));
		return -1;
	}
	//设置文件锁内容（写锁，整个文件加锁）
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;
	//给文件加锁
	if(fcntl(fd, F_SETLK, &lock) < 0) {
		writeLogInfo(1, "fcntl(pidfile) failed[%s], lock pid file failed", strerror(errno));
		return -1; 
	}
	//清空文件内容
	if(ftruncate(fd, 0) < 0) {
		writeLogInfo(1, "truncate(pidfile) failed[%s]", strerror(errno));
		return -1; 
	}
	//取得进程id,把进程id写入标识文件
	sprintf(buf, "%d\n", getpid());
	if(write(fd, buf, strlen(buf)) != strlen(buf)) {
		writeLogInfo(1, "write(pidfile) failed[%s]", strerror(errno));
		return -1; 
	}
	//取得文件描述符标志
	val = fcntl(fd, F_GETFD, 0); 
	if(val < 0) {
		writeLogInfo(1, "fcntl(pidfile) failed[%s], get file descriptor flag failed", strerror(errno));
		return -1;
	}
	//设置文件描述符标志，增加exec关闭文件标志
	val |= FD_CLOEXEC;
	if(fcntl(fd, F_SETFD, val) < 0) {
		writeLogInfo(1, "fcntl(pidfile) failed[%s], set file descriptor flag failed", strerror(errno));
		return -1;
	}
	//设置成功返回0

	return 0;
}

static void preloadInfoPrint(struct config_st *config)
{
	if (config->version) {
		versionInfoPrint();
		exit(0);
	}
	if (config->help) {
		helpInfoPrint();
		exit(0);
	}
}

static void clean_param(struct config_st *cfg)
{
	if (NULL != cfg->command) {
		free(cfg->command);
	}
	if (NULL != cfg->listname) {
		free(cfg->listname);
	}
}

static void cleanUp(void)
{
	writeLogInfo(0, 
			"PreloadTotalTime: [%ld seconds], total download size[%ld bytes]", 
			(long)(time(NULL) - start_time), 
			total_size);
	if (global_init_flag) {
		if (NULL != tasklist_sync.fp) {
			fclose(tasklist_sync.fp);
		}   
		pthread_mutex_destroy(&alloc_mutex);  
		pthread_mutex_destroy(&count_mutex);  
		pthread_mutex_destroy(&tasklist_sync.mutex);
		pthread_mutex_destroy(&tasklist_sync.handle.mutex);
		pthread_mutex_destroy(&tasklist_sync.prepare.mutex);
		pthread_cond_destroy(&alloc_cond);  
		pthread_cond_destroy(&tasklist_sync.cond);
		pthread_cond_destroy(&tasklist_sync.handle.cond);
		pthread_cond_destroy(&tasklist_sync.prepare.cond);
		rmLeftGbg();
		unlink(PID_FILE_PATH);
	}
	writeLogInfo(0, "-------------------------------------- exit --------------------------------------------");
	if (NULL != log_fp) {
		fclose(log_fp);
	}   
}

static void openLogFile(void)
{
	int count;
	struct stat st;
	char backup_name[MAX_COMM_BUF_SZIE];

	count = 0;
	do {
		log_fp = fopen(LOG_FILE_PATH, "a+");
		if (NULL == log_fp && 3 == count) {
			writeLogInfo(1, "fopen(log_file) failed[%s]", strerror(errno));
			exit(1);
		}
	} while (NULL == log_fp && ++count <= 3);

	if (stat(LOG_FILE_PATH, &st) < 0) {
		writeLogInfo(1, "stat() failed[%s]", strerror(errno));
		exit(1);
	}
	if (st.st_size > MAX_PRELOAD_LOGFILE_SIZE) {
		fclose(log_fp);
		snprintf(backup_name, MAX_COMM_BUF_SZIE, "%s.%ld", LOG_FILE_PATH, (long)time(NULL));
		rename(LOG_FILE_PATH, backup_name);
		count = 0;
		do {
			log_fp = fopen(LOG_FILE_PATH, "a+");
			if (NULL == log_fp && 3 == count) {
				writeLogInfo(1, "fopen(log_file) failed[%s]", strerror(errno));
				exit(1);
			}
		} while (NULL == log_fp && ++count <= 3);
		//truncate(LOG_FILE_PATH, 0);
		//fseek(log_fp, 0, SEEK_SET);
	}
}

int main(int argc, char **argv)
{
	/* figure the time for this preload */
	start_time = time(NULL);
	/* remove left garbage */
	rmLeftGbg();
	/* register destructor function */
	atexit(cleanUp);
	/* open log file for write log information */
	openLogFile();
	/* write pid file, ensure operational example uniqueness */
	if (writepidFile() < 0) {
		fprintf(stderr, "WARNING: an example has been in operation, now exit\n");
		exit(1);
	}
	/* init config */
	configMemberInit(&config);
	/* parse param */
	parseArgv(argc, argv);
	/* check param and do task */
	preloadInfoPrint(&config);
	/* global init for handle */
	globalInit();
	/* handle task */
	multiHandle(&config);
	/* clean */
	clean_param(&config);

	return 0;
}


/*
 * fixed by xin.yao: 2011-11-19
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "preloadmgr.h"

#define SUCCEED "3"
#define FAILED  "4"
#define MAX_THREAD_NUM 64
#define MAX_PATH_LEN 1024
#define EMPTY_TASK "EMPTY"
#define BACK_SUCCESS "SUCCESS"

#define debug(s, args...) \
{   \
	if (threadconfig->verbose == 1) \
	printf(s, ## args);  \
}

static int flag = 0;
static int threadnum = 0;
static int alloc_sync = 1;
static struct config_st *cfg = NULL;
static struct config_st *threadconfig;
char command[MAX_URL_BUF_SIZE * 2];
static struct thread_task_st thread_task[MAX_THREAD_NUM];

extern off_t total_size;
extern pthread_mutex_t threadlock;
extern  pthread_cond_t alloc_cond;
extern pthread_mutex_t alloc_mutex;
extern pthread_mutex_t count_mutex;
extern char store_dir[MAX_PATH_LEN];
extern char tasklist_prev[MAX_PATH_LEN];
extern char tasklist_task[MAX_PATH_LEN];
extern char devid_buf[MAX_COMM_BUF_SZIE];
extern char back_url[MAX_URL_BUF_SIZE * 4];
extern struct tasklist_sync_st tasklist_sync;
extern char report_task_url[MAX_URL_BUF_SIZE];
extern char get_task_url_pre[MAX_URL_BUF_SIZE];
extern char get_task_url[MAX_URL_BUF_SIZE + MAX_COMM_BUF_SZIE];

static void postResultToServer(struct handle_res_st *handle_res)
{
	char comm[MAX_URL_BUF_SIZE * 4];

	memset(comm, 0, MAX_URL_BUF_SIZE * 4);
	if (handle_res->detail) {
		snprintf(comm, 
				MAX_URL_BUF_SIZE * 4, 
				"/usr/bin/wget -q --timeout=5 --tries=3 --wait=1 -O /dev/null \"%s?taskid=%s&status=%s&devid=%s&taskurl=%s&md5=%s&Code=%s&Content-Length=%s&Last-Modified=%s&Date=%s&ETag=%s&Powered-By-ChinaCache=%s\"", 
				report_task_url, 
				handle_res->taskid, 
				handle_res->status,
				handle_res->devid,
				handle_res->task_url,
				handle_res->local_md5,
				handle_res->res_hdrs.res_code.value,
				handle_res->res_hdrs.cont_len.value,
				handle_res->res_hdrs.last_modified.value,
				handle_res->res_hdrs.date.value,
				handle_res->res_hdrs.etag.value,
				handle_res->res_hdrs.powered_by.value
				);
	} else {
		snprintf(comm, 
				MAX_URL_BUF_SIZE * 4, 
				"/usr/bin/wget -q --timeout=5 --tries=3 --wait=1 -O /dev/null \"%s?taskid=%s&status=%s&devid=%s&taskurl=%s&md5=%s\"", 
				report_task_url, 
				handle_res->taskid, 
				handle_res->status,
				handle_res->devid,
				handle_res->task_url,
				handle_res->local_md5
				);
	}
	debug("ReportTaskResult: %s\n",comm);
	writeLogInfo(0, "ReportTaskResult--> %s\n",comm);
	system(comm);
	if (NULL != handle_res) {
		free(handle_res);
		handle_res = NULL;
	}
}

static void purgeCacheObj(const char *url)
{
	char commbuf[MAX_URL_BUF_SIZE * 2];

	memset(commbuf, 0, MAX_URL_BUF_SIZE * 2);
	/*FIXME: squidclient usage BUG */
	//snprintf(commbuf, MAX_URL_BUF_SIZE * 2, "/usr/local/squid/bin/squidclient -p 80 -m PURGE %s", url);
	snprintf(commbuf, MAX_URL_BUF_SIZE * 2, "/usr/local/squid/bin/refresh_cli -f \"%s\" 1> /dev/null 2>&1", url);
	system(commbuf);
}

/* 
 * get HTTP response data
 */
static void getResHdrs(char *path, FILE *fp, http_res_hdrs_t *hdrs)
{
	struct stat st;
	char *str = NULL;
	char *saveptr = NULL;
	char line[HDR_VALUE_LEN];
	char cont_buf[MAX_COMM_BUF_SZIE];

	while (!feof(fp)) {
		memset(line, 0, HDR_VALUE_LEN);
		if (NULL == fgets(line, 1024, fp)) {
			continue;
		}    
		if (!hdrs->res_code.exist && NULL != strstr(line, "HTTP/1.")) {
			str = strtok_r(line, " :\t\n", &saveptr);		
			if (NULL == str) {
				continue;
			} 
			str = strtok_r(NULL, " \n", &saveptr);
			if (NULL == str) {
				continue;
			} 
			strncpy(hdrs->res_code.value, str, HDR_VALUE_LEN);
			hdrs->res_code.exist = 1;
		} else if (!hdrs->cont_len.exist && NULL != strstr(line, "Content-Length:")) {
			str = strtok_r(line, " :\t\n", &saveptr);		
			if (NULL == str) {
				continue;
			} 
			str = strtok_r(NULL, "\n", &saveptr);
			if (NULL == str) {
				continue;
			} 
			while (' ' == *str && ++str);
			strncpy(hdrs->cont_len.value, str, HDR_VALUE_LEN);
			hdrs->cont_len.exist = 1;
		} else if (!hdrs->last_modified.exist && NULL != strstr(line, "Last-Modified:")) {
			str = strtok_r(line, " :\t\n", &saveptr);		
			if (NULL == str) {
				continue;
			} 
			str = strtok_r(NULL, "\n", &saveptr);
			if (NULL == str) {
				continue;
			} 
			while (' ' == *str && ++str);
			strncpy(hdrs->last_modified.value, str, HDR_VALUE_LEN);
			hdrs->last_modified.exist = 1;
		} else if (!hdrs->date.exist && NULL != strstr(line, "Date:")) {
			str = strtok_r(line, " :\t\n", &saveptr);		
			if (NULL == str) {
				continue;
			} 
			str = strtok_r(NULL, "\n", &saveptr);
			if (NULL == str) {
				continue;
			} 
			while (' ' == *str && ++str);
			strncpy(hdrs->date.value, str, HDR_VALUE_LEN);
			hdrs->date.exist = 1;
		} else if (!hdrs->etag.exist && NULL != strstr(line, "ETag:")) {
			str = strtok_r(line, " :\t\n", &saveptr);		
			if (NULL == str) {
				continue;
			} 
			str = strtok_r(NULL, "\n", &saveptr);
			if (NULL == str) {
				continue;
			} 
			while (' ' == *str && ++str);
			strncpy(hdrs->etag.value, str, HDR_VALUE_LEN);
			hdrs->etag.exist = 1;
		} else if (!hdrs->powered_by.exist && NULL != strstr(line, "Powered-By-ChinaCache:")) {
			str = strtok_r(line, " :\t\n", &saveptr);		
			if (NULL == str) {
				continue;
			} 
			str = strtok_r(NULL, "\n", &saveptr);
			if (NULL == str) {
				continue;
			} 
			str = strrchr(str, ' ');
			if (NULL == str) {
				continue;
			}
			++str;
			strncpy(hdrs->powered_by.value, str, HDR_VALUE_LEN);
			hdrs->powered_by.exist = 1;
		}
	}
	if (strcmp(hdrs->res_code.value, "200")) {
		memset(hdrs->cont_len.value, 0, HDR_VALUE_LEN);
		return;	
	}
	if (!hdrs->cont_len.exist) {
		if (stat(path, &st) < 0) {
			writeLogInfo(2, "get file Content-Length failed[%s]", strerror(errno));
			return;
		}
	}
	/* here increase scalability */
	if (!hdrs->cont_len.exist) {
		memset(cont_buf, 0, MAX_COMM_BUF_SZIE);
		snprintf(cont_buf, MAX_COMM_BUF_SZIE, "%ld", st.st_size);
		strncpy(hdrs->cont_len.value, cont_buf, HDR_VALUE_LEN);
		hdrs->cont_len.exist = 1;
	}
	pthread_mutex_lock(&count_mutex);
	total_size += atol(hdrs->cont_len.value);
	pthread_mutex_unlock(&count_mutex);
}

static void handlePreload(struct handle_res_st *handle_res)
{
	int i;
	time_t start;
	FILE *fp = NULL;
	FILE *res_fp = NULL;
	char path[MAX_PATH_LEN];
	char res_path[MAX_PATH_LEN];
	char cmd[MAX_PATH_LEN * 2];
	char commbuf[MAX_PATH_LEN * 2];

	/*
	 * purge all cache object in tasklist, and download again 
	 */
	purgeCacheObj(handle_res->task_url);
	memset(path, 0, MAX_PATH_LEN);
	memset(res_path, 0, MAX_PATH_LEN);
	memset(cmd, 0, MAX_PATH_LEN * 2);
	memset(commbuf, 0, MAX_PATH_LEN * 2);
	snprintf(path, MAX_PATH_LEN, "%s/cont.%s", store_dir, handle_res->taskid);
	snprintf(res_path, MAX_PATH_LEN, "%s/hdrs.%s", store_dir, handle_res->taskid);
	snprintf(commbuf, MAX_PATH_LEN * 2, "/usr/bin/wget --timeout=10 --tries=1 -O %s -S -o %s -e http_proxy=127.0.0.1 \"%s\"", path, res_path, handle_res->task_url);
	start = time(NULL);
	system(commbuf);
	writeLogInfo(0, "PreloadHandleTime: task[%s] download time[%ld seconds]", handle_res->task_url, (long)(time(NULL) - start));
	res_fp = fopen(res_path, "r");
	getResHdrs(path, res_fp, &handle_res->res_hdrs);
	fclose(res_fp);
	if (strcmp(handle_res->res_hdrs.res_code.value, "200")) {
		strcpy(handle_res->status, FAILED);
		postResultToServer(handle_res);
		return;
	}
	start = time(NULL);
	snprintf(cmd, MAX_PATH_LEN * 2, "/usr/bin/md5sum %s", path);
	fp = popen(cmd, "r");
	fgets(handle_res->local_md5, MAX_COMM_BUF_SZIE, fp);
	pclose(fp);
	unlink(path);
	unlink(res_path);
	writeLogInfo(0, 
			"PreloadHandleTime: task[%s] md5sum time[%ld seconds], content length[%s bytes]", 
			handle_res->task_url, 
			(long)(time(NULL) - start), 
			handle_res->res_hdrs.cont_len.value);
	for (i = 0; i < MAX_COMM_BUF_SZIE; ++i) {
		if (isalpha(handle_res->local_md5[i]) || isdigit(handle_res->local_md5[i])) {
			continue;
		} else {
			handle_res->local_md5[i] = '\0';
			break;
		}   
	}   
	strcpy(handle_res->status, SUCCEED);
	postResultToServer(handle_res);
}

static void hdrs_init(http_res_hdrs_t *hdrs)
{
	hdrs->res_code.exist = 0;
	hdrs->cont_len.exist = 0;
	hdrs->last_modified.exist = 0;
	hdrs->date.exist = 0;
	hdrs->etag.exist = 0;
	hdrs->powered_by.exist = 0;
	strncpy(hdrs->res_code.name, "HTTP/1.", MAX_COMM_BUF_SZIE);
	strncpy(hdrs->cont_len.name, "Content-Length", MAX_COMM_BUF_SZIE);
	strncpy(hdrs->last_modified.name, "Last-Modified", MAX_COMM_BUF_SZIE);
	strncpy(hdrs->date.name, "Date", MAX_COMM_BUF_SZIE);
	strncpy(hdrs->etag.name, "ETag", MAX_COMM_BUF_SZIE);
	strncpy(hdrs->powered_by.name, "Powered-By-ChinaCache", MAX_COMM_BUF_SZIE);
}

static void handleResultAllocInit_r(struct handle_res_st **handle_res)
{
	pthread_mutex_lock(&alloc_mutex);
	while (!alloc_sync) {
		pthread_cond_wait(&alloc_cond, &alloc_mutex);
	}
	alloc_sync = 0;
	int size = sizeof(struct handle_res_st);
	*handle_res = malloc(size);
	if (NULL == *handle_res) {
		writeLogInfo(2, "malloc() for handle_res failed[%s]", strerror(errno));
		exit(-1);
	}   
	memset(*handle_res, 0, size);
	/* NOTE: operation the symbols of the priority */
	hdrs_init(&((*handle_res)->res_hdrs));
	alloc_sync = 1;
	pthread_cond_broadcast(&alloc_cond);
	pthread_mutex_unlock(&alloc_mutex);
}

static void * threadProcess_r(void *ptr)
{
	char *ok;
	char *str = NULL;
	char buf[MAX_URL_BUF_SIZE * 2];
	struct thread_task_st *arg = ptr;
	struct handle_res_st *handle_res = NULL;

	pthread_detach(pthread_self());
	handleResultAllocInit_r(&handle_res);

	while(1) {
		pthread_mutex_lock(&tasklist_sync.handle.mutex);
		if (TASKLIST_EMPTY == tasklist_sync.status) {
			pthread_cond_broadcast(&tasklist_sync.handle.cond);
			pthread_mutex_unlock(&tasklist_sync.handle.mutex);
			goto FINISHED;
		}
		if (feof(tasklist_sync.fp)) {
			tasklist_sync.status = TASKLIST_PREPARING;
			pthread_cond_signal(&tasklist_sync.prepare.cond);
			while (TASKLIST_PREPARING == tasklist_sync.status) {
				pthread_cond_wait(&tasklist_sync.handle.cond, &tasklist_sync.handle.mutex);
			}
			if (TASKLIST_EMPTY == tasklist_sync.status) {
				pthread_cond_broadcast(&tasklist_sync.handle.cond);
				pthread_mutex_unlock(&tasklist_sync.handle.mutex);
				goto FINISHED;
			}
			fseek(tasklist_sync.fp, 0, SEEK_SET);
		}
		memset(buf, 0, MAX_URL_BUF_SIZE * 2);
		fgets(buf, MAX_URL_BUF_SIZE * 2, tasklist_sync.fp);
		writeLogInfo(0, "handleTask--> %s", buf);
		//handleResultAllocInit_r(&handle_res);
		pthread_cond_broadcast(&tasklist_sync.handle.cond);
		pthread_mutex_unlock(&tasklist_sync.handle.mutex);
		if ('\0' == buf[0] || '\r' == buf[0] || '\n' == buf[0]) {
			continue;
		}
		strcpy(handle_res->devid, devid_buf);
		buf[strlen(buf) - 1] = '\0';
		//parse task_type
		str = strtok_r(buf, ";", &ok);
		if (NULL == str) {
			writeLogInfo(3, "tasklist configure error[task_type]");
			continue;
		}
		handle_res->task_type = atoi(str);
		//parse taskid
		str = strtok_r(NULL, ";", &ok);
		if (NULL == str) {
			writeLogInfo(3, "task list configure error[taskid]");
			continue;
		}
		strcpy(handle_res->taskid, str);
		//parse taskurl
		str = strtok_r(NULL, ";", &ok);
		if (NULL == str) {
			writeLogInfo(3, "task list configure error[taskurl]");
			continue;
		}
		strcpy(handle_res->task_url, str);
		//ignore class, no return info
		if (NULL == strtok_r(NULL, ";", &ok)) {
			writeLogInfo(3, "task list configure error[class]");
			continue;
		}
		//parse report detail flag
		str = strtok_r(NULL, ";\n", &ok);
		if (NULL == str) {
			writeLogInfo(3, "task list configure error[detail]");
			continue;
		}
		handle_res->detail = atoi(str);
		handlePreload(handle_res);
	}

FINISHED:
	pthread_mutex_lock(&threadlock);
	--threadnum;
	debug("NOTE: threads[%d] now finished, remaining number: %d\n", arg->myvalue, threadnum);
	writeLogInfo(1, "threads[%d] now finished, remaining number: %d", arg->myvalue, threadnum);
	pthread_mutex_unlock(&threadlock);

	return NULL;
}

static int backSuccess(const char *path)
{
	int  count = 0;
	FILE *fp = NULL;
	char *tmp = NULL;
	char line[MAX_URL_BUF_SIZE];

	while (NULL == (fp = fopen(path, "rb+")) && ++count < 3) usleep(500);
	if (NULL == fp) {
		writeLogInfo(3, "fopen() for backSuccess failed[%s]", strerror(errno));
		fclose(fp);
		return -1;
	}
	memset(line, 0, MAX_URL_BUF_SIZE);
	fgets(line, MAX_COMM_BUF_SZIE, fp);
	fclose(fp);
	printf("BACK INFO: %s\n", line);
	tmp = line;
	while (' ' == *tmp || '\t' == *tmp) {
		++tmp;
	}
	if (strncmp(line, BACK_SUCCESS, strlen(BACK_SUCCESS))) {
		writeLogInfo(1, "not 'SUCCESS', obtainBackTo received from server[%s]", line);
		return -1;	
	}
	return 0;
}

static void obtainBackTo(const char *post_file)
{
	int  count;
	FILE *fp = NULL;
	char *p, *tmp;
	char line[MAX_URL_BUF_SIZE];
	char taskid[MAX_COMM_BUF_SZIE];
	char class[MAX_COMM_BUF_SZIE];
	char commbuf[MAX_URL_BUF_SIZE * 2];
	char back_success_path[MAX_PATH_LEN];
	char back_url_buf[MAX_URL_BUF_SIZE * 4];

	count = 0;
	do {
		fp = fopen(post_file, "rb+");
		if (NULL == fp && 3 == count) {
			writeLogInfo(3, "open '%s' failed[%s]", post_file, strerror(errno));
			exit(-1);
		}
	} while (NULL == fp && ++count <= 3);
	memset(back_url_buf, 0, MAX_URL_BUF_SIZE * 4);
	strcpy(back_url_buf, back_url);
	strcat(back_url_buf, "?devid=");
	strcat(back_url_buf, devid_buf);
	strcat(back_url_buf, "&context=");
	while (!feof(fp)) {
		memset(line, 0, MAX_URL_BUF_SIZE);
		memset(taskid, 0, MAX_COMM_BUF_SZIE);
		memset(class, 0, MAX_COMM_BUF_SZIE);
		fgets(line, MAX_URL_BUF_SIZE, fp);
		if (!strncmp(line, EMPTY_TASK, strlen(EMPTY_TASK))) {
			writeLogInfo(1, "obtainBackTo: server response is 'EMPTY'");
			return;
		}
		if ('\0' == line[0] || '\r' == line[0] || '\n' == line[0]) {
			continue;
		}
		p = strchr(line, ';');
		if (NULL == p) {
			writeLogInfo(2, "task line configure format error[%s]", line);
			continue;
		}
		tmp = p + 1;
		p = strchr(tmp, ';');
		if (NULL == p) {
			writeLogInfo(2, "task line configure error[%s]", line);
			continue;
		}
		strncpy(taskid, tmp, p - tmp);
		p = strrchr(tmp, ';');
		if (NULL == p) {
			writeLogInfo(2, "task line configure error[%s]", line);
			continue;
		}
		*p = '\0';
		p = strrchr(tmp, ';');
		if (NULL == p) {
			writeLogInfo(2, "task line configure error[%s]", line);
			continue;
		}
		*p = '\0';
		p = strrchr(tmp, ';');
		if (NULL == p) {
			writeLogInfo(2, "task line configure error[%s]", line);
			continue;
		}
		++p;
		strncpy(class, p, strlen(p));
		strcat(back_url_buf, taskid);
		strcat(back_url_buf, ",");
		strcat(back_url_buf, class);
		strcat(back_url_buf, ";");
	}
	fclose(fp);
	memset(commbuf, 0, MAX_URL_BUF_SIZE * 4);
	memset(back_success_path, 0, MAX_PATH_LEN);
	snprintf(back_success_path, MAX_PATH_LEN, "%s/back_success.local", store_dir);
	snprintf(commbuf, MAX_URL_BUF_SIZE * 4, "/usr/bin/wget -q --timeout=10 --tries=3 --wait=1 -O %s \"%s\"", back_success_path, back_url_buf);
	printf("BackObtainResult: %s\n", commbuf);
	writeLogInfo(0, "BackObtainResult: %s\n", commbuf);
	system(commbuf);
	backSuccess(back_success_path);
}

static int haveMoreTasks(void)
{
	int count = 0;
	FILE *fp = NULL;
	char *errstr = NULL;
	char buf[MAX_URL_BUF_SIZE];

	do {
		usleep(500);
		fp = fopen(tasklist_prev, "rb+");
		if (NULL == fp && 3 == count) {
			errstr = "open tasklist_prev failed";
			goto EMPTY;
		}
	} while (NULL == fp && ++count <= 3);
	memset(buf, 0, MAX_URL_BUF_SIZE);
	fgets(buf, MAX_URL_BUF_SIZE, fp);
	fclose(fp);
	if(!strncmp(buf, EMPTY_TASK, strlen(EMPTY_TASK))) {
		errstr = "no tasklist remained, it is 'EMPTY'";
		goto EMPTY;
	}
	pthread_mutex_lock(&tasklist_sync.mutex);
	tasklist_sync.status = TASKLIST_MORE;
	pthread_mutex_unlock(&tasklist_sync.mutex);
	return 0;

EMPTY:
	pthread_mutex_lock(&tasklist_sync.mutex);
	tasklist_sync.status = TASKLIST_EMPTY;
	pthread_mutex_unlock(&tasklist_sync.mutex);
	writeLogInfo(1, "haveMoreTask() [%s]", errstr);
	//fprintf(stderr, "%s", errstr);
	return -1;
}

static int obtainTasklist(void)
{
	int count = 0;
	FILE *fp = NULL;
	char cmd[MAX_URL_BUF_SIZE];
	char buf[MAX_URL_BUF_SIZE];

	memset(cmd, 0, MAX_URL_BUF_SIZE); 
	snprintf(cmd, 
			MAX_URL_BUF_SIZE, 
			"/usr/bin/wget -q --timeout=10 --tries=%d --wait=1 -O %s \"%s\"", 
			cfg->gettaskCounts,
			tasklist_prev, 
			get_task_url); 
	system(cmd);
	do {
		fp = fopen(tasklist_prev, "r+");
		if (NULL != fp) {
			break;
		}
		if (3 == count) {
			writeLogInfo(3, "open '%s' file failed", tasklist_prev);
			exit(-1);
		}
		usleep(500);
	} while (++count <= 3);
	memset(buf, 0, MAX_URL_BUF_SIZE);
	fgets(buf, MAX_URL_BUF_SIZE, fp);
	fclose(fp);
	if(!strlen(buf)) {
		writeLogInfo(3, "get tasklist failed from server, check it");
		exit(-1);
	}

	if (cfg->verbose) {
		printf("\n************************************ Tasklist *************************************\n");
		memset(cmd, 0, MAX_URL_BUF_SIZE);
		snprintf(cmd, MAX_URL_BUF_SIZE, "cat %s", tasklist_prev); 
		system(cmd);
		printf("\n***********************************************************************************\n");
	} 
	return 0;
}

static void createNewTasklist(void)
{
	char cmd[1024];

	snprintf(cmd, 1024, "> %s; cat %s >> %s; > %s", tasklist_task, tasklist_prev, tasklist_task, tasklist_prev);
	system(cmd);
}

static void * getAndPrepareForTasklist(void *arg)
{
	pthread_detach(pthread_self());

	while (1) {
		pthread_mutex_lock(&tasklist_sync.prepare.mutex);
		while (TASKLIST_HANDLING == tasklist_sync.status) {
			pthread_cond_wait(&tasklist_sync.prepare.cond, &tasklist_sync.prepare.mutex);
		}
		tasklist_sync.status = TASKLIST_PREPARING;
		obtainTasklist();
		obtainBackTo(tasklist_prev);
		haveMoreTasks();
		if (TASKLIST_EMPTY == tasklist_sync.status) {
			if (flag) {
				pthread_cond_broadcast(&tasklist_sync.handle.cond);
			} else {
				flag = 1;
				pthread_cond_signal(&tasklist_sync.cond);
			}
			pthread_mutex_unlock(&tasklist_sync.prepare.mutex);
			break;
		}
		if (TASKLIST_MORE == tasklist_sync.status) {
			createNewTasklist();
			fseek(tasklist_sync.fp, 0, SEEK_SET);
		}
		if (flag) {
			pthread_cond_broadcast(&tasklist_sync.handle.cond);
		} else {
			flag = 1;
			pthread_cond_signal(&tasklist_sync.cond);
		}
		tasklist_sync.status = TASKLIST_HANDLING;
		pthread_mutex_unlock(&tasklist_sync.prepare.mutex);
	}

	return NULL;
}

static void getTasklistThreadInit(void)
{
	pthread_t thread;

	if (pthread_create(&thread, NULL, getAndPrepareForTasklist, NULL)) {
		writeLogInfo(2, "pthread_create() error[%s]", strerror(errno));
		exit(1);
	}
}

int multiHandle(struct config_st *myconfig)
{
	int get_task_times;
	pthread_t thread_pid;

	//atexit(cleanUp);

	cfg = myconfig;
	get_task_times = 0;
	threadconfig = myconfig;

	//globalInit();
	getTasklistThreadInit();

	pthread_mutex_lock(&tasklist_sync.mutex);
	if (TASKLIST_NULL == tasklist_sync.status) {
		pthread_cond_wait(&tasklist_sync.cond, &tasklist_sync.mutex);
	} 
	if (TASKLIST_EMPTY == tasklist_sync.status) {
		pthread_mutex_unlock(&tasklist_sync.mutex);
		pthread_exit(NULL);
	}
	pthread_mutex_unlock(&tasklist_sync.mutex);
	for (threadnum = 0; threadnum < myconfig->number; ) {
		thread_task[threadnum].myvalue = threadnum;
		thread_task[threadnum].myhandlenumber = 0;
		thread_task[threadnum].subthreads = myconfig->subthreads;
		if (pthread_create(&thread_pid, NULL, threadProcess_r, &thread_task[threadnum])) {
			writeLogInfo(2, "pthread_create() error[%s]", strerror(errno));
			continue;
		}
		memcpy(&thread_task[threadnum].pid, &thread_pid, sizeof(thread_pid));
		++threadnum;
	}

	while (threadnum  > 0) usleep(50000);

	return 0;
}


#include "cc_framework_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fnmatch.h>

#ifdef CC_FRAMEWORK

#define REFRESHD_PATH "/usr/local/squid/bin/refreshd"
#define REFRESHD_PIDFILE  "/var/run/refreshd.pid"
#define REFRESHD_PORT   21108
#define REFRESHD_ADDRESS "127.0.0.1"
#define READ_BUFFER_SIZE 10240
#define WRITE_BUFFER_SIZE 32768

#define M_MAX_DIR_REFRESH_HASH_NUM 2000
#define M_MAX_DIR_REFRESH_NUM 2000
#define M_MAX_URL_LEN 4096

typedef int dir_compare(const char *token, const char *url);

unsigned long count = 0;
unsigned int max_dir_refresh_time = 0;

static cc_module *mod = NULL;
int start_times = 0;

typedef struct _rf_read_buffer
{
	char buffer[READ_BUFFER_SIZE];
	int len;
} rf_read_buffer;

typedef struct _rf_write_buffer
{
	char buffer[WRITE_BUFFER_SIZE];
	int len;
	int send_len;

	struct _rf_write_buffer *next;
} rf_write_buffer;

typedef struct _rf_write_list
{
	struct _rf_write_buffer *first;
	struct _rf_write_buffer *last;
	int count;
} rf_write_list;

typedef enum
{
	RF_FC_ACTION_UNKNOWN,
	RF_FC_ACTION_INFO,
	RF_FC_ACTION_ADD,
	RF_FC_ACTION_VERIFY_START,
	RF_FC_ACTION_VERIFY_FINISH,
	RF_FC_ACTION_URL_PURGE,
	RF_FC_ACTION_URL_EXPIRE,
	RF_FC_ACTION_DIR_PURGE,
	RF_FC_ACTION_DIR_EXPIRE,
	RF_FC_ACTION_URL_PURGE_RET,
	RF_FC_ACTION_URL_EXPIRE_RET,
	RF_FC_ACTION_DIR_PURGE_RET,
	RF_FC_ACTION_DIR_EXPIRE_RET,
	RF_FC_ACTION_VERIFY_RET,
	RF_FC_ACTION_DIR_EXPIRE_REFRESH_PREFIX_START,
	RF_FC_ACTION_DIR_EXPIRE_REFRESH_WILDCARD_START,
	RF_FC_ACTION_DIR_PURGE_REFRESH_PREFIX_START,
	RF_FC_ACTION_DIR_PURGE_REFRESH_WILDCARD_START,
	RF_FC_ACTION_DIR_REFRESH_FINISH,
	RF_FC_ACTION_DIR_REFRESH_RESULT
} rf_fc_action;

typedef enum
{
	RF_NORMAL,
	RF_URL_VERIFYING
} rf_status;

enum {
	RF_WL_ACTION_ADD_TAIL,          //FIXME: help comment the function
	RF_WL_ACTION_ADD_FIRST, 
	RF_WL_ACTION_DEL
};

static int refresh_socket = -1;
static bool refresh_socket_flag = false;
static rf_read_buffer rf_buf;
static rf_write_list rf_wl;
static int rf_store_pos  = 0;
static int rf_verify_number = 0;
static int rf_list_limit = 80000;
static rf_status run_status = RF_NORMAL;

static int rf_process_cmd(char *buff);
static void rf_send_id();
static int rf_module_connect(int flag);
static void rf_verify_start();
static void rf_store_table_walk();
static void rf_verify_finish();

static void rf_clean_write_list();
static void rf_clean_read_buffer();
static void rf_clean_rw_buffer();

/********** realtime dir refresh **********/
struct refresh_dir_node
{
	char sessionid[128];	
	int action;	//expire or purge
	char dir[M_MAX_URL_LEN];
	time_t recv_time;
	int count;	//for dir refresh result

	dir_compare *compare_func;

	struct refresh_dir_node* next;
};

struct refresh_dir_list
{
	struct refresh_dir_node *head;
	int count;
};

struct refresh_dir_list refresh_hash_array[M_MAX_DIR_REFRESH_HASH_NUM];
int dir_count = 0;	// all dir count
char hostname[4096];
static MemPool *m_mod_refresh_dir_node_pool = NULL;
static MemPool *m_mod_refresh_rf_write_buffer_pool = NULL;

/************ mempool api ************/

static struct refresh_dir_node * mod_refresh_dir_node_pool_alloc(void)
{
	struct refresh_dir_node *obj = NULL; 
	if (NULL == m_mod_refresh_dir_node_pool)
	{       
		m_mod_refresh_dir_node_pool = memPoolCreate("mod_refresh refresh_dir_node_struct", sizeof(struct refresh_dir_node));
	}       
	return obj = memPoolAlloc(m_mod_refresh_dir_node_pool);
}

static rf_write_buffer * mod_rf_write_buffer_pool_alloc(void)
{
	rf_write_buffer *obj = NULL; 
	if (NULL == m_mod_refresh_rf_write_buffer_pool)
	{       
		m_mod_refresh_rf_write_buffer_pool = memPoolCreate("mod_refresh rf_write_buffer_struct", sizeof(rf_write_buffer));
	}       
	return obj = memPoolAlloc(m_mod_refresh_rf_write_buffer_pool);
}

void mod_refresh_mempool_cleanup()
{
	if(NULL != m_mod_refresh_dir_node_pool)
	{
		memPoolDestroy(m_mod_refresh_dir_node_pool);
	}

	if(NULL != m_mod_refresh_rf_write_buffer_pool)
	{
		memPoolDestroy(m_mod_refresh_rf_write_buffer_pool);
	}

	return;
}


/************ mempool api ************/



/* release an object from a cache */
/* for dir real_time purge refresh */
/************like the storeRelease func************/
static int m_storeRelease(StoreEntry * e)
{
	if (storeEntryLocked(e))
	{
		return 0;
	}
	if (store_dirs_rebuilding && e->swap_filen > -1)
	{
		if (e->swap_filen > -1)
		{
			return 0;
		}
	}

	return 1;
}

void hash_init()
{
	memset(refresh_hash_array, 0, sizeof(refresh_hash_array));
	dir_count = 0;
}

char *rf_url_hostname(const char *url, char *hostname)
{
	size_t len = strlen(url);

	if (len > M_MAX_URL_LEN)
	{
		debug(91, 0)("mod_refresh: url length %d too long than %d, drop it: %s\n", len, M_MAX_URL_LEN, url);
		return NULL;
	}
	char *a = strstr(url, "//");
	if(a == NULL)
	{
		debug(91, 0)("mod_refresh: url format error, drop it: %s\n", url);
		return NULL;
	}
	a += 2;
	strcpy(hostname,a);
	char *c = hostname;
	while(*c && *c != '/')
	{
		c++;
	}
	if(*c != '/')
	{
		debug(91, 0)("mod_refresh: url format error, drop it: %s\n", url);
		return NULL;
	}
	*c = '\0';

	return hostname;
}

// BKDR Hash Function
unsigned int BKDRHash(char *str)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	while (*str)
	{
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}

unsigned int hash_pos(char *dir)
{
	return BKDRHash(dir) % M_MAX_DIR_REFRESH_HASH_NUM;
}

int url_match_prefix(const char *token, const char *url)
{
	return strncasecmp(token, url, strlen(token));
}

int url_match_wildcard(const char *token, const char *url)
{
	return fnmatch(token, url, 0);
}

bool dir_in_list(char *dir, char *sessid)
{
	int i;
	char host_name[4096];

	memset(host_name, 0, sizeof(host_name));

	if(rf_url_hostname(dir, host_name) == NULL)
		return true;

	//Add Start: for refreshd dir host wildcard, by xin.yao
	if (strchr(host_name, '*') != NULL)
	{
		strncpy(host_name, sessid, 4096);
	}
	//Add Ended: for refreshd dir host wildcard, by xin.yao

	i = hash_pos(host_name);

	if(refresh_hash_array[i].count == 0)
		return false;

	struct refresh_dir_node *tmp = NULL;
	tmp = refresh_hash_array[i].head;

	while(tmp)
	{
		//already in list
		if((strcasecmp(dir, tmp->dir)) == 0)
		{
			return true;
		}

		tmp = tmp->next;
	}

	return false;
}

struct refresh_dir_node *find_dir_node(char *url, char *sessid)
{
	int i;
	char host_name[4096];
	struct refresh_dir_node *tmp = NULL;

	if(url == NULL || sessid == NULL)
		return NULL;
	memset(host_name, 0, sizeof(host_name));
	if(rf_url_hostname(url, host_name) == NULL)
	{
		return NULL;
	}
	i = hash_pos(host_name);
	if(refresh_hash_array[i].count == 0)
	{
		goto RE_MATCH;
		//return NULL;
	}
	tmp = refresh_hash_array[i].head;
	while(tmp)
	{
		if(strcmp(sessid, tmp->sessionid) == 0)
		{
			return tmp;
		}
		tmp = tmp->next;
	}
RE_MATCH:
	//Add Start: for refreshd dir host wildcard, by xin.yao
	strncpy(host_name, sessid, 4096);
	i = hash_pos(host_name);
	if(refresh_hash_array[i].count == 0)
		return NULL;
	tmp = refresh_hash_array[i].head;
	while(tmp)
	{
		if(strcmp(sessid, tmp->sessionid) == 0)
		{
			return tmp;
		}
		tmp = tmp->next;
	}
	//Add Ended: for refreshd dir host wildcard, by xin.yao
	return NULL;
}


void insert_dir_to_list(char *sessid, int action, char *dir, dir_compare* func)
{
	int i = 0;
	char host_name[4096];

	memset(host_name, 0, sizeof(host_name));

	if(rf_url_hostname(dir, host_name) == NULL)
		return ;

	//Add Start: for refreshd dir host wildcard, by xin.yao
	if (strchr(host_name, '*') != NULL)
	{
		strncpy(host_name, sessid, 4096);
	}
	//Add Ended: for refreshd dir host wildcard, by xin.yao

	struct refresh_dir_node *new_dir_node = NULL;

	i = hash_pos(host_name);

	if(refresh_hash_array[i].count == 0)
	{
		refresh_hash_array[i].head = NULL;
		refresh_hash_array[i].count = 0;
	}

	new_dir_node = mod_refresh_dir_node_pool_alloc();
	memset(new_dir_node->dir, 0, sizeof(new_dir_node->dir));
	memset(new_dir_node->sessionid, 0, sizeof(new_dir_node->sessionid));
	strcpy(new_dir_node->dir, dir);
	strcpy(new_dir_node->sessionid, sessid);
	new_dir_node->action = action;
	new_dir_node->recv_time = squid_curtime;
	new_dir_node->count = 0;
	new_dir_node->compare_func = func;

	debug(91, 3)("add new dir refresh to dirlist:\n\tsessionid:\t[%s]\n\tdir:\t\t[%s]\n\trecv_time:\t[%ld]\n", sessid, dir, squid_curtime);

	new_dir_node->next = refresh_hash_array[i].head;
	refresh_hash_array[i].head = new_dir_node;
	refresh_hash_array[i].count++;

	dir_count++;

	return;
}

void delete_dir_from_list(char *dir, char *sessid)
{
	int i = 0;
	char host_name[4096];

	memset(host_name, 0, sizeof(host_name));

	if(rf_url_hostname(dir, host_name) == NULL)
		return ;

	//Add Start: for refreshd dir host wildcard, by xin.yao
	if (strchr(host_name, '*') != NULL)
	{
		strncpy(host_name, sessid, 4096);
	}
	//Add Ended: for refreshd dir host wildcard, by xin.yao

	struct refresh_dir_node *tmp;
	struct refresh_dir_node *tmp2;

	i = hash_pos(host_name);

	if(refresh_hash_array[i].count == 0)
		return;

	tmp = refresh_hash_array[i].head;

	if(strcmp(dir, tmp->dir) == 0)
	{
		refresh_hash_array[i].head = tmp->next;
		refresh_hash_array[i].count--;

		dir_count--;

		memPoolFree(m_mod_refresh_dir_node_pool, tmp);
		tmp = NULL;

		return;
	}

	while(tmp->next != NULL)
	{
		if(strcmp(dir, tmp->next->dir) == 0)
		{
			tmp2 = tmp->next;
			tmp->next = tmp->next->next;
			refresh_hash_array[i].count--;

			dir_count--;

			memPoolFree(m_mod_refresh_dir_node_pool, tmp2);
			tmp2 = NULL;

			return;
		}

		tmp = tmp->next;
	}

	return;
}

void clean_refresh_hash_array()
{
	int i;
	struct refresh_dir_node *tmp = NULL;
	struct refresh_dir_node *tmp2 = NULL;

	for( i = 0; i < M_MAX_DIR_REFRESH_HASH_NUM; i++)
	{
		if(refresh_hash_array[i].count != 0)
		{
			tmp = refresh_hash_array[i].head;

			while(tmp)
			{
				tmp2 = tmp->next;

				memPoolFree(m_mod_refresh_dir_node_pool, tmp);
				tmp = NULL;

				refresh_hash_array[i].count--;
				dir_count--;

				tmp = tmp2;
			}

			assert(refresh_hash_array[i].count == 0);

		}
	}

	assert(dir_count == 0);
}

void check_refresh_hash_array()
{
	int i;
	struct refresh_dir_node *tmp = NULL;
	struct refresh_dir_node *tmp2 = NULL;

	debug(91, 0)("current dir refresh count: %d\n", dir_count);

	for( i = 0; i < M_MAX_DIR_REFRESH_HASH_NUM; i++)
	{
		if(refresh_hash_array[i].count != 0)
		{
			tmp = refresh_hash_array[i].head;

			while(tmp)
			{
				tmp2 = tmp->next;

				if (squid_curtime - tmp->recv_time > max_dir_refresh_time)
				{
					debug(91, 0)("dir: %s realtime dir refresh timeout, cancel it!\n", tmp->dir);

					delete_dir_from_list(tmp->dir, tmp->sessionid);
				}
				else
				{
					debug(91, 0)("current dir refresh: %s\n", tmp->dir);
				}

				tmp = tmp2;
			}
		}
	}

	eventAdd("rf_check_refresh_hash_array", check_refresh_hash_array, NULL, 60, 0);
}

/********** realtime dir refresh **********/

static pid_t get_running_pid(void)
{
	static pid_t last_pid = -1;

	if( last_pid == -1 ) 
	{
		FILE *fd;
		if( (fd = fopen(REFRESHD_PIDFILE, "r")) == NULL )
		{
			debug(91, 1) ("mod_refresh : %s open error\n",REFRESHD_PIDFILE);
			return last_pid;
		}

		char pid[128];
		fgets(pid, sizeof(pid), fd);
		fclose(fd);

		last_pid = atoi(pid);

		debug(91, 1) ("mod_refresh : refreshd last_pid=[%d]\n",last_pid);

		if( last_pid < 0 ) 
		{
			debug(91, 1) ("mod_refresh : refreshd last_pid error!! [%d]\n",last_pid);
			last_pid = -1;
			return -1;
		}
	}

	assert(last_pid != -1);

	char buff[1024];
	FILE *procfd;
	snprintf(buff, sizeof(buff), "/proc/%d/status", last_pid);

	if( (procfd = fopen(buff,"r")) == NULL ) 
	{
		debug(91, 1) ("mod_refresh : %s open error\n",buff);
		last_pid = -1;
		return last_pid;
	}

	fgets(buff, sizeof(buff), procfd);
	fclose(procfd);
	if( strstr(buff, "refreshd") == NULL ) 
	{
		debug(91, 1) ("mod_refresh : no refreshd pid in the /proc/.../status.s\n");
		last_pid = -1;
		return last_pid;
	}

	return last_pid;
}


static int rf_get_return_action(int action)
{
	int ret_action = -1;

	switch (action)
	{
		case RF_FC_ACTION_URL_PURGE:
			ret_action = RF_FC_ACTION_URL_PURGE_RET;
			break;
		case RF_FC_ACTION_URL_EXPIRE:
			ret_action = RF_FC_ACTION_URL_EXPIRE_RET;
			break;
		case RF_FC_ACTION_DIR_REFRESH_FINISH:
			ret_action = RF_FC_ACTION_DIR_REFRESH_RESULT;
			break;
		default:
			return -1;
	}

	return ret_action;
}

static void rf_disconnect(int fd)
{
	if(fd < 0)
		return ;

	rf_clean_rw_buffer();
	clean_refresh_hash_array();

	commSetSelect(fd,COMM_SELECT_READ,NULL,NULL,0);
	commSetSelect(fd,COMM_SELECT_WRITE,NULL,NULL,0);
	comm_close(fd);
}

static bool is_socket_alive(int sock_fd)
{
	int error;
	socklen_t e_len = sizeof(error);
	if ( getsockopt(sock_fd,SOL_SOCKET,SO_ERROR,&error,&e_len) < 0||error != 0)
		return false;
	return true;
}

static int rf_parse_buff(void *buf)
{
	rf_read_buffer *rf_buff = (rf_read_buffer *)buf;

	char buffer[READ_BUFFER_SIZE];
	int i,a;
	i = a = 0;

	for (; i < rf_buff->len;i++)
	{
		if (rf_buff->buffer[i] == '\n')
		{
			memset(buffer,0,READ_BUFFER_SIZE);
			memcpy(buffer,rf_buff->buffer + a, i - a);

			while (rf_buff->buffer[i] == '\n') i++;

			int ret = rf_process_cmd(buffer);

			if (ret != 0)
			{
				debug(91, 0) ("refresh : process cmd error!\n");
			}

			a = i;
		}
	}

	if (i >= rf_buff->len && (a != rf_buff->len))
	{
		i = rf_buff->len;
		memmove(rf_buff->buffer,rf_buff->buffer + a,i - a);
		rf_buff->len = i - a;
	}
	else
	{
		rf_buff->len = 0;
	}

	return 0;
}

static void rf_module_read_handle(int fd,void *buf)
{
	int retry = 0;
	rf_read_buffer *rf_buff = (rf_read_buffer *)buf;

	if(rf_buff->len == READ_BUFFER_SIZE)
		goto OUT;

	memset(rf_buff->buffer + rf_buff->len, 0, READ_BUFFER_SIZE - rf_buff->len);

	do {
		int ret = recv(fd, rf_buff->buffer + rf_buff->len, READ_BUFFER_SIZE - rf_buff->len, 0);
		if (0 == ret)
		{
			//connect closed!
			debug(91, 2)("(mod_refresh) --> refreshd close the socket !!!!\n");
			rf_disconnect(refresh_socket);
			refresh_socket = -1;
			return;
		}
		else if (ret < 0)
		{
			//error
			if (ignoreErrno(errno))
			{
				//try again: EAGAIN, EINTR
				continue;
			}
			debug(91,2)("(mod_refresh) --> recv from refreshd error!\n");
			goto OUT;
		}
		rf_buff->len += ret;
		break;
	} while (++retry < 3);
	rf_parse_buff(rf_buff);
OUT:
	commSetSelect(fd,COMM_SELECT_READ,rf_module_read_handle,&rf_buf,0);
}

static void rf_clean_read_buffer()
{
	memset(rf_buf.buffer, 0, READ_BUFFER_SIZE);
	rf_buf.len = 0;

	return;
}

static void rf_module_write_handle(int fd,void *buf)
{
	if ((refresh_socket == fd) &&  (refresh_socket_flag == false))
	{
		if (is_socket_alive(fd))
		{
			debug(91,2)("(mod_refresh) --> connection to refreshd successed!");
			rf_send_id();
			commSetSelect(fd,COMM_SELECT_READ,rf_module_read_handle,&rf_buf,0);
			refresh_socket_flag = true;
		}
		else
		{
			rf_disconnect(fd);
			refresh_socket = -1;
			return;
		}
	}

	int len = 0;
	int s_count = 0;
	int retry = 0;

	while (rf_wl.first)
	{
		retry = 0;
		rf_write_buffer *v = rf_wl.first;
		do {
			len = write(fd, v->buffer + v->send_len, v->len - v->send_len);
			if (len < 0)
			{
				if (ignoreErrno(errno))
				{
					//ignore error: EAGAIN, EINTR
					continue;
				}
				rf_disconnect(fd);
				refresh_socket = -1;
				return;
			}
			v->send_len += len;
			if (v->send_len == v->len)
			{
				rf_wl.first = rf_wl.first->next;
				rf_wl.count--;
				memPoolFree(m_mod_refresh_rf_write_buffer_pool, v);
				v = NULL;
				s_count++;
			}
			break;
		} while (++retry < 3);
		if (retry >= 3)
		{
			rf_wl.first = rf_wl.first->next;
			rf_wl.count--;
			memPoolFree(m_mod_refresh_rf_write_buffer_pool, v);
			v = NULL;

			s_count++;

			//			if((s_count % 200) == 0)
			//				break;
		}
	}

	if (rf_wl.first == NULL)
	{
		assert(rf_wl.count == 0);
		rf_wl.last = NULL;
	}
	else
	{
		commSetSelect(fd,COMM_SELECT_WRITE,rf_module_write_handle,NULL,0);
	}
}

static void rf_clean_write_list()
{
	while (rf_wl.first)
	{
		rf_write_buffer *v = rf_wl.first;
		rf_wl.first = rf_wl.first->next;
		rf_wl.count--;
		memPoolFree(m_mod_refresh_rf_write_buffer_pool, v);
		v = NULL;
	}

	if (rf_wl.first == NULL)
	{
		assert(rf_wl.count == 0);
		rf_wl.last = NULL;
	}
}

static void rf_clean_rw_buffer()
{
	rf_clean_read_buffer();
	rf_clean_write_list();
}



//pid file is unique in multi-squid,so get pidfile as fc_identity
static char * get_fc_ident()
{
	static char ident[256];
	if (*ident)
		return ident;

	char *c = Config.pidFilename + strlen(Config.pidFilename);

	while (*c != '/') c--;

	strcpy(ident,c + 1);
	debug(91,2)("(mod_refresh) --> squid identity : %s\n",ident);

	return ident;
}

static int rf_module_connect(int flag)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if ( fd < 0 )
	{
		return -1;
	}

	struct sockaddr_in  maddress;
	memset(&maddress, 0, sizeof(struct sockaddr_in));
	if (inet_aton(REFRESHD_ADDRESS,&maddress.sin_addr) == 0)
	{
		close(fd);
		return -1;
	}

	maddress.sin_family = AF_INET;
	maddress.sin_port   = htons(REFRESHD_PORT);

	fd_open(fd,FD_SOCKET,"socket_to_refreshd");

	if (flag)
	{
		int ret = commSetNonBlocking(fd);
		if (ret != 0)
		{
			comm_close(fd);
			return ret;
		}
	}
	commSetCloseOnExec(fd);

	refresh_socket_flag = false;
	int n = 0;
	if ( (n = connect(fd , (struct sockaddr *)&maddress, sizeof(struct sockaddr))) != 0)
	{
		if ( errno != EINPROGRESS )
		{
			debug(91, 0) ("connect to refreshd error! (%s)\n",strerror(errno));
			comm_close(fd);
			return -1;
		}
		debug(91, 1) ("waiting refreshd connect...,fd : %d\n",fd);
	}
	else
	{
		rf_send_id();
		refresh_socket_flag = true;
	}

	struct sockaddr_in sin;
	int len = sizeof(struct sockaddr_in);
	if(getsockname(fd,(struct sockaddr *)&sin, (socklen_t *)&len) == 0){
		int port = ntohs(sin.sin_port);
		if(port == REFRESHD_PORT){
			debug(91,0)("connect to refreshd ,port equal to refreshd(21108),so close it!!");
			comm_close(fd);
			return -1;
		}
	}

	commSetSelect(fd,COMM_SELECT_WRITE,rf_module_write_handle,NULL,0);
	refresh_socket = fd;
	return 0;
}

static int rf_wl_append(rf_write_buffer *s,rf_write_buffer *d){
	if((s->len + d->len) >  WRITE_BUFFER_SIZE){
		return -1;
	}

	memcpy(d->buffer + d->len,s->buffer,s->len);
	d->len += s->len;
	memPoolFree(m_mod_refresh_rf_write_buffer_pool,s);
	s = NULL;

	return 0;
}

static void rf_wl_add(rf_write_buffer *wb,int action){
	assert(wb != NULL);

	if(rf_wl.first == NULL){
		assert(rf_wl.last == NULL);
		assert(rf_wl.count == 0);

		rf_wl.first = rf_wl.last = wb;
		rf_wl.count++;
	}
	else if(action == RF_WL_ACTION_ADD_TAIL){
		if(rf_wl_append(wb,rf_wl.last) != 0){
			rf_wl.last->next = wb;
			rf_wl.last = wb;

			rf_wl.count++;
		}
	}
	else if(rf_wl_append(wb,rf_wl.last) != 0){
		wb->next = rf_wl.first->next;
		rf_wl.first->next = wb;

		//wb is the last one!
		if(wb->next == NULL)
			rf_wl.last = wb;

		rf_wl.count++;
	}

	if(refresh_socket > 0)
		commSetSelect(refresh_socket,COMM_SELECT_WRITE,rf_module_write_handle,NULL,0);
}

static rf_write_buffer * rf_cmd_gen_wl(int action,const char *url,char *other){
	rf_write_buffer *wb = mod_rf_write_buffer_pool_alloc();
	memset(wb,0,sizeof(rf_write_buffer));

	char *fc_id = get_fc_ident();
	wb->len += snprintf(wb->buffer + wb->len,WRITE_BUFFER_SIZE - wb->len,"%s",fc_id);	wb->len++;
	wb->len += snprintf(wb->buffer + wb->len,WRITE_BUFFER_SIZE - wb->len,"%d",action);	wb->len++;

	if(url != NULL){
		wb->len += snprintf(wb->buffer + wb->len,WRITE_BUFFER_SIZE - wb->len,"%s",url);
	}
	wb->len++;	//skip '\0'

	if(other != NULL){
		wb->len += snprintf(wb->buffer + wb->len,WRITE_BUFFER_SIZE - wb->len,"%s",other);
	}
	wb->len++;	//skip '\0'

	wb->len += snprintf(wb->buffer + wb->len,WRITE_BUFFER_SIZE - wb->len,"\n");

	return wb;
}

static int rf_process_url(int action,const cache_key *key)
{

	StoreEntry *e = storeGet(key);
	if (e == NULL)
	{
		return -1;
	}

	((action == RF_FC_ACTION_DIR_PURGE) || (action == RF_FC_ACTION_URL_PURGE)) ? storeRelease(e) : storeExpireNow(e);
	return 0;
}

static int rf_process_action(int action,char *sessionid,char *url,char *params)
{
	bool b_success = false;
	int retcode;

	//	fprintf(stderr, "squid process : action: %d, url: %s\n", action, url);

	int ret = rf_process_url(action,storeKeyPublic(url,METHOD_GET));
	if (ret == 0)
	{
		b_success = true;
		debug(91,2)("(mod_refresh) --> action %d,url : %s,method : GET,success!\n",action,url);
	}


	ret = rf_process_url(action,storeKeyPublic(url,METHOD_HEAD));
	if (ret == 0)
	{
		b_success = true;
		debug(91,2)("(mod_refresh) --> action %d,url : %s,method : HEAD,success!\n",action,url);
	}


	if (! b_success)
	{
		debug(91,5)("(mod_refresh) --> action : %d,url : %s , do not find any storeentry !!\n",action,url);
		retcode = 1000; //return 200
	}
	else
	{
		retcode = 200;
	}

	//	fprintf(stderr, "process result: %d\n", retcode);

	static char buffer[256];
	memset(buffer,0,256);
	snprintf(buffer,256,"%s,%d",sessionid,retcode);

	if((action == RF_FC_ACTION_DIR_PURGE) || (action == RF_FC_ACTION_DIR_EXPIRE))
	{
		struct refresh_dir_node *dir_node = NULL;
		if((dir_node = find_dir_node(url, sessionid)) != NULL)
		{
			dir_node->count++;
		}
		else
		{
			debug(91, 2)("(mod_refresh) --> dir refresh action recved, but not find session:[ %s ] in dir list\n", sessionid);
		}	

	}
	else if((action == RF_FC_ACTION_URL_PURGE) || (action == RF_FC_ACTION_URL_EXPIRE))
	{
		rf_write_buffer *wb = rf_cmd_gen_wl(rf_get_return_action(action),url,buffer);
		rf_wl_add(wb,RF_WL_ACTION_ADD_FIRST);
	}

	return 0;
}

static int rf_process_cmd(char *buff)
{
	int action;
	char buffer[256];
	char *sessionid;
	char *a,*b,*c,*d;
	a = buff;
	b = a + strlen(a) + 1;
	c = b + strlen(b) + 1;
	d = c + strlen(c) + 1;

	debug(91,2)("(mod_refresh) --> action : %s,url : %s ,sessionid : %s,params : %s\n",a,b,c,d);
	//	fprintf(stderr,"mod_refresh-->action: %s, url: %s, sessionid: %s, params: %s\n", a, b, c, d); 
	action = atoi(a);
	sessionid = c;

	switch (action)
	{
		case RF_FC_ACTION_URL_PURGE:
		case RF_FC_ACTION_URL_EXPIRE:
		case RF_FC_ACTION_DIR_PURGE:
		case RF_FC_ACTION_DIR_EXPIRE:
			rf_process_action(action,sessionid,b,d);
			break;
		case RF_FC_ACTION_VERIFY_START:
			rf_verify_start();
			rf_store_table_walk();
			break;
		case RF_FC_ACTION_DIR_EXPIRE_REFRESH_PREFIX_START:
		case RF_FC_ACTION_DIR_PURGE_REFRESH_PREFIX_START:
			if(strlen(d) != 0)
				max_dir_refresh_time = atoi(d);

			if(!dir_in_list(b, c))
			{
				if(dir_count >= M_MAX_DIR_REFRESH_NUM)
				{
					debug(91,0)("WARNING: there are too many realtime refresh dirs, ignore this one: %s\n", b);
					break;
				}

				debug(91, 9)("c:::::::::%s\n", c);
				insert_dir_to_list(c, action, b, url_match_prefix);
			}
			break;
		case RF_FC_ACTION_DIR_EXPIRE_REFRESH_WILDCARD_START:
		case RF_FC_ACTION_DIR_PURGE_REFRESH_WILDCARD_START:
			if(strlen(d) != 0)
				max_dir_refresh_time = atoi(d);

			if(!dir_in_list(b, c))
			{
				if(dir_count >= M_MAX_DIR_REFRESH_NUM)
				{
					debug(91,0)("WARNING: there are too many realtime refresh dirs, ignore this one: %s\n", b);
					break;
				}

				debug(91, 9)("c:::::::::%s\n", c);
				insert_dir_to_list(c, action, b, url_match_wildcard);
			}
			break;
		case RF_FC_ACTION_DIR_REFRESH_FINISH:
			memset(buffer,0,256);

			struct refresh_dir_node *dir_node = NULL;
			if((dir_node = find_dir_node(b, sessionid)) != NULL)
			{
				snprintf(buffer,256,"%s,%d",sessionid,dir_node->count);
				rf_write_buffer *wb = rf_cmd_gen_wl(rf_get_return_action(action),b,buffer);
				rf_wl_add(wb,RF_WL_ACTION_ADD_TAIL);
			}
			else
			{
				debug(91, 2)("(mod_refresh) --> dir refresh finish action recved, but not find session in dir list\n");
			}
			if(dir_in_list(b, c))
			{
				delete_dir_from_list(b, c);
			}
			break;

		default:
			debug(91,2) ("can not process cmd : %d\n",action);
			break;
	}

	return 0;
}

static int rf_start_refreshd()
{
	enter_suid();

	int cid = fork();
	if (cid == 0) 	//child
	{
		int ret = execl(REFRESHD_PATH, "refreshd","-d",(char *)0);;
		if (ret < 0)
		{
			debug(91,2)("(mod_refresh) --> execl error : %s\n",xstrerror());
		}
		exit(-1);
	}
	else if (cid < 0)
	{
		debug(91,2) ("fork error!!\n");
		leave_suid();
		return -1;
	}

	leave_suid();
	return 0;
}

static void rf_check_refreshd()
{
	pid_t pid = get_running_pid();

	if (pid < 0)
	{
		debug(91,2)("(mod_refresh) --> Refreshd stopped! Now starting it ...\n");

		rf_clean_rw_buffer();
		clean_refresh_hash_array();

		int ret = rf_start_refreshd();
		if (ret != 0)
		{
			debug(91,1)("(mod_refresh) --> ERROR : start refreshd error!!!\n");
		}
	}

	eventAdd("rf_check_refreshd", rf_check_refreshd, NULL, 60, 0);
}

static void rf_check_connection()
{
	pid_t pid = get_running_pid();
	if (pid < 0)
		goto OUT;

	if(refresh_socket > 0 ){
		if(is_socket_alive(refresh_socket))
			goto OUT;
		else{
			rf_disconnect(refresh_socket);
			refresh_socket = -1;
		}
	}
	debug(91,2) ("Connection with refreshd has been closed , now connect to refreshd ..\n");
	rf_module_connect(1);
OUT:
	eventAdd("rf_check_connection", rf_check_connection, NULL,60, 0);
}

//refresh module init , call it when squid init
int rf_module_init()
{
start:
	start_times++;
	pid_t pid = get_running_pid();

	if (pid < 0)
	{
		int wait_times = 0;
		debug(91,1)("(mod_refresh) --> Refreshd stopped! Now starting it ...\n");
		int ret = rf_start_refreshd();
		if (ret != 0)
		{
			debug(91,1)("(mod_refresh) --> ERROR : start refreshd error!!!\n");
		}

		while(1)
		{
			wait_times++;
			sleep(1);
			pid_t pid = get_running_pid();
			if (pid < 0)
			{
				debug(91,2)("(mod_refresh) --> ERROR : refreshd is not running !!!\n");
				if(wait_times == 10)
				{
					if(start_times >= 3)
					{
						debug(91,1)("mod_refresh) --> ERROR: can't start the refreshd, exit....\n");
						abort();
					}
					else
					{
						debug(91, 1)("mod_refresh) --> restart refreshd in init module\n");
						goto start;
					}
				}
			}
			else
			{
				break;
			}
		}
	}

	rf_wl.first = rf_wl.last = NULL;
	rf_wl.count = 0;
	
	hash_init();
	rf_module_connect(1);
	eventAdd("rf_check_refreshd", rf_check_refreshd, NULL, 60, 0);
	eventAdd("rf_check_connection", rf_check_connection, NULL,60, 0);
	eventAdd("rf_check_refresh_hash_array", check_refresh_hash_array, NULL, 60, 0);

	return 0;
}

static void rf_store_table_walk(){
	hash_link *link;

	if(rf_wl.count > (int)(rf_list_limit / 2)){
		eventAdd("rf_store_table_walk", rf_store_table_walk, NULL, 1, 0);
		return;
	}

	debug(91,2)("(mod_refresh) --> store_table->size : %d,rf_store_pos : %d, url_verify : %d\n",store_table->size,rf_store_pos,rf_verify_number);

	int i = 0;
	int ve_count = 0;
	for(;i < store_table->size ;i++) {
		rf_store_pos++;
		ve_count = 0;

		if(rf_store_pos >= store_table->size) {
			rf_verify_finish();
			debug(91,1)("Squid verify %d storeEntry!\n",rf_verify_number);
			break;
		}

		link = store_table->buckets[rf_store_pos];
		if(NULL != link) {
			while(link) {
				StoreEntry* e = (StoreEntry*)link;
				const char *key = storeKeyText(e->hash.key);

				if(key != NULL){
					if(! EBIT_TEST(e->flags, ENTRY_SPECIAL)){
						storeExpireNow(e);
						ve_count++;
					}
				}
				link = link->next;
			}
		}

		rf_verify_number += ve_count;

		//避免一直占有squid资源
		if(rf_verify_number && ve_count && ((rf_verify_number % (int)(rf_list_limit / 2)) == 0)){
			eventAdd("rf_store_table_walk", rf_store_table_walk, NULL, 0.1, 0);
			break;
		}
	}
}

void rf_send_id()
{
	rf_write_buffer *wb = rf_cmd_gen_wl(RF_FC_ACTION_INFO,NULL,NULL);
	rf_wl_add(wb,RF_WL_ACTION_ADD_TAIL);
}

//url verify start
void rf_verify_start()
{
	debug(91,2) ("starting verify ...\n");
	rf_verify_number = 0;
	rf_store_pos  = 0;
	run_status = RF_URL_VERIFYING;

	rf_write_buffer *wb = rf_cmd_gen_wl(RF_FC_ACTION_VERIFY_START,NULL,NULL);
	rf_wl_add(wb,RF_WL_ACTION_ADD_TAIL);
}

void rf_verify_finish()
{
	debug(91,2) ("finish verify ...\n");
	run_status = RF_NORMAL;
	rf_write_buffer *wb = rf_cmd_gen_wl(RF_FC_ACTION_VERIFY_FINISH,NULL,NULL);
	rf_wl_add(wb,RF_WL_ACTION_ADD_TAIL);
}

void rf_add_url(const char *url)
{
	//FIXME:it will loss some URL
	if(rf_wl.count > rf_list_limit)
	{
		debug(91,0)("Warning !!! write list url count : %d\n",rf_wl.count);

	return;
	}

	//ignore len> 4096 url
	if(strlen(url) > 4096) 
	{	
		return;
	}

	rf_write_buffer *wb = rf_cmd_gen_wl(RF_FC_ACTION_ADD,url,NULL);
	rf_wl_add(wb,RF_WL_ACTION_ADD_TAIL);
}

void rf_module_dest()
{
	if (refresh_socket != -1)
		rf_disconnect(refresh_socket);
}

/****************** start module ****************************/

static int cleanup(cc_module *module)
{
	debug(91,3)("(mod_refresh) --> cc_mod_refresh cleanup\n");
	rf_module_dest();
	mod_refresh_mempool_cleanup();

	return 1;
}

void hook_rf_verify_start(){
	if(store_dirs_rebuilding){
		debug(91,3)("(mod_refresh) --> store_dirs_rebuilding == %d,eventAdd -> hook_rf_verify_start\n",store_dirs_rebuilding);
		eventAdd("hook_rf_verify_start", hook_rf_verify_start, NULL, 60, 0);
	}
	else
		rf_verify_start();
}

static int hook_init(cc_module *module)
{
	debug(91,3)("(mod_refresh) --> cc_mod_refresh hook_init\n");

	/* refresh init */
	start_times = 0;
	rf_module_init();

	//do not verify when startup
	//	hook_rf_verify_start();
	return 1;
}

static int hook_http_read_reply(int fd,HttpStateData *data)
{
	debug(91,3)("(mod_refresh) --> cc_mod_refresh hook_http_read_reply\n");
	StoreEntry *entry = data->entry;
	const request_t *request = data->request;
	int flag = 1;

	if (EBIT_TEST(entry->flags, RELEASE_REQUEST))
		flag = 0;
	if (EBIT_TEST(entry->flags, KEY_PRIVATE))
		flag = 0;
	if (EBIT_TEST(entry->flags, ENTRY_SPECIAL))
		flag = 0;
	if (EBIT_TEST(entry->flags, ENTRY_ABORTED))
		flag = 0;
	if (!EBIT_TEST(entry->flags, ENTRY_CACHABLE))
		flag = 0;
	if(request->method != METHOD_GET)
		flag = 0;
	if(data->entry->mem_obj->request->flags.refresh)
		flag = 1;

	if(flag){
		const char *url = storeLookupUrl(entry);
		debug(91,3) ("rf_add_url -- url : %s\n",url);
		rf_add_url(url);
	}
	return 1;
}

static int hook_private_http_req_process2(clientHttpRequest *http)
{
	debug(91,3)("(mod_refresh) --> cc_mod_refresh hook_private_http_req_process2\n");

	if(dir_count == 0)
		return 0;

	StoreEntry *entry = http->entry;

	if(entry == NULL)
		return 0;

	const char *url = ((http->request->store_url != NULL)?http->request->store_url:http->request->canonical);

	if(url == NULL)
		return 0;

	debug(91, 3)("request url: %s\n", url);

	int i;

	if(rf_url_hostname(url, hostname) == NULL)
		return 0;

	i = hash_pos(hostname);

	if(refresh_hash_array[i].count == 0)
		return 0;

	debug(91, 3)("this url in hash[%d], dir nums: %d\n", i, refresh_hash_array[i].count);

	struct refresh_dir_node *tmp = NULL;
	tmp = refresh_hash_array[i].head;

	while(tmp)
	{
		if((entry->timestamp > tmp->recv_time) && (entry->timestamp <= squid_curtime))
		{
			debug(91,3)("request url: %s entry timestamp time: %ld > dir refresh time: %ld, hit it!\n", url, entry->timestamp, tmp->recv_time);
			return 0;
		}

		if(tmp->compare_func(tmp->dir, url) == 0)
		{
			debug(91,3)("request url: %s , entry timestamp: [[%ld]]  dir recv_time[[%ld]]\n", url, entry->timestamp,tmp->recv_time);
			debug(91,3)("request url: %s match refresh dir: %s , expire/release its entry now!\n", url, tmp->dir);

			if((tmp->action == RF_FC_ACTION_DIR_EXPIRE_REFRESH_PREFIX_START) || (tmp->action == RF_FC_ACTION_DIR_EXPIRE_REFRESH_WILDCARD_START))
			{
				storeExpireNow(entry);
				return 0;
			}
			else
				//		storeRelease(entry);	
			{
				//ReleaseRequese bit will be set
				if(m_storeRelease(entry) == 0)
				{
					storeRelease(entry);	
				}
				else	//store entry will be released
				{
					storeRelease(entry);	
					http->entry = NULL;
				}

#if USE_CACHE_DIGESTS
				http->lookup_type = http->entry ? "HIT" : "MISS";
#endif
				return LOG_TCP_MISS;
			}
		}

		tmp = tmp->next;
	}

	return 0;
}

/* module init */
int mod_register(cc_module *module)
{
	debug(91,3)("(mod_refresh) --> cc_mod_refresh init: init module\n");

	strcpy(module->version, "7.0.R.16488.i686");
	/* necessary functions*/
	//module->hook_func_sys_cleanup = cleanup;
	cc_register_hook_handler(HPIDX_hook_func_sys_cleanup,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_cleanup),
			cleanup);
	//module->hook_func_sys_init = hook_init;
	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			hook_init);

	//module->hook_func_http_repl_read_start = hook_http_read_reply;
	cc_register_hook_handler(HPIDX_hook_func_http_repl_refresh_start,
			module->slot, 
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_http_repl_refresh_start),
			hook_http_read_reply);

	//module->hook_func_private_http_req_process2(clientHttpRequest *http) = hook_http_req_process2;
	cc_register_hook_handler(HPIDX_hook_func_private_http_req_process2,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_private_http_req_process2),
			hook_private_http_req_process2);

	if(reconfigure_in_thread)
		mod = (cc_module*)cc_modules.items[module->slot];
	else
		mod = module;

	return 1;
}

#endif

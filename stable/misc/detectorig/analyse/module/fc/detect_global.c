#include "detect_orig.h"
#include "hashtable.h"

//const int kLINK_DATA_READ_COUNT_MAX = 3;        /* 最多读取kLINK_DATA_READ_COUNT_MAX个link.data文件中的数据 */
//const int kLINK_DATA_IGNORE_COUNT = 1;          /* 忽略掉最差的几个结果 */
HashTable *g_detect_tasks_index = NULL;
struct DetectOrigin **g_detect_tasks = NULL;	//record all the hosts information
int max_tasks_capacity = INITIAL_VALUE;         //current memory capacity of the tasks which malloced in need
struct origDomain **g_pstOrigDomain = NULL;
struct origDomainHistory **g_pstOrigDomainHistory = NULL;
int max_origDomain_capacity = INITIAL_VALUE;
int max_origDomainHistory_capacity = INITIAL_VALUE;
//detect_domain_t *g_detect_domain = NULL;
HashTable *g_detect_domain_table = NULL;
int max_detectDomain_capacity = INITIAL_VALUE;
int max_detectCustom_capacity = 100;
int max_detectUpperIp_capacity = 100;

int g_detect_task_count = 0;   //配置文件里实际有多少条合法记录
int g_detect_anyhost_count = 0;   //anyhost文件中有多少项合法记录
int handled_task_index = 0;		//detect中上次执行的task的索引号
int count = 0;  //统计运行的第几个实例
int detect_time;
int g_iOrigDomainItem = 0;
int g_iOrigDomainHistoryItem = 0;
float conf_timer = 3.0001;
struct timeval current_time;
double current_dtime;
time_t detect_curtime;
//long g_detect_timeout = PROCESS_TIMEOUT_DEFAULT;
struct config_st config;
struct detect_custom_conf_st custom_cfg;
char *custom_keyword[] = {
	"timeout",
	"proportion",
	NULL
};


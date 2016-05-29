#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <glob.h>
#include "display.h"
#include "child_mgr.h"
#include "assert.h"
#include "digRun.h"
#include "misc.h"

#define MAX_FP_NUM 20
#define MAX_IP_ONE_LINE 64
#define MAX_HOST_NUMBER 4096 
#define MAX_ONE_LINE_LENGTH 2048
#define CUSTOM_DIR "/usr/local/squid/etc/custom_domain"
int write_rcms_log(const char *filename, char *content);
static const int FIELD_NUMBER_ERROR = -1;
static const int HOST_REPEAT_ERROR = -2;
static const int METHOD_ERROR = -3;
static const int MEMORY_ERROR = -4;
static const int IP_ERROR = -5;
static const int DETECTWEIGHT_ERROR = -6;
static const char* g_pszErrorType[] = {"field_number", "host_repeat", "method_error",
	"memory_error", "ip"};

static const char IP_WORK[] = "ip_work";
static const char IP_GOOD[] = "ip_good";
static const char IP_BAD[] = "ip_bad";
static const char IP_DOWN[] = "ip_down";

//added by chinacache zenglj
static struct origDomain* g_pstOrigDomain[MAX_HOST_NUMBER];
static struct origDomainHistory * g_pstOrigDomainHistory[MAX_HOST_NUMBER];
static int g_iOrigDomainItem = 0;
static int g_iOrigDomainHistoryItem = 0;
//-------------------------------------------------------------------------------
/*一个要检查的ip的结构*/
struct IpDetect
{
	char ip[16];
	int ok;   //检查结果，0--失败，1--成功
	char returnCode[4];
	double usedtime;  //检查花费的时间，以秒为单位
	double usedtime1;
};


//added by chinacache zenglj
struct IpDnsDetect
{
	char ip[16];
	int ok;
};

struct origDomain
{
	/* Partition: configuration */
	char host[128];
	char origDomain[256];
	int useDefault;
	int ipNum;
	struct IpDnsDetect* ip;
	int travelDns;   /* 0: N, no travel; 1: Y, travel */
	int ignoreFailTimes; /* 0: N, no ignore; 1: Y, ignore*/
	/* Partition: state */
	int usedIP;
	int success;
	int needDetect;
	double usedtime;
	int working;
};

struct origDomainHistory
{
	char host[128];
	char origDomain[256];
	struct IpDetect* ip;
	int ipNum;
	int failtimes;
	int flag;
};

struct detectwithtags
{
	void* detectorigin;
	int tags;
	int ipNum;
};

struct file_handler {
	int num; 
	FILE *fp[MAX_FP_NUM];
};

void freeOneOrigDomain(struct origDomain* pstOrigDomain)
{
	if(NULL != pstOrigDomain->ip)
	{
		free(pstOrigDomain->ip);
		pstOrigDomain->ip = NULL;
	}
	if(NULL != pstOrigDomain )
	{
		free(pstOrigDomain);
		pstOrigDomain = NULL;
	}
}

void freeOneOrigDomainHistory(struct origDomainHistory* pstOrigDomainHistory)
{
	if(NULL != pstOrigDomainHistory->ip)
	{
		free(pstOrigDomainHistory->ip);
		pstOrigDomainHistory->ip = NULL;
	}
	if (NULL != pstOrigDomainHistory)
	{
		free(pstOrigDomainHistory);
		pstOrigDomainHistory = NULL;
	}
}

void clean_dig_run_mem(void)
{
	int i;

	for (i = 0; i < g_iOrigDomainItem; ++i)
	{
		freeOneOrigDomain(g_pstOrigDomain[i]);
	}
	for (i = 0; i < g_iOrigDomainHistoryItem; ++i)
	{
		freeOneOrigDomainHistory(g_pstOrigDomainHistory[i]);
	}
}

//modified by chinacache zenglj
// for mulit dig only host origdomain both equal,retutn repeat error
int checkRepeatOrigDomain(const char* host,const char *origDomain)
{
	int i;

	for (i = 0; i < g_iOrigDomainItem; ++i)
	{
		if (!strcmp(origDomain, g_pstOrigDomain[i]->origDomain) && 
				!strcmp(host,g_pstOrigDomain[i]->host))
		{
			return 1;
		}
	}

	return 0;
}

int checkRepeatOrigDomainHistory(const char* host)
{
	int i;

	for (i = 0; i < g_iOrigDomainHistoryItem; ++i)
	{
		if (!strcmp(host, g_pstOrigDomainHistory[i]->origDomain))
		{
			return 1;
		}
	}

	return 0;
}

//---------------------------------------------------------------------

/*检查一个ip是否合法（目前用inet_pton检测）
 *输入：
 *    ip----字符串表示的ip
 *返回值：
 *    0----合法
 *    -1----不合法
 */
int checkOneIp(const char* ip)
{
	unsigned int result;

	return (inet_pton(AF_INET, ip, &result) > 0 ? 0 : -1);
}

/*从配置文件ip字段得到各个ip，并分配内存
 *输入：
 *    str----配置文件ip字段
 *输出：
 *    ip----分配的ip结构
 *返回值：
 *    -1----没有ip
 *    -2----ip格式错误
 *    -3----内存错误
 *    整数----ip的个数
 */
int getIp(char *str, struct IpDetect **ip, int *ipNum)
{
	//得到第一个ip
	int i;
	int ret;
	char *ptr;
	ptr = NULL;
	char tempIp[MAX_IP_ONE_LINE][16];

	assert(ptr == NULL);

	str = strtok_r(str, "\t\n|:", &ptr);
	if (NULL == str)
	{
		return -1;
	}
	memset(tempIp, 0, sizeof(tempIp));
	if (0 == atoi(str))
	{
		return -1;
	}
	else
	{
		ret = checkOneIp(str);
		if (-1 == ret)
		{
			return -2;
		}
	}
	strcpy(tempIp[0], str);
	//得到其他的ip
	for (i = 1; i < MAX_IP_ONE_LINE; ++i)
	{
		str = strtok_r(NULL, "\t\n|:", &ptr);
		if (NULL == str)
		{
			break;
		}
		ret = checkOneIp(str);
		if (-1 == ret)
		{
			return -2;
		}
		strcpy(tempIp[i], str);
	}

	int number = i;
	int j, repeat;
	struct IpDetect *tmp;

	for (i = 0; i < number; ++i)
	{
		repeat = 0;
		/* check repeat IP */
		for (j = 0; j < *ipNum; ++j)
		{
			if (!strcmp((*ip)[j].ip, tempIp[i]))
			{
				repeat = 1;
				break;
			}
		}
		if (repeat)
		{
			/* ignore repeat IP */
			continue;
		}
		tmp = realloc(*ip, (*ipNum + 1) * sizeof(struct IpDetect));
		if (NULL == tmp)
		{
			return -3;
		}
		*ip = tmp;
		memset((*ip) + *ipNum, 0, sizeof(struct IpDetect));
		strcpy((*ip)[*ipNum].ip, tempIp[i]);
		(*ipNum)++;
	}

	return number;
}

int checkRepeatDnsIP(char string[][16], char *str, int len)
{
	int i;

	for(i = 0; i < len; ++i)
	{
		if(!strcmp(string[i], str))
			return 1;
	}

	return 0;
}

int getDnsIP(char *str, struct IpDnsDetect **ip, int useDefault)
{
	int ret;
	int i = 0;
	int none = 0;
	int number = 0;
	char *ptr = NULL;
	char tempIp[MAX_IP_ONE_LINE][16];

	if (!strcasecmp(str, "none"))
	{
		none = 1;
	}

	if (0 == none)
	{
		str = strtok_r(str, "|:", &ptr);
		if (NULL == str)
		{
			return -1;
		}
		memset(tempIp, 0, sizeof(tempIp));
		if (0 != atoi(str))
		{
			ret = checkOneIp(str);
			if (-1 == ret)
			{
				return -2;
			}
		}
		strcpy(tempIp[0], str);
		for (i = 1; i < MAX_IP_ONE_LINE; ++i)
		{
			str = strtok_r(NULL, "|:", &ptr);
			if (NULL == str)
			{
				break;
			}
			ret = checkOneIp(str);
			if (-1 == ret)
			{
				return -2;
			}
			strcpy(tempIp[i], str);
		}
		number = i;
	}
	/* parse default dns ip */
	if (1 == useDefault)
	{
		char fcexternalFilename[128];
		memset(fcexternalFilename, 0, sizeof(fcexternalFilename));
		strcpy(fcexternalFilename, "/usr/local/squid/etc/fcexternal.conf");

		FILE* fp = fopen(fcexternalFilename, "r");
		if (NULL == fp)
		{
			addInfoLog(2, "Open fcexternal.conf file error");
			//return -1;
		}
		else
		{
			//int i;
			char line[MAX_ONE_LINE_LENGTH];
			char* externalStr;
			char* externalIP;
			char* externalName;
			//for (i = 1; i < MAX_HOST_NUMBER; i++)
			while(NULL != (externalStr = fgets(line, MAX_ONE_LINE_LENGTH, fp)))
			{
				//externalStr = fgets(line, MAX_ONE_LINE_LENGTH, fp);
				if (NULL == externalStr)
				{
					if (0 == feof(fp))
					{
						continue;
					}
					break;
				}
				if (('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0]))
				{
					continue;
				}
				externalName = strtok(externalStr, " \t\r\n");
				if(NULL == externalName)
				{
					addInfoLog(2, "fcexternal.conf file external_dns error");
				}
				else
				{
					if(strcasecmp("external_dns", externalName))
					{
						continue;
					}
					externalIP = strtok(NULL, " \t\r\n");
					if(NULL == externalIP)
					{
						addInfoLog(2, "fcexternal.conf file external_dns ip error");
						//return -1;
					}
					else
					{
						ret = checkOneIp(externalIP);
						if (-1 == ret)
						{
							addInfoLog(2, "fcexteranl.conf file error");
							//return -1;
						}
						else
						{
							ret = checkRepeatDnsIP(tempIp, externalIP, number - 1);
							if(1 == ret)
							{
								addInfoLog(1, "external IP repeat");
								//return -2;
							}
							else
							{
								if (number < MAX_IP_ONE_LINE)
								{
									number++;
									strcpy(tempIp[number - 1], externalIP);
								}
								else
								{
									break;
								}
							}
						}
					}
				}
			}
			fclose(fp);
		}
	}

	*ip = (struct IpDnsDetect *)malloc((number + 1) * sizeof(struct IpDnsDetect));
	if (NULL == *ip)
	{
		return -3;
	}
	memset(*ip, 0, (number + 1) * sizeof(struct IpDnsDetect));
	struct IpDnsDetect* pstIp = *ip;
	for (i = 0; i < number; ++i)
	{
		strcpy(pstIp[i].ip, tempIp[i]);
	}

	return number;
}

int processOneLine(char* line, struct origDomain* pstOrigDomain, int count, int check_flag)
{
	char info[40960] = {0};
	char *errstr = NULL;
	char *p = NULL;
	char backup[8192] = {0};
	memcpy(backup, line, strlen(line));

	if ((p = strchr(backup, '\r')))
		*p = '\0';
	if ((p = strchr (backup, '\n')))
		*p = '\0';

	int ret = 0;
	char *saveptr = NULL;
	char *str = strtok_r(line, " \t", &saveptr);

	if (NULL == str)
	{
		if (check_flag)
		{
			errstr = "<domain> is NULL";
			snprintf(info, 40960, "[%d] [%s] [%s]\n", count, backup, errstr);
			write_rcms_log(DIGRUN_RCMS_LOG, info);
			fprintf(stderr,"'%s' ERROR\n", backup);
		}
		return FIELD_NUMBER_ERROR;
	}
	strcpy(pstOrigDomain->host, str);

	str = strtok_r(NULL, " \t", &saveptr);
	if (NULL == str)
	{
		if (check_flag)
		{
			errstr = "<origDomain> is NULL";
			snprintf(info, 40960, "[%d] [%s] [%s]\n", count, backup, errstr);
			write_rcms_log(DIGRUN_RCMS_LOG, info);
			fprintf(stderr,"'%s' ERROR\n", backup);
		}
		return FIELD_NUMBER_ERROR;
	}
	strcpy(pstOrigDomain->origDomain, str);
	ret = checkRepeatOrigDomain(pstOrigDomain->host,pstOrigDomain->origDomain);
	if (1 == ret)
	{
		addInfoLog(2,"origDomain repeat\n");
		if (check_flag)
		{
			errstr = "<origDomain> is repeat";
			snprintf(info, 40960, "[%d] [%s] [%s]\n", count, backup, errstr);
			write_rcms_log(DIGRUN_RCMS_LOG, info);
			fprintf(stderr,"'%s' ERROR\n", backup);
		}
		return HOST_REPEAT_ERROR;
	}

	str = strtok_r(NULL, " \t", &saveptr);
	if (NULL == str)
	{
		if (check_flag)
		{
			errstr = "<useDefault> is NULL";
			snprintf(info, 40960, "[%d] [%s] [%s]\n", count, backup, errstr);
			write_rcms_log(DIGRUN_RCMS_LOG, info);
			fprintf(stderr,"'%s' ERROR\n", backup);
		}
		return FIELD_NUMBER_ERROR;
	}
	if ('y' == str[0] || 'Y' == str[0])
	{
		pstOrigDomain->useDefault = 1;
	}
	else
	{
		pstOrigDomain->useDefault = 0;
	}

	str = strtok_r(NULL, " \t\r\n", &saveptr);
	if (NULL == str)
	{
		if (check_flag)
		{
			errstr = "<ips> is NULL";
			snprintf(info, 40960, "[%d] [%s] [%s]\n", count, backup, errstr);
			write_rcms_log(DIGRUN_RCMS_LOG, info);
			fprintf(stderr,"'%s' ERROR\n", backup);
		}
		return FIELD_NUMBER_ERROR;
	}
	ret = getDnsIP(str, &(pstOrigDomain->ip), pstOrigDomain->useDefault);
	pstOrigDomain->ipNum = ret;
	if (0 == ret)
	{
		addInfoLog(2, "origdomain.conf, config line has't any ip");
		if (check_flag)
		{
			errstr = "origdomain.conf, config line has't any ip";
			snprintf(info, 40960, "[%d] [%s] [%s]\n", count, backup, errstr);
			write_rcms_log(DIGRUN_RCMS_LOG, info);
			fprintf(stderr,"'%s' ERROR\n", backup);
		}
		return 0;
	}
	else if (-1 == ret)
	{
		if (check_flag)
		{
			errstr = "dns ip is error1";
			snprintf(info, 40960, "[%d] [%s] [%s]\n", count, backup, errstr);
			write_rcms_log(DIGRUN_RCMS_LOG, info);
			fprintf(stderr,"'%s' ERROR\n", backup);
		}
		return IP_ERROR;
	}
	else if (-2 == ret)
	{
		if (check_flag)
		{
			errstr = "dns ip is error2";
			snprintf(info, 40960, "[%d] [%s] [%s]\n", count, backup, errstr);
			write_rcms_log(DIGRUN_RCMS_LOG, info);
			fprintf(stderr,"'%s' ERROR\n", backup);
		}
		return IP_ERROR;
	}
	else if (-3 == ret)
	{
		if (check_flag)
		{
			errstr = "dns ip is error3";
			snprintf(info, 40960, "[%d] [%s] [%s]\n", count, backup, errstr);
			write_rcms_log(DIGRUN_RCMS_LOG, info);
			fprintf(stderr,"'%s' ERROR\n", backup);
		}
		return MEMORY_ERROR;
	}
	str = strtok_r(NULL, " \t\r\n", &saveptr);
	if (NULL != str)
	{
		pstOrigDomain->travelDns = !strcmp(str, "Y");
	}

	str = strtok_r(NULL, " \t\r\n", &saveptr);
	if (NULL != str)
	{
		pstOrigDomain->ignoreFailTimes = !strcmp(str, "Y");
	}
	return ret;
}

int processOneLineHistory(char *line, struct origDomainHistory *pstOrigDomainHistory)
{
	int ret = 0;
	char* str = NULL;
	char *saveptr = NULL;

	/* parse Host */
	str = strtok_r(line, " \t", &saveptr);
	if (NULL == str)
	{
		return FIELD_NUMBER_ERROR;
	}
	strcpy(pstOrigDomainHistory->host, str);

	/* parse origin Host */
	str = strtok_r(NULL, " \t", &saveptr);
	if (NULL == str)
	{
		return FIELD_NUMBER_ERROR;
	}
	strcpy(pstOrigDomainHistory->origDomain, str);

	/* parse ip */
	str = strtok_r(NULL, " \t", &saveptr);
	if (NULL == str)
	{
		return FIELD_NUMBER_ERROR;
	}
	ret = getIp(str, &(pstOrigDomainHistory->ip), &pstOrigDomainHistory->ipNum);
	//pstOrigDomainHistory->ipNum = ret;
	if (pstOrigDomainHistory->ipNum <= 0)
	{
		addInfoLog(2, "Open origin_domain_ip file error");
		return -1;
	}

	/* parse dns query failed times */
	str = strtok_r(NULL, " \t\r\n", &saveptr);
	if(NULL == str)
	{
		return FIELD_NUMBER_ERROR;
	}
	ret = atoi(str);
	if(ret <= 0)
	{
		pstOrigDomainHistory->failtimes = 0;
	}
	else
	{
		pstOrigDomainHistory->failtimes = ret;
	}

	return 0;
}

int getOrigDomainHistory(FILE *fp)
{
	int i;
	int ret;
	int flag = 0;
	char *str = NULL;
	char line[MAX_ONE_LINE_LENGTH];
	char line2[MAX_ONE_LINE_LENGTH];

	for (i = 1, g_iOrigDomainHistoryItem = 0; g_iOrigDomainHistoryItem < MAX_HOST_NUMBER; ++i)
	{
		str = fgets(line, MAX_ONE_LINE_LENGTH, fp);
		if (NULL == str)
		{
			if (0 == feof(fp))
			{
				continue;
			}
			break;
		}
		/* ignore invalid line */
		if (('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0]))
		{
			continue;
		}
		g_pstOrigDomainHistory[g_iOrigDomainHistoryItem] = (struct origDomainHistory *)malloc(sizeof(struct origDomainHistory));
		if (NULL == g_pstOrigDomainHistory[g_iOrigDomainHistoryItem])
		{
			return -1;
		}
		memset(g_pstOrigDomainHistory[g_iOrigDomainHistoryItem], 0, sizeof(struct origDomainHistory));

		strcpy(line2, line);
		ret = processOneLineHistory(line, g_pstOrigDomainHistory[g_iOrigDomainHistoryItem]);
		if (ret < 0)
		{
			char buf[1024];

			flag = -1;
			// add warning log here
			snprintf(buf, sizeof(buf),
					"conf_error [%s] in line=[%d] line=[%.*s]",
					g_pszErrorType[-ret-1], i, (int32_t)(strlen(line2)-1), line2);
			debug(1, "conf_parse there may be  %s\n", buf);
			continue;
		}
		g_iOrigDomainHistoryItem++;
	}

	return flag;
}

int getOrigDomainConf(FILE* fp, int check_flag)
{
	char line[MAX_ONE_LINE_LENGTH];
	char line2[MAX_ONE_LINE_LENGTH];
	char* str;
	g_iOrigDomainItem = 0;
	int ret;
	int flag = 0;
	int i;
	int count = 0;
	for (i=1;g_iOrigDomainItem<MAX_HOST_NUMBER;i++)
	{
		str = fgets(line, MAX_ONE_LINE_LENGTH, fp);
		if (NULL == str)
		{
			if (!feof(fp))
			{
				continue;
			}
            break;
		}
		count += 1;
		if (('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0]))
		{
			continue;
		}
		g_pstOrigDomain[g_iOrigDomainItem] = (struct origDomain *)malloc(sizeof(struct origDomain));
		if (NULL == g_pstOrigDomain[g_iOrigDomainItem])
		{
			return -1;
		}
		memset(g_pstOrigDomain[g_iOrigDomainItem], 0, sizeof(struct origDomain));

		strcpy(line2, line);
		ret = processOneLine(line, g_pstOrigDomain[g_iOrigDomainItem], count, check_flag);
		if (ret < 0)
		{
			flag = -1;
			char errorInfo[1024];
			// add warning log here
			snprintf(errorInfo, sizeof(errorInfo),
					"[%s] in line=[%d] line=[%.*s]",
					g_pszErrorType[-ret-1], i, (int32_t)(strlen(line2)-1), line2);
			addInfoLog(2, errorInfo);
			memset(line, 0, MAX_ONE_LINE_LENGTH);
			continue;
		}
		g_iOrigDomainItem++;
	}

	if(0 == g_iOrigDomainItem)
	{
		flag = -1;
	}
	else
	{
		flag = 0;
	}

	return flag;
}
//--------------------------------------------------------------------------

//added by chinacache zenglj
void processDnsDetectResult(void *task, int fd)
{

	assert(NULL != task);

	int ret;
	int count = 0;
	int offset = 0;
	char ip[64];
	char ipBuf[2048];
	char buffer[MAX_ONE_LINE_LENGTH];
	struct origDomain *pstOrigDomain = (struct origDomain *)task;

	memset(buffer, 0, sizeof(buffer));
	pstOrigDomain->working = 0;

	while (offset < MAX_ONE_LINE_LENGTH)
	{
		ret = read(fd, buffer+offset, MAX_ONE_LINE_LENGTH - offset);
		if (-1 == ret)
		{
			if (EINTR == errno)
			{
				continue;
			}
		}
		offset += ret;
		if ('\n' == buffer[offset-1])
		{
			break;
		}
	}
	if (0 == offset || 1 == offset )
	{
		addInfoLog(2, "Can not get dig detect result");
		return;
	}
	debug(2, "dig process result buf:[%s]\n", buffer);
	pstOrigDomain->usedtime = atof(buffer);
	char* result = strchr(buffer, ' ');

	assert(result != NULL);
	result = result + 1;

	if (!strcmp("timeout\n", result))
	{
		if (pstOrigDomain->travelDns)
		{
			snprintf(ipBuf, sizeof(ipBuf), "%s", pstOrigDomain->ip[pstOrigDomain->usedIP - 1].ip);
			addAlertLog(3, pstOrigDomain->host, ipBuf);
		}
		else
		{
			if (pstOrigDomain->usedIP == pstOrigDomain->ipNum)
			{
				int i;
				int failtimes = 0;

				memset(ipBuf, 0, sizeof(ipBuf));

				for (i = 0; i < pstOrigDomain->ipNum; i++)
				{
					if(i == pstOrigDomain->ipNum - 1)
						snprintf(ip, sizeof(ip), "%s", pstOrigDomain->ip[i].ip);
					else
						snprintf(ip, sizeof(ip), "%s:", pstOrigDomain->ip[i].ip);
					strncat(ipBuf, ip, sizeof(ip));
				}
				addAlertLog(3, pstOrigDomain->host, ipBuf);
				for(i = 0; i < g_iOrigDomainHistoryItem; i++)
				{
					if(!strcmp(pstOrigDomain->host, g_pstOrigDomainHistory[i]->host))
					{
						g_pstOrigDomainHistory[i]->failtimes++;
						failtimes = g_pstOrigDomainHistory[i]->failtimes;
						break;
					}
				}
			}
		}
	}
	else
	{
		//add to history file
		int i;
		int founded = 0;

		if(0 == pstOrigDomain->usedIP - 1)
		{
			addAlertLog(1, pstOrigDomain->host, ipBuf);
		}
		else
		{
			if (0 == pstOrigDomain->travelDns)
			{
				for(i = 0; i < pstOrigDomain->usedIP - 1; i++)
				{
					if(i == pstOrigDomain->usedIP - 2)
						snprintf(ip, sizeof(ip), "%s", pstOrigDomain->ip[i].ip);
					else
						snprintf(ip, sizeof(ip), "%s:", pstOrigDomain->ip[i].ip);
					strncat(ipBuf, ip, sizeof(ip));
				}
				addAlertLog(2, pstOrigDomain->host, ipBuf);
			}
		}
		pstOrigDomain->success = 1; //detect has successed
		//pstOrigDomain->usedIP = 0;
		for (i = 0; i < g_iOrigDomainHistoryItem; i++)
		{
			if (!strcmp(g_pstOrigDomainHistory[i]->host, pstOrigDomain->host) &&
					(!strcmp(g_pstOrigDomainHistory[i]->origDomain, pstOrigDomain->origDomain)))
			{
				founded = 1;
				/* make a copy start*/ 
				int ip_number_privious = g_pstOrigDomainHistory[i]->ipNum; 
				int failtimes_previous = g_pstOrigDomainHistory[i]->failtimes;
				struct IpDetect *pointer_ip = NULL; 
				pointer_ip = (struct IpDetect*)malloc((ip_number_privious)*sizeof(struct IpDetect)); 
				if (NULL == pointer_ip) 
				{    
					addInfoLog(2,"malloc error"); 
					return ; 
				}    
				memset(pointer_ip, 0, (ip_number_privious)*sizeof(struct IpDetect)); 
				for (count = 0;count<ip_number_privious;count++) 
				{    
					strcpy(pointer_ip[count].ip, g_pstOrigDomainHistory[i]->ip[count].ip); 
				}    
				/* make a copy end */
				//ret = getHistoryIP(result, &(g_pstOrigDomainHistory[i]->ip));

				if (0 == g_pstOrigDomainHistory[i]->flag)
				{
					free(g_pstOrigDomainHistory[i]->ip);
					g_pstOrigDomainHistory[i]->ip = NULL;
					g_pstOrigDomainHistory[i]->ipNum = 0;
				}

				ret = getIp(result, &(g_pstOrigDomainHistory[i]->ip), &g_pstOrigDomainHistory[i]->ipNum);
				g_pstOrigDomainHistory[i]->flag = 1;
				//g_pstOrigDomainHistory[i]->ipNum = ret;

				if (ret < 1)
				{
					founded = 1;
					g_pstOrigDomainHistory[i]->ip = pointer_ip;
					g_pstOrigDomainHistory[i]->failtimes=failtimes_previous;
					g_pstOrigDomainHistory[i]->failtimes++;
					g_pstOrigDomainHistory[i]->ipNum = ip_number_privious;

					for (count = 0;count<ip_number_privious;count++)
					{
						strcpy(g_pstOrigDomainHistory[i]->ip[count].ip,pointer_ip[count].ip);
					}
					char message[128];
					snprintf(message,128,"%s dig invalid ip,we use the origin ip",g_pstOrigDomainHistory[i]->host);
					addInfoLog(2,message);

				}
				else
				{
					g_pstOrigDomainHistory[i]->failtimes = 0;
				}
				break;
			}
		}
		debug(1,"%s: founded =%d\n",pstOrigDomain->host,founded);
		char result2[2048];
		strcpy(result2, result);
		//新增一行在历史文件origin_domain_ip中
		if (!founded)
		{
			g_iOrigDomainHistoryItem++;
			g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1] = (struct origDomainHistory*)malloc(sizeof(struct origDomainHistory));
			memset(g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1], 0, sizeof(struct origDomainHistory));
			strcpy(g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1]->host, pstOrigDomain->host);
			strcpy(g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1]->origDomain, pstOrigDomain->origDomain);
			ret = getIp(result, &(g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1]->ip), &g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1]->ipNum);
			//g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1]->ipNum = ret;
			if (ret < 1)
			{
				//this is when the first time ,the digDetect return bad ip
				char domainFileName[128];
				int number_of_record = 0;
				int count_domain_conf = 0;
				char line[MAX_ONE_LINE_LENGTH];
				char line2[MAX_ONE_LINE_LENGTH];
				char host_in_domain[MAX_HOST_NUMBER];
				char *str;
				char ip[MAX_IP_ONE_LINE];
				memset(domainFileName,0,sizeof( domainFileName ));
				strcpy(domainFileName, "/usr/local/squid/etc/domain.conf");

				FILE *fp = fopen(domainFileName, "r");
				if (NULL == fp) {
					addInfoLog(2, "In processDnsDetectResult ,cannot open domain.conf");
					return ;
				}
				for (count_domain_conf=1;number_of_record<MAX_HOST_NUMBER;count_domain_conf++) {
					str = fgets(line, MAX_ONE_LINE_LENGTH, fp);
					if (NULL == str) {
						if (0 == feof(fp))
							continue;
						break;
					}
					strcpy(line2,line);

					//注释和空行丢掉
					if (('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0]))
						continue;

					char* str = strtok(line2, " \t");
					if (NULL == str)
					{
						addInfoLog(2, "In processDnsDetectResult,domain.conf format error,check it!");
						return ;
					}
					strcpy(host_in_domain, str);
					int count_for_ip = 0;
					for(;count_for_ip <12;count_for_ip++)//取第十三列
					{
						str = strtok(NULL, " \t");
						if (NULL == str)
						{
							addInfoLog(2, "In processDnsDetectResult,domain.conf format error,check it!");
							return;
						}
					}
					strcpy(ip,str);

					if(!strncmp(host_in_domain,pstOrigDomain->host,strlen(pstOrigDomain->host)))
					{
						ret = getIp(ip, &(g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1]->ip), &g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1]->ipNum);
						//g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1]->ipNum = ret;
						g_pstOrigDomainHistory[g_iOrigDomainHistoryItem - 1]->failtimes ++;

						break;
					}
					number_of_record++;
				}
				fclose(fp);

			}//end (if ret<1)
		}

	}
}

int detect(int processNumber1, char* processName1, double timer1)
{
	char seconds[16];
	sprintf(seconds, "%f", timer1);
	char* const argv[] = {processName1, seconds, NULL};
	struct ProcessInfo* ppstProcessInfo;
	ppstProcessInfo = initProcess(processNumber1, argv, processDnsDetectResult);
	if (NULL == ppstProcessInfo)
	{
		addInfoLog(2, "Can not create ppstProcessInfo[0]");
		return -1;
	}
	if (0 == g_iOrigDomainItem)
	{
		return -1;
	}
	int maxFd = getMaxFileno(ppstProcessInfo);
	int ret;
	fd_set rset;
	FD_ZERO(&rset);

	while (1)
	{
		//distribute dig detect
		int digLineNum = 0;
		int dnsfinish = 1;
		int iDigProcessFree = 0;

		while((iDigProcessFree = getFreeProcessNumber(ppstProcessInfo)) > 0)
		{
			char command[256];

			if(0 == g_pstOrigDomain[digLineNum]->ipNum)
			{
				g_pstOrigDomain[digLineNum]->success = -1;
			}


			if ((g_pstOrigDomain[digLineNum]->travelDns ||
						0 == g_pstOrigDomain[digLineNum]->success) &&
					g_pstOrigDomain[digLineNum]->usedIP < g_pstOrigDomain[digLineNum]->ipNum)
			{


				dnsfinish = 0;
				sprintf(command, "%s %s\n",
						g_pstOrigDomain[digLineNum]->origDomain,
						g_pstOrigDomain[digLineNum]->ip[g_pstOrigDomain[digLineNum]->usedIP].ip);


				if(0 == g_pstOrigDomain[digLineNum]->working)
				{
					g_pstOrigDomain[digLineNum]->usedIP++;
					g_pstOrigDomain[digLineNum]->working = 1;
					if(g_pstOrigDomain[digLineNum]->usedIP == g_pstOrigDomain[digLineNum]->ipNum)
					{
						g_pstOrigDomain[digLineNum]->success = -1;
					}
					debug(2, "digRun send command:[%s]\n", command);
					distributeOneCommand(command, ppstProcessInfo, g_pstOrigDomain[digLineNum], &rset);
				}
			}
			digLineNum++;
			if (digLineNum >= g_iOrigDomainItem)
			{
				break;
			}
		}

		if (0 == dnsfinish)
		{
			ret = waitFreeProcess(&rset, maxFd, NULL, &ppstProcessInfo, 1);
		}
		if (1 == dnsfinish)
		{
			break;
		}
	}//while(1)
	waitAllFreeProcess(&rset, maxFd, &ppstProcessInfo, 1);
	killAllProcess(&ppstProcessInfo, 1);

	return 1;
}

int writeOrigDomainIP(void)
{
	int i;
	char host[1024];
	char origDomain[1024];
	char origDomainIPHistoryFileName[128];
	char tempFile[128];
	char ip[64];
	char iplist[1024];
	char failtimes[1024];
	char line[2048];
	char null_ip[64]="255.255.255.255";

	memset(origDomainIPHistoryFileName, 0, sizeof(origDomainIPHistoryFileName));
	strcpy(origDomainIPHistoryFileName,"/usr/local/squid/etc/origin_domain_ip");
	memset(tempFile, 0, sizeof(tempFile));
	strcpy(tempFile, "/usr/local/squid/etc/origin_domain_ip.tmp");

	//FILE* fp = fopen(origDomainIPHistoryFileName, "w+");
	FILE* fp = fopen(tempFile, "w+");
	if(NULL == fp)
	{
		return -1;
	}

	for(i = 0; i < g_iOrigDomainHistoryItem; i++)
	{
		int j;

		for(j = 0; j < g_iOrigDomainItem; j++)
		{
			if(!strcmp(g_pstOrigDomainHistory[i]->host, g_pstOrigDomain[j]->host) &&
					!strcmp(g_pstOrigDomainHistory[i]->origDomain, g_pstOrigDomain[j]->origDomain))
			{
				snprintf(host, sizeof(host), "%s", g_pstOrigDomainHistory[i]->host);
				snprintf(origDomain, sizeof(origDomain), "%s", g_pstOrigDomainHistory[i]->origDomain);
				if( g_pstOrigDomain[j]->ignoreFailTimes )
					snprintf(failtimes, sizeof(failtimes), "0");
				else
					snprintf(failtimes, sizeof(failtimes), "%d", g_pstOrigDomainHistory[i]->failtimes);
				int k;
				memset(iplist, 0, sizeof(iplist));
				for(k = 0; k < g_pstOrigDomainHistory[i]->ipNum; k++)
				{
					if(k == g_pstOrigDomainHistory[i]->ipNum - 1)
						snprintf(ip, sizeof(ip), "%s", g_pstOrigDomainHistory[i]->ip[k].ip);
					else
						snprintf(ip, sizeof(ip), "%s:", g_pstOrigDomainHistory[i]->ip[k].ip);
					strncat(iplist, ip, sizeof(ip));
				}
				if (0 == iplist[0] )
				{
					strcpy(iplist,null_ip);
				}
				snprintf(line, sizeof(line), "%s %s %s %s", host, origDomain, iplist, failtimes);
				fprintf(fp, "%s\n", line);
			}
		}
	}

	fclose(fp);

	if(-1 == rename(tempFile, origDomainIPHistoryFileName))
	{
		addInfoLog(3, "copy temp file to origin_domain_ip file error");
		return -1;
	}

	return 1;
}

static void str_rtrim(char* str, const char* tokens)
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

int addResultLog(void)
{
	char resultLogFileName[128];
	memset(resultLogFileName, 0, sizeof(resultLogFileName));
	strcpy(resultLogFileName, "/var/log/chinacache/digRun.log");
	FILE* resultFile = fopen(resultLogFileName, "a+");
	if(NULL == resultFile)
	{
		return -1;
	}

	fprintf(resultFile, "Dig Detect Log Begin----------------------------\n");
	time_t recTime;
	recTime = time(NULL);
	char detectTime[1024];
	snprintf(detectTime, sizeof(detectTime), "%s",ctime(&recTime));
	fprintf(resultFile, "%s", detectTime);
	int i;
	for(i = 0; i< g_iOrigDomainItem; i++)
	{
		char buf[BUFSIZ];
		char methond[1024];
		char host[1024];
		char origDomain[1024];
		char dnsIP[1024];
		char ip[17];
		char iplist[1024];
		char usedtime[16];
		//        if(0 == g_pstOrigDomain[i]->needDetect)
		//        {
		int j;
		for(j = 0; j < g_iOrigDomainHistoryItem; j++)
		{
			if(!strcmp(g_pstOrigDomainHistory[j]->host, g_pstOrigDomain[i]->host)&&
					!strcmp(g_pstOrigDomainHistory[j]->origDomain, g_pstOrigDomain[i]->origDomain))
			{
				int k;

				memset(dnsIP, 0, 1024);
				memset(iplist, 0, 1024);

				snprintf(methond, sizeof(methond), "%s", "dig");
				snprintf(host, sizeof(host), "%s", g_pstOrigDomain[i]->host);
				snprintf(origDomain, sizeof(origDomain), "%s", g_pstOrigDomain[i]->origDomain);
				if (g_pstOrigDomain[i]->travelDns)
				{
					for (k = 0; k < g_pstOrigDomain[i]->ipNum; ++k)
					{
						if (k == g_pstOrigDomain[i]->ipNum - 1)
						{
							snprintf(ip, sizeof(ip), "%s", g_pstOrigDomain[i]->ip[k].ip);
						}
						else
						{
							snprintf(ip, sizeof(ip), "%s:", g_pstOrigDomain[i]->ip[k].ip);
						}
						strncat(dnsIP, ip, sizeof(ip));
					}
				}
				else
				{
					snprintf(dnsIP, sizeof(dnsIP), "%s", g_pstOrigDomain[i]->ip[g_pstOrigDomain[i]->usedIP - 1].ip);
				}
				snprintf(usedtime, sizeof(usedtime), "%f", g_pstOrigDomain[i]->usedtime);
				for(k = 0; k < g_pstOrigDomainHistory[j]->ipNum; k++)
				{
					if(k == g_pstOrigDomainHistory[j]->ipNum - 1)
						snprintf(ip, sizeof(ip), "%s", g_pstOrigDomainHistory[j]->ip[k].ip);
					else
						snprintf(ip, sizeof(ip), "%s:", g_pstOrigDomainHistory[j]->ip[k].ip);
					strncat(iplist, ip, sizeof(ip));
				}
				snprintf(buf, sizeof(buf), "%s %s %s %s %s %s\n", methond, host, dnsIP,origDomain, iplist, usedtime);
				fprintf(resultFile, buf);
			} //if
		} //for(j = 0...
		//        } //if( 1 = ...
	} //for (i = 0...

	fprintf(resultFile, "Dig Detect Log End------------------------------\n");
	fclose(resultFile);

	return 1;
}

int addInfoLog(int type, const char* str)
{
	char infoLogFileName[128];
	memset(infoLogFileName, 0, sizeof(infoLogFileName));
	strcpy(infoLogFileName, "/var/log/chinacache/digRun_info.log");

	FILE* fp = fopen(infoLogFileName, "a+");
	if(NULL == fp)
	{
		return -1;
	}

	time_t t;
	t = time(NULL);
	char timeStr[64];
	snprintf(timeStr, sizeof(timeStr), "%s", ctime(&t));
	str_rtrim(timeStr, "\n");

	if(1 == type)
	{
		fprintf(fp, "Warning[%s]:%s\n", timeStr, str);
	}
	else
		if(2 == type)
		{
			fprintf(fp, "Error[%s]:%s\n", timeStr, str);
		}
		else
			if(3 == type)
			{
				fprintf(fp, "Fatal[%s]:%s\n", timeStr, str);
			}

	fclose(fp);

	return 1;
}

int addAlertLog(int type, char* host, char* ipBuf)
{
	char alertLogFileName[128];
	memset(alertLogFileName, 0, sizeof(alertLogFileName));
	strcpy(alertLogFileName, "/usr/local/squid/var/dig_alert.dat");
	char tempFileName[128];
	memset(tempFileName, 0, sizeof(tempFileName));
	strcpy(tempFileName, "/usr/local/squid/var/dig_alert.dat.tmp");

	FILE* fp = fopen(tempFileName, "a");
	if(NULL == fp)
	{
		return -1;
	}

	if(1 == type)
	{
		fprintf(fp, "%s pass\n", host);
	}
	if(2 == type)
	{
		fprintf(fp, "%s error %s", host, ipBuf);
		fprintf(fp, "\n");
	}
	if(3 == type)
	{
		fprintf(fp, "%s fail %s", host, ipBuf);
		fprintf(fp, "\n");
	}

	fclose(fp);

	return 1;
}
//--------------------------------------------------------------------------

#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <glob.h>
#include "detect_orig.h"

enum {
    CONTROLS = 0,
    IP,
    OPTIONS,
    NONE
};

const int FIELD_NUMBER_ERROR = -1;
const int HOST_REPEAT_ERROR = -2;
const int METHOD_ERROR = -3;
const int MEMORY_ERROR = -4;
const int IP_ERROR = -5;
const int DETECTWEIGHT_ERROR = -6;

const char* g_pszErrorType[] = {"field_number", "host_repeat", "method_error",
    "memory_error", "ip"};
#define  FILE_MODE       (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

void freeOneOrigDomain(struct origDomain* pstOrigDomain)
{
    if(NULL != pstOrigDomain->ip)
    {
        cc_free(pstOrigDomain->ip);
        pstOrigDomain->ip = NULL;

    }

    if(NULL != pstOrigDomain)
    {
        cc_free(pstOrigDomain);
        pstOrigDomain = NULL;
    }
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void freeOneOrigDomainHistory(struct origDomainHistory* pstOrigDomainHistory)
{
    if(NULL != pstOrigDomainHistory->ip)
    {
        cc_free(pstOrigDomainHistory->ip);
        pstOrigDomainHistory->ip = NULL;
    }

    if(NULL != pstOrigDomainHistory)
    {
        cc_free(pstOrigDomainHistory);
        pstOrigDomainHistory = NULL;
    }
}

//-------------------------------------------------------------------------------

/*释放一个主机
*/
void freeOneDetectOrigin(struct DetectOrigin* pstDetectOrigin)
{
    if (NULL == pstDetectOrigin)
        return;

    if (NULL != pstDetectOrigin->code)
    {
        cc_free(pstDetectOrigin->code);
        pstDetectOrigin->code = NULL;
    }
    if (NULL != pstDetectOrigin->ip)
    {
        cc_free(pstDetectOrigin->ip);
        pstDetectOrigin->ip = NULL;
    }
    if (NULL != pstDetectOrigin->bkip)
    {
        cc_free(pstDetectOrigin->bkip);
        pstDetectOrigin->ip = NULL;
    }
#ifdef ROUTE
    if( pstDetectOrigin->final_ip_number != 0 )
    {
        free( pstDetectOrigin->final_ip );
        free( pstDetectOrigin->final_ip_port );
    }
#endif /* ROUTE */

    cc_free(pstDetectOrigin);
    pstDetectOrigin = NULL;
}

/*add by xin.yao: 2011-11-14 */
void g_detect_tasks_free(void)
{
    if( g_detect_tasks_index )
    {
        HashDestroy( g_detect_tasks_index );
        free( g_detect_tasks_index );
        g_detect_tasks_index = NULL;
    }
    if (NULL != g_detect_tasks) {
        cc_free(g_detect_tasks);
        g_detect_tasks = NULL;
    }
}

void global_mem_free(void)
{
    g_detect_tasks_free();
    HashDestroy( g_detect_domain_table );
}

void one_mem_free(void)
{
    int i;

    for(i = 0; i < g_detect_task_count; i++) {
        freeOneDetectOrigin(g_detect_tasks[i]);
    }
}

/* end by xin.yao:2011-11-14 */

void free_rfc_detectOrigin(struct DetectOrigin* pstDetectOrigin)
{
    if (NULL != pstDetectOrigin->code)
    {
        cc_free(pstDetectOrigin->code);
        pstDetectOrigin->code = NULL;
    }
    if (NULL != pstDetectOrigin->ip)
    {
        cc_free(pstDetectOrigin->ip);
        pstDetectOrigin->ip = NULL;
    }

}


int clean_detect_mem(void)
{
    one_mem_free();
    global_mem_free();

    return 0;
}

/* add for detect_cutom.conf by xin.yao: 2011-12-19 */
//void handleRepeatFromDomainConf(int task_index, detect_custom_t *detect_custom)
//{
//	int i;
//
//	for (i = 0; i < g_detect_tasks[task_index]->ipNumber; ++i) {
//		if (!strncmp(g_detect_tasks[task_index]->ip[i].ip, detect_custom->ip, 16)) {
//			//printf("ip1: %s, ip2: %s\n", g_detect_tasks[task_index]->ip[i].ip, detect_custom->ip);
//			if (-1 == detect_custom->reuse_task_index) {
//				detect_custom->reuse_ip_index = i;
//				detect_custom->reuse_task_index = task_index;
//				g_detect_tasks[task_index]->ip[i].reuse_nodetect_flag = 0;
//				return;
//			}
//			g_detect_tasks[task_index]->ip[i].reuse_nodetect_flag = 1;
//			g_detect_tasks[task_index]->ip[i].reuse_ip_index = detect_custom->reuse_ip_index;
//			g_detect_tasks[task_index]->ip[i].reuse_task_index = detect_custom->reuse_task_index;
//			return;
//		}
//	}
//}

/*检查这个主机是否已经有了（通过依次与数组里的内容做对比来判断）
 *输入：
 *    host----新的主机
 *返回值：
 *    1----已经有了
 *    0----没有
 */
//int checkRepeatHost(const char* host)
//{
//	int i;
//
//	for (i = 0; i < g_detect_task_count; ++i) {
//		if (!strcmp(host, g_detect_tasks[i]->host)) {
//			//printf("same host: %s\n", host);
//			return i;
//		}
//	}
//	return -1;
//}
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  DetectOriginIndexFill
 *  Description:
 * =====================================================================================
 */
    int
DetectOriginIndexFill( DetectOriginIndex *index, char *host )
{
    HashKey sum, xor;
    char *p, *pp;
    assert( index );
    assert( host );
    index->hostname = host;
    p = host;
    pp = host + 1;
    sum = xor = 0;
    for( ; *pp != '\0'; ++p, ++pp )
    {
        sum = (sum<<24) + (sum<<9) + sum + *p;
        xor = (xor<<1) ^ xor ^ *p;
    }
    if( *p != '.' )
    {
        sum = (sum<<24) + (sum<<9) + sum + *p;
        xor = (xor<<1) ^ xor ^ *p;
    }
    index->sum = sum;
    index->xor = xor;
    Debug( 50, "host %s hash value, sum %X, xor %X\n", host, sum, xor );
    return 0;
} /* - - - - - - - end of function DetectOriginIndexFill - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  SearchHost
 *  Description:  查找这个主机是否已经有了。
 *                NULL表示无
 *                否则直接返回重复的struct DetectOrigin
 * =====================================================================================
 */
    struct DetectOrigin*
SearchHost( char *host )
{
    DetectOriginIndex input_data_index;
    DetectOriginIndex *indexed;
    DetectOriginIndexFill( &input_data_index, host );
    indexed = (DetectOriginIndex*)HashSearch( g_detect_tasks_index, (void*)&input_data_index );
    if( indexed != NULL )
        return (struct DetectOrigin*)( (char*)indexed->hostname - (ptrdiff_t)((struct DetectOrigin*)0)->host );
    else
        return NULL;
} /* - - - - - - - end of function SearchHost - - - - - - - - - - - */

int checkRepeatAnyHost(char* host)
{
    //int i;
    DetectOriginIndex data;

    DetectOriginIndexFill( &data, host );
    if( HashSearch( g_detect_domain_table, &data ) != NULL )
        return 1;
    else
        return 0;

    //for (i = 0; i < g_detect_anyhost_count; ++i) {
    //    if (!strcmp(host, g_detect_domain[i].detect_domain)) {
    //        return 1;
    //    }
    //}


    /*
       if (!have_recent_added) {
       have_recent_added = 1;
       }
       */
    return 0;
}

/*得到请求方法，同时得到请求时用的主机和请求的文件
 *输入：
 *    method----配置文件包含请求方法的字段（格式：GET:hostname/ll.jpg, 方法后可以是:或者|，后面主机名和文件都是可选的，默认主机名同主机名字段，文件名为时间.jpg）
 *    host----主机名字段
 *输出：
 *    rHost----请求时的主机名
 *    filename----请求的文件名
 *返回值：
 *    -1----失败
 *    整数----请求类型（1----get，3----head）
 */
int getMethod(const char* method, char* host, char* rHost, char* filename)
{
    int request;
    if ((0==strcasecmp(method, "yes")) || (0==strcasecmp(method, "y")) ||
            (0==strcasecmp(method, "no")) || (0==strcasecmp(method, "n")))  //默认为get请求
    {
        request = 1;
        strcpy(rHost, host);
        time_t t = time(NULL);
        snprintf(filename, 128, "/%u.jpg", (unsigned int)t);
    }
    else
    {
        //请求方法后面跟:或者|，如果后面没有东西，也可以什么都没有
        const char* str = method;
        const char* str2 = strchr(str, ':');
        if (NULL == str2)
            str2 = strchr(str, '|');
        if (NULL == str2)
            str2 = str+strlen(str);
        //目前请求只有get和head
        if ((3==str2-str) && (0==strncasecmp(str, "get", 3)))
            request = 1;
        else if ((4==str2-str) && (0==strncasecmp(str, "head", 3)))
            request = 3;
        else
            return -1;
        if (0 == *str2) {
            //只有请求方法
            strcpy(rHost, host);
            time_t t = time(NULL);
            snprintf(filename, 128, "/%u.jpg", (unsigned int)t);
        } else {
            str2++;
            if (0 == *str2)    //只有请求方法
            {
                strcpy(rHost, host);
                time_t t = time(NULL);
                snprintf(filename, 128, "/%u.jpg", (unsigned int)t);
            }
            else if ('/' == *str2) //没有主机名，只有文件名
            {
                strcpy(rHost, host);
                strcpy(filename, str2);
            }
            else  //有主机名
            {
                str = strchr(str2, '/');
                if (NULL == str)   //没有文件名
                {
                    strcpy(rHost, str2);
                    time_t t = time(NULL);
                    snprintf(filename, 128, "/%u.jpg", (unsigned int)t);
                }
                else  //有文件名
                {
                    sprintf(rHost, "%.*s", (int32_t)(str-str2), str2);
                    strcpy(filename, str);
                }
            }
        }
    }
    int ret = strlen(rHost);
    if ('.' == rHost[ret-1])
        rHost[ret-1] = 0;
    return request;
}


int is_ip_ever_have(char ip[],struct IpDetect* list,int num)
{
    int ret = 0;
    int i = 0;
    if (0 == num)
        ret = 0;
    else
        for(i=0;i<num;i++)
        {
            if(!strcmp(list[i].ip,ip))
                ret = 1;
        }

    return ret;
}

int checkRepeatHostOrigDomain(const char* host)
{
    int i;
    for(i = 0; i < g_iOrigDomainItem; i++) {
        if(!strcmp(host, g_pstOrigDomain[i]->host))
            return 1;
    }
    return 0;
}

int checkRepeatDnsIP(char string[][16], char* str, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        if(!strcmp(string[i], str))
            return 1;
    }
    return 0;
}

int getDnsIP(char* str, struct IpDnsDetect** ip, int useDefault)
{
    char* ptr;
    int none = 0;
    int ret;
    char tempIp[MAX_IP_ONE_LINE][16];
    int i = 0;
    if (!strcasecmp(str, "none"))
        none = 1;

    int number = 0;
    if(!none)
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
                return -2;
        }
        strcpy(tempIp[0], str);

        i = 1;
        for (i=1;i<MAX_IP_ONE_LINE;i++)
        {
            str = strtok_r(NULL, "|:", &ptr);
            if (NULL == str)
                break;
            ret = checkOneIp(str);
            if (-1 == ret)
                return -2;
            strcpy(tempIp[i], str);
        }
        number = i;
    }

    if (1 == useDefault)
    {
        char fcexternalFilename[128];
        memset(fcexternalFilename, 0, sizeof(fcexternalFilename));
        strcpy(fcexternalFilename, "/usr/local/squid/etc/fcexternal.conf");

        FILE* fp = fopen(fcexternalFilename, "r");
        if (NULL == fp)
            addInfoLog(2, "Open fcexternal.conf file error");
        else
        {
            char line[MAX_ONE_LINE_LENGTH];
            char* externalStr;
            char* externalIP;
            char* externalName;
            while(NULL != (externalStr = fgets(line, MAX_ONE_LINE_LENGTH, fp)))
            {
                if (NULL == externalStr)
                {
                    if (0 == feof(fp))
                        continue;
                    break;
                }
                if (('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0]))
                    continue;
                externalName = strtok(externalStr, " \t\r\n");
                if(NULL == externalName)
                    addInfoLog(2, "fcexternal.conf file external_dns error");
                else
                {
                    if(strcasecmp("external_dns", externalName))
                        continue;
                    externalIP = strtok(NULL, " \t\r\n");
                    if(NULL == externalIP)
                        addInfoLog(2, "fcexternal.conf file external_dns ip error");
                    else
                    {
                        ret = checkOneIp(externalIP);
                        if (-1 == ret)
                            addInfoLog(2, "fcexteranl.conf file external_dns ip error");
                        else
                        {
                            ret = checkRepeatDnsIP(tempIp, externalIP, number - 1);
                            if(1 == ret)
                                addInfoLog(2, "external IP repeat");
                            else
                            {
                                if (number < MAX_IP_ONE_LINE)
                                {
                                    number++;
                                    strcpy(tempIp[number - 1], externalIP);
                                }
                                else
                                    break;
                            }
                        }
                    }
                }
            }
            fclose(fp);
        }
    }

    *ip = (struct IpDnsDetect*)cc_malloc((number)*sizeof(struct IpDnsDetect));
    if (NULL == *ip)
        return -3;
    memset(*ip, 0, (number)*sizeof(struct IpDnsDetect));
    struct IpDnsDetect* pstIp = *ip;
    for (i=0; i<number; i++)
        strcpy(pstIp[i].ip, tempIp[i]);
    return number;
}

int processOneLine(char* line, struct origDomain* pstOrigDomain)
{
    char* str = strtok(line, " \t");
    int ret = 0;
    if (NULL == str)
        return FIELD_NUMBER_ERROR;
    strcpy(pstOrigDomain->host, str);
    if ('.' != str[strlen(str)-1])
        strcat(pstOrigDomain->host, ".");


    if( checkRepeatHostOrigDomain(pstOrigDomain->host) == 1 )
        return HOST_REPEAT_ERROR;

    str = strtok(NULL, " \t");
    if (NULL == str)
        return FIELD_NUMBER_ERROR;
    strcpy(pstOrigDomain->origDomain, str);
    str = strtok(NULL, " \t");
    if (NULL == str)
        return FIELD_NUMBER_ERROR;
    if ('y' == str[0] || 'Y' == str[0])
        pstOrigDomain->useDefault = 1;
    else
        pstOrigDomain->useDefault = 0;

    str = strtok(NULL, " \t\r\n");
    if (NULL == str)
        return FIELD_NUMBER_ERROR;
    ret = getDnsIP(str, &(pstOrigDomain->ip), pstOrigDomain->useDefault);
    pstOrigDomain->ipNum = ret;
    if (0 == ret)
    {
        addInfoLog(2, "origdomain.conf, config line has't any ip");
        return 0;
    }
    else if (-1 == ret)
        return IP_ERROR;
    else if (-2 == ret)
        return IP_ERROR;
    else if (-3 == ret)
        return MEMORY_ERROR;

    return ret;
}

int checkRepeatHostOrigDomainHistory(const char* host)
{
    int i;
    int count=1;
    for(i = 0; i < g_iOrigDomainHistoryItem; i++) {
        if(!strcmp(host, g_pstOrigDomainHistory[i]->host))
            count ++;
    }    
    return count;
}

int processOneLineHistory(char* line, struct origDomainHistory* pstOrigDomainHistory)
{
    char* str = strtok(line, " \t");
    int ret = 0;
    if (NULL == str)
        return FIELD_NUMBER_ERROR;
    strcpy(pstOrigDomainHistory->host, str);
    if ('.' != str[strlen(str)-1])
        strcat(pstOrigDomainHistory->host, ".");
    ret = checkRepeatHostOrigDomainHistory(pstOrigDomainHistory->host);
    {
        pstOrigDomainHistory->sequence = ret;
        cclog(1,"in history file: domain:%s  squence %d",pstOrigDomainHistory->host,pstOrigDomainHistory->sequence);
    }

    str = strtok(NULL, " \t");
    if (NULL == str)
        return FIELD_NUMBER_ERROR;
    strcpy(pstOrigDomainHistory->origDomain, str);

    str = strtok(NULL, " \t");
    if (NULL == str)
        return FIELD_NUMBER_ERROR;
    ret = getIp(str, &(pstOrigDomainHistory->ip));
    pstOrigDomainHistory->ipNum = ret;
    if (pstOrigDomainHistory->ipNum <= 0)
    {
        addInfoLog(2, "Open origin_domain_ip file error");
        return -1;
    }

    str = strtok(NULL, " \t\r\n");
    if(NULL == str)
        return FIELD_NUMBER_ERROR;

    ret = atoi(str);
    if(ret <= 0)
        pstOrigDomainHistory->failtimes = 0;
    else
        pstOrigDomainHistory->failtimes = ret;

    return 0;
}

int getOrigDomainHistory(FILE* fp)
{
    int i, ret; 
    char* str; 
    char line[MAX_ONE_LINE_LENGTH];
    char line2[MAX_ONE_LINE_LENGTH];

    i = 0; 
    g_iOrigDomainHistoryItem = 0; 

    while (++i) {
        memset(line, 0 , MAX_ONE_LINE_LENGTH);
        str = fgets(line, MAX_ONE_LINE_LENGTH, fp); 
        if (NULL == str) 
        {    
            if (0 == feof(fp))
                continue;
            break;
        }    

        if (('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0]))
            continue;
        if (g_iOrigDomainHistoryItem >= max_origDomainHistory_capacity) {
            max_origDomainHistory_capacity = g_iOrigDomainHistoryItem + INCREASE_STEP;
            g_pstOrigDomainHistory = realloc(g_pstOrigDomainHistory, max_origDomainHistory_capacity * sizeof(struct origDomainHistory *));
            if (NULL == g_pstOrigDomainHistory) {
                addInfoLog(2, "cannot realloc for g_pstOrigDomainHistory");
                return -1;
            }
        }
        g_pstOrigDomainHistory[g_iOrigDomainHistoryItem] = (struct origDomainHistory*)cc_malloc(sizeof(struct origDomainHistory));
        if (NULL == g_pstOrigDomainHistory[g_iOrigDomainHistoryItem])
            return -1;
        memset(g_pstOrigDomainHistory[g_iOrigDomainHistoryItem], 0, sizeof(struct origDomainHistory));

        strcpy(line2, line);
        ret = processOneLineHistory(line, g_pstOrigDomainHistory[g_iOrigDomainHistoryItem]);
        if (ret < 0) {
            char buf[1024];
            snprintf(buf, sizeof(buf), "In history file:conf_error [%s] in line=[%d] line=[%.*s]", g_pszErrorType[-ret-1], i, (int32_t)(strlen(line2)-1), line2);
            addInfoLog(2,buf);
            continue;
        }
        ++g_iOrigDomainHistoryItem;
    }

    return 0;
}

int getOrigDomainConf(FILE* fp)
{
    int i, ret;
    char *str;
    char line[MAX_ONE_LINE_LENGTH];
    char line2[MAX_ONE_LINE_LENGTH];

    i = 0;
    g_iOrigDomainItem = 0;

    while (++i)
    {
        memset(line, 0, MAX_ONE_LINE_LENGTH);
        str = fgets(line, MAX_ONE_LINE_LENGTH, fp);
        if (NULL == str)
        {
            if (0 == feof(fp))
                continue;
            break;
        }
        if (('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0]))
            continue;
        if (g_iOrigDomainItem >= max_origDomain_capacity)
        {
            max_origDomain_capacity = g_iOrigDomainItem + INCREASE_STEP;
            g_pstOrigDomain = realloc(g_pstOrigDomain, max_origDomain_capacity * sizeof(struct origDomain *));
            if (NULL == g_pstOrigDomain)
            {
                addInfoLog(2, "cannot realloc for g_pstOrigDomain");
                return -1;
            }
        }
        if( (g_pstOrigDomain[g_iOrigDomainItem] = (struct origDomain*)cc_malloc(sizeof(struct origDomain))) == NULL )
            return -1;
        memset(g_pstOrigDomain[g_iOrigDomainItem], 0, sizeof(struct origDomain));
        strcpy(line2, line);
        ret = processOneLine(line, g_pstOrigDomain[g_iOrigDomainItem]);
        if (ret < 0) {
            char errorInfo[1024];
            snprintf(errorInfo, sizeof(errorInfo),
                    "In origdomain.conf: [%s] in line=[%d] line=[%.*s]",
                    g_pszErrorType[-ret-1], i, (int32_t)(strlen(line2)-1), line2);
            addInfoLog(2, errorInfo);
            continue;
        }
        ++g_iOrigDomainItem;
    }

    return 0;
}

/*处理配置文件的一行
 *输入:
 *    line----配置文件的一行
 *输出:
 *    pstDetectOrigin----一个主机
 *返回值:
 *    FIELD_NUMBER_ERROR----字段个数错误
 *    HOST_REPEAT_ERROR----主机重复
 *    METHOD_ERROR----方法错误
 *    MEMORY_ERROR----内存错误
 *    IP_ERROR----ip错误
 *    -6----不侦测不修改
 *   正整数----成功，ip的个数
 */
int dealwithOneLine(char* line, struct DetectOrigin* pstDetectOrigin,int detect_time)
{
    int ret;
    char *str = NULL;
    char *ptr = NULL;

    ReleaseCpu();
    str = strtok(line, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    strcpy(pstDetectOrigin->host, str);
    if ('.' != str[strlen(str) - 1])
        strcat(pstDetectOrigin->host, ".");
    if( NULL != SearchHost( pstDetectOrigin->host ) )
    {
        return HOST_REPEAT_ERROR;
    }
    pstDetectOrigin->recent_added = checkRepeatAnyHost(pstDetectOrigin->host) ? 0 : 1;
    //得到info, yes表示写其它文件
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    pstDetectOrigin->info = strncasecmp(str, "Y", 1) ? 0 : 1;
    //得到是否进行检测,yes表示要进行检测
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    pstDetectOrigin->detect = strncasecmp(str, "Y", 1) ? 0 : 1;

    //得到侦测次数
    str = strtok(NULL, " \t");
    if (NULL == str)
        return FIELD_NUMBER_ERROR;
    pstDetectOrigin->times = atoi(str);
    if (pstDetectOrigin->times <= 0) {
        pstDetectOrigin->times = 3;
    }
    //得到告警时间
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    pstDetectOrigin->warning_time = atof(str);

    //得到好的响应时间
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    if(!strchr(str,'%')) {
        pstDetectOrigin->good_time = atof(str);
        pstDetectOrigin->ignoreDetectTimeChgPercent = 0;
    } else {
        str = strtok_r(str,"%",&ptr);
        assert(str != NULL);
        pstDetectOrigin->good_time = atof(str);
        cclog(6,"pstDetectOrigin->good_time=%f",pstDetectOrigin->good_time);
        pstDetectOrigin->ignoreDetectTimeChgPercent = atoi(ptr);
        cclog(6,"pstDetectOrigin->ignoreDetectTimeChgPercent=%d",pstDetectOrigin->ignoreDetectTimeChgPercent);
    }

    //得到取源站内容长度
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    pstDetectOrigin->length = atoi(str);
    if (pstDetectOrigin->length < 0) {
        pstDetectOrigin->length = 500;
    }
    //得到最后取几个好的ip
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    pstDetectOrigin->goodIp = atoi(str);
    if (pstDetectOrigin->goodIp <= 0) {
        pstDetectOrigin->goodIp = 2;
    }

    //是否按照侦测结果进行修改,yes表示按照侦测结果进行修改
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    pstDetectOrigin->modify = strncasecmp(str, "Y", 1) ? 0 : 1;
    //得到backup
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    strcpy(pstDetectOrigin->backup, str);
    ret = getIp(str, &(pstDetectOrigin->bkip));
    if(ret > 0) {
        pstDetectOrigin->nbkip = ret;
        pstDetectOrigin->backupIsIp = 1;
    } else {
        pstDetectOrigin->backupIsIp = 0;
        pstDetectOrigin->nbkip = 0;
    }
    //得到请求方法、请求的主机和文件名
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    ret = getMethod(str, pstDetectOrigin->host, pstDetectOrigin->rHost, pstDetectOrigin->filename);
    //在兼容旧配置文件的情况夏，设置默认的method2等值
    ret = getMethod(str, pstDetectOrigin->host, pstDetectOrigin->rHost2, pstDetectOrigin->filename2);
    if (-1 == ret) {
        return METHOD_ERROR;
    }
    pstDetectOrigin->method = ret;
    pstDetectOrigin->method2 = ret;

    //得到正确的响应代码集合
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    strcpy(pstDetectOrigin->rtn_code, str);
    ret = getCode(str, &(pstDetectOrigin->code));
    if (-1 == ret) {
        return MEMORY_ERROR;
    }

    //得到要探测的ip集合
    str = strtok(NULL, " \t\r\n");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    ret = getIp(str, &(pstDetectOrigin->ip));
    pstDetectOrigin->ipNumber = ret;
    if (-1 == ret || -2 == ret) {
        return IP_ERROR;
    } else if (-3 == ret) {
        return MEMORY_ERROR;
    }

    //add by chinacache zenglj
    //首先设置好需要用到的默认值。如果是老的配置文件则使用默认值。
    pstDetectOrigin->port = 80;
    pstDetectOrigin->weight0 = 100;
    pstDetectOrigin->weight1 = 0;
    pstDetectOrigin->sortIp = 1;
    pstDetectOrigin->current_read_buf = NULL;
    pstDetectOrigin->current_write_buf = NULL;
    pstDetectOrigin->tcp_detect_time = 1;
    pstDetectOrigin->http_detect_time = 1;

    if(pstDetectOrigin->nbkip >= 1) {
        make_new_ip(pstDetectOrigin);
    }

    pstDetectOrigin->ready = -1;
    str = strtok(NULL, " \t\r\n");
    if (NULL == str)
    {
        { // add for inserting host name to hash table
            DetectOriginIndex *data = (DetectOriginIndex*)malloc( sizeof(DetectOriginIndex) );
            if( data == NULL )
                return FIELD_NUMBER_ERROR;
            DetectOriginIndexFill( data, pstDetectOrigin->host );
            HashInsert( g_detect_tasks_index, (void*)data );
        }
        return ret;//说明是老的配置文件，直接返回。
    }
    ret = getMethod(str, pstDetectOrigin->host, pstDetectOrigin->rHost2, pstDetectOrigin->filename2);
    if (-1 == ret) {
        return METHOD_ERROR;
    }
    pstDetectOrigin->method2 = ret;

    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    pstDetectOrigin->port = atoi(str);
    if(pstDetectOrigin->port > 65535) {
        return -1;
    }
    if (pstDetectOrigin->port <= 0) {
        pstDetectOrigin->port = 80;
    }

    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    pstDetectOrigin->origDomain = strncasecmp(str, "Y", 1) ? 0 : 1;
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    char* weightStr0 = str;
    pstDetectOrigin->weight0 = atoi(weightStr0);
    char* weightStr1 = strstr(str, ":");
    pstDetectOrigin->weight1 = atoi(weightStr1+1);
    if ((pstDetectOrigin->weight0 + pstDetectOrigin->weight1) != 100) {
        return DETECTWEIGHT_ERROR;
    }
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    pstDetectOrigin->historyCount = atoi(str);
    if (pstDetectOrigin->historyCount <= 0) {
        pstDetectOrigin->historyCount = 5;
    }

    str = strtok(NULL, " \t\r\n");
    if (NULL == str) {
        return FIELD_NUMBER_ERROR;
    }
    pstDetectOrigin->sortIp = strncasecmp(str, "Y", 1) ? 0 : 1;
    str = strtok(NULL, " \t\r\n");
    if (NULL != str) {
        pstDetectOrigin->tcp_detect_time = atoi(str);
        cclog(5,"pstDetectOrigin->tcp_detect_time = %d",pstDetectOrigin->tcp_detect_time);
    }

    str = strtok(NULL, " \t\r\n");
    if (NULL != str) {
        pstDetectOrigin->http_detect_time = atoi(str);
        cclog(5,"pstDetectOrigin->http_detect_time = %d",pstDetectOrigin->http_detect_time);
    }
#ifdef ROUTE
    str = strtok(NULL, " \t\r\n");
    str = strtok(NULL, " \t\r\n");
    if( str != NULL )
    {
        int total;
        char *p;
        char *token;
        uint32_t *ip;
        uint16_t *port;
        total = 1;
        for( p = str; *p != '\0'; ++p )
            if( *p == '|' )
                ++total;
        p = str;
        pstDetectOrigin->final_ip_number = total;
        ip = pstDetectOrigin->final_ip = (uint32_t*) malloc( sizeof(uint32_t) * total );
        port = pstDetectOrigin->final_ip_port = (uint16_t*) malloc( sizeof(uint16_t) * total );
        while(( token = strsep( &p, "|" )) != NULL )
        {
            --total;
            inet_pton( AF_INET, strsep( &token, ":" ), ip );
            ++ip;
            *port = htons( atoi( token ) );
            ++port;
        }
        assert( total == 0 );
    }
#endif /* ROUTE */
    { // add for inserting host name to hash table
        DetectOriginIndex *data = (DetectOriginIndex*)malloc( sizeof(DetectOriginIndex) );
        if( data == NULL )
            return FIELD_NUMBER_ERROR;
        DetectOriginIndexFill( data, pstDetectOrigin->host );
        HashInsert( g_detect_tasks_index, (void*)data );
    }

    pstDetectOrigin->detect_time = detect_time;

    return 0;
}

int getRecentAddedFromAnyhost(const char *filename)
{
    int i = 0;
    int count = 0;
    int fgInclude = 0;  //是否进入了;-----里
    FILE *fp = NULL;
    char* str = NULL;
    char line[MAX_ONE_LINE_LENGTH];

    do {
        fp = fopen(filename, "r");
        if (NULL == fp && 3 == count) {
            addInfoLog(2, "can not open /var/named/chroot/var/named/anyhost\n");
            return -1;
        }
    } while (NULL == fp && ++count <= 3);

    while(!feof(fp)) {
        memset(line, 0, MAX_ONE_LINE_LENGTH);
        str = fgets(line, MAX_ONE_LINE_LENGTH, fp);
        if (NULL == str) {
            continue;
        }
        //注释和空行丢掉
        if (('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0]))
            continue;

        if (!memcmp(line, ";-----", 6)) {
            fgInclude = fgInclude ? 0 : 1;
            continue;
        }
        if(!fgInclude) {
            str = strtok(line," \t");
            if (NULL == str) {
                continue;
            }
            if(str[0] != ';') {
                /* FIXME: don't give up the line ';' ??? */
                //if (i >= max_detectDomain_capacity) {
                //	max_detectDomain_capacity = i + INCREASE_STEP;
                //	g_detect_domain = realloc(g_detect_domain, max_detectDomain_capacity * sizeof(detect_domain_t));
                //	if (NULL == g_detect_domain) {
                //		addInfoLog(2, "can not realloc for g_detect_domain");
                //		fclose(fp);
                //		return -1;
                //	}
                //}
                //strcpy(g_detect_domain[i].detect_domain, str);
                DetectOriginIndex *data;
                char *hostname;
                hostname = strdup( str );       /* need to call free() */
                if( hostname == NULL )
                {
                    fclose( fp );
                    return -1;
                }
                data = (DetectOriginIndex*) malloc( sizeof(DetectOriginIndex) ); /* need to call free */
                if( data == NULL )
                {
                    fclose( fp );
                    free( hostname );
                    return -1;
                }
                g_detect_anyhost_count = ++i;
                DetectOriginIndexFill( data, hostname );
                if( !HashSearch( g_detect_domain_table, data ) )
                    HashInsert( g_detect_domain_table, data );
            }
        }
    }

    fclose(fp);

    return 0;
}

int parseDomainConf(const char *filename) 
{
    int i, ret;
    char* str;
    char line[MAX_ONE_LINE_LENGTH];
    char line2[MAX_ONE_LINE_LENGTH];
    FILE *fp = fopen(filename, "r");
    if (NULL == fp) {
        addInfoLog(2, "can not open domain.conf, now exit");
        return -1;
    }
    Debug( 40, "parsing %s", filename);
    i = 0;
    while(!feof(fp)) {
        memset(line, 0, MAX_ONE_LINE_LENGTH);
        str = fgets(line, MAX_ONE_LINE_LENGTH, fp);
        if (NULL == str) {
            continue;
        }
        ++i;
        //注释和空行丢掉
        if (('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0])) {
            continue;
        }
        if (g_detect_task_count >= max_tasks_capacity) {
            max_tasks_capacity = g_detect_task_count + INCREASE_STEP;
            g_detect_tasks = realloc(g_detect_tasks, max_tasks_capacity * sizeof(struct DetectOrigin *));
            if (NULL == g_detect_tasks) {
                addInfoLog(2, "can not realloc for g_detect_tasks");
                fclose(fp);
                return -1;
            }
        }
        g_detect_tasks[g_detect_task_count] = (struct DetectOrigin*)cc_malloc(sizeof(struct DetectOrigin));
        if (NULL == g_detect_tasks[g_detect_task_count]) {
            fclose(fp);
            return -1;
        }
        memset(g_detect_tasks[g_detect_task_count], 0, sizeof(struct DetectOrigin));
        strcpy(line2, line);
        ret = dealwithOneLine(line, g_detect_tasks[g_detect_task_count], detect_time);
        if (ret < 0) {
            char errorInfo[1024];
            snprintf(errorInfo, sizeof(errorInfo), "[%s] in line=[%d] line=[%.*s]", g_pszErrorType[-ret-1], i, (int32_t)(strlen(line2)-1), line2);
            addInfoLog(2, errorInfo);
            freeOneDetectOrigin( g_detect_tasks[g_detect_task_count] );
            //cc_free( g_detect_tasks[g_detect_task_count] );
            continue;
        }
        ++g_detect_task_count;
        ReleaseCpu();
    }
    fclose(fp);
    return 0;
}

int parseAllDomainConf(void)
{
    int i;
    //const char *pattern = "/usr/local/squid/etc/*_origdomain.conf";
    const char *pattern = "/usr/local/squid/etc/custom_domain/*_domain.conf";
    glob_t glob_data;
    int ret;
	ret = glob(pattern, GLOB_NOSORT, NULL, &glob_data);
    if( ret )
    {
        Debug(30, "cannot find %s", pattern);
    }
    else
    {
        for( i = 0; i < glob_data.gl_pathc; i++ )
            parseDomainConf(glob_data.gl_pathv[i]);
        globfree(&glob_data);
    }
    return 0;
}

/*从配置文件读取要侦测的主机，不合格的行要写log
 *输入：
 *    fp----配置文件指针
 *返回值：
 *    -1----失败
 *    0----成功
 *    fixed by xin.yao:2011-11-14
 */
int getDetectOrigin(const char * filename)
{
    FILE *fp_detect;
    char tmp_buf[20];
    int ret, fd;
    int detect_times = 0;

    /* start: record the detect times */
    memset(tmp_buf, 0, 20);
    errno = 0;
    fp_detect = fopen("/usr/local/squid/etc/detect_time.tmp", "r+");
    if(NULL == fp_detect) {
        detect_times = 1;
#ifndef FRAMEWORK
        fd = creat("/usr/local/squid/etc/detect_time.tmp", FILE_MODE );
#else /*  FRAMEWORK */
        fd = open( "/usr/local/squid/etc/detect_time.tmp", O_CREAT|O_RDWR|O_TRUNC|FILE_MODE, 0644 );
#endif /*  FRAMEWORK */
        fp_detect = fdopen(fd, "r+");
        snprintf(tmp_buf, 20, "%d\n", detect_times);
    }
    else {
        ret = fread(tmp_buf, 1, sizeof(tmp_buf), fp_detect);
        if( ret )
        {
            detect_times = atoi(tmp_buf);
            ++detect_times;
            sprintf(tmp_buf, "%d\n", detect_times);
            fd = fileno(fp_detect);
        }
        else
        {
            Debug( 30, "Something wrong while call fread().%s", strerror( errno ) );
            fd = -1;
        }
    }
    assert(fp_detect != NULL);
    lseek(fd, 0, SEEK_SET);
    if( ftruncate(fd, 0) )
        Debug( 30, "Something wrong while call ftruncate()." );
    fseek(fp_detect, 0, SEEK_SET);
    fprintf(fp_detect, "%s", tmp_buf);
    fflush(fp_detect);
    close(fd);
    fclose(fp_detect);
    /* end: record the detect times */

    g_detect_task_count = 0;
    //打开要侦测的文件
    parseDomainConf(filename);
    parseAllDomainConf();

    return g_detect_task_count ? 0 : -1;
}


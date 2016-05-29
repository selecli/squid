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
#include <assert.h>
#include "detect_orig.h"
#include "shortest_path.h"

static const char kRESULT_LOG[] = "/var/log/chinacache/detectorigin.log";
static FILE *result_log = NULL;
static time_t named_timestamp = 0;


char g_szLogFile[256] = "/var/log/chinacache/";    //log文件名，现在只是目录，初始化的时候加上文件名，文件名为执行程序名+.log

const char IP_WORK[] = "ip_work";
const char IP_GOOD[] = "ip_good";
const char IP_BAD[] = "ip_bad";
const char IP_DOWN[] = "ip_down";

#ifdef ROUTE
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  WriteToShortestPathFile
 *  Description:
 * =====================================================================================
 */
    static int
WriteToShortestPathFile( const struct IpDetect *ip_detect )
{
    RoutePath path;
    assert( ip_detect );
    path.ip_from = 0;
    inet_pton( AF_INET, ip_detect->ip, &path.ip_to );
    path.ip_to_port = ip_detect->final_ip_port;
    path.state.used_time = ip_detect->usedtime;
    Debug( 50, "Write route path. ip is %u:%u, used time is %lf", path.ip_to, path.ip_to_port, path.state.used_time );
    return ShortestPathWriteFileData( &path );
} /* - - - - - - - end of function WriteToShortestPathFile - - - - - - - - - - - */
#endif /* ROUTE */

/*比较两个ip探测结果，探测成功的放在前面，同样成功的时间短的放在前面*/
int compareIpDetect(const void* ptr1, const void* ptr2)
{
    const struct IpDetect* ip1 = (const struct IpDetect*)ptr1;
    const struct IpDetect* ip2 = (const struct IpDetect*)ptr2;
    if (ip1->ok < ip2->ok) //成功的ok为1，失败的为0
    {
        return 1;
    }
    else if (ip1->ok > ip2->ok)
    {
        return -1;
    }
    else
    {
        if (ip1->usedtime > ip2->usedtime)
        {
            return 1;
        }
        else if (ip1->usedtime < ip2->usedtime)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
}

//-------------------------------------------------------------------
/*比较两个ip探测结果，探测成功的放在前面，同样成功的时间短的放在前面*/
/*第一个数要加权，目的是防止源站ip切换频繁*/
int compareIpDetectWithIndex(const void* ptr1, const void* ptr2, int percent)
{
    int index = percent;
    const struct IpDetect* ip1 = (const struct IpDetect*)ptr1;
    const struct IpDetect* ip2 = (const struct IpDetect*)ptr2;

    if (ip1->ok < ip2->ok) {  //成功的ok为1，失败的为0
        return -1;
    } else if (ip1->ok > ip2->ok) {
        return 0;
    } else {
        if (ip1->usedtime - ip2->usedtime > ip1->usedtime*index/100) {
            return -1;
        } else if (ip1->usedtime < ip2->usedtime) {
            return  1;
        } else {
            return  0;
        }
    }
}

void select_sort(void* arr, int num, int percent)   
{   
    int i, j;  
    int number_ip = num;
    struct IpDetect temp; 
    struct IpDetect* ip = (struct IpDetect*)arr;

    memset(&temp, 0, sizeof(struct IpDetect));

    for(i = 0; i < number_ip; ++i) {   
        for(j = i + 1; j < number_ip; ++j) {   
            if(-1 == compareIpDetectWithIndex(&ip[i], &ip[j], percent)) {   
                memcpy(&temp, &ip[j], sizeof(struct IpDetect));   
                memcpy(&ip[j], &ip[i], sizeof(struct IpDetect));   
                memcpy(&ip[i], &temp, sizeof(struct IpDetect));   
            }   
        }   
    }   
}   

double compareIpDetectWithProportion(const void* ptr1, const void* ptr2, double proportion, int flag)
{
    const struct IpDetect *ip1 = (const struct IpDetect*)ptr1;
    const struct IpDetect *ip2 = (const struct IpDetect*)ptr2;

    if (NULL == ip1 || NULL == ip2) {
        return 0;
    }

    if (ip1->ok != ip2->ok) {  //成功的ok为1，失败的为0
        //puts("ip1->ok != ip2->ok");
        switch (flag) {
            case 1:
                return ip1->ok - ip2->ok;
            case 2:
                return ip2->ok - ip1->ok;
            case 3:
                return ip1->ok - ip2->ok;
            default:
                return ip1->ok - ip2->ok;
        }
    } else {
        //puts("ip1->ok == ip2->ok");
        switch (flag) {
            case 1:
                //printf("time-->%f\n", ip2->usedtime * proportion - ip1->usedtime);
                return ip2->usedtime * proportion - ip1->usedtime;
            case 2:
                //printf("time-->%f\n", ip1->usedtime - ip2->usedtime * proportion);
                return ip1->usedtime - ip2->usedtime * proportion;
            case 3:
                return ip2->usedtime - ip1->usedtime;
            default:
                return ip2->usedtime * proportion - ip1->usedtime;
        }
    }
}

/* < 0: sort in ascending order, >= 0: no sort*/
void selectSortByProportion(void* arr, int num, double proportion)
{
    int i, j;
    double ret;   
    int number_ip = num; 
    struct IpDetect temp; 
    struct IpDetect* ip = (struct IpDetect*)arr;

    if (num <= 1) {
        return;
    }

    memset(&temp, 0, sizeof(struct IpDetect));

    for(i = 0; i < number_ip; ++i) {    
        for(j = i + 1; j < number_ip; ++j) {    
            ret = 0;
            if (!ip[i].upper_ip && ip[j].upper_ip) {
                //printf("ip1 = %s, ip2 = %s\n", ip[i].ip, ip[j].ip);
                ret = compareIpDetectWithProportion(&ip[i], &ip[j], proportion, 1);
            } else if (ip[i].upper_ip && !ip[j].upper_ip) {
                //printf("ip1 = %s, ip2 = %s\n", ip[j].ip, ip[i].ip);
                ret = compareIpDetectWithProportion(&ip[j], &ip[i], proportion, 2);
            } else {
                ret = compareIpDetectWithProportion(&ip[i], &ip[j], 0, 3);
            }
            if(ret < 0) {    
                memcpy(&temp, &ip[j], sizeof(struct IpDetect));   
                memcpy(&ip[j], &ip[i], sizeof(struct IpDetect));   
                memcpy(&ip[i], &temp, sizeof(struct IpDetect));   
            }    
        }    
    }    
}

/*初始化log文件，得到log文件名，记录程序开始时间
 *输入：
 *    processName----执行程序名
 */
void initLog(const char* processName)
{
    const char* str = strrchr(processName, '/');
    if (NULL == str)
        str = processName;
    else
        str++;
    strcat(g_szLogFile, str);
    strcat(g_szLogFile, ".log");
    FILE* fp = fopen(g_szLogFile, "a");
    if (NULL == fp)
        return;
    fprintf(fp, "---------------------------------------------------\n");
    time_t t = time(NULL);
    char* str2 = ctime(&t);
    fprintf(fp, "start at: [%.*s] process=[%s] pid=[%u]\n", (int32_t)(strlen(str2)-1), str2, str, getpid());
    fclose(fp);
}

/*给log文件增加一行*/
void addOneLog(const char* str)
{
    FILE* fp = fopen(g_szLogFile, "a");
    if (NULL == fp)
    {
        return;
    }
    fprintf(fp, "\t%s\n", str);
    fclose(fp);
}


//-------------------------------------------------------------------
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

//-------------------------------------------------------------------
//-------------------------------------------------------------------
#ifndef FRAMEWORK
int addResultLog(void)
{
    char resultLogFileName[128];
    memset(resultLogFileName, 0, sizeof(resultLogFileName));
    strcpy(resultLogFileName, "/var/log/chinacache/detectorigin.log");
    FILE* resultFile = fopen(resultLogFileName, "a+");
    if(NULL == resultFile)
    {
        return -1;
    }

    fprintf(resultFile, "Detect Log Begin----------------------------\n");
    time_t recTime;
    recTime = time(NULL);
    char detectTime[1024];
    snprintf(detectTime, sizeof(detectTime), "%s",ctime(&recTime));
    fprintf(resultFile, "%s", detectTime);
    int i;

    for(i = 0; i < g_detect_task_count; i++)
    {
        char buf[BUFSIZ];
        char method[1024];
        char host[1024];
        char IP[1024];
        char returnCode[1024];
        char useTime[1024];
        char state[1024];
        snprintf(method, sizeof(method), "%s", (g_detect_tasks[i]->method == 1) ? "Get" : "Head");
        snprintf(host, sizeof(host), "%s", g_detect_tasks[i]->host);
        int j;
        if(0 == g_detect_tasks[i]->detect)
        {
            char NoDetect[64];
            snprintf(IP, sizeof(IP), "%s", g_detect_tasks[i]->ip[0].ip);
            snprintf(NoDetect, sizeof(NoDetect), "%s", "No_Detect");
            snprintf(buf, sizeof(buf), "%s\t%s%s\t%s\n", method, host, IP, NoDetect);
            fprintf(resultFile, buf);
        }
        else
        {
            int totalIP = 0;
            if(1 == g_detect_tasks[i]->backupIsIp &&
                    -1 == g_detect_tasks[i]->ready)
				totalIP = g_detect_tasks[i]->ipNumber + g_detect_tasks[i]->nbkip;
            else
                if(g_detect_tasks[i]->backupIsIp)
					totalIP = g_detect_tasks[i]->ipNumber + g_detect_tasks[i]->nbkip;
                else
                    totalIP = g_detect_tasks[i]->ipNumber;
            for(j = 0; j < totalIP; j++)
            {
                snprintf(IP, sizeof(IP), "%s", g_detect_tasks[i]->ip[j].ip);
                snprintf(returnCode, sizeof(returnCode), "%s", g_detect_tasks[i]->ip[j].returnCode);
                snprintf(useTime, sizeof(useTime), "%f", g_detect_tasks[i]->ip[j].usedtime);
                if(1 == g_detect_tasks[i]->ip[j].ok)
                {
                    if(g_detect_tasks[i]->ip[j].usedtime < g_detect_tasks[i]->good_time)
                    {
                        snprintf(state, sizeof(state), "%s", IP_GOOD);
                    }
                    else
                    {
                        snprintf(state, sizeof(state), "%s", IP_BAD);
                    }
                }
                else
                {
                    snprintf(state, sizeof(state), "%s", IP_DOWN);
                }
                snprintf(buf, sizeof(buf), "%s\t%s\t%s\t%s\t%s\t%s\n", method, host, IP, returnCode, useTime, state);
                fprintf(resultFile, buf);
            }
        }
    }

    fprintf(resultFile, "Detect Log End------------------------------\n");
    fclose(resultFile);

    return 1;
}
#else /* FRAMEWORK */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ResultLogInit
 *  Description:
 * =====================================================================================
 */
    int
ResultLogInit( void )
{
    time_t t;
    result_log = fopen( kRESULT_LOG, "a" );
    if( result_log == NULL )
    {
        Debug( 20, "Cannot open %s.", kRESULT_LOG );
        return 1;
    }
    time( &t );
    fprintf( result_log, "Detect Log Begin----------------------------\n" );
    fprintf( result_log, "%s", ctime( &t ) );
    return 0;
} /* - - - - - - - end of function ResultLogInit - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ResultLogAdd
 *  Description:
 * =====================================================================================
 */
    int
ResultLogAdd( const struct DetectOrigin *detect_origin, const struct IpDetect *ip )
{
    static const char kMETHOD_GET[] = "GET";
    static const char kMETHOD_HEAD[] = "HEAD";
    const char *ip_status;
    const char *method;
    assert( detect_origin );
    assert( ip );
    if( result_log == NULL )
        return 0;
    method = detect_origin->method == 1 ? kMETHOD_GET : kMETHOD_HEAD;
    /* no detect */
    if( detect_origin->detect == 0 )
        fprintf( result_log, "%-4s %-30s %-15s No_Detect\n", method, detect_origin->host, ip->ip );
    else
    {
        if( ip->ok == 1 )
            if( ip->usedtime < detect_origin->good_time )
                ip_status = IP_GOOD;
            else
                ip_status = IP_BAD;
        else
            ip_status = IP_DOWN;
        fprintf( result_log, "%-4s %-30s %-15s %-3s %-10.6lf %-7s\n", method, detect_origin->host, ip->ip, ip->returnCode, ip->usedtime, ip_status );
    }
    return 0;
} /* - - - - - - - end of function ResultLogAdd - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  ResultLogEnd
 *  Description:
 * =====================================================================================
 */
    int
ResultLogEnd( void )
{
    if( result_log != 0 )
    {
        fprintf( result_log, "Detect Log End------------------------------\n");
        fclose( result_log );
    }
    return 0;
} /* - - - - - - - end of function ResultLogEnd - - - - - - - - - - - */
#endif /* FRAMEWORK */

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int addInfoLog(int type, const char* str)
{
    char infoLogFileName[128];
    Debug( 20, "detectorig %d: %s", type, str );
    return 0;
    memset(infoLogFileName, 0, sizeof(infoLogFileName));
    strcpy(infoLogFileName, "/var/log/chinacache/detectorigin_info.log");

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
        fprintf(fp, "Warning[%s]:%s\n", timeStr, str);
    else {
        if(2 == type)
            fprintf(fp, "Error[%s]:%s\n", timeStr, str);
        else {
            if(3 == type)
                fprintf(fp, "Fatal[%s]:%s\n", timeStr, str);
        }
    }

    fclose(fp);

    return 1;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int addAlertLog(int type, char* host, char* ipBuf)
{
    char alertLogFileName[128];
    memset(alertLogFileName, 0, sizeof(alertLogFileName));
    strcpy(alertLogFileName, "/usr/local/squid/var/dig_alert.dat");

    FILE* fp = fopen(alertLogFileName, "a+");
    if(NULL == fp)
        return -1;

    if(1 == type)
        fprintf(fp, "%s pass\n", host);
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
//*******************************************************
void modifyUsedTime(void)
{
    int i = 0;
    int j = 0;

    for (i=0;i<g_detect_task_count;i++)
    {
        /*
        if (have_recent_added) {
            if (!g_detect_tasks[i]->recent_added) {
                //printf("modifyUsedTime--> %s\n", g_detect_tasks[i]->host);
                continue;
            }
        }
        */
        struct IpDetect* ip = g_detect_tasks[i]->ip;
        //      for (j=0;j<g_detect_tasks[i]->ipNumber+g_detect_tasks[i]->backupIsIp;j++)
        for (j=0;j<g_detect_tasks[i]->ipNumber+g_detect_tasks[i]->nbkip;j++)
        {
            if(0 == g_detect_tasks[i]->flag_http_tcp)
            {
                cclog(2,"g_detect_tasks[%d]->ip[%d].ip=%s",i,j,g_detect_tasks[i]->ip[j].ip);
                ip[j].usedtime /= g_detect_tasks[i]->times;
                ip[j].usedtime1 /= g_detect_tasks[i]->times;
                ip[j].usedtime = (ip[j].usedtime * g_detect_tasks[i]->weight0 +
                        ip[j].usedtime1 * g_detect_tasks[i]->weight1)/100;
                cclog(2,"ip[%d].usedtime=%f",j,ip[j].usedtime);
                cclog(2,"ip[%d].usedtime1=%f",j,ip[j].usedtime1);
                cclog(2,"ip[%d].times=%d",j,g_detect_tasks[i]->times);
            }
            else
            {
                cclog(2,"TCP:g_detect_tasks[%d]->ip[%d].ip=[%s] time[%f]\t ok [%d]",i,j,g_detect_tasks[i]->ip[j].ip,ip[j].usedtime,g_detect_tasks[i]->ip[j].ok);
            }
        }
    }
}

//****************************下面的内容为写最后的结果*************************

/*写ip为0的情况。如果backup是ip，记录这条ip；如果backup是字符串，写这个字符串
 */
void writeIpZero(FILE* fp, struct DetectOrigin* pstDetectOrigin)
{
    struct IpDetect* pstIpDetect = pstDetectOrigin->ip;
    int i = 0;
    if (1 == pstDetectOrigin->backupIsIp)  //backup是ip
    {
#ifdef ROUTE
        WriteToShortestPathFile( &pstIpDetect[pstDetectOrigin->ipNumber] ); /* 选最好的backup ip */
#endif /* ROUTE */
        for(;i<pstDetectOrigin->nbkip;i++)
        {
            char warning = 'N';
            if (pstIpDetect[pstDetectOrigin->ipNumber+i].usedtime > pstDetectOrigin->warning_time)
            {
                warning = 'Y';
            }
            fprintf(fp, "%-24s IN      A       %s ; %.6f backup",
                    pstDetectOrigin->host, pstIpDetect[pstDetectOrigin->ipNumber+i].ip,
                    pstIpDetect[pstDetectOrigin->ipNumber+i].usedtime);
            if (1 == pstIpDetect[pstDetectOrigin->ipNumber+i].ok)
            {
                if (pstIpDetect[pstDetectOrigin->ipNumber+i].usedtime > pstDetectOrigin->good_time)
                {
                    fprintf(fp, " %s %s %c\n", IP_WORK, pstIpDetect[pstDetectOrigin->ipNumber+i].returnCode, warning);
                }
                else
                {
                    fprintf(fp, " %s %s %c\n", IP_BAD, pstIpDetect[pstDetectOrigin->ipNumber+i].returnCode, warning);
                }
            }
            else
            {
                fprintf(fp, " %s", IP_DOWN);
                if (0 == pstIpDetect[pstDetectOrigin->ipNumber+i].returnCode[0])
                    fprintf(fp, " 000 %c\n", warning);
                else
                    fprintf(fp, " %s %c\n", pstIpDetect[pstDetectOrigin->ipNumber+i].returnCode, warning);
            }
        }
    }
    else if ((0!=strcasecmp(pstDetectOrigin->backup, "yes")) &&
            (0!=strcasecmp(pstDetectOrigin->backup, "y")) &&
            (0!=strcasecmp(pstDetectOrigin->backup, "no")) &&
            (0!=strcasecmp(pstDetectOrigin->backup, "n")))
    {
        fprintf(fp, "%-24s IN      CNAME   %s\n", pstDetectOrigin->host,
                pstDetectOrigin->backup);
    }
}

/*写侦测但却不修改anyhost文件的主机
*/
void wirteWastedDetectLog(struct DetectOrigin* pstDetectOrigin)
{
    int i;
    FILE* fp = NULL;
    struct IpDetect* pstIpDetect = pstDetectOrigin->ip;

    fp = fopen(g_szLogFile, "a");
    if (NULL == fp) {
        return;
    }
    for (i = 0; i < pstDetectOrigin->ipNumber+pstDetectOrigin->nbkip; ++i) {
        if (0 == pstIpDetect[i].ok) {
            fprintf(fp, "\tINFO detect failure: host=[%s] ip=[%s];usedtime=[%.6f]\n",
                    pstDetectOrigin->host, pstIpDetect[i].ip, pstIpDetect[i].usedtime);
        } else {
            fprintf(fp, "\tINFO detect success: host=[%s] ip=[%s];usedtime=[%.6f]\n",
                    pstDetectOrigin->host, pstIpDetect[i].ip, pstIpDetect[i].usedtime);
        }
    }
    fclose(fp);
}

/*写不需要侦测的情况（即认为所有ip都是好的）
 */
void writeNoDetect(FILE* fp, struct DetectOrigin* pstDetectOrigin)
{
    struct IpDetect* pstIpDetect = pstDetectOrigin->ip;
    if (pstDetectOrigin->goodIp > pstDetectOrigin->ipNumber)
        pstDetectOrigin->goodIp = pstDetectOrigin->ipNumber;
    int i;
    for (i=0;i<pstDetectOrigin->goodIp;i++) {
        fprintf(fp, "%-24s IN      A       %s ; 0 ip no_detect 000 N\n",
                pstDetectOrigin->host, pstIpDetect[i].ip);
    }
    for (;i<pstDetectOrigin->ipNumber+pstDetectOrigin->nbkip;i++) {
        fprintf(fp, ";%-23s IN      A       %s ; 0 ip no_detect 000 N\n",
                pstDetectOrigin->host, pstIpDetect[i].ip);
    }
}

/*写有ip比较好的情况。这种情况最多写配置文件指定的ip，其它的写成注释，好的ip前面加;，坏的ip前面也是;不通的也是;
 */
void writeGoodIp(FILE* fp, struct DetectOrigin* pstDetectOrigin)
{
    int i;
    char warning = 'N';
    struct IpDetect* pstIpDetect = pstDetectOrigin->ip;

    warning = pstIpDetect[0].usedtime > pstDetectOrigin->warning_time ? 'Y' : 'N';
    /*
       if (pstIpDetect[0].usedtime > pstDetectOrigin->warning_time)
       warning = 'Y';
     */
    //写一个好的ip
#ifdef ROUTE
    WriteToShortestPathFile( pstIpDetect );
#endif /* ROUTE */
    fprintf(fp, "%-24s IN      A       %s ; %.6f ip %s %s %c\n",
            pstDetectOrigin->host, pstIpDetect[0].ip, pstIpDetect[0].usedtime,
            IP_WORK, pstIpDetect[0].returnCode, warning);
    if (pstDetectOrigin->goodIp > pstDetectOrigin->ipNumber) {
        pstDetectOrigin->goodIp = pstDetectOrigin->ipNumber;
    }
    for (i = 1; i < pstDetectOrigin->goodIp; ++i) {
        warning = pstIpDetect[i].usedtime > pstDetectOrigin->warning_time ? 'Y' : 'N';
        /*
           if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
           warning = 'Y';
           else
           warning = 'N';
         */
        if (pstIpDetect[i].ok && (pstIpDetect[i].usedtime < pstDetectOrigin->good_time)) {
            fprintf(fp, "%-24s IN      A       %s ; %.6f ip %s %s %c\n",
                    pstDetectOrigin->host, pstIpDetect[i].ip, pstIpDetect[i].usedtime,
                    IP_WORK, pstIpDetect[i].returnCode, warning);
        } else {
            break;
        }
    }
    for ( ; i < pstDetectOrigin->ipNumber; ++i) {
        warning = pstIpDetect[i].usedtime > pstDetectOrigin->warning_time ? 'Y' : 'N';
        /*
           if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
           warning = 'Y';
           else
           warning = 'N';
         */
        if (pstIpDetect[i].ok) {
            if (pstIpDetect[i].usedtime < pstDetectOrigin->good_time) {
                fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s %s %c\n",
                        pstDetectOrigin->host, pstIpDetect[i].ip,
                        pstIpDetect[i].usedtime, IP_GOOD, pstIpDetect[i].returnCode, warning);
            } else {
                fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s %s %c\n",
                        pstDetectOrigin->host, pstIpDetect[i].ip,
                        pstIpDetect[i].usedtime, IP_BAD, pstIpDetect[i].returnCode, warning);
            }
        } else {
            fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s",
                    pstDetectOrigin->host, pstIpDetect[i].ip,
                    pstIpDetect[i].usedtime, IP_DOWN);
            if (0 == pstIpDetect[i].returnCode[0]) {
                fprintf(fp, " 000 %c\n", warning);
            } else {
                fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
            }
        }
    }
    //如果backup是ip，写backup
    for(i = pstDetectOrigin->ipNumber; i < pstDetectOrigin->nbkip+pstDetectOrigin->ipNumber; ++i) {
        if (pstDetectOrigin->backupIsIp) {
            warning = pstIpDetect[i].usedtime > pstDetectOrigin->warning_time ? 'Y' : 'N';
            /*
               if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
               warning = 'Y';
               else
               warning = 'N';
             */
            fprintf(fp, ";%-23s IN      A       %s ; %.6f backup",
                    pstDetectOrigin->host, pstIpDetect[i].ip, pstIpDetect[i].usedtime);
            if (pstIpDetect[i].ok) {
                if (pstIpDetect[i].usedtime < pstDetectOrigin->good_time) {
                    fprintf(fp, " %s %s %c\n", IP_GOOD, pstIpDetect[i].returnCode, warning);
                } else {
                    fprintf(fp, " %s %s %c\n", IP_BAD, pstIpDetect[i].returnCode, warning);
                }
            } else {
                fprintf(fp, " %s 000 %c\n", IP_DOWN, warning);
            }
        }
    }
}

/*没有好的ip，backup为yes的情况，把坏的ip写成注释，前面加;
 */
void writeBackupYes(FILE* fp, struct DetectOrigin* pstDetectOrigin)
{
    char warning;
    int i;
    struct IpDetect* pstIpDetect = pstDetectOrigin->ip;
    for (i=0;i<pstDetectOrigin->ipNumber;i++)
    {
        if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
            warning = 'Y';
        else
            warning = 'N';

        if (1 == pstIpDetect[i].ok)
        {
            fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s %s %c\n",
                    pstDetectOrigin->host, pstIpDetect[i].ip, pstIpDetect[i].usedtime,
                    IP_BAD, pstIpDetect[i].returnCode, warning);
        } else {
            fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s",
                    pstDetectOrigin->host, pstIpDetect[i].ip,
                    pstIpDetect[i].usedtime, IP_DOWN);
            if (0 == pstIpDetect[i].returnCode[0])
                fprintf(fp, " 000 %c\n", warning);
            else
                fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
        }
    }
}

/*没有好的ip，backup为no的情况。如果坏的ip存在，写一个坏的（其它的写成注释）；否则写所有的
 */
void writeBackupNo(FILE* fp, struct DetectOrigin* pstDetectOrigin)
{
    char warning;
    int i;
    struct IpDetect* pstIpDetect = pstDetectOrigin->ip;
    if (1 == pstIpDetect[0].ok)  //如果有坏的ip，写时间最短的
    {
#ifdef ROUTE
        WriteToShortestPathFile( pstIpDetect );
#endif /* ROUTE */
        if (pstIpDetect[0].usedtime > pstDetectOrigin->warning_time)
            warning = 'Y';
        else
            warning = 'N';

        fprintf(fp, "%-24s IN      A       %s ; %.6f ip %s %s %c\n",
                pstDetectOrigin->host, pstIpDetect[0].ip, pstIpDetect[0].usedtime,
                IP_BAD, pstIpDetect[0].returnCode, warning);
        for (i=1;i<pstDetectOrigin->ipNumber;i++)
        {
            if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                warning = 'Y';
            else
                warning = 'N';

            if (1 == pstIpDetect[i].ok) {
                fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s %s %c\n",
                        pstDetectOrigin->host, pstIpDetect[i].ip,
                        pstIpDetect[i].usedtime, IP_BAD, pstIpDetect[i].returnCode, warning);
            } else {
                fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s",
                        pstDetectOrigin->host, pstIpDetect[i].ip,
                        pstIpDetect[i].usedtime, IP_DOWN);
                if (0 == pstIpDetect[i].returnCode[0])
                    fprintf(fp, " 000 %c\n", warning);
                else
                    fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
            }
        }
    }
    else  //如果都不通，写所有的ip
    {
        for (i=0;i<pstDetectOrigin->ipNumber;i++)
        {
            if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                warning = 'Y';
            else
                warning = 'N';
            fprintf(fp, "%-24s IN      A       %s ; %.6f ip %s",
                    pstDetectOrigin->host, pstIpDetect[i].ip,
                    pstIpDetect[i].usedtime, IP_DOWN);
            if (0 == pstIpDetect[i].returnCode[0])
                fprintf(fp, " 000 %c\n", warning);
            else
                fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
        }
    }
}

/*没有好的ip，backup为ip的情况。如果backup是好的ip，写这个ip；
 *否则从侦测结果里选一个坏的ip，其它的写注释；否则如果backup是坏的，写backup；
 *否则，写所有的ip（不包括backup）
 */
void writeBackupIp(FILE* fp, struct DetectOrigin* pstDetectOrigin)
{
    struct IpDetect* pstIpDetect = pstDetectOrigin->ip;
    char warning;
    int i;
    if ((1==pstIpDetect[pstDetectOrigin->ipNumber].ok) && (pstIpDetect[pstDetectOrigin->ipNumber].usedtime<pstDetectOrigin->good_time))  //backup的ip是好的
    {
#ifdef ROUTE
        WriteToShortestPathFile( &pstIpDetect[pstDetectOrigin->ipNumber] ); /* 写入最好的backup ip */
#endif /* ROUTE */
        if (pstIpDetect[pstDetectOrigin->ipNumber].usedtime > pstDetectOrigin->warning_time)
            warning = 'Y';
        else
            warning = 'N';
        fprintf(fp, "%-24s IN      A       %s ; %.6f backup %s %s %c\n",
                pstDetectOrigin->host, pstIpDetect[pstDetectOrigin->ipNumber].ip,
                pstIpDetect[pstDetectOrigin->ipNumber].usedtime, IP_WORK,
                pstIpDetect[pstDetectOrigin->ipNumber].returnCode, warning);
        for(i=pstDetectOrigin->ipNumber+1; i< pstDetectOrigin->ipNumber+pstDetectOrigin->nbkip;i++)
        {
            if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                warning = 'Y';
            else
                warning = 'N';
            if (1 == pstIpDetect[i].ok)
            {
                fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s %s %c\n",
                        pstDetectOrigin->host, pstIpDetect[i].ip,
                        pstIpDetect[i].usedtime, IP_BAD, pstIpDetect[i].returnCode, warning);
            } else {
                fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s",
                        pstDetectOrigin->host, pstIpDetect[i].ip,
                        pstIpDetect[i].usedtime, IP_DOWN);
                if (0 == pstIpDetect[i].returnCode[0])
                    fprintf(fp, " 000 %c\n", warning);
                else
                    fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
            }
            //          fprintf(fp, ";%-24s IN      A       %s ; %.6f backup %s %s %c\n",
            //                  pstDetectOrigin->host, pstIpDetect[i].ip,
            //                  pstIpDetect[i].usedtime, IP_WORK,
            //                  pstIpDetect[i].returnCode, warning);
        }

        for (i=0;i<pstDetectOrigin->ipNumber;i++)
        {
            if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                warning = 'Y';
            else
                warning = 'N';
            if (1 == pstIpDetect[i].ok)
            {
                fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s %s %c\n",
                        pstDetectOrigin->host, pstIpDetect[i].ip,
                        pstIpDetect[i].usedtime, IP_BAD, pstIpDetect[i].returnCode, warning);
            } else {
                fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s",
                        pstDetectOrigin->host, pstIpDetect[i].ip,
                        pstIpDetect[i].usedtime, IP_DOWN);
                if (0 == pstIpDetect[i].returnCode[0])
                    fprintf(fp, " 000 %c\n", warning);
                else
                    fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
            }
        }
    }
    else
    {
        if (1 == pstIpDetect[0].ok)  //侦测里有坏的ip，选一个最好的
        {
#ifdef ROUTE
            WriteToShortestPathFile( pstIpDetect );
#endif /* ROUTE */
            if (pstIpDetect[0].usedtime > pstDetectOrigin->warning_time)
            {
                warning = 'Y';
            }
            else
            {
                warning = 'N';
            }
            fprintf(fp, "%-24s IN      A       %s ; %.6f ip %s %s %c\n",
                    pstDetectOrigin->host, pstIpDetect[0].ip, pstIpDetect[0].usedtime,
                    IP_BAD, pstIpDetect[0].returnCode, warning);

            for (i=1;i<pstDetectOrigin->ipNumber;i++)
            {
                if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                    warning = 'Y';
                else
                    warning = 'N';
                if (1 == pstIpDetect[i].ok)
                {
                    fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s %s %c\n",
                            pstDetectOrigin->host, pstIpDetect[i].ip,
                            pstIpDetect[i].usedtime, IP_BAD, pstIpDetect[i].returnCode, warning);
                } else {
                    fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s",
                            pstDetectOrigin->host, pstIpDetect[i].ip,
                            pstIpDetect[i].usedtime, IP_DOWN);
                    if (0 == pstIpDetect[i].returnCode[0])
                        fprintf(fp, " 000 %c\n", warning);
                    else
                        fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
                }
            }
            //写backup

            for(i=pstDetectOrigin->ipNumber;i<pstDetectOrigin->nbkip+pstDetectOrigin->ipNumber;i++)
            {
                if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                    warning = 'Y';
                else
                    warning = 'N';
                if (1 == pstIpDetect[i].ok) {
                    fprintf(fp, ";%-23s IN      A       %s ; %.6f backup %s %s %c\n",
                            pstDetectOrigin->host, pstIpDetect[i].ip,
                            pstIpDetect[i].usedtime, IP_BAD, pstIpDetect[i].returnCode, warning);
                } else {
                    fprintf(fp, ";%-23s IN      A       %s ; %.6f backup %s",
                            pstDetectOrigin->host, pstIpDetect[i].ip,
                            pstIpDetect[i].usedtime, IP_DOWN);
                    if (0 == pstIpDetect[i].returnCode[0])
                        fprintf(fp, " 000 %c\n", warning);
                    else
                        fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
                }
            }
        }
        else
        {
            //侦测的ip都不通
            if (1 == pstIpDetect[pstDetectOrigin->ipNumber].ok)  //backup是坏的
            {
#ifdef ROUTE
                WriteToShortestPathFile( &pstIpDetect[pstDetectOrigin->ipNumber] );
#endif /* ROUTE */
                if (pstIpDetect[pstDetectOrigin->ipNumber].usedtime > pstDetectOrigin->warning_time)
                    warning = 'Y';
                else
                    warning = 'N';
                fprintf(fp, "%-24s IN      A       %s ; %.6fs backup %s %s %c\n",
                        pstDetectOrigin->host, pstIpDetect[pstDetectOrigin->ipNumber].ip,
                        pstIpDetect[pstDetectOrigin->ipNumber].usedtime, IP_BAD,
                        pstIpDetect[pstDetectOrigin->ipNumber].returnCode, warning);

                for(i=pstDetectOrigin->ipNumber+1;i<pstDetectOrigin->nbkip+pstDetectOrigin->ipNumber;i++)
                {
                    if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                        warning = 'Y';
                    else
                        warning = 'N';
                    fprintf(fp, "%-24s IN      A       %s ; %.6fs backup %s %s %c\n",
                            pstDetectOrigin->host, pstIpDetect[i].ip,
                            pstIpDetect[i].usedtime, IP_BAD,
                            pstIpDetect[i].returnCode, warning);
                }
                for (i=0;i<pstDetectOrigin->ipNumber;i++) {
                    if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                        warning = 'Y';
                    else
                        warning = 'N';

                    fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s",
                            pstDetectOrigin->host, pstIpDetect[i].ip,
                            pstIpDetect[i].usedtime, IP_DOWN);
                    if (0 == pstIpDetect[i].returnCode[0])
                        fprintf(fp, " 000 %c\n", warning);
                    else
                        fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
                }
            }
            else
            {
                //backup不通
                for (i=0;i<pstDetectOrigin->ipNumber;i++) {
                    if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                        warning = 'Y';
                    else
                        warning = 'N';

                    fprintf(fp, "%-24s IN      A       %s ; %.6f ip %s",
                            pstDetectOrigin->host, pstIpDetect[i].ip,
                            pstIpDetect[i].usedtime, IP_DOWN);
                    if (0 == pstIpDetect[i].returnCode[0])
                        fprintf(fp, " 000 %c\n", warning);
                    else
                        fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
                }

                if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                    warning = 'Y';
                else
                    warning = 'N';

                for(i=pstDetectOrigin->ipNumber;i<pstDetectOrigin->nbkip+pstDetectOrigin->ipNumber;i++)
                {
                    fprintf(fp, ";%-23s IN      A       %s ; %.6f backup %s",
                            pstDetectOrigin->host, pstIpDetect[i].ip,
                            pstIpDetect[i].usedtime, IP_DOWN);
                    if (0 == pstIpDetect[i].returnCode[0])
                        fprintf(fp, " 000 %c\n", warning);
                    else
                        fprintf(fp, " %s %c\n", pstIpDetect[i].returnCode, warning);
                }
            }
        }
    }
}

/*没有好的ip，backup为字符串的情况。写这个字符串，坏的ip写成注释
 */
void writeBackupString(FILE* fp, struct DetectOrigin* pstDetectOrigin)
{
    fprintf(fp, "%-24s IN      CNAME   %s\n", pstDetectOrigin->host,
            pstDetectOrigin->backup);
    int i;
    struct IpDetect* pstIpDetect = pstDetectOrigin->ip;
    for (i=0;i<pstDetectOrigin->ipNumber;i++)
    {
        if (1 == pstIpDetect[i].ok)
        {
            fprintf(fp, ";%-23s IN      A       %s ; %.6f ip %s %s",
                    pstDetectOrigin->host, pstIpDetect[i].ip,
                    pstIpDetect[i].usedtime, IP_BAD, pstIpDetect[i].returnCode);
            if (pstIpDetect[i].usedtime > pstDetectOrigin->warning_time)
                fprintf(fp, " Y\n");
            else
                fprintf(fp, " N\n");
        } else {
            fprintf(fp, ";%-23s IN      A       %s ; %.6f backup %s",
                    pstDetectOrigin->host, pstIpDetect[i].ip,
                    pstIpDetect[i].usedtime, IP_DOWN);
            if (0 == pstIpDetect[i].returnCode[0])
                fprintf(fp, " 000 Y\n");
            else
                fprintf(fp, " %s Y\n", pstIpDetect[i].returnCode);
        }
    }
}

void copySameIPDetectRes(void)
{
    int i, j;

    for (i = 0; i < g_detect_task_count; ++i) {
        for (j = 0; j < g_detect_tasks[i]->ipNumber; ++j) {
            if(g_detect_tasks[i]->ip[j].reuse_nodetect_flag) {
                g_detect_tasks[i]->ip[j].ok = g_detect_tasks[g_detect_tasks[i]->ip[j].reuse_task_index]->ip[g_detect_tasks[i]->ip[j].reuse_ip_index].ok; 
                g_detect_tasks[i]->ip[j].usedtime = g_detect_tasks[g_detect_tasks[i]->ip[j].reuse_task_index]->ip[g_detect_tasks[i]->ip[j].reuse_ip_index].usedtime; 
                g_detect_tasks[i]->ip[j].usedtime1 = g_detect_tasks[g_detect_tasks[i]->ip[j].reuse_task_index]->ip[g_detect_tasks[i]->ip[j].reuse_ip_index].usedtime1; 
                strncpy(g_detect_tasks[i]->ip[j].returnCode, g_detect_tasks[g_detect_tasks[i]->ip[j].reuse_task_index]->ip[g_detect_tasks[i]->ip[j].reuse_ip_index].returnCode, 4); 
            }   
        }
    }
}

/* 写配置文件里的一条记录 */
void writeOneAnyhostHost(FILE* fp, struct DetectOrigin* pstDetectOrigin)
{
    if (NULL == fp) {
        fprintf(stderr, "fp is NULL\n");
        return;
    }
    if(2 == pstDetectOrigin->flag_http_tcp) {
        return;
    }
    if (0 == atoi(pstDetectOrigin->ip->ip)) {    //ip为0，默认不通（优先级高于不修改）
        writeIpZero(fp, pstDetectOrigin);
        return;
    }
    if (0 == pstDetectOrigin->modify) {   //配置文件说不修改，写anyhost文件
        wirteWastedDetectLog(pstDetectOrigin);
        return;
    }
    if (0 == pstDetectOrigin->detect) {
        writeNoDetect(fp, pstDetectOrigin);
        return;
    }
    struct IpDetect* pstIpDetect = pstDetectOrigin->ip;
    //按照响应时间给ip排序
    //modified by chinacache zenglj
    if(pstDetectOrigin->origDomain || pstDetectOrigin->sortIp) {
        /* add by xin.yao: 2011-11-21 */
        if(0 == pstDetectOrigin->ignoreDetectTimeChgPercent) {
            qsort(pstIpDetect, pstDetectOrigin->ipNumber, sizeof(struct IpDetect), compareIpDetect);
        } else {
            select_sort(pstIpDetect, pstDetectOrigin->ipNumber, pstDetectOrigin->ignoreDetectTimeChgPercent);
        }
    }
    if (custom_cfg.proportion) {
        selectSortByProportion(pstIpDetect, pstDetectOrigin->ipNumber, custom_cfg.proportion);
    }
    if(pstDetectOrigin->nbkip > 1) {
        qsort(&pstIpDetect[pstDetectOrigin->ipNumber], pstDetectOrigin->nbkip, sizeof(struct IpDetect), compareIpDetect);
        if (custom_cfg.proportion) {
            selectSortByProportion(&pstIpDetect[pstDetectOrigin->ipNumber], pstDetectOrigin->nbkip, custom_cfg.proportion);
        }
    }
    if ((1 == pstIpDetect[0].ok) && (pstIpDetect[0].usedtime<pstDetectOrigin->good_time)) {   //有好的ip
        writeGoodIp(fp, pstDetectOrigin);
    } else { //没有好的ip
        if ((0==strcasecmp(pstDetectOrigin->backup, "yes")) || (0==strcasecmp(pstDetectOrigin->backup, "y"))) { //backup为yes，只写注释
            writeBackupYes(fp, pstDetectOrigin);
        } else if ((0==strcasecmp(pstDetectOrigin->backup, "no")) || (0==strcasecmp(pstDetectOrigin->backup, "n"))) {//backup为no
            writeBackupNo(fp, pstDetectOrigin);
        } else if (1 == pstDetectOrigin->backupIsIp) {   //backup是ip
            writeBackupIp(fp, pstDetectOrigin);
        } else { //backup是字符串，写CNAME记录
            writeBackupString(fp, pstDetectOrigin);
        }
    }
}

/*找到一个主机
 *输入：
 *    host----新的主机
 *返回值：
 *    NULL----数组里没有这个主机
 *    指针----新的主机的指针
 */
/* see function SearchHost() */
//struct DetectOrigin* findDetectOrigin(const char* host)
//{
//    int i;
//
//    for (i = 0; i < g_detect_task_count; ++i) {
//        if (!strcmp(host, g_detect_tasks[i]->host)) {
//            return g_detect_tasks[i];
//        }
//    }
//    return NULL;
//}

/*这行是一个主机行（anyhost文件）
 *输入：
 *    line----anyhost文件的一行
 *返回值：
 *    0----不是
 *    1----是
 */
int isHostInAnyhost(const char* line)
{
    char *str;
    char buffer[MAX_ONE_LINE_LENGTH];

    //注释，空格开头和空行
    if (
            ';' == line[0]  || 
            '#' == line[0]  || 
            ' ' == line[0]  || 
            '\t' == line[0] || 
            '\r' == line[0] || 
            '\n' == line[0]
       ) {
        return 0;
    }
    strcpy(buffer, line);
    //跳过第一个字段（域名）
    str = strtok(buffer, " \t");
    if (NULL == str) {
        return 0;
    }
    //第二个字段应该为IN
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return 0;
    }
    if (strcmp(str, "IN")) {
        return 0;
    }
    //第三个字段应该为A或者CNAME
    str = strtok(NULL, " \t");
    if (NULL == str) {
        return 0;
    }
    if (strcmp(str, "A") && strcmp(str, "CNAME")) {
        return 0;
    }

    return 1;
}

/* add by xin.yao 
 * 检查anyhost文件中是否包含固定内容 
 * 如果没有：添加该内容
 * 如果有：直接结束检查，并进入下一步流程
 */
static int checkAnyhost(FILE *fpNamed, FILE *fpNew)
{
    int fgInclude = 0;  //判断是否含有: ";------"，在其内的为保持不动的内容
    char line[MAX_ONE_LINE_LENGTH] = {0}; 
    char *str = NULL; 
    char *need_content = 
        ";-----\n"
        "$TTL 60\n"
        "$ORIGIN .\n"
        "@ IN SOA ns1. root.localhost. (\n"
        "\t20051213;\n"
        "\t7000;\n"
        "\t3000;\n"
        "\t15000;\n"
        "\t86400;\n"
        ");\n"
        "@ 86400 \tIN NS   ns1\n"
        "ns1 86400\tIN A    127.0.0.1\n"
        ";-----\n"
        ;    

    fseek(fpNamed, 0, SEEK_SET);

    while (!feof(fpNamed)) {
        str = fgets(line, MAX_ONE_LINE_LENGTH, fpNamed);
        if (NULL == str) {
            continue;
        }    
        if (!memcmp(line, ";-----", 6)) {
            if (++fgInclude >= 2) { 
                //fseek(fpNamed, 0, SEEK_SET);
                return 1;
            }    
        }    
    }    
    //fseek(fpNamed, 0, SEEK_SET);
    fseek(fpNew, 0, SEEK_SET);
    fprintf(fpNew, "%s", need_content);
    fflush(fpNew);
    return 0;
}

void modify_anyhost_timestamp()
{
    char *modify_cmd_fmt = "/bin/sed -i '/^@[ \\t]\\+IN[ \\t]\\+SOA/{n;s/[0-9]\\+/%d/}' /var/named/chroot/var/named/anyhost";
    char cmd[1024];
    time(&named_timestamp);
    snprintf(cmd, 1024, modify_cmd_fmt, (int)named_timestamp);
    system(cmd);
    return;
}

/*扫描anyhost文件（或类似文件），把需要保持不动的内容写入新文件
 *输入：
 *    info----info类型
 *    fpNamed----anyhost文件指针
 *    fpNew----新的文件指针
 */
void scanAnyhost(int info, FILE* fpNamed, FILE* fpNew)
{
    int fgInclude = 0;  //是否进入了;-----里
    int ret, have_fixed_cont;
    char* str;
    char host[128];
    char line[MAX_ONE_LINE_LENGTH];
    struct DetectOrigin* pstDetectOrigin;

    have_fixed_cont = checkAnyhost(fpNamed, fpNew);

    fseek(fpNamed, 0, SEEK_SET);

    while (!feof(fpNamed)) {
        str = fgets(line, MAX_ONE_LINE_LENGTH, fpNamed);
        if (NULL == str) {
            continue;
        }
        if (!memcmp(line, ";-----", 6)) {
            fgInclude = fgInclude ? 0 : 1;
            if (have_fixed_cont) {
                fprintf(fpNew, "%s", line);
            } else {
                fgInclude = 0;
            }
            continue;
        }

        if (fgInclude) {   //只有在;------里的才写
            ret = isHostInAnyhost(line);
            if (!ret) {   //如果不是主机，直接写
                fprintf(fpNew, "%s", line);
                continue;
            } else { //如果是主机，只有配置文件没有的才写 
                //得到主机名
                str = strchr(line, ' ');
                if (NULL == str) {
                    str = strchr(line, '\t');
                    if (NULL == str)
                        continue;
                }
                char host[128];
                memcpy(host, line, str-line);
                if ('.' == host[str-line-1]) {
                    host[str-line] = 0;
                } else {
                    host[str-line] = '.';
                    host[str-line+1] = 0;
                }
                //pstDetectOrigin = findDetectOrigin(host);
                pstDetectOrigin = SearchHost(host);
                if (NULL == pstDetectOrigin && config.model) { // && !have_recent_added) {
                    fprintf(fpNew, "%s", line);
                }
                if (NULL != pstDetectOrigin && config.model) { //&& !have_recent_added) {
                    dealwithDomainHostModelF(info, fpNew, pstDetectOrigin);
                }
                if ((NULL == pstDetectOrigin) || ((0 != atoi(pstDetectOrigin->ip->ip)) && (0 == pstDetectOrigin->modify))) {
                    fprintf(fpNew, "%s", line);
                }
                continue;
            }
        } else {
            str = strchr(line, ' ');
            if (NULL == str) {
                str = strchr(line, '\t');
                if (NULL == str) {
                    continue;
                }
            }
            memcpy(host, line, str - line);
            if ('.' == host[str - line - 1]) {
                host[str - line] = '\0';
            } else {
                host[str - line] = '.';
                host[str - line + 1] = '\0';
            }
            if (';' == host[0]) {
                memmove(host, &host[1], str - line);
            }
            //pstDetectOrigin = findDetectOrigin(host);
            pstDetectOrigin = SearchHost(host);
            if(NULL == pstDetectOrigin) {
                if (config.model) { // && !have_recent_added) {
                    fprintf(fpNew, "%s", line);
                }
                continue;
            }
            if (2 == pstDetectOrigin->flag_http_tcp) {
                fprintf(fpNew, "%s", line);
                continue;
            }
            if (NULL != pstDetectOrigin && config.model) { // && !have_recent_added) { /* no use anymore, consider to delete those code  */
                dealwithDomainHostModelF(info, fpNew, pstDetectOrigin);
            }
            continue;
        }
    }
}

/*处理侦测的主机情况
 *输入：
 *    info----info类型
 *    fpNew----新的文件指针
 */
void dealwithDomainHost(int info, FILE* fpNew)
{
    //处理侦测的主机，
    int i;

    for (i = 0;i < g_detect_task_count; i++) {
        /*
        if (have_recent_added) {
            if (!g_detect_tasks[i]->recent_added) {
                continue;
            }
        }
        */
        if (info != g_detect_tasks[i]->info) { //不属于这个文件
            continue;
        }
        //写通过的ip
        writeOneAnyhostHost(fpNew, g_detect_tasks[i]);
        ReleaseCpu();
    }
}

void dealwithDomainHostModelF(int info, FILE* fpNew,  struct DetectOrigin *pstDetectOrigin)
{
    if (info != pstDetectOrigin->info) {
        return;
    }
    //写通过的ip
    writeOneAnyhostHost(fpNew, pstDetectOrigin);
}

/*得到新的anyhost文件
 *输入：
 *    info----info类型
 *    fpNamed----原来的anyhost文件指针
 *    fpNew----新的anyhost文件指针
 *返回值：
 *    无意义
 */
int getNewAnyhost(int info, FILE* fpNamed, FILE* fpNew)
{
    //copySameIPDetectRes();
    //setUpperIpFlag();

    /*
    if (have_recent_added) {
        recentAddedWriteAnyhost(fpNamed, fpNew);
        dealwithDomainHost(info, fpNew);
        return 0;
    }
    */
    scanAnyhost(info, fpNamed, fpNew);
    if (!config.model) {
        dealwithDomainHost(info, fpNew);
    }
    return 0;
}


#include <sys/time.h>
#include <stddef.h>
#ifndef INFINITY
#define INFINITY (1E8)
#endif /* not INFINITY */
#include "detect_orig.h"
#include "shortest_path.h"


int max_fd = 0;
time_t now; 
DetectOrigin_fd *fd_table;

int copy_detect_ip(struct DetectOrigin *pstDetectOrigin,struct IpDetect* ip_src,struct IpDetect* ip_dst)
{
    int i = 0;
    for(;i< pstDetectOrigin->ipNumber;i++)
        strcpy(ip_dst[i].ip,ip_src[i].ip);
    strcpy(ip_dst[i].ip,pstDetectOrigin->backup);
    return 1;
}
//把fd_table里面的数据拷贝到原来的数组中
int fd_table_to_gtask(struct DetectOrigin* task )
{
    int task_index = task->task_index;
    int ip_index = task->ip_index;

    cclog(2,"fd_table_to_gtask:task_index=%d",task_index);
    strcpy(g_detect_tasks[task_index]->host,task->host);
    strcpy(g_detect_tasks[task_index]->filename,task->filename);

    strcpy(g_detect_tasks[task_index]->ip[ip_index].ip,task->ip[ip_index].ip);
    strcpy(g_detect_tasks[task_index]->ip[ip_index].returnCode,task->ip[ip_index].returnCode);
    g_detect_tasks[task_index]->ip[ip_index].ok = task->ip[ip_index].ok;
    if(task->usedtime>-0.0000001 && task->usedtime < 0.0000001)
        task->usedtime = conf_timer +1; //avoid this to bigger
    g_detect_tasks[task_index]->ip[ip_index].usedtime += task->usedtime;
    g_detect_tasks[task_index]->ip[ip_index].usedtime1 += task->usedtime;

    return 1;
}

int wait_for_a_while(int a)
{
    static int i = 0;
    i++ ;
    if( i < a)
    {
        return 1;
    }

    else
    {
        i = 0;
        return 0;
    }
}

int origin_detect_epoll()
{
    int ret = 0;
    int orig_detect_ok = -1;
    struct DetectOrigin * task = NULL;

    cclog(2,"g_detect_task_count=%d", g_detect_task_count);
    if(handled_task_index < g_detect_task_count){
        task = g_detect_tasks[handled_task_index];

        /* add by xin.yao: for new configuration line out immediately */
        /*
           if (have_recent_added) {
        //not new configuration will not do detection
        if (!task->recent_added) {
        //printf("host: %s\n", task->host);
        ++handled_task_index;
        goto end;
        }
        }
        */
        //printf("detect--> %s\n", task->host);
        /* end by xin.yao */

        //跳过不必测试的任务
        if (0 == task->detect) {
            //if no need to detect, skip it
            task->detected = 1;
            handled_task_index++;
            goto end;
        }

        //跳过不必测试的任务 -- 无ip
        if(0 == task->ipNumber && 0 == task->backupIsIp) {
            task->detected = 1;
            ++handled_task_index;
            goto end;
        }
        cclog(3, "do detect for %d[%s]", handled_task_index, task->host);
        if(0 == handled_task_index % 30) {
            ret = wait_for_a_while(50); //50 * 10 =500 ms
            if(ret) {
                return 0; 
            }
            cclog(0, "too many filedisciptor open,rest for a while");
        }
        orig_detect_ok = start_connect(task);
        if(-1 == orig_detect_ok) {
            cclog(3,"start_connect error happend,go next");
        } 
        ++handled_task_index;
    } else {
        cclog(2,"handled_task_index >= g_detect_task_count");
    }

end:
    return orig_detect_ok;
}

int detect_fd_check()
{
    int i;
    static time_t last_check_time = 0;

    if ( 0 == last_check_time )
    {
        last_check_time = now;
    }

    if((now - last_check_time) < 1 )
        return 0;

    for(i = 0; i <= max_fd; ++i)
    {
        /*
           if (have_recent_added) {
           if (!g_detect_tasks[fd_table[i].task_index]->recent_added) {
           continue;
           }
           }
           */
        //printf("detect_fd_check host--> %s\n", g_detect_tasks[fd_table[i].task_index]->host);
        if(1 == fd_table[i].inuse)
        {
            cclog(2,"max_fd = %d \t i = %d",max_fd, i);

            struct timeval stTimeval;
            double usedtime;

            gettimeofday(&stTimeval, NULL);
            usedtime = stTimeval.tv_sec-fd_table[i].start.tv_sec + (stTimeval.tv_usec-fd_table[i].start.tv_usec)/1000000.0;
            fd_table[i].usedtime = usedtime;

            cclog(2,"in detect_fd_check fd_table in use i= %d,fd_table[%d].usedtime = %f",i,i,usedtime);
            if ( usedtime > conf_timer)//if timeout disconnect the connection
            {
                if(fd_table[i].flag_fd_sts != FD_STS_UNCONNECT)
                {
                    fd_table_to_gtask(&fd_table[i]); 
                    free_rfc_detectOrigin(&fd_table[i]);
                    cclog(2,"disconnect fd = %d \t , domain = %s , sts =%d",fd_table[i].fd,fd_table[i].host,fd_table[i].flag_fd_sts);
                }
                //              if(fd_table[i].flag_fd_sts == FD_STS_UNCONNECT)
                disconnect_fd(i);
            }

        }
    }//end for  
    last_check_time = now;
    return 0;
}

int detect_exit_check()
{
    int i;

    for(i = 0; i <= max_fd; ++i) {
        /*
           if (have_recent_added) {
           if (!g_detect_tasks[fd_table[i].task_index]->recent_added) {
           continue;
           }   
           }
           */
        if((0 == fd_table[i].inuse) && (max_fd == i) && (handled_task_index >= g_detect_task_count)) {
            return 1;
        } else if (1 == fd_table[i].inuse) {
            break;
        } else {
            continue;
        }
    }
    return 0;
}

#ifndef FRAMEWORK
int detect_core_loop_epoll(void)
{
    while (1) {
        now = time(NULL);
        int orig_detect_ok = 0;

        getCurrentTime();
        // origin web server detect
        orig_detect_ok = origin_detect_epoll();

        eventRun();

        detect_epoll_wait(handle_read_event, handle_write_event); /* read data from net */

        detect_fd_check();                      /* write time to struct */

        cclog(2,"detect_exit_check =%d",detect_exit_check());
        if(1 == detect_exit_check())            /* check whether something more to be check */
        {
            break;
        }
    }
    return 0;
}
int detect(double timer1)
{
    //param prepare
    conf_timer = timer1;

    detect_core_loop_epoll();

    detect_epoll_dest();

    return 0;
}
#else /* not FRAMEWORK */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  GetUsedTimeFromShortestPath
 *  Description:
 * =====================================================================================
 */
    static double
GetUsedTimeFromShortestPath( struct IpDetect *ip_detect, const struct DetectOrigin *detect_origin )
{
    double min_used_time;
    int i;
    int total;
    RoutePath *path;
    RoutePath path_info;
    assert( ip_detect );
    assert( detect_origin );
    min_used_time = INFINITY;
    inet_pton( AF_INET, ip_detect->ip, (void*)&path_info.ip_from );
    total = detect_origin->final_ip_number;
    for( i = 0; i < total; ++i )
    {
        double t;
        path_info.ip_to = detect_origin->final_ip[i];
        path_info.ip_to_port = detect_origin->final_ip_port[i];
        if( path_info.ip_from == path_info.ip_to )
        {
            ip_detect->final_ip = path_info.ip_to;
            ip_detect->final_ip_port = path_info.ip_to_port;
            return 0.0;
        }
        //RoutePath* ShortestPathGetRecord( const RoutePath *path );
        path = ShortestPathGetRecord( &path_info );
        //path = HashSearch( route_table, &path_info );
        /* 如果有对应的记录 */
        if( path != NULL )
        {
            Debug( 50, "Fc find path, from %X to %X:%X, %lfs(min %lfs)",
                    path->ip_from,
                    path->ip_to,
                    path->ip_to_port,
                    path->state.used_time,
                    min_used_time );
            t = path->state.used_time;
            if( t < min_used_time )
            {
                ip_detect->final_ip = path->ip_to;
                ip_detect->final_ip_port = path->ip_to_port;
                min_used_time = t;
            }
        }
    }
    return min_used_time;
} /* - - - - - - - end of function GetUsedTimeFromShortestPath - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  CompareUsedTime
 *  Description:  Need to used this function to sort data from worst to best.
 * =====================================================================================
 */
    static int
CompareUsedTime( const void *prev, const void *next )
{
    double *p = (double*)prev;
    double *n = (double*)next;
    assert( p );
    assert( n );
    if( !isnormal(*p) )
        return 0;
    if( !isnormal(*n) )
        return 1;
    return *p < *n;
} /* - - - - - - - end of function CompareUsedTime - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  CalculateUsedTime
 *  Description:
 * =====================================================================================
 */
    int
CalculateUsedTime( void )
{
    int i, j, k;
    double average;
    for( i = 0; i < g_detect_task_count; ++i )
    {
        struct DetectOrigin *detect_origin = g_detect_tasks[i];
        Debug( 50, "%s has %d ip, %d backip.",
                detect_origin->host,
                g_detect_tasks[i]->ipNumber,
                g_detect_tasks[i]->nbkip );
        for( j = 0; j < g_detect_tasks[i]->ipNumber + g_detect_tasks[i]->nbkip; ++j )
        {
            struct IpDetect *ip = g_detect_tasks[i]->ip + j;
            Debug( 50, "start qsort for %s %s",
                    detect_origin->host,
                    ip->ip );
            qsort( ip->used_time, ip->used_time_count, sizeof(double), CompareUsedTime );
            average = 0.0;
            if( ip->used_time_count == 0 )
            {
                Debug( 40, "No link data. Ignore %s %s.",
                        detect_origin->host,
                        ip->ip);
                continue;
            }
            else if( ip->used_time_count <= kFC_LINK_DATA_IGNORE_COUNT )
            {
                Debug( 40, "Not enough link data. Use latest one instead of average for %s %s.",
                        detect_origin->host,
                        ip->ip);
                average = ip->used_time[ip->used_time_count-1];
            }
            else if( ip->used_time_count > kFC_LINK_DATA_READ_COUNT_MAX )
            {
                Debug( 20, "Array subscript is above array bounds in %s %s!",
                        detect_origin->host,
                        ip->ip);
            }
            else
            {
                Debug( 50, "Domain %s, ip %s, used_time_count %d",
                        detect_origin->host,
                        ip->ip,
                        ip->used_time_count );
                for( k = ip->used_time_count - 1; k >= kFC_LINK_DATA_IGNORE_COUNT; --k )
                {
                    Debug( 50, "Domain %s, ip %s, used_time No.%d %lf",
                            detect_origin->host,
                            ip->ip,
                            k,
                            ip->used_time[k] );
                    average += ip->used_time[k];
                }
                average /= ip->used_time_count - kFC_LINK_DATA_IGNORE_COUNT;
            }
            ip->usedtime = average;
#ifdef ROUTE
            if( detect_origin->final_ip_number )
                ip->usedtime += GetUsedTimeFromShortestPath( ip, detect_origin );
#endif /* ROUTE */
            /* TODO: 算完了均值了，接下来可以测试结果是不是正确，然后让模块去修改读取的文件个数 */
        }
    }
    return 0;
} /* - - - - - - - end of function CalculateUsedTime - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  DetectSaveData
 *  Description:  
 * =====================================================================================
 */
    static int
DetectSaveData( const void *data )
{
    int j;
    DataInfo *input_data = (DataInfo*)data;
    char ip[16];
    unsigned long ip_to;
    char code[4];
    struct DetectOrigin *detect_origin;
    struct IpDetect *ip_detect = NULL;

    if( input_data->file_order >= kFC_LINK_DATA_READ_COUNT_MAX )
        return 0;
    ip_to = input_data->ip_to;
    if( inet_ntop( AF_INET, (const void*)&ip_to, ip, 16 ) == NULL )
        return 1;
    snprintf( code, sizeof(code), "%03u", input_data->http_status );
    Debug( 50, "Get input data with hostname %s, ip %s.", input_data->hostname, ip );
    detect_origin = SearchHost( input_data->hostname );
    if( detect_origin != NULL )
    {
        Debug( 50, "Find channel \"%s\".", detect_origin->host );
        /* 输入的数据表明ip来自dns */
        if( input_data->ip_type == kIP_TYPE_DNS_IP )
        {
            /* domain.conf认为ip应该去dns取 */
            if( detect_origin->origDomain )
            {
                /* 尚不曾将ip写入该频道 */
                if( strcmp( detect_origin->ip[0].ip, "0.0.0.0" ) == 0 ) {
                    strncpy( detect_origin->ip[0].ip, ip, 16 );
                    Debug( 50, "write %s to ip[0]", ip );
                }
                else
                {
                    detect_origin->ip = (struct IpDetect*)realloc( (void*)detect_origin->ip,
                            ( detect_origin->ipNumber + detect_origin->nbkip + 1 ) * sizeof(struct IpDetect) );
                    if( detect_origin->nbkip )
                        memmove( detect_origin->ip + detect_origin->ipNumber + 1,
                                detect_origin->ip + detect_origin->ipNumber, detect_origin->nbkip * sizeof(struct IpDetect) );
                    memset( detect_origin->ip + detect_origin->ipNumber, 0, sizeof(struct IpDetect) );
                    strncpy( detect_origin->ip[detect_origin->ipNumber].ip, ip, 16 );
                    Debug( 50, "write %s to ip[%d]", ip, detect_origin->ipNumber );
                    ++detect_origin->ipNumber;
                }
                ip_detect = detect_origin->ip + detect_origin->ipNumber - 1;
            }
            else
            {
                Debug( 20, "Input data says that %s uses DNS ip, but domain.conf does not think so.", input_data->hostname );
                return 1;
            }
        }
        else
        {
            for (j=0;j<detect_origin->ipNumber+detect_origin->nbkip;j++)
            {
                /* 没错，就是这个ip，恭喜你找到了 */
                if( 0 == detect_origin->flag_http_tcp && strncmp( detect_origin->ip[j].ip, ip, 16 ) == 0 )
                {
                    ip_detect = detect_origin->ip + j;
                    break;
                }
            }
        }
        if( ip_detect != NULL )
        {
            ip_detect->ok = 0;
            /* http探测的判定规则 */
            if( (input_data->detect_type == kDETECT_TYPE_HTTP ||
                 input_data->detect_type == kDETECT_TYPE_HTTPS
                        )
                    && code[0] != 0 )
            {
                if( inCode( code, detect_origin->code ) == 1 )
                    ip_detect->ok = 1;
            }
            /* tcp探测的判定规则 */
            else if( input_data->detect_type == kDETECT_TYPE_TCP )
            {
                if( input_data->connect_time > 0 || input_data->port_status != kPORT_DOWN )
                    ip_detect->ok = 1;
            }
            else
            {
                Debug( 30, "Unknown detect error for %s. Neither tcp detect, nor http detect with un-zero return code", input_data->hostname );
                return 1;
            }
            ip_detect->usedtime = input_data->detect_used_time;
            if( input_data->port_status == kPORT_DOWN )
                ip_detect->used_time[ip_detect->used_time_count] = INFINITY;
            else
                ip_detect->used_time[ip_detect->used_time_count] = input_data->detect_used_time;
            ++ip_detect->used_time_count;
            //#ifdef ROUTE
            //            if( detect_origin->final_ip_number )
            //                ip_detect->usedtime += GetUsedTimeFromShortestPath( ip_detect, detect_origin );
            //#endif /* ROUTE */
            sprintf( ip_detect->returnCode, "%03u", input_data->http_status );
            ResultLogAdd( detect_origin, ip_detect );
        }
    }
    else {
        Debug( 50, "host %s is not for FC", input_data->hostname );
    }
    return 0;
} /* - - - - - - - end of function DetectSaveData - - - - - - - - - - - */
int detect_core_loop_epoll(const void *data)
{
    now = time(NULL);

    getCurrentTime();
    // origin web server detect
    //    orig_detect_ok = origin_detect_epoll();

    eventRun();

    /* add 'const void *data' here */

    /* 从表中寻找data中的ip和端口 */
    /* 将data中的数据写入到对应的表中 */
    DetectSaveData( data );
    return 0;
}
#endif /* not FRAMEWORK */

void dump_task( struct DetectOrigin* pstDetectOrigin)
{
    cclog(0,"_____________dump start---");
    int i = 0;
    for(i=0;i<pstDetectOrigin->ipNumber;i++)
    {
        cclog(0,"pstDetectOrigin->ip[%d].ip=%s",i,pstDetectOrigin->ip[i].ip);
        cclog(0,"pstDetectOrigin->ip[%d].returnCode=%s",i,pstDetectOrigin->ip[i].returnCode);
        cclog(0,"pstDetectOrigin->ip[%d].ok=%d",i,pstDetectOrigin->ip[i].ok);
    }
    cclog(0,"pstDetectOrigin->fd = %d",pstDetectOrigin->fd);
    cclog(0,"pstDetectOrigin->host = %s",pstDetectOrigin->host);
    cclog(0,"pstDetectOrigin->times = %d",pstDetectOrigin->times);
    cclog(0,"pstDetectOrigin->filename = %s",pstDetectOrigin->filename);
    cclog(0,"pstDetectOrigin->warningtime = %f",pstDetectOrigin->warning_time);
    cclog(0,"pstDetectOrigin->good_time = %f",pstDetectOrigin->good_time);
    cclog(0,"pstDetectOrigin->backup = %s",pstDetectOrigin->backup);
    cclog(0,"pstDetectOrigin->backupIsIp = %d",pstDetectOrigin->backupIsIp);
    cclog(0,"pstDetectOrigin->start->tv_sec = %ld",pstDetectOrigin->start.tv_sec);
    cclog(0,"pstDetectOrigin->start->tv_usec = %ld",pstDetectOrigin->start.tv_usec/1000000);
    cclog(0,"pstDetectOrigin->end->tv_sec = %ld",pstDetectOrigin->end.tv_sec);
    cclog(0,"pstDetectOrigin->end->tv_sec = %ld",pstDetectOrigin->end.tv_usec/1000000);
    cclog(0,"pstDetectOrigin->usedtime = %ld",pstDetectOrigin->usedtime);
    cclog(0,"pstDetectOrigin->inuse = %d",pstDetectOrigin->inuse);
    cclog(2,"_____________dump end---");

}

static int set_no_blocking(int fd) 
{
    int flags;
    int dummy = 0;

    if ((flags = fcntl(fd, F_GETFL, dummy)) < 0) {
        cclog(1, "set_no_blocking: %d fcntl F_GETFL: %s", fd, strerror(errno));
        return -1; 
    }   

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        cclog(1, "set_no_blocking: FD %d: %s", fd, strerror(errno));
        return -1; 
    }   
    return 1;
}


static int safe_connect(int sockfd, const struct sockaddr *address, int address_len)
{
    int n = 0;
    if( (n = connect(sockfd , address, address_len)) < 0) {
        if( errno != EINPROGRESS ) {
            close(sockfd);
            return -1;
        }       
    }
    cclog(8,"IP[%s] sockfd[%d]",inet_ntoa(((struct sockaddr_in*)address)->sin_addr),sockfd);
    return 1;
}

#if 0
static int safe_connect(int sockfd, const struct sockaddr *address, int address_len)
{

    int retry_times = 0;
    int ret = 0;

    while(retry_times < 3)
    {
        ret = connect(sockfd, address, address_len);

        if(ret == -1)
        {
            usleep(1000000);
            retry_times++;      
            continue;
        }   
        cclog(8,"IP[%s] sockfd[%d] ret[%2d]",inet_ntoa(((struct sockaddr_in*)address)->sin_addr),sockfd,ret);
        return 1;

    }
    return -1;
}
#endif

int  write_socket(int socket_fd, void *write_buf, size_t write_len)
{
    unsigned char *des = (unsigned char*)write_buf;
    size_t nwrite = 0;

    nwrite = write(socket_fd, des, write_len);

    if(nwrite < 0) {
        if( errno == EINTR )
        {
            cclog(2,"errno == EINTR");
            return 0;   // ok ,wait next
        }
        if(errno == EAGAIN)
        {
            cclog(2,"errno == EAGAIN");
            return 0;   // ok ,wait next
        }
        else
            return -1;//peer close
    } 
    cclog(2,"nwrite = %d ",nwrite);


    return nwrite;
}

ssize_t read_socket(int socket_fd, void *read_buff, size_t read_len)
{
    ssize_t nread = 0;
    unsigned char*  des = (unsigned char*)read_buff;

    nread = read(socket_fd, des, read_len);
    if (nread < 0) {
        if( errno == EINTR )
        {
            cclog(2,"errno == EINTR");
            return 1;
        }
        if(errno == EAGAIN)
        {
            cclog(2,"errno == EAGAIN");
            return 1;
        }
        else
            return -1;
    }

    //0 or size of read
    return nread;
}

void set_tcp_result_not_connected(int fd)
{
    struct DetectOrigin *rfc = &fd_table[fd];
    int ip_index = rfc->ip_index;
    struct timeval sttimeval2;
    gettimeofday(&sttimeval2, NULL);
    double usedtime = sttimeval2.tv_sec-rfc->start.tv_sec + (sttimeval2.tv_usec-rfc->start.tv_usec)/1000000.0;

    rfc->ip[ip_index].ok = 0;
    rfc->inuse = 0;
    rfc->usedtime = usedtime;
    fd_table_to_gtask(rfc);
    close(fd);

}

void set_tcp_result_connected(int fd)
{
    struct DetectOrigin *rfc = &fd_table[fd];
    int ip_index = rfc->ip_index;
    struct timeval sttimeval2;
    gettimeofday(&sttimeval2, NULL);
    double usedtime = sttimeval2.tv_sec-rfc->start.tv_sec + (sttimeval2.tv_usec-rfc->start.tv_usec)/1000000.0;

    rfc->ip[ip_index].ok = 1;
    rfc->inuse = 0;
    rfc->usedtime = usedtime;
    fd_table_to_gtask(rfc);
    close(fd);

}

static void  initilize_write_buf(int fd)
{
    char g_szBuffer[MAX_BUF_SIZE];
    char host[100];
    char url[100];
    char header[100];
    const char* method[] = {"GET","POST" ,"HEAD"};
    int method1 , method2, i;

    memset(g_szBuffer,0,MAX_BUF_SIZE);
    memset(host,0,100);

    DetectOrigin_fd *rfc = &fd_table[fd];

    method1 = rfc->method;
    method2 = rfc->method2;

    if(0 == rfc->flag_refer)    
    {
        strncpy(host,rfc->rHost,sizeof(host));
        cclog(3,"in initilize_write_buf rfc->rHost=%s %d",rfc->rHost,sizeof(rfc->rHost));
        strcpy(url,rfc->filename);
        snprintf(header,100,"Connection: close");
        snprintf(g_szBuffer,MAX_BUF_SIZE,"%s %s HTTP/1.1\r\nHost: %s\r\nUser-Agent:ChinaCache\r\nAccpet: */*\r\n%s\r\n\r\n",method[method1-1],url,host,header);
    }
    else if(1 == rfc->flag_refer)
    {
        strncpy(host,rfc->rHost2,sizeof(host));
        cclog(3,"in initilize_write_buf rfc->host2=%s %d",rfc->rHost2,sizeof(rfc->rHost2));
        strcpy(url,rfc->filename2);
        snprintf(header,100,"Connection: close");
        //add by xin.yao for custom header: 2012-1-9
        snprintf(g_szBuffer, 
                MAX_BUF_SIZE,
                "%s %s HTTP/1.1\r\nHost: %s\r\nUser-Agent:ChinaCache\r\nAccpet: */*\r\n%s\r\n",
                method[method2-1], 
                url, 
                rfc->host, 
                header);
        int flag = 0;
        if (NULL != custom_cfg.hdrs) {
            for (i = 0; i < custom_cfg.hdr_count; ++i) {
                if (!strcmp(rfc->host, custom_cfg.hdrs[i].host)) {
                    snprintf((char *)g_szBuffer + strlen(g_szBuffer), 
                            MAX_BUF_SIZE,
                            "%s\r\n",
                            custom_cfg.hdrs[i].header);
                    if (!strncmp(custom_cfg.hdrs[i].hdr_name, "Referer", 7)) {
                        flag = 1;
                    }
                }
            }   
        }
        if (!flag) {
            snprintf((char *)g_szBuffer + strlen(g_szBuffer), 
                    MAX_BUF_SIZE,
                    "Referer: http://%s/\r\n",
                    host);
        }
        strcat(g_szBuffer, "\r\n");
        //printf("%s\n", g_szBuffer);
        //end by xin.yao
    }

    strcpy(rfc->current_write_buf,g_szBuffer);
    rfc->need_send_len = strlen(rfc->current_write_buf);
    cclog(6,"g_szBuffer = %s",rfc->current_write_buf);

}

int detect_fd_init(int fd,struct DetectOrigin* pstDetectOrigin,int ip_index,int flag_refer)
{
    if(fd > FD_MAX){
        cclog(0,"fatal ! fd(%d) > FD_MAX(%d) !!",fd,FD_MAX);
        assert(fd < FD_MAX);
    }   
    struct DetectOrigin *rfc = &fd_table[fd];

    cclog(5,"in detect_fd_init  fd = %d ",fd);

    rfc->fd = fd;
    rfc->flag_fd_sts = FD_STS_UNCONNECT; 
    rfc->detect = pstDetectOrigin->detect;
    //rfc->recent_added = pstDetectOrigin->recent_added;
    rfc->inuse = 1;

    strncpy(rfc->host,pstDetectOrigin->host,128);
    strcpy(rfc->filename,pstDetectOrigin->filename);
    strcpy(rfc->rHost,pstDetectOrigin->rHost);
    strcpy(rfc->filename2,pstDetectOrigin->filename2);
    strcpy(rfc->rHost2,pstDetectOrigin->rHost2);
    strcpy(rfc->backup,pstDetectOrigin->backup);
    strcpy(rfc->rtn_code,pstDetectOrigin->rtn_code);

    rfc->warning_time = pstDetectOrigin->warning_time;
    rfc->good_time = pstDetectOrigin->good_time;
    rfc->ipNumber = pstDetectOrigin->ipNumber;
    rfc->times = pstDetectOrigin->times;
    rfc->method = pstDetectOrigin->method;
    rfc->method2 = pstDetectOrigin->method2;
    rfc->ip_index = ip_index;
    rfc->task_index = handled_task_index;
    rfc->flag_refer = flag_refer;



    struct timeval stTimeval;
    gettimeofday(&stTimeval, NULL);
    rfc->start = stTimeval;

    struct IpDetect* ip;
    ip = (struct IpDetect*)cc_malloc((pstDetectOrigin->ipNumber+pstDetectOrigin->nbkip)*sizeof(struct IpDetect));
    if (NULL == ip)
    {
        cclog(0,"malloc error");
        return -1;
    }
    memset(ip, 0, (pstDetectOrigin->ipNumber+pstDetectOrigin->nbkip)*sizeof(struct IpDetect));
    rfc->ip = ip;

    if(pstDetectOrigin->backupIsIp)
        //      strcpy(pstDetectOrigin->ip[pstDetectOrigin->ipNumber].ip,pstDetectOrigin->backup);
        make_new_ip(pstDetectOrigin);

    int i=0;
    for (i=0;i<pstDetectOrigin->ipNumber + pstDetectOrigin->nbkip;i++)
    {
        strcpy(rfc->ip[i].ip,pstDetectOrigin->ip[i].ip);
    }

    getCode(rfc->rtn_code, &(rfc->code));

    if(0 != pstDetectOrigin->flag_http_tcp) //因为只有HTTP探测需要发送数据和接受数据的BUFFER
        return 0;

    rfc->read_len = 0; 
    rfc->need_read_len = MAX_BUF_SIZE;
    rfc->current_read_buf =(char *)cc_malloc(MAX_BUF_SIZE + 1);
    if( NULL == rfc->current_read_buf)
    {
        cclog(0,"malloc error");
        return -1;

    }
    memset(rfc->current_read_buf,0,MAX_BUF_SIZE + 1 );

    rfc->send_len = 0;
    rfc->need_send_len = MAX_BUF_SIZE;
    rfc->current_write_buf =(char *)cc_malloc(MAX_BUF_SIZE);
    if( NULL == rfc->current_write_buf )
    {
        cclog(0,"malloc error");
        return -1;

    }
    memset(rfc->current_write_buf,0,MAX_BUF_SIZE);

    initilize_write_buf(fd);

    return 0;
}

int get_max_fd(int fd)
{
    return fd > max_fd ? fd:max_fd;
}

int connect_addr(int sockfd, struct sockaddr *address)
{

    int ret = 0;
    ret = connect(sockfd, address, sizeof(*address));
    cclog(0,"connect_addr:ip[%s] sockfd[%d] ret[%2d] error[%s]",inet_ntoa(((struct sockaddr_in*)address)->sin_addr),sockfd,ret,strerror(errno));

    if(ret == -1 && errno == EINPROGRESS )
        ret =  CON_INPROGRESS; 
    else if(ret == -1)
        ret = CON_ERROR;
    else if( ret == 0 )
        ret = CON_OK;   
    else
        ret = CON_ERROR;


    return ret;

}

void commReconnect(void *data)
{
    int ret ;
    ConnectStateData *cs = data;
    cs->tries ++;
    ret = connect_addr(cs->fd, (struct sockaddr*)&cs->S);
    cclog(1,"commReconnect cs->tries [%d],ret[%d]",cs->tries,ret);

    if(ret!=CON_OK && cs->tries < 3)
        eventAdd("commReconnect", commReconnect,cs, 0.8, 0);
    else if(ret == CON_OK)
    {
        cs->callback(cs->fd);
        cc_free(cs);
    }
    else
    {
        cs->callback_no(cs->fd);
        cc_free(cs);
    }

}

int handle_detect_connect(int fd, void *data)
{

    int ret = -1;

    ConnectStateData *cs = data;

    switch (connect_addr(cs->fd, (struct sockaddr*)&cs->S)) 
    { 
        case CON_INPROGRESS: 
            cclog(0,"handle_detect_connect: fd %d: inprogress", fd); 
            eventAdd("commReconnect", commReconnect,cs, 0.5, 0);

            break; 
        case CON_OK: 
            cs->callback(cs->fd);
            cc_free(cs);
            cclog(0,"handle_detect_connect: connect ok");
            ret = 0;
            return ret;
        default: 
            cclog(0,"handle_detect_connect: fd %d: error", fd); 
            eventAdd("commReconnect", commReconnect,cs, 0.5, 0);
            break;
    }

    return ret;
}

/*  return value:
 *  0: ok
 *  -1: error occur
 */
int start_connect(struct DetectOrigin* pstDetectOrigin)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    int detectwithtags = 0;
    int sockfd;

    //pstDetectOrigin->detect_time = 5;
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port =ntohs(pstDetectOrigin->port);

    if((0 == pstDetectOrigin->detect_time % pstDetectOrigin->http_detect_time)) { // || (pstDetectOrigin->recent_added == 1)) {
        //cclog(1,"host[%s] http detect start! recent_Added [%d]",pstDetectOrigin->host,pstDetectOrigin->recent_added);
        cclog(1,"host[%s] http detect start!",pstDetectOrigin->host);
        pstDetectOrigin->flag_http_tcp = 0;
        while(j < pstDetectOrigin->times*2) {
            if( pstDetectOrigin->weight0 != 0 && j < pstDetectOrigin->times) {
                cclog(2,"start_connect pstDetectOrigin times_index = %d,pstDetectOrigin->times=%d",j,pstDetectOrigin->times);
                for(i = 0; i < pstDetectOrigin->ipNumber + pstDetectOrigin->nbkip; i++) {
                    /* add by xin.yao: 2011-12-19 */
                    if (pstDetectOrigin->ip[i].reuse_nodetect_flag) {
                        //printf(">>>>>>>>>>>>>> %s\n", pstDetectOrigin->ip[i].ip);
                        continue;   
                    }
                    /* end by xin.yao: 2011-12-19 */
                    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) <= 0 ) {
                        if (EMFILE == errno || ENFILE == errno) {
                            cclog(0, "already reached the file descriptors upper limit");
                            return -1;
                        } 
                        cclog(0, "socket creat error,host:[%s],ip [%s],err: [%s]",pstDetectOrigin->host,pstDetectOrigin->ip[i].ip, strerror(errno));
                        continue;
                    } 
                    max_fd = get_max_fd(sockfd);
                    if(inet_aton(pstDetectOrigin->ip[i].ip,&servaddr.sin_addr) == 0){ 
                        cclog(0,"inet_aton error,host:[%s],ip [%s]",pstDetectOrigin->host,pstDetectOrigin->ip[i].ip);
                        ret = -1;
                        continue;
                    }   
                    set_no_blocking(sockfd);
                    if( (ret = safe_connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr))) < 0 ) {
                        cclog(0,"connect error,host:[%s],ip [%s]",pstDetectOrigin->host,pstDetectOrigin->ip[i].ip);
                        ret = -1;
                        continue;
                        //although connect error,we must put it into epoll,otherwise,this will not be detected
                        //the best way is let epoll manage connect
                    }
                    detect_epoll_add(sockfd,EPOLLOUT|EPOLLHUP|EPOLLERR|EPOLLIN);
                    cclog(3,"connect success");
                    detect_fd_init(sockfd,pstDetectOrigin,i,detectwithtags);
                }
                ++j;
            } else if( pstDetectOrigin->weight1 != 0 && j < pstDetectOrigin->times*2 && j >= pstDetectOrigin->times) {
                cclog(2,"with refer start_connect pstDetectOrigin times_index = %d",j);
                detectwithtags = 1;
                for(i=0;i<pstDetectOrigin->ipNumber+pstDetectOrigin->nbkip;i++)
                {
                    /* add by xin.yao: 2011-12-19 */
                    if (pstDetectOrigin->ip[i].reuse_nodetect_flag) {
                        continue;   
                    }
                    /* end by xin.yao: 2011-12-19 */
                    if( ( sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
                    {
                        if (EMFILE == errno || ENFILE == errno) {
                            cclog(0, "already reached the file descriptors upper limit");
                            return -1; 
                        }  
                        cclog(0,"socket creat error,host:[%s],ip [%s],err: %s",pstDetectOrigin->host,pstDetectOrigin->ip[i].ip, strerror(errno));
                        ret = -1;
                        continue;
                    }
                    max_fd = get_max_fd(sockfd);

                    if(inet_aton(pstDetectOrigin->ip[i].ip,&servaddr.sin_addr) == 0){ 
                        cclog(0,"inet_aton error,host:[%s],ip [%s]",pstDetectOrigin->host,pstDetectOrigin->ip[i].ip);
                        ret = -1;
                        continue;
                    }   
                    set_no_blocking(sockfd);

                    if( (ret = safe_connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr))) < 0 ) {
                        cclog(0,"connect error,host:[%s],ip [%s]",pstDetectOrigin->host,pstDetectOrigin->ip[i].ip);
                        ret = -1;
                        //although connect error,we must put it into epoll,otherwise,this will not be detected
                        //the best way is let epoll manage connect
                        continue;
                    }
                    detect_epoll_add(sockfd,EPOLLOUT | EPOLLHUP | EPOLLERR);
                    cclog(2,"connect success");
                    detect_fd_init(sockfd,pstDetectOrigin,i,detectwithtags);
                }
                j++;
            } else {
                j++;
            }
        }
    } else if(0 == pstDetectOrigin->detect_time % pstDetectOrigin->tcp_detect_time) {
        cclog(1,"host[%s] tcp detect start!",pstDetectOrigin->host);
        pstDetectOrigin->flag_http_tcp = 1;
        for(i=0;i<pstDetectOrigin->ipNumber+pstDetectOrigin->nbkip;i++) {
            /* add by xin.yao: 2011-12-19 */
            if (pstDetectOrigin->ip[i].reuse_nodetect_flag) {
                continue;   
            }
            /* end by xin.yao: 2011-12-19 */
            if(( sockfd = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
            {
                if (EMFILE == errno || ENFILE == errno) {
                    cclog(0, "already reached the file descriptors upper limit");
                    return -1; 
                }  
                cclog(0,"socket creat error,host:[%s],ip [%s],err: [%s]",pstDetectOrigin->host,pstDetectOrigin->ip[i].ip, strerror(errno));
                ret = -1;
                set_tcp_result_not_connected(sockfd);
                continue;
            }
            max_fd = get_max_fd(sockfd);
            if(inet_aton(pstDetectOrigin->ip[i].ip,&servaddr.sin_addr) == 0)
            { 
                cclog(0,"inet_aton error,host:[%s],ip [%s]",pstDetectOrigin->host,pstDetectOrigin->ip[i].ip);
                ret = -1;
                set_tcp_result_not_connected(sockfd);
                continue;
            } 
            set_no_blocking(sockfd);   //here we must set fd to no blocking
            struct timeval sttimeval1; 
            gettimeofday(&sttimeval1, NULL);
            pstDetectOrigin->start = sttimeval1;
            detect_fd_init(sockfd,pstDetectOrigin,i,detectwithtags);
            ConnectStateData *cs = cc_malloc(sizeof(ConnectStateData)); //remember to free
            cs->fd = sockfd ;
            cs->in_addr = servaddr.sin_addr;
            cs->S = servaddr; 
            cs->tries = 0;
            cs->callback = (CNCB *)set_tcp_result_connected ;
            cs->callback_no = (CNCB *)set_tcp_result_not_connected;
            cs->data = sockfd ;
            handle_detect_connect(sockfd,cs);
        }
    } else {
        cclog(1,"host[%s] no detect start!",pstDetectOrigin->host);
        pstDetectOrigin->flag_http_tcp = 2; //indicate this time keep old ones
    }
    return ret;
    }


    bool is_socket_alive(int sock_fd){
        int err = 0;
        errno = 0;
        socklen_t e_len = sizeof(err);
        int x = getsockopt(sock_fd,SOL_SOCKET,SO_ERROR,&err,&e_len);
        if(x == 0)
            errno = err;

        if (errno == 0 || errno == EISCONN)
            return true;;

        return false;
    } 

    int handle_write_event(int fd)
    {

        DetectOrigin_fd *rfc = &fd_table[fd];

        if((rfc->flag_fd_sts == FD_STS_UNCONNECT) && !is_socket_alive(fd))
        {
            cclog(0,"connnect error && socket[%d] not alive",fd);
            //  close(fd);
            return -1;

        }


        int write_len = 0 ;
        int ret = 0 ;

        rfc->flag_fd_sts = FD_STS_READY_SEND; 

        write_len = write_socket(fd,rfc->current_write_buf + rfc->send_len,rfc->need_send_len - rfc->send_len);
        cclog(6,"rfc->current_write_buf = %s\n,write_len =%d",rfc->current_write_buf,write_len);

        if(write_len < 0 ){
            ret = -1; //disconnect 
        }
        else if(write_len == 0){
            ret = -1;   // ok
        }
        else if (write_len == rfc->need_send_len)
        {
            rfc->send_len += write_len;
            rfc->flag_fd_sts = FD_STS_HAVE_SENDED; 
            ret = 1;
        }
        else
        {
            rfc->send_len += write_len;
            ret = 0 ;
        }
        cclog(6,"rfc->send_len = %d\t,rfc->need_send_len = %d ",rfc->send_len,rfc->need_send_len);

        return ret;
    }


    char * check_http_header_end( char *buf)
    {
        return strstr(buf,"\r\n\r\n");

    }

    int handle_read_event(int fd)
    {
        DetectOrigin_fd *rfc = &fd_table[fd];
        int read_len = 0 ;
        int ret = 0 ;

        if((rfc->flag_fd_sts == FD_STS_UNCONNECT) && !is_socket_alive(fd))
        {
            cclog(0,"connnect error && socket[%d] not alive && host[%s]",fd,rfc->host);
            //  close(fd);
            return -1;

        }

        rfc->flag_fd_sts = FD_STS_READY_RCV; 

        read_len = read_socket(fd,rfc->current_read_buf + rfc->read_len,rfc->need_read_len - rfc->read_len);

        if(read_len < 0 ){
            ret = -1; //disconnect 
        }
        else if(read_len == 0){
            ret = 1;    // ok
        }
        else if(check_http_header_end(rfc->current_read_buf))
        {
            rfc->read_len += read_len;
            rfc->flag_fd_sts = FD_STS_HAVE_RCVED; 
            ret = 1;
        } 
        else if (read_len == rfc->need_read_len)
        {
            rfc->read_len += read_len;
            rfc->flag_fd_sts = FD_STS_HAVE_RCVED; 
            ret = 1;
        }
        else
        {
            rfc->read_len += read_len;
            ret = 0 ;
        }
        cclog(6,"rfc->read_len = %d\t,rfc->need_read_len = %d ",rfc->read_len,rfc->need_read_len);

        return ret;
    }

    int after_handle_read_event(int fd)
    {
        //assert(fd != 0 );

        struct DetectOrigin *rfc = &fd_table[fd];

        struct timeval sttimeval;
        double usedtime;


        gettimeofday(&sttimeval, NULL);

        rfc->end  = sttimeval;
        usedtime = sttimeval.tv_sec-rfc->start.tv_sec + (sttimeval.tv_usec-rfc->start.tv_usec)/1000000.0;
        rfc->usedtime = usedtime;

        cclog(3,"usedtime is %f,flag_fd_sts = %d",usedtime,rfc->flag_fd_sts);

        char read_buffer[MAX_BUF_SIZE] ;
        strncpy(read_buffer,rfc->current_read_buf,rfc->read_len);
        cclog(6,"fd = %d, host = %s,read_buffer is %s\n %s,rfc->read_len is =%d ",rfc->fd,rfc->host,read_buffer,rfc->current_read_buf,rfc->read_len);

        char* p_read = NULL;
        p_read = strchr(read_buffer, '\n');                         
        if(NULL == p_read) {                                             
            cclog(0,"no data read from the socket %d,domain= %s,ip= %s",fd,rfc->host,rfc->ip[rfc->ip_index].ip);
            return -1;                                                
        }                                                             
        p_read++;                                                        
        char responsefirstline[MAX_BUF_SIZE];
        memset(responsefirstline,0,MAX_BUF_SIZE);

        memcpy(responsefirstline, read_buffer, p_read-read_buffer);    
        responsefirstline[p_read-read_buffer] = 0;                    

        cclog(2,"responsefirstline is %s ",responsefirstline);

        char onecode[4];
        int ret = 0;
        onecode[0] = 0;

        char* str = responsefirstline;
        if (NULL != str) {
            ret = strncasecmp(str, "http/1.", 7);
            if (0 == ret) {
                str = strchr(str, ' ');
                if (NULL != str) {
                    str++;
                    while (' ' == *str)
                        str++;
                    memcpy(onecode, str, 3);
                    onecode[3] = 0;
                }
            }
        }

        struct IpDetect* pstIpDetect;
        int ip_index = rfc->ip_index;

        int tags = rfc->flag_refer;
        pstIpDetect = &(rfc->ip[ip_index]);

        if (0!=onecode[0]) {
            strncpy(pstIpDetect->returnCode,onecode,4);
            ret = inCode(onecode, rfc->code);
            if (1 == ret)
                pstIpDetect->ok = 1;
        }
        else
        {
            pstIpDetect->ok = 0;
        }

        if (tags)
            pstIpDetect->usedtime1 += usedtime;
        else
            pstIpDetect->usedtime += usedtime;



        fd_table_to_gtask(rfc);
        free_rfc_detectOrigin(rfc);
        disconnect_fd(fd);

        return 0;

    }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>

#include "detect_orig.h"
#include "framework.h"
#include "dbg.h"
#include "shortest_path.h"
#ifdef FRAMEWORK
static int module_enable = 1;
#endif /* FRAMEWORK */

int mod_fc_debug_level = 0;
int delay_start = 0;
const char helpStr[] =
"Usage:\n"
"\t-v, --version       display the version and exit\n"
"\t-h, --help          print the help\n"
"\t-d, --debug         set debug level\n"
"\t-f conf_file        content that will be detected, \n"
"\t                    default is /usr/local/squid/etc/domain.conf\n"
"\t--conf conf_file    is same the previous\n"
"\t-F conf_file        detected, update or append to anyhost\n"
"\t-a anyhost          optional, anyhost's filename, \n"
"\t                    default is /var/named/chroot/var/named/anyhost\n"
"\t--anyhost anyhost   is same the previous\n"
"\t-t timer            optional, request's max time, \n"
"\t                    default is 3.000001s\n"
"\t-s                  set no need to sleep before start, \n"
;

static int set_process_rlimit(void)
{
    struct rlimit rlmt;
    /* no need anymore, was used for open masses of socket
     * in order to use valgrind
     */
    return 0;

    getrlimit(RLIMIT_NOFILE, &rlmt);
    if (rlmt.rlim_max < FD_MAX) {
        rlmt.rlim_max = FD_MAX;
        rlmt.rlim_cur = rlmt.rlim_max;
    } else if (rlmt.rlim_cur < rlmt.rlim_max) {
        rlmt.rlim_cur = rlmt.rlim_max;
    }
    if (0 != setrlimit(RLIMIT_NOFILE, &rlmt)) {
        cclog(0, "set process resouce limit failed");
        return -1;
    }
    cclog(0, "set rlim_cur = rlim_max: rlim_cur->%ld, rlim_max->%ld", rlmt.rlim_cur, rlmt.rlim_max);
    return 0;
}

void cmd_parser(int argc, char **argv)
{
    int opt, option_index;
    static struct option long_options[] = {
        {"conf",    1, 0, 'f'},
        {"conf_F",  1, 0, 'F'},
        {"process", 1, 0, 'p'},
        {"anyhost", 1, 0, 'a'},
        {"version", 0, 0, 'v'},
        {"help",    0, 0, 'h'},
        {"debug",   1, 0, 'd'},
        {0, 0, 0, 0}
    };

    optind = 1;
    while (-1 != (opt = getopt_long(argc, argv, "d:hvf:F:n:p:t:a:s", long_options, &option_index))) {
        switch (opt) {
            case 'a':
                memset(config.anyhost, 0, sizeof(config.anyhost));
                snprintf(config.anyhost, sizeof(config.anyhost)-1, "%s", optarg);
                break;
            case 'd':
                mod_fc_debug_level = atoi(optarg);
                break;
            case 'f':
                memset(config.confFilename, 0, sizeof(config.confFilename));
                snprintf(config.confFilename, sizeof(config.confFilename)-1, "%s", optarg);
                config.model = 0;
                break;
            case 'F':
                memset(config.confFilename, 0, sizeof(config.confFilename));
                snprintf(config.confFilename, sizeof(config.confFilename) - 1, "%s", optarg);
                config.model = 1;
                break;
            case 'h':
                printf("%s", helpStr);
                exit(0);
            case 'n':
                config.processNumber = atoi(optarg);
                break;
            case 's':
                delay_start = 1;
                break;
            case 't':
                config.timer = atof(optarg);
                break;
            case 'v':
                printf("version: %s BuildTime: %s %s\n", DETECT_VERSION, __DATE__, __TIME__);
                exit(0);
            case ':':
                fprintf(stderr, "option  [%c] needs a value\n", optopt);
                exit(-1);
            case '?':
                fprintf(stderr, "unknown option: [%c]\n", optopt);
                exit(-1);
        }
    }
}


void load_default_conf(void)
{
    memset(config.anyhost, 0, sizeof(config.anyhost));
    memset(config.anyhosttmp, 0, sizeof(config.anyhosttmp));
    memset(config.confFilename, 0, sizeof(config.confFilename));

    strcpy(config.anyhost, "/var/named/chroot/var/named/anyhost");
    strcpy(config.confFilename, "/usr/local/squid/etc/domain.conf");
    strcpy(config.OrigDomainFileName, "/usr/local/squid/etc/origdomain.conf");
    strcpy(config.origDomainIPHistoryFileName,"/usr/local/squid/etc/origin_domain_ip");

    sprintf(config.anyhosttmp, "%s.tmp", config.anyhost);

    config.model = 0;
    config.processNumber = 5;
    config.timer = conf_timer;
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  DetectOriginIndexGetKey
 *  Description:
 * =====================================================================================
 */
    HashKey
DetectOriginIndexGetKey( const void *data )
{
    DetectOriginIndex *p = (DetectOriginIndex*) data;
    assert( p );
    return (HashKey)p->sum;
} /* - - - - - - - end of function DetectOriginIndexGetKey - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  DetectOriginIndexIsEqual
 *  Description:
 * =====================================================================================
 */
    int
DetectOriginIndexIsEqual( const void *data1, const void *data2 )
{
    DetectOriginIndex *p1 = (DetectOriginIndex*) data1;
    DetectOriginIndex *p2 = (DetectOriginIndex*) data2;
    char *s1, *s2;
    assert( p1 );
    assert( p2 );
    if( p2->xor != p1->xor )
        return 0;
    s1 = p1->hostname;
    s2 = p2->hostname;
    while( *s1 != '\0' && *s2 != '\0' )
    {
        if( *s1 != *s2 )
            return 0;
        ++s1; ++s2;
    }
    if( *s1 == '\0' )
    {
        if( *s2 == '\0' )
            return 1;
        else if( s2[0] == '.' && s2[1] == '\0' )
            return 1;
        else
            return 0;
    }
    else if( *s2 == '\0' && s1[0] == '.' && s1[1] == '\0' )
        return 1;
    else
        return 0;
} /* - - - - - - - end of function DetectOriginIndexIsEqual - - - - - - - - - - - */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  DetectOriginIndexIsNull
 *  Description:
 * =====================================================================================
 */
    static int
DetectOriginIndexIsNull( const void *data )
{
    DetectOriginIndex *p = (DetectOriginIndex*) data;
    assert( p );
    return p->hostname == NULL;
} /* - - - - - - - end of function DetectOriginIndexIsNull - - - - - - - - - - - */

static void g_detect_tasks_init(void)
{
    int size;

    HashConfigure conf = {0};
    conf.name = "detect_tasks_init";
    conf.total = (2<<16)+1;
    conf.GetKey = DetectOriginIndexGetKey;
    conf.IsEqual = DetectOriginIndexIsEqual;
    conf.IsNull = DetectOriginIndexIsNull;
    conf.DoFree = (HashDoFree)free;
    g_detect_tasks_index = HashCreate( &conf );

    size = sizeof(struct DetectOrigin *) * max_tasks_capacity;
    g_detect_tasks = cc_malloc(size);
    assert(g_detect_tasks != NULL);
    memset(g_detect_tasks, 0, size);
}

    static int
DetectDomainIndexDoFree( const void *data )
{
    DetectOriginIndex *p = (DetectOriginIndex*) data;
    assert( data );
    if( p->hostname )
        free( p->hostname );
    p->hostname = NULL;
    free( p );
    return 0;
}

static void g_detect_domain_init(void)
{
    //int size;

    HashConfigure conf = {0};
    conf.name = "detect_domain_init";
    conf.total = (2<<16)+1;
    conf.GetKey = DetectOriginIndexGetKey;
    conf.IsEqual = DetectOriginIndexIsEqual;
    conf.IsNull = NULL;
    conf.DoFree = (HashDoFree)DetectDomainIndexDoFree;
    g_detect_domain_table = HashCreate( &conf );
}

static void globalVarInit(void)
{
    count = 0;  //??????????
    conf_timer = 3.0001;
    handled_task_index = 0;     //detect??????task????
    g_detect_task_count = 0;   //???????????????
    g_detect_anyhost_count = 0;   //anyhost???????????
    g_detect_tasks = NULL;    //record all the hosts information
    g_detect_domain_table = NULL;
    max_detectUpperIp_capacity = 100;
    max_detectCustom_capacity = 100;
    max_tasks_capacity = INITIAL_VALUE;  //current memory capacity of the tasks which malloced in need
    max_origDomain_capacity = INITIAL_VALUE;
    max_detectDomain_capacity = INITIAL_VALUE;
    max_origDomainHistory_capacity = INITIAL_VALUE;
    custom_cfg.detect_timeout = PROCESS_TIMEOUT_DEFAULT;
    custom_cfg.proportion = 0;
    custom_cfg.hdrs = NULL;
    custom_cfg.hdr_count = 0;
}

static void detectGlobalInit(void)
{
    globalVarInit();
    g_detect_tasks_init();
    g_detect_domain_init();
}
/* end by xin.yao:2011-11-14 */

int write_anyhost_file(void)
{
    FILE* fpNew = NULL;
    FILE* fpNamed = NULL;
    char anyhostInfo[128];
    char anyhostInfotmp[256];
    // why we not use rndc. ref: http://sis.ssr.chinacache.com/rms/view.php?id=4525

    //?õ?????дwild??????????????????
    fpNamed = fopen(config.anyhost, "r");
    if (NULL == fpNamed) {
        addInfoLog(2, "can not open anyhost and exit");
        return -1;
    }
    fpNew = fopen(config.anyhosttmp, "w");
    if (NULL == fpNew) {
        addInfoLog(2, "can not open anyhost.tmp file and exit");
        return -1;
    }
    getNewAnyhost(0, fpNamed, fpNew);
    fclose(fpNamed);
    fclose(fpNew);
    if(-1 == rename(config.anyhosttmp, config.anyhost)) {
        addInfoLog(3, "copy temp file to anyhost file error");
        return -1;
    }
    //дanyhost.info
    strcpy(anyhostInfo, config.anyhost);
    strcat(anyhostInfo, ".info");
    sprintf(anyhostInfotmp, "%s.tmp", anyhostInfo);
    fpNamed = fopen(anyhostInfo, "r");
    if (NULL == fpNamed) {
        fpNamed = fopen(anyhostInfo, "w");
        if (NULL == fpNamed) {
            addInfoLog(2, "can not open anyhost.info and exit");
            return -1;
        }
        fclose(fpNamed);
        fpNamed = fopen(anyhostInfo, "r");
        if (NULL == fpNamed) {
            addInfoLog(2, "can not open anyhost.info and exit");
            return -1;
        }
    }
    fpNew = fopen(anyhostInfotmp, "w");
    if (NULL == fpNew) {
        addInfoLog(2, "can not open /tmp/wsg.txt and exit");
        return -1;
    }
    getNewAnyhost(1, fpNamed, fpNew);
    fclose(fpNamed);
    fclose(fpNew);
    if(-1 == rename(anyhostInfotmp, anyhostInfo)) {
        addInfoLog(3, "copy temp file to anyhost.info file error");
        return -1;
    }
    modify_anyhost_timestamp();
    return 0;
}

#ifndef FRAMEWORK
int process_core_task(void)
{
    int rtn = 0;

    //have_recent_added = 0;
    rtn = getRecentAddedFromAnyhost(config.anyhost); /* open anyhost */
    if( -1 == rtn){
        addInfoLog(2, "getRecentAddedFromAnyhost error(there are errors in anyhost file)  and exit");
    }
    //?õ?Ҫ??????????
    rtn = getDetectOrigin(config.confFilename); /* open domain.conf */
    if (-1 == rtn) {
        addInfoLog(2, "getDetectOrigin error(there are errors in domain.conf file)  and exit");
        return -1;
    }
    //rtn = checkRepeatRecordFromDomainConf();    /* 检查是否有集中探测，我这里不需要 */
    //????????
    rtn = detect(config.timer);
    if (-1 == rtn) {
        addInfoLog(2, "detectOrigin error and exit");
        return -1;
    }
    modifyUsedTime();
    copySameIPDetectRes();
    addResultLog();
    //write detect result
    rtn = write_anyhost_file();
    return rtn;
}
#else /* not FRAMEWORK */
int process_core_task_before(void)
{
    int rtn = 0;

    //have_recent_added = 0;
    rtn = getRecentAddedFromAnyhost(config.anyhost); /* open anyhost */
    if( -1 == rtn){
        addInfoLog(2, "getRecentAddedFromAnyhost error(there are errors in anyhost file)  and exit");
    }
    //?õ?Ҫ??????????
    rtn = getDetectOrigin(config.confFilename); /* open domain.conf */
    if (-1 == rtn) {
        addInfoLog(2, "getDetectOrigin error(there are errors in domain.conf file)  and exit");
        return -1;
    }
    //rtn = checkRepeatRecordFromDomainConf();
    conf_timer = config.timer;
    return rtn;
}
int process_core_task_middle(const void *data)
{
    int detect_core_loop_epoll(const void *data);
    return detect_core_loop_epoll( data );
}
int process_core_task_after(void)
{
    int rtn = 0;

    //copySameIPDetectRes();
    //write detect result
    CalculateUsedTime();
    rtn = write_anyhost_file();
    return rtn;
}
#endif /* not FRAMEWORK */

#ifndef FRAMEWORK
static void fd_table_init(void)
{
    int size;

    size = sizeof(struct DetectOrigin) * FD_MAX;
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
#else /* not FRAMEWORK */
#endif /* not FRAMEWORK */

static void alarmTimeoutHandler(int sig)
{
    kill(0, SIGTERM);
}

static void g_pstOrigDomain_init(void)
{
    int size;

    size = sizeof(struct origDomain *) * max_origDomain_capacity;
    g_pstOrigDomain = cc_malloc(size);
    assert(g_pstOrigDomain != NULL);
    memset(g_pstOrigDomain, 0, size);
}

static void g_pstOrigDomainHistory_init(void)
{
    int size;

    size = sizeof(struct origDomainHistory *) * max_origDomainHistory_capacity;
    g_pstOrigDomainHistory = cc_malloc(size);
    assert(g_pstOrigDomainHistory != NULL);
    memset(g_pstOrigDomainHistory, 0, size);
}

int check_conf_file_valid(void)
{
    FILE* fp = NULL;

    fp = fopen(config.OrigDomainFileName, "r");
    if(NULL == fp)
        addInfoLog(2, "can not open origdomain.conf and exit");
    else
    {
        /* add by xin.yao: 2011-11-14 */
        g_pstOrigDomain_init();
        /* end by xin.yao: 2011-11-14 */
        if(-1 == getOrigDomainConf(fp)) {
            addInfoLog(2, "getOriginDomainConf error[there are errors in conf file]");
        }
        fclose(fp);
    }
    fp = fopen(config.origDomainIPHistoryFileName, "r");
    if (NULL == fp)
        addInfoLog(2, "can not open origin_domain_ip");
    else
    {
        g_pstOrigDomainHistory_init();	
        if (-1 == getOrigDomainHistory(fp))
            addInfoLog(2, "getOriginDomainHistory error[here are errors in conf file]");
        fclose(fp);
    }
    return 0;
}

#ifndef FRAMEWORK
int main(int argc, char **argv)
#else /* not FRAMEWORK */
int mod_main(int argc, char **argv)
#endif /* not FRAMEWORK */
{
    /* parse command line param */
    cmd_parser(argc, argv);
    /* open log file first,
     * because of we need write log in the following handled 
     */
    open_log();
    /* initialize the config variable, 
     * using default configuration or value 
     */
    load_default_conf();
    detectGlobalInit();
    check_conf_file_valid();

    /* fork for detect, parent process will wait for child process */
#ifndef FRAMEWORK
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork()");
        kill(0, SIGTERM);
        exit(1);
    }   
    if (!pid)
#else /* not FRAMEWORK */
    {
        /* 复制自下部分代码，关于fork后主进程的工作 */
        struct sigaction sa;
        struct itimerval itv;

        sa.sa_handler = alarmTimeoutHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGALRM, &sa, NULL);
        itv.it_interval.tv_sec = 1;
        itv.it_interval.tv_usec = 0;
        itv.it_value.tv_sec = custom_cfg.detect_timeout;//g_detect_timeout;
        //printf("detect_timeout: %ld\n", custom_cfg.detect_timeout);
        itv.it_value.tv_usec = 0;
        setitimer(ITIMER_REAL, &itv, NULL);
    }
#endif /* not FRAMEWORK */
    {
        int rtn = 0;
#ifndef FRAMEWORK
        int sleep_time;

        /* set current process resource limit
         * Limits on resources crontab, and the process is the child process crontab, 
         * inherited this limit, so in order to the stability of the program to modify resource constraints
         */
        if (set_process_rlimit() < 0) {
            return -1;
        }
        /* delayed time: [10, 30)s, 
         * in order to try to spread more FC and upper detection  
         */
        if(!delay_start) {
            srand((unsigned)time(NULL));          //??ʼ????????
            sleep_time = rand() % 20 + 10; 
            fprintf(stdout, "sleep %d\n", sleep_time);
            sleep(sleep_time); 
        }
        /* core task */
        detect_epoll_init();
        fd_table_init();
        rtn = process_core_task();
        if (rtn < 0) {
            rtn =  -1;
        }
        fd_table_dest();
        eventFreeMemory();
        clean_detect_mem();
        close_log();
        return rtn;
#else /* not FRAMEWORK */
        if (set_process_rlimit() < 0) {
            module_enable = 0;
            return 1;
        }
        rtn = process_core_task_before();
        if (rtn < 0)
        {
            module_enable = 0;
            return 1;
        }
        return 0;
#endif /* not FRAMEWORK */
    }
#ifndef FRAMEWORK
    else
    {
        int status;
        pid_t ret_pid;
        struct sigaction sa;
        struct itimerval itv;

        sa.sa_handler = alarmTimeoutHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGALRM, &sa, NULL);
        itv.it_interval.tv_sec = 1;
        itv.it_interval.tv_usec = 0;
        itv.it_value.tv_sec = custom_cfg.detect_timeout;//g_detect_timeout;
        //printf("detect_timeout: %ld\n", custom_cfg.detect_timeout);
        itv.it_value.tv_usec = 0;
        setitimer(ITIMER_REAL, &itv, NULL);

        while ((ret_pid = wait(&status)) != pid) {
            if (ret_pid < 0) {
                if (EINTR == errno) {
                    continue;
                }
            }
            if (!WIFEXITED(status) || WIFSIGNALED(status)) {
                continue;
            }
        }
        clean_detect_mem();
        close_log();
        return 0;
    }       
#else /* not FRAMEWORK */
#endif /* not FRAMEWORK */
}

#ifndef FRAMEWORK
#else /* not FRAMEWORK */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ModuleFCDataInput
 *  Description:  
 * =====================================================================================
 */
    int
ModuleFCDataInput( const void *data )
{
    if( module_enable == 0 )
        return 0;
    return process_core_task_middle( data );
} /* - - - - - - - end of function ModuleFCDataInput - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  mod_fc_init
 *  Description:  
 * =====================================================================================
 */
    int
ModuleFCInit( const void *data )
{
    int argc = 2;
    char *argv[] = {"exe_name","-s",NULL};
    SetLinkDataReadCountMax( kFC_LINK_DATA_READ_COUNT_MAX );
    mod_main( argc, argv );
    ResultLogInit();
    return 0;
} /* - - - - - - - end of function mod_fc_init - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ModuleFCEnd
 *  Description:  
 * =====================================================================================
 */
    int
ModuleFCEnd( const void *data )
{
    if( module_enable == 0 )
        return 0;
    process_core_task_after();
    ResultLogEnd();
    eventFreeMemory();
    clean_detect_mem();
    close_log();
    return 0;
} /* - - - - - - - end of function ModuleFCEnd - - - - - - - - - - - */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  RegisterModule
 *  Description:  
 * =====================================================================================
 */
    int
RegisterModule( void )
{
    int ret = 0;
    Debug( 40, "Register module detectorig." );
    ret |= AddHookFunction( kHOOK_INIT, ModuleFCInit );
    ret |= AddHookFunction( kHOOK_DATA_INPUT, ModuleFCDataInput );
    ret |= AddHookFunction( kHOOK_END, ModuleFCEnd );
    return ret;
} /* - - - - - - - end of function RegisterModule - - - - - - - - - - - */
#endif /* not FRAMEWORK */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "display.h"
#include "digRun.h"
#include "child_mgr.h"
#include "misc.h"
#include "write_rcms_log.h"

static const char helpStr[] =   "Usage:\n"
				"    -v, --version       display the version and exit\n"
				"    -d, --debug         debug digRun\n"
				"    -h, --help          print the help\n"
				"\n"
				"    -n number           optional, subProcess's number, default is 10\n"
				"    -t timer            optional, request's max time, \n"
				"    -k check            parse configuration file, \n"
				"                        default is 3.000001s\n";

static const char g_szPidFile[] = "/var/run/detectorig/digRun.pid";

int main(int argc, char* argv[])
{
	//added by chinacache zenglj
	char OrigDomainFileName[128];
	memset(OrigDomainFileName, 0, sizeof(OrigDomainFileName));
	strcpy(OrigDomainFileName, "/usr/local/squid/etc/origdomain.conf");

	char origDomainIPHistoryFileName[128];
	memset(origDomainIPHistoryFileName, 0, sizeof(origDomainIPHistoryFileName));
	strcpy(origDomainIPHistoryFileName,"/usr/local/squid/etc/origin_domain_ip");

	char alertLogFileName[128];
	memset(alertLogFileName, 0, sizeof(alertLogFileName));
	strcpy(alertLogFileName, "/usr/local/squid/var/dig_alert.dat");

	char tempFileName[128];
	memset(tempFileName, 0, sizeof(tempFileName));
	strcpy(tempFileName, "/usr/local/squid/var/dig_alert.dat.tmp");

	char processDigName[128];
	memset(processDigName, 0, sizeof(processDigName));
	strcpy(processDigName, "/usr/local/detectorig/bin/digDetect");

	static struct option long_options[] = {
		{"timer", 1, 0, 't'},{"number", 1, 0, 'n'},{"debug",1,0,'d'},
		{"check", 1, 0, 'k'},{"version",0, 0, 'v'},{"help", 0, 0, 'h'},{0, 0, 0, 0}
	};

	//int processDigNumber = 10;
	int processDigNumber = 1;
	double timerDig = 3.0;
	int opt;
	int option_index;
	int check_flag = 0;

	while ((opt=getopt_long(argc, argv, "d:t:n:k:vh", long_options, &option_index)) != -1) {

		switch (opt) {
			case 'd':   //debug_level
				debug_level = atoi(optarg);         
				break;
			case 'h':   //输入参数为h，输出帮助信息，退出程序
				printf("%s", helpStr);
				return 0;
			case 'v':   //输入参数为v，表示版本号
				printf("BuildTime %s %s, version: 0.61\n",__DATE__, __TIME__);
				return 0;
			case 't':   //输入参数为t，最大请求时间，赋给对应的变量
				timerDig = atof(optarg);
				break;
			case 'n':   //输入参数为n，内容为请求进程的个数
				processDigNumber = atoi(optarg);
				break;
			case 'k':
                if ((int) strlen(optarg) < 1)
					printf("%s", helpStr);
				if (!strncmp(optarg, "check", strlen(optarg)))
					check_flag = 1;
				else
					printf("%s", helpStr);
				break;
			case ':':   //如果输入选项没有对应的值，退出程序
				fprintf(stderr, "option  [%c] needs a value\n", optopt);
				exit(-1);
			case '?':   //如果输入为未知选项，退出程序
				fprintf(stderr, "unknown option: [%c]\n", optopt);
				exit(-1);
		}
	}

	int ret;
	if (0 == check_flag)
	{
		if( (ret = write_pid_file(g_szPidFile)) == -1 ) {	//bug，未考虑目录不存在的情况Tom@cc
			fprintf(stderr, "the same process exists and exit\n");
			return -1;
		}
	}

	unlink(tempFileName);

    FILE* fp;
	if( (fp = fopen(OrigDomainFileName, "r")) == NULL ) {
		addInfoLog(2, "cannot open origdomain.conf and exit");
		return -1;
	}

	if( (ret = getOrigDomainConf(fp, check_flag)) == -1 ) {
		addInfoLog(2, "getOriginDomainConf error(there are errors in conf file)");
	}
	fclose(fp);

	if( (fp = fopen(origDomainIPHistoryFileName, "r")) == NULL ) {
		addInfoLog(2, "cannot open origin_domain_ip");
	} else {
		ret = getOrigDomainHistory(fp);
		if(-1 == ret) {
			addInfoLog(2, "getOriginDomainHistory error(there are errors in conf file)");
		}
		fclose(fp);
	}

	if (check_flag)
		exit(0);

	//进行侦测
	if( (ret = detect(processDigNumber, processDigName, timerDig)) == -1 ) {
		addInfoLog(2, "detectOrigin error and exit");
		return -1;
	}

	if(writeOrigDomainIP() < 0) {
		addInfoLog(3, "write origin_domain_ip file error");
	}

	addResultLog();

	if(0 == access(tempFileName, F_OK))
	{
		if(-1 == rename(tempFileName, alertLogFileName))
		{
			addInfoLog(3, "copy temp file to dig_alert.dat file error");
		}
	}
	clean_dig_run_mem();
	return 1;

}

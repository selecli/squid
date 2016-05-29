//********************************
//Desc:   get disk speed
//Author: LuoJun.Zeng
//Date :  2006-04-15
//********************************

#include<stdio.h>
#include<sys/time.h>
#include<string.h>
#include <stdlib.h>
#define MILLION 1000000L

#define VERSION "1.3"

char appName[BUFSIZ];

const char* conf_pathName = "squid_configure";
const char* conf_datasizeName = "disk_data_size";
const char* conf_diskdirName = "disk_dir";

char conf_path[BUFSIZ];
char conf_datasize[BUFSIZ];
char conf_diskdir[BUFSIZ];

void getSquidConf();
static void str_ltrim(char*, const char*);
static void str_rtrim(char*, const char*);

void getSquidConf()
{
    FILE* confFile;
    FILE* squidFile;
    char line[BUFSIZ];

    if ((confFile = fopen("../etc/monitor.conf", "r")) == NULL)
    {
        perror("monitor.conf");
        exit(1);
    }

    while (fgets(line, BUFSIZ, confFile) != NULL)
    {
        str_ltrim(line, "\t");
        str_rtrim(line, "\t\r\n");
        if (!strncmp(conf_pathName, line, strlen(conf_pathName)))
        {
            int i = 0;
            while (conf_path[i] = line[i + 1 + strlen(conf_pathName)])
            {
                i++;
            }
            //printf("%s\n", conf_path);
        }
        if (!strncmp(conf_datasizeName, line, strlen(conf_datasizeName)))
        {
            int i = 0;
            while (conf_datasize[i] = line[i + 1 + strlen(conf_datasizeName)])
            {
                i++;
            }
            //printf("%s\n", conf_datasize);
        }
        if (!strncmp(conf_diskdirName, line, strlen(conf_diskdirName)))
        {
            int i = 0;
            while (conf_diskdir[i] = line[i + 1 + strlen(conf_diskdirName)])
            {
                i++;
            }
			char* pResult = NULL;
			pResult = strtok(conf_diskdir, " ");
			sprintf(conf_diskdir,"%s",pResult);
            //printf("%s\n", conf_diskdir);
        }
    }
}

static void str_ltrim(char* str, const char* tokens)
{
    if (str == NULL)
    {
        return;
    }
    char* p = str;
    while ((*p) && (strchr(tokens, *p)))
    {
        p++;
    }
    memmove(str, p, strlen(p) + 1);
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

main(int argc, char** argv)
{
    int n = 1;
    FILE* source;
    FILE* target;
    char buff[BUFSIZ];
    long fileSize = 200*1024*1024;
    time_t beginTime;
    time_t endTime;
    double spendTime;
    double writeSpeed;
    double readSpeed;
    time_t updateTime;
    long timedif;
    struct timeval tpend;
    struct timeval tpstart;
    FILE* poolFile;
    char strLine[150];
    char strSquidConf[BUFSIZ];

	sprintf(appName, "%s", *argv);
	str_ltrim(appName, "./");

	if(argc > 2)
	{
		printf("error argument.\n");
		printf("Use \"%s -h\" for help.\n", appName);
		exit(0);
	}
	argc--;
	argv++;
	while(argc && *argv[0] == '-')
	{
		if (!strcmp(*argv, "-v"))
		{
			printf("%s Version: %s\n", appName, VERSION);
			exit(0);
		}
		if (!strcmp(*argv, "-h"))
		{
			printf("Use \"%s -v\" for version number.\n", appName, VERSION);
			printf("Use \"%s -h\" for help.\n", appName, VERSION);
			exit(0);
		}
		else
		{
			printf("error argument.\n");
			printf("Use \"%s -h\" for help.\n", appName);
			exit(0);
		}

		argv++;
		argc--;
	}

    getSquidConf();
    fileSize = atoi(conf_datasize) * 1024 * 1024;

    gettimeofday(&tpstart, NULL);
    if ((source = fopen(strcat(conf_diskdir, "/SpeedTest.File"), "w+")) == NULL)
    {
        perror("SpeedTest.File"); 
        exit(1);
    }

    //printf("\n");
    //printf("Write Disk Start\n");
    
    while (BUFSIZ * n <= fileSize)
    {
        fwrite(buff, sizeof(buff), 1, source);
        n += 1;
        //printf("%d\n",n);
    }
    fclose(source);
    gettimeofday(&tpend, NULL);
    timedif = (tpend.tv_sec - tpstart.tv_sec) * MILLION; 
    timedif += tpend.tv_usec - tpstart.tv_usec;
    //printf("%ld, %ld\n", tpstart.tv_sec, tpend.tv_sec);
    //printf("%ld,  %ld\n", tpstart.tv_usec, tpend.tv_usec);
    //printf("Write Disk End\n");
    //printf("The spend time is %ld microseconds\n", timedif);
    spendTime = timedif;
    writeSpeed = fileSize/1024/1024/spendTime*1000000;
    //printf("%f Mb/s\n", fileSize/1024/1024/spendTime*1000000);

    //printf("\n");

//****************************************************
//Get UpdateTime
    updateTime = time(NULL);

//****************************************************
//Read File
    gettimeofday(&tpstart, NULL);
    if ((target = fopen(conf_diskdir, "r")) == NULL)
    {
        perror("SpeedTest.File");
        exit(1);
    }

    n = 1;
    //printf("Read Disk Start...\n");
    
    while (BUFSIZ * n <= fileSize)
    {
        fread(buff, sizeof(char), BUFSIZ, target);
        //printf("%d\n", n);
        n += 1;
    }
    fclose(target);
	unlink(conf_diskdir);
    gettimeofday(&tpend, NULL);
    timedif = (tpend.tv_sec - tpstart.tv_sec) * MILLION; 
    timedif += tpend.tv_usec - tpstart.tv_usec;
    //printf("%ld, %ld\n", tpstart.tv_sec, tpend.tv_sec);
    //printf("%ld,  %ld\n", tpstart.tv_usec, tpend.tv_usec);
    //rintf("Read Disk End\n");
    //rintf("Spend time is %ld microseconds\n", timedif);
    spendTime = timedif;
    readSpeed = fileSize/1024/1024/spendTime*1000000;
    //printf("%f Mb/s\n", fileSize/1024/1024/spendTime*1000000);
    //printf("\n");

//*******************************************************
//Write To the diskMon.pool
    if ((poolFile = fopen("../pool/diskSpeedMon.pool", "w+")) == NULL)
    {
        perror("diskSpeedMon.pool");
        exit(1);
    }
    sprintf(strLine, "diskWriteSpeedMBps: %.2f\n", writeSpeed);
    fputs(strLine, poolFile);

    sprintf(strLine, "diskReadSpeedInMBps: %.2f\n", readSpeed);
    fputs(strLine, poolFile);

    sprintf(strLine, "diskSpeedUpdateTime: %ld\n", updateTime);
    fputs(strLine, poolFile);

    fclose(poolFile);
}

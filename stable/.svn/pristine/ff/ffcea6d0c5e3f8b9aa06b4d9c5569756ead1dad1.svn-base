//********************************
//Desc:   Detect log fiile transfer status
//Author: LuoJun.Zeng
//Date :  2006-04-15
//********************************

#include<stdio.h>
#include<dirent.h>
#include<errno.h>
#include<time.h>
#include<sys/stat.h>
#include<string.h>
#include <stdlib.h>

#define CTIME_SIZE 26
#define VERSION "1.3"

char appName[BUFSIZ];

const char* conf_logdirName = "log_dir";
const char* conf_logtimeoutName = "log_timeout";

char conf_logdir[BUFSIZ];
char conf_logtimeout[BUFSIZ];

int hasLogdir = 0;
int hasLogtimeout = 0;

void getSquidConf();
static void str_ltrim(char*, const char*);
static void str_rtrim(char*, const char*);

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
        if (!strncmp(conf_logdirName, line, strlen(conf_logdirName)))
        {
            int i = 0;
            while (conf_logdir[i] = line[i + 1 + strlen(conf_logdirName)])
            {
                i++;
            }
			hasLogdir = 1;
			//printf("%s\n", conf_logdir);
        }
         if (!strncmp(conf_logtimeoutName, line, strlen(conf_logtimeoutName)))
         {
             int i = 0;
             while (conf_logtimeout[i] = line[i + 1 + strlen(conf_logtimeoutName)])
             {
                 i++;
             }
			 hasLogtimeout = 1;
			 //printf("%s\n", conf_logtimeout);
         }
		
    }

}

main(int argc, char** argv)
{
	FILE* poolFile;
	struct dirent* direntp;
	DIR* dirp;
	int iResult = 1;
	time_t updateTime;
	char strLine[BUFSIZ];

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
	//if((dirp = opendir(argv[1])) == NULL)
	if(!hasLogdir)
	{
		strcpy(conf_logdir, "/proclog/log/squid/archive");
	}
	if ((dirp = opendir(conf_logdir)) == NULL)
	{
		perror("Failied to open directory");
		iResult = -1;
		//exit(1);
	}
	 else
	{
		while ((direntp = readdir(dirp)) != NULL)
		{
			//printf("%s\n",direntp->d_name);
			struct stat fileStat;
			char pathname[BUFSIZ];
			time_t nowTime;

			strcpy(pathname, conf_logdir);
			str_rtrim(pathname,"/");
			strcat(pathname,"/");
			//printf("%s\n",pathname);
			stat(strcat(pathname,direntp->d_name), &fileStat);
			//stat("/proclog/log/squid/archive/test3.file", &fileStat);
			//printf("%s %d\n", ctime(&fileStat.st_mtime), fileStat.st_mtime);
			//printf("%s %d\n", ctime(&fileStat.st_atime), fileStat.st_atime);
			//printf("%s %d\n", ctime(&fileStat.st_ctime), fileStat.st_ctime);

            char * pLog = NULL;
			pLog = strtok(direntp->d_name, ".");
			while (pLog != NULL)
			{
				//printf("%s\n", pLog);
				if (!strcmp(pLog, "log"))
				{
                      nowTime = time(NULL);
                      //printf("%s\n",ctime(&fileStat.st_mtime));
                      //printf("%f\n", difftime(nowTime, fileStat.st_mtime));
                      //printf("%d", atoi(conf_logtimeout) * 60);
                      if(difftime(nowTime, fileStat.st_mtime) > atoi(conf_logtimeout) * 60)
                      {
                          iResult = 0;
                      }
				}

				pLog = strtok(NULL, ".");
			}
		}

		while ((closedir(dirp) == -1) && (errno == EINTR));
	}



	//****************************************************
	//Get UpdateTime
	updateTime = time(NULL);

	//*******************************************************
	//Write To the squidLogTransMon.pool
	if ((poolFile = fopen("../pool/squidLogTransMon.pool", "w+")) == NULL)
	{
		perror("squidLogTransMon.pool");
		exit(1);
	}

	sprintf(strLine, "squidLogTransResult: %d\n", iResult);
	fputs(strLine, poolFile);

	sprintf(strLine, "squidLogTransResultUpdateTime: %ld\n", updateTime);
	fputs(strLine, poolFile);

	fclose(poolFile);

	//printf("%d\n",access("test.log",0));
}

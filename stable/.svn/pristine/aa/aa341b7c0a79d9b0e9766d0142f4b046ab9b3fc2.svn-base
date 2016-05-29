//********************************
//Desc:   get applications version number
//Author: LuoJun.Zeng
//Date :  2006-04-15
//********************************
#include<stdio.h>
#include<dirent.h>
#include<errno.h>
#include<time.h>
#include<sys/stat.h>
#include<unistd.h>
#include <string.h>
#include <stdlib.h>

#define CTIME_SIZE 26
#define VERSION "1.3"

char appName[BUFSIZ];

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

void getAllVersion()
{
	struct dirent* direntp;
	DIR* dirp;
	int iResult = 1;

	if ((dirp = opendir(".")) == NULL)
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
			char fileName[BUFSIZ];
			sprintf(pathname, "%s", "./");
			time_t nowTime;
			stat(direntp->d_name, &fileStat);
			sprintf(fileName,"%s%s", pathname, direntp->d_name);

			if(!S_ISDIR(fileStat.st_mode))
			{
				if(access(direntp->d_name, X_OK) == 0 && strcmp(fileName, appName))
				{
					//printf("%s\n", fileName);
					//printf("%s\n", direntp->d_name);
//                     if (execl(fileName,direntp->d_name,"-v", NULL))
//                     {
//                         printf("%s\n",pathname);
//
//                         perror(direntp->d_name);
//                         exit(128);
//                     }
				   char command[BUFSIZ];
				   sprintf(command, "%s %s", fileName, "-v");
				   //printf("%s\n", command);
				   system(command);
				}
			}

		}

		while ((closedir(dirp) == -1) && (errno == EINTR));
	}

}

main(int argc, char** argv)
{
	FILE* poolFile;
	time_t updateTime;
	char strLine[BUFSIZ];

	sprintf(appName,"%s", *argv);
	//str_ltrim(appName, "./");

	if(argc == 1)
	{
		printf("Use \"%s -h\" for help.\n", appName);
		exit(0);
	}

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
			str_ltrim(appName, "./");
			printf("%s Version: %s\n", appName, VERSION);
			exit(0);
		}
		if (!strcmp(*argv, "-h"))
		{
			printf("Use \"%s -v\" for version number.\n", appName, VERSION);
			printf("Use \"%s -av\" for all version number.\n", appName, VERSION);
			printf("Use \"%s -h\" for help.\n", appName, VERSION);
			exit(0);
		}
		if(!strcmp(*argv, "-av"))
		{
			char appName1[BUFSIZ];
			sprintf(appName1, "%s", appName);
			str_ltrim(appName1, "./");
			printf("%s Version: %s\n", appName1, VERSION);
			getAllVersion();
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



	//****************************************************
	//Get UpdateTime
	updateTime = time(NULL);

	//printf("%d\n",access("test.log",0));
}

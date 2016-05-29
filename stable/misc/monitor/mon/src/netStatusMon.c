//********************************
//Desc:   Detect net status
//Author: LuoJun.Zeng
//Date :  2006-04-15
//********************************

//**************************************************************
//                Update  Log
//--------------------------------------------------------------
// version 1.4 : 2006-04-27
//       add: save temp device data to /tmp/netDev2.dat
//       fixed by LuoJun.Zeng
//**************************************************************

#include<stdio.h>
#include<time.h>
#include<string.h>
#include <stdlib.h>

#define VERSION "1.4"

char appName[BUFSIZ];

static void str_ltrim(char*, const char*);
static void str_rtrim(char*, const char*);

void getSquidConf();

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
		str_ltrim(line, "\t ");
		str_rtrim(line, "\t\r\n ");
/*
		 if (!strncmp(conf_deviceName, line, strlen(conf_deviceName)))
		 {
			 int i = 0;
			 while (conf_device[i] = line[i + 1 + strlen(conf_deviceName)])
			 {
				 i++;
			 }
			 //printf("%s\n", conf_device);
		 }
*/
	}
}

main(int argc, char** argv)
{
	int filedes1[2], filedes2[2];
	int pid;
	FILE* devFile;
	FILE* netDataFile;
	FILE* poolFile;
	char line[BUFSIZ];
	const char* devNameHead = "eth";
	int devNum = 0;
	char devName[BUFSIZ];
	char oldRecErrs[BUFSIZ];
	char oldTrnErrs[BUFSIZ];
	char oldTrnColls[BUFSIZ];
	char recErrs[BUFSIZ];
	char trnErrs[BUFSIZ];
	char trnColls[BUFSIZ];
	char maxRecErrs[BUFSIZ];
	char maxTrnErrs[BUFSIZ];
	char maxTrnColls[BUFSIZ];
	char maxErrs[BUFSIZ];
	char strLine[BUFSIZ];
	char maxRecErrsDev[BUFSIZ];
	char maxTrnErrsDev[BUFSIZ];
	char maxTrnCollsDev[BUFSIZ];
	char maxErrsDev[BUFSIZ];
	time_t updateTime;

	sprintf(appName, "%s", *argv);
	str_ltrim(appName, "./");

	if (argc > 2)
	{
		printf("error argument.\n");
		printf("Use \"%s -h\" for help.\n", appName);
		exit(0);
	}
	argc--;
	argv++;
	while (argc && *argv[0] == '-')
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
	sprintf(maxRecErrs, "%d", 0);
	sprintf(maxTrnErrs, "%d", 0);
	sprintf(maxTrnColls, "%d", 0);
	sprintf(maxErrs, "%d", 0);

	if ((devFile = fopen("/proc/net/dev", "r")) == NULL)
	{
		perror("/proc/net/dev");
		exit(1);
	}

	while (fgets(line, BUFSIZ, devFile) != NULL)
	{
		str_ltrim(line, "\t ");
		str_rtrim(line, "\t\r\n ");

		if (!strncmp(devNameHead, line, strlen(devNameHead)))
		{
			int count = 1;
			char* colonPos = strstr(line, ": ");
			if (colonPos)
			{
				--count;
			}

			char * pValue = NULL;
			pValue = strtok(line, " ");

			while (pValue != NULL)
			{
				//printf("%s\n", pValue);

				if (count == 3)
				{
					sprintf(recErrs, "%s",pValue);
					//printf("recErrs is %s\n", recErrs);
				}
				if (count == 11)
				{
					sprintf(trnErrs, "%s",pValue);
					//printf("trnErrs is %s\n",trnErrs);
				}
				if (count == 14)
				{
					sprintf(trnColls, "%s",pValue);
				}

				count++;
				pValue = strtok(NULL, " ");
			}
			sprintf(devName, "%s%d","eth",devNum);
			sprintf(maxErrsDev, "%s", devName);
			sprintf(maxTrnCollsDev, "%s", devName);
			//printf("device face is %s\n", devName);
			sprintf(strLine, "%s %s %s %s\n", devName, recErrs, trnErrs, trnColls);
			//printf("new line is: %s\n", strLine);

			////////////////////////////////////////
			//Read old data
			if ((netDataFile = fopen("/tmp/netDev.dat", "r")) != NULL)
			{
				while (fgets(line, BUFSIZ, netDataFile) != NULL)
				{
					str_ltrim(line, "\t ");
					str_rtrim(line, "\t\r\n ");
					if (!strncmp(devName, line, strlen(devName)))
					{
						pValue = NULL;
						pValue = strtok(line, " ");
						count = 1;
						while (pValue != NULL)
						{
							if (count == 2)
							{
								sprintf(oldRecErrs, "%s", pValue);
							}
							if (count == 3)
							{
								sprintf(oldTrnErrs, "%s", pValue);
							}
							if (count == 4)
							{
								sprintf(oldTrnColls, "%s", pValue);
							}

							count++;
							pValue = strtok(NULL, " ");
						}
					}
				}//while(fgets
				fclose(netDataFile);
			}//if ((netDataFile

			////////////////////////////////////////////
			//Write new data to temp file
			if (devNum == 0)
			{
				if ((netDataFile = fopen("/tmp/netDev2.dat", "w+")) == NULL)
				{
					perror("netDev.dat");
					exit(1);
				}
			}
			else
			{
				if ((netDataFile = fopen("/tmp/netDev2.dat", "a")) ==  NULL)
				{
					perror("netDev2.dat");
					exit(1);
				}
			}
			fputs(strLine, netDataFile);
			fclose(netDataFile);

			///////////////////////////////////////////
			//Compare value
			if (atoi(recErrs) - atoi(oldRecErrs) > atoi(maxRecErrs))
			{
				sprintf(maxRecErrsDev, "%s", devName);
				sprintf(maxRecErrs, "%d", atoi(recErrs) - atoi(oldRecErrs));
			}
			if (atoi(trnErrs) - atoi(oldTrnErrs) > atoi(maxTrnErrs))
			{
				sprintf(maxTrnErrsDev, "%s", devName);
				sprintf(maxTrnErrs, "%d", atoi(trnErrs) - atoi(oldTrnErrs));
			}
			if (atoi(trnColls) - atoi(oldTrnColls) > atoi(maxTrnColls))
			{
				sprintf(maxTrnCollsDev, "%s", devName);
				sprintf(maxTrnColls, "%d", atoi(trnColls) - atoi(oldTrnColls));
			}
			if (atoi(maxRecErrs) + atoi(maxTrnErrs) > atoi(maxErrs))
			{
				sprintf(maxErrsDev, "%s", devName);
				sprintf(maxErrs, "%d", atoi(maxRecErrs) + atoi(maxTrnErrs));
			}

			devNum++;
		}//if(!strncmp(devNameHead
	}//while(fgets
	fclose(devFile);

	//****************************************************
	//write old data to /tmp/netDev.dat
	FILE* source;
	FILE* target;
	if ((source = fopen("/tmp/netDev2.dat", "r")) == NULL)
	{
		perror("netDev2.dat");
		exit(1);
	}
	if ((target = fopen("/tmp/netDev.dat", "w+")) == NULL)
	{
		perror("netDev.dat");
		exit(1);
	}
	while (fgets(line, BUFSIZ, source) != NULL)
	{
		fputs(line, target);
	}
	fclose(source);
	fclose(target);

	//****************************************************
	//Get UpdateTime
	updateTime = time(NULL);

	////////////////////////////////////////
	//write to the pool file
	if ((poolFile =fopen("../pool/netStatusMon.pool","w+")) == NULL)
	{
		perror("netConnMon.pool");
		exit(1);
	}

	sprintf(strLine, "netStatusMaxErrosDiff: %s\n", maxErrs);
	fputs(strLine, poolFile);
	printf("netStatusMaxErrosDiff: %s\n", maxErrs);

	sprintf(strLine, "netStatusMaxErrosDiffDev: %s\n", maxErrsDev);
	fputs(strLine, poolFile);
	printf("netStatusMaxErrosDiffDev: %s\n", maxErrsDev);

	sprintf(strLine, "netStatusMaxCollsDiff: %s\n", maxTrnColls);
	fputs(strLine, poolFile);
	printf("netStatusMaxCollsDiff: %s\n", maxTrnColls);

	sprintf(strLine, "netStatusMaxCollsDiffDev: %s\n", maxTrnCollsDev);
	fputs(strLine, poolFile);
	printf("netStatusMaxCollsDiffDev: %s\n", maxTrnCollsDev);

	sprintf(strLine, "netStatusUpdateTime: %d\n", updateTime);
	fputs(strLine, poolFile);

	fclose(poolFile);
}

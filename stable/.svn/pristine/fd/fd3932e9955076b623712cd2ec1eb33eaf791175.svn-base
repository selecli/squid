//********************************
//Desc:   Detect "NtpDate" response status
//Author: LuoJun.Zeng
//Date :  2006-04-15
//********************************

//**************************************************************
//                Update  Log
//--------------------------------------------------------------
// version 1.4 : 2006-06-12
//       fixed: npt_timeout item name in config file is fixed.
//       fixed by LuoJun.Zeng
//**************************************************************

//**************************************************************
//                Update  Log
//--------------------------------------------------------------
// version 1.4 : 2007-05-09
//       fixed: Include <string.h>.
//       fixed by LuoJun.Zeng
//**************************************************************

#include<stdio.h>
#include<time.h>
#include<string.h>
#include <stdlib.h>

#define VERSION "1.4"

char appName[BUFSIZ];

const char* conf_ntpTimeoutName = "ntp_timeout";
const char* conf_ntpTimeoutNameOld = "npt_timeout";
char conf_ntpTimeout[BUFSIZ];

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

	if ((confFile = fopen("/monitor/etc/monitor.conf", "r")) == NULL)
	{
		perror("monitor.conf");
		exit(1);
	}

	while (fgets(line, BUFSIZ, confFile) != NULL)
	{
		str_ltrim(line, "\t ");
		str_rtrim(line, "\t\r\n ");

		 if (!strncmp(conf_ntpTimeoutName, line, strlen(conf_ntpTimeoutName)))
		 {
			 int i = 0;
			 while (conf_ntpTimeout[i] = line[i + 1 + strlen(conf_ntpTimeoutName)])
			 {
				 i++;
			 }
			 str_ltrim(line, "\t ");
			 str_rtrim(line, "\t\r\n ");
			 //printf("%s\n", conf_ntpTimeout);
		 }
		if (!strncmp(conf_ntpTimeoutNameOld, line, strlen(conf_ntpTimeoutName)))
		 {
			 int i = 0;
			 while (conf_ntpTimeout[i] = line[i + 1 + strlen(conf_ntpTimeoutName)])
			 {
				 i++;
			 }
			 str_ltrim(line, "\t ");
			 str_rtrim(line, "\t\r\n ");
			 //printf("%s\n", conf_ntpTimeout);
		 }

	}
}

main(int argc, char** argv)
{
	FILE* ntpsyncFile;
	FILE* poolFile;
	char line[BUFSIZ];
	char strLine[BUFSIZ];
	int timeNtpChkResult = 0;
	int hasOffset = 0;
	char timeNtpChkOffset[BUFSIZ] = "-1";
	time_t updateTime;
	int years;
	int months;
	int days;
	int hours;
	int minutes;
	int seconds;

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

	if ((ntpsyncFile = fopen("/var/log/chinacache/ntpsync.log", "r")) == NULL)
    {
        perror("ntpsync.log");
        exit(1);
    }

	while(fgets(line, BUFSIZ, ntpsyncFile) != NULL)
	{
		str_ltrim(line, "\t ");
		str_rtrim(line, "\t\r\n ");

		int countLine = 1;
		if (countLine == 1)
		{
			int countWords = 1;
			char * pValue = NULL;
			pValue = strtok(line, " ");
			while (pValue != NULL)
			{
				//printf("%s\n", pValue);

				if (timeNtpChkResult == 1 && hasOffset == 1)
				{
					sprintf(timeNtpChkOffset, "%s", pValue);
					hasOffset = 0;
					//printf("timeNtpChkOffset = %s\n", timeNtpChkOffset);
					str_ltrim(timeNtpChkOffset, "-");
				}

				if (!strcmp(pValue, "offset"))
				{
                    timeNtpChkResult = 1;
					hasOffset = 1;
					//printf("timeNtpChkResult = %d\n", timeNtpChkResult);
				}

				if (countWords == 1)
				{
					days = atoi(pValue);
				}
				if (countWords == 2)
				{
					if (!strcmp(pValue, "Jan"))
					{
						months = 0;
					}
					if (!strcmp(pValue, "Feb"))
					{
						months = 1;
					}
					if (!strcmp(pValue, "Mar"))
					{
						months = 2;
					}
					if (!strcmp(pValue, "Apr"))
					{
						months = 3;
					}
					if (!strcmp(pValue, "May"))
					{
						months = 4;
					}
					if (!strcmp(pValue, "Jun"))
					{
						months = 5;
					}
					if (!strcmp(pValue, "Jul"))
					{
						months = 6;
					}
					if (!strcmp(pValue, "Aug"))
					{
						months = 7;
					}
					if (!strcmp(pValue, "Sep"))
					{
						months = 8;
					}
					if (!strcmp(pValue, "Oct"))
					{
						months = 9;
					}
					if (!strcmp(pValue, "Nov"))
					{
						months = 10;
					}
					if (!strcmp(pValue, "Dec"))
					{
						months = 11;
					}
				}

				if (countWords == 3)
				{
                     char timeWords[BUFSIZ];
                     sprintf(timeWords, "%s", pValue);
                     char cHours[BUFSIZ];
					 strncpy(cHours, timeWords, 2);
					 hours = atoi(cHours);

					 char cMinutes[BUFSIZ];
					 int i = 0;
					 while (i < 2)
					 {
						 cMinutes[i] = timeWords[i + 3];
						 i++;
					 }
					 minutes = atoi(cMinutes);

					 char cSeconds[BUFSIZ];
					 i = 0;
					 while (i < 2)
					 {
						 cSeconds[i] = timeWords[i + 6];
						 i++;
					 }
					 seconds = atoi(cSeconds);
				}

				pValue = strtok(NULL, " ");
				countWords++;
			}
		}//if (count ==1)
		countLine++;
	}//while(fgets(line

	////////////////////////////////////////////////
	//Get Command Start Time
	struct tm tmStart;

	time_t now;
    gettimeofday(&now, NULL);
    struct tm* timetm;
    timetm = localtime(&now);
    years = timetm->tm_year;

	tmStart.tm_hour = hours;
	tmStart.tm_min = minutes;
	tmStart.tm_sec = seconds;
	tmStart.tm_mday = days;
	tmStart.tm_mon = months;
	tmStart.tm_year = years;

    char* resTime;
    resTime = asctime(&tmStart);
	//printf("%s\n", resTime);
	time_t startTime = mktime(&tmStart);
	
	//****************************************************
	//Get UpdateTime
	updateTime = time(NULL);

	if (abs(updateTime - startTime) > atoi(conf_ntpTimeout))
	{
		timeNtpChkResult = -1;
		sprintf(timeNtpChkOffset, "%s", "-1");
	}

	//*******************************************************
	//Write To the squidLogTransMon.pool
	if ((poolFile = fopen("../pool/timeNtpChkMon.pool", "w+")) == NULL)
	{
		perror("timeNtpChkMon.pool");
		exit(1);
	}

	sprintf(strLine, "timeNtpChkResult: %d\n", timeNtpChkResult);
	fputs(strLine, poolFile);

	sprintf(strLine, "timeNtpChkOffset: %s\n", timeNtpChkOffset);
	fputs(strLine, poolFile);

	sprintf(strLine, "timeNtpChkUpdateTime: %ld\n", updateTime);
	fputs(strLine, poolFile);

	fclose(poolFile);
}

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include<unistd.h>
#include<sys/time.h>

static const char helpStr[] = "Help Information append here";

static double timeout = 3.000;

void signalAlarm(int sig)
{
	;
}

void registerSignal()
{
    struct sigaction act;

    act.sa_handler = signalAlarm;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGALRM, &act, 0);
}

void processResult(FILE* digStream, struct timeval stTimeval1)
{
	char buf[BUFSIZ];
	char costtime[64];
	int haveip = 0;
	char ip[20];
	char iplist[1024];
	int answerSection = 0;

	iplist[0] = '\0';
	while(fgets(buf, sizeof(buf), digStream))
	{
		int haveInAndA = 0;
		if('\r' == buf[0] || '\n' == buf[0])
			continue;
		if(';' == buf[0] && ';' == buf[1])
		{
			char* tmpStr = NULL;
			tmpStr = strtok(buf, " \t");
			if(NULL == tmpStr)
				continue;
			tmpStr = strtok(NULL, " \t");
			if(!strcmp("ANSWER", tmpStr))
			{
				answerSection = 1;
			}
			else
			{
				answerSection = 0;
			}
			continue;
		}

		char* str = NULL;
		str = strtok(buf, " \t\r\n");
		while(str != NULL && 1 == answerSection)
		{
			str = strtok(NULL, " \t\r\n");
			if(NULL != str && 2 == haveInAndA)
			{
				//here got the ip
				haveip = 1;
				//printf("%s\t", str);
				snprintf(ip, sizeof(ip), "%s\t", str);
				strncat(iplist, ip, sizeof(ip));
			}

			if(NULL != str && !strcmp("IN", str))
				haveInAndA = 1;
			if(NULL != str && !strcmp("A", str) && 1 == haveInAndA )
				haveInAndA++;
		}
	}
	setitimer(ITIMER_REAL, NULL, NULL);
	if(feof(digStream))
	{
		struct timeval stTimeval2;
		gettimeofday(&stTimeval2, NULL);
		double usedtime = stTimeval2.tv_sec-stTimeval1.tv_sec + (stTimeval2.tv_usec-stTimeval1.tv_usec)/1000000.0;
		snprintf(costtime, sizeof(costtime), "%f", usedtime);
		if(haveip)
			printf("%s %s", costtime, iplist);
		printf("\n");
		fflush(stdout);
	}
	else
	{
		printf("%f timeout\n", timeout);
		fflush(stdout);
	}

}

int processDigCmd(char* line, const struct timeval tv)
{
	char* origDomain;
	origDomain = strtok(line, " \r\n");
	if(NULL == origDomain)
	{
		return -1;
	}
	//printf("processCmd, origDomain:%s\n", origDomain);

	char* origIP;
	origIP= strtok(NULL, " \r\n");
	if(NULL == origIP)
	{
		return -1;
	}
	//printf("processCmd, origIP:%s\n", origIP);

	char digCmd[1024];
	snprintf(digCmd, 1024, "dig @%s %s", origIP, origDomain);
    //printf("processCmd, digCmd:%s\n", digCmd);

	struct itimerval itimer;
	memset(&itimer, 0, sizeof(itimer));
	itimer.it_value.tv_sec = tv.tv_sec;
	itimer.it_value.tv_usec = tv.tv_usec;
	setitimer(ITIMER_REAL, &itimer, NULL);

	struct timeval stTimeval1;
	gettimeofday(&stTimeval1, NULL);
	FILE * digStream = popen(digCmd, "r");

	if(!digStream)
	{
		perror("popen error\n");
		return -1;
	}

	processResult(digStream, stTimeval1);
	//setitimer(ITIMER_REAL, NULL, NULL);

	int closeRet = pclose(digStream);
	if(-1 == closeRet)
	{
	    return -1;
	}
	return 1;
}


int main(int argc, char* argv[])
{
	registerSignal();

	char line[1024];
	char* str;

	if(argc != 1) {
		timeout = atof(argv[1]);
		if(timeout <= 0)
		{
			timeout = 3.000000;
		}
	}

	while(1) {
		str = fgets(line, 1024, stdin);
		if(NULL == str)
		{
			if(0 == feof(stdin))
			{
				continue;
			}
			break;
		}

		if(('#'==line[0]) || ('\r'==line[0]) || ('\n'==line[0]))
		{
			fprintf(stdout, "\n");
			fflush(stdout);
			continue;
		}

		struct timeval tv;
		tv.tv_sec = (int)timeout;
		tv.tv_usec = (int)((timeout-(int)timeout)*1000000);
		int ret = processDigCmd(str, tv);
		if(-1 == ret)
		{
			fprintf(stdout, "\n");
			fflush(stdout);
		}
	}

    return 1;
}

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <AngelWing/AngelWingProcess.h>
#include <AngelWing/AngelWingString.h>
#include <string.h>
#include <signal.h>

#define THREAD_COUNT	256


uint64_t count[THREAD_COUNT] = {0};
bool shutdown = false;


void* ConnecitProcess(void *arg)
{
	int domain = (int)arg;
	int html = domain;

	while( shutdown == false ) {

		html = (html+1)%10;

		char reg[1024];
		memset(reg, 0, 1024);

		sprintf(reg + strlen(reg), "GET /billingtest%d HTTP/1.1\r\n", html);
		sprintf(reg + strlen(reg), "Accept: */*\r\n");
		sprintf(reg + strlen(reg), "Accept-Language: zh-cn\r\n");
		sprintf(reg + strlen(reg), "UA-CPU: x86\r\n");
		sprintf(reg + strlen(reg), "Accept-Encoding: gzip, deflate\r\n");
		sprintf(reg + strlen(reg), "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.2; SV1; .NET CLR 1.1.4322)\r\n");
		sprintf(reg + strlen(reg), "Host: www.xx%d.com\r\n", domain);
		sprintf(reg + strlen(reg), "Connection: Keep-Alive\r\n");
		strcat(reg, "\r\n");

	//	s << reg;

	//	int ret;
	//	ret = s.TCPSendRecv(0, 0, "192.168.6.214", 80, 0);
		int billing_socket = AngelWing::TCPConnect("192.168.6.214", 80, 0);
		if( billing_socket < 0 )
			continue;

		int ret;
		ret = AngelWing::WriteSocket(billing_socket, reg, strlen(reg));

		while( 1 ) {
			ret = AngelWing::ReadSocket(billing_socket, reg, 1024);

			if( ret < 0 ) {
				printf(" ret = %d\n");
				break;
			}

			if( ret == 0 )
				break;

			count[domain] += ret;
		}
	}

}


void sigprog(int signum)
{
	shutdown = true;
}

int main()
{
	signal(SIGINT, sigprog);

	pthread_t ppid[THREAD_COUNT];
	for( int i = 0 ; i < THREAD_COUNT ; i++ ) {

		pthread_create(&ppid[i], NULL, ConnecitProcess, (void*)i);
	}

	while(shutdown == false) {
		sleep(1);
	}


	for( int i = 0 ; i < THREAD_COUNT ; i++ ) {
		pthread_join(ppid[i], NULL);
	}

	for( int i = 0 ;i < THREAD_COUNT ; i++ ) {

		printf("www.xx%d.com  recv = %llu\n", i, (unsigned long long)count[i]);
	}

}

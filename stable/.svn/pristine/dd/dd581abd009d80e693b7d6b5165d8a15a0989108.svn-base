#include "refresh_header.h"
static int port = 2108;
static char *ip = "127.0.0.1";
static int fd;
typedef void test_func();

typedef struct _call_func{
	char *desc;
	test_func *func;
} call_func;

void usage(){
	fprintf(stdout,"usage : test_fc -p [port]\n");
}

static void options_parse(int argc, char **argv)
{
	int c;
	while(-1 != (c = getopt(argc, argv, "p:h:"))) {
		switch(c) {
			case 'p':
				if(optarg) {
					port = atoi(optarg);
				}
				else{
					usage();
					exit(-1);
				}
				break;
			default:
				fprintf(stderr, "Unknown arg: %c\n", c);
				usage();
				exit(2);
			   	break;
		}
	}
}
int rf_write(int fd,char *buffer,uint32_t len){
	int s_len = 0;
	int r_len = len;

	while(r_len > 0){
		s_len = write(fd,buffer,r_len);
		if(s_len > 0 ){
			r_len -= s_len;
			continue;
		}
		
		if(s_len == -1){
			fprintf(stderr,"write error : %s\n",strerror(errno));
			return s_len;
		}
	}

#if 0
	write(2,buffer,len);
#endif
	return r_len;
}

int fd_connect(){
	struct sockaddr_in  maddress;
	memset(&maddress, 0, sizeof(struct sockaddr_in));
	if(inet_aton(ip,&maddress.sin_addr) == 0)
		return -1;

	maddress.sin_family = AF_INET;
	maddress.sin_port   = htons(port);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if( fd < 0 ) {
		return -1;
	}

	int n = 0;
	if( (n = connect(fd, (struct sockaddr *)&maddress, sizeof(struct sockaddr))) != 0) {
		perror("connect:");
		return -1;
	}

	return 0;
}

void test_add_url(){
	uint32_t num = 0;
	time_t start,now;
	start = now = time(NULL);

	char buffer[1024];
	unsigned long index = 0;
	while(1){
		memset(buffer,0,1024);
		char *wb = buffer;

		int len = 0;
		len += sprintf(wb + len,"squid.pid");	len++;
		len += sprintf(wb + len,"1");		len++;
		len += sprintf(wb + len,"XXXXXXXX-%lu",index);	len++;
		len += sprintf(wb + len,"http://wwww.hutu.com\n");
		rf_write(fd,buffer,len);
		num++;

		now = time(NULL);
		if( (now != start) && (((now - start) % 60) == 0)){
			fprintf(stderr,"%d times / min\n",num);
			num = 0;
			start = now;
		}

		index++;
//		usleep(100000);
	}
}

void test_read_data(){
	fprintf(stderr,"call test_read_data ..\n");

	char buffer[10240];
	int len = 0;
	len = recv(fd,buffer,10240,0);

	if(len > 0){
		fprintf(stderr,"recv OK!\n");
		write(2,buffer,len);
	}
	else if(len == 0){
		fprintf(stderr,"peer close the socket!\n");
	}
	else{
		fprintf(stderr,"recv failed!\n");
	}
}


int main(int argc,char *argv[]){
	options_parse(argc,argv);

	int ret = fd_connect();
	if(ret != 0){
		fprintf(stderr,"connect refreshd error!");
		exit(-1);
	}

	call_func func_array[] = {
		{"test_read_data",test_read_data},
		{"test_add_url",test_add_url},
		{NULL,NULL}
	};
	
	int index = 0;
	for(;func_array[index].desc != NULL;index++){
		printf("%d - %s\n",index,func_array[index].desc);
	}

	int number;
	scanf("%d",&number);

	(func_array[number].func)();
	return 0;
}


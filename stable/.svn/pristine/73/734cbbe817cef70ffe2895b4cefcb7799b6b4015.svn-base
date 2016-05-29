#include "refresh_header.h"
#include <netdb.h>

#define L_BUFFER_LEN 204800
#define M_BUFFER_LEN 2048
#define S_BUFFER_LEN 128

static int port = 21108;
static char ip[S_BUFFER_LEN] = "127.0.0.1";
static int fd;

void usage(){
	fprintf(stdout,"usage : refresh_cli\n"
		       	"\t-h [ip or hostname] \n"
			"\t-p [port] \n"
			);
}

struct in_addr * atoaddr(char *address) {
	struct hostent *host;
	static struct in_addr saddr;

	saddr.s_addr = inet_addr(address);
	if (saddr.s_addr != -1) {
		return &saddr;
	}
	host = gethostbyname(address);
	if (host != NULL) {
		return (struct in_addr *) *host->h_addr_list;
	}
	return NULL;
}

static void options_parse(int argc, char **argv)
{
	int c;
	while(-1 != (c = getopt(argc, argv, "kp:h:"))) {
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
			case 'h':
				if(optarg){
					memset(ip,0,S_BUFFER_LEN);
					strcpy(ip,optarg);
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

	return r_len;
}

int rf_read(int fd){
	static char buffer[L_BUFFER_LEN];
	memset(buffer,0,L_BUFFER_LEN);

	int len = 0;
	while((len = recv(fd,buffer,L_BUFFER_LEN,0)) != 0){
		if(len > 0){
			write(2,buffer,len);
		}
		else{
			fprintf(stderr,"recv error!");
			break;
		}
	}

	return 0;
}

int fd_connect(){
	struct sockaddr_in  maddress;
	memset(&maddress, 0, sizeof(struct sockaddr_in));
	struct in_addr *ina = atoaddr(ip);
	if(ina == NULL)
		return -1;

	maddress.sin_addr = *ina;
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

static char * get_fc_ident(){
	return "test";
}

static void rf_free_wb(rf_write_buffer *wb){
	free(wb->buffer);
	free(wb);
}

static rf_write_buffer * rf_cmd_gen_wl(int action,char *md5,char *other){
	rf_write_buffer *wb = malloc(sizeof(rf_write_buffer));
	memset(wb,0,sizeof(rf_write_buffer));
	wb->buffer = malloc(M_BUFFER_LEN);
	memset(wb->buffer,0,M_BUFFER_LEN);

	char *fc_id = get_fc_ident();
	wb->len += sprintf(wb->buffer + wb->len,"%s",fc_id);    wb->len++;
	wb->len += sprintf(wb->buffer + wb->len,"%d",action);   wb->len++;

	if(md5 != NULL){
		wb->len += sprintf(wb->buffer + wb->len,"%s",md5);
	}
	wb->len++;      //skip '\0'

	if(other != NULL){
		wb->len += sprintf(wb->buffer + wb->len,"%s",other);
	}
	wb->len++;      //skip '\0'

	wb->len += sprintf(wb->buffer + wb->len,"\n");

	return wb;
}

int main(int argc,char *argv[]){
	options_parse(argc,argv);
	fd_connect();

	char buff[M_BUFFER_LEN];
	memset(buff,0,M_BUFFER_LEN);

	time_t ti = time(NULL);


	rf_write_buffer *wb = rf_cmd_gen_wl(RF_FC_ACTION_INFO,NULL,NULL);
	rf_write(fd,wb->buffer,wb->len);
	rf_free_wb(wb);

	int i = 0;
	int j = 0;
	while(++i){
		sprintf(buff,"http://www.chinacache.com/logo.gif?%d",i);
		wb = rf_cmd_gen_wl(RF_FC_ACTION_ADD,buff,buff);
		rf_write(fd,wb->buffer,wb->len);
		rf_free_wb(wb);

		time_t now = time(NULL);

		if((now - ti) > 10){
			fprintf(stderr,"total : %d\nlast 10sec : %d\n\n",i,i-j);
			ti = now;
			j = i;
		}
	}

	return 0;
}

#ifndef DETECT_STRUCTS_H
#define DETECT_STRUCTS_H

typedef void LLIST;
typedef struct comm_st comm_t;
typedef struct code_st code_t;
typedef struct codes_st codes_t;
typedef struct detect_st detect_t;
typedef struct data_ip_st data_ip_t;
typedef struct keyword_st keyword_t;
typedef struct data_ips_st data_ips_t;
typedef struct commdata_st commdata_t;
typedef struct data_attr_st data_attr_t;
typedef struct detect_ip_st detect_ip_t;
typedef struct type_size_st type_size_t;
typedef struct data_index_st data_index_t;
typedef struct detect_cfg_st detect_cfg_t;
typedef struct detect_way_st detect_way_t;
typedef struct detect_once_st detect_once_t;
typedef struct http_header_st http_header_t;
typedef struct detect_data_st detect_data_t;
typedef struct detect_unit_st detect_unit_t;
typedef struct ctl_keyword_st ctl_keyword_t;
typedef struct hash_factor_st hash_factor_t;
typedef struct http_headers_st http_headers_t;
typedef struct detect_index_st detect_index_t;
typedef struct detect_event_st detect_event_t;
typedef struct detect_output_st detect_output_t;
typedef struct config_domain_st config_domain_t;
typedef struct detect_config_st detect_config_t;
typedef struct detect_socket_st detect_socket_t;
typedef struct config_control_st config_control_t;

typedef int CKCB(char *str);                          //check keyword callback
typedef void TRCB(void *data);                        //event travel callback
typedef int SKCB(const int sockfd);                   //send or receive callback
typedef int COMP(void *key, void *data);              //compare
typedef void EWCB(const int type, const int sockfd);  //epoll wait callback
typedef void CMCB(const int sockfd, const int type);  //communication callback
typedef void CNCB(const int sockfd, const int error); //connect callback

struct type_size_st {                         //type size
	uint16_t code_t_size;                     //size of code_t    
	uint16_t codes_t_size;                    //size of codes_t
	uint16_t data_ip_t_size;                  //size of data_ip_t
	uint16_t sockaddr_in_size;                //size of struct sockaddr_in
	uint16_t detect_ip_t_size;                //size of detect_ip_t
	uint16_t data_index_t_size;               //size of data_index_t
	uint16_t detect_cfg_t_size;               //size of detect_cfg_t
	uint16_t hash_factor_t_size;              //size of hash_factor_t
	uint16_t ctl_keyword_t_size;              //size of ctl_keyword_t
	uint16_t http_header_t_size;              //size of http_header_t
	uint16_t detect_unit_t_size;              //size of detect_unit_t
	uint16_t detect_event_t_size;             //size of detect_event_t
	uint16_t detect_config_t_size;            //size of detect_config_t
	uint16_t detect_output_t_size;            //size of detect_output_t
};

struct data_attr_st {                         //attribute
	uint32_t grow_step;                       //grow size 
	uint32_t current_num;                     //current number of the XXX
	uint32_t max_capacity;                    //the max capacity of XXX
};

struct config_domain_st {                     //config data structure
	int mode;                                 //mode for FC or TTA
	char dnsIpFile[BUF_SIZE_256];             //file of dns resolved ip
	char domainFile[BUF_SIZE_256];            //domain.conf
	char ttaConfFile[BUF_SIZE_256];           //tta_domain.conf
};

struct ctl_keyword_st {                       //keyword 
	char keyword[BUF_SIZE_256];               //keyword name
	CKCB *handler;                            //keyword handler
};

struct keyword_st {
	int num;                                  //keywords number
	ctl_keyword_t *keywords;                  //keywords array
};

struct config_control_st {
	char ctlFile[BUF_SIZE_256];               //detectorig.conf
	keyword_t keys;                           //keys
};

struct detect_config_st {
	config_domain_t domain;                   //domain configurations
	config_control_t ctl;                     //control configurations
};

struct detect_event_st {
	int flag;                                 //status flag
	detect_ip_t *dip;                         //rfc: detect_ip_t
	detect_unit_t *unit;                      //rfc: detect_unit_t
};

struct code_st {
	char code[4];                             //response code
};

struct codes_st {
	int num;                                  //successful response codes number
	code_t *code;                             //successful response codes set
};

struct detect_socket_st {
	int sockfd;                               //save socket fd which created
	int domain;                               //communication domain
	int socktype;                             //communication semantics
	int protocol;                             //communication protocol
	char *common_buffer;                      //common detect buffer
	char *referer_buffer;                     //referer detect buffer
	size_t common_buflen;                     //common buffer length
	size_t referer_buflen;                    //referer buffer length
	int sockaddr_len;                         //IPv4 address length
	struct sockaddr_in sockaddr;              //IPv4 address
	CNCB *handler;                            //callback after connected
};

struct commdata_st {                          //communication data
	int flag;                                 //flag: initialized or not
	void *buffer;                             //communication data buffer
	size_t bufsize;                           //communication  data buffer size
	size_t offset;                            //handled data length
	size_t curlen;                            //current handled data length
	CMCB *handler;                            //callback for communication
};

struct detect_once_st {                       //for central detect
	int nodetect;                             //mark for: detect or not
	detect_ip_t *rfc;                         //reference to the same ip
};

struct comm_st {                              //communication
	int flag;                                 //flag: 
	commdata_t in;                            //communication data recieved
	commdata_t out;                           //communication data sent
};

struct hash_factor_st {
	void *rfc;                                //reference to XXX
	uint32_t ftimes;
	char ip[BUF_SIZE_16];                     //ip
	char uri[BUF_SIZE_1024];                  //uri
	struct hash_factor_st *next;              //next
};

struct detect_unit_st {
	int ok;                                   //detect success or not
	int flag;                                 //flag: READY,CONNECT,SEND,RECV,DONE
	int type;
	int sockfd;                               //socket fd for current IP detect
	int pstate;                               //port state: 0: DOWN; 1: WORK
	int code_ok;                              //if parsed response code or not
	int inprogress;                           //connect in progressing
	char code[4];                             //response code
	double cntime;                            //connect start time
	double cdtime;                            //connected time
	double cutime;                            //connect used time
	double sdtime;                            //send data time
	double fbtime;                            //first byte time
	double edtime;                            //end time
	comm_t comm;                              //communication data
	SSL *ssl;                                 //ssl for https
	SSL_CTX *ctx;                             //ctx for https
	/*
	   struct {
	   unsigned int ok:1;
	   unsigned int type:4;
	   unsigned int status:4;
	   unsigned int pstate:1;
	   unsigned int inprogress:1;
	   } flags;
	 */
};

struct detect_output_st {
	uint8_t times;                            //detect times
	uint8_t oktimes;                          //detect success times
	uint16_t port;                            //port of detect
	double cutime;                            //connect used time
	double fbtime;                            //first byte recieved time
	double dltime;                            //download used time
	double ustime;                            //detect used time
	char *dway;                               //detect way: TCP,HTTP,HTTPS
	char *iptype;                             //which IP group: IP, Backup IP or DNS IP
	char *pstate;                             //port state: work, or down
	char *detect;                             //0: no detect; 1: detect; 2: reuse last
	char *hostname;                           //hostname
	char code[4];                             //response code
	char lip[BUF_SIZE_16];                    //localhost IP
	char sip[BUF_SIZE_16];                    //server IP
};

struct detect_ip_st {                         //one IP infomation for detecting
	int index;                                //index 
	uint8_t times;                            //detect times
	uint8_t iptype;                           //ip type: IP, Backup IP, DNS IP
	char ip[BUF_SIZE_16];                     //IPv4 dotted decimal
	detect_output_t dop;                      //struct for link.data output
	detect_once_t once;                       //only detect once when more than one same ip
	detect_socket_t socket;                   //socket infomation
	detect_unit_t *units;                     //pointer of time array
	detect_cfg_t *rfc;                        //pointer of relevance domain
};

struct data_ip_st {
	int iptype;                               //ip type: ip|bkip|dnsip
	char ip[BUF_SIZE_16];                     //ip
	detect_ip_t *rfc;                         //reference to dip
};

struct data_ips_st {
	int num;                                  //ip number
	data_ip_t *ip;                            //ip set
};

struct http_header_st {                       //for customized header
	int repeat;                               //check whether header repeat or not
	char name[BUF_SIZE_512];                  //header name
	char value[BUF_SIZE_1024];                //header value
};

struct http_headers_st {
	int num;                                  //headers number
	http_header_t *hdrs;                      //header set
};

struct method_st {
	char *posttype;                           //post file type
	char *hostname;                           //hostname
	char *postfile;                           //post file
	char *filename;                           //filename
	uint8_t method;                           //method: GET|HEAD|POST
};

struct detect_cfg_st {
	char *hostname;                           //hostname
	char *backup;                             //backup alias
	uint8_t cwt;                              //common detect weight
	uint8_t rwt;                              //referer detect weight
	uint8_t times;                            //detect times
	uint8_t ngood;                            //good ip number
	uint16_t port;                            //port of detect
	uint16_t dns_times;                       //DNS resolved maximum failed times
	uint16_t tcp_itime;                       //interval time of TCP detect: MINUTES
	uint16_t http_itime;                      //interval time of HTTP detect: MINUTES
	size_t rlength;                           //request length getting from source server
	double timeout;                           //detect timeout
	double wntime;                            //warning time
	double gdtime;                            //good time
	struct method_st *method1;                //request method1
	struct method_st *method2;                //request method2
	codes_t codes;                            //successful codes
	data_ips_t ips;                           //detect ip
	http_headers_t hdrs;                      //custom headers
	struct {
		unsigned int sort:1;                  //sort ip by detect time
		unsigned int indns:1;                 //domain.conf's hostname in dns or not
		unsigned int usedns:1;                //use dns or not
		unsigned int detect:2;                //0: no detect; 1: detect; 2: reuse last
		unsigned int referer:1;               //use common or refrerer
		unsigned int protocol:2;              //protacol: TCP, HTTP, HTTPS
	} flags;
};

struct data_index_st {
	int inuse;                                //whether in using or not
	detect_event_t *event;                    //event
};

struct detect_index_st {
	int max_sockfd;                           //the max socket fd
	data_attr_t attr;                         //data attribution
	data_index_t *idx;                        //socket fd index table
};

struct detect_data_st {
	LLIST *cllist;                            //cfg link list
	LLIST *pllist;                            //ip link list
	LLIST *ellist;                            //event link list
	detect_index_t index;                     //index table of pllist: index by sockfd
};

struct detect_option_st {
	int travel;                               //travel number
	int epollwait;                            //epoll_wait timeout
	double timeout;                           //how long to disconnect one connection
	time_t killtime;                          //how long to kill current program instance
	time_t savetime;                          //how long to save link.data files
	char *DNSservice;                         //DNS service path
	rlim_t rlimit;                            //resource limits
	struct {
		unsigned int print:1;                 //print debug information
		unsigned int result:1;                //update anyhost or other
		unsigned int dlstart:1;               //delay start
		unsigned int central:1;               //detect once when same ip and uri
		unsigned int cleanup:1;               //clean up or not[on: yes; off: no]
		unsigned int dblevel:4;               //debug level: current max level 4
	} F;
	int check_flag;
};

struct detect_runstate_st {
	int pid;                                  //proccess identifier
	int state;                                //run state
	uint32_t runtimes;                        //run times since first install
	uint64_t count;                           //ip detect count
	time_t endtime;                           //detect end time
	time_t lasttime;                          //last run time
	time_t starttime;                         //current program instance start time
};

struct detect_other_st {
	FILE *logfp;                              //log file handler
	FILE *lastfp;                             //last link.data.timestamp fp
	char localaddr[BUF_SIZE_16];              //local host address
};

struct detect_st {
	type_size_t tsize;                        //type size
	detect_data_t data;                       //detect data
	detect_config_t config;                   //detect configuration
	struct detect_other_st other;             //detect other information
	struct detect_option_st opts;             //detect command options
	struct detect_runstate_st runstate;       //detect run state
};

#endif

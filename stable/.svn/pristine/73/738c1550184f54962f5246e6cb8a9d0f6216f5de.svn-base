#ifndef DETECT_ENUMS_H
#define DETECT_ENUMS_H

enum log {                   //here must begin from 0, and order can't changed
	COMMON = 0,              //log info without any keyword
	WARNING,                 //log info begin with keyword "WARNING"
	ERROR,                   //log info begin with keyword "ERROR"
	FATAL,                   //log info begin with keyword "FATAL"
	LOG_LAST                 //end
};

enum method {                //here must begin from 0, and order can't changed
	METHOD_GET = 0,          //0: request type 'GET'
	METHOD_POST,             //1: request type 'POST'
	METHOD_HEAD,             //2: request type 'HEAD'
	METHOD_ONE,              //method 1
	METHOD_TWO,              //method 2
	METHOD_NONE              //none
};

enum iptype {
	IPTYPE_IP = 0,           //type: ip
	IPTYPE_DNS,              //type: dns
	IPTYPE_BACKUP            //type: backup
};

enum socket {
	SOCK_SEND,               //send data
	SOCK_RECV,               //receive data
	SOCK_GETFD,              //get socket fd
	SOCK_VALID,
	SOCK_ADDR_REUSE,         //set address reuse
	SOCK_BLOCKING,           //set socket fd non-blocking
	SOCK_NONBLOCKING         //set socket fd non-blocking
};

enum epoll {
	EPOLL_IN,                //socket fd can read
	EPOLL_OUT,               //socket fd can write
	EPOLL_ADD,               //epoll add
	EPOLL_MOD,               //epoll modify
	EPOLL_DEL,               //epoll delete
	EPOLL_HUP,               //epoll hang up
	EPOLL_ERR,               //epoll error
	EPOLL_WAIT,              //epoll wait
	EPOLL_CREATE,            //epoll initialize
	EPOLL_DESTROY            //epoll destroy
};

enum object {
	OBJ_ADD,                 //add object into link list
	OBJ_MOD,                 //modify object in link list
	OBJ_DEL,                 //delete object from link list
	OBJ_FIND,                //find object in link list
	OBJ_NUMBER,              //get link list node number
	OBJ_CREATE,              //create link list
	OBJ_TRAVEL,              //travel link list
	OBJ_TRAVEL2,             //travel link list
	OBJ_ISEMPTY,             //check link list is empty or not
};

enum state {
	STATE_START,             //prepare for detect, like: open log, check or write PID file
	STATE_CONFIG,            //handle configuration
	STATE_DETECT,            //detect
	STATE_OUTPUT,            //handle output data 
	STATE_RESULT,            //create new anyhost
	STATE_FINISHED,          //destruct, like: free the alloc memory, close file handler, and so on
	STATE_TERMINATE,         //terminate detect program
	CONFIG_CHECK
};

enum start {
	START_GLOBAL,
	START_CONFIG,
	START_DETECT,
	START_OUTPUT,
	START_RESULT,
	START_DESTROY
};

enum protocol {
	PROTOCOL_NONE = 0,
	PROTOCOL_TCP = 1,
	PROTOCOL_HTTP = 2,
	PROTOCOL_HTTPS = 3
};

enum protocol_port {
	PROTO_HTTP = 80,
	PROTO_HTTPS = 443
};

enum dway {
	DWAY_COMMON,             //common detect
	DWAY_REFERER             //referer detect
};

enum time {
	TIME_TIME,               //time format
	TIME_TIMEVAL             //timeval format
};

enum port {
	PORT_WORK,
	PORT_DOWN
};

enum comm_time {
	COMM_CNTIME,             //connect start time
	COMM_CDTIME,             //connect end time
	COMM_SDTIME,             //socket send time
	COMM_FBTIME,             //first byte time
	COMM_EDTIME              //communaction end time
};

enum comm_state {
	READY,
	CONNECTING,
	CONNECTED,
	SENDING,
	SENT,
	RECVING,
	RECV_REDO,
	RECVED,
	FIRST_BYTE,
	OTHER_BYTE,
	FINISHED,
	DONE
};

enum print {
	PRINT_START,
	BEFORE_GLOBAL,
	AFTER_GLOBAL,
	BEFORE_PARSE,
	AFTER_PARSE,
	BEFORE_DETECT,
	AFTER_DETECT,
	BEFORE_OUTPUT,
	AFTER_OUTPUT,
	BEFORE_READY,
	AFTER_READY,
	BEFORE_CLEAN,
	AFTER_CLEAN,
	BEFORE_RESULT,
	AFTER_RESULT
};

enum llist {
	HEAD,
	TAIL
};

enum flags {
	ADDR,
	DATA,
	NOFD,
	SUCC,
	FAILED
};

#endif

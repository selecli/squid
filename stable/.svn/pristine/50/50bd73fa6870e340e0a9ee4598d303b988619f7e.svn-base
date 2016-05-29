#ifndef QUEUE_H
#define QUEUE_H
#include "h264_streamd.h"
//#include "define.h"
//#include "log.h"

#define REQ_SIZE 2048
#define REP_SIZE 4096
typedef enum {
	QUEUE_EMPTY,
	QUEUE_REGULAR,
	QUEUE_FULL
} queue_flag;

struct req_queue_con {
	int fd;
	struct mp4_req *mp4_ptr;
//	void *req_data;
};

struct rep_queue_con {
	int fd; 
	void *rep_data;
};

struct type_trans {
	char type[32];
	void **ptr;
};

extern struct req_queue_con **req_queue_head;
extern struct req_queue_con **req_queue_rear;
extern struct rep_queue_con **rep_queue_head;
extern struct rep_queue_con **rep_queue_rear;
extern struct req_queue_con *req_queue[REQ_SIZE];
extern int req_queue_count;
extern struct rep_queue_con *rep_queue[REP_SIZE];
extern int rep_queue_count;
extern void  queue_init();
extern queue_flag req_queue_test();
extern queue_flag rep_queue_test();
extern void req_queue_add(struct req_queue_con *elem);
extern void rep_queue_add(struct rep_queue_con *elem);
extern  struct req_queue_con * req_queue_del();
extern  struct rep_queue_con * rep_queue_del();
extern void req_queue_trace();

/*
void  queue_init() 
{
	req_queue_head = req_queue_rear = &req_queue[0];
	rep_queue_head = rep_queue_rear = &rep_queue[0];
}

queue_flag req_queue_test()
{
	if (req_queue_head == req_queue_rear)
		return QUEUE_EMPTY;
	if ((req_queue_count+1) == REQ_SIZE)
		return QUEUE_FULL;
	return QUEUE_REGULAR;
}

queue_flag rep_queue_test() 
{
	if (rep_queue_head == rep_queue_rear)
		return QUEUE_EMPTY;
	if((req_queue_count +1) == REP_SIZE)
		return QUEUE_FULL;
	return QUEUE_REGULAR;
}

void req_queue_add(struct req_queue_con *elem)
{
	*req_queue_rear = elem;
	if (req_queue_rear == &req_queue[REQ_SIZE-1])
		req_queue_rear = &req_queue[0];
	else 
		req_queue_rear++; 
	req_queue_count++;
}

void rep_queue_add(struct rep_queue_con *elem)
{
	*rep_queue_rear = elem;
	if (rep_queue_rear == &rep_queue[REP_SIZE-1])
		rep_queue_rear = &rep_queue[0];
	else 
		rep_queue_rear++;
	rep_queue_count++;
}

struct req_queue_con * req_queue_del()
{
	struct req_queue_con *cur = *req_queue_head;
	if(req_queue_head == &req_queue[REQ_SIZE-1])
		req_queue_head = &req_queue[0];
	else
		req_queue_head++;
	req_queue_count--;
	return cur;
}

struct rep_queue_con * rep_queue_del()
{
	struct rep_queue_con *cur = *rep_queue_head;
	if(rep_queue_head == &rep_queue[REP_SIZE-1])
		rep_queue_head = &rep_queue[0];
	else
		rep_queue_head++;
	rep_queue_count--;
	return cur;
}

void req_queue_trace()
{
//	int i = 0;
	struct req_queue_con **cur = req_queue_head;
	while (cur != req_queue_rear) { 
	//	printf("req_queue: %d: cur->fd %d, cur->req_data %s\n", i++, (*cur)->fd, (char *)(*cur)->req_data);
		if(cur == &req_queue[REQ_SIZE-1])
			cur = &req_queue[0];
		else
			cur++;
	}
}
*/
#endif

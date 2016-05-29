#include "queue.h"

struct req_queue_con **req_queue_head = NULL;
struct req_queue_con **req_queue_rear = NULL;
struct rep_queue_con **rep_queue_head = NULL;
struct rep_queue_con **rep_queue_rear = NULL;
struct req_queue_con *req_queue[REQ_SIZE];
int req_queue_count = 0;
struct rep_queue_con *rep_queue[REP_SIZE];
int rep_queue_count = 0;

void queue_init() 
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
//	do_log(LOG_DEBUG," req_queue_add: accept a request elem->fd %d\n", elem->fd);
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
//	do_log(LOG_DEBUG," req_queue_del\n");
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



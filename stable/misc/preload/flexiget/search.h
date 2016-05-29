#ifndef __SEARCH_H
#define __SEARCH_H



typedef struct
{
	char url[MAX_STRING];
	double speed_start_time;
	int speed, size;
	pthread_t speed_thread[1];
	conf_t *conf;
} search_t;

int search_makelist( search_t *results, char *url );
int search_getspeeds( search_t *results, int count );
void search_sortlist( search_t *results, int count );


#endif


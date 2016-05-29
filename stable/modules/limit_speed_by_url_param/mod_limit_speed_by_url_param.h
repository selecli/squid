#ifndef MOD_INDIVIDUAL_LIMIT_SPEED_H
#define MOD_INDIVIDUAL_LIMIT_SPEED_H
#include <time.h>

typedef struct _speed_control
{
    const char *sc_param_str;
	int init_bytes;
	int restore_bps;
	int max_bytes;
	time_t now;
	int last_bytes;
	int no_limit_size;
	int no_limit_bytes;
}speed_control;

typedef struct _individual_speed_control
{   
    const char *param_str;
	int init_bytes;
	int restore_bps;
	int max_bytes;
	int no_limit_bytes;
}individual_speed_control;

struct cc_defer_fd_st {
	int is_client;
	int fd; 
	void * data; 
	PF *resume_handler;
};


typedef struct _mod_config
{
	individual_speed_control *limit_speed_individual;
}mod_config;

typedef struct _limit_speed_private_data
{
	speed_control *sc;
	int client_write_defer;
}limit_speed_private_data;

#endif

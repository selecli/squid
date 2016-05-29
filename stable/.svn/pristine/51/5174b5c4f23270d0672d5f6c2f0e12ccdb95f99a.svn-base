#ifndef MOD_INDIVIDUAL_LIMIT_SPEED_H
#define MOD_INDIVIDUAL_LIMIT_SPEED_H

#include <time.h>
#include "mod_limit_hashtable.h"

typedef struct _individual_speed_control
{
	int init_bytes;
	int restore_bps;
	int max_bytes;
} individual_speed_control;

typedef struct _mod_config
{
	individual_speed_control *limit_speed_individual;
	// host speed limit
	wordlist *host_list;
} mod_config;

// fd_table
struct cc_defer_fd_st {
	short is_client;
	int fd; 
	void * data; 
	PF *resume_handler;
	// host speed limit
	struct hash_entry *host_node;
};

typedef struct _speed_control
{
	int init_bytes;
	int restore_bps;
	int max_bytes;
	time_t now;
	int last_bytes;
} speed_control;

typedef struct _limit_speed_private_data
{
	speed_control *sc;
	int client_write_defer;
} limit_speed_private_data;

#endif

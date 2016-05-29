#ifndef __STRUCTS_H__
#define __STRUCTS_H__

struct _Config
{
	enum_cache_type cache_type; // squid, lighttpd, ncache ...
	char * cache_path;
	char * cache_conf_path;
	int cache_processes;
	int cc_multisquid;

	enum_lb_type lb_type; // haproxy, nginx ...
	char * lb_path;
	char * lb_conf_path;
	int log_level;
	int kill_port;
	int *cache_ports;
    int ports_num;
	char * need_free_ptr[5];
	int need_free_pos;
    int watch_interval;
    int watch_timeout;
};
typedef struct _Config Config;

struct _CacheManager
{
	pid_t pids[MAX_CACHE_PROCS];
	int num_pids;
	STARTER *starter;
	RELOADER *reloader;
	TERMINATOR *terminator;
	CHECK_RESTARTER *checkRestarter;
};

typedef struct _CacheManager CacheManager;

struct _LbManager
{
	pid_t *pids;
	STARTER *starter;
	RELOADER *reloader;
	TERMINATOR *terminator;
	CHECK_RESTARTER *checkRestarter;
};

typedef struct _LbManager LbManager;

struct _WatchManager
{
	pid_t pids[MAX_CACHE_PROCS];
    int num_pids;
	STARTER *starter;
	TERMINATOR *terminator;
	CHECK_RESTARTER *checkRestarter;
};

typedef struct _WatchManager WatchManager;

#endif

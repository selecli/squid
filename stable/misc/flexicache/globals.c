#include "flexicache.h"

Config g_config;
FILE *g_debug_log = NULL;
char *g_version_string = "7.0.0";
CacheManager g_cache_manager;
LbManager g_lb_manager;
WatchManager g_watch_manager;
char **g_watch_argv;

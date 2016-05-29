#ifndef __CACHE_MANAGER_H__
#define __CACHE_MANAGER_H__

void cacheManagerInit();
void cacheStart();
void cacheCheckRestart(pid_t pid);
void cacheTerminate();
void cacheReconfigure();
#endif

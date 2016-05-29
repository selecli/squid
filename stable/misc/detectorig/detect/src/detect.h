#ifndef NEW_DETECT_H
#define NEW_DETECT_H

#include "include/include.h"

extern detect_t detect;                          //primary data variable
extern hash_factor_t **hashtable;
extern hash_factor_t **hashtable1;
extern const char *methods[];

//INTERFACE: call
int configCall(const int calltype);
int detectCall(const int calltype);
int outputCall(const int calltype);
int resultCall(const int calltype);
int globalCall(const int calltype, int argc, char **argv);
int socketCall(const int calltype, const int sockfd, detect_socket_t *socket);
int epollCall(const int calltype, const int sockfd, const int event, EWCB *callback);
//INTERFACE: global.c
void printVersion(void);
//INTERFACE: config.c
int keepLastResult(detect_ip_t *dip);
//INTERFACE: log.c
void openLogFile(void);
void closeLogFile(void);
void detectlog(const int type, const int sockfd, const char *filename, const int nline, char *format, ...);
//INTERFACE: misc.c
void xfclose(FILE **fp);
void hashtableCreate(void);
void hashtableCreate1(void);
void getLocalAddress(void);
void setResourceLimit(void);
void runningSched(const int interval);
void writeDetectIdentify(const int type);
void xtime(const int type, void *curtime);
void xfree(void **ptr);
void *xmalloc(size_t size);
void *xcalloc(size_t n, size_t sz);
void *xrealloc(void *ptr, size_t size);
void *xstrdup(const char *src);
void *xstrndup(const char *src, size_t n);
void xabort(const char *info);
void debug(const int type);
int xisspace(const char c);
const char *xerror(void);
FILE *xfopen(const char *path, const char *mode);
unsigned int hashKey(const void *value, unsigned int size);
void outputGetDway(detect_ip_t *dip);
void outputGetIpType(detect_ip_t *dip);
//INTERFACE: cleaner.c
void cleaner(time_t cln_timestamp);
//INTERFACE: https.c
int httpsConnect(const int sockfd);
void httpsSendData(const int sockfd);
void httpsRecvData(const int sockfd);
void httpsClose(const int sockfd);
//INTERFACE: llist.c
LLIST *llistCreate(int size);
int llistNumber(LLIST *head);
int llistEmpty(LLIST *head);
int llistIsEmpty(LLIST *head);
void llistDestroy(LLIST *head);
void *llistInsert(LLIST *head, void *data, int mode);
int llistDelete(LLIST *head, void *key, COMP *cmp);
void *llistFind2(LLIST *head, void *key, COMP *cmp);
int llistFind(LLIST *head, void *key, COMP *cmp, void *ret);
int llistTravel(LLIST *head, TRCB *travel);
void llistTravel2(LLIST *head, TRCB *travel, int number);
int llistModify(LLIST *head, void *key, COMP *cmp, void *modify);

int write_rcms_log(const char *filename, char *content);
#endif


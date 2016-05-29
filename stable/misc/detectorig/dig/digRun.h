#ifndef DETECT_ORIG_H
#define DETECT_ORIG_H

#define DIGRUN_RCMS_LOG "/var/log/chinacache/digRunConfigError.log"

void initLog(const char* processName);
void addOneLog(const char* str);
//int getDetectOrigin(FILE* fp);
//int getOrigDomainConf(FILE* fp);
int getOrigDomainConf(FILE* fp, int check_flag);
int getOrigDomainHistory(FILE* fp);
void printDetectOrigin();
//int detectOrigin(int processNumber, const char* processName, double timer);
void modifyUsedTime();
int getNewAnyhost(int info, FILE* fpNamed, FILE* fpNew);
int detect(int processNumber1, char* processName1, double timer1);
int addResultLog(void);
int addInfoLog(int type, const char* str);
int addAlertLog(int type, char* host, char* ipBuf);
int writeOrigDomainIP(void);
void clean_dig_run_mem(void);
#endif	//DETECT_ORIG_H

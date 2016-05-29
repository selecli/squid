/*
 * =====================================================================================
 *
 *       Filename:  dbg.h
 *
 *    Description:  
 *
 *        Created:  03/15/2012 12:12:57 PM
 *       Compiler:  gcc
 *         Author:  Xu Jian (XJ), xujian@chinacache.com
 *        COMPANY:  ChinaCache
 *
 * =====================================================================================
 */
#ifndef _DBG_H_1DAC920E_1822_4D46_9324_68222C85412B_
#define _DBG_H_1DAC920E_1822_4D46_9324_68222C85412B_ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
extern int debug_level;
void SetDebugOutputToStderr( void );
int SetDebugLevel( int level );
int _Debug( const char *format, ... );
#define Debug(level,format,args...) do{\
        if (level <= debug_level) {\
                    time_t t;\
                    char time_string[32] = "";\
                    time(&t);\
                    strftime(time_string,32,"%F %T",localtime(&t));\
                    if (debug_level >= 40)\
                        _Debug("%s %d %s:%d "format"\n",\
                                            time_string, level, __FILE__, __LINE__,\
                                            ##args);\
                    else\
                        _Debug("%s %d "format"\n",\
                                            time_string, level,\
                                            ##args);\
                }\
}while(0)
int DebugInit( void );
int DebugEnd( void );
#endif /* _DBG_H_1DAC920E_1822_4D46_9324_68222C85412B_ */

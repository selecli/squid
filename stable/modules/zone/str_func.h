#ifndef _STR_FUC_H_
#define _STR_FUC_H_ 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <stdbool.h>

#ifdef WIN32
#include <glib.h>
#else
//#include <glib-2.0/glib.h>
//#include <glib.h>
#endif

extern char toup_table[];
extern int black_table[];

typedef  unsigned int u_int32_t ;
#define UNUSED(x)      (void)(x)
#define INT_IS_NULL INT_MIN 

char * ip_str2socket(register char *cp, unsigned int *p_ip);
int cidr_int_to_char2(unsigned int ip, unsigned int mask, char *str);


char * cidr_str2i(register char *cp, unsigned int *p_ip,unsigned int *p_mask);
char * str2num(register char *cp, int *num);
char * str_name(register char *cp);
unsigned char *trim(unsigned char *str);
char * str_n_cmp(char *a , const char *b, const int len);
char * str_domain(register char *cp);
char *find_head(char *str);


void *new_zero(size_t size);
#define NEW_ZERO(x) ((x*)new_zero(sizeof(x)))

//检查解析检查配置文件的宏
#define FIND_DOMAIM		head = tail; tail = str_domain(head);if (tail == NULL){goto cleanup;}else{pb=head;pe=tail;}
#define CHECK_DOMAIM	FIND_HEAD CMP_CHR('\"') FIND_DOMAIM CMP_CHR('\"') FIND_END

#define  VALUES char line[MAX_LINE_BUF+1];char *head;char *tail;char *pb,*pe;
#define  NULL_RE_NULL(Val)  if((Val) == NULL) return NULL;;
#define  NULL_RE_FALSE(Val)  if((Val) == NULL) return FALSE;;

#define MAX_LINE_BUF 4096
#define STR_N_CMP(str1 , str2) str_n_cmp(str1, str2, sizeof(str2) - 1)
#define IF_STR_N_CMP(cmp_str) if ((tail = STR_N_CMP(head, cmp_str)) != NULL)

#define FIND_HEAD		head = tail; tail = find_head(head);if (tail == NULL){goto cleanup;}
#define CMP_STR(str)	head = tail; tail = STR_N_CMP(head, str); if (tail == NULL){goto cleanup;}
#define CMP_CHR(ch)		head = tail; tail++;if (*head != ch){goto cleanup;}
#define FIND_END		head = tail; tail = find_head(head);if (tail != NULL){goto cleanup;}

#define FIND_NAME		head = tail; tail = str_name(head);if (tail == NULL){goto cleanup;}else{pb=head;pe=tail;}
#define FIND_NUM(num)		head = tail; tail = str2num(head,&(num));if (tail == NULL){goto cleanup;}

#define CHECK_NUM(num)	FIND_HEAD FIND_NUM((num)) FIND_END
#define CHECK_NAME		FIND_HEAD CMP_CHR('\"') FIND_NAME CMP_CHR('\"') FIND_END
#define CHECK_BEGIN		if (*tail != '{'){FIND_HEAD;}CMP_CHR('{');FIND_END;
#define CHECK_END		FIND_END;

#define NEW_STR(new_str) new_str=(char*)new_zero(pe - pb + 1);if (new_str){memcpy(new_str,pb,pe - pb); new_str[pe - pb]='\0';}

#define CHECK_NEW_STR(new_str) CHECK_NAME;NEW_STR(new_str);

#define IF_CHECK_NEW_STR(cmp_str, new_str)   IF_STR_N_CMP(cmp_str){CHECK_NAME;NEW_STR(new_str);}
#define IF_CHECK_NEW_NUM(cmp_str, num)   IF_STR_N_CMP(cmp_str){CHECK_NUM(num);}

#define NO_NULL_FREE(Val)		if((Val) != NULL) {free(Val);Val = NULL;}
#define NO_NULL_USER(Val, Fuc)	if((Val) != NULL) {Fuc(Val);Val = NULL;}


#define CHECK_NAME_SIZE(Val) if (pe - pb >= sizeof(Val)){goto cleanup;};
#define COPY_NAME(Val) memcpy(Val, pb, pe-pb); (Val)[pe-pb] = '\0';
#define IF_IS_TRUE(Val) if (!(Val)){goto cleanup;}

#endif

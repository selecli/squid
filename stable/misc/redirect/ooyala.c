#ifdef OOYALA

#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/utsname.h>
#include "redirect_conf.h"
#include <sys/types.h>


#if 1
extern int g_fdDebug;
extern FILE *g_fpLog;
#define CRITICAL_ERROR(args, ...)    fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define NOTIFICATION(args,...)       fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define DEBUG(args, ...)             if(g_fdDebug > 3) {fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);}
//#define DEBUG(args, ...)             fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);

#else

#define CRITICAL_ERROR(args, ...)    printf(args, ##__VA_ARGS__);fflush(stdout);
#define NOTIFICATION(args,...)       printf(args, ##__VA_ARGS__);fflush(stdout);
#define DEBUG(args, ...)             printf(args, ##__VA_ARGS__);fflush(stdout);

#endif
static int sensitive = 0;
typedef struct _wordlist {
	char *key;
	struct _wordlist *next;
}wordlist;

	static const char *
wordlistAdd(wordlist ** list, char *key)
{
	while (*list)
		list = &(*list)->next;
	*list = (wordlist*) malloc(sizeof(wordlist));
	(*list)->key = strdup(key);
	(*list)->next = NULL;
	return (*list)->key;
}

static inline void safe_free(void *p)
{
	if (p) free(p);
}
	static void
wordlistDestroy(wordlist ** list)
{
	wordlist *w = NULL;
	while ((w = *list) != NULL)
	{
		*list = w->next;
		safe_free(w->key);
		safe_free(w);
	}           
	*list = NULL;
}


/* case http://sis.ssr.chinacache.com/rms/view.php?id=4137 */
int analysisOoyalaCfg(struct redirect_conf *pstRedirectConf)
{
	char *pstC;
	wordlist *word = NULL;

    while (NULL != (pstC = strtok(NULL, " \t\r\n"))) {
		if (!strcasecmp(pstC, "contain")) {
			pstRedirectConf->regex_incase = 1;    
			break;
		}
		else if (!strcasecmp(pstC, "equal")) {
			pstRedirectConf->regex_incase = 2;    
			break;
		}
		else 
			wordlistAdd(&word, pstC);
	}

	if (NULL == word)
		goto error;

	pstRedirectConf->other = word;	// parameter name
    if (NULL == (pstC = strtok(NULL, " \t\r\n")))
		goto error;
	if (!strcasecmp(pstC, "sensitive"))
		sensitive = 1;    
	else if (!strcasecmp(pstC, "insensitive"))
		sensitive = 2;    
	else 
		goto error;

    DEBUG("OOYALA Redirect: match=%d, sensitive=%d\n",pstRedirectConf->regex_incase,sensitive);
	while (word) {
		DEBUG("OOYALA Redirect: need delete parameter [%s]\n", word->key);
		word = word->next;
	}
    return 0;

error:
    DEBUG("OOYALA Redirect: analysisOoyalaCfg failed\n");
    if (NULL != word){
		wordlistDestroy(&word);	
		word = NULL;
		pstRedirectConf->other = NULL;
	}
	return -1;
}

static inline void param_process(const struct redirect_conf *pstRedirectConf,char *param, char *uri)
{
	assert(pstRedirectConf->regex_incase == 1 || pstRedirectConf->regex_incase ==2);

	char *tmp;
    char name[100];
    memset(name,0,100);

	if (NULL == ( tmp = strchr(param,'='))) {
		strcat(uri,param);
		strcat(uri,"&");
		return;
	}

	wordlist *word = pstRedirectConf->other;
	strncpy(name, param, tmp-param);
	int found = 0;
	if (pstRedirectConf->regex_incase == 1) {	// contain
		if (1 == sensitive ) {
			while (word) {
				if (strstr(name, word->key)){
					DEBUG("[%s] matched parameter [%s]\n", name, word->key);
					found = 1;
					return;
				}
				word = word->next;
			}
		} 
		else if (2 == sensitive) {
			while (word) {
				if (strcasestr(name, word->key)){
					DEBUG("[%s] matched parameter [%s]\n", name, word->key);
					found = 1;
					return;
				}
				word = word->next;
			}
		}
	} else if (pstRedirectConf->regex_incase == 2)  {		// eqaul
		if (1 == sensitive) {		// case-sensitive
			while (word) {
				if (!strcmp(name, word->key)){
					DEBUG("[%s] matched parameter [%s]\n", name, word->key);
					found = 1;
					return;
				}
				word = word->next;
			}
		} 
		else if (2 == sensitive) {	// case-insensitive
			while (word) {
				if (!strcasecmp(name, word->key)){
					DEBUG("[%s] matched parameter [%s]\n", name, word->key);
					found = 1;
					return;
				}
				word = word->next;
			}
		} 
	}

	if (!found){
		DEBUG("[%s] do not matched any parameter\n",param);
		strcat(uri,param);
		strcat(uri,"&");
	}
}

int OoyalaVerify(const struct redirect_conf *pstRedirectConf, char *url, char *ip, char *other)
{
    int one_param = 0;
    char *tmp = NULL;
    char *param = NULL;
    char uri[BUFSIZ];
    memset(uri, 0, BUFSIZ);

    DEBUG("client url is %s \n", url);
    if (NULL == (tmp = strchr(url, '?')))
        goto donot_verify;
    tmp++;
    if ('\0' == *tmp)		// No parameter
        goto donot_verify;

    strncpy(uri,url,tmp-url);
    if (NULL == (param = strtok(tmp, "&")))
    {
        param = tmp;    // only one parameter
        one_param = 1;
    }
    param_process(pstRedirectConf, param, uri);    
    while (0 == one_param)
    {
        if (NULL == (param = strtok(NULL, "&")))
            break;
        param_process(pstRedirectConf, param, uri);    
    }

    int len = strlen(uri) - 1;
    if ('&' == uri[len] || '?' == uri[len])
        uri[len] = '\0';

    DEBUG("url after redirect is %s \n", uri);
    printf("%s %s", uri, other);
    DEBUG("REDIRECT SUCCESS OUTPUT: %s %s %s\n", uri, ip, other);
    fflush(stdout);
    return 1;

donot_verify:
    DEBUG("DO NOT REDIRECT\n");
    printf("%s %s",url,other); 
    fflush(stdout);
    return 1;
}

#endif

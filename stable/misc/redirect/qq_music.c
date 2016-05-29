#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE* g_fpLog;

#define PERSONAL_LOG(args,...)      /* fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);*/
#define CRITICAL_ERROR(args,...)    fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define NOTIFICATION(args,...)      fprintf(g_fpLog, args, ##__VA_ARGS__);fflush(g_fpLog);
#define MAX_BYTES_PER_LINE  1024    /* max character number per line in configure file */
#define MAX_FROMTAG_SIZE    1024    /* max length of fromtag array */
#define MAX_DOMAIN_NAME_SIZE     256
#define MAX_CONFIG_NUMBERS  128     /* max number of fromtags */
#define MAX_CONFICT_ENTRYS_SIZE        (MAX_CONFIG_NUMBERS)
#define MAX_HASH_TABLE_SIZE (MAX_CONFICT_ENTRYS_SIZE * 4)    /* max hash table size */

#define PREFIX_TOKEN       '$'
#define SAPERATE_TOKEN     ((char*)" \r\n\t")     /* space as token */
#define GET_HASH_INDEX(hashvalue)   ((hashvalue) % MAX_HASH_TABLE_SIZE)

/* hash table has 1024 entrys, only use the first 512 entrys in normal process.
 * When insert domain, hash conflict occur, find the next valible entry until 
 * reach MAX_CONFIG_NUMBERS.
 * When search, hash conflict occur, find the next entry until 
 * reach MAX_CONFIG_NUMBERS.
 */

typedef struct DoAndFromtag
{
    struct DoAndFromtag *pstNext;
    unsigned long hashvalue;
    char fromtag[MAX_FROMTAG_SIZE];
    char domain[MAX_DOMAIN_NAME_SIZE];
}DOMAIN_FROMTAG;

static DOMAIN_FROMTAG *gpst_DomainFromtag[MAX_HASH_TABLE_SIZE];


static unsigned long strhash(const char *str) 
{ 
    register unsigned long h; 
    register unsigned char *p; 

    for(h=0, p = (unsigned char *)str; *p ; p++) 
        h = 31 * h + *p; 

    return h; 
} 


/* FUNCTION NAME: int checkDomainAndFromtag(char *pdomain, int fromtag)
 * INPUTS:        char *pdomain: domain name, string
 *                int fromtag  : fromtag of qq, must >= 0 
 * 
 * OUTPUT:  0  legal domain and fromtag
 *          others illegal domain or fromtag
 */
int checkDomainAndFromtag(char *pdomain, int fromtag)
{
    unsigned long hashvalue;
    unsigned int index;
    DOMAIN_FROMTAG *pd;
    char aFromtag[sizeof(int)*8];
    char *pfromtag = aFromtag;

    if(NULL==pdomain || fromtag < 0)
    {
        PERSONAL_LOG("illegal args\n");
        return -1;
    }

    if(0 == strlen(pdomain))
    {
        PERSONAL_LOG("domain length is 0\n");
        return -1;
    }

    sprintf(pfromtag, "\\%d\\", fromtag);

    hashvalue = strhash(pdomain);
    index = GET_HASH_INDEX(hashvalue);

    PERSONAL_LOG("domain %s hash %lu\n", pdomain, hashvalue);

    pd = gpst_DomainFromtag[index];

    while(NULL != pd)
    {
        if(pd->hashvalue == hashvalue && 0 == strcmp(pd->domain, pdomain) && NULL != strstr(pd->fromtag, pfromtag))
        {
            return 0;
        }
        pd = pd->pstNext;
    }

    PERSONAL_LOG("did not find domain %s fromtag %s\n", pdomain, pfromtag);
    return -1;
}

/* init domain and fromtag search list
 */
int initHashTable(char* pdomain, char *pfromtag)
{
    unsigned long hashvalue;
    unsigned int index = 0;
    DOMAIN_FROMTAG  *pstDomainFromtag;
    DOMAIN_FROMTAG  *pstTemp;

    if(NULL == pdomain || NULL == pfromtag)
    {   
        PERSONAL_LOG("NUll args\n");
        return -1;
    }
    
    pstDomainFromtag = (DOMAIN_FROMTAG*)malloc(sizeof(DOMAIN_FROMTAG));

    if(NULL == pstDomainFromtag)
    {
        CRITICAL_ERROR("No memory\n");
        return -1;
    }

    memset(pstDomainFromtag, 0, sizeof(DOMAIN_FROMTAG));

    hashvalue = strhash(pdomain);
    index = GET_HASH_INDEX(hashvalue);

    pstTemp = gpst_DomainFromtag[index];

    /* insert to the header */
    gpst_DomainFromtag[index] = pstDomainFromtag;
    pstDomainFromtag->pstNext = pstTemp;

    pstDomainFromtag->hashvalue = hashvalue;
    if(strlen(pfromtag) > MAX_FROMTAG_SIZE - 1)
    {
        CRITICAL_ERROR("Fromtag length more than %d\n", MAX_FROMTAG_SIZE - 1);
        return -1;
    }
    strcpy(pstDomainFromtag->fromtag, pfromtag);

    if(strlen(pdomain) > MAX_DOMAIN_NAME_SIZE - 1)
    {
        CRITICAL_ERROR("Domain/referer length more than %d\n", MAX_DOMAIN_NAME_SIZE - 1);
        return -1;
    }

    strcpy(pstDomainFromtag->domain, pdomain);

    PERSONAL_LOG("insert domain [%s] fromtag [%s] hash [%lu] index [%lu] success\n", pdomain, pfromtag, hashvalue, index);

    return 0;
}

/* release hash table
 */
void releaseHashTable()
{
    int i;
    DOMAIN_FROMTAG  *pd;
    DOMAIN_FROMTAG  *pstTemp;
    for(i=0; i<MAX_HASH_TABLE_SIZE; i++)
    {
        pd = gpst_DomainFromtag[i];
        while(NULL != pd)
        {
            pstTemp = pd->pstNext;
            free(pd);
            pd = pstTemp;
        }
        gpst_DomainFromtag[i] = NULL;        
    }
    return;
}

/* init domain and fromtag datebase
 */
int initDomainAndFromtagDb(char *pfilename)
{
    FILE *pfile;
    char buf[MAX_BYTES_PER_LINE];
    int linenum = 0;

    if(NULL == pfilename)
    {
        CRITICAL_ERROR("NULL filename\n");
        return -1;
    }

    pfile = fopen(pfilename, "r");
    if(NULL == pfile)
    {
        CRITICAL_ERROR("Can not open file [%s]\n", pfilename);
        return -1;
    }

    memset(gpst_DomainFromtag, 0, sizeof(gpst_DomainFromtag));

    while(NULL != fgets(buf, MAX_BYTES_PER_LINE, pfile))
    {
        linenum++;
        if(PREFIX_TOKEN != buf[0])
        {
            continue;
        }
        else
        {
            /* format:$music.qq.com \0\9\10\11\12\
            */
            char *pdomain;
            char *pfromtag;
            char *pline;
            unsigned int length;

            length = strlen(buf);

            /* only $ is illegal, check the length is needed */
            if(length < 2)
            {
                releaseHashTable();
                CRITICAL_ERROR("failed in analyse domain filed in file [%s:%d]\n", pfilename, linenum);
                fclose(pfile);
                return -1;
            }

            pline = &buf[1];
            /* get domain */
            pdomain = strtok(pline, SAPERATE_TOKEN);
            if(NULL == pdomain)
            {
                releaseHashTable();
                CRITICAL_ERROR("failed in analyse domain filed in file [%s:%d]\n", pfilename, linenum);
                fclose(pfile);
                return -1;
            }
            PERSONAL_LOG("domain %s\n", pdomain);

            pfromtag = strtok(NULL, SAPERATE_TOKEN);
            /* get fromtag */
            if(NULL == pfromtag)
            {
                releaseHashTable();
                CRITICAL_ERROR("failed in analyse fromtag filed in file [%s:%d]\n", pfilename, linenum);
                fclose(pfile);
                return -1;
            }
            PERSONAL_LOG("fromtag list %s\n", pfromtag);

            if(initHashTable(pdomain, pfromtag))
            {
                releaseHashTable();
                CRITICAL_ERROR("InitHashTable failed\n");
                fclose(pfile);
                return -1;
            }
        }
    }

    PERSONAL_LOG("Init Domain and fromtag data base succesfull\n");

    fclose(pfile);
    return 0;
}

/* release Domain and fromtag database
 */
void releaseDomainAndFromtagDb()
{
    releaseHashTable();
}


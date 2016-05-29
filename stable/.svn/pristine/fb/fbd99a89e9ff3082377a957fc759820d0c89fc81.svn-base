#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define DB FILE
#define DB_NUM 5

#define ORI_DB_NAME NULL
#define URL_MAX_LEN 1024

DB *dbs[DB_NUM];
char ori_db_name[256];

int get_result_handlers()
{
	int i;

	static char db_file[256];

	for(i = 0; i < DB_NUM; i++)
	{
		strcpy(db_file, ori_db_name);
		sprintf(db_file + strlen(ori_db_name) - 3, "_%02d.db", i);

		dbs[i] = fopen(db_file,"w");
		if(dbs[i] == NULL)   
		{
			fprintf(stderr,"fopen : %s",strerror(errno));
			return -1;
		}
	}

	return 0;
}
		
unsigned long ELFhash(char *key)
{
        unsigned long h = 0;

        while(*key)
        {
                h = (h<<4) + *key++;

                unsigned long g = h & 0Xf0000000L;

                if(g)
                        h ^= g >> 24;
                h &= ~g;
        }

        return h;
}

char * rf_url_hostname(const char *url,char *hostname)
{
        char *a = strstr(url,"//");
        if(a == NULL)   
                return NULL;
                        
        a += 2;

        strcpy(hostname,a);     
                
        char *c = hostname;
        while(*c && *c != '/'){
                c++;
        }       
                        
        if(*c != '/')
                return NULL;

        *c = '\0';
        
        return hostname;
}

int get_url_filenum(char *url)
{
        if(url == NULL)
                goto err;

        static char hostname[256];
        if(rf_url_hostname(url, hostname) == NULL)
                goto err;

        return (int)(ELFhash(hostname) % DB_NUM);

err:
//	fprintf(stderr, "get url filenum failed\n");
        return -1;
}

int main(int argc, char* argv[])
{
	int i, n, num, ret = 0;
	char url_buffer[URL_MAX_LEN];
	
	memset(url_buffer, 0, sizeof(url_buffer));

	if(argc < 2)
	{
		printf("input format error! use:  hash_db db_path\n");
		
		return 0;
	}

	for(n = 1; n < argc; n++)
	{
		memset(ori_db_name, 0, sizeof(ori_db_name));
		strcpy(ori_db_name, argv[n]);

		DB *ff;
	
		if((ff = fopen(argv[n], "r")) == NULL)
		{
			fprintf(stderr, "open original db failed\n");

			ret = -1;
			goto out;
		}
	
		if(get_result_handlers() != 0)
		{
			fprintf(stderr, "fopen hash db files failed\n");

			ret = -1;
			goto out;
		}

		while(fgets(url_buffer, URL_MAX_LEN, ff) != NULL)
		{
			//ignore it
			if((num = get_url_filenum(url_buffer)) == -1)
				continue;

			fwrite(url_buffer, strlen(url_buffer), 1, dbs[num]);
		}

out:
		if(ff != NULL)
			fclose(ff);

		for(i = 0; i < DB_NUM; i++)
		{
			if(dbs[i] != NULL)
				fclose(dbs[i]);
		}
	}

	return ret;
}		

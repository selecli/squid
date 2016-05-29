#include "refresh_header.h"
#include "squid_enums.h"
#include "squid_md5.h"
#include <db.h>

static char dbfile[1024];
static DB *dbp = NULL;           

void usage(){
	fprintf(stdout,"usage : gen_large_db [-f db_file] \n");
}

DB* get_db_handler(const char *db_file){
	if(dbp != NULL){
		return dbp;
	}

	int ret = db_create(&dbp, NULL, 0);
	if(ret != 0){
		dbp = NULL;
		return NULL;
	}


	u_int32_t flags;  
	flags = DB_CREATE;    
	ret = dbp->open(dbp, NULL, dbfile, NULL, DB_BTREE, flags, 0); 
	if(ret != 0){
		return NULL;
	}

	return dbp;
}

static void options_parse(int argc, char **argv)
{
	if(argc < 2){
		usage();
		exit(-2);
	}

	int c;
	int g_f = 0;
	while(-1 != (c = getopt(argc, argv, "f:"))) {
		switch(c) {
			case 'f':
				if(optarg) {
					strcpy(dbfile,optarg);
					g_f = 1;
				}
				else{
					usage();
					exit(2);
				}

				break;
			default:
				fprintf(stderr, "Unknown arg: %c\n", c);
				usage();
				exit(2);
			   	break;
		}
	}

	if(! g_f){
		usage();
		exit(2);
	}
}

void add_url(DB *db,char *url){
	const char *md5 = storeKeyText(storeKeyPublic(url,METHOD_GET));
	if(md5 == NULL){
		fprintf(stderr,"Gen md5 for %s error!\n",url);
		exit(-1);
	}

	DBT key,data;
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = (void *)md5;
	key.size = strlen(md5);
	
	data.data = (void *)url;
	data.size = strlen(url);

	int ret = 0;
	ret = db->put(db, NULL, &key, &data,DB_NOOVERWRITE); 
	if(ret != 0){
		if(ret == DB_KEYEXIST){
			fprintf(stderr,"%s has been existed! ",md5);
		}
		else{
			fprintf(stderr,"db put error! %s",db_strerror(ret));
		}
	}
}

int main(int argc,char *argv[]){
	options_parse(argc,argv);
	DB *db = get_db_handler(dbfile);

	char buffer[1024];
	uint32_t i,j,count;

	count = 0;
	for(i = 0;i < 10000 ; i++){
			for(j = 0;j < 1000; j++){
					memset(buffer,0,1024);
					sprintf(buffer,"http://blog.sina.com.cn/blog_4a39b61b0100c5gf/6a79b61bb100c5gfythn/%d/%d",i,j);
					add_url(db,buffer);
					count++;

					if((count % 500) == 0){
							fprintf(stderr,"%s\n",buffer);
							db->sync(db,0);
					}
			}
	}
	return 0;
}

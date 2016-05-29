#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <db.h>
#include <ctype.h>

static char dbfile[1024];
static DB *dbp = NULL;
int isblank(int c);

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

void add_url(DB *db,char *md5,char *url){
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
//                        fprintf(stderr,"%s has been existed! ",md5);
                }
                else{
 //                       fprintf(stderr,"db put error! %s",db_strerror(ret));
                }
        }
}

void usage(char *filename){
        fprintf(stdout,"usage : %s [-f db_file] \n",filename);
}

static void options_parse(int argc, char **argv)
{
        if(argc < 2){
                usage(argv[0]);
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
                                        usage(argv[0]);
                                        exit(2);
                                }

                                break;
                        default: 
				fprintf(stderr, "Unknown arg: %c\n", c);
				usage(argv[0]);
				exit(2);
				break;
		}
	}

	if(! g_f){
		usage(argv[0]);
		exit(2);
	}
}

int main(int argc,char *argv[]){
	options_parse(argc,argv);
	DB *db = get_db_handler(dbfile);

	char buffer[1024];
	uint32_t count = 0;
	char *c = NULL;

	while(fgets(buffer,1024,stdin) != NULL){
		c = buffer;
		while(!isblank(*c)) c++;
		*c++ = '\0';

		while(isblank(*c)) c++;

		add_url(db,buffer,c);
		count++;

		if((count % 10000) == 0) db->sync(db,0);

	}

	db->sync(db,0);
	return 0;
}

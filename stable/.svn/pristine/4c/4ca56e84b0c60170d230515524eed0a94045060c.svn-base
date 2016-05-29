/*
 * $Id$
 * author : liwei(liwei@chinacache.com)
 * desc   : Berkely DB functions
 */
#include "refresh_header.h"
//static pthread_mutex_t n_mutex = PTHREAD_MUTEX_INITIALIZER;

//for stop dir refresh thread
extern bool stop;

int rf_db_close(DB *db){
	assert(db != NULL);
	fclose(db);

	return RF_OK;
}

DB* get_db_handler(const char *db_file){
	DB* f = fopen(db_file,"a+");
	if(f == NULL)
		cclog(0,"fopen : %s",strerror(errno));
	return f;
}

int rf_add_url_real(DB *db,const char *u_data,int u_data_len){
	return rf_add_url(db,u_data);
}

int rf_add_url(DB *db,const char *u_data){
	static char buffer[10240];
	memset(buffer,0,10240);
	strcpy(buffer,u_data);
	strcat(buffer,"\n");
	int len = fwrite(buffer,strlen(buffer),1,db);
	if(len < 0)
		cclog(0,"fwrite : %s",strerror(errno));

	rf_flush(db);
	return RF_OK;
}

int rf_flush(DB *db){
	assert(db != NULL);
	fflush(db);
	return RF_OK;
}

void hash_thread_dir_sess(rf_thread_stack* thread_stack)
{
	int i, n;

	for(i = 0; i < db_num; i++)
	{
		hash_dir_sess_stacks[i].sess_count = 0;
	}

	for(i = 0; i < thread_stack->sess_count; i++)
	{
		if(db_num != 1)
			n = get_url_filenum(thread_stack->sess_stack[i]->url_list->buffer);
		else
			n = 0;

		if(n != -1)
			hash_dir_sess_stacks[n].sess_stack[hash_dir_sess_stacks[n].sess_count++] = thread_stack->sess_stack[i];
	}
}

/*
static void dirRemoveHost(rf_url_list *url)
{
	size_t i, len;
	char *t1 = NULL;
	char *t2 = NULL;

	if (NULL == config.url_remove_host)
		return;

	t1 = strstr(url->buffer, "://");
	if (NULL == t1) 
		return;
	t1 += 3;
	t2 = strchr(t1, '/');
	len = (NULL == t2) ? strlen(t1) : t2 - t1; 
	if (0 == len)
		return;
	for (i = 0; i < config.url_remove_host_count; i++)
	{   
		if (strlen(config.url_remove_host[i]) != len)
			continue;
		if (strncmp(config.url_remove_host[i], t1, len))
			continue;
		t2++;
		len = strlen(t2);
		memmove(url->buffer, t2, len);
		url->buffer[len] = '\0';
		url->len = len;
		return;
	}   
}
*/
		
//Warning: in the thread function, cclog will cause SIGSEGV, because it was not threadsafe.
int rf_match_url(rf_client *rfc,rf_match_callback *url_match_callback){

	DB *ff[DB_NUM];

	char buffer[10240];
	memset(buffer,0,10240);

	char buffer2[256];
	memset(buffer2, 0, sizeof(buffer2));

	long search_count = 0;
	long match_count = 0;
	long match_ctrl =  0;
	int ret = 0;
	int i, j;

	rf_url_compare *compare_func = NULL;

	cclog(9,"sess_count : %d\n",thread_stack.sess_count);

	hash_thread_dir_sess((rf_thread_stack*)&thread_stack);

	for(i = 0; i < db_num; i++)
	{
		ff[i] = fopen(rfc->db_files[i], "r");
		
		if(ff[i] == NULL)
		{
			cclog(9, "DB file %d has not been created yet!\n", i);
			goto match_out;
		}	
	}

	for(i = 0; i < db_num; i++)
	{
		if(hash_dir_sess_stacks[i].sess_count != 0)	
		{
			while (fgets(buffer, 10240, ff[i]) != NULL) 
			{
				search_count++;

				/********** for cancel thread **********/
				if(stop == true)
					goto match_out;
				//	pthread_testcancel();
				/********** for cancel thread **********/

				char *c = NULL;
				if((c = strchr(buffer,'\n')) != NULL)
					*c = '\0';

				for(j = 0;j < hash_dir_sess_stacks[i].sess_count; j++)
				{
					if(stop == true)
							goto match_out;

					rf_cli_session *sess = hash_dir_sess_stacks[i].sess_stack[j];

/*
					//too many urls to process in fc ???
					{
				again:
					{
					if(sess->url_number - (sess->success + sess->fail) > 30000)
					{
						if(stop == true)
							goto match_out;

						sprintf(buffer2, "current submitted url number: %"PRINTF_UINT64_T", received result number: %"PRINTF_UINT64_T" ", sess->url_number, sess->success + sess->fail);
						cclog(3, buffer2);
						cclog(2, "pthread sleep 1s to avoid FC malloc failed, because there are too many process result has't return from FC");
						sleep(1);
						goto again;
					}
					}
*/

					rf_cli_dir_params *dp = (rf_cli_dir_params *) sess->params;
					if(dp->action == 0)
					{
						compare_func = rf_url_match_prefix;
					}
					else 
					{
						compare_func = rf_url_match_regex;
					}

					/* Add Start: dir remove host, by xin.yao, 2012-03-04 */
					//dirRemoveHost(sess->url_list);
					/* Add Ended: by xin.yao */

					char *token = sess->url_list->buffer;
	
					//pthread_mutex_lock(&n_mutex);
					if(0 == compare_func(token,buffer))
					{
						sess->url_number++;
						ret = url_match_callback(rfc,sess,buffer);

						match_count++;
						match_ctrl++;
					}
					//pthread_mutex_unlock(&n_mutex);

					if(search_count && (search_count % 10000) == 0)
						cclog(10,"search count : %ld,match count : %ld\n",search_count,match_count);

					if(match_ctrl > 1000)
					{
						cclog(10,"so many match_count : %ld,thread sleep 200ms now!",match_count);
						match_ctrl = 0;
						usleep(200);
					}
				}
			}
		}
	}

match_out:

        for(i = 0; i < db_num; i++)
        {
		if(ff[i] !=  NULL)
			fclose(ff[i]);
	}
	
	return RF_OK;
}

int rf_store_init(){
	return RF_OK;
}

void rf_store_dest(){
}

void rf_db_check(){
}

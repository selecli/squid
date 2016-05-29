
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

//#define MAX_FILE_LINE_NUM 5000

int safe_free(void* data)
{
	if(data == NULL)
	{
		return 0;
	}
	free(data);
	data = NULL;
	return 0;
}

int main(int argc, char* argv[])
{

/* parse the param */
/*
* -h: help info
* -o: original receipt path
* -m: receipt path for transfer to trend
* -d: destination url for post
* -b: backup dir
* -n: how many lines per time for transfer
* 
*/
	char* path_name = NULL;
	char* file_name = NULL;
	char* upload_dir = NULL; // the upload_files direcroty
	char* backup_dir = NULL; // the backup_dir direcroty
	char* upload_address = NULL; //the upload address provided by trend
	int result;
	int lines_per_time = 5000;
	while((result = getopt(argc,argv,"ho:m:d:b:n:"))!=-1)
	{
		switch(result)	
		{
			case 'h':
				fprintf(stderr,"\n\
						-h: help info\n\
						-o: original receipt path\n\
						-m: receipt path for transfer to trend\n\
						-d: destination url for post\n\ 
						-b: backup dir\n\
						-n: how many lines per time for transfer\n");

				return 0 ;
				break;
			case 'o':
				path_name = strdup(optarg);	
//				fprintf(stderr,"%s",path_name);
				break;
			case 'd':
				upload_address = strdup(optarg);
//				fprintf(stderr,"%s",upload_address);
				break;
			case 'b':
				backup_dir = strdup(optarg);
//				fprintf(stderr,"%s",backup_dir);
				break;
			case 'n':
				lines_per_time = atoi(optarg);
//				fprintf(stderr,"%d",lines_per_time);
				break;
			case 'm':
				upload_dir = strdup(optarg);
				if(upload_dir[strlen(upload_dir)-1] == '/')
				{
					upload_dir[strlen(upload_dir)-1] ='\0';
				}
				//fprintf(stderr,"%s\n",upload_dir);
				break;
			case '?':
			default:
				fprintf(stderr,"\n\
						-h: help info\n\ 
						-o: original receipt path\n\
						-m: receipt path for transfer to trend\n\
						-d: destination url for post\n\
						-b: backup dir\n\
						-n: how many lines per time for transfer\n");
				return 0;
				break;

		}
	}


	/*open log file */
	char *log_file  = "/var/log/trend_receipt_transfer.log";
	FILE* log = fopen(log_file,"a");


	if(upload_dir== NULL || backup_dir ==NULL || path_name ==NULL || upload_address == NULL)
	{
		fprintf(stderr,"ERROR: original receipt path , backup_dir, destination url for post , or receipt path for transfer to trend, one of which not be set\n");
		fprintf(log,"ERROR: original receipt path , backup_dir, destination url for post , or receipt path for transfer to trend, one of which not be set\n");
		fflush(log);
		return -1;

	}


	mkdir(upload_dir,0755);
	mkdir(backup_dir,0755);
	

		//return 0;
	/*
	   argv[1] is the orig_trend_receipt direcroty name
	 */
	//初始化及定义变量
	struct dirent* ent;
	DIR *dir ;
	//char* host_name = argv[5]; //the host_name used for the upload file name 
	char host_name[100] ; //the host_name used for the upload file name 

	FILE * host_file = fopen("/proc/sys/kernel/hostname","r");
	if(host_file == NULL)
	{
		fprintf(log,"open /proc/sys/kernel/hostname error\n");
		fflush(log);
		strncpy(host_name,"default_host",strlen("default_host"));

	}
	else
	{
		fgets(host_name,100,host_file);	

		host_name[strlen(host_name)-1] = '\0';
	}
	//fprintf(stderr,"host_name is: %s, strlen is: %d \n",host_name,strlen(host_name));
	//	return 0;
	struct stat statbuf;
	FILE* file_upload ; //上传到趋势的日志文件（压缩前）
	char upload_file_name[128];//上传到趋势的日志文件名

	//打开日志文件所在目录
	dir = opendir(path_name); //open the directory
	if(dir == NULL)
	{
		fprintf(log,"opendir %s error\n",path_name);
		fflush(log);
		return -1;
	}


	//更改进程的起始目录为制定目录
	chdir(path_name);//change the process dir


	struct timeval now ;
	gettimeofday(&now,NULL);
	sprintf(upload_file_name,"%s/%s_upload_%ld_line%u.log",upload_dir,host_name,now.tv_sec,(unsigned int)0);
	file_upload = fopen(upload_file_name,"a");

	if(file_upload == NULL)
	{
		fprintf(log,"file_upload %s error\n",upload_file_name);
		fflush(log);
		return -1;
		
	}
	//fprintf(stderr,"upload_file_name is : %s\n",upload_file_name);
//return 0;


	while(1)
	{


		while(ent = readdir(dir))
		{
			file_name = strdup(ent->d_name);
			//fprintf(stderr,"file name is : %s \n",file_name);


			if(lstat(file_name,&statbuf)<0)
			{
				fprintf(log,"lsstat %s %s error\n",file_name,strerror(errno));
				fflush(log);
				safe_free(file_name);
				continue;
				//return -1;
			}
			if(S_ISDIR(statbuf.st_mode))
			{
				safe_free(file_name);
				continue;
			}

			if(S_ISREG(statbuf.st_mode))
			{
				char * has_string = strstr(file_name,"trend_receipt");
				if(has_string)
				{
					char * tmp = strstr(has_string,"tmp");
					if(tmp)
					{
						has_string = NULL;
					}
				}
				if(has_string) //the file is a trend_receipt file, wo need to process it 
				{
					FILE* file = fopen(file_name,"r");// 打开要上传的日志文件

					if(NULL == file)
					{

						fprintf(log,"file open error\n");
						fflush(log);

						safe_free(file_name);
						continue;
					}
					static unsigned int line_count = 0;
					if(NULL == file_upload)
					{
						fprintf(log,"file upload open error\n");
						fflush(log);

						safe_free(file_name);
						continue;
					}
					char buf[8192];
					char buf_send[8192];
					while(fgets(buf,8192,file))
					{
						char *tmp = strstr(buf,"&u=");
						if(tmp !=NULL)
						{
							strncpy(buf_send,buf,tmp - buf);
							strcat(buf_send,"&g=chinacache");
							strcat(buf_send,tmp);
							//fprintf(stderr,"%s",buf);
							//fprintf(stderr,"%s",buf_send);
							fprintf(file_upload,"%s",buf_send);
							//return 0;
						}
						else
						{
							continue;
						}


						if(((++line_count % lines_per_time) == 0) && (line_count !=0))
						{

							fclose(file_upload);


							gettimeofday(&now,NULL);


							//压缩要上传的文件  
							char cmd1[1024]; //gzip command
							sprintf(cmd1,"gzip %s",upload_file_name);

						system(cmd1);
							//上传压缩后的文件  
							char cmd3[1024];
							sprintf(cmd3,"curl  -m 10 --data-binary @%s.gz %s >/dev/null 2&>1",upload_file_name,upload_address);
							system(cmd3);
							usleep(10000);
							fprintf(log,"%ld\t%s\t%s\n",now.tv_sec,file_name,cmd3);
							fflush(log);
							//删除上传后的文件  
							char cmd4[1024];
							sprintf(cmd4,"rm -rf %s.gz",upload_file_name);
							fprintf(log,"%ld\t%s\n",now.tv_sec,cmd4);
							fflush(log);
							system(cmd4);
//							return 0;
							//打开 下一个要上传的文件  
							memset(upload_file_name,0,128);
							sprintf(upload_file_name,"%s/%s_upload_%ld_line%u.log",upload_dir,host_name,now.tv_sec,line_count);
							fopen(upload_file_name,"a");
							//		return 0;	
						}
						memset(buf,0,8192);
						memset(buf_send,0,8192);
					}

					fclose(file);

					//备份日志文件
					char cmd2[1024];
					sprintf(cmd2,"mv %s %s",file_name,backup_dir);
					system(cmd2);
					//return 0;
				}
			}


			safe_free(file_name);

		}
		closedir(dir);


		//打开日志文件所在目录
		dir = opendir(path_name); //open the directory
		if(dir == NULL)
		{
			fprintf(log,"opendir %s error\n",path_name);
			fflush(log);
			return -1;
		}


		//更改进程的起始目录为制定目录
		chdir(path_name);//change the process dir


		/*
		struct timeval now ;
		gettimeofday(&now,NULL);
		sprintf(upload_file_name,"%s/%s_upload_%ld_line%u.log",upload_dir,host_name,now.tv_sec,(unsigned int)0);
		file_upload = fopen(upload_file_name,"a");
		*/

		sleep(3);
	}
	closedir(dir);
	fclose(file_upload);
	return 0;
}

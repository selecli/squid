#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "log.h"

#define M_MAX_LINE_LEN (1024*4 + 64)
#define TMP_F_NUM 50*2
#define M_MAX_MEM (4096*1024*8)
#define PAGE_SIZE getpagesize()

#define TMP_DIR "/tmp/check_db_tmp/"

static int read_pos;		//for mmap get line
static int last_complete_line = 1;
static off_t tmp_file_size; 
char last_incomplete_line[M_MAX_LINE_LEN];
pid_t check_pid;
int ret_code = -1;
int check_return = 0;

struct linebuffer
{
	size_t size;			
	size_t length;		
	char *buffer;
};

struct sub_file_line_buf
{
	int file_n;
	char line_buf[M_MAX_LINE_LEN];
	int len;

	struct sub_file_line_buf *prev;
	struct sub_file_line_buf *next;
};

void line_dup (struct linebuffer *dst, struct linebuffer *src)
{
	dst->length = src->length;
	dst->size = src->size;
	dst->buffer = src->buffer;
}

void freebuffer (struct linebuffer *linebuffer)
{
	free (linebuffer->buffer);
}
/*
#define SWAP_LINES(A, B)			\
  do						\
    {						\
      struct linebuffer *_tmp;			\
      _tmp = (A);				\
      (A) = (B);				\
      (B) = _tmp;				\
    }						\
  while (0)
*/

#define SWAP_LINES(A, B)			\
  do						\
    {						\
      char *_tmp; 				\
      _tmp = (A);				\
      (A) = (B);				\
      (B) = _tmp;				\
    }						\
  while (0)

/*
static int different (char *old, char *new, size_t oldlen, size_t newlen)
{
	return (oldlen != newlen) || (memcmp (old, new, oldlen));
}
*/

static int _linebuffer_cmp (const char *line1, const char *line2, int line1_length, int line2_length)
{
	int ret;
	if (line1_length == line2_length)
		return memcmp (line1, line2, line1_length);

	int short_len = (line1_length < line2_length)?line1_length:line2_length;

	ret = memcmp (line1, line2, short_len);

	if (ret == 0)
	{
		if (line1_length > line2_length)
			return 1;
		else 
			return -1;
	}
	else
		return ret;
}

static int linebuffer_cmp (const struct linebuffer *line1, const struct linebuffer *line2)
{
	return _linebuffer_cmp(line1->buffer, line2->buffer, line1->length, line2->length);
}
		
char *_readline(char *linebuffer, int* read_len, FILE *stream)
{

	if(fgets(linebuffer, M_MAX_LINE_LEN, stream) != NULL)
	{
		*read_len = strlen(linebuffer);

		if(*read_len == 0)
			return NULL;

		return linebuffer;
	}
	else
	{
		return NULL;
	}
	
}

struct linebuffer *readline (struct linebuffer *linebuffer, FILE *stream)
{

	int read_len = -1;
	static char read_buffer[M_MAX_LINE_LEN];

	_readline(read_buffer, &read_len, stream);

	if (read_len <= 0)
	{
		return 	NULL;
	}
	linebuffer->length = read_len;
	linebuffer->size = read_len + 1;
	linebuffer->buffer = malloc(linebuffer->size);

	memcpy(linebuffer->buffer,read_buffer,read_len);

	return linebuffer;
}

/*
*	buf: return value of the mmap
*	buf_size: mmap size
*	return value: 
*			>0 line len
*			-1 if not find '\n'
*		
*/
int get_map_line(struct linebuffer *linebuffer, char *buf, int buf_size)
{
	int c;
	int n = 0;
	int end = 0;

	//the last map one
	if(buf_size != M_MAX_MEM)
	{
		end = 1;
	}

	//skip the first incomplete line that we have got at last map process
	if((read_pos == 0) && (last_complete_line == 0))
	{
		while((c = buf[read_pos++]) != '\n');
	}

	linebuffer->buffer = buf + read_pos;
	
	//NULL LINE
	if(buf[read_pos] == '\0')
	{
		cclog(0, "NULL line");
		return -2;
	}

	do
	{

		c = buf[read_pos++];
		n++;
	}
	while(c != '\n');

	linebuffer->length = linebuffer->size = n;

	if(linebuffer->length > M_MAX_LINE_LEN)
	{
		cclog(0, "too long line ...");
		return -3;
	}

	//over the page bounder, but we also get a complete line, because we mapped one more page
	if(read_pos > buf_size)
	{
	
		last_complete_line = 0;
		return -1;
	}

	last_complete_line = 1;

	return 0;
}

/*
static void check_file (const char *infile, const char *outfile)
{
	FILE *istream;
	FILE *ostream;
	char *thisline, *prevline;
  
	int thislen = 0;
	int prevlen = 0;
	int tmp_len = 0;

	static char read_buffer1[M_MAX_LINE_LEN];
	static char read_buffer2[M_MAX_LINE_LEN];

	thisline = read_buffer1;
	prevline = read_buffer2;

 	istream = fopen (infile, "r");
	if (istream == NULL)
		printf("open infile failed\n");

	ostream = fopen (outfile, "w");
	if (ostream == NULL)
		printf("open outfile failed\n");

	while (!feof (istream))
	{
		if (_readline (thisline, &thislen, istream) == 0)
			break;
	  	
		if (prevlen == 0 || different (thisline, prevline, thislen, prevlen))
		{
			fwrite (thisline, sizeof (char), thislen, ostream);

			SWAP_LINES (prevline, thisline);

			tmp_len = thislen;
			thislen = prevlen;
			prevlen = tmp_len;
		}
	}
	
	fclose(istream);
	fclose(ostream);
}
*/

void merge (struct linebuffer *line_buf, int low, int mid, int high)
{
	int i, k;

	struct linebuffer *tmp_line_buf = (struct linebuffer *)malloc(sizeof(struct linebuffer)*(high - low + 1));
	
	int begin1 = low;
	int end1 = mid;
	int begin2 = mid + 1;
	int end2 = high;

	for(k = 0;begin1 <= end1 && begin2 <= end2;k++)
	{
		if (linebuffer_cmp (&line_buf[begin1], &line_buf[begin2]) <= 0)
			line_dup (&tmp_line_buf[k], &line_buf[begin1++]);
		else
			line_dup (&tmp_line_buf[k], &line_buf[begin2++]);
	}

	if(begin1 <= end1)
	{
		for(i = begin1;i <= end1;i++, k++)
			line_dup (&tmp_line_buf[k], &line_buf[i]);
	}

	if(begin2 <= end2)
	{
		for(i = begin2;i <= end2;i++, k++)
			line_dup (&tmp_line_buf[k], &line_buf[i]);
	}

	for(i = 0; i < high - low + 1;i++)
	{
		line_dup (&line_buf[low + i], &tmp_line_buf[i]);
	}

	free(tmp_line_buf);

	return ;
}

void merge_sort (struct linebuffer *line_buf, int first , int last)
{
	int mid = 0;
	
	if(first < last)
	{
		mid = (first + last) / 2;
		
		merge_sort (line_buf, first, mid);
		merge_sort (line_buf, mid + 1, last);
		
		merge(line_buf, first, mid, last);
	}
}

void sort_lines (struct linebuffer *line_buf, int line_num)
{
	merge_sort (line_buf, 0, line_num - 1);
}

int sort_into_sub_file (const char *infile)
{
	int tmp_line_num = 1024;
	int tmp_file_num = 0;
	int ret;
	int map_times;
	int read_len = 0;
	int fd_in = -1, fd_out = -1;
	int file_pages,last_map_size;
	size_t map_size;
	off_t set_pos;
	char tmp_file_name[128];
	char *tmp_dir = TMP_DIR;
	char line_buf[M_MAX_LINE_LEN + 1];
	
	memset(line_buf, 0, sizeof(line_buf));

	char *src;
	char *dst;

	struct stat st;

	if((fd_in = open(infile, O_RDONLY)) == -1)
	{
	//	fprintf(stderr, "open: %s\n", strerror(errno));
		cclog(0, "open: %s", strerror(errno));
		return -1;
	}
	
	if(fstat(fd_in, &st) == 0)
		tmp_file_size = st.st_size;
	else
	{
		close(fd_in);
		cclog(0, "fstate: %s", strerror(errno));
		return -1;
	}

	if(tmp_file_size < M_MAX_LINE_LEN)
	{
	//	printf("file size is: %d\n", tmp_file_size);
		cclog(0, "file size is: %lld, return... ", tmp_file_size);
		close(fd_in);
		return -2;
	}
	else
	{
	//	printf("file size is: %d\n", tmp_file_size);
		cclog(0, "file size is: %lld", tmp_file_size);
	}


	if((ret = lseek(fd_in, tmp_file_size - M_MAX_LINE_LEN, SEEK_SET)) == -1)
	{
	//	printf("%s\n", strerror(errno));
		cclog(0, "lseek: %s\n", strerror(errno));
		close(fd_in);
		return -1;
	}

	int i, line_num;

	while(1)
	{
		if((ret = read(fd_in, line_buf + read_len, M_MAX_LINE_LEN - read_len)) != 0)
		{
			if(ret < 0)
			{
			//	printf("read error: %s\n", strerror(errno));
				cclog(0, "read error: %s", strerror(errno));
				close(fd_in);
				return -1;
			}
			read_len += ret;

			continue;
		}
		
		break;
	}

//	if(line_buf[read_len - 1] != '\n')
//		printf("not a complete line\n");

	i = 1;
	while(line_buf[read_len - i] != '\n')
	{
		tmp_file_size--;
		i++;

		if(i > read_len)
		{
			cclog(0, "not find new line");
			close(fd_in);
			return -1;
		}
	}
			

//	printf("final tmp_file_size: %d\n", tmp_file_size);
			
	mkdir(tmp_dir, S_IRWXU);

	set_pos = 0;

	file_pages = tmp_file_size % PAGE_SIZE;
	
	map_times = tmp_file_size / M_MAX_MEM + 1;

	if(map_times > TMP_F_NUM)
	{
		cclog(0, "too many sub files, return ...");
		return -1;
	}

	last_map_size = tmp_file_size % M_MAX_MEM;

//	printf("file size: %d\n file_pages: %d\n map_times: %d\n last_map_size: %d\n", tmp_file_size, file_pages, map_times, last_map_size);

	map_size = M_MAX_MEM;
	size_t page_size = PAGE_SIZE;

	for( i = 0; i < map_times; i++)
	{

		if(i == (map_times -1))
		{
			map_size = last_map_size;
			page_size = 0;
		}

//		printf("cur map_size: %d\n", map_size);

		memset(tmp_file_name, 0, sizeof(tmp_file_name));

		sprintf(tmp_file_name,"%s%d",tmp_dir,tmp_file_num);
		printf("tmp_file_name: %s\n",tmp_file_name);

		struct linebuffer *lines_buf = (struct linebuffer *)malloc(sizeof(struct linebuffer)*tmp_line_num);

		if((fd_out = open(tmp_file_name, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
		{	
			cclog(0, "open failed: %s", strerror(errno));
			perror("1");
			goto sort_into_sub_file_out;
		}		

		//map one more PAGE for the last incomplete line at the map border
		if((src = mmap(NULL, map_size + page_size, PROT_READ, MAP_SHARED, fd_in, set_pos)) == (void*)-1)
		{	
			cclog(0, "mmap failed: %s", strerror(errno));
			perror("2");
			goto sort_into_sub_file_out;
		}

		if((ret = madvise(src, map_size + page_size, MADV_WILLNEED)) != 0)
			perror("madvise src: ");
	
		if(lseek(fd_out, map_size + page_size - 1, SEEK_SET) == -1)
		{
			cclog(0, "lseek failed: %s", strerror(errno));
			goto sort_into_sub_file_out;
		}
			
		write(fd_out, "", 1);

		if((dst = mmap(NULL, map_size + page_size, PROT_WRITE, MAP_SHARED, fd_out, 0)) == (void*)-1)
		{
			cclog(0, "mmap fd_out failed: %s", strerror(errno));
			goto sort_into_sub_file_out;
		}

		if((ret = madvise(dst, map_size + page_size, MADV_WILLNEED)) != 0)
			perror("madvise dst: ");

                set_pos += M_MAX_MEM;

		int n = 0;
		read_pos = 0;

		while(1)
        	{
			if(read_pos >= map_size + page_size)
				break;
			
			if (n == tmp_line_num)
			{
				tmp_line_num += 1024;
			
				lines_buf = (struct linebuffer *)realloc(lines_buf, sizeof(struct linebuffer)*tmp_line_num);

			}

			if((read_len = get_map_line(&lines_buf[n], src, map_size)) < 0)
			{
				if(read_len == -2)
					break;

				if(read_len == -3)
					goto sort_into_sub_file_out;

				n++;	
				break;
			}
		
			n++;

			if(n % 300 == 0)
				usleep(1);
		}

		line_num = n;

		if(line_num == 0)
		{
			cclog(0, "line_num is 0, it maybe a error...");
			goto sort_into_sub_file_out;
		}

		sort_lines (lines_buf, line_num);

		int tmp_len = 0;

		for(n = 0;n < line_num;n++)
		{
			memcpy(dst + tmp_len, lines_buf[n].buffer, lines_buf[n].length);
			tmp_len += lines_buf[n].length;

			if(n % 300 == 0)
				usleep(1);
		}

		free(lines_buf);
		
		if((ret = madvise(src, map_size + page_size, MADV_DONTNEED)) != 0)
			perror("un madvise: ");

		if((ret = madvise(dst, map_size + page_size, MADV_DONTNEED)) != 0)
			perror("un madvise: ");

		munmap(src, map_size + page_size);
		munmap(dst, map_size + page_size);

		tmp_file_num++;
		tmp_line_num = 0;

		close(fd_out);
	}

	close(fd_in);

	return 0;

sort_into_sub_file_out:

	close(fd_in);
	close(fd_out);
	
	return -1;
}


int get_sub_files(char file_name[][256], int *file_nums)
{
	DIR *dp;
	char *path = TMP_DIR;

	struct dirent *entry;

	int file_num = 0;


	if((dp = opendir(path)) == NULL)
	{
		printf("can't open directory: %s\n", path);
		cclog(0, "can't open directory: %s\n", path);
		return -1;
	}

	chdir(path);

	while((entry = readdir(dp)) != NULL)
	{
		if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0)
			continue;

		strcpy(file_name[file_num], TMP_DIR);
		strcat(file_name[file_num++], entry->d_name);
	}

	*file_nums = file_num;
	if(file_num == 0)
	{
		return -1;
	}

	chdir("..");

	closedir(dp);
	
	return file_num;
}

void clean_tmp_dir()
{
	char *tmp_dir = TMP_DIR;
	char sub_files[TMP_F_NUM][256];
	int sub_files_num = -1;
	int i;

	mkdir(tmp_dir, S_IRWXU);

	get_sub_files(sub_files, &sub_files_num);

	for(i = 0; i < sub_files_num; i++)
		unlink(sub_files[i]);

	rmdir(tmp_dir);

	return;
}


int get_sub_file_line(struct sub_file_line_buf *line, FILE* stream, int file_no)
{

	line->prev = line->next = NULL;
	line->len = -1;

	if(_readline(line->line_buf, &(line->len), stream) == NULL)
		return -1;

	line->file_n = file_no;

	return 0;
}

void insert_to_line_buf_list(struct sub_file_line_buf *head, struct sub_file_line_buf *sub_files_buf)
{
	struct sub_file_line_buf *tmp = head->next;

	if(head->next == NULL)
	{
		head->next = sub_files_buf;
		return;
	}

        while(tmp)
        {
        	if(_linebuffer_cmp(sub_files_buf->line_buf, tmp->line_buf, sub_files_buf->len, tmp->len) <= 0)
        	{
                	if(tmp == head->next)
                	{
                        	sub_files_buf->next = tmp;
                        	tmp->prev = sub_files_buf;
                                head->next = sub_files_buf;

				break;
                	}
                        else
                        {
                        	sub_files_buf->prev = tmp->prev;
                        	tmp->prev->next = sub_files_buf;
                                tmp->prev = sub_files_buf;
                                sub_files_buf->next = tmp;

                                break;
                        }
                }

               	if(tmp->next == NULL)
               	{
                        tmp->next = sub_files_buf;
                        sub_files_buf->prev = tmp;

                        break;
                }

                tmp = tmp->next;
    	}

	return;
}
	
int merge_sorted_sub_files(char file_name[][256], int file_nums, char* out_file_name)
{
	FILE *sub_files[TMP_F_NUM];
	FILE *out_file;
	char tmp_line_buf[M_MAX_LINE_LEN];

	int tmp_line_len;
	int tmp_f_nums;
	int small_f ;

	int n = 0;

	struct sub_file_line_buf _head;
	memset(&_head, 0, sizeof(struct sub_file_line_buf));
	struct sub_file_line_buf *head = &(_head);

	struct sub_file_line_buf sub_files_buf[TMP_F_NUM];
	memset(sub_files_buf, 0, sizeof(struct sub_file_line_buf)*TMP_F_NUM);

	memset(tmp_line_buf, 0, sizeof(tmp_line_buf));
	tmp_line_buf[0] = 'a';
	tmp_line_len = 1;

	tmp_f_nums = file_nums;

	int i;

	for(i = 0;i < tmp_f_nums;i++)
	{
		if((sub_files[i] = fopen(file_name[i], "r")) == NULL)
		{
			printf("open sub file error\n");
			cclog(0, "open sub file error: %s", strerror(errno));
			return -1;
		}
	
		printf("open sub file: %s\n",file_name[i]);
	}
	
	if((out_file = fopen(out_file_name, "w")) == NULL)
	{
		printf("open out_file error\n");
		cclog(0, "open out file error: %s", strerror(errno));
		return -1;
	}

	get_sub_file_line(&(sub_files_buf[0]), sub_files[0], 0);

	head->next = &sub_files_buf[0];
	
	small_f = -1;

	for(i = 1; i < tmp_f_nums; i++)
	{
		if(get_sub_file_line(&(sub_files_buf[i]), sub_files[i], i) == -1) 
		{
			if (i != tmp_f_nums -1)
			{	
				cclog(0, "get sub file line 0 ???");
				return -1;
			}
			else
			{
				//last file is null
				tmp_f_nums--;
	                        fclose(sub_files[i]);
	                        printf("%d\n", tmp_f_nums);
				break;
			}
		}

		insert_to_line_buf_list(head, &sub_files_buf[i]);
	}

	while(tmp_f_nums != 0)
	{
		if(_linebuffer_cmp(tmp_line_buf, head->next->line_buf, tmp_line_len, head->next->len) != 0)
		{
			fwrite(head->next->line_buf, head->next->len, 1,out_file);

			if(((++n) % 300) == 0)
				usleep(1);
		
			memcpy(tmp_line_buf, head->next->line_buf, head->next->len);
			tmp_line_len = head->next->len;
		}
		
		small_f = head->next->file_n;
		head->next = head->next->next;

		if(get_sub_file_line(&(sub_files_buf[small_f]), sub_files[small_f], small_f) == -1)
		{
			tmp_f_nums--;
			fclose(sub_files[small_f]);
			printf("%d\n", tmp_f_nums);
			
			continue;
		}
		
		insert_to_line_buf_list(head, &sub_files_buf[small_f]);
			continue;
	}

        fclose(out_file);

        return 0;

}

/*
 *      sig_chld
 *      ??и║?б┴ио??3им
 */
void sig_chld()
{
        pid_t   pid;
        int     stat;

        while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        {
                //if hash db return
                if(pid == check_pid)
                {
			check_return = 1;

			if(WIFEXITED(stat))
				ret_code = WEXITSTATUS(stat);

                        cclog(0, "check db return: %d", ret_code);
                }
        }
}

static void signals_handle(int sig)
{
	if(sig == SIGCHLD)
		sig_chld();
}

static void signal_init(void)
{
	if(signal(SIGCHLD, signals_handle) == SIG_ERR)
		cclog(0,"set signal handler failed!");
}

int main(int argc, char **argv)
{
	int i = -1;

	char *in_file;
	char out_file[256];

	char sub_files[TMP_F_NUM][256];
	int sub_files_num = -1;

	struct stat st;

	off_t cur_file_size;

	int fd1;
	int fd2;

	char line_buf[M_MAX_LINE_LEN];
	int len;
	
	int n = 0;
	int ret = -1;


	cclog(0, "check_db start....................................");

	if(argc < 2)
	{
	//	printf("input format error! please use: program_name file_name1 file_name2 .... file_namen\n");
		cclog(0, "input format error! please use: program_name file_name1 file_name2 .... file_namen");
		return -1;
	}

	if((check_pid = fork()) != 0)
	{
		time_t start_time = time(NULL);
		time_t cur_time;
		signal_init();

		while(1)
		{
			sleep(10);
			
			if(check_return)
				return (ret_code == 0)?0:-1;

			if((cur_time = time(NULL)) - start_time > 1800)
			{
				cclog(0, "check db time out ... ");
				kill(check_pid, 9);
				return -1;
			}
		}
	}

	//wait for signal register, hope 5 seconds is enough ... 
	sleep(5);

	for(i = 0; i < argc - 1; i++)
	{
		in_file = argv[i + 1];
	//	printf("in_file[%d]: %s\n", i, in_file);
		cclog(0, "in_file[%d]: %s", i, in_file);
		
		memset(out_file, 0, sizeof(out_file));

		strcpy(out_file, in_file);
		strcat(out_file,".result");

	//	printf("out_file[%d]: %s\n", i, out_file);	
		
		clean_tmp_dir();

		if((ret = sort_into_sub_file(in_file)) != 0)
		{
			if(ret == -2)
				continue;

		//	printf("sort into sub file failed !\n");
			cclog(0, "sort into sub file failed !");

			return -1;
		}

		memset(sub_files, 0, TMP_F_NUM*256*sizeof(char));

		if(get_sub_files(sub_files, &sub_files_num) <= 0)
		{
			cclog(0, "get sub files failed: %s", strerror(errno));
			return -1;
		}

		if(merge_sorted_sub_files(sub_files, sub_files_num, out_file) != 0)
		{
			cclog(0, "merge sorted sub files failed");
			clean_tmp_dir();
			return -1;
		}

		clean_tmp_dir();

		if(lstat(in_file, &st) != 0)
		{
			cclog(0, "lstate failed: %s", strerror(errno));
			return -1;
		}

		cur_file_size = st.st_size;

		if(cur_file_size != tmp_file_size)
		{
			if((fd1 = open(in_file, S_IRUSR)) < 0)
			{
				printf("%s\n", strerror(errno));
				cclog(0, "open in file failed: %s\n", strerror(errno));
				goto main_out;
			}
			if((fd2 = open(out_file, O_RDWR|O_APPEND)) < 0)
			{
				cclog(0, "open out file failed: %s\n", strerror(errno));
				printf("%s\n", strerror(errno));
				goto main_out;
			}

			if(lseek(fd1, tmp_file_size, SEEK_SET) == -1)
			{
				cclog(0, "lseek file failed: %s\n", strerror(errno));
				goto main_out;
			}

			while((len = read(fd1, line_buf, M_MAX_LINE_LEN)) != 0)
			{
				if(len < 0)
				{
					printf("%s\n", strerror(errno));
					cclog(0, "read error: %s\n", strerror(errno));
					goto main_out;
				}

				if(write(fd2, line_buf, len) < 0)
				{
					printf("%s\n", strerror(errno));
					cclog(0, "write error: %s\n", strerror(errno));
					goto main_out;
				}
			}
	
			close(fd1);
			close(fd2);
		}

		if((fd1 = open(in_file, O_RDWR|O_TRUNC|O_APPEND)) == -1)
		{
		//	printf("can't trunc open file: %s\n", strerror(errno));
			cclog(0, "can't trunc open file: %s", strerror(errno));
			goto main_out;
		}

		if((fd2 = open(out_file, S_IRUSR)) == -1)
		{
		//	printf("can't open file: %s for error: %s\n", out_file, strerror(errno));
			cclog(0, "can't open file: %s for error: %s", out_file, strerror(errno));
			goto main_out;
		}

		n = 0;

		while((len = read(fd2, line_buf, M_MAX_LINE_LEN)) != 0)
                {
                        if(len < 0)
                        {
                                printf("%s\n", strerror(errno));
                                cclog(0, "read error: %s\n", strerror(errno));
                                goto main_out;
                        }

                        if(write(fd1, line_buf, len) < 0)
                        {
                               printf("%s\n", strerror(errno));
                               cclog(0, "write error: %s\n", strerror(errno));
                               goto main_out;
                        }

			if(((++n) % 300) == 0)
				usleep(1);

		}	

		close(fd1);
		close(fd2);

		unlink(out_file);

		if(lstat(in_file, &st) != 0)
		{
			cclog(0, "lstate failed: %s", strerror(errno));
			return -1;
		}

	        cclog(0, "after check_db, the current file size is: %lld ", st.st_size);
	}

	cclog(0, "check_db finish....................................");

	return 0;

main_out:	
	return -1;
}

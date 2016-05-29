#include "xping.h"

#define MAX_FILE_NUM  4096

struct file_data_st {
	time_t time;
	char path[BUF_SIZE_256];
}; 

struct filedata_st {
	int pos;
	struct file_data_st array[MAX_FILE_NUM];
} filedata;

static int parseFiledata(const char *filename)
{
	char *ptr;

	ptr = strrchr(filename, '.');
	if (NULL == ptr)
		return -1;
	++ptr;
	filedata.array[filedata.pos].time = atol(ptr);
	strcpy(filedata.array[filedata.pos].path, filename);
	filedata.pos++;

	return 0;
}

static time_t getMaxFileTime(void)
{
	int i;
	time_t max = 0;	

	for (i = 0; i < filedata.pos; ++i)
	{
		if (filedata.array[i].time > max)
			max = filedata.array[i].time;
	}

	return max;
}

static int removeOldFile(time_t min)
{
	int i, count = 0;

	for (i = 0; i < filedata.pos; ++i)
	{
		if (filedata.array[i].time < min)
		{
			count++;
			remove(filedata.array[i].path);
			xlog(COMMON, "remove data file, FILE=[%s]", filedata.array[i].path);
		}
	}

	return 0;
}

void cleaner(void)
{
	int ret, i;
	time_t max;
	glob_t glob_res;
	char pattern[BUF_SIZE_1024];

	memset(pattern, 0, BUF_SIZE_1024);
	snprintf(pattern, BUF_SIZE_1024, "%s/xping.data.*", XPINGDATA_DIR);
	ret = glob(pattern, GLOB_NOSORT|GLOB_NOCHECK, NULL, &glob_res);
	if (ret != 0)
	{
		xlog(WARNING, "glob() failed, PATTERN=[%s]", pattern);
		return;
	}
	memset(&filedata, 0, sizeof(filedata));
	for (i = 0; i < glob_res.gl_pathc; ++i)
	{
		if (filedata.pos >= MAX_FILE_NUM)
			break;
		parseFiledata(glob_res.gl_pathv[i]);
	}
	globfree(&glob_res);
	max = getMaxFileTime();
	removeOldFile(max - xping.opts.savetime);
}


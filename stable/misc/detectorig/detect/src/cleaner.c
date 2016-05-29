#include "detect.h"

void fileUnlink(const char *path)
{
	unlink(path);
	if (detect.opts.F.print)
	{
		fprintf(stdout, " ==> Unlink: %s\n", path);
		fflush(stdout);
	}
}

void cleaner(time_t cln_timestamp)
{
	char *ptr;
	int ret, i;
	glob_t glob_res;
	time_t min_timestamp;
	time_t lnk_timestamp;
	char pattern[BUF_SIZE_1024];

	if (0 == detect.opts.F.cleanup)
		return;
	debug(BEFORE_CLEAN);
	xmemzero(pattern, BUF_SIZE_1024);
	snprintf(pattern, BUF_SIZE_1024, "%s/link.data.*", LINKDATA_DIR);
	ret = glob(pattern, GLOB_NOSORT, NULL, &glob_res);
	if (ret != 0)
	{
		dlog(WARNING, NOFD, "glob() failed, PATTERN=[%s]", pattern);
		return;
	}
	if (cln_timestamp <= 0)
	{
		cln_timestamp = time(NULL);
	}
	min_timestamp = cln_timestamp - detect.opts.savetime;
	for (i = 0; i < glob_res.gl_pathc; ++i)
	{
		ptr = strrchr(glob_res.gl_pathv[i], '/');
		if (NULL == ptr)
		{
			/* file name is not link.data.timestamp, unlink it. */
			fileUnlink(glob_res.gl_pathv[i]);
			continue;
		}
		ptr = strstr(ptr, "link.data.");
		if (NULL == ptr)
		{
			/* file name is not link.data.timestamp, unlink it. */
			fileUnlink(glob_res.gl_pathv[i]);
			continue;
		}
		ptr += 10; /* 10: strlen("link.data.") */ 
		lnk_timestamp = strtol(ptr, NULL, 10);
		if (lnk_timestamp < min_timestamp || lnk_timestamp > cln_timestamp)
		{
			/* unlink the files: out of savetime or bigger than cln_timestamp */
			fileUnlink(glob_res.gl_pathv[i]);
		}
	}
	globfree(&glob_res);
	debug(AFTER_CLEAN);
}


#include "xping.h"

static FILE *ofp = NULL;
static char oldpath[BUF_SIZE_512];
static char newpath[BUF_SIZE_512];

static int checkPath(void)
{
	if (access(XPINGDATA_DIR, F_OK) < 0)
	{   
		if (mkdir(XPINGDATA_DIR, 0755) < 0)
		{   
			xlog(WARNING, "mkdir() failed, PATH=[%d]", XPINGDATA_DIR);
			return FAILED;
		}   
	}   

	return SUCC;
}

static int openResultFile(void)
{
	checkPath();
	memset(oldpath, 0, BUF_SIZE_512);
	memset(newpath, 0, BUF_SIZE_512);
	snprintf(oldpath, BUF_SIZE_512, "%s/xping.data.tmp", XPINGDATA_DIR);
	snprintf(newpath, BUF_SIZE_512, "%s/xping.data.%ld", XPINGDATA_DIR, time(NULL));
	ofp = fopen(oldpath, "a+");

	return (NULL == ofp) ? FAILED : SUCC;
}

static int resultFileRename(void)
{
	if (NULL != ofp)
	{
		fclose(ofp);
		ofp = NULL;
	}
	if (rename(oldpath, newpath) < 0)
	{   
		xlog(ERROR, "rename '%s' to '%s' failed, ERROR=[%s]", strerror(errno));
		return FAILED;
	}   
	return SUCC;
}

static void xpingTravel(void *data)
{
	ping_packet_t *packet = data;

	xpingIcmp(packet);	
	fprintf(ofp, "%s|%s|%d|%.6lf\n", xping.localaddr, packet->ip, packet->lossrate, packet->rtt);
	fflush(ofp);
}

static void doPing(void)
{
	llistTravel(xping.list, xpingTravel);
}

int main(int argc, char *argv[])
{
	globalStart(argc, argv);
	configStart();
	openResultFile();
	doPing();
	resultFileRename();
	cleaner();
	globalDestroy();

	return 0;
}


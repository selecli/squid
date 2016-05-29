#include "detect.h"

struct response_code_st {
	int num;
	char code[4];
};

typedef struct result_code_st {
	int num;
	struct response_code_st *codes;
} result_code_t;

typedef struct result_time_st {
	int oktimes;
	double cutime;
	double fbtime;
	double dltime;
	double ustime;
} result_time_t;

static FILE *ofp = NULL;
static char oldpath[BUF_SIZE_512];
static char newpath[BUF_SIZE_512];
static time_t new_timestamp = 0;

static int getAverageTime(double *dst, double t0, double t1)
{
	double tt;

	if (t0 < DOUBLE_NUMBER_MIN || t1 < DOUBLE_NUMBER_MIN)
		return FAILED;
	tt = t1 - t0;
	if (tt < DOUBLE_NUMBER_MIN)
		return FAILED;
	if (*dst < DOUBLE_NUMBER_MIN)
	{
		*dst = tt;
		return SUCC;
	}
	*dst = (*dst + tt) / 2.0;
	return SUCC;
}

static void getResponseCode(detect_unit_t *units, result_code_t *result, int times)
{
	size_t size;
	int i, j, find;

	size = sizeof(struct response_code_st);
	xmemzero(result, size);
	result->codes = NULL;

	for (i = 0; i < times; ++i)
	{
		find = 0;
		for (j = 0; j < result->num; ++j)
		{
			if (!strcmp(units[i].code, result->codes[j].code))
			{
				find = 1;
				result->codes[j].num++;
				break;
			}
		}
		if (0 == find)
		{
			if (NULL == result->codes)
				result->codes = xcalloc(1, size);	
			else
				result->codes = xrealloc(result->codes, (result->num + 1) * size);	
			strncpy(result->codes[result->num].code, units[i].code, 4);
			result->codes[result->num].num = 1;
			result->num++;
		}
	}
}

static void outputFigureTime(
		detect_ip_t *dip,
		result_time_t *result,
		int dway,
		int protocol)
{
	int i = 0;
	double cutime = 0;
	double fbtime = 0;
	double dltime = 0;
	double ustime = 0;
	detect_unit_t *unit;

	if (DWAY_COMMON == dway)
		i = 0;
	else if (DWAY_REFERER == dway)
		i = dip->rfc->times;
	else
		xabort("outputFigureTime: no this dway");
	for ( ; i < dip->rfc->times; ++i)
	{
		unit = dip->units + i;
		switch (unit->flag)
		{    
			case READY:
			case CONNECTING:
				break;
			case CONNECTED:
			case SENDING:
				unit->ok = 1;
				result->oktimes++;
				getAverageTime(&cutime, unit->cntime, unit->cdtime);
				getAverageTime(&ustime, unit->cntime, unit->edtime);
				break;
			case SENT:
			case RECVING:
				unit->ok = 1;
				result->oktimes++;
				getAverageTime(&cutime, unit->cntime, unit->cdtime);
				getAverageTime(&ustime, unit->cntime, unit->edtime);
				break;
			case FIRST_BYTE:
				unit->ok = 1;
				result->oktimes++;
				getAverageTime(&cutime, unit->cntime, unit->cdtime);
				getAverageTime(&fbtime, unit->sdtime, unit->fbtime);
				getAverageTime(&dltime, unit->fbtime, unit->edtime);
				getAverageTime(&ustime, unit->cntime, unit->edtime);
				break;
			case OTHER_BYTE:
			case RECV_REDO:
			case RECVED:
				unit->ok = 1;
				result->oktimes++;
				getAverageTime(&cutime, unit->cntime, unit->cdtime);
				getAverageTime(&fbtime, unit->sdtime, unit->fbtime);
				getAverageTime(&dltime, unit->fbtime, unit->edtime);
				getAverageTime(&ustime, unit->cntime, unit->edtime);
				break;
			default:
				xabort("eventRuninngHandle: no this time->flag");
		}    
	}
	result->cutime = cutime;
	result->fbtime = fbtime;
	result->dltime = dltime;
	result->ustime = (PROTOCOL_TCP == dip->rfc->flags.protocol) ? cutime : ustime;
}

static int isSuccCode(detect_ip_t *dip, const char *code)
{
	int i;
	const char *c;

	for (i = 0; i < dip->rfc->codes.num; ++i)
	{
		c = dip->rfc->codes.code[i].code;
		if (!strncmp(c, code, strlen(c)))
			return 1;
	}

	return 0;
}

static void chooseSuccCode(detect_ip_t *dip, result_code_t *result)
{
	int i, max = 0, max1 = 0;
	struct response_code_st *p = NULL, *p1 = NULL;

	for (i = 0; i < result->num; ++i)
	{
		if (result->codes[i].num > max)
		{
			if (isSuccCode(dip, result->codes[i].code))
			{
				max = result->codes[i].num;
				p = result->codes + i;
				continue;
			}
		}
		if (result->codes[i].num > max1)
		{
			if (result->codes[i].code[0] != '\0')
			{
				max1 = result->codes[i].num;
				p1 = result->codes + i;
			}
		}
	}
	if (NULL != p)
		strncpy(dip->dop.code, p->code, 4);
	else if (NULL != p1)
		strncpy(dip->dop.code, p1->code, 4);
	else
		strncpy(dip->dop.code, "000", 4);
}

static void outputFigure(detect_ip_t *dip)
{
	result_time_t common;
	result_time_t referer;
	result_code_t result;
	detect_cfg_t *cfg = dip->rfc;
	detect_output_t *dop = &dip->dop;

	xmemzero(&common, sizeof(common));
	outputFigureTime(dip, &common, DWAY_COMMON, cfg->flags.protocol);
	dop->cutime = common.cutime;
	dop->fbtime = common.fbtime;
	dop->dltime = common.dltime;
	dop->ustime = common.ustime;
	dop->oktimes = common.oktimes;
	dop->times = cfg->times;
	getResponseCode(dip->units, &result, cfg->times);
	if (cfg->flags.referer)
	{
		xmemzero(&referer, sizeof(referer));
		outputFigureTime(dip, &referer, DWAY_REFERER, cfg->flags.protocol);
		dop->cutime = dop->cutime * cfg->cwt + referer.cutime * cfg->rwt;
		dop->fbtime = dop->fbtime * cfg->cwt + referer.fbtime * cfg->rwt;
		dop->dltime = dop->dltime * cfg->cwt + referer.dltime * cfg->rwt;
		dop->ustime = dop->ustime * cfg->cwt + referer.ustime * cfg->rwt;
		dop->oktimes = (dop->oktimes + referer.oktimes) / 2; 
		getResponseCode(dip->units, &result, cfg->times);
	}
	chooseSuccCode(dip, &result);
}

void outputGetDway(detect_ip_t *dip)
{
	switch (dip->rfc->flags.protocol)
	{
		case PROTOCOL_TCP:
			dip->dop.dway = xstrdup("TCP");
			break;
		case PROTOCOL_HTTP:
			dip->dop.dway = xstrdup("HTTP");
			break;
		case PROTOCOL_HTTPS:
			dip->dop.dway = xstrdup("HTTPS");
			break;
		default:
			dlog(WARNING, NOFD,
					"no this detect way, PROTOCOL=[%d]",
					dip->rfc->flags.protocol);
			break;
	}
}

static void outputGetPortStatus(detect_ip_t *dip)
{
	if (0 == dip->dop.oktimes)
		dip->dop.pstate = xstrdup("down");
	else
		dip->dop.pstate = xstrdup("work");
}

void outputGetIpType(detect_ip_t *dip)
{
	switch (dip->iptype)
	{
		case IPTYPE_IP:
			dip->dop.iptype = xstrdup("ip");
			break;
		case IPTYPE_BACKUP:
			dip->dop.iptype = xstrdup("bkip");
			break;
		case IPTYPE_DNS:
			dip->dop.iptype = xstrdup("dnsip");
			break;
		default:
			xabort("outputGetIpType: no this iptype");
	}
}

//hostname|IP0->IP1|iptype|cntime|fbtime|dltime|ustime|dtimes|oktimes|port|ptstat|dtype|code
static int outputWrite(detect_ip_t *dip)
{
	char wbuf[BUF_SIZE_2048];

	xmemzero(wbuf, BUF_SIZE_2048);
	snprintf(wbuf,
			BUF_SIZE_2048,
			"%s|%s|%s|%s|%.6lf|%.6lf|%.6lf|%.6lf|%d|%d|%d|%s|%s|%s|%s",
			dip->dop.hostname, dip->dop.lip, dip->dop.sip, dip->dop.iptype,
			dip->dop.cutime, dip->dop.fbtime, dip->dop.dltime, dip->dop.ustime,
			dip->dop.times, dip->dop.oktimes, dip->dop.port, dip->dop.pstate,
			dip->dop.dway, dip->dop.code, dip->dop.detect
			);
	fprintf(ofp, "%s\n", wbuf);
	fflush(ofp);

	return SUCC;
}

static void resultTravel(void *data)
{
	detect_ip_t *dip = data;

	runningSched(300);

	if (1 == dip->rfc->flags.detect)
	{
		if (dip->once.nodetect)
		{
			memcpy(&dip->dop, &dip->once.rfc->dop, detect.tsize.detect_output_t_size);
			outputGetIpType(dip);
			dip->dop.detect = xstrdup("central");
			dip->dop.hostname = xstrdup(dip->rfc->hostname);
		}
		else
		{
			outputGetIpType(dip);
			dip->dop.port = dip->rfc->port;
			dip->dop.detect = xstrdup("detect");
			dip->dop.hostname = xstrdup(dip->rfc->hostname);
			strncpy(dip->dop.sip, dip->ip, BUF_SIZE_16);
			strncpy(dip->dop.lip, detect.other.localaddr, BUF_SIZE_16);
			outputFigure(dip);
			outputGetDway(dip);
			outputGetPortStatus(dip);
		}
	}
	outputWrite(dip);
}

static void outputDataHandle(void)
{
	//output by configuration order
	llistTravel(detect.data.pllist, resultTravel);
	fclose(ofp);
}

static int outputOpenFile(void)
{
	xmemzero(oldpath, BUF_SIZE_512);
	xmemzero(newpath, BUF_SIZE_512);
	new_timestamp = time(NULL);
	snprintf(oldpath, BUF_SIZE_512,
			"%s/link.data.tmp",
			LINKDATA_DIR);
	snprintf(newpath, BUF_SIZE_512,
			"%s/link.data.%ld",
			LINKDATA_DIR, new_timestamp);
	ofp = xfopen(oldpath, "a+");

	return (NULL == ofp) ? FAILED : SUCC;
}

static int outputRename(void)
{
	if (rename(oldpath, newpath) < 0)
	{
		dlog(ERROR, NOFD,
				"rename '%s' to '%s' failed, ERROR=[%s]",
				oldpath, newpath, xerror());
		return FAILED;
	}
	return SUCC;
}

static void outputClean(void)
{
	cleaner(new_timestamp);
}

int outputCheckPath(void)
{
	if (access(LINKDATA_DIR, F_OK) < 0)
	{
		if (mkdir(LINKDATA_DIR, 0755) < 0)
		{
			dlog(WARNING, NOFD,
					"mkdir() failed, PATH=[%d]",
					LINKDATA_DIR);
			return FAILED;
		}
	}

	return SUCC;
}

void outputStart(void)
{
	debug(BEFORE_OUTPUT);
	outputCheckPath();
	outputOpenFile();
	outputDataHandle();
	outputRename();
	outputClean();
	debug(AFTER_OUTPUT);
}

int outputCall(const int calltype)
{
	switch (calltype)
	{
		case START_OUTPUT:
			outputStart();
			return STATE_RESULT;
		default:
			dlog(WARNING, NOFD,
					"no this call type, TYPE=[%d]",
					calltype);
			return FAILED;
	}

	return FAILED;
}


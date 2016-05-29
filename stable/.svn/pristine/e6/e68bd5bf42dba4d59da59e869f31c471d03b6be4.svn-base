#include "detect.h"

struct data_result1_st {
	struct {
		unsigned int ok:1;
		unsigned int good:1;
		unsigned int warning:1;
	} status;
	data_ip_t *dip;
};

struct data_result_st {
	struct {
		unsigned int number:8;
		unsigned int work:1;
		unsigned int down:1;
		unsigned int good:1;
	} all;
	struct data_result1_st *ips;
};

struct detect_result_st {
	detect_cfg_t *rfc;
	struct data_result_st ip;
	struct data_result_st bkip;
};

struct dns_soa_st {
	char *ns;
	char *nsip;
	char *owner;
	char *email;
	char *origin;
	char *SOA;
	uint32_t ttl;
	uint32_t minttl;
	uint32_t serial;
	uint32_t refresh;
	uint32_t retry;
	uint32_t expire;
	uint32_t nsttl;
};

enum {
	E_DUMP_BEGIN,
	E_DUMP_FINISH
};

enum {
	DNS_RELOAD,
	DNS_RESTART,
};

#define SOA_FMT \
";-----\n\
$TTL %d\n\
$ORIGIN %s\n\
%s IN SOA %s %s (\n\
		%d;\n\
		%d;\n\
		%d;\n\
		%d;\n\
		%d;\n\
		);\n\
%s %d IN NS %s\n\
%s %d IN A %s\n\
;-----\n"

static FILE *tfp = NULL;   //anyhost.tmp's fp
static FILE *nfp = NULL;   //anyhost's fp
static FILE *dfp = NULL;   //dump file's fp
static struct dns_soa_st *soa = NULL;
static const char *IP_BAD = "ip_bad";
static const char *IP_WORK = "ip_work";
static const char *IP_GOOD = "ip_good";
static const char *IP_DOWN = "ip_down";
static const char *NO_DETECT = "no_detect";
#define FMT_IP_VALID "%-24s IN      A       %-15s ; %.6f ip %s %s %s\n"
#define FMT_IP_INVALID ";%-23s IN      A       %-15s ; %.6f ip %s %s %s\n"
#define FMT_IP_NODETECT_VALID "%-24s IN      A       %-15s ; %.6lf ip %s %s %s\n"
#define FMT_IP_NODETECT_INVALID ";%-23s IN      A       %-15s ; %.6lf ip %s %s %s\n"
#define FMT_BKUP_VALID "%-24s IN      A       %-15s ; %.6f backup %s %s %s\n"
#define FMT_BKUP_INVALID ";%-23s IN      A       %-15s ; %.6f backup %s %s %s\n"
#define FMT_BKUP_NODETECT_INVALID ";%-23s IN      A       %-15s ; 0.000000 backup no_detect 000 N\n"
#define FMT_BKUP_CNAME "%-24s IN      CNAME   %s\n"
#define FMT_DUMP "%-4s %-24s %-15s %s %.6lf %.6lf %.6lf %.6lf %-7s %s\n"
#define DUMP_BEGIN  "detectorig Starting: %s ==================\n"
#define DUMP_FINISH "detectorig Finished: %s ==================\n"
static struct detect_result_st RS;

static void dnsInitSOA(void)
{
	char dns_soa[BUF_SIZE_2048];

	soa = xcalloc(1, sizeof(struct dns_soa_st));
	soa->ttl = 60;
	/* use current time as the serial number */
	soa->serial = time(NULL);
	soa->refresh = 7000;
	soa->retry = 3000;
	soa->expire = 15000;
	soa->minttl = 86400;
	soa->nsttl = 86400;
	soa->ns = xstrdup("ns1");;
	soa->nsip = xstrdup("127.0.0.1");;
	soa->origin = xstrdup(".");;
	soa->owner = xstrdup("@");
	soa->email = xstrdup("root.localhost.");
	xmemzero(dns_soa, BUF_SIZE_2048);
	snprintf(dns_soa,
			BUF_SIZE_2048,
			SOA_FMT,
			soa->ttl,
			soa->origin,
			soa->owner,
			soa->ns,
			soa->email,
			soa->serial,
			soa->refresh,
			soa->retry,
			soa->expire,
			soa->minttl,
			soa->owner,
			soa->nsttl,
			soa->ns,
			soa->ns,
			soa->nsttl,
			soa->nsip);
	soa->SOA = xstrdup(dns_soa);
}

static void dnsProcess(const int type)
{
	FILE *fp = NULL;
	char *left = NULL;
	char *right = NULL;
	char *param = NULL;
	char *keyword = NULL;
	char buffer[BUF_SIZE_512];
	char command[BUF_SIZE_512];

	switch (type)
	{
		case DNS_RELOAD:
			param = "reload";
			break;
		case DNS_RESTART:
			param = "restart";
			break;
		default:
			xabort("no this DNS process type");
	}
	xmemzero(command, BUF_SIZE_512);
	snprintf(command, 
			BUF_SIZE_512,
			"%s %s",
			detect.opts.DNSservice,
			param);
	fp = popen(command, "r");
	if (NULL == fp)
	{
		dlog(ERROR, NOFD,
				"popen(%s) for named reload failed, ERROR=[%s]",
				command, xerror());
		return;
	}
	xmemzero(buffer, BUF_SIZE_512);
	fread(buffer, 1, BUF_SIZE_512, fp);
	right = strrchr(buffer, ']');
	if (NULL == right)
		goto NRF;
	left = strrchr(buffer, '[');
	if (NULL == left || left > right)
		goto NRF;
	keyword = strstr(left, "OK");
	if (NULL == keyword || keyword < left || keyword > right)
		goto NRF;
	pclose(fp);
	return;

NRF:
	if (NULL != fp)
		pclose(fp);
	dlog(ERROR, NOFD, "%s failed", command);
	return;
}

static void dnsReload(void)
{
	dnsProcess(DNS_RELOAD);
}

static void dnsReloadCheck(void)
{
	int serial = 0;
	FILE *fp = NULL;
	char *token = NULL;
	char *backup = NULL;
	char command[BUF_SIZE_512];
	char buffer[BUF_SIZE_1024];
	const char *dig = "/usr/bin/dig";
	const char *digip = "127.0.0.1";
	const uint16_t port = 53;
	const char *zone = ".";

	/* /usr/bin/dig -p 53 @127.0.0.1 . soa +norec +short */

	snprintf(command, BUF_SIZE_512,
			"%s -p %d @%s %s soa +norec +short",
			dig, port, digip, zone);
	fp = popen(command, "r");
	if (NULL == fp)
	{
		dlog(ERROR, NOFD,
				"popen(%s) for named reload check failed, ERROR=[%s]",
				command, xerror());
		return;
	}
	xmemzero(buffer, BUF_SIZE_1024);
	fread(buffer, 1, BUF_SIZE_1024, fp);
	pclose(fp);
	backup = xstrdup(buffer);
	strtok(buffer, " \t");
	strtok(NULL, " \t");
	/* here, SOA serial */
	token = strtok(NULL, " \t");
	if (NULL == token)
	{
		dlog(WARNING, NOFD,
				"DNS dig have no SOA serial, dig SOA: %s",
				backup);	
		return;
	}
	serial = atoi(token);
	if (serial != soa->serial)
	{
		dlog(WARNING, NOFD,
				"dig DNS serial(%d) unmatch (%d);\nSOA: %s;\nRetart DNS Now\n",
				serial, soa->serial, backup);
		dnsProcess(DNS_RESTART);
	}
}

static void anyhostWrite(
		const char *format, 
		const char *status,
		const char *warning,
		const detect_output_t *dop
		)
{
	fprintf(tfp, format, 
			dop->hostname, dop->sip, dop->ustime,
			status, dop->code, warning);
	fprintf(dfp, FMT_DUMP,
			methods[RS.rfc->method1->method],
			dop->hostname, dop->sip, dop->code,
			dop->cutime, dop->fbtime, dop->dltime,
			dop->ustime, status, dop->iptype);
}

static void anyhostWriteSOA(void)
{
	dnsInitSOA();
	rewind(tfp);
	fprintf(tfp, "%s", soa->SOA);
	fflush(tfp);
}

static int checkStatusDown(detect_ip_t *dip)
{
	int i;
	const char *code;

	if (0 == dip->dop.oktimes)
		return 1;
	if (dip->dop.ustime > dip->rfc->timeout)
		return 1;
	for (i = 0; i < dip->rfc->codes.num; ++i)
	{
		code = dip->rfc->codes.code[i].code;
		if (!strncmp(code, dip->dop.code, strlen(code)))
			return 0;
	}
	return 1;
}

static void anyhostRecordOneByNodetect(void)
{
	int i, ngood;
	const char *FORMAT = NULL;

	ngood = RS.rfc->ngood > RS.ip.all.number ? RS.ip.all.number : RS.rfc->ngood;

	for (i = 0; i < RS.ip.all.number; ++i)
	{
		FORMAT = ngood-- > 0 ? FMT_IP_NODETECT_VALID : FMT_IP_NODETECT_INVALID;
		anyhostWrite(FORMAT, NO_DETECT, "N", &RS.ip.ips[i].dip->rfc->dop);
	}
	for (i = 0; i < RS.bkip.all.number; ++i)
	{
		anyhostWrite(FMT_BKUP_NODETECT_INVALID,
				NO_DETECT, "N",
				&RS.bkip.ips[i].dip->rfc->dop);
	}
}

static void anyhostRecordOneByGoodIp(void)
{
	int i, ngood;
	const char *status, *FORMAT;

	ngood = RS.rfc->ngood > RS.ip.all.number ? RS.ip.all.number : RS.rfc->ngood;

	for (i = 0; i < RS.ip.all.number; ++i)
	{
		if (RS.ip.ips[i].status.ok)
		{
			status = RS.ip.ips[i].status.good ? (ngood > 0 ? IP_WORK : IP_GOOD) : IP_BAD;
			FORMAT = RS.ip.ips[i].status.good ? (ngood-- > 0 ? FMT_IP_VALID : FMT_IP_INVALID) : FMT_IP_INVALID;
		}
		else
		{
			status = IP_DOWN;
			FORMAT = FMT_IP_INVALID;
		}
		anyhostWrite(FORMAT, status,
				RS.ip.ips[i].status.warning ? "Y" : "N",
				&RS.ip.ips[i].dip->rfc->dop);
	}
	for (i = 0; i < RS.bkip.all.number; ++i)
	{
		if (RS.bkip.ips[i].status.ok)
			status = RS.bkip.ips[i].status.good ? IP_GOOD : IP_BAD;
		else
			status = IP_DOWN;
		anyhostWrite(FMT_BKUP_INVALID, status,
				RS.bkip.ips[i].status.warning ? "Y" : "N",
				&RS.bkip.ips[i].dip->rfc->dop);
	}
}

static void anyhostRecordOneByBackupYES(void)
{
	int i;

	for (i = 0; i < RS.ip.all.number; ++i)
	{
		anyhostWrite(FMT_IP_INVALID,
				RS.ip.ips[i].status.ok ? IP_BAD : IP_DOWN,
				RS.ip.ips[i].status.warning ? "Y" : "N",
				&RS.ip.ips[i].dip->rfc->dop);
	}
}

static void anyhostRecordOneByBackupNO(void)
{
	int i, mark;
	const char *status, *FORMAT;

	for (i = 0, mark = 0; i < RS.ip.all.number; ++i)
	{
		//if all down, record all ip to A
		if (RS.ip.all.down)
		{
			anyhostWrite(FMT_IP_VALID, IP_DOWN,
					RS.ip.ips[i].status.warning ? "Y" : "N",
					&RS.ip.ips[i].dip->rfc->dop);
			continue;
		}
		//otherwise, record by ustime
		if (RS.ip.ips[i].status.ok)
		{
			status = IP_BAD;
			FORMAT = (0 == mark++) ? FMT_IP_VALID : FMT_IP_INVALID;
		}
		else
		{
			status = IP_DOWN;
			FORMAT = FMT_IP_INVALID;
		}
		anyhostWrite(FORMAT, status,
				RS.ip.ips[i].status.warning ? "Y" : "N",
				&RS.ip.ips[i].dip->rfc->dop);
	}
}

static void anyhostRecordOneByBackupCNAME(void)
{
	int i;

	fprintf(tfp, FMT_BKUP_CNAME, RS.rfc->hostname, RS.rfc->backup);

	for (i = 0; i < RS.ip.all.number; ++i)
	{
		anyhostWrite(FMT_IP_INVALID,
				RS.ip.ips[i].status.ok ? IP_BAD : IP_DOWN,
				RS.ip.ips[i].status.warning ? "Y" : "N",
				&RS.ip.ips[i].dip->rfc->dop);
	}
}

static void anyhostRecordOneByBackupIP(void)
{
	int i, mark;	
	const char *status, *FORMAT;

	//ip have good ip
	if (RS.bkip.all.good)
	{
		for (i = 0, mark = 0; i < RS.bkip.all.number; ++i)
		{
			if (RS.bkip.ips[i].status.ok)
			{
				status = RS.bkip.ips[i].status.good ? (0 == mark ? IP_WORK : IP_GOOD) : IP_BAD;
				FORMAT = RS.bkip.ips[i].status.good ? (0 == mark++ ? FMT_BKUP_VALID : FMT_BKUP_INVALID) : FMT_BKUP_INVALID;
			}
			else
			{
				status = IP_DOWN;
				FORMAT = FMT_BKUP_INVALID;
			}
			anyhostWrite(FORMAT, status,
					RS.bkip.ips[i].status.warning ? "Y" : "N",
					&RS.bkip.ips[i].dip->rfc->dop);
		}
		for (i = 0; i < RS.ip.all.number; ++i)
		{
			anyhostWrite(FMT_IP_INVALID,
					RS.ip.ips[i].status.ok ? IP_BAD : IP_DOWN,
					RS.ip.ips[i].status.warning ? "Y" : "N",
					&RS.ip.ips[i].dip->rfc->dop);
		}
		return;
	}
	//ip all down
	if (0 == RS.ip.all.down)
	{
		for (i = 0, mark = 0; i < RS.ip.all.number; ++i)
		{
			if (RS.ip.ips[i].status.ok)
			{
				status = IP_BAD;
				FORMAT =  (0 == mark++) ? FMT_IP_VALID : FMT_IP_INVALID;	
			}
			else
			{
				status = IP_DOWN;
				FORMAT = FMT_IP_INVALID;	
			}
			anyhostWrite(FORMAT, status,
					RS.ip.ips[i].status.warning ? "Y" : "N",
					&RS.ip.ips[i].dip->rfc->dop);
		}
		for (i = 0; i < RS.bkip.all.number; ++i)
		{
			anyhostWrite(FMT_BKUP_INVALID,
					RS.bkip.ips[i].status.ok ? IP_BAD : IP_DOWN,
					RS.bkip.ips[i].status.warning ? "Y" : "N",
					&RS.bkip.ips[i].dip->rfc->dop);
		}
		return;
	}
	//backup ip all down
	if (RS.bkip.all.down)
	{
		for (i = 0; i < RS.ip.all.number; ++i)
		{
			anyhostWrite(FMT_IP_VALID, IP_DOWN,
					RS.ip.ips[i].status.warning ? "Y" : "N",
					&RS.ip.ips[i].dip->rfc->dop);
		}
		for (i = 0; i < RS.bkip.all.number; ++i)
		{
			anyhostWrite(FMT_BKUP_INVALID, IP_DOWN,
					RS.bkip.ips[i].status.warning ? "Y" : "N",
					&RS.bkip.ips[i].dip->rfc->dop);
		}
		return;
	}
	//ip all down, but backup ip not all down
	if (RS.ip.all.down && !RS.bkip.all.down)
	{
		for (i = 0; i < RS.bkip.all.number; ++i)
		{
			if (RS.bkip.ips[i].status.ok)
			{
				status = IP_BAD;
				FORMAT =  FMT_BKUP_VALID;	
			}
			else
			{
				status = IP_DOWN;
				FORMAT = FMT_BKUP_INVALID;	
			}
			anyhostWrite(FORMAT, status,
					RS.bkip.ips[i].status.warning ? "Y" : "N",
					&RS.bkip.ips[i].dip->rfc->dop);
		}
		for (i = 0; i < RS.ip.all.number; ++i)
		{
			anyhostWrite(FMT_IP_INVALID, IP_DOWN,
					RS.ip.ips[i].status.warning ? "Y" : "N",
					&RS.ip.ips[i].dip->rfc->dop);
		}
	}
}

static void anyhostRecordOneByBackup(void)
{
	//backup is not ip
	if (0 == RS.bkip.all.number)
	{
		if (!strcasecmp(RS.rfc->backup, "YES") || !strcasecmp(RS.rfc->backup, "Y"))
		{  
			anyhostRecordOneByBackupYES();
			return;
		}
		if (!strcasecmp(RS.rfc->backup, "NO") || !strcasecmp(RS.rfc->backup, "N"))
		{
			anyhostRecordOneByBackupNO();
			return;
		}
		anyhostRecordOneByBackupCNAME();
		return;
	}
	//backup is ip
	anyhostRecordOneByBackupIP();
}

static void anyhostRecordOneByDetectTime(void)
{
	if (RS.ip.all.good)
		anyhostRecordOneByGoodIp();
	else
		anyhostRecordOneByBackup();
}

static void anyhostRecordOneByIpNumberZero(void)
{
	int i, rs0, rs1, rs2, rs3;
	const char *status;

	if (0 == RS.bkip.all.number)
	{
		if (NULL == RS.rfc->backup)
		{
			dlog(ERROR, NOFD, "backup config error, HOSTNAME=[%s]", RS.rfc->hostname);
			return;
		}
		rs0 = strcasecmp(RS.rfc->backup, "yes");
		rs1 = strcasecmp(RS.rfc->backup, "y");
		rs2 = strcasecmp(RS.rfc->backup, "no");
		rs3 = strcasecmp(RS.rfc->backup, "n");
		if (rs0 && rs1 && rs2 && rs3)
			fprintf(tfp, FMT_BKUP_CNAME, RS.rfc->hostname, RS.rfc->backup);
		return;
	}
	for (i = 0; i < RS.bkip.all.number; ++i)
	{
		if (RS.bkip.ips[i].status.ok)
			status = RS.bkip.ips[i].status.good ? IP_WORK : IP_BAD;
		else
			status = IP_DOWN;
		anyhostWrite(FMT_BKUP_VALID, status,
				RS.bkip.ips[i].status.warning ? "Y" : "N",
				&RS.bkip.ips[i].dip->rfc->dop);
	}
}

static  void anyhostRecordOne2(void)
{
	//main ip number is zero
	if (0 == RS.ip.all.number)
	{
		anyhostRecordOneByIpNumberZero();
		return;
	}
	//no detect
	if (0 == RS.rfc->flags.detect)
	{
		anyhostRecordOneByNodetect();
		return;
	}
	//otherwise
	anyhostRecordOneByDetectTime();
}

static void sortRS(struct data_result_st *DST)
{
	int i, j;
	size_t size;
	struct data_result1_st tmp;
	struct data_result1_st *dip1;
	struct data_result1_st *dip2;

	size = sizeof(struct data_result1_st);

	for (i = 0; i < DST->all.number -1 ; ++i)
	{
		dip1 = DST->ips + i;
		for (j = i + 1; j < DST->all.number; ++j)
		{
			dip2 = DST->ips + j;
			if (dip1->dip->rfc->dop.ustime > dip2->dip->rfc->dop.ustime)
			{
				memcpy(&tmp, dip1, size);
				memcpy(dip1, dip2, size);
				memcpy(dip2, &tmp, size);
			}
		}
	}
}

static void printRS(const char *prefix)
{
	int i;

	if (!detect.opts.F.print)
		return;

	for (i = 0; i < RS.ip.all.number; ++i)
	{
		printf(" ==> %s main ip: %s, ustime: %.6lf\n",
				prefix, RS.ip.ips[i].dip->ip,
				RS.ip.ips[i].dip->rfc->dop.ustime);
	}
	for (i = 0; i < RS.bkip.all.number; ++i)
	{
		printf(" ==> %s backup ip: %s, ustime: %.6lf\n",
				prefix, RS.bkip.ips[i].dip->ip,
				RS.bkip.ips[i].dip->rfc->dop.ustime);
	}
}

static void anyhostSortRS(void)
{
	if (RS.rfc->flags.sort)
	{
		printRS("before sort: ");
		sortRS(&RS.ip);
		sortRS(&RS.bkip);
		printRS("after sort: ");
	}
}

static void anyhostCheckOK(void)
{
	int i, ok;
	double ustime;

	for (i = 0, ok = 0; i < RS.ip.all.number; ++i)
	{
		ustime = RS.ip.ips[i].dip->rfc->dop.ustime;
		if (checkStatusDown(RS.ip.ips[i].dip->rfc))
		{
			RS.ip.ips[i].status.ok = 0;
			RS.ip.ips[i].status.warning = (ustime > DOUBLE_NUMBER_MIN && ustime > RS.rfc->wntime) ? 1 : 0;
			continue;	
		}
		++ok;
		RS.ip.ips[i].status.ok = 1;
		RS.ip.ips[i].status.good = ustime < RS.rfc->gdtime ? 1 : 0;
		RS.ip.ips[i].status.warning = ustime > RS.rfc->wntime ? 1 : 0;
		if (!RS.ip.all.good && RS.ip.ips[i].status.good)
			RS.ip.all.good = 1;
	}
	RS.ip.all.down = (0 == ok) ? 1 : 0;
	RS.ip.all.work = (ok == RS.ip.all.number) ? 1 : 0;
	for (i = 0, ok = 0; i < RS.bkip.all.number; ++i)
	{
		ustime = RS.bkip.ips[i].dip->rfc->dop.ustime;
		if (checkStatusDown(RS.bkip.ips[i].dip->rfc))
		{
			RS.bkip.ips[i].status.ok = 0;
			RS.bkip.ips[i].status.warning = (ustime > DOUBLE_NUMBER_MIN && ustime > RS.rfc->wntime) ? 1 : 0;
			continue;	
		}
		++ok;
		RS.bkip.ips[i].status.ok = 1;
		RS.bkip.ips[i].status.good = (ustime < RS.rfc->gdtime) ? 1 : 0;
		RS.bkip.ips[i].status.warning = ustime > RS.rfc->wntime ? 1 : 0;
		if (!RS.bkip.all.good && RS.bkip.ips[i].status.good)
			RS.bkip.all.good = 1;
	}
	RS.bkip.all.down = (0 == ok) ? 1 : 0;
	RS.bkip.all.work = (ok == RS.bkip.all.number) ? 1 : 0;
}

static void anyhostRecordOne(void *data)
{
	int i;
	size_t size, psize;
	detect_cfg_t *cfg = data;
	struct data_result_st *P = NULL;

	xmemzero(&RS, sizeof(RS));
	psize = sizeof(struct data_result1_st);
	RS.rfc = cfg;

	for (i = 0; i < cfg->ips.num; ++i)
	{
		switch (cfg->ips.ip[i].iptype)
		{
			case IPTYPE_IP:
			case IPTYPE_DNS:
				P = &RS.ip;
				break;
			case IPTYPE_BACKUP:
				P = &RS.bkip;
				break;
			default:
				xabort("anyhostRecordOne: no this iptype");
		}
		size = (P->all.number + 1) * psize;
		P->ips = P->all.number ? xrealloc(P->ips, size) : xcalloc(1, size);
		P->ips[P->all.number].dip = cfg->ips.ip + i;
		P->all.number++;
	}
	anyhostCheckOK();
	anyhostSortRS();
	anyhostRecordOne2();
}

static void anyhostDumpIdentify(const int type)
{
	char timebuf[BUF_SIZE_256];	

	switch (type)
	{
		case E_DUMP_BEGIN:
			xmemzero(timebuf, BUF_SIZE_256);
			strftime(timebuf,
					BUF_SIZE_256,
					"%Y/%m/%d %H:%M:%S",
					localtime(&detect.runstate.starttime));
			fprintf(dfp, DUMP_BEGIN, timebuf);
			break;
		case E_DUMP_FINISH:
			xmemzero(timebuf, BUF_SIZE_256);
			strftime(timebuf,
					BUF_SIZE_256,
					"%Y/%m/%d %H:%M:%S",
					localtime(&detect.runstate.endtime));
			fprintf(dfp, DUMP_FINISH, timebuf);
			break;
		default:
			xabort("anyhostDumpIdentify() no this type");
	}
	fflush(dfp);
}

static void anyhostRecord(void)
{
	anyhostDumpIdentify(E_DUMP_BEGIN);
	llistTravel(detect.data.cllist, anyhostRecordOne);
	anyhostDumpIdentify(E_DUMP_FINISH);
}

static void anyhostOpen(void)
{
	//remove anyhost.tmp if it exist
	unlink(ANYHOST_TMP);
	tfp = xfopen(ANYHOST_TMP, "a");
	if (NULL == tfp)
		xabort("fopen() for anyhost.tmp failed");
	dfp = xfopen(ANYHOST_DUMP, "a");
	if (NULL == dfp)
		xabort("open for detectorigin.log failed");
	nfp = xfopen(ANYHOST_NEW, "r");
	anyhostWriteSOA();
}

static void anyhostClose(void)
{
	xfclose(&tfp);
	xfclose(&nfp);
	xfclose(&dfp);
}

static void anyhostRename(void)
{
	if (rename(ANYHOST_TMP, ANYHOST_NEW) < 0)
	{   
		dlog(ERROR, NOFD,
				"rename '%s' to '%s' failed, ERROR=[%s]",
				ANYHOST_TMP, ANYHOST_NEW, xerror());
		xabort("resultRename() failed, abort now");
	}   
}

static void anyhostHandle(void)
{
	anyhostOpen();
	anyhostRecord();
	anyhostClose();
	anyhostRename();
}

static void dnsHandle(void)
{
	dnsReload();
	dnsReloadCheck();
}

static void resultStart(void)
{
	//if not open the switch for result, return immediately
	if (!detect.opts.F.result)
		return;
	debug(BEFORE_RESULT);
	anyhostHandle();
	dnsHandle();
	debug(AFTER_RESULT);
}

int resultCall(const int calltype)
{
	switch (calltype)
	{
		case START_RESULT:
			resultStart();
			return STATE_FINISHED;
		default:
			dlog(WARNING, NOFD, "no this call type, TYPE=[%d]", calltype);
			return FAILED;
	}
	return FAILED;
}


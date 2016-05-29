#include "detect.h"

static const char help_info[] =   
"Usage: detectorig [options] [arguments]\n"
"\t-c, --central       detect once when more than one same ip\n"
"\t-d, --debug         set debug information level\n"
"\t-e, --epollwait     epollwait timeout\n"
"\t-n, --travel        travel number\n"
"\t-D, --discleanup    disable internal cleanup\n"
"\t-f  conffile        detect configuration file\n"
"\t                    default: /usr/local/squid/etc/domain.conf\n"
"\t-h, --help          print the help message\n"
"\t-k killtime         how long to kill current program instance\n"
"\t--kill=killtime     the same to -k\n"
"\t-M                  check\n"
"\t                    Parse configuration file\n"
"\t-p                  print debug information\n"
"\t-r                  create and update anyhost\n"
"\t-s                  disable delay start\n"
"\t-F, --fc            read and detect FC's domain.conf\n"
"\t-N, --nofc          don't read and detect FC's domain.conf\n"
"\t-T, --tta           read and detect TTA's tta_domain.conf\n"
"\t-S savetime         link.data files retain how long(seconds)\n"
"\t--savetime=savetime the same to -S\n"
"\t-R rlimit           reset progress recource limits\n"
"\t-t timeout          timeout for once detect, default: 3.000000s\n"
"\t--timeout=timeout   the same to -t\n"
"\t-v, --version       print version, subversion\n"
;

void printVersion(void)
{
	printf("%s, BuildTime: %s %s\nCopyright (C) 2012 ChinaCache\n",
			DETECTORIG_VERSION, __DATE__, __TIME__);
}

static void handleOptionSavetime(const char *optstr)
{
	switch (optstr[strlen(optstr) - 1])
	{
		case 's':
		case 'S':
			detect.opts.savetime = atol(optstr);
			break;
		case 'm':
		case 'M':
			detect.opts.savetime = atol(optstr) * 60;
			break;
		case 'h':
		case 'H':
			detect.opts.savetime = atol(optstr) * 60 * 60;
			break;
		case 'd':
		case 'D':
			detect.opts.savetime = atol(optstr) * 24 * 60 * 60;
			break;
		default:
			detect.opts.savetime = SAVETIME_MIN;
			break;
	}
	if (detect.opts.savetime < SAVETIME_MIN)
		detect.opts.savetime = SAVETIME_MIN;
}

static void parseCmdLineParams(int argc, char **argv)
{
	int opt, option_index;
	const char *optstring;
	struct option long_options[] = {
		{"fc",         0, NULL, 'F'},
		{"tta",        0, NULL, 'T'},
		{"nofc",       0, NULL, 'N'},
		{"help",       0, NULL, 'h'},
		{"kill",       1, NULL, 'k'},
		{"debug",      1, NULL, 'd'},
		{"rlimit",     1, NULL, 'R'},
		{"travel",     1, NULL, 'n'},
		{"timeout",    1, NULL, 't'},
		{"version",    0, NULL, 'v'},
		{"central",    0, NULL, 'c'},
		{"savetime",   1, NULL, 'S'},
		{"epollwait",  1, NULL, 'e'},
		{"discleanup", 0, NULL, 'D'},
		{0, 0, 0, 0}
	};

	detect.config.domain.mode = MODE_FC;
	optstring = "d:e:f:t:k:n:R:S:M:chprsvDNTF";

	do {
		opt = getopt_long(argc, argv, optstring, long_options, &option_index);
		if (-1 == opt)
			break;
		switch (opt) {
			case 'p':
				detect.opts.F.print = 1;
				break;
			case 'r':
				detect.opts.F.result = 1;
				break;
			case 'c':
				detect.opts.F.central = 1;
				break;
			case 'D':
				detect.opts.F.cleanup = 0;
				break;
			case 's':
				detect.opts.F.dlstart = 0;
				break;
			case 'n':
				detect.opts.travel = atoi(optarg);
				break;
			case 'd':
				detect.opts.F.dblevel = atoi(optarg);
				break;
			case 't':
				detect.opts.timeout = atof(optarg);
				break;
			case 'k':
				detect.opts.killtime = atol(optarg);
				break;
			case 'M':
				if ((int) strlen(optarg) < 1)
					printf("%s", help_info);
				if (!strncmp(optarg, "check", strlen(optarg)))
					detect.opts.check_flag = 1;
				else
					printf("%s", help_info);
				break;
			case 'e':
				detect.opts.epollwait = atoi(optarg);
				break;
			case 'R':
				detect.opts.rlimit = (rlim_t)atol(optarg);
				break;
			case 'f':
				xmemzero(detect.config.domain.domainFile, BUF_SIZE_256);
				snprintf(detect.config.domain.domainFile, 
						BUF_SIZE_256, "%s", optarg);
				break;
			case 'F':
				detect.config.domain.mode |= MODE_FC;
				break;
			case 'N':
				detect.config.domain.mode &= MODE_NOFC;
				break;
			case 'S':
				handleOptionSavetime(optarg);
				break;
			case 'T':
				detect.config.domain.mode |= MODE_TTA;
				xmemzero(detect.config.domain.ttaConfFile, BUF_SIZE_256);
				snprintf(detect.config.domain.ttaConfFile, 
						BUF_SIZE_256,
						CONFIG_DOMAIN_TTA);
				break;
			case 'h':
				printf("%s", help_info);
				writeDetectIdentify(STATE_TERMINATE);
				exit(0);
			case 'v':
				printVersion();
				writeDetectIdentify(STATE_TERMINATE);
				exit(0);
			case ':':
				fprintf(stderr, "option needs a value, OPTION=[%c]\n", optopt);
				dlog(WARNING, NOFD,
						"option needs a value, OPTION=[%c]\n", optopt);
				break;
			case '?':
				fprintf(stderr, "unkown command option, OPTION=[%c]", opt);
				dlog(WARNING, NOFD,
						"unkown command option, OPTION=[%c]", opt);
				break;
		}
	} while (1);

	if (0 == detect.config.domain.mode)
		detect.config.domain.mode = MODE_FC;   //default: FC domain.conf
	if (detect.opts.F.print)
		debug(PRINT_START);
}

static void writePidFile(void)
{
	time_t tm; 
	int fd, val;
	mode_t old_mask;
	struct flock lock;
	char buf[BUF_SIZE_256];

	if (access(DETECT_PID_DIR, F_OK) < 0)
	{   
		if (mkdir(DETECT_PID_DIR, 0755) < 0)
		{   
			fprintf(stderr,
					"mkdir() failed, DIR=[%s], ERROR=[%s]",
					DETECT_PID_DIR, xerror());
			dlog(WARNING, NOFD,
					"mkdir() failed, DIR=[%s], ERROR=[%s]",
					DETECT_PID_DIR, xerror());
			xabort("abort now");
		}   
	}  
	old_mask = umask(022);
	fd = open(DETECT_PID_FILE, O_WRONLY|O_CREAT|O_TRUNC|O_APPEND, 0644);
	if(fd < 0)
	{
		dlog(WARNING, NOFD, "open(pidfile) failed, ERROR=[%s]", xerror());
		xabort("abort now");
	}
	umask(old_mask);
	//set file write lock
	lock.l_len = 0;
	lock.l_start = 0;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLK, &lock) < 0)
	{
		fprintf(stderr, "detectorig is running, retry later please!\n");
		dlog(WARNING, NOFD, 
				"detectorig is running, retry later please, ERROR=[%s]",
				xerror());
		exit(1);
	}   
	tm = time(NULL);
	xmemzero(buf, BUF_SIZE_256);
	sprintf(buf, "%d\n%ld\n", getpid(), tm);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
	{
		dlog(WARNING, NOFD, "write(pidfile) failed, ERROR=[%s]", xerror());
		xabort("abort now");
	}
	val = fcntl(fd, F_GETFD, 0);
	if(val < 0) 
	{
		dlog(WARNING, NOFD, "fcntl(F_GETFD) failed, ERROR=[%s]", xerror());
		xabort("abort now");
	}
	//set fd flag: close-on-exec
	val |= FD_CLOEXEC;
	if(fcntl(fd, F_SETFD, val) < 0)
	{
		dlog(WARNING, NOFD, "fcntl(F_SETFD) failed, ERROR=[%s]", xerror());
		xabort("abort now");
	}
}

static void writeDetectTimes(void)
{
	FILE *fp;
	char buf[BUF_SIZE_256];

	if (NULL == (fp = xfopen(DETECT_TIME_FILE, "a+")))
		return;
	xmemzero(buf, BUF_SIZE_256); 
	if (NULL == fgets(buf, BUF_SIZE_256, fp))
	{
		detect.runstate.runtimes = 1;
		detect.runstate.lasttime = time(NULL);
	}
	else
	{
		detect.runstate.runtimes = atoi(buf) + 1;
		xmemzero(buf, BUF_SIZE_256); 
		if (NULL == fgets(buf, BUF_SIZE_256, fp))
			detect.runstate.lasttime = time(NULL);
		else
			detect.runstate.lasttime = atol(buf);
	}
	if( truncate(DETECT_TIME_FILE, 0) )
            fprintf(stdout, "Something wrong with truncate()\n");
	fseek(fp, 0, SEEK_SET);
	fprintf(fp, "%d\n%ld\n", detect.runstate.runtimes, (long)time(NULL));
	fflush(fp);
	fclose(fp);
	if (detect.opts.F.print)
	{
		fprintf(stdout,
				" ==> lastest run time: %ld\n",
				(long)detect.runstate.lasttime);
		fprintf(stdout,
				" ==> lastest run times: %d\n",
				detect.runstate.runtimes - 1);
		fflush(stdout);
	}
}


static void typeSizeInit(void)
{
	detect.tsize.code_t_size = sizeof(code_t);
	detect.tsize.codes_t_size = sizeof(codes_t);
	detect.tsize.data_ip_t_size = sizeof(data_ip_t);
	detect.tsize.detect_ip_t_size = sizeof(detect_ip_t);
	detect.tsize.data_index_t_size = sizeof(data_index_t);
	detect.tsize.detect_cfg_t_size = sizeof(detect_cfg_t);
	detect.tsize.hash_factor_t_size = sizeof(hash_factor_t);
	detect.tsize.http_header_t_size = sizeof(http_header_t);
	detect.tsize.ctl_keyword_t_size = sizeof(ctl_keyword_t);
	detect.tsize.detect_unit_t_size = sizeof(detect_unit_t);
	detect.tsize.detect_event_t_size = sizeof(detect_event_t);
	detect.tsize.sockaddr_in_size = sizeof(struct sockaddr_in);
	detect.tsize.detect_config_t_size = sizeof(detect_config_t);
	detect.tsize.detect_output_t_size = sizeof(detect_output_t);
}

static void detectOptsInit(void)
{
	detect.opts.timeout = 0;
	detect.opts.F.print = 0;
	detect.opts.F.result = 0;
	detect.opts.F.dblevel = 0;
	detect.opts.F.central = 0;
	detect.opts.F.cleanup = 1;
	detect.opts.F.dlstart = 1;
	detect.opts.travel = TRAVEL_NUMBER;
	detect.opts.rlimit = DETECT_RLIMIT;
	detect.opts.epollwait = EPOLL_TIMEOUT;
	detect.opts.killtime = DETECT_KILLTIME;
	detect.opts.savetime = SAVETIME_DEFAULT;
	detect.opts.DNSservice = xstrdup(DNSSERVICE_PATH);
	detect.opts.check_flag = 0;
}

static void detectRunstateInit(void)
{
	detect.runstate.count = 0;
	detect.runstate.pid = getpid();
	time(&detect.runstate.starttime);
}

static void detectIndexInit(void)
{
	detect.data.index.idx = NULL;
	detect.data.index.attr.current_num = 0;
	detect.data.index.attr.max_capacity = 0;
	detect.data.index.attr.grow_step = GROW_STEP;
}

static void detectListInit(void)
{
	detect.data.pllist = llistCreate(detect.tsize.detect_ip_t_size);
	detect.data.cllist = llistCreate(detect.tsize.detect_cfg_t_size);
	detect.data.ellist = llistCreate(detect.tsize.detect_event_t_size);
}

static void globalVariableInit(void)
{
	xmemzero(&detect, sizeof(detect));

	typeSizeInit();
	detectListInit();
	detectIndexInit();
	detectOptsInit();
	detectRunstateInit();
}

static void globalConfigInit(void)
{
	strncpy(detect.config.domain.dnsIpFile,
			CONFIG_ORIGIN_DOMAIN_IP,
			BUF_SIZE_256);
	strncpy(detect.config.domain.domainFile,
			CONFIG_DOMAIN_FC,
			BUF_SIZE_256);
}

static void globalInit(void)
{
	globalVariableInit();
	globalConfigInit();
	getLocalAddress();
}

static void globalFree(void)
{
	//FIXME: here free
	//When a program exit, memory will be released automatically
	//But we want to release by ourself
}

static void globalDestruct(void)
{
	globalFree();
	writeDetectIdentify(STATE_TERMINATE);
	fclose(detect.other.logfp);
}

static void globalDelayStart(void)
{
	int sleeptime;

	if (detect.opts.F.dlstart && (0 == detect.opts.check_flag))
	{
		srand((unsigned int)time(NULL));
		sleeptime = rand() % DELAY_TIME_RAND + DELAY_TIME_BASE;
		fprintf(stdout, "detectorig delay start: sleep %ds\n", sleeptime);
		sleep(sleeptime);
	}
}

static void openLastResultFile(void)
{
	char *ptr;
	int ret, i;
	glob_t glob_res;
	time_t max = -1;
	time_t timestamp;
	char path[BUF_SIZE_1024];
	char pattern[BUF_SIZE_1024];

	detect.other.lastfp = NULL;
	xmemzero(pattern, BUF_SIZE_1024);
	snprintf(pattern, BUF_SIZE_1024, "%s/link.data.*", LINKDATA_DIR);
	ret = glob(pattern, GLOB_NOSORT, NULL, &glob_res);
	if (ret != 0)
	{    
		dlog(WARNING, NOFD, "glob() failed, PATTERN=[%s]", pattern);
		return;
	}    
	for (i = 0; i < glob_res.gl_pathc; ++i) 
	{    
		ptr = strrchr(glob_res.gl_pathv[i], '.');
		if (NULL == ptr) 
			continue;
		timestamp = atol(++ptr);
		if (timestamp > max) 
		{    
			max = timestamp;
			xmemzero(path, BUF_SIZE_1024);
			strcpy(path, glob_res.gl_pathv[i]);
		}    
	}    
	globfree(&glob_res);
	if (max != -1)
		detect.other.lastfp = fopen(path, "r");
}

static void globalStart(int argc, char **argv)
{
	globalInit();
	openLogFile();
	parseCmdLineParams(argc, argv);
	if (!detect.opts.check_flag)
	{
		debug(BEFORE_GLOBAL);
		writePidFile();
		writeDetectTimes();
		openLastResultFile();
		globalDelayStart();
		debug(AFTER_GLOBAL);
	}
}

int globalCall(const int calltype, int argc, char **argv)
{
	switch (calltype)
	{
		case START_GLOBAL:
			globalStart(argc, argv);
			return STATE_CONFIG;
		case START_DESTROY:
			globalDestruct();
			return STATE_TERMINATE; 
		default:
			dlog(WARNING, NOFD, "no this call type, TYPE=[%d]", calltype);
			return FAILED;
	}

	return FAILED;
}


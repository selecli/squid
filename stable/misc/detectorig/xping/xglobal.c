#include "xping.h"

static const char help_info[] =   
"Usage: xping [options] [arguments]\n"
"\t-c, --central       xping once when more than one same ip\n"
"\t-d, --debug         set debug information level\n"
"\t-e, --epollwait     epollwait timeout\n"
"\t-n, --travel        travel number\n"
"\t-D, --discleanup    disable internal cleanup\n"
"\t-f  conffile        xping configuration file\n"
"\t                    default: /usr/local/squid/etc/domain.conf\n"
"\t-h, --help          print the help message\n"
"\t-k killtime         how long to kill current program instance\n"
"\t--kill=killtime     the same to -k\n"
"\t-p                  print debug information\n"
"\t-i times            interval times to delete file link.data.xxx\n"
"\t--interval=times    the same to -i\n"
"\t-s                  disable delay start\n"
"\t-F, --fc            read and xping FC's domain.conf\n"
"\t-N, --nofc          don't read and xping FC's domain.conf\n"
"\t-T, --tta           read and xping TTA's tta_domain.conf\n"
"\t-S savetime         link.data files retain how long\n"
"\t--savetime=savetime the same to -S\n"
"\t-t timeout          timeout for once xping, default: 3.000000s\n"
"\t--timeout=timeout   the same to -t\n"
"\t-v, --version       print version, subversion\n"
;

void printVersion(void)
{
	printf("%s, BuildTime: %s %s\nCopyright (C) 2012 ChinaCache\n",
			XPING_VERSION, __DATE__, __TIME__);
}

static void parseCmdLineParam(int argc, char **argv)
{
	int opt, option_index;
	struct option long_options[] = {
		{"fc", 		   0, NULL, 'F'},
		{"tta", 	   0, NULL, 'T'},
		{"nofc",  	   0, NULL, 'N'},
		{"help",       0, NULL, 'h'},
		{"kill",       1, NULL, 'k'},
		{"debug",      1, NULL, 'd'},
		{"timeout",    1, NULL, 't'},
		{"version",    0, NULL, 'v'},
		{"central",    0, NULL, 'c'},
		{"interval",   1, NULL, 'i'},
		{"savetime",   1, NULL, 'S'},
		{"epollwait",  1, NULL, 'e'},
		{"travel",     1, NULL, 'n'},
		{"discleanup", 0, NULL, 'D'},
		{0, 0, 0, 0}
	};

	xping.config.mode = MODE_FC;

	do {
		opt = getopt_long(argc, argv, "d:e:f:t:k:i:n:S:chpsvDNTF", long_options, &option_index);
		if (-1 == opt) {
			break;
		}
		switch (opt) {
			case 'p':
				xping.opts.print = 1;
				break;
			case 'c':
				xping.opts.central = 1;
				break;
			case 'D':
				xping.opts.cleanup = 0;
				break;
			case 's':
				xping.opts.dlstart = 0;
				break;
			case 'n':
				xping.opts.travel = atoi(optarg);
				break;
			case 'd':
				xping.opts.dblevel = atoi(optarg);
				break;
			case 't':
				xping.opts.timeout = atof(optarg);
				break;
			case 'i':
				xping.opts.interval = atol(optarg);
				break;
			case 'k':
				xping.opts.killtime = atol(optarg);
				break;
			case 'e':
				xping.opts.epollwait = atoi(optarg);
				break;
			case 'f':
				memset(xping.config.domainFile, 0, BUF_SIZE_256);
				snprintf(xping.config.domainFile, BUF_SIZE_256, "%s", optarg);
				break;
			case 'F':
				xping.config.mode |= MODE_FC;
				break;
			case 'N':
				xping.config.mode &= MODE_NOFC;
				break;
			case 'S':
				switch (optarg[strlen(optarg) - 1])
				{
					case 's':
					case 'S':
						xping.opts.savetime = atol(optarg);
						break;
					case 'm':
					case 'M':
						xping.opts.savetime = atol(optarg) * 60;
						break;
					case 'h':
					case 'H':
						xping.opts.savetime = atol(optarg) * 60 * 60;
						break;
					case 'd':
					case 'D':
						xping.opts.savetime = atol(optarg) * 24 * 60 * 60;
						break;
					default:
						//xping.opts.savetime = SAVETIME_MIN;
						break;
				}
				//if (xping.opts.savetime < SAVETIME_MIN)
				//	xping.opts.savetime = SAVETIME_MIN;
				break;
			case 'T':
				xping.config.mode |= MODE_TTA;
				memset(xping.config.ttaConfFile, 0, BUF_SIZE_256);
				snprintf(xping.config.ttaConfFile, BUF_SIZE_256, CONFIG_DOMAIN_TTA);
				break;
			case 'h':
				printf("%s", help_info);
				//writeDetectIdentify(STATE_TERMINATE);
				exit(0);
			case 'v':
				printVersion();
				//writeDetectIdentify(STATE_TERMINATE);
				exit(0);
			case ':':
				fprintf(stderr, "option needs a value, OPTION=[%c]\n", optopt);
				xlog(WARNING,
						"parse command params error, option lack value, OPTION=[%c]\n", optopt);
				break;
			case '?':
				fprintf(stderr, "unkown command option, OPTION=[%c]", opt);
				xlog(WARNING,
						"parse command params error, unkown command option, OPTION=[%c]", opt);
				break;
		}
	} while (1);

	if (0 == xping.config.mode)
		xping.config.mode = MODE_FC;   //default: FC domain.conf
}

static void setResourceLimit(void)
{
	struct rlimit rlmt;

	getrlimit(RLIMIT_NOFILE, &rlmt); 
	if (rlmt.rlim_max < XPING_FD_MAX)
	{
		rlmt.rlim_max = XPING_FD_MAX; 
		rlmt.rlim_cur = rlmt.rlim_max;
	}
	if (rlmt.rlim_cur < rlmt.rlim_max)
		rlmt.rlim_cur = rlmt.rlim_max;
	if (setrlimit(RLIMIT_NOFILE, &rlmt) < 0)
		xlog(ERROR, "setrlimit() failed, ERROR=[%s]", strerror(errno));
	else
		xlog(COMMON, "setrlimit() succeed, rlim_cur=[%ld], rlim_max=[%ld]", rlmt.rlim_cur, rlmt.rlim_max);
}

static void writePidFile(void)
{
	time_t tm; 
	int fd, val;
	struct stat st; 
	struct flock lock;
	char buf[BUF_SIZE_256];

	if (stat(XPING_PID_DIR, &st) < 0)
	{   
		if (mkdir(XPING_PID_DIR, 0755) < 0)
		{   
			fprintf(stderr,
					"mkdir() failed, DIR=[%s], ERROR=[%s]",
					XPING_PID_DIR, strerror(errno));
			xlog(WARNING, "mkdir(%s) failed, ERROR=[%s]", XPING_PID_DIR, strerror(errno));
			xabort("abort now");
		}   
	}
	fd = open(XPING_PID_FILE, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
	if(fd < 0)
	{
		xlog(WARNING, "open(pidfile) failed, ERROR=[%s]", strerror(errno));
		xabort("abort now");
	}
	//set file write lock
	lock.l_len = 0;
	lock.l_start = 0;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLK, &lock) < 0)
	{
		fprintf(stderr, "Already have one xping instance running!\n");
		xlog(WARNING, "already have one instance running, ERROR=[%s]", strerror(errno));
		exit(1);
	}   
	if(ftruncate(fd, 0) < 0)
	{
		xlog(WARNING, "truncate(pidfile) failed, ERROR=[%s]", strerror(errno));
		xabort("abort now");
	}   
	tm = time(NULL);
	memset(buf, 0, BUF_SIZE_256);
	sprintf(buf, "%d\n%ld\n", getpid(), tm);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
	{
		xlog(WARNING, "write(pidfile) failed, ERROR=[%s]", strerror(errno));
		xabort("abort now");
	}
	val = fcntl(fd, F_GETFD, 0);
	if(val < 0) 
	{
		xlog(WARNING, "fcntl(F_GETFD) failed, ERROR=[%s]", strerror(errno));
		xabort("abort now");
	}
	//set fd flag: close-on-exec
	val |= FD_CLOEXEC;
	if(fcntl(fd, F_SETFD, val) < 0)
	{
		xlog(WARNING, "fcntl(F_SETFD) failed, ERROR=[%s]", strerror(errno));
		xabort("abort now");
	}
}

static void typeSizeInit(void)
{
	xping.tsize.ping_packet_t_size = sizeof(ping_packet_t);
	xping.tsize.hash_factor_t_size = sizeof(hash_factor_t);
}

static void xpingOptsInit(void)
{
	xping.opts.savetime = SAVETIME_DEFAULT;
	/*
	   xping.opts.print = 0;
	   xping.opts.dblevel = 0;
	   xping.opts.timeout = 0;
	   xping.opts.central = 0;
	   xping.opts.cleanup = 1;
	   xping.opts.dlstart = 1;
	   xping.opts.travel = TRAVEL_NUMBER;
	   xping.opts.epollwait = EPOLL_TIMEOUT;
	   xping.opts.interval = LINKDATA_CLEAN_TIME;
	   xping.opts.killtime = DETECT_KILLTIME;
	 */
}

/*
   static void xpingRunstateInit(void)
   {
   xping.runstate.count = 0;
   xping.runstate.pid = getpid();
   time(&xping.runstate.starttime);
   }

   static void xpingIndexInit(void)
   {
   xping.data.index.idx = NULL;
   xping.data.index.attr.current_num = 0;
   xping.data.index.attr.max_capacity = 0;
   xping.data.index.attr.grow_step = GROW_STEP;
   }
 */

static void xpingListInit(void)
{
	xping.list = llistCreate(xping.tsize.ping_packet_t_size);
}

static void globalVariableInit(void)
{
	memset(&xping, 0, sizeof(xping));

	typeSizeInit();
	//xpingIndexInit();
	xpingOptsInit();
	//xpingRunstateInit();
}

static void globalConfigInit(void)
{
	strncpy(xping.config.dnsIpFile, CONFIG_ORIGIN_DOMAIN_IP, BUF_SIZE_256);
	strncpy(xping.config.domainFile, CONFIG_DOMAIN_FC, BUF_SIZE_256);
}

static void globalInit(void)
{
	globalVariableInit();
	globalConfigInit();
	getLocalAddress();
	xpingListInit();
}

static void globalFree(void)
{
	//FIXME: here free
}

void globalDestroy(void)
{
	globalFree();
	//writeDetectIdentify(STATE_TERMINATE);
	fclose(xping.logfp);
}

/*
   static void globalDelayStart(void)
   {
   int sleeptime;

   if (xping.opts.dlstart)
   {
   srand((unsigned int)time(NULL));
   sleeptime = rand() % DELAY_TIME_RAND + DELAY_TIME_BASE;
   fprintf(stdout, "xping delay start: sleep %ds\n", sleeptime);
   sleep(sleeptime);
   }
   }
 */

void globalStart(int argc, char **argv)
{
	globalInit();
	openLogFile();
	parseCmdLineParam(argc, argv);
	writePidFile();
	setResourceLimit();
	//globalDelayStart();
}


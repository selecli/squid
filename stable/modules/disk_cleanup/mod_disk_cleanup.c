//#ifdef CC_FRAMEWORK
#include "cc_framework_api.h"
#include "squid.h"
#include "../src/fs/aufs/async_io.h"
#include "../src/fs/aufs/store_asyncufs.h"

#define XAVG(X) (dt ? (f->X > l->X ? ((double) (f->X - l->X) / dt) : 0.0) : 0.0)

extern StatCounters CountHist[N_COUNT_HIST];
static int count = 0;

static double get_avg_conn_stat(int minutes)
{
	StatCounters *f;
	StatCounters *l;
	double dt;
	double ct;

	assert(N_COUNT_HIST > 1);
	assert(minutes > 0 );
	f = &CountHist[0];
	l = f;

	if (minutes > N_COUNT_HIST - 1)
		minutes = N_COUNT_HIST - 1;
	l = &CountHist[minutes];

	dt = tvSubDsec(l->timestamp, f->timestamp);
	ct = f->cputime - l->cputime;

	return XAVG(client_http.requests);
}


static double getloadavg_15s()
{
	double avg[3];
	int loads;

	loads = getloadavg(avg, 3);

	if(loads > 2)
		return avg[2];
	else if(loads > 1)
		return avg[1];
	else if(loads > 0)
		return avg[0];
	else
		return -1;
}

static int storeAufsDirValidFileno(SwapDir * SD, sfileno filn, int flag)
{
	squidaioinfo_t *aioinfo = (squidaioinfo_t *) SD->fsdata;
	if (filn < 0)
		return 0;
	/*
	 * If flag is set it means out-of-range file number should
	 * be considered invalid.
	 */
	if (flag)
		if (filn > aioinfo->map->max_n_files)
			return 0;
	return 1;
}

static int storeAufsDirMapBitTest(SwapDir * SD, sfileno filn)
{
	squidaioinfo_t *aioinfo;
	aioinfo = (squidaioinfo_t *) SD->fsdata;
	return file_map_bit_test(aioinfo->map, filn);
}

/*
 * Does swapfile number 'fn' belong in cachedir #F0,
 * level1 dir #F1, level2 dir #F2?
 */
static int storeAufsFilenoBelongsHere(int fn, int F0, int F1, int F2)
{
	int D1, D2;
	int L1, L2;
	int filn = fn;
	squidaioinfo_t *aioinfo;
	assert(F0 < Config.cacheSwap.n_configured);
	aioinfo = (squidaioinfo_t *) Config.cacheSwap.swapDirs[F0].fsdata;
	L1 = aioinfo->l1;
	L2 = aioinfo->l2;
	D1 = ((filn / L2) / L2) % L1;
	if (F1 != D1)
		return 0;
	D2 = (filn / L2) % L2;
	if (F2 != D2)
		return 0;
	return 1;
}

static int rev_int_sort(const void *A, const void *B)
{
	const int *i1 = A;
	const int *i2 = B;
	return *i2 - *i1;
}

static void storeAufsDirClean()
{
	debug(187, 1) ("mod_disk_cleanup: Begining store dir clean !!!!!\n");
	count = 0;
	static int l1 = 0, l2 = 0;
	static int swap_index = 0;
	int N1, N2;
	while (swap_index < Config.cacheSwap.n_configured) {
		SwapDir *SD = &Config.cacheSwap.swapDirs[swap_index];
		if(strcmp(SD->type, "aufs")) {
			debug(187,2)("mod_disk_cleanup: thia cache_dir not aufs, skip it\n");
			swap_index++;
			continue;
		}

		DIR *dp = NULL;
		struct dirent *de = NULL;
		LOCAL_ARRAY(char, p1, MAXPATHLEN + 1);
		LOCAL_ARRAY(char, p2, MAXPATHLEN + 1);
#if USE_TRUNCATE
		struct stat sb;
#endif
		int files[1024];
		int swapfileno;
		int fn;                     /* same as swapfileno, but with dirn bits set */
		int n = 0;
		int k = 0;


		squidaioinfo_t *aioinfo = (squidaioinfo_t *) SD->fsdata;
		N1 = aioinfo->l1;
		N2 = aioinfo->l2;

		while(l1 < N1) {
			while(l2 < N2) {
				snprintf(p1, SQUID_MAXPATHLEN, "%s/%02X/%02X", Config.cacheSwap.swapDirs[swap_index].path, l1, l2);
				debug(187, 3) ("mod_disk_cleanup: storeDirClean: Cleaning directory %s\n", p1);

				dp = opendir(p1);
				if (dp == NULL) {
				/*
					if (errno == ENOENT) {
						debug(187, 0) ("mod_disk_cleanup: storeDirClean: WARNING: Creating %s\n", p1);
							if (mkdir(p1, 0777) == 0)
								continue ;
					}
					debug(187, 0) ("mod_disk_cleanup: storeDirClean: %s: %s\n", p1, xstrerror());
					safeunlink(p1, 1);
				*/
					debug(187,2)("mod_disk_cleanup: opendir [%s] failed %s\n ",p1, xstrerror());
					goto next;
				}

				k = 0;
				while ((de = readdir(dp)) != NULL && k < 20) {
					if (sscanf(de->d_name, "%X", &swapfileno) != 1)
						continue;
					fn = swapfileno;        /* XXX should remove this cruft ! */
					if (storeAufsDirValidFileno(SD, fn, 1))
						if (storeAufsDirMapBitTest(SD, fn))
							if (storeAufsFilenoBelongsHere(fn, swap_index, l1, l2))
								continue;
#if USE_TRUNCATE
					if (!stat(de->d_name, &sb))
						if (sb.st_size == 0)
							continue;
#endif
					files[k++] = swapfileno;
				}
				closedir(dp);

				//no file need process in current dir? out! then next dir loop!
				if (k > 0) {
					qsort(files, k, sizeof(int), rev_int_sort);
					for (n = 0; n < k; n++) {
						debug(187, 3) ("mod_disk_cleanup: storeDirClean: Cleaning file %08X\n", files[n]);
						snprintf(p2, MAXPATHLEN + 1, "%s/%08X", p1, files[n]);
#if USE_TRUNCATE
						truncate(p2, 0);
#else
						safeunlink(p2, 0);
#endif
						statCounter.swap.files_cleaned++;
						count++;
					}
				}
				debug(187, 3)("mod_disk_cleanup: Cleaned %d unused files from %s\n", k, p1);
				goto next;
			}
		}
	}

next:
	// update value of l1, l2, swap_index
	debug(187, 3)("mod_disk_cleanup: finish one loop\n");
	if(++l2 == N2) {
		l2 = 0;
		if(++l1 == N1) {
			l1 = 0;
			if(++swap_index == Config.cacheSwap.n_configured) {
				debug(187, 0)("mod_disk_cleanup: storeDirClean has checked all the cache_dir....\n"); 
				swap_index = 0;
			}
		}
	}
}

static void disk_cleanup()
{
	time_t cur_time = squid_curtime;
	struct tm* loc_time;
	loc_time = localtime(&cur_time);
	debug(187, 3)("mod_disk_cleanup: current hour is: %d \n", loc_time->tm_hour);
	if((loc_time->tm_hour < 2) || (loc_time->tm_hour >= 9)) {
		debug(187, 3)("mod_disk_cleanup: current time is not match the clean condition\n");
		eventAdd("disk_cleanup", disk_cleanup, NULL, 600, 1);
		return;
	}

	double loadavg;
	loadavg = getloadavg_15s();
	debug(187, 3)("mod_disk_cleanup: load average is: %.2f \n", loadavg);
	if(loadavg > 3) {
		debug(187, 3)("mod_disk_cleanup: current loadavg is: %lf, return.\n", loadavg);
		eventAdd("disk_cleanup", disk_cleanup, NULL, 60, 1);
		return;
	}

	int conn_count;
	conn_count = get_avg_conn_stat(5);
	debug(187, 3)("mod_disk_cleanup: current socket connect count is: %d\n", conn_count);
	if(conn_count > 100) {
		debug(187, 3)("mod_disk_cleanup: current socket connect count is: %d, return.\n", conn_count);
		eventAdd("disk_cleanup", disk_cleanup, NULL, 60, 1);
		return;
	}

	if (store_dirs_rebuilding) {
		debug(187, 3) ("mod_disk_cleanup: store dir is rebuilding now, return.\n");
		eventAdd("disk_cleanup", disk_cleanup, NULL, 60, 1);
		return;
	}


	/********** store clean action **********/
	storeAufsDirClean();
	/********** store clean action **********/

	if (count == 0)
		eventAdd("disk_cleanup", disk_cleanup, NULL, 6, 0);
	else
		eventAdd("disk_cleanup", disk_cleanup, NULL, 5, 0);

	return;
}

static int hook_init(cc_module *module)
{
	debug(187,0)("(mod_disk_cleanup) --> cc_mod_disk_cleanup hook_init\n");
	eventAdd("disk_cleanup", disk_cleanup, NULL, 180, 0);
	return 1;
}


int mod_register(cc_module *module)
{
	debug(187,3)("(mod_disk_cleanup) --> cc_mod_disk_cleanup_init: init module\n");
	strcpy(module->version, "7.0.R.16488.i686");

	cc_register_hook_handler(HPIDX_hook_func_sys_init,
			module->slot,
			(void **)ADDR_OF_HOOK_FUNC(module, hook_func_sys_init),
			hook_init);

	return 1;
}

//#endif

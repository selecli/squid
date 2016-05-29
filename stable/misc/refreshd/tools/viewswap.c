#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "list.h"

/* defines */
#define ERROR strerror(errno)
#define debug(fmt, ...) printf("[%s.%d %s] "fmt, \
				__FILE__, __LINE__, __func__, __VA_ARGS__)
#define MD5_DIGEST_CHARS 16
#define VERSION "viewswap 1.0"
/* structures and typedefs, enums etc. */
typedef unsigned short u_short;
#if defined(LARGE_CACHE_FILE)
typedef int64_t squid_file_sz;
#define SIZE_FORMAT "%lld"
#else
typedef long int squid_file_sz;
#define SIZE_FORMAT "%ld"
#endif
#define MAX_BUFFER 4096
typedef signed int sfileno;

typedef struct {
	char op; 
	int version;
	int record_size;
}swap_log_header_t;

typedef struct {
	int l1;
	int l2;
	FILE *state;
	char *path;
	struct list_head list;
}swap_dir_t;

typedef struct {
	struct list_head swaps;
	int url_include;
	char *conf;
}setting_t;

typedef struct _storeSwapLogData {
	char op;
	sfileno swap_filen;
	time_t timestamp;
	time_t lastref;
	time_t expires;
	time_t lastmod;
	squid_file_sz swap_file_sz;
	u_short refcount;
	u_short flags;
	unsigned char key[MD5_DIGEST_CHARS];
}store_swap_log_data_t;

typedef enum {
	SWAP_LOG_NOP,
	SWAP_LOG_ADD,
	SWAP_LOG_DEL,
	SWAP_LOG_VERSION,
	SWAP_LOG_MAX
} swap_log_op;

/* functions */
static char *store_full_path(sfileno fileno, const char *dir, int l1, int l2);
static char *time_to_string(const time_t *t);
static char *swap_log_op_to_string(int op);
static void swap_parse(FILE *swap, int l1, int l2, const char *path);
static void options_parse(int argc, char **argv);
static void settings_default(setting_t *set);
static void usage();
static char *storeKeyText(const unsigned char *key) ;
static void conf_parse(const char *path);

/* global data */
setting_t settings;

static void conf_parse(const char *path)
{
	if(!path)
		usage();

	char buf[MAX_BUFFER];
	int res;
	FILE *fconf;

	if((fconf = fopen(path, "r")) == NULL) {
		debug("error %s when open %s\n", ERROR, path);
		usage();
	}

	while((fgets(buf, MAX_BUFFER - 1, fconf))) {
		swap_dir_t *dir = calloc(1, sizeof(*dir));
		char *token = strtok(buf, "\r\n\t ");
		char state_path[MAX_BUFFER];

		INIT_LIST_HEAD(&dir->list);
		/* get type */
		token = strtok(NULL, " \r\n\t");
		assert(token);
		/* get path */
		token = strtok(NULL, " \r\n\t");
		assert(token);
		res = snprintf(state_path, MAX_BUFFER - 1, "%s/swap.state", token);	
		if(res > 0) {
			debug("open %s\n", state_path);
			dir->state = fopen(state_path, "r");
			if(dir->state == NULL) {
				debug("error %s when open %s\n", ERROR, state_path);
			}
			dir->path = strdup(token);
		}
		/* get size */
		token = strtok(NULL, " \r\n\t");
		assert(token);
		/* get l1 */
		token = strtok(NULL, " \r\n\t");
		assert(token);
		dir->l1 = atoi(token);
		/* get l2 */
		token = strtok(NULL, " \r\n\t");
		assert(token);
		dir->l2 = atoi(token);

		list_add_tail(&dir->list, &settings.swaps);
	}
}

static void swap_parse(FILE *fswap, int l1, int l2, const char *path)
{
	assert(fswap);

	store_swap_log_data_t store;
	swap_log_header_t head;
	size_t ss ;
	int64_t scaned = 0LL;
	int res = -1;

	ss = sizeof(head);
	if((res = fread(&head, ss, 1, fswap)) == -1) {
		debug("error %s\n", ERROR);
		return;
	}	
	printf("Header Infomation:\nop=%s, version=%d, record_size=%d\n", \
					swap_log_op_to_string(head.op), head.version, head.record_size);
	if( -1 == fseek(fswap, head.record_size - sizeof(head), SEEK_CUR)) {
		debug("error %s\n", ERROR);
		return;
	}
	ss = sizeof(store);
	debug("store_swap_log_data_t %d bytes\n", ss);

	for(; ;) {
		int url_len = 0;
		char url[MAX_BUFFER];

		if((res = fread(&store, ss, 1, fswap)) == -1) {
			debug("error %s\n", ERROR);
			break;
		}
		if(res != 1) {
			debug("not read %d bytes, res %d\n", ss, res);
			break;
		}
		
		if(store.op != SWAP_LOG_ADD && store.op != SWAP_LOG_DEL)
			continue;
		
		/*
		printf("\n=======\n" \
				"op = %s\n" \
				"swap_filen = %u\n" \
				"swap_file_sz = "SIZE_FORMAT"\n" \
				"refcount = %u\n" \
				"flags = %u\n" \
				"key = %s\n" \
				"full_path = %s\n", 
				swap_log_op_to_string(store.op),
				store.swap_filen, 
				store.swap_file_sz,
				store.refcount,
				store.flags,
				storeKeyText(store.key),
				store_full_path(store.swap_filen, path, l1, l2));
		printf("timestamp = %s(%u)\n",
				time_to_string(&store.timestamp),  store.timestamp);
		printf("lastref = %s(%u)\n",
				time_to_string(&store.lastref), store.lastref);
		printf("expires = %s(%u)\n",
				time_to_string(&store.expires), store.expires);
		printf("lastmod = %s(%u)\n",
				time_to_string(&store.lastmod), store.lastmod);
			*/

		printf("full_path = %s\n",store_full_path(store.swap_filen, path, l1, l2));

		if(settings.url_include && store.op == SWAP_LOG_ADD) {
			if((res = fread(&url_len, sizeof(int), 1, fswap)) == -1) {
				debug("error %s\n", ERROR);
				break;
			}
			if(1 != res) {
				debug("error not read %d bytes\n", sizeof(int));
				break;
			}
	//		printf("url length = %d\n", url_len);
			/* read url */
			assert(url_len < 4096);
			if((res = fread(url, url_len, 1, fswap)) == -1) {
				debug("error %s\n", ERROR);
				break;
			}
			if(1 != res) {
				debug("error not read %d bytes\n", url_len);
				break;
			}
			url[url_len] = '\0';
			printf("url = %s\n", url);
		}
//		printf("======\n");
		scaned++;
	}
	fclose(fswap);
	printf("Summary:\n\tscaned %lld\n", scaned);
}

static void settings_default(setting_t *set)
{
	if(!set)
		return;

	INIT_LIST_HEAD(&set->swaps);
	set->url_include = 1;
	set->conf = NULL;
}

static void usage()
{
	printf("Usage: viewswap [options]\n" \
					"Options:\n" \
					"\t-f conf              Specify program configure.\n"\
					"\t                     You only need paste the configure of cache_dir\n"\
					"\t                     in the squid.conf to here. Now support aufs only.\n"\
					"\t                     Example:\n"\
					"\t                     \tcache_dir aufs /data/cache2 360000 128 128\n"\
					"\t-v                   Print version\n" \
					"\t-h                   Print help information\n"
					"\t-n                   Not include url. Url is included by default\n");

	exit(0);
}

static void options_parse(int argc, char **argv)
{
	int c;
	
	while((c = getopt(argc, argv, "f:hvn?")) != -1) {
		switch(c) {
			case 'f':
				if(optarg) {
					settings.conf = strdup(optarg);
				}
				break;
			case 'h' :
				usage();
				break;
			case 'v':
				printf("Version: %s\n", VERSION);
				break;
			case 'n':
				settings.url_include = 0;
				break;
			case '?':
				printf("unknown %c\n", c);
				break;
			default:
				break;
		}
	}
}

int main(int argc, char **argv) 
{
	struct list_head *p, *n;

	settings_default(&settings);
	options_parse(argc, argv);
	conf_parse(settings.conf);
	
	list_for_each_safe(p, n, &settings.swaps) {
		swap_dir_t *entry = list_entry(p, swap_dir_t, list);
		swap_parse(entry->state, entry->l1, entry->l2, entry->path);
	}

	return 0;
}

static char *swap_log_op_to_string(int op)
{
	switch(op) {
		case SWAP_LOG_ADD:
			return "SWAP_LOG_ADD";
		case SWAP_LOG_DEL:
			return "SWAP_LOG_DEL";
		case SWAP_LOG_NOP:
			return "SWAP_LOG_NOP";
		case SWAP_LOG_VERSION:
			return "SWAP_LOG_VERSION";
		case SWAP_LOG_MAX:
			return "SWAP_LOG_MAX";
		default:
			return "SWAP_LOG_UNKNOWN";
	}
}

static char *storeKeyText(const unsigned char *key) 
{                       
	static char buf[32];
	memset(buf, 0, 32);
	int i;              
	for(i = 0; i < 16; i++) {
		snprintf(buf + 2*i, 32, "%02X", *(key + i));
	}                   
	buf[32] = '\0';
	return buf;
}

static char *time_to_string(const time_t *time)
{
	static char *tmp;
	const char *spec = "%a, %d %b %G %H:%M:%S GMT";
	struct tm *t = NULL;
	int res;
	
	if(!tmp) 
		tmp = calloc(1, MAX_BUFFER);
	memset(tmp, 0, MAX_BUFFER);
	t = gmtime(time);
	res = strftime(tmp, MAX_BUFFER, spec, t);
	tmp[res] = '\0';
	//debug("%u strftime=%s\n", *time, tmp);

	return tmp;
}

static char *store_full_path(sfileno fileno, const char *dir, int L1, int L2)
{
	static char *dirpath;
	
	if(!dirpath)
		dirpath = calloc(1, MAX_BUFFER);
	memset(dirpath, 0, sizeof(dirpath));
	snprintf(dirpath, MAX_BUFFER- 1, "%s/%02X/%02X/%08X", \
						dir, 
						((fileno / L2) / L2) % L1, 
						(fileno / L2) % L2,
						fileno);
	return dirpath;
}

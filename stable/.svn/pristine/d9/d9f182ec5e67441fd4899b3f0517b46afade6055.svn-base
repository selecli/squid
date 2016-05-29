#include "server_header.h"


#define HttpHeaderInitPos (-1)
typedef int HttpHeaderPos;
#define assert_eid(id) assert((id) < HDR_ENUM_END)

rms_t RequestMethods[] =
{
        {(char *) "NONE", 4},
        {(char *) "GET", 3},
        {(char *) "POST", 4},
        {(char *) "PUT", 3},
        {(char *) "HEAD", 4},
        {(char *) "CONNECT", 7},
        {(char *) "TRACE", 5},
        {(char *) "PURGE", 5},
        {(char *) "OPTIONS", 7},
        {(char *) "DELETE", 6},
        {(char *) "PROPFIND", 8},
        {(char *) "PROPPATCH", 9},
        {(char *) "MKCOL", 5},
        {(char *) "COPY", 4},
        {(char *) "MOVE", 4},
        {(char *) "LOCK", 4},
        {(char *) "UNLOCK", 6},
        {(char *) "BMOVE", 5},
        {(char *) "BDELETE", 7},
        {(char *) "BPROPFIND", 9},
        {(char *) "BPROPPATCH", 10},
        {(char *) "BCOPY", 5},
        {(char *) "SEARCH", 6},
        {(char *) "SUBSCRIBE", 9},
        {(char *) "UNSUBSCRIBE", 11},
        {(char *) "POLL", 4},
	{(char *) "REPORT", 6},
        {(char *) "MKACTIVITY", 10},
        {(char *) "CHECKOUT", 8},
        {(char *) "MERGE", 5},
        {(char *) "%EXT00", 6},
        {(char *) "%EXT01", 6},
        {(char *) "%EXT02", 6},
        {(char *) "%EXT03", 6},
        {(char *) "%EXT04", 6},
        {(char *) "%EXT05", 6},
        {(char *) "%EXT06", 6},
        {(char *) "%EXT07", 6},
        {(char *) "%EXT08", 6},
        {(char *) "%EXT09", 6},
        {(char *) "%EXT10", 6},
        {(char *) "%EXT11", 6},
        {(char *) "%EXT12", 6},
        {(char *) "%EXT13", 6},
        {(char *) "%EXT14", 6},
        {(char *) "%EXT15", 6},
        {(char *) "%EXT16", 6},
        {(char *) "%EXT17", 6},
        {(char *) "%EXT18", 6},
        {(char *) "%EXT19", 6},
        {(char *) "ERROR", 5},
};

/*
struct _HttpHeaderEntry {
    http_hdr_type id;
    int active;
    String name;
    String value;
};
*/
/* per field statistics */
struct _HttpHeaderFieldStat {
    int aliveCount;             /* created but not destroyed (count) */
    int seenCount;              /* #fields we've seen */
    int parsCount;              /* #parsing attempts */
    int errCount;               /* #pasring errors */
    int repCount;               /* #repetitons */
};
typedef struct _HttpHeaderFieldStat HttpHeaderFieldStat;

/* compiled version of HttpHeaderFieldAttrs plus stats */
/*
struct _HttpHeaderFieldInfo {
    http_hdr_type id;
    String name;
    field_type type;
    HttpHeaderFieldStat stat;
};
typedef struct _HttpHeaderFieldInfo HttpHeaderFieldInfo;
*/


/* possible types for http header fields */
/*
typedef enum {
    ftInvalid = HDR_ENUM_END,   /// to catch nasty errors with hdr_id<->fld_type clashes 
    ftInt,
    ftStr,
    ftDate_1123,
    ftETag,
    ftPCc,
    ftPContRange,
    ftPRange,
    ftDate_1123_or_ETag,
    ftSize
} field_type;
*/

/* constant attributes of http header fields */
/*
struct _HttpHeaderFieldAttrs {
    const char *name;
    http_hdr_type id;
    field_type type;
};
*/
//typedef struct _HttpHeaderFieldAttrs HttpHeaderFieldAttrs;

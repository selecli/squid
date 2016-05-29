#include "server_header.h"

const String StringNull = { 0, 0, NULL };

void
stringLimitInit(String * s, const char *str, int len);

static void
stringInitBuf(String * s, size_t sz)
{
	s->buf = cc_malloc(sz);
	assert(sz < 65536);
	s->size = sz;
}

void
stringInit(String * s, const char *str)
{
	assert(s);
	if (str)
		stringLimitInit(s, str, strlen(str));
	else
		*s = StringNull;
}

void
stringLimitInit(String * s, const char *str, int len)
{
	assert(s && str);
	stringInitBuf(s, len + 1);
	s->len = len;
	xmemcpy(s->buf, str, len);
	s->buf[len] = '\0';
}

String
stringDup(const String * s)
{
	String dup;
	assert(s);
	stringInit(&dup, s->buf);
	return dup;
}

void
stringClean(String * s)
{
	assert(s);
	if (s->buf)
		cc_free(s->buf);
	*s = StringNull;
}

void
stringReset(String * s, const char *str)
{
	stringClean(s);
	stringInit(s, str);
}

void
stringAppend(String * s, const char *str, int len)
{
	assert(s);
	assert(str && len >= 0);
	if (s->len + len < s->size)
	{
		strncat(s->buf, str, len);
		s->len += len;
	}
	else
	{
		String snew = StringNull;
		snew.len = s->len + len;
		stringInitBuf(&snew, snew.len + 1);
		if (s->buf)
			xmemcpy(snew.buf, s->buf, s->len);
		if (len)
			xmemcpy(snew.buf + s->len, str, len);
		snew.buf[snew.len] = '\0';
		stringClean(s);
		*s = snew;
	}
}

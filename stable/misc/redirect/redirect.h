#ifndef REDIRECT_H
#define REDIRECT_H

int dealwithRequest(const char* line);
extern int checkDomainAndFromtag(char *pdomain, int fromtag);
extern int initDomainAndFromtagDb(char *pfilename);
extern void releaseDomainAndFromtagDb();

extern void rfc1738_unescape(char *s );
extern char * rfc1738_escape(const char* s );
extern char * rfc1738_escape_unescaped(const char* s );
extern char * rfc1738_escape_part(const char* s );


#endif	//REDIRECT_H

#ifndef COOKIE_MD5_H_
#define COOKIE_MD5_H_ 

typedef struct cookie_md5_cfg
{
	time_t 	interval;	/* 过期时间 */
	char	key[128];	/* cookie中认证串的关键字 */
	char	Referer[32][128];	/* referer */
    int     iReferCount;
}COOKIE_MD5_CFG_ST;


#endif

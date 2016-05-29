#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "detect_orig.h"
/*判断一个返回的代码是否在预定的代码组里
 *输入：
 *    oneCode----响应代码
 *    code----预设代码（每四个字节一组，长度1到3）
 *返回值：
 *    1----在
 *    0----不在
 */
int inCode(const char* oneCode, const char* code)
{
	while (0 != *code) {
		if (0 == strncmp(oneCode, code, strlen(code)))
			return 1;
		code += 4;
	}
	return 0;
}

/*检查一个ip是否合法（目前用inet_pton检测）
 *输入：
 *    ip----字符串表示的ip
 *返回值：
 *    0----合法
 *    -1----不合法
 */
int checkOneIp(const char* ip)
{
	unsigned int result;
	int ret = inet_pton(AF_INET, ip, &result);
	if (ret > 0)
		return 0;
	else
		return -1;
}


/*从配置文件ip字段得到各个ip，并分配内存
 *输入：
 *    str----配置文件ip字段
 *输出：
 *    ip----分配的ip结构
 *返回值：
 *    -1----没有ip
 *    -2----ip格式错误
 *    -3----内存错误
 *    整数----ip的个数
 */
int getIp(char* str, struct IpDetect** ip)
{
	//得到第一个ip
	char* ptr;
	str = strtok_r(str, "\t\n|:", &ptr);
	if (NULL == str)
		return -1;
	char tempIp[MAX_IP_ONE_LINE][16];
	memset(tempIp, 0, sizeof(tempIp));
	int ret;
//	if (0 != atoi(str))
//	{
		ret = checkOneIp(str);
		if (-1 == ret)
			return -2;
//	}
	strcpy(tempIp[0], str);
	//得到其他的ip
	int i = 1;
	for (i=1;i<MAX_IP_ONE_LINE;i++)
	{
		str = strtok_r(NULL, "\t\n|:", &ptr);
		if (NULL == str)
			break;
		ret = checkOneIp(str);
		if (-1 == ret)
			return -2;
		strcpy(tempIp[i], str);
	}
	int number = i;

	*ip = (struct IpDetect*)cc_malloc((number+1)*sizeof(struct IpDetect));
	if (NULL == *ip)
		return -3;
	memset(*ip, 0, (number+1)*sizeof(struct IpDetect));
	struct IpDetect* pstIp = *ip;
	for (i = 0; i < number; ++i) {
		strcpy(pstIp[i].ip, tempIp[i]);
	}
	return number;
}
/*得到正确的响应代码。结果放在一个新分配的字符串里，每四个字节放一个，最后以0结束
 *输入：
 *    str----配置文件里的code字段（这里按照没四个字节取三个，没有检查第四个字节）
 *输出：
 *    code----解析后的，新分配的内存
 *返回值：
 *    -1----失败
 *    0----成功
 */
int getCode(const char* str, char** code)
{
	int length = strlen(str);
	*code = (char*)cc_malloc(length+4);
	if (NULL == *code)
		return -1;
	char* tmpCode = *code;
	memset(tmpCode, 0, length+4);
	int i;
	//每四个字节取三个
	for (i=0;i<length;i+=4) {
		tmpCode[i] = str[i];  //第一个字节直接取
		//后俩个字节如果为*，表示任意匹配
		if ('*' == str[i+1])
			tmpCode[i+1] = 0;
		else {
			tmpCode[i+1] = str[i+1];
			if ('*' == str[i+2])
				tmpCode[i+2] = 0;
			else
				tmpCode[i+2] = str[i+2];
		}
	}

	return 0;
}

int make_new_ip(struct DetectOrigin* pstDetectOrigin)
{
	struct IpDetect* ip_all;
	int i = 0;

	pstDetectOrigin->backupIsIp = 1;
	if(pstDetectOrigin->nbkip == 1)
		strcpy(pstDetectOrigin->ip[pstDetectOrigin->ipNumber].ip,pstDetectOrigin->backup);
	else
	{
		ip_all = (struct IpDetect*)cc_malloc((pstDetectOrigin->nbkip + pstDetectOrigin->ipNumber)*sizeof(struct IpDetect));
		if (NULL == ip_all)
			return -1;

		struct IpDetect* pstIp = ip_all;
		for ( i=0;i<pstDetectOrigin->ipNumber;i++)
			strcpy(pstIp[i].ip,pstDetectOrigin->ip[i].ip );
		cc_free(pstDetectOrigin->ip);
		pstDetectOrigin->ip = ip_all;
		for ( i=0;i<pstDetectOrigin->nbkip;i++)
			strcpy(pstIp[i+pstDetectOrigin->ipNumber].ip,pstDetectOrigin->bkip[i].ip );

	}

	return 0;
}

time_t getCurrentTime(void)
{
#if GETTIMEOFDAY_NO_TZP
	gettimeofday(&current_time);
#else
	gettimeofday(&current_time, NULL);
#endif
	current_dtime = (double) current_time.tv_sec +(double) current_time.tv_usec / 1000000.0;
	return detect_curtime = current_time.tv_sec;
}

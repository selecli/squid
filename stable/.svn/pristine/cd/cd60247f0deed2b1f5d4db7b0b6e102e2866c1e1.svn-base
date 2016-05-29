/*用a-zA-Z0-9外加两个特别的字符来替换一个字符串的字符,达到好像加密的目的
 *方法是每三个字符转换为四个字符,其中每六个bit对应一个字符
*/

#include <string.h>

//另外两个额外的字符
#define g_cExtraChar1 '?'
#define g_cExtraChar2 '='

//总共64个替换的字符,注意顺序与函数getPositionFromChar相关,修改的时候要注意同步修改
static char g_stCode[64] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
			'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', g_cExtraChar1, g_cExtraChar2};

/*把三个原字符转换为四个替换的字符,不考虑字符串长度.每六个bit对应一个字符。两个参数不能有交叉
 *输入:
 *    three--待转换的三个字符
 *输出:
 *    four--存放结果的四个字符
*/
inline void threeToFour(const char* three, char* four)
{
	four[0] = g_stCode[(three[0]&0xff)>>2];
	four[1] = g_stCode[((three[0]&0x03)<<4) + ((three[1]&0xff)>>4)];
	four[2] = g_stCode[((three[1]&0x0f)<<2) + ((three[2]&0xff)>>6)];
	four[3] = g_stCode[three[2]&0x3f];
}

/*得到一个字符在g_stCode里的位置。跟字符集的内容和顺序密切相关
 *输入：
 *    c--待查字符
 *返回值：
 *    -1--待查字符不在字符集里
 *    整数--位置
*/
inline int getPositionFromChar(char c)
{
	if(('a'<=c) && (c<='z'))
	{
		return c-'a';
	}
	else if(('A'<=c) && (c<='Z'))
	{
		return 26+c-'A';
	}
	else if(('0'<=c) && (c<='9'))
	{
		return 52+c-'0';
	}
	else
	{
		if(g_cExtraChar1 == c)
		{
			return 62;
		}
		else if(g_cExtraChar2 == c)
		{
			return 63;
		}
		else
		{
			return -1;
		}
	}
}

/*把字符集里的四个字符转换为对应的三个字符。两个输入可以有交叉
 *输入：
 *    four--字符集里的四个字符
 *输出：
 *    three--原来的三个字符
 *返回值：
 *    0--成功
 *    -1--four里有字符不在字符集里
*/
inline int fourToThree(const char* four, char* three)
{
	int position[4];
	int i;
	for(i=0;i<4;i++)
	{
		position[i] = getPositionFromChar(four[i]);
		if(-1 == position[i])
		{
			return -1;
		}
	}
	three[0] = (position[0]<<2) + (position[1]>>4);
	three[1] = ((position[1]&0x0f)<<4) + (position[2]>>2);
	three[2] = ((position[2]&0x03)<<6) + position[3];
	return 0;
}

/*一个字符串转换为字符集里的字符，不考虑接受结果的字符串的长度。对于长度不是3的倍数，后面补\0
 *两个字符串不能有交叉
 *输入：
 *    three--待转换的字符串
 *    length--字符串的长度
 *输出：
 *    four--转换的字符集的字符串
 *返回值：
 *    整数--转换后的字符串的长度
*/
int multiThreeToFour(const char* three, int length, char* four)
{
	int i;
	for(i=0;i<length/3;i++)
	{
		threeToFour(three+i*3, four+i*4);
	}
	if(3*i != length)
	{
		char str[4];
		memset(str, 0, sizeof(str));
		memcpy(str, three+3*i, length-3*i);
		threeToFour(str, four+i*4);
		return i*4+4;
	}
	else
	{
		return i*4;
	}
}

/*把字符集里的字符的字符串还原为原来的字符串，不考虑长度限制。两个字符串可以相同
 *输入：
 *    four--字符集里的字符的字符串
 *    length--字符串的长度（应该为4的倍数，如果不是，程序取前面的4的倍数的）
 *输出：
 *    three--还原后的字符串
 *返回值：
 *    -1--four里含有不是字符集里的字符
 *    整数--还原后的字符串的长度
*/
int multiFourToThree(const char* four, int length, char* three)
{
	int ret;
	int i;
	for(i=0;i<length/4;i++)
	{
		ret = fourToThree(four+i*4, three+i*3);
		if(-1 == ret)
		{
			return -1;
		}
	}
	//还原出原来填补的0，减去添加的字符
	if('\0' == three[i*3-1])
	{
		if('\0' == three[i*3-2])
		{
			return i*3-2;
		}
		else
		{
			return i*3-1;
		}
	}
	else
	{
		return i*3;
	}
}

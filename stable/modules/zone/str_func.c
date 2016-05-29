#include "str_func.h"

#define TRUE 1
#define FALSE 0
char toup_table[] = 
{
	'\x0',  '\x1',  '\x2',  '\x3',  '\x4',  '\x5',  '\x6',  '\x7',
	'\x8',  '\x9',  '\xa',  '\xb',  '\xc',  '\xd',  '\xe',  '\xf',
	'\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
	'\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
	'\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27',
	'\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
	'\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
	'\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',
	'\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
	'\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f',
	'\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57',
	'\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
	'\x60', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
	'\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f',
	'\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57',
	'\x58', '\x59', '\x5a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',
	'\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87',
	'\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
	'\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97',
	'\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',
	'\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
	'\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',
	'\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7',
	'\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
	'\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7',
	'\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf',
	'\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7',
	'\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',
	'\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7',
	'\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',
	'\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7',
	'\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff'
};

//black_table()
int black_table[] =
{
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	0,	-1,	-1,	0,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	0,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
};

//num_table()
int num_table[] =
{
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
};

//en_table()
int en_table[] =
{
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1,	-1,	-1,	-1,	-1,	
	-1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
};

//domain_table()
int domain_table[] =
{
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	-1,	-1,	
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1,	-1,	-1,	-1,	-1,	
	-1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
};

//name_table()
int name_table[] =
{
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	0,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	-1,	-1,	
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1,	-1,	-1,	-1,	0,	
	-1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
};



unsigned char *trim(unsigned char *str)
{
	unsigned char *p;
	unsigned char *head;
	if(str == NULL)
	{
		return NULL;
	}

	//找非“空白”字符
	for(p = str; black_table[*p] == 0; p++);

	head = p;

	for ( ; *p != '\0'; p++)
	{
		if( *p == '#' || *p == '\r' || *p == '\n' )
		{
			*p = '\0';
			break;
		}

		if( *p == '/' && p[1] == '/')
		{
			*p = '\0';
			break;
		}

		//变小写
		*p = *p >= 'A' && *p <= 'Z' ? *p += 32 : *p; 
	}

	//纯注释行
	if (head == p)
	{
		return NULL;
	}
	return head;
}


//“行” 规范化

// 除去注释 和结尾换行符
// 出去前后空白字符，中间多个空白字符合并一个空格
// 统一为小写

void trim_2(unsigned char *str1, unsigned char *str2)
{
	unsigned char *p1;
	unsigned char *p2;
	int black_flag = TRUE;

	unsigned char *head;
	if(str2 == NULL )
	{
		return;
	}

	if(str1 == NULL )
	{
		*str2 = '\0';
	}

	//找非“空白”字符
	for(p1 = str1; black_table[*p1] == 0; p1++);

	head = p1;
	p2 = str2;
	black_flag = FALSE;


	for ( ; *p1 != '\0'; p1++)
	{
		if (black_table[*p1] == 0)
		{
			if (black_flag == FALSE)
			{
				*p2++ = ' ';
				black_flag = TRUE;
				continue;
			}
		}
		
		if( *p1 == '#' || *p1 == '\r' || *p1 == '\n' )
		{
			break;
		}

		if( *p1 == '/' && p1[1] == '/')
		{
			break;
		}

		*p2++ = *p1 >= 'A' && *p1 <= 'Z' ? *p1 += 32 : *p1; 
		black_flag = FALSE;
	}
}


char * str2num(register char *cp, int *num)
{
	register int val;	
	register unsigned int c;

	c = *cp;
	val = 0;
	if (num_table[c] == -1)
	{
		return NULL;
	}
	for (;;) 
	{
		if (num_table[c] == 0) 
		{
			val = (val * 10) + (c - '0');
			c = *++cp;
		}
		else
		{
			break;
		}
	}

	*num = val;
	return cp;
}




char * str_name(register char *cp)
{
	register unsigned int c;

	c = *cp;
	if (name_table[c] == -1)
	{
		return NULL;
	}

	for (;;) 
	{
		if (name_table[c] == 0) 
		{
			c = *++cp;
		}
		else
		{
			break;
		}
	}
	return cp;
}


unsigned char * cmp_timestamp_for_linkpath(register unsigned char *cp, 
								  register unsigned char *now, 
								  int *p_has_expired)
{
	int has_expired = 0;
	while((*cp==*now)) 
	{ 
		if (*now == '\0')
		{
			//相同
			*p_has_expired = 0;
			return cp;
		}
		cp++; 
		now++; 
	}

	//计算大小
	has_expired = (int)*cp - (int)*now;
	//检查剩余字符是否为数字
	while (*now) 
	{
		if(num_table[(int)(*cp)] != 0)
		{
			return NULL;//如果不是数字(包括\0)
		}
		cp++; 
		now++; 
	}
	*p_has_expired = has_expired;
	return cp;
}











unsigned int mask_table[] =
{
	0x00000000,

		0x80000000,
		0xc0000000,
		0xe0000000,
		0xf0000000,

		0xf8000000,
		0xfc000000,
		0xfe000000,
		0xff000000,

		0xff800000,
		0xffc00000,
		0xffe00000,
		0xfff00000,

		0xfff80000,
		0xfffc0000,
		0xfffe0000,
		0xffff0000,

		0xffff8000,
		0xffffc000,
		0xffffe000,
		0xfffff000,

		0xfffff800,
		0xfffffc00,
		0xfffffe00,
		0xffffff00,

		0xffffff80,
		0xffffffc0,
		0xffffffe0,
		0xfffffff0,

		0xfffffff8,
		0xfffffffc,
		0xfffffffe,
		0xffffffff,
};


char * cidr_str2i(register char *cp, unsigned int *p_ip,unsigned int *p_mask)
{
	register unsigned int val;	
	register unsigned int val2;	
	register unsigned int c;
	unsigned int parts[4];
	register unsigned int *pp = parts;

	c = *cp;
	for (;;)
	{
		if (num_table[c] == -1)
		{
			return NULL;
		}

		val = 0;
		for (;;) 
		{
			if (num_table[c] == 0) 
			{
				val = (val * 10) + (c - '0');
				c = *++cp;
			}
			else
			{
				break;
			}
		}

		if (c == '.') 
		{
			if (pp >= parts + 3)
			{
				return NULL;
			}

			if (val > 0xfful)
			{
				return NULL;
			}

			*pp++ = val;
			c = *++cp;
		}
		else
		{
			break;
		}
	}

	if (pp - parts + 1 != 4)
	{
		return NULL;
	}

	if (c != '/') 
	{
		return NULL;
	}
	else
	{
		c = *++cp;
	}

	val2 = 0;
	for (;;) 
	{
		if (num_table[c] == 0) 
		{
			val2 = (val2 * 10) + (c - '0');
			c = *++cp;
		}
		else
		{
			break;
		}
	}

	if (val2 > 32)
	{
		return NULL;
	}

	val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
	*p_ip = val;
	*p_mask = mask_table[val2];

	return ++cp;
}


char * ip_str2b(register char *cp, unsigned int *p_ip);
char * ip_str2b(register char *cp, unsigned int *p_ip)
{
	register unsigned int val;	
	register unsigned int c;
	unsigned int parts[4];
	register unsigned int *pp = parts;

	c = *cp;
	for (;;)
	{
		if (num_table[c] == -1)
		{
			return NULL;
		}

		val = 0;
		for (;;) 
		{
			if (num_table[c] == 0) 
			{
				val = (val * 10) + (c - '0');
				c = *++cp;
			}
			else
			{
				break;
			}
		}

		if (c == '.') 
		{
			if (pp >= parts + 3)
			{
				return NULL;
			}

			if (val > 0xfful)
			{
				return NULL;
			}

			*pp++ = val;
			c = *++cp;
		}
		else
		{
			break;
		}
	}

	if (pp - parts + 1 != 4)
	{
		return NULL;
	}

	val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
	*p_ip = val;
	return ++cp;
}



union  _IP_temp
{
	unsigned int uiIp;
	struct UCIP
	{
		//peerip at bind
		unsigned char part1;
		unsigned char part2;
		unsigned char part3; 
		unsigned char part4;
		//
		//		unsigned char part4;
		//		unsigned char part3;
		//		unsigned char part2;
		//		unsigned char part1;
	} ucIp;
};
typedef union _IP_temp _IP_temp_t;


char * ip_str2socket(register char *cp, unsigned int *p_ip);
char * ip_str2socket(register char *cp, unsigned int *p_ip)
{
	register unsigned int val;	
	register unsigned int c;
	unsigned int parts[4];
	register unsigned int *pp = parts;
	_IP_temp_t IP;

	c = *cp;
	for (;;)
	{
		if (num_table[c] == -1)
		{
			return NULL;
		}

		val = 0;
		for (;;) 
		{
			if (num_table[c] == 0) 
			{
				val = (val * 10) + (c - '0');
				c = *++cp;
			}
			else
			{
				break;
			}
		}

		if (c == '.') 
		{
			if (pp >= parts + 3)
			{
				return NULL;
			}

			if (val > 0xfful)
			{
				return NULL;
			}

			*pp++ = val;
			c = *++cp;
		}
		else
		{
			break;
		}
	}

	if (pp - parts + 1 != 4)
	{
		return NULL;
	}

	//val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
	//p_ip = val;
	IP.ucIp.part4 = val;
	IP.ucIp.part3 = parts[2];
	IP.ucIp.part2 = parts[1];
	IP.ucIp.part1 = parts[0];
	*p_ip = IP.uiIp;

	return ++cp;
}


char * str_n_cmp(char *a , const char *b, const int len)
{
	if(strncmp(a, b, len) == 0)
	{
		return a + len;
	}
	else
	{
		return NULL;
	}
}



char *find_head(char *str)
{
	char *p;
	if(str == NULL)
	{
		return NULL;
	}

	//找非“空白”字符
	for(p = str; *p != '\0'; p++)
	{
		if(*p != ' '&& *p != '\t')
		{
			break;
		}
	}

	if(str == p)
	{
		return NULL;
	}

	if (*p == '\0')
	{
		return NULL;
	}
	return p;
}



int 
cidr_int_to_char2(unsigned int ip, unsigned int mask, char *str)
{
	unsigned int ip_part1;
	unsigned int ip_part2;
	unsigned int ip_part3;
	unsigned int ip_part4;

	unsigned int masklen = 0;

	ip_part1 = (ip >> (8+8+8)) % 256;
	ip_part2 = (ip >> (8+8  )) % 256;
	ip_part3 = (ip >> (8    )) % 256;
	ip_part4 = (ip >> (0    )) % 256;

	switch(mask) {
		case 0xffffffffu:
			masklen = 32;	
			break;
		case 0xfffffffeu:
			masklen = 31;	
			break;
		case 0xfffffffcu:
			masklen = 30;	
			break;
		case 0xfffffff8u:
			masklen = 29;	
			break;
		case 0xfffffff0u:
			masklen = 28;	
			break;
		case 0xffffffe0u:
			masklen = 27;	
			break;
		case 0xffffffc0u:
			masklen = 26;	
			break;
		case 0xffffff80u:
			masklen = 25;	
			break;
		case 0xffffff00u:
			masklen = 24;	
			break;
		case 0xfffffe00u:
			masklen = 23;	
			break;
		case 0xfffffc00u:
			masklen = 22;	
			break;
		case 0xfffff800u:
			masklen = 21;	
			break;
		case 0xfffff000u:
			masklen = 20;	
			break;
		case 0xffffe000u:
			masklen = 19;	
			break;
		case 0xffffc000u:
			masklen = 18;	
			break;
		case 0xffff8000u:
			masklen = 17;	
			break;
		case 0xffff0000u:
			masklen = 16;	
			break;
		case 0xfffe0000u:
			masklen = 15;	
			break;
		case 0xfffc0000u:
			masklen = 14;	
			break;
		case 0xfff80000u:
			masklen = 13;	
			break;
		case 0xfff00000u:
			masklen = 12;	
			break;
		case 0xffe00000u:
			masklen = 11;	
			break;
		case 0xffc00000u:
			masklen = 10;	
			break;
		case 0xff800000u:
			masklen = 9;	
			break;
		case 0xff000000u:
			masklen = 8;	
			break;
		case 0xfe000000u:
			masklen = 7;	
			break;
		case 0xfc000000u:
			masklen = 6;	
			break;
		case 0xf8000000u:
			masklen = 5;	
			break;
		case 0xf0000000u:
			masklen = 4;	
			break;
		case 0xe0000000u:
			masklen = 3;	
			break;
		case 0xc0000000u:
			masklen = 2;	
			break;
		case 0x80000000u:
			masklen = 1;	
			break;
		case 0x00000000u:
			masklen = 0;	
			break;
		default:
			return 25;
	}

	sprintf(str,"%u.%u.%u.%u/%u", 
		ip_part1, ip_part2, ip_part3, ip_part4, masklen);
	return 0;
}


void *new_zero(size_t size)
{
	void *re = malloc(size);
	if (re == NULL)
	{
		return re;
	}

	memset(re, 0, size);
	return re;
}

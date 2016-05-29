#include "md5.h"
#include <string.h>
#include "display.h"

int main(int argc, char* argv[])
{
	if(1 == argc)
	{
		char str[1024];
		unsigned char digest[32];
		while(1)
		{
			fprintf(stderr, "please input string\n");
			if(NULL == fgets(str, 1024, stdin))
			{
				continue;
			}
			getMd5Digest32(str, strlen(str)-1, digest);
			fprintf(stderr, "digest=[%.32s]\n", digest);
		}
	}
	else
	{
		char str[] = "wsg";
		unsigned char digest[32];
		getMd5Digest32(str, strlen(str), digest);
		fprintf(stderr, "digest=[%.32s]\n", digest);
	}
	return 0;
}


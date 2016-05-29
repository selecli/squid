#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "SHA256.h"

#define BUF_SIZE_256    256
#define BUF_SIZE_1024   1024

int main(int argc, char *argv[])
{
	int x, end;
	Sha256Calc cal;
	char str[BUF_SIZE_1024];

	if (1 == argc)
	{
		do {   
			fprintf(stderr, "Please input SHA256 string:\n");
			if(NULL == fgets(str, BUF_SIZE_1024, stdin))
				continue;
			end = strlen(str) - 1;
			if ('\n' == str[end])
				str[end] = '\0';
			Sha256Calc_reset(&cal);
			Sha256Calc_calculate(&cal, (SZ_CHAR *)str, strlen(str));
			printf("SHA256 result: ");
			for( x = 0; x < 32; x ++ ) printf("%02X", (SZ_UCHAR)(cal.Value[x]));
			printf("\n");
			Sha256Calc_uninit(&cal);
		} while (1);  
	}
	else
	{
		Sha256Calc_reset(&cal);
		Sha256Calc_calculate(&cal, (SZ_CHAR *)argv[1], strlen(argv[1]));
		printf("SHA256 result: ");
		for( x = 0; x < 32; x ++ ) printf("%02X", (SZ_UCHAR)(cal.Value[x]));
		printf("\n");
		Sha256Calc_uninit(&cal);
	}

	return 0;
}

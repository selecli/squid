#include "display.h"
#include "three_to_four.h"
#include <string.h>

int main(int argc, char* argv[])
{
	if(1 == argc)
	{
		char input[512];
		char output[1024];
		int flag = 0;
		int ret;
		while(1)
		{
			fprintf(stderr, "please input type:\n\tt--three to four\n\tf--four to three\n");
			if(NULL == fgets(input, 512, stdin))
			{
				continue;
			}
			if('t' == input[0])
			{
				flag = 0;
			}
			else if('f' == input[0])
			{
				flag = 1;
			}
			else
			{
				continue;
			}
			fprintf(stderr, "please input your string\n");
			if(NULL == fgets(input, 512, stdin))
			{
				continue;
			}
			if(0 == flag)
			{
				ret = multiThreeToFour(input, strlen(input)-1, output);
				fprintf(stderr, "result=[%.*s]\n", ret, output);
			}
			else
			{
				ret = multiFourToThree(input, strlen(input)-1, output);
				fprintf(stderr, "result=[%.*s]\n", ret, output);
			}
		}
	}
	else
	{
		char input[] = "wsg1";
		char output[1024];
		int ret = multiThreeToFour(input, strlen(input), output);
		fprintf(stderr, "three_to_four=[%.*s]\n", ret, output);
		ret = multiFourToThree(input, strlen(input), output);
		fprintf(stderr, "four_to_three=[%.*s]\n", ret, output);
	}
	return 0;
}


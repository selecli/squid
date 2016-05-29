#include "refresh_header.h"
rf_config config;
typedef void test_func();

typedef struct _call_func{
	char *desc;
	test_func *func;
} call_func;

void t_strto_mi(){
	char *str = "1234005678,2345321";

	int *mi = strto_mi(str,',');

	printf("int 0 : %d, int 1 : %d\n",mi[0],mi[1]);
}


void t_storeKeyPublic(){
	char *buf = "http://www.baidu.com";
	char * md5 = (char *)storeKeyPublic(buf,METHOD_GET);
	printf("md5 : %s\n",md5);
}
/*
static void options_parse(int argc, char **argv)
{
	int c;
	while(-1 != (c = getopt(argc, argv, "p:h:"))) {
		switch(c) {
			case 'p':
				break;
			default:
			   	break;
		}
	}
}

*/

int main(int argc,char *argv[]){
	call_func func_array[] = {
		{"t_strto_mi",t_strto_mi},
		{"t_storeKeyPublic",t_storeKeyPublic},
		{NULL,NULL}
	};
	
	int index = 0;
	for(;func_array[index].desc != NULL;index++){
		printf("%d - %s\n",index,func_array[index].desc);
	}

	int number;
	while(true){
		printf("\nSelect a function to run :");
		scanf("%d",&number);

		(func_array[number].func)();
	}
	return 0;
}


#include "cc_billing.h"
#include "cc_hashtable.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

//#define offsetof(type, f) ((size_t)((char *)&((type *)0)->f - (char *)(type *)0))

static void test_domain(uint32_t host)
{

	char hostname[32];
	sprintf(hostname, "www.test%03u.com", host);

	billing_open(12);

	billing_bind(hostname, 12, REMOTE_CLIENT);

	uint32_t i = 0;
	for( i = 0 ; i < 10000 ; i++ ) {
		billing_flux_read(12, 100);
		billing_flux_write(12, 100);
	}

	billing_close(12);
}

int main(int argc, char** argv)
{

	billing_init(50, 1000000, "127.0.0.1", 8877);


	uint32_t i;
	for( i = 0 ; i < 1000 ; i++ ) {
		test_domain(i);
	}


	billing_destroy();

	return 0;
}

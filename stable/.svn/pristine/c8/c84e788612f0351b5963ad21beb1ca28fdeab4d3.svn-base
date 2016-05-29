
#include "server_header.h"
/*
#define MAX_CA_IP 10

typedef struct _in_addrs
{
	struct in_addr m_addr;
	char good_ip;
} in_addrs
*/


//get_ca_ip(ip, &maddress.sin_addr)
static in_addrs host_ip[MAX_CA_IP];
static int ip_num = 0;
static int good_ip_num = 0;
static int pos = 0;
//int hash_request_uri = 0;
extern server_config config;

static void get_name_real(char *host);

static int get_pos(const char *uri)
{
	if(!config.hash_request_url)
	{
		pos++;
		return ((pos >= ip_num)?(pos = 0):pos);
	}

	assert(uri && strlen(uri));

	return ELFhash(uri)%ip_num;
}

static int init_host_ip_from_file(char *file)
{

	FILE *fp;
	static char addrbuf[32];
	int a1 = 0, a2 = 0, a3 = 0, a4 = 0;
	struct in_addr A;
	char x;


	memset(host_ip, 0, sizeof(host_ip));
	ip_num = 0;
	good_ip_num = 0;
	pos = 0;

	assert(file);

	if((fp = fopen(file, "r")) == NULL)
	{
		cclog(0, "can not open file: %s", file);
		abort();
	}

	int i = 0;
	while(fgets(addrbuf, 32 - 1, fp))
	{
		if(addrbuf[strlen(addrbuf) - 1] == '\n')
			addrbuf[strlen(addrbuf) - 1] = '\0';

		if(strlen(addrbuf) == 0)
			continue;

		if(sscanf(addrbuf, "%d.%d.%d.%d%c", &a1, &a2, &a3, &a4, &x) == 4)
		{
			if (a1 < 0 || a1 > 255)
				goto out1;
			if (a2 < 0 || a2 > 255)
				goto out1;
			if (a3 < 0 || a3 > 255)
				goto out1;
			if (a4 < 0 || a4 > 255)
				goto out1;

			snprintf(addrbuf, 32, "%d.%d.%d.%d", a1, a2, a3, a4);
			A.s_addr = inet_addr(addrbuf);

			host_ip[i].m_addr = A;
			host_ip[i].good_ip = 1;



			if(i <= MAX_CA_IP - 1)
			{
				i++;
				ip_num++;
				good_ip_num++;
				continue;
			}
			else
			{
				cclog(1, "ca ip list full, out now!");
				break;
			}

out1:
			cclog(1, "error host ip format of: %s", addrbuf);
			continue;
		}
		else
		{
			cclog(1, "error host ip format of: %s", addrbuf);
			continue;
		}

	}

	if(fp)
		fclose(fp);

	cclog(3, "after init ca ip table, we get %d ip", ip_num);

	if(ip_num > 0)
                return 0;
        else
	{
		cclog(1, "get ca ip failed");
		abort();
	}
}

static int init_host_ip_from_host(char *host)
{
	static char addrbuf[32];
        int a1 = 0, a2 = 0, a3 = 0, a4 = 0;
        struct in_addr A;
        char x;

	memset(host_ip, 0, sizeof(host_ip));
	ip_num = 0;
	good_ip_num = 0;
	pos = 0;

	if(sscanf(host, "%d.%d.%d.%d%c", &a1, &a2, &a3, &a4, &x) == 4)
	{
		if (a1 < 0 || a1 > 255)
			goto out;
		if (a2 < 0 || a2 > 255)
			goto out;
		if (a3 < 0 || a3 > 255)
			goto out;
		if (a4 < 0 || a4 > 255)
			goto out;

		snprintf(addrbuf, 32, "%d.%d.%d.%d", a1, a2, a3, a4);
		A.s_addr = inet_addr(addrbuf);

		host_ip[0].m_addr = A;
		host_ip[0].good_ip = 1;

		ip_num = 1;
		good_ip_num = 1;

		return 0;
	}
	
out:
	get_name_real(host);

	if(ip_num > 0)
		return 0;
	else
		return -1;
}
		




static void get_name_real(char *host)
{
	struct hostent *he;
	char ** p;
	struct in_addr addr;

	int i = 0;
	he = gethostbyname(host);

	p = he->h_addr_list;
	while(*p!=NULL)
	{
		addr.s_addr = *((unsigned int *)*p);
		cclog(3, "address is %s", inet_ntoa(addr));

		host_ip[i].m_addr = addr;
		host_ip[i].good_ip = 1;
		ip_num++;
		good_ip_num++;
		p++;

		if(i++ == MAX_CA_IP)
			break;
	}

	cclog(3, "we get %d address for %s", ip_num, host);
} 

int init_ca_host_ip(char * host, int type)
{
	if(type == HOST_IP)
		return init_host_ip_from_host(host);
	else
		return init_host_ip_from_file(host);
}

//host: ca host; uri: FC request uri, used for hash to get ip (if hash mod is set); addr: result host ip addr; type: host or host table in file type;


int get_ca_ip(char *host, int type, const char *uri, struct in_addr * addr)
{
	if(good_ip_num <= 0)
	{
//		if(init_host_ip_from_host(host))
		if(init_ca_host_ip(host, type))
		{
			cclog(1, "can not get ca ip for: %s", host);
			abort();
		}
	}

	int i;
	int n = 0;
	while(1)
	{
		i = get_pos(uri);


		if(n++ == ip_num)
			break;
		if(host_ip[i].good_ip)
		{
			*addr = host_ip[i].m_addr;
			cclog(8, "get ca ip %s in pos: %d", inet_ntoa(*addr), i);
		}
		return 0;
	}

	return -1;
}

int mark_bad_ip(struct in_addr addr)
{
	int i;
	for(i = 0; i < ip_num; i++)
	{
		if(memcmp(&host_ip[i].m_addr, &addr, sizeof(struct in_addr)) == 0)
		{
			host_ip[i].good_ip = 0;
			good_ip_num--;
			return 0;
		}
	}

	cclog(2, "in mark_bad_ip, can't find addr of %s", inet_ntoa(addr));
	return -1;
}

void dump_ca_host_ip()
{
	if(ip_num <= 0)
	{
		cclog(0, "ca host ip num is <= 0  exit");
		abort();
	}

	int i;
	for(i = 0; i < ip_num; i++)	
		cclog(1, inet_ntoa(host_ip[i].m_addr)?inet_ntoa(host_ip[i].m_addr):null_string);
}

#ifndef _FHR_BASE
#define _FHR_BASE

#include "fhr_territory.h"
#include "fhr_ipset.h"
#include "str_func.h"

#define MAX_CITY_NUM 100 
#define MAX_DOMAIN_SIZE 256 
#define MAX_TERRITORY_SIZE 20
#define PROTOCOL_LENGTH 20
#define MAX_URL_LENGTH 2048

typedef struct fhr_main_date fhr_main_date_t;
struct fhr_main_date
{
	fhr_ipset_t *ipset;
	fhr_territory_t *territory;
};
typedef struct zone_territory_weight
{
	char territory[MAX_TERRITORY_SIZE]; 
	char dstdomain[MAX_DOMAIN_SIZE];
	int  weight; 
}ST_ZONE_WEIGHT;

typedef struct zone_cfg
{
	char dstdomain[MAX_DOMAIN_SIZE];
	unsigned int sequence;
	unsigned int use302; 
	unsigned int subdomain;
	unsigned int url_invalid;
	unsigned int municipal_city_num; 
	ST_ZONE_WEIGHT item[MAX_CITY_NUM];  
	char default_city[MAX_DOMAIN_SIZE];
	int no_default;
}ST_ZONE_CFG;

typedef struct private_data
{
	ST_ZONE_CFG *pointer;
	int find_or_not;
}PRIVATE_DATA;

typedef struct zone_url
{   
	char protocol[PROTOCOL_LENGTH];
	char domain[MAX_DOMAIN_SIZE];
	char* other;    //
}ZONE_URL;

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <glib.h>
#else
#include <glib.h>
#endif

#endif

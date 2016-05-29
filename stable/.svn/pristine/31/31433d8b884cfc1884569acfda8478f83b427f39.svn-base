#ifndef _FHR_TERRITORY
#define _FHR_TERRITORY

#include <limits.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define G_THREADS_ENABLED
#ifdef WIN32
#include <glib.h>
#else
//#include <glib-2.0/glib.h>
#include <glib.h>
#endif

typedef struct _territory _territory_t;
struct _territory
{
	char *name;
	int weight;
	char *father;
};

typedef struct fhr_territory fhr_territory_t;
struct fhr_territory
{
	GString *db_file;
	//String *db_file;
	GHashTable *territory_table;
	//hash_table *territory_table;
};

//clean load add del output find
fhr_territory_t *load_territory_db(const char *db_file);
int output_territory_db(fhr_territory_t *fhr_territory, FILE * file);

int add_territory_db(const char *db_file, const char *territory_file);
int del_territory_db(const char *db_file, const char *territory_name);

_territory_t *find_territory(fhr_territory_t *fhr_territory, const char *territory_name);

#endif

#include <limits.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define G_THREADS_ENABLED
#ifdef WIN32
#include <glib.h>
#else
#include <glib.h>
#endif

#include "fhr_territory.h"

typedef GSList territory_list;

typedef struct ipset ipset_t;


union  _IP
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
typedef union _IP IP_t;


struct ipset
{
	char ipset_name[64+4];
	int grade;
	char group[64+4];
	char country[64+4];
	char isp[64+4];
	char region[64+4];
	char province[64+4];
	char city[64+4];
};

typedef struct _cidr _cidr_t;
struct _cidr
{
	unsigned int ip;
	unsigned int mask;
	ipset_t *ipset;
	_cidr_t *father;
	GSList *t_list;
};


//Trie Tree
typedef struct trie_tree_node trie_tree_node_t;
typedef struct trie_tree_node_array trie_tree_node_array_t;

struct trie_tree_node
{
	trie_tree_node_array_t *next_subtree;
	_cidr_t *ipset_list;
};

struct trie_tree_node_array
{
	trie_tree_node_t node_array[256];
	trie_tree_node_array_t *list_next;
};

typedef struct ip_trie_tree
{
	trie_tree_node_array_t *root;
	trie_tree_node_array_t *node_list;
}ip_trie_tree_t;



typedef struct fhr_ipset fhr_ipset_t;
struct fhr_ipset
{
	GString *db_file;

	GHashTable* ipset_table;
	GSList* cidr_list;
	ip_trie_tree_t *trie_tree;
};

fhr_ipset_t *load_ipset_db(const char *db_file);
int output_ipset_db(fhr_ipset_t *fhr_ipset, FILE * file);


//ip_trie_tree_t *init_trie_tree();
int destroy_trie_tree(ip_trie_tree_t **p_trie_tree);
//int make_trie_tree(ip_trie_tree_t *trie_tree, GSList* cidr_list);
_cidr_t * trie_tree_lookup(ip_trie_tree_t *trie_tree, unsigned int ip);


//int add_territory_info(GSList* cidr_list, fhr_territory_t *fhr_territory);

int make_fhr_ip_find_tree(fhr_ipset_t *fhr_ipset, fhr_territory_t *fhr_territory);

_territory_t *find_territory_form_cidr(_cidr_t *cidr, char *str);



GString *territory_str(_cidr_t *cidr);
GString *ipset_str(_cidr_t *cidr);


int cidr_int_to_char(const _cidr_t *cidr, char *str);

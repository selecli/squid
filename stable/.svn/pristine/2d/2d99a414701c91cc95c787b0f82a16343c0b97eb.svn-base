#include "fhr_ipset.h"
#include "str_func.h"

int
cidr_int_to_char(const _cidr_t *cidr, char *str)
{
	u_int32_t ip_part1;
	u_int32_t ip_part2;
	u_int32_t ip_part3;
	u_int32_t ip_part4;
	u_int32_t ip = cidr->ip;
	u_int32_t masklen = 0;

	ip_part1 = (ip >> (8+8+8)) % 256;
	ip_part2 = (ip >> (8+8  )) % 256;
	ip_part3 = (ip >> (8    )) % 256;
	ip_part4 = (ip >> (0    )) % 256;

	switch(cidr->mask) {
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

/*
static int CompareCidr_(const _cidr_t *x , const _cidr_t *y )
{
	if (x->ip > y->ip)
	{
		return 1;
	}
	else if (x->ip < y->ip)
	{
		return -1;
	}
	else
	{
		if (x->mask > y->mask)
		{
			return 1;
		}
		else if (x->mask < y->mask)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
}


*/

static  void DestroyKey(gpointer data)
{
	UNUSED(data);
}


void destroy_ipset(ipset_t *ipset)
{
	NO_NULL_FREE(ipset);
}

static   void DestroyVal(gpointer data)
{
	destroy_ipset((ipset_t*)data);
}


fhr_ipset_t *new_ipset_db(const char *db_file)
{
	fhr_ipset_t *fhr_ipset = NULL;

	fhr_ipset = NEW_ZERO(fhr_ipset_t);
	fhr_ipset->db_file = g_string_new(db_file);
	fhr_ipset->ipset_table = g_hash_table_new_full
		(g_str_hash, g_str_equal, DestroyKey, DestroyVal); 
	return fhr_ipset;

}



fhr_ipset_t *load_ipset_db(const char *db_file)
{
	FILE * file = NULL;

	//GString *father = NULL;
	fhr_ipset_t *fhr_ipset = NULL;
	ipset_t *ipset = NULL;
	GSList *cidr_list = NULL;
	unsigned int ip, mask;
	VALUES;

	file = fopen(db_file, "r");
	if (file == NULL)
	{
		return FALSE;
	}

	
	fhr_ipset = new_ipset_db(db_file);

	while( fgets( line, MAX_LINE_BUF, file ) != NULL)
	{
		line[MAX_LINE_BUF] = '\0';

		head = (char*)trim( (unsigned char*)line);
		if (head == NULL)
		{
			continue;
		}
		IF_STR_N_CMP("ipset")
		{
			CHECK_BEGIN;
		}
		else IF_STR_N_CMP("name")
		{
			CHECK_NAME;
			IF_IS_TRUE(ipset == NULL);
			IF_IS_TRUE(cidr_list == NULL);

			*pe = '\0';
			ipset = g_hash_table_lookup(fhr_ipset->ipset_table, pb);
			if (ipset == NULL)
			{
				ipset = NEW_ZERO(ipset_t);
				CHECK_NAME_SIZE(ipset->ipset_name);
				COPY_NAME(ipset->ipset_name);
				g_hash_table_insert(fhr_ipset->ipset_table, ipset->ipset_name, ipset);
			}
		}
		else IF_IS_TRUE(ipset != NULL)
		else IF_IS_TRUE(*ipset->ipset_name != '\0')
		else if ((tail = cidr_str2i(head, &ip, &mask)) != NULL)
		{
			_cidr_t *cidr = NEW_ZERO(_cidr_t);
			ip &= mask;
			cidr->ip = ip;
			cidr->mask = mask;
			cidr->ipset = ipset;
			cidr_list = g_slist_prepend(cidr_list, cidr);
		}
		else IF_STR_N_CMP("group")
		{					
			//CHECK_NAME;
		}		
		else IF_STR_N_CMP("grade")
		{
			//CHECK_NAME;
		}
		else IF_STR_N_CMP("country")
		{
			//CHECK_NAME;
		}
		else IF_STR_N_CMP("isp")
		{
			//CHECK_NAME;
		}
		else IF_STR_N_CMP("region")
		{
			//CHECK_NAME;
		}
		else IF_STR_N_CMP("province")
		{
			//CHECK_NAME;
		}
		else IF_STR_N_CMP("city")
		{
			//CHECK_NAME;
		}
		else IF_STR_N_CMP("node")
		{
			//CHECK_NAME;
		}
		else IF_IS_TRUE(cidr_list != NULL)
		else IF_STR_N_CMP("}")
		{
			CHECK_END;
			fhr_ipset->cidr_list = g_slist_concat (cidr_list, fhr_ipset->cidr_list);
			ipset = NULL;
			cidr_list = NULL;
		}
	}

	if (ipset != NULL)
	{
		goto cleanup;
	}
	fclose(file);
	return fhr_ipset;

cleanup:
	fclose(file);
	return NULL;
}




int insert_trie(ip_trie_tree_t *trie_tree, _cidr_t *p_cidr);

ip_trie_tree_t *init_trie_tree(void);
trie_tree_node_array_t *insert_empty_subtree(ip_trie_tree_t *trie_tree);
ip_trie_tree_t *init_trie_tree(void)
{
	ip_trie_tree_t *trie_tree;
	trie_tree = (ip_trie_tree_t *)malloc(sizeof(ip_trie_tree_t));
	if (trie_tree == NULL)
	{
		return NULL;
	}

	trie_tree->root = NULL;
	trie_tree->node_list = NULL;
	return trie_tree;
}

int destroy_trie_tree(ip_trie_tree_t **p_trie_tree)
{
	trie_tree_node_array_t *p_node;
	ip_trie_tree_t *trie_tree;

	if (p_trie_tree == NULL)
	{
		return FALSE;
	}

	trie_tree = *p_trie_tree;
	if(trie_tree == NULL)
	{
		return FALSE;
	}


	while(trie_tree->node_list != NULL)
	{
		p_node = trie_tree->node_list;
		trie_tree->node_list = trie_tree->node_list->list_next;
		free(p_node);
	}

	free(trie_tree);

	return TRUE;
}



int make_trie_tree(ip_trie_tree_t *trie_tree, GSList* cidr_list)
{
	GSList *p_node = NULL;
	_cidr_t *p_cidr = NULL;
	unsigned int ip;
	unsigned int mask;
	int i= 0;

	_cidr_t *ipset_list = NULL;

	if (trie_tree == NULL)
	{
		return FALSE;
	}

	ipset_list = NULL;
	for(p_node = cidr_list; p_node != NULL; p_node = p_node->next)
	{
		p_cidr = p_node->data;
		ip = p_cidr->ip;
		mask = p_cidr->mask;
		i++;

		//if (strcmp("ipset-usr-uni-soufun", p_cidr->ipset_name) == 0)
		//{
		//	printf("");
		//}
		//
		for(; ipset_list != NULL ;  ipset_list = ipset_list->father)
		{
			if(~(ipset_list->ip ^ ip) >= ipset_list->mask)//判断father关系
			{
				break;
			}
		}
		p_cidr->father = ipset_list;
		ipset_list = p_cidr;

		if(insert_trie(trie_tree, ipset_list))
		{
			//free Trie Tree
		}
	}
	return TRUE;
}


trie_tree_node_array_t *insert_empty_subtree(ip_trie_tree_t *trie_tree)
{
	unsigned int i;
	trie_tree_node_array_t *trie_tree_node_array = (trie_tree_node_array_t*)malloc(sizeof(trie_tree_node_array_t));

	if(trie_tree_node_array != NULL)
	{
		//for free.
		trie_tree_node_array->list_next = trie_tree->node_list;
		trie_tree->node_list = trie_tree_node_array;



		for (i = 0; i < 256; i++)
		{
			trie_tree_node_array->node_array[i].next_subtree = NULL;
			trie_tree_node_array->node_array[i].ipset_list = NULL;
		}
		return trie_tree_node_array;
	}
	else
	{
		return NULL;
	}
}

int insert_trie(ip_trie_tree_t *trie_tree, _cidr_t *p_cidr)
{
	unsigned int ip_part4 = p_cidr->ip % 256;
	unsigned int ip_part3 = p_cidr->ip/256 % 256;
	unsigned int ip_part2 = p_cidr->ip/(256*256) % 256;
	unsigned int ip_part1 = p_cidr->ip/(256*256*256);

	unsigned int max_ip = p_cidr->ip + ~ p_cidr->mask;

	unsigned int max_ip_part4 = max_ip % 256;
	unsigned int max_ip_part3 = max_ip / 256 % 256;
	unsigned int max_ip_part2 = max_ip / (256*256) % 256;
	unsigned int max_ip_part1 = max_ip / (256*256*256);

	unsigned int i = 0;
	trie_tree_node_t *node_array = NULL;


	if(trie_tree == NULL)
	{
		return FALSE;
	}

	if(trie_tree->root == NULL)
	{
		trie_tree->root = insert_empty_subtree(trie_tree);
		if(trie_tree->root == NULL)
		{
			//freeTrieTree
			return FALSE;
		}
	}

	node_array = trie_tree->root->node_array;
	if (p_cidr->mask <= 0xff000000)
	{
		for(i = ip_part1; i <= max_ip_part1; i++)
		{
			node_array[i].ipset_list = p_cidr;
		}
		return TRUE;
	}
	else
	{
		if (node_array[ip_part1].next_subtree == NULL)
		{
			if((node_array[ip_part1].next_subtree = insert_empty_subtree(trie_tree)) == NULL)
			{
				//freeTrieTree
				return FALSE;
			}
		}
	}

	node_array = node_array[ip_part1].next_subtree->node_array;
	if (p_cidr->mask <= 0xffff0000)
	{
		for(i = ip_part2; i <= max_ip_part2; i++)
		{
			node_array[i].ipset_list = p_cidr;
		}
		return TRUE;
	}
	else
	{
		if (node_array[ip_part2].next_subtree == NULL)
		{
			if((node_array[ip_part2].next_subtree = insert_empty_subtree(trie_tree)) == NULL)
			{
				//freeTrieTree
				return FALSE;
			}
		}
	}


	node_array = node_array[ip_part2].next_subtree->node_array;
	if (p_cidr->mask <= 0xffffff00)
	{
		for(i = ip_part3; i <= max_ip_part3; i++)
		{
			node_array[i].ipset_list = p_cidr;
		}
		return TRUE;
	}
	else
	{
		if (node_array[ip_part3].next_subtree == NULL)
		{
			if((node_array[ip_part3].next_subtree = insert_empty_subtree(trie_tree)) == NULL)
			{
				//freeTrieTree
				return FALSE;
			}
		}
	}

	node_array = node_array[ip_part3].next_subtree->node_array;
	for(i = ip_part4; i <= max_ip_part4; i++)
	{
		node_array[i].ipset_list = p_cidr;
	}
	return TRUE;
}


_cidr_t * trie_tree_lookup(ip_trie_tree_t *trie_tree, unsigned int ip)
{
	trie_tree_node_t *node_array = NULL;
	_cidr_t * temp_results_list = NULL;//must
	IP_t ip2;

	ip2.uiIp = ip;


	//printf("%x:%x.%x.%x.%x\n", ip,(unsigned int)ip2.ucIp.part1, (unsigned int)ip2.ucIp.part2, (unsigned int)ip2.ucIp.part3, (unsigned int)ip2.ucIp.part4);

	if(trie_tree == NULL)
	{
		return FALSE;
	}
	if(trie_tree->root == NULL)
	{
		return FALSE;
	}

	node_array = &trie_tree->root->node_array[ip2.ucIp.part1];
	if (node_array->ipset_list != NULL)
	{
		temp_results_list = node_array->ipset_list;
	}
	if(node_array->next_subtree == NULL)
	{
		return temp_results_list;
	}

	node_array = &node_array->next_subtree->node_array[ip2.ucIp.part2];
	if (node_array->ipset_list != NULL)
	{
		temp_results_list = node_array->ipset_list;
	}
	if(node_array->next_subtree == NULL)
	{
		return temp_results_list;
	}

	node_array = &node_array->next_subtree->node_array[ip2.ucIp.part3];
	if (node_array->ipset_list != NULL)
	{
		temp_results_list = node_array->ipset_list;
	}
	if(node_array->next_subtree == NULL)
	{
		return temp_results_list;
	}

	node_array = &node_array->next_subtree->node_array[ip2.ucIp.part4];
	if (node_array->ipset_list != NULL)
	{
		return node_array->ipset_list;
	}
	return temp_results_list;
}

int add_territory_info(GSList* cidr_list, fhr_territory_t *fhr_territory)
{
	
	GSList *p_node = NULL;
	GSList *p_f_list = NULL;

	_cidr_t *p_cidr = NULL;
	_cidr_t *p_f = NULL;

	_territory_t *territory = NULL;

	NULL_RE_FALSE(cidr_list);
	NULL_RE_FALSE(fhr_territory);

	for(p_node = cidr_list; p_node != NULL; p_node = p_node->next)
	{
		p_cidr = p_node->data;

		for (territory = g_hash_table_lookup(fhr_territory->territory_table, p_cidr->ipset->ipset_name);
			territory != NULL;
			territory = territory->father == NULL ? NULL : g_hash_table_lookup(fhr_territory->territory_table, territory->father))
		{
			for (p_f = p_cidr; p_f != NULL; p_f = p_f->father)
			{
				for (p_f_list = p_f->t_list; p_f_list != NULL; p_f_list = p_f_list->next)
				{
					if (territory == (_territory_t *)(p_f_list->data))
					{
						break;//找到相同territory终止遍历
					}
				}
				if (p_f_list != NULL)
				{
					break;//找到相同territory终止遍历
				}
			}
			if (p_f != NULL)
			{
				break;;//找到相同territory终止遍历
			}
			//遍历后没有找到,加入列表
			p_cidr->t_list = g_slist_prepend (p_cidr->t_list, territory);
		}
	}
	return TRUE;	
}



gint CidrCmpFunc(gconstpointer a, gconstpointer b)
{
	const _cidr_t *ca = a;
	const _cidr_t *cb = b;

	if (ca->ip > cb->ip)
	{
		return 1;
	}
	else if (ca->ip < cb->ip)
	{
		return -1;
	}
	else if (ca->mask > cb->mask)
	{
		return 1;
	}
	else if (ca->mask < cb->mask)
	{
		return -1;
	}
	else 
	{
		return strcmp(ca->ipset->ipset_name, cb->ipset->ipset_name);
	}

}



int make_fhr_ip_find_tree(fhr_ipset_t *fhr_ipset, fhr_territory_t *fhr_territory)
{
	fhr_ipset->cidr_list = g_slist_sort (fhr_ipset->cidr_list, CidrCmpFunc);

	fhr_ipset->trie_tree = init_trie_tree();
	make_trie_tree(fhr_ipset->trie_tree, fhr_ipset->cidr_list);

	add_territory_info(fhr_ipset->cidr_list, fhr_territory);

	return TRUE;
}

_territory_t *find_territory_form_cidr(_cidr_t *cidr, char *str)
{
	_cidr_t *p_c;
	GSList * p_t;
	for(p_c = cidr; p_c != NULL; p_c = p_c->father)
	{
		for (p_t = p_c->t_list; p_t != NULL; p_t = p_t->next)
		{
			if (strcmp(((_territory_t *)p_t->data)->name , str) == 0)
			{
				return (_territory_t *)p_t->data;
			}			
		}
	}
	return NULL;
}

void  InitIpserStrFunc(gpointer key, gpointer value, gpointer user_data)
{
	g_hash_table_insert(user_data, value, g_string_new(""));
}

gint  OutIpserStrFunc(gpointer key, gpointer value, gpointer user_data)
{
	ipset_t *ipset = key;
	GString *cidr_list_str = value;
	if (key == NULL)
	{
		return TRUE;
	}

	fprintf(user_data, "ipset {\n");
	fprintf(user_data, "\tname\t\"%s\"\n", ipset->ipset_name);
	fprintf(user_data, "%s", cidr_list_str->str);
	fprintf(user_data, "}\n\n");
	g_string_free(cidr_list_str	, TRUE);
	return TRUE;
}


int output_ipset_db(fhr_ipset_t *fhr_ipset, FILE * file)
{
	//FILE *file = NULL;
	GSList *p;
	_cidr_t *cidr = NULL;
	GHashTable *ipser_str = g_hash_table_new(g_direct_hash, g_direct_equal);
	GString *str = NULL;
	char c_cidr[sizeof("XXX.XXX.XXX.XXX/XX")];

	if(file == NULL)
	{ 
		return FALSE;
	}
	
	//init
	g_hash_table_foreach(fhr_ipset->ipset_table, InitIpserStrFunc, ipser_str);
	for (p = fhr_ipset->cidr_list; p != NULL; p = p->next)
	{
		cidr = p->data;
		str = g_hash_table_lookup(ipser_str, cidr->ipset);
		if (str)
		{
			cidr_int_to_char(cidr, c_cidr);
			g_string_append_printf(str, "\t%s\n", c_cidr);
		}
	}
	
	//out & remove
	g_hash_table_foreach_remove(ipser_str, OutIpserStrFunc, file);

	g_hash_table_destroy(ipser_str);	

	return TRUE;
}



GString *territory_str(_cidr_t *cidr)
{
	_cidr_t *p_c;
	GSList * p_t;
	_territory_t *territory;
	GString *str = g_string_new("");
	
	for(p_c = cidr; p_c != NULL; p_c = p_c->father)
	{
		for (p_t = p_c->t_list; p_t != NULL; p_t = p_t->next)
		{
			territory = p_t->data;
			g_string_append_printf(str, "%s;", territory->name);			
		}
	}

	g_string_append_printf(str, "default;");			
	
	return str;
}


GString *ipset_str(_cidr_t *cidr)
{
	_cidr_t *p_c;
	GString *str = g_string_new("");

	for(p_c = cidr; p_c != NULL; p_c = p_c->father)
	{
		g_string_append_printf(str, "%s;", p_c->ipset->ipset_name);			
	}

	g_string_append_printf(str, "default;");			

	return str;
}

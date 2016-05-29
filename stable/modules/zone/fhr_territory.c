#include "fhr_territory.h"
#include "str_func.h"

void DestroyKey(gpointer data)
{
	UNUSED(data);
}

void destroy_territory(_territory_t *territory)
{
	if (territory == NULL)
	{
		return;
	}
	NO_NULL_FREE(territory->name);
	NO_NULL_FREE(territory->father);
}

void DestroyVal(gpointer data)
{
	destroy_territory((_territory_t*)data);
}

fhr_territory_t *new_territory_db(const char *db_file)
{
	fhr_territory_t *fhr_territory = NULL;

	fhr_territory = NEW_ZERO(fhr_territory_t);
	fhr_territory->db_file = g_string_new(db_file);
	fhr_territory->territory_table = g_hash_table_new_full
		(g_str_hash, g_str_equal, DestroyKey, DestroyVal); 
	return fhr_territory;
}

void destroy_territory_db(fhr_territory_t *fhr_territory )
{
	if (fhr_territory == NULL)
	{
		return;
	}

	NO_NULL_USER(fhr_territory->territory_table, g_hash_table_destroy);
	if (!fhr_territory->db_file)
	{
		g_string_free(fhr_territory->db_file,  TRUE);
	}
	NO_NULL_FREE(fhr_territory);	
}

fhr_territory_t *init_territory_db(const char *db_file)
{
	fhr_territory_t *fhr_territory = NULL;

	fhr_territory = NEW_ZERO(fhr_territory_t);
	fhr_territory->db_file = g_string_new(db_file);
	fhr_territory->territory_table = g_hash_table_new_full
		(g_str_hash, g_str_equal, DestroyKey, DestroyVal); 
	return fhr_territory;
}


fhr_territory_t *load_territory_db(const char *db_file)
{
	FILE * file = NULL;

	GString *father = NULL;
	fhr_territory_t *fhr_territory = NULL;
	_territory_t *territory = NULL;
	VALUES;

	file = fopen(db_file, "r");
	if (file == NULL)
	{
		return FALSE;
	}

	father = g_string_new("");
	
	fhr_territory = new_territory_db(db_file);

	while( fgets( line, MAX_LINE_BUF, file ) != NULL)
	{
		line[MAX_LINE_BUF] = '\0';

		head = (char*)trim((unsigned char*) line);
		if (head == NULL)
		{
			continue;
		}
		
		IF_STR_N_CMP("territory")
		{
			CHECK_BEGIN;

			territory = NEW_ZERO(_territory_t);
			territory->weight = INT_IS_NULL;
			g_string_erase(father, 0, -1);
		}
		else if (territory == NULL)
		{
			goto cleanup;
		}
		else IF_STR_N_CMP("}")
		{
			CHECK_END;
			
			
			{
				_territory_t *territory2 =  g_hash_table_lookup(fhr_territory->territory_table, territory->name);
				if (territory2 != NULL)
				{
					if ( territory2->father != NULL)
					{
						if (strcmp(father->str, territory2->father) != 0)
						{
							if (*father->str != '\0')
							{
								goto cleanup;
							}
						}
					}
					if (father->len != 0)
					{
						territory2->father = strdup(father->str);					
					}

					if (territory->weight != INT_IS_NULL)
					{
						territory2->weight= territory->weight;
					}
					destroy_territory(territory);
					territory = NULL;
				}				
				else
				{
					if (father->len != 0)
					{
						territory->father = strdup(father->str);					
					}
					g_hash_table_insert (fhr_territory->territory_table, territory->name, territory);
					territory = NULL;
				}
			}
			

		}
		else IF_CHECK_NEW_STR("name", territory->name)
		else IF_CHECK_NEW_NUM("weight", territory->weight)
		else IF_STR_N_CMP("father")
		{
			CHECK_NAME;
			g_string_append_len(father, pb, (gssize)(pe - pb));
			//g_string_append_c(father, ';');
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
	}

	g_string_free(father,  TRUE);
	if (territory != NULL)
	{
		goto cleanup;
	}
	fclose(file);
	return fhr_territory;

cleanup:
	g_string_free(father,  TRUE);
	destroy_territory(territory);
	destroy_territory_db(fhr_territory);
	fclose(file);
	return NULL;
}


void output_territory(_territory_t *territory, FILE *file)
{
	fprintf(file, "territory {\n");
	fprintf(file, "\tname\t\"%s\"\n", territory->name);
	if (territory->weight != INT_IS_NULL) fprintf(file, "\tweight\t%d\n", territory->weight);
	if (territory->father != NULL) 
	{
		gchar** fathers = g_strsplit(territory->father, ";", -1);
		if (fathers != NULL)
		{
			gchar** p;
			for (p = fathers; *p != NULL; p++)
			{
				if (**p != '\0')
				{
					fprintf(file, "\tfather\t\"%s\"\n", *p);
				}
			}
			g_strfreev(fathers);  
		}
	}
	fprintf(file, "}\n");
}


void OutputTerritory(gpointer key, gpointer value, gpointer user_data)
{
	output_territory((_territory_t *)value, (FILE *)user_data);
}


int output_territory_db(fhr_territory_t *fhr_territory, FILE * file)
{
	if (file == NULL)
	{
		return FALSE;
	}
	g_hash_table_foreach(fhr_territory->territory_table, OutputTerritory, file);
	return TRUE;
}


_territory_t *find_territory(fhr_territory_t *fhr_territory, const char *territory_name)
{
	NULL_RE_NULL(fhr_territory);
	NULL_RE_NULL(territory_name);
	NULL_RE_NULL(fhr_territory->territory_table	);
	return g_hash_table_lookup(fhr_territory->territory_table, territory_name);
}


int add_territory_db(const char *db_file, const char *territory_file);
int del_territory_db(const char *db_file, const char *territory_name);

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <stdint.h>

//hashtable长度，由于是记录域名，entry量应该不会很大，1024足够了
#define TABLESIZE	0x400UL	//1024
#define	HOSTSIZE	4096

extern uint32_t hashtable_entry_count;	//hasntable内节点数量

struct flux_connection
{
	uint64_t	read_size;
	uint64_t	write_size;
	uint32_t	connection_times;
};


struct billing_attr
{
	struct	flux_connection	client;
	struct	flux_connection	source;
};


//hash_entry中不再用*data之类的指针指向真实数据，达到松耦合
//而把数据直接挂在节点上，完全是从性能上以及内存碎片的考虑
struct hash_entry
{
	int32_t key;
	int32_t	used;					//bind后++，fd close前不能删除节点
	char	host[HOSTSIZE];			//计费域名
	struct	billing_attr local;		//本地客户端，如preload
	struct	billing_attr remote;	//远程客户端
	struct	billing_attr fc;		//下层fc来的请求
	struct	hash_entry *next;
};


//初始化hashtable
extern void hashtable_init();


//销毁hashtable
extern void hashtable_dest();


//创建一个entry
extern struct hash_entry* hashtable_create(const char* host);


//打开一个entry
extern struct hash_entry* hashtable_get(const char* host);


//
extern void hashtable_delete(struct hash_entry *entry);


//外部通过hashtable_entry_count得到hashtable内节点数，先申请好内存
//再调用该函数，得到所有节点的地址
extern uint32_t get_all_entry(struct hash_entry ** entry_array);


#endif /*HASHTABLE_H_*/

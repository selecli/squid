#include "mod_billing_cal.h"
#include "mod_billing_hashtable.h"

/*
 *	在应用层计录每个host的流量是非常困难的，因为而要跨层操作
 *	fdtable记录每个fd的流量，包括socket、filefd、pipe ...
 *	在应用层只要将host和相应fd作绑定即可
 */
struct fd2host
{
	struct hash_entry	*billing_node;
	int32_t type;		//引起流量的类型，fc、preload、client ...
	unsigned long long int read_size;
	unsigned long long int write_size;
};


//最多记录fdtable_size个fd，超过的话将会出错
#define fdtable_size  (1024 * 1024)

struct fd2host  fdtable[fdtable_size];


//mereg 进程地址端口
char billing_host[16] = {0};
unsigned short billing_port = 0;

/* 总得计费读写流量 */
unsigned long long int read_count = 0;
unsigned long long int write_count = 0;

static int sync_socket = -1;			//同步socket，关闭时一定要置为-1
static int sync_socket_flag = -1;		//-1准备链接，0链接中，1链接成功


MemBuf	billing_send_mb;	//计费信息数据,使用squid提供的基于MemPool机制的MemBuf类型
static char*	current_send_buff = NULL;	//当前发送到哪了

bool inited = false;

static time_t last_ok = 0;  //最后一次发送成功的时间

extern struct mod_conf_param *dir_config;


//初始化计费功能，sync_time为每次少秒同步一次计费信息，或者每增加多少流量同步一次计费信息
//sync_time == 0时，忽略该条件，sync_size == 0时，忽略该条件
//time_out，
//squid在reload时，不用重新调动该函数
void billing_init(const char* host_ip, const unsigned short port)
{
	assert(inited == false);
	assert( host_ip );
	assert(strlen(host_ip) <= 15);

	uint32_t i;
	for( i = 0 ; i < strlen(host_ip) ; i++ ) {
		assert( (host_ip[i] == '.') || ((host_ip[i] >= '0') && (host_ip[i] <= '9')) );
	}

	for( i = 0 ; i < fdtable_size ; i++ ) {
		fdtable[i].billing_node = NULL;
		fdtable[i].type = -1;
		fdtable[i].read_size = 0;
		fdtable[i].write_size = 0;
	}

	hashtable_init();

	strcpy(billing_host, host_ip);
	billing_port = port;

	inited = true;
	last_ok = time(0);
	memBufDefInit(&billing_send_mb); //MemBuf初始化
}


//销毁计费功能，squid在reload时，不用重新调用本函数
void billing_destroy()
{
	assert( inited == true );

	//cleanup 的hook提前, 导致此时fd没关闭.
	//在billing_destroy前，要保证所有的fd都关闭
	uint32_t i;
//	for( i = 0 ; i < fdtable_size ; i++ ) {
		//在destroy前，所有计费(执行过bind)的fd都应该关闭，
//		assert( fdtable[i].billing_node == NULL );
//		assert( fdtable[i].type == -1 );

		//这两句应该出错，非记费的fd，碰巧在打开中，是有可能的
		//下面这两句应该关闭。但最好是在所有的fd都close时，再destroy，这样也不会出错
		//assert( fdtable[i].read_size == 0 );
		//assert( fdtable[i].write_size == 0 );
//	}

	for( i = 0 ; i < fdtable_size ; i++ ) {
		billing_unbind(i);
	}

	//5次机会连接
	for( i = 0 ; i < 20  ; i++ ) {
		//100ms超时
		if( billing_sync(true) == 4 ) //build_buff() return 4;
			break;
		cc_usleep(100000);
	}

	for( i = 0 ; i < 100 ; i++ ) {
		if( billing_sync(true) == 3 ) //send ok
			break;
		cc_usleep(100);
	}

	hashtable_dest();

	memBufClean(&billing_send_mb); //销毁MemBuf
	current_send_buff = NULL;
	inited = false;
}


//
//长联接处理
//如果是长联接的话，在一个fd上会有不同的host
//也就是说，在fd close之前，会有多个host
//就要显示的做bind脱离
//
void billing_unbind(int fd)
{
	//该fd从来没来过
	if( fdtable[fd].billing_node == NULL ) {
		//httprequest异常的情况下，会发生过未走bind()流程，而进入unbind流程
		return;
	}

	//执行过bind，流量已移交到hashtable中
	assert( fdtable[fd].billing_node );
	assert(fdtable[fd].read_size == 0);
	assert(fdtable[fd].write_size == 0);
	assert(fdtable[fd].billing_node->used > 0);

	fdtable[fd].billing_node->used--;
	fdtable[fd].billing_node = NULL;
	fdtable[fd].type = -1;
}


//billing定时同步，包括链接，维护等一系统工作，约每秒钟调用一次为宜
//time_out 为ms，在time_out内，必须返回
//按理说，只处只应该管bind，但出于时效性要求，在bind前的流量记在fdtable中，一但bind后
//流量就记在hashtable中，可以随时发出，而不是在fd close的时候再移到hashtable中。
void billing_bind(const char* host, int fd, int type)
{
	assert(type); //type是offset，为0则错
	//assert( fdtable[fd].billing_node == NULL );
	//assert( fdtable[fd].type == -1 );


//一般来说，这个值为空。 但由于某种原因， 一定绑定过了，那么就改变原有绑定。 总以目前的情况为真
	if( fdtable[fd].billing_node != NULL )
		billing_unbind(fd);

	fdtable[fd].billing_node = hashtable_get(host);

	if( fdtable[fd].billing_node == NULL ) {
		fdtable[fd].billing_node = hashtable_create(host);
	}

	fdtable[fd].billing_node->used++;

	struct flux_connection *flux = (void*)fdtable[fd].billing_node + type;
	flux->read_size += fdtable[fd].read_size;
	flux->write_size += fdtable[fd].write_size;
	flux->connection_times++;
	fdtable[fd].read_size = 0;
	fdtable[fd].write_size = 0;
	fdtable[fd].type = type;

	//总流量计数器没直接在read()、write()的入口处计，是因为有些非socket的流量
	//一定要在bind之后的流量才是有效的，才计入总流量
	read_count += flux->read_size;
	write_count += flux->write_size;
}



char* fd2host(int fd)
{
	//查看的一定是绑定过的
	assert( fdtable[fd].billing_node );

	return fdtable[fd].billing_node->host;
}


void billing_open(int fd)
{
	assert(fdtable[fd].billing_node == NULL);
	assert(fdtable[fd].read_size == 0);
	assert(fdtable[fd].write_size == 0);
	assert(fdtable[fd].type = -1);
}


void   billing_flux_read(int fd, uint32_t read_size)
{
	//当squid操作fd，但fd对应的
	if( fdtable[fd].billing_node == NULL ) {
		fdtable[fd].read_size += read_size;
		return;
	}

	//open的时候，已经fd的流量转到host上了
	assert(fdtable[fd].read_size == 0);
	assert(fdtable[fd].write_size == 0);
	assert(fdtable[fd].billing_node->used > 0);

	unsigned long long int *flux = (void*)(fdtable[fd].billing_node) + fdtable[fd].type + offsetof(struct flux_connection, read_size);

	*flux = *flux + read_size;

	//总流量计数器没直接在read()、write()的入口处计，是因为有些非socket的流量
	//一定要在bind之后的流量才是有效的，才计入总流量
	read_count += read_size;
}


void   billing_flux_write(int fd, uint32_t write_size)
{
	//当squid操作fd，但fd对应的
	if( fdtable[fd].billing_node == NULL ) {
		fdtable[fd].write_size += write_size;
		return;
	}

	//open的时候，已经fd的流量转到host上了
	assert(fdtable[fd].read_size == 0);
	assert(fdtable[fd].write_size == 0);
	assert(fdtable[fd].billing_node->used > 0);

	unsigned long long int *flux = (void*)(fdtable[fd].billing_node) + fdtable[fd].type + offsetof(struct flux_connection, write_size);

	*flux = *flux + write_size;

	//总流量计数器没直接在read()、write()的入口处计，是因为有些非socket的流量
	//一定要在bind之后的流量才是有效的，才计入总流量
	write_count += write_size;
}


void billing_close(int fd)
{
	if( fdtable[fd].billing_node == NULL ) {	//文件、pipe等非计费(未执行过bind)的流量
		fdtable[fd].read_size = 0;
		fdtable[fd].write_size = 0;
		fdtable[fd].type = -1;
		return;
	}

	//执行过bind，流量已移交到hashtable中
	assert(fdtable[fd].read_size == 0);
	assert(fdtable[fd].write_size == 0);
	assert(fdtable[fd].billing_node->used > 0);



	fdtable[fd].billing_node->used--;
	fdtable[fd].billing_node = NULL;
	fdtable[fd].type = -1;
}


//===========================================================================
//===========================================================================


static void sync_connect()
{
	debug(92,3)("sync_connect enter\n");
	struct sockaddr_in  maddress;
	memset(&maddress, 0, sizeof(struct sockaddr_in));
	inet_aton(billing_host, &maddress.sin_addr);
	maddress.sin_family = AF_INET;
	maddress.sin_port   = htons(billing_port);

	assert(sync_socket == -1);
	assert(sync_socket_flag == -1);

	if( (sync_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		debug(92,0)("[sync_connect] socket creat error\n");
		sync_socket = -1;
		assert(sync_socket_flag == -1);
		return;
	}

#ifndef	TCP_NODELAY
#define TCP_NODELAY	1
#endif
	int nodelay = 1;
	if( setsockopt(sync_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay)) == -1 ) {
		debug(92,0)("[sync_connect] socket setsockopt error\n");
		close(sync_socket);
		sync_socket = -1;
		assert(sync_socket_flag == -1);
		return;
	}

	int socket_status = 0;
	if( (socket_status = fcntl(sync_socket, F_GETFL, 0)) < 0 ) {
		debug(92,0)("[sync_connect] fcntl(sync_socket, F_GETFL, 0) error\n");
		close(sync_socket);
		sync_socket = -1;
		assert(sync_socket_flag == -1);
		return;
	}

	if( fcntl(sync_socket, F_SETFL, socket_status | O_NONBLOCK) != 0 ) {
		debug(92,0)("[sync_connect] fcntl(sync_socket, F_SETFL, socket_status | O_NONBLOCK) error\n");
		close(sync_socket);
		sync_socket = -1;
		assert(sync_socket_flag == -1);
		return;
	}

	int ret;
	if( (ret = connect(sync_socket, (struct sockaddr *)&maddress, sizeof(struct sockaddr))) != 0) {

		if( errno != EINPROGRESS ) { //异步连接出错，重头来
			debug(92,0)("[sync_connect] connect error\n");
			close(sync_socket);
			sync_socket = -1;
			assert(sync_socket_flag == -1);
			return;
		}
	}

	//此时无论是否联接成功，都必须走入check_connect流程，里面要把socket设回非阻塞
		// 0 - connecting, 1 - ready to write/read
	assert(sync_socket);
	sync_socket_flag = 0;
}

static void check_connect()
{
	assert(sync_socket >= 0);
	assert(sync_socket_flag == 0);

	int error;
	socklen_t e_len = sizeof(error);

	if( getsockopt(sync_socket,SOL_SOCKET,SO_ERROR,&error,&e_len) < 0 || error != 0 ) {
		debug(92,0)("[check_connect] getsockopt error\n");
		close(sync_socket);
		sync_socket_flag = -1;
		sync_socket = -1;
		return;
	}


	struct sockaddr_in sin;
	int len = sizeof(struct sockaddr_in);
	if(getsockname(sync_socket,(struct sockaddr *)&sin, (socklen_t *)&len) == 0)
	{
		int port = ntohs(sin.sin_port);
		if(port == billing_port)
		{
			debug(92,0)("[check_connect] getsockname error\n");
			close(sync_socket);
			sync_socket_flag = -1;
			sync_socket = -1;
			return;
		}
	}


/*  原来是非阻塞联接，阻塞写，用select保证超时
    现改为非阻塞联接，非阻塞读写，不用设回阻塞

	int socket_status = 0;
	if( (socket_status = fcntl(sync_socket, F_GETFL, 0)) < 0 ) {
		close(sync_socket);
		sync_socket = -1;
		sync_socket_flag = -1;
		return;
	}

	if( fcntl(sync_socket, F_SETFL, socket_status & (!O_NONBLOCK)) != 0 ) {
		close(sync_socket);
		sync_socket = -1;
		sync_socket_flag = -1;
		return;
	}
*/
	//联接成功
    sync_socket_flag = 1;
}


static int is_entry_empty(struct hash_entry *entry)
{
    if (NULL == entry)
        return -1;
    if (entry->local.client.read_size != 0)
        return -1;
    if (entry->local.client.write_size != 0)
        return -1;
    if (entry->local.source.read_size != 0)
        return -1;
    if (entry->local.source.write_size != 0)
        return -1;
    if (entry->remote.client.read_size != 0)
        return -1;
    if (entry->remote.client.write_size != 0)
        return -1;
    if (entry->remote.source.read_size != 0)
        return -1;
    if (entry->remote.source.write_size != 0)
        return -1;
    if (entry->fc.client.read_size != 0)
        return -1;
    if (entry->fc.client.write_size != 0)
        return -1;
    if (entry->fc.source.read_size != 0)
        return -1;
    if (entry->fc.source.write_size != 0)
        return -1;
    return 0;
}

static void reset_entry(struct hash_entry *entry)
{
    if (NULL == entry)
        return;
    entry->local.client.read_size = 0;
    entry->local.client.write_size = 0;
    entry->local.source.read_size = 0;
    entry->local.source.write_size = 0;
    entry->remote.client.read_size = 0;
    entry->remote.client.write_size = 0;
    entry->remote.source.read_size = 0;
    entry->remote.source.write_size = 0;
    entry->fc.client.read_size = 0;
    entry->fc.client.write_size = 0;
    entry->fc.source.read_size = 0;
    entry->fc.source.write_size = 0;
    entry->local.client.connection_times = 0;
    entry->local.source.connection_times = 0;
    entry->remote.client.connection_times = 0;
    entry->remote.source.connection_times = 0;
    entry->fc.client.connection_times = 0;
    entry->fc.source.connection_times = 0;
}


static void write_buff(struct hash_entry *entry)
{
    // filter the channels configured
    if (!strcmp(dir_config->type,"filter")){
        wordlist *pC = dir_config->channel;
        while (pC)
        {   
            if ( !strncmp(entry->host, pC->key, sizeof(entry->host)) )
                return;
            pC = pC->next;
        }  
    }

    // memBufPrintf called
    memBufPrintf(&billing_send_mb, "%s\t",   entry->host);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->local.client.read_size);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->local.client.write_size);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->local.source.read_size);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->local.source.write_size);
    memBufPrintf(&billing_send_mb, "%u\t",   entry->local.client.connection_times);
    memBufPrintf(&billing_send_mb, "%u\t",   entry->local.source.connection_times);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->remote.client.read_size);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->remote.client.write_size);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->remote.source.read_size);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->remote.source.write_size);
    memBufPrintf(&billing_send_mb, "%u\t",   entry->remote.client.connection_times);
    memBufPrintf(&billing_send_mb, "%u\t",   entry->remote.source.connection_times);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->fc.client.read_size);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->fc.client.write_size);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->fc.source.read_size);
    memBufPrintf(&billing_send_mb, "%llu\t", entry->fc.source.write_size);
    memBufPrintf(&billing_send_mb, "%u\t",   entry->fc.client.connection_times);
    memBufPrintf(&billing_send_mb, "%u\n",   entry->fc.source.connection_times);
}

void build_buff()
{
	assert(billing_send_mb.size == 0);
	assert(current_send_buff == NULL);

	if( hashtable_entry_count == 0 )
		return;


	struct hash_entry **entry_array;
	entry_array = xmalloc(sizeof(struct hash_entry*) * hashtable_entry_count);

//	printf("hashtable_entry_count = %u\n", hashtable_entry_count);
	debug(92,3)("build_buff:hashtable_entry_count = %d \n",hashtable_entry_count);

	get_all_entry(entry_array);

	current_send_buff = billing_send_mb.buf;

	uint32_t i;
	uint32_t entry_count = hashtable_entry_count;
	for( i = 0 ; i < entry_count ; i++ ) 
    {
        if (-1 == is_entry_empty(entry_array[i]))
            write_buff(entry_array[i]);
		if (entry_array[i]->used == 0) 
			hashtable_delete(entry_array[i]);
        else
            reset_entry(entry_array[i]);
	}

	xfree(entry_array);
	//assert(billing_send_mb.size); 
	//assert(current_send_buff);   
    if (0 == billing_send_mb.size)      // buff can be empty
        current_send_buff = NULL;
    else
        assert(current_send_buff);   

}


int send_buff()
{
	assert(sync_socket >= 0);
	assert(sync_socket_flag == 1);
	assert(billing_send_mb.size);
	assert(current_send_buff);


	debug(92,3)("send buffer enter\n");
	ssize_t ret;
	if( (ret = write(sync_socket, current_send_buff, billing_send_mb.size)) < 0 ) {

		debug(92, 3) ("(mod_billing: send_buff(): %s\n",xstrerror());
		if( errno == EINTR )
			return -1;

		if( errno == EAGAIN )
			return -1;

		close(sync_socket);
		sync_socket = -1;
		sync_socket_flag = -1;
		return -1;
	}

	debug(92,5)("billing_left_len =%lu , ret =%ld\n",(long unsigned int)billing_send_mb.size,(long)ret);
	//没有发完
	if( ret > 0 ) {

		current_send_buff += ret;
		billing_send_mb.size -= ret;
	}
	if (billing_send_mb.size > 0)
		return ret;

	debug(92,5)("A:billing_left_len =%lu , ret =%ld\n",(long unsigned int)billing_send_mb.size,(long)ret);

	//buff发送完成
	assert(billing_send_mb.size == 0);
	memBufReset(&billing_send_mb);
	current_send_buff = NULL;

	return ret;
}

int32_t billing_sync(bool donow)
{
	int32_t ret = 0;

	if( (time(0) - last_ok) > 300)
	{
		debug(92,3)("billing_sync error override 300 seconds\n");
	}

	//未联结，则联接
	if( sync_socket_flag == -1 ) {
		assert(sync_socket == -1);	//socket一定是关闭中
		sync_connect();
		ret = 1;
		goto out;
	}

	assert( sync_socket_flag >= 0 );
	assert(sync_socket >= 0);

	//执行过联接，检察联接是否成功
	if( sync_socket_flag == 0 ) {
		assert(sync_socket >= 0);	//socket一定是打开中
		check_connect();
		ret = 2;
		goto out;
	}

	assert(sync_socket_flag == 1);
	assert(sync_socket >= 0);

	//联接正常，有未发送走的数据
	if( billing_send_mb.size != 0 ) {
		if(send_buff() != -1)
		{
			last_ok = time(0);

		}
		ret = 3;
		goto out;
	}


	assert(billing_send_mb.size == 0);
	assert(sync_socket >= 0);
	assert(sync_socket_flag == 1);

	build_buff();
	ret = 4;
	goto out;


	//发送完成，并且没有新的数据了
out:
	return ret;
}

/*
 * case 2954: customize for wasu TV 
 * forbbiden billing serveral channels
 * written by chenqi @ Mar. 29, 2012
*/
void parse_list()
{
    if (NULL == dir_config)
        return;
    if (strcmp(dir_config->type, "filter"))
    {   
        debug(92,1)("mod_billing: Cannot recognize keyword: %s\n",dir_config->type);
        return;
    }  

    FILE *fp = NULL;
    if ( NULL == (fp = fopen(dir_config->black_list_path, "r")) )
    {   
        debug(92,1)("mod_billing: Cannot open file %s\n",dir_config->black_list_path);
        return;
    }   

    char config_input_line[256];
    char *ptr;
    int channel_num = 0;
    while (fgets(config_input_line, BUFSIZ, fp) != NULL)
    {
        // ignore black line
        if ( NULL == (ptr = strtok(config_input_line, w_space)))
            continue;
        wordlistAdd(&dir_config->channel,ptr);
        channel_num++;
    }

    fclose(fp);
    fp = NULL;

    if (!channel_num)
        debug(92,2)("mod_billing: the file %s is empty\n", dir_config->black_list_path);
    else
        debug(92,3)("mod_billing: we have read %d channel from file %s\n", channel_num, dir_config->black_list_path);
    return;
}



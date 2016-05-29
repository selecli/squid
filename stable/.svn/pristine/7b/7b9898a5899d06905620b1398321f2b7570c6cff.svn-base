#ifndef CONN_H
#define CONN_H

#define COMM_OK       (0)
#define COMM_ERR   (-1)
#define COMM_NOMESSAGE   (-3)
#define COMM_TIMEOUT     (-4)
#define COMM_SHUTDOWN    (-5)
#define COMM_INPROGRESS  (-6)
#define COMM_ERR_CONNECT (-7)
#define COMM_ERR_CLOSING (-9)

void * recv_handler(void * args);
int server_init(short server_port, char *ip);
int send_moov_data(int fd, char *moov_data, int data_size);
int moov_data_handler(int fd, struct mp4_req *req);
int create_new_mp4(int start, int end, char *path, char *new_path, int mp4_offset);
int safe_read(int fd,void *buffer,int length);
int set_no_blocking(int fd);
#endif

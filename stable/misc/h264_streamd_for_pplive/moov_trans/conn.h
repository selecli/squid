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

int moov_data_handler(char* obj_path);
#endif

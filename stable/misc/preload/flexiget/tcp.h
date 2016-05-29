#ifndef __TCP_H
#define __TCP_H



int tcp_connect( char *hostname, int port, char *local_if );
int get_if_ip( char *iface, char *ip );


#endif


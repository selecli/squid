#include "flexiget.h"

/* Get a TCP connection */
int tcp_connect( char *hostname, int port, char *local_if )
{
	struct hostent *host = NULL;
	struct sockaddr_in addr;
	struct sockaddr_in local;
	int fd;

#ifdef DEBUG
	socklen_t i = sizeof( local );

	fprintf( stderr, "tcp_connect( %s, %i ) = ", hostname, port );
#endif
	//fprintf( stderr, "Xcell:Debug: tcp_connect( %s, %i ) = \n", hostname, port );

	/* Why this loop? Because the call might return an empty record.
	   At least it very rarely does, on my system...	
	FIXME: 	to Skip DNS when ip is numberic
	*/
	int j = 0;
	for( j = 0; j < 5; j++ ) {
		debug("Lookup DNS for %s! %d times\n", hostname, j);
		if( ( host = gethostbyname( hostname ) ) == NULL )
			return -1 ;
		if( *host->h_name ) 
			break;
	}

	if( !host || !host->h_name || !*host->h_name )
		return( -1 );

	if( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
		return( -1 );

	local.sin_family = AF_INET;
	local.sin_port = 0;
	local.sin_addr.s_addr = htonl(INADDR_ANY);;
	if( bind( fd, (struct sockaddr *) &local, sizeof( struct sockaddr_in ) ) == -1 )
	{
		close( fd );
		return( -1 );
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons( port );
	addr.sin_addr = *( (struct in_addr *) host->h_addr );

	time_t now = time(NULL);
retry:
	//FIXME: mulit thread but only one clock
	//alarm(30);
	if ( connect( fd, (struct sockaddr *) &addr, sizeof( struct sockaddr_in ) ) == -1 ) {
		
		if (time(NULL) - now < 10) {
			sleep(1);
			fprintf( stderr, "connect %s failed, try again after 1 second!\n", hostname);
			goto retry;
		}
		close(fd);
		fprintf( stderr, "connectting to %s failded!\n", hostname);
		return -1;
	} else {
		//alarm(0);
	} 

#ifdef DEBUG
	getsockname( fd, &local, &i );
	fprintf( stderr, "%i\n", ntohs( local.sin_port ) );
#endif
	fprintf(stderr, "%d-----------------------------------\n", fd);
	return( fd );
}

int get_if_ip( char *iface, char *ip )
{
	struct ifreq ifr;
	int fd = socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );

	memset( &ifr, 0, sizeof( struct ifreq ) );

	strcpy( ifr.ifr_name, iface );
	ifr.ifr_addr.sa_family = AF_INET;
	if( ioctl( fd, SIOCGIFADDR, &ifr ) == 0 )
	{
		struct sockaddr_in *x = (struct sockaddr_in *) &ifr.ifr_addr;
		strcpy( ip, inet_ntoa( x->sin_addr ) );
		return( 1 );
	}
	else
	{
		return( 0 );
	}
}

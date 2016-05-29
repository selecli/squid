#include "xping.h"

static pid_t pid;

static double tv_sub(struct timeval *tv0, struct timeval *tv1)
{   
	return (tv0->tv_sec - tv1->tv_sec) * 1000.0 + (tv0->tv_usec - tv1->tv_usec) / 1000.0;
}

static unsigned short cal_chksum(unsigned short *addr,int len)
{       
	int sum = 0;
	int nleft = len;
	unsigned short *w = addr;
	unsigned short answer = 0;

	while(nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}
	if(1 == nleft)
	{   
		*(unsigned char *)(&answer) = *(unsigned char *)w;
		sum += answer;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;

	return answer;
}

static int pack(int pack_no, char *sendpacket, int psize)
{
	int packsize;
	struct icmp *icmp;
	struct timeval *tval;

	icmp = (struct icmp*)sendpacket;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_cksum = 0;
	icmp->icmp_seq = pack_no;
	icmp->icmp_id = pid;
	packsize = 8 + psize;
	tval = (struct timeval *)icmp->icmp_data;
	gettimeofday(tval, NULL);
	icmp->icmp_cksum = cal_chksum((unsigned short *)icmp, packsize);

	return packsize;
}

static int unpack(char *buf, int len, struct sockaddr_in *from, double *rtt)
{
	int iphdrlen;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;
	struct timeval tvrecv;

	ip = (struct ip *)buf;
	iphdrlen = ip->ip_hl << 2;
	icmp = (struct icmp *)(buf + iphdrlen);
	len -= iphdrlen;
	if(len < 8)
	{  
		printf("ICMP packets\'s length is less than 8\n");
		return FAILED;
	}
	if(ICMP_ECHOREPLY == icmp->icmp_type && icmp->icmp_id == pid)
	{  
		gettimeofday(&tvrecv, NULL);
		tvsend = (struct timeval *)icmp->icmp_data;
		*rtt += tv_sub(&tvrecv, tvsend);
		/*
		   printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n",
		   len,
		   inet_ntoa(from->sin_addr),
		   icmp->icmp_seq,
		   ip->ip_ttl,
		 *rtt);
		 */
		return SUCC;
	}

	return FAILED;
}

static int send_packet(const int sockfd, const int npack, unsigned long itime, int size, struct sockaddr *peeraddr)
{    
	ssize_t ret;
	size_t offset, psize;
	int n, count, nsend;
	char sendpacket[BUF_SIZE_4096];

	nsend = 0;
	n = npack;
	while(n-- > 0)
	{       
		psize = pack(nsend, sendpacket, size);
		count = 0;
		offset = 0;
		do {
			ret = sendto(sockfd, sendpacket + offset, psize, 0, (void *)peeraddr, sizeof(*peeraddr));
			if (ret < 0)
			{
				if (EINTR == errno)
				{
					usleep(1);
					continue;
				}
				xlog(ERROR, "sendto() failed, ERROR=[%s]", strerror(errno));
				break;
			}
			offset += ret;
			psize -= ret;
			if (psize <= 0)
			{
				++nsend;
				break;
			}
		} while (++count < 5);
		usleep(itime);
	}

	return nsend;
}

static int checkTimeout(struct timeval *tv)
{
	double usedtime;
	struct timeval curtv;

	gettimeofday(&curtv, NULL);
	usedtime = (curtv.tv_sec - tv->tv_sec) * 1000000 + (curtv.tv_usec - tv->tv_usec);

	return (usedtime > ICMP_TIMEOUT) ? TRUE : FALSE;
}

static int recv_packet(const int sockfd, const int npack, double *rtt)
{      
	socklen_t fromlen;
	struct timeval tvstart;
	struct sockaddr_in from;
	int ret, count, nreceived = 0;
	char recvpacket[BUF_SIZE_4096];

	fromlen = sizeof(from);
	gettimeofday(&tvstart,NULL);
	do {      
		if ((nreceived == npack) || (TRUE == checkTimeout(&tvstart)))
			break;
		count = 0;
		do {
			ret = recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0, (struct sockaddr *)&from, &fromlen);
			if (ret >= 0)
			{
				if(FAILED == unpack(recvpacket, ret, &from, rtt))
					continue;
				nreceived++;
				break;	
			}
			if(EAGAIN == errno || EINTR == errno)
			{
				usleep(1);
				continue;
			}
			xlog(ERROR, "recvfrom() failed, ERROR=[%s]", strerror(errno));
			break;
		} while (++count < 5);
	} while (1);

	return nreceived;
}

static int setSockfdNonblock(const int sockfd) 
{
	int flags;

	flags = fcntl(sockfd, F_GETFL);
	if (flags < 0)
	{   
		xlog(WARNING, "fcntl(F_GETFL) failed, SOCKFD=[%d], ERROR=[%s]", sockfd, strerror(errno));
		return FAILED;
	}       
	if (fcntl(sockfd, F_SETFL, flags|O_NONBLOCK) < 0)
	{   
		xlog(WARNING, "fcntl(F_SETFL) failed, SOCKFD=[%d], ERROR=[%s]", sockfd, strerror(errno));

		return FAILED;
	}       
	return SUCC;
}

int xpingIcmp(ping_packet_t *pack)
{
	int size = 50 * 1024;   //50KB
	int sockfd, nsend, nrecv;
	struct protoent *protocol;
	struct sockaddr_in peeraddr;
	int npack, psize;
	double itime;

	pid = getpid();
	protocol = getprotobyname("icmp");
	if (NULL == protocol)
	{
		xlog(WARNING, "getprotobyname() failed, ERROR=[%s]", strerror(errno));
		return FAILED;
	}
	sockfd = socket(AF_INET, SOCK_RAW, protocol->p_proto);
	if (sockfd < 0)
	{ 
		xlog(WARNING, "socket() failed, ERROR=[%s]", strerror(errno));
		return FAILED;
	}
	setSockfdNonblock(sockfd);
	setuid(getuid());
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	memset(&peeraddr, 0, sizeof(peeraddr));
	peeraddr.sin_family = AF_INET;
	if (inet_pton(AF_INET, pack->ip, (void *)&peeraddr.sin_addr) <= 0)
	{
		xlog(WARNING, "inet_pton(%s) failed, ERROR=[address invalid]", pack->ip);
		return FAILED;
	}
	/*
	   printf("PING %s(%s): %d bytes data in ICMP packets.\n",
	   pack->ip, inet_ntoa(peeraddr.sin_addr), pack->psize);
	 */
	npack = (pack->npack > ICMP_MAX_PNUM) ? ICMP_MAX_PNUM : pack->npack;
	psize = (pack->psize > ICMP_MAX_PSIZE) ? ICMP_MAX_PSIZE : pack->psize;
	itime = (pack->itime > ICMP_MAX_ITIME) ? ICMP_MAX_ITIME : pack->itime;
	nsend = send_packet(sockfd, npack, itime, psize, (void *)&peeraddr);
	nrecv = recv_packet(sockfd, nsend, &pack->rtt);
	pack->lossrate = (nsend - nrecv) / nsend * 100;
	if (nrecv > 0)
		pack->rtt /= nrecv;
	close(sockfd);

	return SUCC;
}


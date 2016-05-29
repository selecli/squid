#Version 1.5
#This is for CPIS PBL dns server change
#it's based on mantis 0000962
#it's associated with /var/named/anyhost /var/named/chroot/etc/named.conf
#author:yong.huang@chinacache.com

LOCAL_PATH=/home/huangyong/work/named.conf.CMN-NJ-2-35Q
ORIGIN_PATH=/home/huangyong/work/named.conf.CMN-KM-1-351

BACKUP_LDNS=/var/named/chroot/etc/named.conf.LDNS
DEFAULT_FILE=/var/named/chroot/etc/named.conf
BACKUP_DEFUAULT=/var/named/chroot/etc/named.conf.bak

ANYHOST_FILE=/var/named/anyhost
IP_STS_FILE=/tmp/ip.list
IP_STS_FILE_UNIQ=/tmp/iplist.uniq

LDNS_STS_FILE=/tmp/dns.stat1
DNS_STS_FILE=/tmp/dns.stat2

#this function used to change LDNS
file()
{
    echo "0">$DNS_STS_FILE
	
	if [ ! -e $LDNS_STS_FILE ];then
		echo "0">$LDNS_STS_FILE
	fi
	
	lastmode_of_LDNS=`cat $LDNS_STS_FILE | awk '{print $1}'`
	if [ $lastmode_of_LDNS -eq 1 ];then
      exit 1
    fi
	
	if [ ! -e $BACKUP_LDNS ];then
       cp $LOCAL_PATH $BACKUP_LDNS
    fi

	if [ ! -e $BACKUP_DEFUAULT ];then
		cp $ORIGIN_PATH $BACKUP_DEFUAULT
	fi
	
    cp $BACKUP_LDNS $DEFAULT_FILE
	
	if [ $lastmode_of_LDNS -eq 0 ];then
	   service named restart
	   lastmode_of_LDNS=1
	   echo "$lastmode_of_LDNS">$LDNS_STS_FILE
    fi
}


#this function used to change to anyhost,back to default
unfile()
{
   echo "0">$LDNS_STS_FILE
   
   if [ ! -e $DNS_STS_FILE ];then
	   echo "0">$DNS_STS_FIL
   fi
   
   lastmode_of_Default=`cat $DNS_STS_FILE | awk '{print $1}'`
   if [ $lastmode_of_Default -eq 1 ];then
      exit 1
   fi
	  
   if [ ! -e $BACKUP_DEFUAULT ];then
       cp $ORIGIN_PATH $BACKUP_DEFUAULT
   fi

   if [ ! -e $BACKUP_LDNS ];then
	   cp $LOCAL_PATH $BACKUP_LDNS
   fi

   cp $BACKUP_DEFUAULT $DEFAULT_FILE 

   if [ $lastmode_of_Default -eq 0 ];then
	   service named restart
	   lastmode_of_Default=1
	   echo "$lastmode_of_Default">$DNS_STS_FILE
   fi
}
>$IP_STS_FILE
if [ ! -e $ANYHOST_FILE ];then
  file
  exit 1
fi

size_anyhost=`wc -c $ANYHOST_FILE | awk '{print $1}'` 
if [ $size_anyhost -eq 0 ];then
  file
  exit 1
fi

while read LINE
do
  IFS=' '
  ip=($LINE)
  if [[ ${ip[3]} =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]];then
	  echo ${ip[7]} >>$IP_STS_FILE
  fi
done < $ANYHOST_FILE

sort $IP_STS_FILE | uniq > $IP_STS_FILE_UNIQ

count=`wc -l $IP_STS_FILE_UNIQ |awk '{print $1}'`  
case $count in
	0)
	file
	exit 1;;
	1)
	stat=`cat $IP_STS_FILE_UNIQ`
	if [ $stat = "ip_down" ]
	then
		file
	elif [ $stat = "ip_work" ]
	then
		unfile
	else
        unfile
	fi;;
	2)
	unfile;;
	*)
	unfile
esac

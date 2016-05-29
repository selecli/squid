#!/bin/bash
# Author: zhaobin@chinacache.com
# Function:
#     1 download the task list
#     2 download every item in the list && execute && report the status
# History:
#     2013/06/13   zhaobin  create

######################### functions ##########################

# brief:
#     report status to the server
# params:
#     $1    task sn
#     $2    status code  
#                0 represents that wget is failed 
#                1 represents that  only wget is success
#                2 represents the job is done but failed 
#                3 represents the job is done successfully
# return:
#     null
reportStatus()
{
    logMsg "task: $1 status:$2"
    doGet "${reportURL}?info=$1_${devSN}__$2"
}

# brief:
#     log message to logfile
# params:
#     $1    message
# return:
#     null
logMsg()
{
    echo $1 >> $logFile
}

# brief:
#     get url to specific file
# params:
#     $1    url
#     $2    file which will store the content
# return:
#     0          success
#     others     fail
doGet()
{
    local ret1=1
    local url=$1
    local target=$2
    if [ -z "$target" ]
    then
        target=/dev/null
    fi
    local domain=$(echo "$url" | sed 's/[^\/]*\/\///;s/\/.*//;s/:.*//;')
    local flag=$(echo "$domain" | perl -ane 'if(/\b(?:\d{1,3}\.){3}\d{1,3}\b/){print "0\n";}else {print "1\n";}')
    local port_and_path=$(echo "$url" | sed 's/[^\//]\/\///;s/[^:]*//;')
  
    if [ $flag -eq "1" ]
    then
        while read server 
        do
            ip=$(dig -t A $domain @$server +noall +answer | perl -ane 'if($F[3] eq "A"){print "$F[-1]\n";}')
            while read i
                         do
							  if [ -z "$i" ]
							  then
								  break
							  fi
                              newURL="http://${i}${port_and_path}"
                              wget -t 3 -T 5 -q -O $target "$newURL"
                              if [ $? -ne 0 ]
                              then
                                  ret1=1
                                  logMsg "download task list fail, url:$newURL"
                                  [ "$targe" != "/dev/null" ] && rm -rf $target
                              else
                                  ret1=0
                                  break
                              fi
                         done< <(echo $ip)
            if [ "$target" != "/dev/null" ] && [ -e "$target" ]
            then
                break
            fi
        done < <(awk '/^#/{next}/^robot_dns/{split($NF,a,"|" );for(i in a)print a[i];}' $CFG)
    else
        wget -t 3 -T 5 -q -O $target "$url"
        if [ $? -ne 0 ]
        then
            ret1=1
            logMsg "download task list fail, url:$url"
            [ "$targe" != "/dev/null" ] && rm -rf $target
        fi
    fi
    if [ $ret1 -ne 0 ]
    then
        newURL="http://61.135.209.134${port_and_path}"
        wget -t 3 -T 5 -q -O $target "$newURL"
        ret1=$?
    fi

    return $ret1
}

############################ main ############################
workDir=/usr/local/automation
taskFile=task_list.tgz
logFile=task.log
taskSNFile=/usr/local/etc/task_sn.conf
taskListFile="taskList.$$"
VERSION="version robot 1.0.0"

# check if it only need print version infomation
if [[ -n $1 ]] && [[ $1 = "-v" || $1 = "-V" ]]
then
    echo "$VERSION"
    exit 0
fi

# make sure only one process at one time
/usr/local/squid/bin/crond_pid.py $0 $$ 
if [ $? == 1 ]
then
    exit 0
fi

# backup squid.conf if need
SQUIDConfFile="/usr/local/squid/etc/squid.conf"
SQUIDConfFileBak="/usr/local/squid/etc/squid.conf.robot.bak"
if [ -e "$SQUIDConfFileBak" ]
then
    md5val=$(md5sum $SQUIDConfFileBak)
    md5now=$(md5sum $SQUIDConfFile)
    if [ "$md5val" != "$md5now" ]
    then  
        /bin/cp -f $SQUIDConfFile $SQUIDConfFileBak
    fi
else
    /bin/cp -f $SQUIDConfFile $SQUIDConfFileBak
fi

# init taskURL & reportURL
CFG=/usr/local/squid/etc/fcexternal.conf
hostISP=$(hostname | awk 'BEGIN{FS="-"}{print $1}')
if [ "$hostISP" == "CNC" ]
then
    taskURL=$(awk '/^ *CncTaskUrl/{print $2}' $CFG 2>/dev/null)
    reportURL=$(awk '/^ *CncPostUrl/{print $2}' $CFG 2>/dev/null)
elif [ "$hostISP" == "CHN" ]
then
    taskURL=$(awk '/^ *ChnTaskUrl/{print $2}' $CFG 2>/dev/null)
    reportURL=$(awk '/^ *ChnPostUrl/{print $2}' $CFG 2>/dev/null)
else
    taskURL=$(awk '/^ *DefTaskUrl/{print $2}' $CFG 2>/dev/null)
    reportURL=$(awk '/^ *DefPostUrl/{print $2}' $CFG 2>/dev/null)
fi
# check 
if [ -z $taskURL -o -z $reportURL ]
then
    echo "conf file error need task_url and report_url" > $logFile
    exit 1
fi

devSN=$([ -f /sn.txt ] && cat /sn.txt || echo "")
if [ -z $devSN ]
then
    echo "need /sn.txt" >> $logFile
    exit 1
fi

# clear log
echo "" > $logFile
# download task list
cd $workDir
rm -rf *
# if taskURL is hostname we need translate this domain to ip
doGet $taskURL $taskFile
ret=$?
if [ $ret -ne 0 ]
then
    exit 1
fi
# decompress
tar -zxvf $taskFile

# extract item we need from task list
if [ -f "$taskSNFile" ]
then
    taskSN=$(cat "$taskSNFile" 2>/dev/null)
    awk -v sn=$devSN -v taskSN="${taskSN}A" 'BEGIN{FS="__"}/^#/{next;}{if($3 == sn && $2"A" > taskSN){print;}}' task_list.txt | sort -d > "$taskListFile"
else
    awk -v sn=$devSN 'BEGIN{FS="__"}/^#/{next;}{if($3 == sn){print;}}' task_list.txt | sort -d > "$taskListFile"
fi

# process each item at one time
while read item
do
    taskURL=$(echo "$item" | awk 'BEGIN{FS="__"}{print $4}')
    taskSN=$(echo "$item" | awk 'BEGIN{FS="__"}{print $2}')
    taskFile=$(echo "$taskURL" | sed 's/.*\///') 
    taskFileCheckSum="$taskFile.txt"
    tryCnt=0
    if [ -z "$taskURL" -o -z "$taskSN" ]
    then
        logMsg "error item:$item"
        continue
    fi

    while [ $tryCnt -lt 3 ]
    do
        # download item && verify
        rm -rf "$taskFile" "${taskFile}.txt"
        rm -rf ${workDir}/${taskSN}
        mkdir -p  ${workDir}/${taskSN} 
        cd ${workDir}/${taskSN}
        doGet "$taskURL" "$taskFile"
        if [ $? -ne 0 ]
        then
            ((tryCnt++))
            continue
        fi
        doGet "${taskURL}.txt" "$taskFileCheckSum"
        if [ $? -ne 0 ]
        then
            ((tryCnt++))
            continue
        fi
        break
    done
        
    # verify md5sum
    flag=$(diff -b <(md5sum -b $taskFile) <(cat $taskFileCheckSum))
    if [ -z "$flag" ]
    then
        reportStatus $taskSN 1
        echo "$taskSN" > $taskSNFile 
        # execute the item if success
        tar -zxvf "$taskFile"
        tmp=${taskFile%.tgz}
        cd $tmp
        for file in `ls -1 *.sh`
        do
            bash $file
            logMsg "execute $file result:$?"
        done
        if [ -f success ]
        then
            reportStatus $taskSN 3
        else
            reportStatus $taskSN 2
        fi
        break
    else
        reportStatus $taskSN 0
    fi

done < "$taskListFile"


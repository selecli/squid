#!/bin/sh

squid_bin=/usr/local/squid/bin
warning_log_def="/data/proclog/log/squid/receive_warning.log"
warning_log_temp=`cat /usr/local/squid/etc/squid.conf|awk '{if($1 ~ /mod_content_length_warning/) print $2}' |awk -F '=' '{if($1 ~ /receive_log_path/) print $2}'`


if [ -e ${warning_log_temp} ] ;then
	warning_log=${warning_log_temp}	
elif [ -e ${warning_log_def} ] ;then
	warning_log=${warning_log_def}
else
	echo "Can't find the warning_log path!!!!"	
	exit
fi

echo ${warning_log}

warning_log_dir=`echo $warning_log |sed 's/\/receive_warning.log$//g' `


echo "Dir:${warning_log_dir}"

warning_log_size=`stat -c %s $warning_log`
echo "file_size:$warning_log_size"

if [[ $warning_log_size -le 0 ]] ;then
	echo "The receive_warning.log size is zero,No need to to the check"
	exit 
fi


if [  !  -d  $warning_log_dir/warning_back ] ;then
	echo "Create the warning_back dir"
	mkdir $warning_log_dir/warning_back
	chown -R squid:squid $warning_log_dir/warning_back
fi


if [  !  -d  $warning_log_dir/check_back ] ;then
	echo "Create the check_back dir"
	mkdir $warning_log_dir/check_back
	chown -R squid:squid $warning_log_dir/check_back
fi

warning_log_back=$warning_log_dir/receive_warning_back.log

cat /dev/null >$warning_log_back
cp $warning_log $warning_log_back
cat /dev/null >$warning_log

cur_time=`date +%Y-%m-%d-%R|sed 's/:/-/'`
cp $warning_log_back $warning_log_dir/warning_back/$cur_time


echo "Start!!!!"


str=`cat ${warning_log_back} |awk -F ',' '{print $4 $5}'|awk -F ': | ' '{if($3 ~ /User-Agent/) if($4 ~ /curl.*/) print $2;else if($4 ~ /Wget.*/) print $2;else if($4 ~ /Opera.*/) print $2; else if($4 ~ /Mozilla.*/) print $2; else if($4 ~ /.*cURL.*/) print $2;}'|uniq`

echo "Url list: $str"

cat  /dev/null >$warning_log_dir/check_result

if [ ! -e $squid_bin/refresh_cli ] ;then 			
	echo "Can't find refresh_cli program">>$warning_log_dir/check_result
	
fi


for i in $str
do
	response_content_length=-1
	response_status=-1
	real_size=-1
	rm -rf $warning_log_dir/response $warning_log_dir/header $warning_log_dir/header2 
	`curl -svo $warning_log_dir/response --dump-header $warning_log_dir/header -x 127.0.0.1:80 $i|awk '{print $0}'`
#	echo "deposit the header!!!!!!!!!"

	#`sed  -i 's/\r/\n/g' ./header > ./header2`
	#`sed  's/\r//g' ./header >./header2`
	
	if [ ! -e $warning_log_dir/header ] ;then
		echo "squid Error"
		echo "Squid Error" >>$warning_log_dir/check_result
		break
	fi

	`tr -d '\r'<$warning_log_dir/header > $warning_log_dir/header2`	
	#`tr -s '\r' '\n' <./header >./header2`

	if [ ! `cat $warning_log_dir/header2|awk '{if($1 ~ /Content-Length:/) print $2;}'` ] ;then
			echo "Response Header haven't Content-Length:!!!!!!"
			echo "Response Header haven't Content-Length! URL:$i" >>$warning_log_dir/check_result
			continue
	fi

	response_content_length=`cat $warning_log_dir/header2|awk '{if($1 ~ /Content-Length:/) print $2;}'`


	if [ ! `cat $warning_log_dir/header2|awk '{if($1 ~ /HTTP\/.*/) print $2;}'` ] ;then
			echo "Response Header haven't Response Status_code:!"
			echo "Response Header haven't Response Status_code! URL:$i" >>$warning_log_dir/check_result
			continue
	fi
	response_status=`cat $warning_log_dir/header2|awk '{if($1 ~ /HTTP\/.*/) print $2;}'`

	if [ ! -e $warning_log_dir/response ] ;then
			$squid_bin/refresh_cli -f $i
			if [ $response_content_length -eq 0 ] ;then
			    echo "Warning Haven't Response Body, Content_length:0 URL:$i">>$warning_log_dir/check_result		
			    continue
			fi
			echo "Response Body haven't"
			echo "Warning Havn't Response Body, Content_length:$response_content_length URL:$i" >>$warning_log_dir/check_result
			continue
	fi
	real_size=`stat -c %s $warning_log_dir/response`

	echo "response_content_length:${response_content_length} real_size:${real_size}"

	if [[ $response_status -gt 400 ]] ;then
#		$squid_bin/refresh_cli -f $i
		echo "Request Filed, Response_code:$response_status"
		echo  -e "Request Failed, Response_code:$response_status, URL:$i" >>$warning_log_dir/check_result
	elif [[ $response_content_length  != ${real_size} ]] ;then
		$squid_bin/refresh_cli -f $i
		echo "Response Don't Complete "
	echo "Response Doesn't corresponed, Content_length:$response_content_length real_size:$real_size URL:$i" >> $warning_log_dir/check_result

	else
		echo "Response Complete Equal"
		echo  -e "Request Success,URL:$i ">>$warning_log_dir/check_result
	fi

done

result_file_sz=`stat -c %s $warning_log_dir/check_result`

if [[ $result_file_sz  -gt 0  && -d $warning_log_dir/check_back  ]] ;then
	echo "need move check_result to check_back!!"
#	echo -e "\n\n" >> $warning_log_dir/check_back/check_result
#	echo "Check Start $cur_time ------------------------->>" >>$warning_log_dir/check_back/check_result
	cp $warning_log_dir/check_result $warning_log_dir/check_back/check_$cur_time	
#	cat $warning_log_dir/check_result  >> $warning_log_dir/check_back/check_result
#	echo "Check End   $cur_time ------------------------->>" >>$warning_log_dir/check_back/check_result
	exit
fi



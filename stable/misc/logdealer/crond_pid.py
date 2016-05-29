#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Writtern by GSP/chenqi @2013/01/16

# 在每个要加入到crontab里面的shell脚本最开始的位置加入下面2行：
# 该脚本会自动检测是否已经有一个实例在运行
# /usr/local/squid/bin/crond_pid.py $0 $$ 
# if [ $? == 1 ] ;then exit 0;fi

import os,sys
script = os.path.basename(sys.argv[1])
pid_path = "/var/run/" +script[:-3] +".pid"

def write_pid_file():
	file_to_write = open(pid_path, "w")
	file_to_write.write(sys.argv[2])
	file_to_write.flush()
	file_to_write.close()


def is_an_instance_running():
	if not os.path.exists(pid_path):
		return False

	file_to_read = open(pid_path,"r")
	pid_num = file_to_read.readline()
	file_to_read.close()
	check_path = "/proc/"+pid_num+"/cmdline"
	if not os.path.exists(check_path):
		return False
	check_file=open(check_path)
	cmd = check_file.readline()
	check_file.close()
	if -1 != cmd.find(script):
		return True
	else:
		return False
	

if is_an_instance_running():
	sys.exit(1)
else:
	write_pid_file()
	sys.exit(0)




#!/bin/bash

function start_proc()
{
	if [ $# -ne 1 ]; then 
		echo "use start_proc proc";
		exit -1
	fi
	
	./$1
	
	pid=`ps -ef |grep $1$|grep -v tail|grep -v grep|awk '{print $2;}'`
	if [ -z "$pid" ]; then
	        echo "start failed !"
	        exit
	fi
	echo "$1 pid: $pid"
	rm _manual_stop_ > /dev/null 2>&1
}


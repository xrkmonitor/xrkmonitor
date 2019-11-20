#!/bin/bash

function stop_proc()
{
	if [ $# -ne 1 ] ; then
		echo "use stop_proc proc";
		exit -1;
	fi

	touch _manual_stop_

	proc=$1
	count=0
	while [ $count -lt 10 ]
	do
		pname=`ps -ef |grep $proc$|grep -v tail|grep -v grep|awk '{print $8;}'`
		pid=`ps -ef |grep $proc$|grep -v tail|grep -v grep|awk '{print $2;}'`
		if [ -z "$pid"  ]; then
			echo "stop ok -- loop try:$count"
			exit
		fi
		for id in $pid
		do
			echo "stop $pname pid:$id ..."
			if [ $count -gt 2 ]; then
				kill -9 $pid > /dev/null 2>&1
			else
				kill -s 10 $pid > /dev/null 2>&1
			fi
		done
		usleep 50000
		count=`expr $count + 1`
	done
	echo "stop failed !"
}


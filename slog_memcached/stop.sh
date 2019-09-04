#!/bin/bash

proc=slog_memcached 
count=0
while [ $count -lt 10 ]
do
	pname=`ps -ef |grep $proc|grep -v tail|grep -v grep|awk '{print $8;}'`
	pid=`ps -ef |grep $proc|grep -v tail|grep -v grep|awk '{print $2;}'`
	if [ -z "$pid"  ]; then
		echo "stop ok -- loop try:$count"
		touch _manual_stop_
		exit
	fi
	for id in $pid
	do
		echo "stop $pname pid:$id ..."
		if [ $count -gt 0 ]; then
			kill -9 $pid
		else
			kill $pid
		fi
	done
	sleep 1
	count=`expr $count + 1`
done
echo "stop failed !"


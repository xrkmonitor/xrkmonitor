#!/bin/bash

proc=slog_mtreport_client
count=0
while [ $count -lt 10 ]
do
	pid=`ps -ef |grep $proc|grep -v tail|grep -v grep|awk '{print $2;}'`
	if [ -z "$pid"  ]; then
		echo "stop ok -- loop try:$count"
		touch _manual_stop_
		exit
	fi
	for id in $pid
	do
		echo "stop $proc pid:$id ..."
		if [ $count -gt 2 ]; then
			kill -9 $pid > /dev/null 2>&1
		else
			kill -s 15 $pid > /dev/null 2>&1
		fi
	done
	sleep 1
	count=`expr $count + 1`
done
echo "stop failed !"


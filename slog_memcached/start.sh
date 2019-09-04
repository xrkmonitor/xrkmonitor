#!/bin/bash

username=mtreport
if [ $# -eq 1 ];then
	username=$1
fi

if [ ! -f slog_memcached ]; then
	cp /usr/bin/memcached slog_memcached
fi

if [ ! -f slog_memcached ]; then
	echo "not find slog_memcached, check file:/usr/bin/memcached exist"
	exit -1
fi

pid=`ps -ef |grep slog_memcached|grep -v tail|grep -v grep|awk '{print $2;}'`
if [ ! -z "$pid" ];then
	echo  "slog_memcached already start:$pid"
	exit 0
fi

./slog_memcached -d -u $username -l 0.0.0.0 -p 12121 -m 2048

pid=`ps -ef |grep slog_memcached|grep -v tail|grep -v grep|awk '{print $2;}'`
if [ -z "$pid" ]; then
	echo "start failed !"
	exit
fi
echo "slog_memcached pid: $pid"
rm _manual_stop_ > /dev/null 2>&1


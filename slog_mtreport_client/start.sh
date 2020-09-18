#!/bin/bash

plugdir=plugin_install_log
proc=slog_mtreport_client
rm _manual_stop_ > /dev/null 2>&1

Count=`pgrep -f $proc -l |grep slog_mtreport|wc -l`
if [ $Count -gt 0 ]; then
	echo "already start"
	exit 0
fi

./$proc
pid=`ps -ef |grep "\./$proc"|grep -v tail|grep -v grep|awk '{print $2;}'`
if [ -z "$pid" ]; then
	echo "start failed !"
	exit
fi
echo "start ok pid: "
echo "$pid"


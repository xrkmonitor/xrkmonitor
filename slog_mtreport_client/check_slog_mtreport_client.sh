#!/bin/bash

shdir=`dirname $0`
cd ${shdir}

# check 下是否手动 stop 的
if [ -f _manual_stop_ ] ; then 
	exit;
fi

NeedCount=2
Count=`pgrep -f slog_mtreport_client -l |grep "^slog_mtreport"|wc -l`
if [ $Count -lt $NeedCount ] ; then 
	./start.sh > /dev/null 2>&1
fi


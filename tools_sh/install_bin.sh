#!/bin/bash
./stop_all.sh
cd ..
tar -zxf slog_all_bin.tar.gz
tar -xf slog_all_bin.tar
md5check=`md5sum slog_config/slog_config`
grep $md5check slog_all_bin.md5
if [ $? -ne 0 ];then
	echo "md5 check failed !"
	exit 1
fi
cd tools_sh
./check_proc_monitor.sh 1

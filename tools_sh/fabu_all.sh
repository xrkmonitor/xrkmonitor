#!/bin/bash
server_list=('122.112.198.251')

tmps=`md5sum slog_all_bin.tar.gz`
lmd5=`echo ${tmps} | awk '{print $1}'`
tmps=`md5sum slog_cgi_bin.tar.gz`
lmd5_cgi=`echo ${tmps} | awk '{print $1}'`

function isRemoteFileExist()
{
	remoteip=$1
	file=$2
	bRet=`ssh -p 13306 mtreport@${remoteip} " if [ -f ${file} ]; then echo 1; else echo 0; fi"`
	return $bRet
}

for s in ${server_list[@]}; do
	echo "update $s - all bin"
	isRemoteFileExist $s /home/mtreport/remote/slog_all_bin.tar.gz
	if [ $? -eq 1 ]; then
		echo "ssh -p 13306 -l mtreport $s \"cd /home/mtreport/remote; md5sum slog_all_bin.tar.gz\""
		tmps=`ssh -p 13306 -l mtreport $s "cd /home/mtreport/remote; md5sum slog_all_bin.tar.gz"`
		rmd5=`echo ${tmps} | awk '{ print $1}'`
	else
		rmd5='null'
	fi

	if [ ${lmd5} == ${rmd5} ]; then
		echo "md5 is same "
		continue
	fi

	isRemoteFileExist $s /srv/www/cgi-bin/mt_slog_showview
	bneed_cgi=$?
	echo "need cgi: $bneed_cgi"

	if [ $bneed_cgi -eq 1 ]; then
		isRemoteFileExist $s /home/mtreport/remote/slog_cgi_bin.tar.gz
		if [ $? -eq 1 ]; then
			echo "ssh -p 13306 -l mtreport $s \"cd /home/mtreport/remote; md5sum slog_cgi_bin.tar.gz\""
			tmps=`ssh -p 13306 -l mtreport $s "cd /home/mtreport/remote; md5sum slog_cgi_bin.tar.gz"`
			rmd5_cgi=`echo ${tmps} | awk '{ print $1}'`
		else 
			rmd5_cgi='nul'
		fi

		if [ ${lmd5_cgi} != ${rmd5_cgi} ]; then
			echo "scp -P 13306 slog_cgi_bin.tar.gz mtreport@$s:/home/mtreport/remote"
			scp -P 13306 slog_cgi_bin.tar.gz mtreport@$s:/home/mtreport/remote
		fi
	fi
	
	echo "scp -P 13306 slog_all_bin.tar.gz mtreport@$s:/home/mtreport/remote"
	scp -P 13306 slog_all_bin.tar.gz mtreport@$s:/home/mtreport/remote

	isRemoteFileExist $s /srv/www/cgi-bin/mt_slog
	bneed_log_cgi=$?

#全部进程退出
	i=0
	while [ $i -lt 10 ]
	do
		echo "ssh -p 13306 mtreport@$s \"cd /home/mtreport/tools_sh; ./stop_all.sh > /dev/null 2>1&\""
		ssh -p 13306 mtreport@$s "cd /home/mtreport/tools_sh; ./stop_all.sh > /dev/null 2>1&"

		if [ $bneed_cgi -eq 1 -o $bneed_log_cgi -eq 1 ]; then
			echo "ssh -p 13306 root@$s \"cd /home/mtreport/httpd_check; ./stop.sh > /dev/null 2>1&\""
			ssh -p 13306 root@$s "cd /home/mtreport/httpd_check; ./stop.sh > /dev/null 2>1&"
			sleep 1
		fi

		echo "ssh -p 13306 -l mtreport $s \"ps -elf |grep slog|wc -l\""
		c=`ssh -p 13306 -l mtreport $s "ps -elf |grep slog|wc -l"`
		if [ $c -le 2 ]; then
			break;
		fi
		i=`expr $i + 1`
		echo "wait stop all slog $i $c"
	done
	if [ $i -ge 10 ]; then
		echo "failed to stop process ($s) !"
		exit 1
	fi

	i=0
	ssh -p 13306 mtreport@$s "cd /home/mtreport/tools_sh; ./rm_zero.sh > /dev/null 2>1&";
	sleep 1
	ssh -p 13306 mtreport@$s "cd /home/mtreport/tools_sh; ./rm_zero.sh > /dev/null 2>1&";
	sleep 2

	ssh -p 13306 mtreport@$s "cd /home/mtreport/tools_sh; ./install_bin.sh > /dev/null 2>1&";


	if [ $bneed_cgi -eq 1 ]; then
		ssh -p 13306 root@$s "cd /srv/www/cgi-bin; cp /home/mtreport/remote/slog_cgi_bin.tar.gz .; tar -zxf slog_cgi_bin.tar.gz; cd /home/mtreport/httpd_check; ./start.sh"
	fi

# 日志系统 需要 mt_slog cgi
	if [ $bneed_cgi -eq 0 ];then
		isRemoteFileExist $s /srv/www/cgi-bin/mt_slog
		if [ $? -eq 1 ]; then
			echo "send fcgi - mt_slog"
			scp -P 13306 ../cgi_fcgi/mt_slog mtreport@$s:/srv/www/cgi-bin
		fi
	fi

#确保全部进程正确重启
	i=0
	while [ $i -lt 10 ]
	do
		ret=`ssh -p 13306 mtreport@$s "sh -x check_proc_monitor.sh 1; echo $?"`
		if [ $ret -eq 0 ]; then
			break;
		fi
		i=`expr $i + 1`
		echo "wait start process $i $ret"
	done
	if [ $i -ge 10 ]; then
		echo "failed to start process ($s) !"
		exit 1
	fi

#root 
	i=0
	while [ $i -lt 10 ]
	do
		isRemoteFileExist $s /home/mtreport/check_proc_monitor_root.sh
		if [ $? -eq 1 ]; then
			ret_root=`ssh -p 13306 root@$s "sh -x /home/mtreport/check_proc_monitor_root.sh 1; echo $?"`
		else
			ret_root=0
		fi

		if [ $ret_root -eq 0 ]; then
			break;
		fi
		i=`expr $i + 1`
		echo "wait start root process $i $ret"
	done
	if [ $i -ge 10 ]; then
		echo "failed to start root process ($s) !"
		exit 1
	fi

done


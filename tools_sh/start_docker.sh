#!/bin/bash

if [ ! -d /var/lib/mysql ]; then
	echo "failed, need mount mysql datadir /var/lib/mysql !"
	exit 1
fi

if [ ! -d /var/lib/mysql/mtreport_db ]; then
	if [ ! -d /var/lib/mysql_bk ]; then
		echo "not find dir /var/lib/mysql_bk"
		exit 2
	fi
	cp /var/lib/mysql_bk/* /var/lib/mysql -r
	chown mysql /var/lib/mysql -R
fi

if [ ! -d /home/mtreport/slog ]; then
	echo "failed, need mount dir /home/mtreport/slog !"
	exit 3
fi

mysqld &
sleep 1

mid=`pgrep -f mysqld`
if [ $? -ne 0 -o "$mid" == '' ]; then
	echo "start mysql failed !"
	exit 4
fi

function reset_xrk_server_ip()
{
        XRK_MYSQL_CONTEXT="mysql -B -umtreport -pmtreport875 mtreport_db "
        cur_ip=`echo "select ip from mt_server where xrk_type=1" | ${XRK_MYSQL_CONTEXT} -N`
        if [ $? -eq 0 -a "$cur_ip" != "" -a "$cur_ip" != "$xrk_host_ip" ]; then
                echo "update mt_server set ip='$xrk_host_ip' where xrk_type=1 or xrk_type=2 or xrk_type=4" | ${XRK_MYSQL_CONTEXT}
		fi

        local_ip=`cat /srv/www/cgi-bin/xrk_fastcgi_global.conf |grep LOCAL_IP|awk '{print $2}'`
		if [ "$local_ip" != "$xrk_host_ip" ]; then
        	mach_ip=`./slog_run_test show ips2d $local_ip|awk '{if(NF==3) print $1}'`
        	mach_ip2=`./slog_run_test show ips2d $xrk_host_ip|awk '{if(NF==3) print $1}'`
        	cur_mach_ip2=`echo "select ip2 from mt_machine where ip1=$mach_ip"| ${XRK_MYSQL_CONTEXT} -N`
        	if [ "$mach_ip2" != "$cur_mach_ip2" ]; then
                echo "update mt_machine set ip2=$mach_ip2 where ip1=$mach_ip" | ${XRK_MYSQL_CONTEXT}
        	fi
			sed -i '/LOCAL_IP/d' /srv/www/cgi-bin/xrk_fastcgi_global.conf
			echo "LOCAL_IP $xrk_host_ip" >> /srv/www/cgi-bin/xrk_fastcgi_global.conf
		fi

		cur_server_master=`cat /home/mtreport/slog_mtreport_client/slog_mtreport_client.conf|grep SERVER_MASTER|awk '{print $2}'`
		if [ "$cur_server_master" != "$xrk_host_ip" ]; then
			sed -i '/SERVER_MASTER/d' /home/mtreport/slog_mtreport_client/slog_mtreport_client.conf
			echo "SERVER_MASTER $xrk_host_ip" >> /home/mtreport/slog_mtreport_client/slog_mtreport_client.conf
		fi
}

function reset_xrk_http_port()
{
	sed -i '/SLOG_LOG_OUT_PORT/d' /srv/www/cgi-bin/mt_slog.conf
	echo "SLOG_LOG_OUT_PORT $xrk_http_port" >> /srv/www/cgi-bin/mt_slog.conf
}

[ "$xrk_host_ip" != "" ] && reset_xrk_server_ip;
[ "$xrk_http_port" != "" ] && reset_xrk_http_port;

cd tools_sh
sh -x check_proc_monitor.sh 1
cd ..

apachectl start
sleep 1
hid=`pgrep -f httpd-prefork`
fcgi=`pgrep -f slog_flogin`
if [ $? -ne 0 -o "$hid" == '' -o "$fcgi" == '' ]; then
	echo "start httpd or cgi failed !"
	exit 5
fi


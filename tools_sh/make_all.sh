#!/bin/bash
cd ..
Name=slog_all
TarF=${Name}.tar
TarP=$(pwd)/${TarF}
BackupDir=release_pack

# check 下所有服务代码是否编译成功
function check_module()
{
	mname=$1
	if [ ! -f ${mname}/${mname} ]; then
		echo "check file: ${mname}/${mname} failed !"
		rm -f ${TarP}
		exit 2
	fi
}
check_module slog_config
check_module slog_mtreport_client 
check_module slog_write 
check_module slog_deal_warn 
check_module slog_mtreport_server 
check_module slog_check_warn 
check_module slog_memcached 
check_module slog_server 
check_module slog_client 
check_module slog_monitor_server 

# check 下所有 cgi 是否编译成功
function check_cgi()
{
	mname=$1
	if [ ! -f cgi_fcgi/${mname} ]; then
		echo "check file: cgi_fcgi/${mname} failed !"
		rm -f ${TarP}
		exit 3
	fi
}
check_cgi mt_slog_machine
check_cgi mt_slog_monitor 
check_cgi mt_slog_showview 
check_cgi mt_slog_attr 
check_cgi mt_slog_view 
check_cgi mt_slog_warn 
check_cgi slog_flogin 
check_cgi mt_slog_user 
check_cgi mt_slog 

tar cvf ${TarP} tools_sh/rm_zero.sh
tar rvf ${TarP} tools_sh/check_proc_monitor.sh
tar rvf ${TarP} tools_sh/start_all.sh
tar rvf ${TarP} tools_sh/stop_all.sh
tar rvf ${TarP} tools_sh/add_crontab.sh
tar rvf ${TarP} tools_sh/start_comm.sh
tar rvf ${TarP} tools_sh/stop_comm.sh
tar rvf ${TarP} tools_sh/install_bin.sh
tar rvf ${TarP} cgi_fcgi/* --exclude *.cpp --exclude Makefile --exclude cgi_debug.txt 

dirlist=`find . -maxdepth 1 -type d`
for dr in $dirlist
do 
        if [ -f $dr/$dr ] ; then
                tar rvf ${TarP} $dr/*.sh
                tar rvf ${TarP} $dr/*.conf
                tar rvf ${TarP} $dr/$dr
        fi
done

cd tools_sh
mkdir _tmp
mv ../${TarF} _tmp
cd _tmp
tar -xf ${TarF}
rm ${TarF}
tar -czf ${TarF}.gz *

cd ..
cp _tmp/${TarF}.gz .
rm -fr _tmp
if [ ! -d ${BackupDir} ]; then
	mkdir -p ${BackupDir}
fi
CurDate=`date "+%Y%m%d"`
cp ${TarF}.gz ${BackupDir}/${TarF}.gz.$CurDate


#!/bin/bash
cd ..
Name=slog_web
TarF=${Name}.tar.gz

# check 下所有 cgi 是否编译成功
function check_cgi()
{
	mname=$1
	if [ ! -f cgi_fcgi/${mname} ]; then
		echo "check file: cgi_fcgi/${mname} failed !"
		rm -f ${TarP}
		exit 1
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

tar -czf ${TarF} html/* cgi_fcgi/mt_slog_user cgi_fcgi/*.conf

mv ${TarF} tools_sh
cd tools_sh
if [ ! -d ${BackupDir} ]; then
	mkdir ${BackupDir}
fi
cp ${TarF} ${BackupDir}/${TarF}.$CurDate


#!/bin/bash
cd ..
Name=slog_web
TarF=${Name}.tar.gz

tar -czf ${TarF} html/* cgi_fcgi/mt_slog_user cgi_fcgi/*.conf
#mt_slog_warn mt_slog slog_flogin mt_slog_view mt_slog_showview mt_slog_attr mt_slog_monitor mt_slog_machine *.conf

mv ${TarF} tools_sh
cd tools_sh
if [ ! -d ${BackupDir} ]; then
	mkdir ${BackupDir}
fi
cp ${TarF} ${BackupDir}/${TarF}.$CurDate


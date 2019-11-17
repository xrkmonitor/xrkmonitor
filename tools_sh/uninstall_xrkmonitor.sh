#!/bin/bash

PATH=/bin:/sbin:/usr/bin/:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin
APACHE_DOCUMENT_ROOT=/srv/www/htdocs22
XRKMONITOR_HTML_PATH=xrkmonitor22
APACHE_CGI_PATH=/srv/www/cgi-bin/
XRKMONITOR_CGI_LOG_PATH=/var/log/mtreport22
MYSQL_USER=
MYSQL_PASS=

ls -l /tmp/pid*slog*pid >/dev/null 2>&1
if [ $? -eq 0 -a -f tools_sh/stop_all.sh -a -f tools_sh/rm_zero.sh ]; then
	echo "开始停止字符云监控系统服务"
	cd tools_sh; ./stop_all.sh; sleep 1; ./rm_zero.sh
	cd ..
	rm /tmp/pid*slog*pid > /dev/null 2>&1
fi

if [ -f $APACHE_DOCUMENT_ROOT/index.html ]; then
	cat $APACHE_DOCUMENT_ROOT/index.html |grep "$XRKMONITOR_HTML_PATH" > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "删除文件: $APACHE_DOCUMENT_ROOT/index.html"
		rm $APACHE_DOCUMENT_ROOT/index.html
	fi
fi

if [ -f $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH/dmt_login.html ]; then
	echo "删除 html/js 文件目录: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH"
	rm -fr $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH
fi

if [ -f "$APACHE_CGI_PATH/mt_slog" ]; then
	echo "删除 cgi 文件"
	rm $APACHE_CGI_PATH/mt_slog*
fi

[ -d "$XRKMONITOR_CGI_LOG_PATH" ] && (echo "删除 cgi 日志目录: $XRKMONITOR_CGI_LOG_PATH"; rm -fr "$XRKMONITOR_CGI_LOG_PATH")
[ -d slog_check_proc ] && (echo "删除目录: slog_check_proc"; rm -fr slog_check_proc)
[ -d xrkmonitor_lib ] && (echo "删除目录: xrkmonitor_lib"; rm -fr xrkmonitor_lib)
[ -d cgi_fcgi ] && (echo "删除目录: cgi_fcgi"; rm -fr cgi_fcgi)
[ -d db ] && (echo "删除目录: db"; rm -fr db)
[ -d html ] && (echo "删除目录: html"; rm -fr html)
[ -d slog_check_warn ] && (echo "删除目录: slog_check_warn"; rm -fr slog_check_warn)
[ -d slog_client ] && (echo "删除目录: slog_client"; rm -fr slog_client)
[ -d slog_config ] && (echo "删除目录: slog_config"; rm -fr slog_config)
[ -d slog_deal_warn ] && (echo "删除目录: slog_deal_warn"; rm -fr slog_deal_warn)
[ -d slog_memcached ] && (echo "删除目录: slog_memcached"; rm -fr slog_memcached)
[ -d slog_monitor_server ] && (echo "删除目录: slog_monitor_server"; rm -fr slog_monitor_server)
[ -d slog_mtreport_client ] && (echo "删除目录: slog_mtreport_client"; rm -fr slog_mtreport_client)
[ -d slog_mtreport_server ] && (echo "删除目录: slog_mtreport_server"; rm -fr slog_mtreport_server)
[ -d slog_server ] && (echo "删除目录: slog_server"; rm -fr slog_server)
[ -d slog_tool ] && (echo "删除目录: slog_tool"; rm -fr slog_tool)
[ -d slog_write ] && (echo "删除目录: slog_write"; rm -fr slog_write)
[ -d tools_sh ] && (echo "删除目录: tools_sh"; rm -fr tools_sh)

if [ -f xrkmonitor_lib.tar.gz ]; then
	echo "删除文件: xrkmonitor_lib.tar.gz"
	rm xrkmonitor_lib.tar.gz
fi

if [ -f /etc/ld.so.conf_xrkmonitor_bk ]; then
	echo "回退动态链接库配置文件: /etc/ld.so.conf"
	cp /etc/ld.so.conf_xrkmonitor_bk /etc/ld.so.conf
	rm /etc/ld.so.conf_xrkmonitor_bk
fi

if [ -f slog_run_test ]; then
	echo "删除运行测试文件: slog_run_test"
	rm slog_run_test
fi

if [ -f slog_all.tar.gz ]; then
	echo "删除安装包文件: slog_all.tar.gz"
	rm slog_all.tar.gz
fi

echo ""
if [ -z "$MYSQL_USER" -o -z "$MYSQL_PASS" ]; then
	MYSQL_CONTEXT="mysql -B "
else 
	MYSQL_CONTEXT="mysql -B -u$MYSQL_USER -p$MYSQL_PASS"
fi

echo "show databases" | ${MYSQL_CONTEXT} |grep mtreport_db > /dev/null 2>&1
if [ $? -eq 0 ]; then
	echo "删除 mysql 数据库 mtreport_db/attr_db"
	echo "drop database mtreport_db" | ${MYSQL_CONTEXT}
	echo "drop database attr_db" | ${MYSQL_CONTEXT}
	echo "删除默认 mysql 账号 mtreport"
	echo "drop user mtreport@localhost" | ${MYSQL_CONTEXT}
fi

echo "已为您清理干净字符云监控系统安装记录, 感谢您的关注."
echo ""


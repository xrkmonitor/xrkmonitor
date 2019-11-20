#!/bin/bash
PATH=/bin:/sbin:/usr/bin/:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin

APACHE_DOCUMENT_ROOT=/srv/www/htdocs
APACHE_CGI_PATH=/srv/www/cgi-bin
XRKMONITOR_HTML_PATH=xrkmonitor
XRKMONITOR_CGI_LOG_PATH=/var/log/mtreport
MYSQL_USER=
MYSQL_PASS=
SLOG_SERVER_FILE_PATH=/home/mtreport/slog/

ls -l /tmp/pid*slog*pid >/dev/null 2>&1
if [ $? -eq 0 -a -f tools_sh/stop_all.sh -a -f tools_sh/rm_zero.sh ]; then
	echo "开始停止字符云监控系统服务, 请耐心等待..."
	cd tools_sh; ./stop_all.sh; 
	echo "开始清理共享内存"
	sleep 1;
	./rm_zero.sh
	cd ..
	rm /tmp/pid*slog*pid > /dev/null 2>&1
fi

if [ -f /tmp/_slog_config_read_ok ]; then
	rm -f /tmp/_slog_config_read_ok
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
	rm $APACHE_CGI_PATH/slog_flogin*
fi

[ -d "$XRKMONITOR_CGI_LOG_PATH" ] && (echo "删除 cgi 日志目录: $XRKMONITOR_CGI_LOG_PATH"; rm -fr "$XRKMONITOR_CGI_LOG_PATH")
[ -d slog_core ] && (echo "删除目录: slog_core"; rm -fr slog_core)
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
[ -f xrkmonitor_lib.tar.gz ] && (echo "删除文件: xrkmonitor_lib.tar.gz"; rm -fr xrkmonitor_lib.tar.gz)
[ -f _run_test_tmp ] && (echo "删除临时文件: _run_test_tmp"; rm -fr _run_test_tmp)

function yn_continue()
{
	read -p "$1" op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do
		read -p "请输入 (y/n): " op
	done
	if [ "$op" != "y" -a "$op" != "Y" ];then
		echo "no" 
	else
		echo "yes"
	fi
}

[ -f slog_run_test ] && (echo "删除测试文件: slog_run_test"; rm -fr slog_run_test)
[ -f slog_all.tar.gz ] && (echo "删除测试文件: slog_all.tar.gz"; rm -fr slog_all.tar.gz)

if [ ! -f /etc/ld.so.conf ]; then
	echo "动态链接库配置文件: /etc/ld.so.conf 不存在!"
else
	cat /etc/ld.so.conf |grep xrkmonitor_lib > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		sed -i '/xrkmonitor_lib/d' /etc/ld.so.conf
	fi
	ldconfig
fi


echo ""
if [ -z "$MYSQL_USER" -o -z "$MYSQL_PASS" ]; then
	MYSQL_CONTEXT="mysql -B "
else 
	MYSQL_CONTEXT="mysql -B -u$MYSQL_USER -p$MYSQL_PASS"
fi

echo "show databases" | ${MYSQL_CONTEXT} |grep mtreport_db > /dev/null 2>&1
if [ $? -eq 0 ]; then
	isyes=$(yn_continue "是否清理 mysql 数据库 mtreport_db/attr_db(y/n)?")
	if [ "$isyes" == "yes" ]; then
		echo "删除 mysql 数据库 mtreport_db/attr_db"
		echo "drop database mtreport_db" | ${MYSQL_CONTEXT}
		echo "drop database attr_db" | ${MYSQL_CONTEXT}
		echo "删除默认 mysql 账号 mtreport"
		echo "drop user mtreport@localhost" | ${MYSQL_CONTEXT}
	fi
else
	echo "未检测到 mysql 数据库: mtreport_db/attr_db, 跳过清理"	
fi

if [ -d "$SLOG_SERVER_FILE_PATH" ]; then
	isyes=$(yn_continue "是否删除日志目录以及日志文件 (y/n)?")
	if [ "$isyes" == "yes" ]; then
		rm -fr $SLOG_SERVER_FILE_PATH
	fi
else
	echo "未检测到日志目录: $SLOG_SERVER_FILE_PATH, 跳过日志清理"
fi

isyes=$(yn_continue "是否删除安装/卸载脚本 (y/n)?")
if [ "$isyes" == "yes" ]; then
	rm online_install.sh 
	rm uninstall_xrkmonitor.sh
fi

echo "已为您清理干净字符云监控系统安装记录, 感谢您的关注."
echo ""


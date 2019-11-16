#!/bin/bash
PATH=/bin:/sbin:/usr/bin/:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin

#
#
# 字符云监控系统卸载脚本
#
# 字符云监控系统 -- 免费/开源的国产监控系统
# 开源演示版网址: http://open.xrkmonitor.com
# QQ 交流群: 699014295
# @2019-11-16, version 1.0
#
# ---------------- 字符云监控系统 安装环境配置 ----------------------- start -

# apache 网站根目录
APACHE_DOCUMENT_ROOT=/srv/www/htdocs2

# 字符云监控 html/js 文件安装路径, 相对网站根目录, 绝对路径为: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH
XRKMONITOR_HTML_PATH=xrkmonitor

# 字符云监控 cgi 文件安装路径
XRKMONITOR_CGI_PATH=/srv/www/cgi-bin

# 字符云监控 cgi 本地日志文件路径, 用于调试, 正常运行时您可以关闭本地日志使用日志中心
XRKMONITOR_CGI_LOG_PATH=/var/log/mtreport/

# 指定本机IP 网卡名,用于获取本机IP, 不指定时自动获取
LOCAL_IP_ETHNAME=

# mysql 数据库操作账号,默认使用匿名账号,如您已禁止匿名账号请在此配置可用操作账号
MYSQL_USER=
MYSQL_PASS=

# ---------------- 字符云监控系统 安装环境配置 ----------------------- end -
#
#
#
#
#
#

if [ -f tools_sh/stop_all.sh -a -f tools_sh/rm_zero.sh ]; then
	echo "开始停止字符云监控系统服务"
	cd tools_sh; ./stop_all.sh; sleep 1; ./rm_zero.sh
	cd ..
fi

if [ -f $APACHE_DOCUMENT_ROOT/index.html ]; then
	cat $APACHE_DOCUMENT_ROOT/index.html |grep xrkmonitor > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "删除文件: $APACHE_DOCUMENT_ROOT/index.html"
		rm $APACHE_DOCUMENT_ROOT/index.html
	fi
fi

if [ -f $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH/dmt_login.html ]; then
	echo "删除 html/js 文件目录: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH"
	rm -fr $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH
fi

if [ -d "$XRKMONITOR_CGI_LOG_PATH" ]; then
	echo "删除 cgi 日志目录: $XRKMONITOR_CGI_LOG_PATH"
	rm -fr $XRKMONITOR_CGI_LOG_PATH
fi

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
[ -d xrkmonitor.tar.gz ] && (echo "删除目录: xrkmonitor.tar.gz"; rm -fr xrkmonitor.tar.gz)

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

if [ -f xrkmonitor.tar.gz ]; then
	echo "删除安装包文件: xrkmonitor.tar.gz"
	rm xrkmonitor.tar.gz
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
	echo "drop user mtreport@\'%\'" | ${MYSQL_CONTEXT}
fi

if [ -f /tmp/pid*slog*pid ]; then
	echo "删除运行时 pid 文件"
	rm /tmp/pid*slog*pid > /dev/null 2>&1
fi

echo "已为您清理干净字符云监控系统安装记录, 感谢您的关注."



#!/bin/bash
PATH=/bin:/sbin:/usr/bin/:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin

#
# 字符云监控系统 -- 免费/开源的国产监控系统
# 开源演示版网址: http://open.xrkmonitor.com
# QQ 交流群: 699014295
#
# ---------------- 字符云监控系统 安装环境配置 ----------------------- start -

# apache 网站根目录, CGI 目录
DOCUMENT_ROOT=/srv/www/htdocs 
CGI_PATH=/srv/www/cgi-bin

# mysql 数据库操作账号,默认使用匿名账号,如您已禁止匿名账号请在此配置可用操作账号
MYSQL_USER=mtreport
MYSQL_PASS=mtreport875

# ---------------- 字符云监控系统 安装环境配置 ----------------------- end -
#
#
#
#

STEP_TOTAL=5
CUR_STEP=1
XRKMONITOR_HTTP=http://open.xrkmonitor.com/xrkmonitor_down
cur_path=`pwd`

if [ $# -eq 1 -a "$1" == "new" ]; then 
	rm xrkmonitor_lib.tar.gz
	rm slog_run_test
	echo "开始全新安装: 字符云监控系统, 共 $STEP_TOTAL 步"
else
	echo "开始自动安装: 字符云监控系统, 共 $STEP_TOTAL 步"
fi

function failed_my_exit()
{
	echo ""
	echo "在线安装字符云监控失败,错误码:$1, 您可以下载源码编译安装或者加入Q群(699014295)获得支持."
	exit $1
}

function check_file()
{
	if [ ! -f $1 ]; then
		echo "安装文件检查失败, 文件:$1 不存在!"
		failed_my_exit $2 
	fi
}

echo ""
echo "($CUR_STEP/$STEP_TOTAL) 安装环境检测"
CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
echo "($CUR_STEP/$STEP_TOTAL) 下载并解压库文件: xrkmonitor_lib.tar.gz"
if [ ! -f xrkmonitor_lib.tar.gz ]; then
	wget ${XRKMONITOR_HTTP}/xrkmonitor_lib.tar.gz 
else
	echo "xrkmonitor_lib.tar.gz 文件已存在"
fi
check_file xrkmonitor_lib.tar.gz 1
tar -zxf xrkmonitor_lib.tar.gz
check_file xrkmonitor_lib/libmtreport_api-1.1.0.so 2
if [ ! -f /etc/ld.so.conf ]; then
	echo "动态链接库配置文件: /etc/ld.so.conf 不存在!"
	failed_my_exit 3
else
	echo ${cur_path}/xrkmonitor_lib >> /etc/ld.so.conf
	ldconfig
fi
CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
echo "($CUR_STEP/$STEP_TOTAL) 下载运行测试文件: slog_run_test"
if [ ! -f slog_run_test ]; then
	wget ${XRKMONITOR_HTTP}/slog_run_test
	chmod +x slog_run_test 
	check_file slog_run_test 4
else
	echo "slog_run_test 文件已存在"
fi
./slog_run_test run_test 
if [ $? -ne 0 ]; then
	echo "运行测试文件失败"
	failed_my_exit 5
fi
CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
echo "($CUR_STEP/$STEP_TOTAL) 下载并解压安装包文件: xrkmonitor.tar.gz, 请您耐心等待..."
if [ ! -f xrkmonitor.tar.gz ]; then
	wget ${XRKMONITOR_HTTP}/xrkmonitor.tar.gz
else
	echo "xrkmonitor.tar.gz 文件已存在"
fi
check_file xrkmonitor.tar.gz 6
tar -zxf xrkmonitor.tar.gz 
check_file db/mtreport_db.sql 7
CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
if [ -z "$MYSQL_USER" -o -z "$MYSQL_PASS" ]; then
	echo "($CUR_STEP/$STEP_TOTAL) 开始尝试使用匿名账号初始化 mysql 数据库"
	MYSQL_CONTEXT="mysql -B "
else 
	echo "($CUR_STEP/$STEP_TOTAL) 开始使用指定账号($MYSQL_USER|$MYSQL_PASS)初始化 mysql 数据库"
	MYSQL_CONTEXT="mysql -B -u$MYSQL_USER -p$MYSQL_PASS"
fi
${MYSQL_CONTEXT} < db/mtreport_db.sql
${MYSQL_CONTEXT} < db/attr_db.mysql
echo "show databases" | ${MYSQL_CONTEXT} |grep mtreport_db > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "导入 mysql 数据库 mtreport_db/attr_db 失败"
	failed_my_exit 8
fi
echo "导入 mysql 数据库 mtreport_db/attr_db 成功"

echo "grant ALL PRIVILEGES ON mtreport_db.* to 'mtreport'@'localhost' identified by 'mtreport875' WITH GRANT OPTION;" | ${MYSQL_CONTEXT} 
echo "grant ALL PRIVILEGES ON attr_db.* to 'mtreport'@'localhost' identified by 'mtreport875' WITH GRANT OPTION;" | ${MYSQL_CONTEXT} 
echo "show tables" | mysql -B -umtreport -pmtreport875 mtreport_db |grep flogin_user > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "授权默认账号:mtreport 访问 mysql 数据库失败"
	failed_my_exit 9
fi
CUR_STEP=`expr 1 + $CUR_STEP`


echo ""
echo "($CUR_STEP/$STEP_TOTAL) 开始初始化安装字符云监控 web 控制台"

CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
echo "恭喜您, 自动安装完成, 现在您可以在浏览器中访问控制台了"



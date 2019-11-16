#!/bin/bash
PATH=/bin:/sbin:/usr/bin/:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin

#
#
# 字符云监控系统 -- 免费/开源的国产监控系统
# 开源演示版网址: http://open.xrkmonitor.com
# QQ 交流群: 699014295
# @2019-11-16, version 1.0
#
# ---------------- 字符云监控系统 安装环境配置 ----------------------- start -

# apache 网站根目录
APACHE_DOCUMENT_ROOT=/srv/www/htdocs

# apache cgi 绝对路径
APACHE_CGI_PATH=/srv/www/cgi-bin/

# apache cgi 访问路径, apache 中使用: ScriptAlias 指定
APACHE_CGI_ACCESS_PATH=/cgi-bin/

# 字符云监控 html/js 文件安装路径, 相对网站根目录, 绝对路径为: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH
XRKMONITOR_HTML_PATH=xrkmonitor

# 字符云监控 cgi 本地日志文件路径, 用于调试, 正常运行时您可以关闭本地日志使用日志中心
XRKMONITOR_CGI_LOG_PATH=/var/log/mtreport22

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

STEP_TOTAL=5
CUR_STEP=1
XRKMONITOR_HTTP=http://open.xrkmonitor.com/xrkmonitor_down
cur_path=`pwd`

function update_config() 
{
	sed -i "/^APACHE_DOCUMENT_ROOT=/cAPACHE_DOCUMENT_ROOT=${APACHE_DOCUMENT_ROOT}"  $1 
	sed -i "/^XRKMONITOR_HTML_PATH=/cXRKMONITOR_HTML_PATH=${XRKMONITOR_HTML_PATH}" $1 
	sed -i "/^APACHE_CGI_PATH=/cAPACHE_CGI_PATH=${APACHE_CGI_PATH}" $1 
	sed -i "/^XRKMONITOR_CGI_LOG_PATH=/cXRKMONITOR_CGI_LOG_PATH=${XRKMONITOR_CGI_LOG_PATH}" $1 
	sed -i "/^MYSQL_USER=/cMYSQL_USER=${MYSQL_USER}" $1 
	sed -i "/^MYSQL_PASS=/cMYSQL_PASS=${MYSQL_PASS}" $1 
}
update_config uninstall_xrkmonitor.sh

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

function yn_exit()
{
	read -p "$1" op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do
		read -p "请输入 (y/n): " op
	done
	if [ $op != "y" -a $op != "Y" ];then
		failed_my_exit $2
	fi
}

function auto_detect_apache_doc_root()
{
	sDocRoot=`apachectl -t -D DUMP_RUN_CFG 2>/dev/null |grep "Main DocumentRoot"|awk '{print $3}'`
	APACHE_DOCUMENT_ROOT=${sDocRoot//\"/}
	if [ $? -ne 0 -o ! -d "$APACHE_DOCUMENT_ROOT" ]; then
		echo "尝试自动探测 apache 网站根目录失败, 请手动指定安装配置: APACHE_DOCUMENT_ROOT 后再试"
		failed_my_exit $LINENO 
	fi
	echo "成功探测到 apache 网站根: $APACHE_DOCUMENT_ROOT"
	sed -i "/^APACHE_DOCUMENT_ROOT=/cAPACHE_DOCUMENT_ROOT=${APACHE_DOCUMENT_ROOT}" uninstall_xrkmonitor.sh 
}

function auto_detect_apache_cgi_path()
{
	if [ -d /etc/apache2 ]; then
		sCgiPath=`cat /etc/apache2/*.conf|grep ^ScriptAlias|awk '{print $3}'`
		APACHE_CGI_PATH=${sCgiPath//\"/}
		sCgiAccessPath=`cat /etc/apache2/*.conf|grep ^ScriptAlias|awk '{print $2}'`
		APACHE_CGI_ACCESS_PATH=${sCgiAccessPath//\"/}		
		if [ $? -ne 0 -o ! -d "$APACHE_CGI_PATH" -o -z "$APACHE_CGI_ACCESS_PATH" ]; then
			echo "尝试自动探测 cgi 目录失败, 请手动指定安装配置: APACHE_CGI_PATH/APACHE_CGI_ACCESS_PATH 后再试"
			failed_my_exit $LINENO
		fi
	else
		echo "尝试自动探测 cgi 目录失败, 请手动指定安装配置: APACHE_CGI_PATH 后再试"
		failed_my_exit $LINENO
	fi

	echo "成功探测到 cgi 绝对路径目录: $APACHE_CGI_PATH"
	sed -i "/^APACHE_CGI_PATH=/cAPACHE_CGI_PATH=${APACHE_CGI_PATH}" uninstall_xrkmonitor.sh
	echo "成功探测到 cgi 访问路径: $APACHE_CGI_ACCESS_PATH"
	sed -i "/^APACHE_CGI_ACCESS_PATH=/cAPACHE_CGI_ACCESS_PATH=${APACHE_CGI_ACCESS_PATH}" uninstall_xrkmonitor.sh
}

echo ""
echo "($CUR_STEP/$STEP_TOTAL) 安装环境检测"
if [ ! -d "$APACHE_DOCUMENT_ROOT" ]; then
	echo "apache 网站根目录: $APACHE_DOCUMENT_ROOT 不存在, 尝试自动探测"
	auto_detect_apache_doc_root
fi

if [ ! -d "$APACHE_CGI_PATH" ]; then
	echo "apache 网站 cgi 目录: $APACHE_CGI_PATH 不存在, 尝试自动探测"
	auto_detect_apache_cgi_path	
fi

if [ ! -d "$APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH" ]; then
	yn_exit "字符云监控 html/js 文件安装路径: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH 不存在,是否新建(y/n)?" 102
	mkdir -p "$APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH"
	if [ ! -d "$APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH" ]; then
		echo "新建目录: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH 失败"
		failed_my_exit $LINENO 
	fi
fi

if [ ! -d "$XRKMONITOR_CGI_LOG_PATH" ]; then
	yn_exit "cgi 本地日志文件路径: $XRKMONITOR_CGI_LOG_PATH 不存在, 是否新建(y/n)?" $LINENO
	mkdir -p "$XRKMONITOR_CGI_LOG_PATH"
	if [ ! -d "$XRKMONITOR_CGI_LOG_PATH" ]; then
		echo "新建目录: $XRKMONITOR_CGI_LOG_PATH 失败"
		failed_my_exit $LINENO 
	fi
	chmod 777 "$XRKMONITOR_CGI_LOG_PATH"
	if [ $? -ne 0 ]; then
		yn_exit "修改 cgi 日志目录权限失败, 请确保cgi 可写日志目录, 是否继续(y/n)" $LINENO 
	fi
fi

if [ ! -z "$LOCAL_IP_ETHNAME" ]; then
	LOCAL_IP=`ip addr |grep inet|grep "$LOCAL_IP_ETHNAME"|grep -v inet6|grep -v "127.0.0.1" |awk '{print $2}'|awk -F "/" '{ if(f==0) print $1; f++; }'`
else
	LOCAL_IP=`ip addr |grep inet|grep -v inet6|grep -v "127.0.0.1" |awk '{print $2}'|awk -F "/" '{ if(f==0) print $1; f++; }'`
fi
if [ $? -ne 0 -o -z "$LOCAL_IP" ]; then
	if [ ! -z "$LOCAL_IP_ETHNAME" ]; then
		LOCAL_IP=`ifconfig eth0|grep inet|grep -v inet6 |awk '{print $2}'|awk -F ":" '{if(f==0) print $2; f++;}'`		
	fi
	if [ $? -ne 0 -o -z "$LOCAL_IP" ]; then
		echo "本机IP 获取失败, 您可以通过 LOCAL_IP_ETHNAME 指定网卡名并使用root 账号运行安装脚本"
		failed_my_exit $LINENO 
	fi
fi
echo "本机IP 获取成功: $LOCAL_IP"
CUR_STEP=`expr 1 + $CUR_STEP`


echo ""
echo "($CUR_STEP/$STEP_TOTAL) 下载并解压库文件: xrkmonitor_lib.tar.gz"
if [ ! -f xrkmonitor_lib.tar.gz ]; then
	wget ${XRKMONITOR_HTTP}/xrkmonitor_lib.tar.gz 
else
	echo "xrkmonitor_lib.tar.gz 文件已存在"
fi
check_file xrkmonitor_lib.tar.gz $LINENO
tar -zxf xrkmonitor_lib.tar.gz
check_file xrkmonitor_lib/libmtreport_api-1.1.0.so $LINENO
if [ ! -f /etc/ld.so.conf ]; then
	echo "动态链接库配置文件: /etc/ld.so.conf 不存在!"
	failed_my_exit $LINENO
else
	cat /etc/ld.so.conf |grep xrkmonitor_lib > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		cp /etc/ld.so.conf /etc/ld.so.conf_xrkmonitor_bk
		echo ${cur_path}/xrkmonitor_lib >> /etc/ld.so.conf
		ldconfig
	fi
fi
CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
echo "($CUR_STEP/$STEP_TOTAL) 下载运行测试文件: slog_run_test"
if [ ! -f slog_run_test ]; then
	wget ${XRKMONITOR_HTTP}/slog_run_test
	chmod +x slog_run_test 
	check_file slog_run_test $LINENO 
else
	echo "slog_run_test 文件已存在"
fi
./slog_run_test run_test 
if [ $? -ne 0 ]; then
	echo "运行测试文件失败"
	failed_my_exit $LINENO
fi
CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
echo "($CUR_STEP/$STEP_TOTAL) 下载并解压安装包文件: xrkmonitor.tar.gz, 请您耐心等待..."
if [ ! -f xrkmonitor.tar.gz ]; then
	wget ${XRKMONITOR_HTTP}/xrkmonitor.tar.gz
else
	echo "xrkmonitor.tar.gz 文件已存在"
fi
check_file xrkmonitor.tar.gz $LINENO 
tar -zxf xrkmonitor.tar.gz 
check_file db/mtreport_db.sql $LINENO 
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
	failed_my_exit $LINENO
fi
echo "导入 mysql 数据库 mtreport_db/attr_db 成功"

echo "grant ALL PRIVILEGES ON mtreport_db.* to 'mtreport'@'localhost' identified by 'mtreport875' WITH GRANT OPTION;" | ${MYSQL_CONTEXT} 
echo "grant ALL PRIVILEGES ON attr_db.* to 'mtreport'@'localhost' identified by 'mtreport875' WITH GRANT OPTION;" | ${MYSQL_CONTEXT} 
echo "show tables" | mysql -B -umtreport -pmtreport875 mtreport_db |grep flogin_user > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "授权默认账号:mtreport 访问 mysql 数据库失败"
	failed_my_exit $LINENO 
fi
echo "update mt_server set ip='$LOCAL_IP'" | mysql -B -umtreport -pmtreport875 mtreport_db > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "初始化字符云监控系统服务器IP失败, mysql update 数据库失败"
	failed_my_exit $LINENO
fi
CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
echo "($CUR_STEP/$STEP_TOTAL) 修改字符云监控服务相关配置"
sed -i '/SERVER_MASTER/d' slog_mtreport_client/slog_mtreport_client.conf
echo "SERVER_MASTER $LOCAL_IP" >> slog_mtreport_client/slog_mtreport_client.conf

CUR_CGI_LOG_PATH=`cat cgi_fcgi/slog_flogin.conf |grep SLOG_LOG_FILE|awk '{print $2}'|xargs dirname`
if [ "$CUR_CGI_LOG_PATH" != "$XRKMONITOR_CGI_LOG_PATH" ]; then
	echo "更新 CGI 配置文件日志文件路径"
	OLD_CGI_LOG_PATH=${CUR_CGI_LOG_PATH//\//\\\/}
	NEW_CGI_LOG_PATH=${XRKMONITOR_CGI_LOG_PATH//\//\\\/}
	sed -i "s/${OLD_CGI_LOG_PATH}/${NEW_CGI_LOG_PATH}/g" `ls cgi_fcgi/*.conf -1`
fi
if [ "$XRKMONITOR_HTML_PATH" != "xrkmonitor" ]; then
	sXrkmonitorHtmlPath=${XRKMONITOR_HTML_PATH//\//\\\/}
	sed -i "s/\/xrkmonitor/$sXrkmonitorHtmlPath/g" `ls cgi_fcgi/*.conf -1`
	sed -i "s/\/xrkmonitor/$sXrkmonitorHtmlPath/g" html/index.html
fi
if [ "$APACHE_CGI_ACCESS_PATH" != "/cgi-bin/" ]; then
	sCgiAccessPath=${APACHE_CGI_ACCESS_PATH//\//\\\/}
	sed -i "s/\/cgi-bin\//$sCgiAccessPath/g" `ls cgi_fcgi/*.conf -1`
	sed -i "s/\/cgi-bin\//$sCgiAccessPath/g" html/index.html
fi
CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
echo "($CUR_STEP/$STEP_TOTAL) 开始初始化安装字符云监控 web 控制台"
cp html/* $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH -fr
check_file $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH/dmt_login.html $LINENO 
cp $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH/index.html $APACHE_DOCUMENT_ROOT
check_file $APACHE_DOCUMENT_ROOT/index.html $LINENO 
cp cgi_fcgi/* $APACHE_CGI_PATH -fr
check_file $APACHE_CGI_PATH/mt_slog_monitor $LINENO 
CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
echo "($CUR_STEP/$STEP_TOTAL) 开始启动字符云监控后台服务, 请您耐心等待..."
cd tools_sh
ls -l /tmp/pid*slog*pid >/dev/null 2>&1
if [ $? -eq 0 ]; then
	./stop_all.sh
fi
./check_proc_monitor.sh 1
cd ..
sleep 1
echo "开始检测确认字符云监控后台服务运行是否正常"
check_file /tmp/pid.slog_config.pid $LINENO
check_file /tmp/pid.slog_client.pid $LINENO 
check_file /tmp/pid.slog_check_warn.pid $LINENO 
check_file /tmp/pid.slog_deal_warn.pid $LINENO 
check_file /tmp/pid.slog_monitor_server.pid $LINENO 
check_file /tmp/pid.slog_mtreport_server.pid $LINENO 
check_file /tmp/pid.slog_server.pid $LINENO 
check_file /tmp/pid.slog_write.pid $LINENO 
./add_crontab.sh > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "安装字符云监控后台服务自动拉起脚本:add_crontab.sh 到 crontab 失败,请您手动安装"
fi
echo "安装字符云监控后台服务自动拉起脚本: add_crontab.sh 到 crontab 成功"
CUR_STEP=`expr 1 + $CUR_STEP`

echo ""
echo "恭喜您, 自动安装完成, 现在您可以在浏览器中访问控制台, 可能的网址: http://$LOCAL_IP"
echo ""

cp online_install.sh online_install.sh_bk
update_config online_install.sh_bk
mv online_install.sh_bk online_install.sh


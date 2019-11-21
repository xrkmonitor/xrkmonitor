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

# 如果您的服务器是云服务器, 请通过以下配置指定外网 IP
SERVER_OUT_IP=

# mysql 数据库操作账号,默认使用匿名账号,如您已禁止匿名账号请在此配置可用操作账号
MYSQL_USER=
MYSQL_PASS=

# 日志中心日志目录, cgi 搜索日志时要读取该目录, 请确保 cgi 的访问权限
SLOG_SERVER_FILE_PATH=/home/mtreport/slog/

# apache 网站默认根目录, 不存在时会自动探测
APACHE_DOCUMENT_ROOT=/srv/www/htdocs

# apache cgi 绝对路径, 字符云监控 cgi 将安装在该目录下, 不存在时会自动探测
APACHE_CGI_PATH=/srv/www/cgi-bin

# apache cgi 访问路径, apache 中使用: ScriptAlias 指定, 注意末尾带上路径符: / 
APACHE_CGI_ACCESS_PATH=/cgi-bin/

# 字符云监控 html/js 文件安装路径, 相对网站根目录, 绝对路径为: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH
XRKMONITOR_HTML_PATH=xrkmonitor

# 字符云监控 cgi 本地日志文件路径, 用于调试, 正常运行时您可以关闭本地日志使用日志中心
XRKMONITOR_CGI_LOG_PATH=/var/log/mtreport

# 指定本机IP 网卡名, 用于获取本机IP, 不指定时自动获取第一个IP 作为本机IP
LOCAL_IP_ETHNAME=

# ---------------- 字符云监控系统 安装环境配置 ----------------------- end -



install_sh_home=`pwd`
function is_ip_valid()
{
	IP=$1
	if [[ $IP =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
		FIELD1=`echo $IP|awk -F "." '{print $1}'`
		FIELD2=`echo $IP|awk -F "." '{print $2}'`
		FIELD3=`echo $IP|awk -F "." '{print $3}'`
		FIELD4=`echo $IP|awk -F "." '{print $4}'`
		[ $FIELD1 -le 255 -a $FIELD2 -le 255 -a $FIELD3 -le 255 -a $FIELD4 -le 255 ] && return 0
	fi
	return 1
}

function yn_continue()
{
	read -p "$1" op
	while [ "$op" != "Y" -a "$op" != "y" -a "$op" != "N" -a "$op" != "n" ]; do
		read -p "请输入 (y/n): " op
	done
	if [ "$op" != "y" -a "$op" != "Y" ];then
		echo "no" 
	else
		echo "yes"
	fi
}

function failed_my_exit()
{
	echo ""
	echo "在线安装字符云监控失败,错误码:$1, 您可以下载源码编译安装或者加入Q群(699014295)获得支持."
	isunin=$(yn_continue "是否清理安装记录(y/n)?")
	if [ "$isunin" == "yes" ]; then
		./uninstall_xrkmonitor.sh
	fi
	exit $1
}

function check_file()
{
	if [ ! -f "$1" ]; then
		echo "安装文件检查失败, 文件:$1 不存在!"
		failed_my_exit "$2" 
	fi
}

function yn_exit()
{
	read -p "$1" op
	while [ "$op" != "Y" -a "$op" != "y" -a "$op" != "N" -a "$op" != "n" ]; do
		read -p "请输入 (y/n): " op
	done
	if [ "$op" != "y" -a "$op" != "Y" ];then
		failed_my_exit $2
	fi
}


CUR_OS_INFO=`cat /etc/issue`

MYSQL_PROC_COUNT=`ps -elf |grep mysql|wc -l`
if [ $MYSQL_PROC_COUNT -lt 2 ]; then
	promt_msg="未检测到依赖的第三方服务 mysql, 请确保该服务已安装运行, 是否继续安装(y/n) ?"
	isyes=$(yn_continue "$promt_msg")
	if [ "$isyes" != "yes" ];then
		exit 0
	fi
fi

APACHE_PROC_COUNT=`ps -elf |grep apache|wc -l`
APACHE_PROC_COUNT_2=`ps -elf |grep httpd|wc -l`
if [ $APACHE_PROC_COUNT -lt 2 -a $APACHE_PROC_COUNT_2 -lt 2 ]; then
	promt_msg="未检测到依赖的第三方服务 apache, 是否继续安装(y/n) ?"
	isyes=$(yn_continue "$promt_msg")
	if [ "$isyes" != "yes" ];then
		exit 0
	fi
fi

if [ -z "$SERVER_OUT_IP" ]; then
	isyes=$(yn_continue "外网IP未指定, 如果您需要在外网访问web控制台, 需要指定外网IP, 是否现在指定(y/n)?")
	if [ "$isyes" == "yes" ];then
		while true; do
			read -p "请输入合法IP:" SERVER_OUT_IP
			is_ip_valid $SERVER_OUT_IP
			[ $? -eq 0 ] && break
		done
		echo "您输入的外网IP 为: $SERVER_OUT_IP, 如输错再次安装时, 可通过脚本中的配置: SERVER_OUT_IP 指定"
		isyes=$(yn_continue "是否继续安装(y/n) ?")
		[ "$isyes" != "yes" ] && exit 0
	fi
fi

echo "show databases" | mysql -B |grep mysql > /dev/null 2>&1
if [ $? -ne 0 -a -z "$MYSQL_USER" ]; then
	echo ""
	isyes=$(yn_continue "mysql 本地匿名账号不可用, 是否现在指定 mysql 操作账号 (y/n) ?")
	if [ "$isyes" == "yes" ];then
		read -p "请输入 MySQL 操作账号名:" MYSQL_USER
		read -p "请输入 MySQL 操作账号密码:" MYSQL_PASS
		echo "您的输入为: $MYSQL_USER / $MYSQL_PASS, 如输错再次安装时, 可通过脚本中的配置: MYSQL_USER MYSQL_PASS 指定"
		isyes=$(yn_continue "是否继续安装(y/n) ?")
		[ "$isyes" != "yes" ] && exit 0
	else
		echo "再次安装时, 您可通过本脚本中的配置: MYSQL_USER, MYSQL_PASS 指定"
		exit 0
	fi
fi

STEP_TOTAL=8
CUR_STEP=1
XRKMONITOR_HTTP=http://open.xrkmonitor.com/xrkmonitor_down

function update_config() 
{
	sed -i "/^SERVER_OUT_IP=/cSERVER_OUT_IP=${SERVER_OUT_IP}" $1 
	sed -i "/^APACHE_DOCUMENT_ROOT=/cAPACHE_DOCUMENT_ROOT=${APACHE_DOCUMENT_ROOT}"  $1 
	sed -i "/^XRKMONITOR_HTML_PATH=/cXRKMONITOR_HTML_PATH=${XRKMONITOR_HTML_PATH}" $1 
	sed -i "/^APACHE_CGI_PATH=/cAPACHE_CGI_PATH=${APACHE_CGI_PATH}" $1 
	sed -i "/^XRKMONITOR_CGI_LOG_PATH=/cXRKMONITOR_CGI_LOG_PATH=${XRKMONITOR_CGI_LOG_PATH}" $1 
	sed -i "/^MYSQL_USER=/cMYSQL_USER=${MYSQL_USER}" $1 
	sed -i "/^MYSQL_PASS=/cMYSQL_PASS=${MYSQL_PASS}" $1 
}

if [ $# -eq 1 -a "$1" == "new" ]; then 
	rm xrkmonitor_lib.tar.gz > /dev/null 2>&1
	rm slog_all.tar.gz > /dev/null 2>&1
	rm slog_run_test > /dev/null 2>&1
	rm uninstall_xrkmonitor.sh > /dev/null 2>&1
	echo "开始全新安装: 字符云监控系统, 共 $STEP_TOTAL 步"
else
	echo "开始自动安装: 字符云监控系统, 共 $STEP_TOTAL 步"
fi


echo ""
echo "STEP: ($CUR_STEP/$STEP_TOTAL) 下载卸载脚本: uninstall_xrkmonitor.sh"
if [ ! -f uninstall_xrkmonitor.sh ];then
	wget ${XRKMONITOR_HTTP}/uninstall_xrkmonitor.sh
	if [ ! -f uninstall_xrkmonitor.sh ]; then
		echo "下载文件 uninstall_xrkmonitor.sh 失败!"
		failed_my_exit $LINENO
	fi
	chmod +x uninstall_xrkmonitor.sh
else
	echo "文件: uninstall_xrkmonitor.sh 已存在"	
fi
CUR_STEP=`expr 1 + $CUR_STEP`
update_config uninstall_xrkmonitor.sh


function auto_detect_apache_doc_root()
{
	if [ -z "$APH_SERVER_CONFIG_PATH" -o ! -d "$APH_SERVER_CONFIG_PATH" ]; then
		sAphServerCfgFile=`apachectl -V|grep SERVER_CONFIG_FILE|awk -F "=" '{print $2}'`
		APH_SERVER_CONFIG_FILE=${sAphServerCfgFile//\"/}
		if [ ! -f "$APH_SERVER_CONFIG_FILE" ]; then
			sAphServerCfgFile=`apachectl -V|grep HTTPD_ROOT|awk -F "=" '{print $2}'`
			APH_SERVER_CONFIG_PATH=${sAphServerCfgFile//\"/}
		else
			APH_SERVER_CONFIG_PATH=`dirname $APH_SERVER_CONFIG_FILE`
		fi
	fi

	if [ -d "$APH_SERVER_CONFIG_PATH" ]; then
		sAphConfList=`find $APH_SERVER_CONFIG_PATH -name *.conf`
		sDocRoot=`grep -v "[[:space:]]*#" $sAphConfList |grep DocumentRoot |awk '{if(FNR==1) print $0}' |awk -F ":" '{print $2}' |awk '{print $2 }'`
		APACHE_DOCUMENT_ROOT=${sDocRoot//\"/}
	else
		sDocRoot=`apachectl -t -D DUMP_RUN_CFG 2>/dev/null |grep "Main DocumentRoot"|awk '{print $3}'`
		APACHE_DOCUMENT_ROOT=${sDocRoot//\"/}
	fi

	if [ ! -d "$APACHE_DOCUMENT_ROOT" ]; then
		echo "尝试自动探测 apache 网站根目录失败, 请手动指定安装配置: APACHE_DOCUMENT_ROOT 后再试"
		failed_my_exit $LINENO 
	fi
	echo "成功探测到 apache 网站根目录: $APACHE_DOCUMENT_ROOT"
	sed -i "/^APACHE_DOCUMENT_ROOT=/cAPACHE_DOCUMENT_ROOT=${APACHE_DOCUMENT_ROOT}" uninstall_xrkmonitor.sh 
}

function auto_detect_apache_cgi_path()
{
	if [ -z "$APH_SERVER_CONFIG_PATH" -o ! -d "$APH_SERVER_CONFIG_PATH" ]; then
		sAphServerCfgFile=`apachectl -V|grep SERVER_CONFIG_FILE|awk -F "=" '{print $2}'`
		APH_SERVER_CONFIG_FILE=${sAphServerCfgFile//\"/}
		if [ ! -f "$APH_SERVER_CONFIG_FILE" ]; then
			sAphServerCfgFile=`apachectl -V|grep HTTPD_ROOT|awk -F "=" '{print $2}'`
			APH_SERVER_CONFIG_PATH=${sAphServerCfgFile//\"/}
		else
			APH_SERVER_CONFIG_PATH=`dirname $APH_SERVER_CONFIG_FILE`
		fi
	fi

	if [ -d "$APH_SERVER_CONFIG_PATH" ]; then
		sAphConfList=`find $APH_SERVER_CONFIG_PATH -name *.conf`
		sCgiPathInfo=`grep -v "[[:space:]]*#" $sAphConfList |grep ScriptAlias |awk '{if(FNR==1) print $0}' `

		sCgiPath=`echo "$sCgiPathInfo" |awk -F ":" '{print $2}' |awk '{print $3}'`
		APACHE_CGI_PATH=${sCgiPath//\"/}
		sCgiAccessPath=`echo "$sCgiPathInfo" |awk -F ":" '{print $2}' |awk '{print $2}'`
		APACHE_CGI_ACCESS_PATH=${sCgiAccessPath//\"/}

		if [ ! -d "$APACHE_CGI_PATH" -o -z "$APACHE_CGI_ACCESS_PATH" ]; then
			echo "尝试探测 cgi 目录失败, 请手动指定配置: APACHE_CGI_PATH/APACHE_CGI_ACCESS_PATH 后再试"
			failed_my_exit $LINENO
		fi
	else
		echo "尝试自动探测 cgi 目录失败, 请手动指定配置: APACHE_CGI_PATH/APACHE_CGI_ACCESS_PATH 后再试"
		failed_my_exit $LINENO
	fi

	echo "成功探测到 cgi 绝对路径目录: $APACHE_CGI_PATH"
	sed -i "/^APACHE_CGI_PATH=/cAPACHE_CGI_PATH=${APACHE_CGI_PATH}" uninstall_xrkmonitor.sh
	echo "成功探测到 cgi 访问路径: $APACHE_CGI_ACCESS_PATH"
	sed -i "/^APACHE_CGI_ACCESS_PATH=/cAPACHE_CGI_ACCESS_PATH=${APACHE_CGI_ACCESS_PATH}" uninstall_xrkmonitor.sh
}


echo ""
echo "STEP: ($CUR_STEP/$STEP_TOTAL) 安装环境检测"
if [ ! -d "$APACHE_DOCUMENT_ROOT" ]; then
	echo "apache 网站根目录: $APACHE_DOCUMENT_ROOT 不存在, 尝试自动探测"
	auto_detect_apache_doc_root
fi

if [ ! -d "$APACHE_CGI_PATH" ]; then
	echo "apache 网站 cgi 目录: $APACHE_CGI_PATH 不存在, 尝试自动探测"
	auto_detect_apache_cgi_path	
fi

if [ ! -d "$SLOG_SERVER_FILE_PATH" ]; then
	echo "日志中心日志文件目录: $SLOG_SERVER_FILE_PATH 不存在, 尝试创建" 
	mkdir -p "$SLOG_SERVER_FILE_PATH"
	if [ ! -d "$SLOG_SERVER_FILE_PATH" ]; then
		echo "新建目录: $SLOG_SERVER_FILE_PATH 失败, 如无权限请授权后再试"
		yn_exit "是否现在重试(y/n) ?" $LINENO 
		mkdir -p "$SLOG_SERVER_FILE_PATH"
		if [ ! -d "$SLOG_SERVER_FILE_PATH" ]; then
			echo "新建 cgi 本地日志文件目录: $SLOG_SERVER_FILE_PATH 失败 !"
			failed_my_exit $LINENO 
		fi
	fi
	echo "新建日志中心日志文件目录: $SLOG_SERVER_FILE_PATH 成功"
	chmod 755 "$XRKMONITOR_CGI_LOG_PATH"
	if [ $? -ne 0 ]; then
		yn_exit "修改日志中心日志文件目录权限失败, 请确保cgi有读权限, 是否继续(y/n)" $LINENO 
	fi
fi


if [ ! -d "$APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH" ]; then
	echo "html/js 文件安装目录 $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH 不存在, 尝试创建"
	mkdir -p "$APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH"
	if [ ! -d "$APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH" ]; then
		echo "新建目录: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH 失败, 如无权限请授权后再试"
		yn_exit "是否现在重试(y/n) ?" $LINENO 
		mkdir -p "$APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH"
		if [ ! -d "$APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH" ]; then
			echo "新建 html/js 文件目录: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH 失败 !"
			failed_my_exit $LINENO 
		fi
	fi
	echo "新建 html/js 文件目录: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH 成功"
fi

if [ ! -d "$XRKMONITOR_CGI_LOG_PATH" ]; then
	echo "cgi 本地日志文件目录: $XRKMONITOR_CGI_LOG_PATH 不存在, 尝试创建" 
	mkdir -p "$XRKMONITOR_CGI_LOG_PATH"
	if [ ! -d "$XRKMONITOR_CGI_LOG_PATH" ]; then
		echo "新建目录: $XRKMONITOR_CGI_LOG_PATH 失败, 如无权限请授权后再试"
		yn_exit "是否现在重试(y/n) ?" $LINENO 
		mkdir -p "$XRKMONITOR_CGI_LOG_PATH"
		if [ ! -d "$XRKMONITOR_CGI_LOG_PATH" ]; then
			echo "新建 cgi 本地日志文件目录: $XRKMONITOR_CGI_LOG_PATH 失败 !"
			failed_my_exit $LINENO 
		fi
	fi
	echo "新建 cgi 本地日志文件目录: $XRKMONITOR_CGI_LOG_PATH 成功"
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
echo "STEP: ($CUR_STEP/$STEP_TOTAL) 下载并解压库文件: xrkmonitor_lib.tar.gz"
if [ ! -f xrkmonitor_lib.tar.gz ]; then
	wget ${XRKMONITOR_HTTP}/xrkmonitor_lib.tar.gz 
	if [ ! -f xrkmonitor_lib.tar.gz ]; then
		echo "下载文件: xrkmonitor_lib.tar.gz 失败!"
		failed_my_exit $LINENO
	fi
else
	echo "文件: xrkmonitor_lib.tar.gz 已存在"
fi
check_file xrkmonitor_lib.tar.gz $LINENO
tar -zxf xrkmonitor_lib.tar.gz
check_file xrkmonitor_lib/slog_tool $LINENO
check_file xrkmonitor_lib/libmtreport_api-1.1.0.so $LINENO

if [ ! -f /etc/ld.so.conf ]; then
	echo "动态链接库配置文件: /etc/ld.so.conf 不存在!"
	failed_my_exit $LINENO
else
	cur_xrkmonitor_lib=`cat /etc/ld.so.conf |grep xrkmonitor_lib`
	if [ $? -ne 0 -o "$cur_xrkmonitor_lib" != "${install_sh_home}/xrkmonitor_lib" ]; then
		if [ ! -z "$cur_xrkmonitor_lib" ]; then
			sed -i '/xrkmonitor_lib/d' /etc/ld.so.conf
		fi
		echo ${install_sh_home}/xrkmonitor_lib >> /etc/ld.so.conf
	fi
	ldconfig
fi

echo "运行测试文件: slog_tool(slog_run_test)"
cp xrkmonitor_lib/slog_tool slog_run_test
chmod +x slog_run_test
XRKMONITOR_LIBS="libneo_utl libneo_cgi libneo_cs libcgicomm libmysqlwrapped libmtreport_api_open libSockets libmyproto libmtreport_api"
ldd slog_run_test > _ldd_slog_run_test 2>&1
THIRD_LIBS=`grep -v GLIBC _ldd_slog_run_test |grep xrkmonitor_lib|awk '{print $1}'|awk -F "." -v mylib="$XRKMONITOR_LIBS" '{if(!match(mylib, $1)) print $0; }'`
THIRD_LIBS_OTHER=`grep -v GLIBC _ldd_slog_run_test |grep "not found"|awk '{print $1}'`
THIRD_LIBS="$THIRD_LIBS $THIRD_LIBS_OTHER"
rm _ldd_slog_run_test >/dev/null 2>&1

if [ ! -z "$THIRD_LIBS" ]; then
	NEW_THIRD_LIBS=''
	for third_lib in $THIRD_LIBS
	do
		ldconfig -p |grep $third_lib |grep -v xrkmonitor_lib  > /dev/null 2>&1
		if [ $? -eq 0 ]; then
			third_lib_base=`echo $third_lib|awk -F "." '{print $1}'`
			rm xrkmonitor_lib/$third_lib_base*
		else
			NEW_THIRD_LIBS="$NEW_THIRD_LIBS $third_lib"
		fi
	done
fi
./slog_run_test run_test > _run_test_tmp 2>&1
if [ $? -ne 0 ]; then
	cat _run_test_tmp
	echo "运行测试文件:./slog_run_test run_test 失败"
	grep "GLIBC" _run_test_tmp|grep "not found" > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "您的系统 glibc 版本太低, 您可以升级系统或者下载源码后编译安装"
		rm _run_test_cmp > /dev/null 2>&1
	elif [ ! -z "$NEW_THIRD_LIBS" ]; then
		echo "依赖以下第三方库文件, 您可以在安装相应库后重试"
		echo "$NEW_THIRD_LIBS"
	fi
	rm _run_test_tmp > /dev/null 2>&1
	failed_my_exit $LINENO
fi
rm _run_test_tmp > /dev/null 2>&1

if [ ! -z "$NEW_THIRD_LIBS" ]; then
	echo ""
	echo "在您的系统中未检测到以下第三方库:" 
	echo "$NEW_THIRD_LIBS"
	usexrk=$(yn_continue "我们已提供这些库且可使用但不保证跟您的系统完全兼容, 是否继续(y/n)?")
	if [ "$usexrk" != "yes" ]; then
		echo "您可以在安装这些依赖库后继续执行字符云监控系统安装, 感谢您的使用."
		failed_my_exit $LINENO
	fi
fi
CUR_STEP=`expr 1 + $CUR_STEP`


echo ""
echo "STEP: ($CUR_STEP/$STEP_TOTAL) 下载并解压安装包文件: slog_all.tar.gz, 请您耐心等待..."
if [ ! -f slog_all.tar.gz ]; then
	wget ${XRKMONITOR_HTTP}/slog_all.tar.gz
	if [ ! -f slog_all.tar.gz ]; then
		echo "下载文件: slog_all.tar.gz 失败"
		failed_my_exit $LINENO
	fi
else
	echo "文件: slog_all.tar.gz 已存在"
fi
tar -zxf slog_all.tar.gz 
CUR_STEP=`expr 1 + $CUR_STEP`


echo ""
if [ -z "$MYSQL_USER" -o -z "$MYSQL_PASS" ]; then
	echo "STEP: ($CUR_STEP/$STEP_TOTAL) 开始尝试使用匿名账号初始化 mysql 数据库"
	MYSQL_CONTEXT="mysql -B "
else 
	echo "STEP: ($CUR_STEP/$STEP_TOTAL) 开始使用指定账号($MYSQL_USER|$MYSQL_PASS)初始化 mysql 数据库"
	MYSQL_CONTEXT="mysql -B -u$MYSQL_USER -p$MYSQL_PASS"
fi
check_file db/mtreport_db.sql $LINENO 
check_file db/attr_db.mysql $LINENO 
${MYSQL_CONTEXT} < db/mtreport_db.sql
${MYSQL_CONTEXT} < db/attr_db.mysql
echo "show databases" | ${MYSQL_CONTEXT} |grep mtreport_db > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "导入 mysql 数据库 mtreport_db/attr_db 失败"
	failed_my_exit $LINENO
fi
   
XRKMONITOR_DB_TABLES='flogin_history flogin_user mt_app_info mt_attr  mt_attr_type  mt_log_config mt_machine mt_module_info mt_server mt_table_upate_monitor mt_view  mt_view_battr mt_view_bmach mt_warn_config mt_warn_info  test_key_list'
for xtable in $XRKMONITOR_DB_TABLES
do
	echo "show create table $xtable" | ${MYSQL_CONTEXT} mtreport_db > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "数据库导入失败 - 导入数据库:mtreport_db, 表: $xtable 失败"
		failed_my_exit $LINENO 
	fi
done
XRKMONITOR_DB_ATTR_TABLES='table_info table_info_day'
for xtable in $XRKMONITOR_DB_ATTR_TABLES
do
	echo "show create table $xtable" | ${MYSQL_CONTEXT} attr_db > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "数据库导入失败 - 导入数据库:attr_db, 表: $xtable 失败"
		failed_my_exit $LINENO 
	fi
done
echo "导入 mysql 数据库 mtreport_db/attr_db 成功"

echo "grant ALL PRIVILEGES ON mtreport_db.* to 'mtreport'@'localhost' identified by 'mtreport875' WITH GRANT OPTION;" | ${MYSQL_CONTEXT} 
echo "grant ALL PRIVILEGES ON attr_db.* to 'mtreport'@'localhost' identified by 'mtreport875' WITH GRANT OPTION;" | ${MYSQL_CONTEXT} 
echo "show tables" | mysql -B -umtreport -pmtreport875 mtreport_db |grep flogin_user > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "授权默认账号:mtreport 访问 mysql 数据库失败"
	failed_my_exit $LINENO 
fi

echo "数据库 mtreport_db 重新初始化"
XRK_MYSQL_CONTEXT="mysql -B -umtreport -pmtreport875 mtreport_db "
echo "delete from flogin_history" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1
echo "delete from mt_machine" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1
echo "delete from test_key_list" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1
echo "delete from mt_table_upate_monitor" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1
echo "delete from mt_view_bmach" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1
echo "delete from mt_warn_info" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1

echo "update mt_server set ip = '$LOCAL_IP'" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1
LOCAL_IP_DEC=`./slog_run_test show ips2d $LOCAL_IP |awk '{if(NF==3) print $1}'`
if [ ! -z "$SERVER_OUT_IP" ]; then
	LOCAL_IP_DEC_OUT=`./slog_run_test show ips2d $SERVER_OUT_IP|awk '{if(NF==3) print $1}'`
	echo "update mt_server set ip = '$SERVER_OUT_IP' where type=1" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1
fi
if [ "$LOCAL_IP_DEC" != "$LOCAL_IP_DEC_OUT" -a ! -z "$LOCAL_IP_DEC_OUT" ]; then
	echo "insert into mt_machine set name='$LOCAL_IP',ip1=$LOCAL_IP_DEC,ip2=$LOCAL_IP_DEC_OUT,create_time=now(),mod_time=now(),machine_desc='auto add'" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1
else
	echo "insert into mt_machine set name='$LOCAL_IP',ip1=$LOCAL_IP_DEC,create_time=now(),mod_time=now(),machine_desc='auto add'" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1
fi

echo "select id from mt_machine where ip1=$LOCAL_IP_DEC" | $XRK_MYSQL_CONTEXT > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "初始化字符云监控数据库失败"
	failed_my_exit $LINENO
fi
CUR_STEP=`expr 1 + $CUR_STEP`


echo ""
echo "STEP: ($CUR_STEP/$STEP_TOTAL) 修改字符云监控服务相关配置"
sed -i "/^SERVER_MASTER/cSERVER_MASTER $LOCAL_IP" slog_mtreport_client/slog_mtreport_client.conf
CUR_CGI_LOG_PATH=`cat cgi_fcgi/slog_flogin.conf |grep SLOG_LOG_FILE|awk '{print $2}'|xargs dirname`
if [ "$CUR_CGI_LOG_PATH" != "$XRKMONITOR_CGI_LOG_PATH" ]; then
	echo "更新 CGI 文件日志文件路径"
	OLD_CGI_LOG_PATH=${CUR_CGI_LOG_PATH//\//\\\/}
	NEW_CGI_LOG_PATH=${XRKMONITOR_CGI_LOG_PATH//\//\\\/}
	sed -i "s/${OLD_CGI_LOG_PATH}/${NEW_CGI_LOG_PATH}/g" `ls cgi_fcgi/*.conf -1`
fi
CUR_CS_PATH=`cat cgi_fcgi/slog_flogin.conf |grep CS_PATH|awk '{print $2}'`
if [ "$CUR_CS_PATH" != "$APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH/" ]; then
	sXrkmonitorHtmlPath=${XRKMONITOR_HTML_PATH//\//\\\/}
	sed -i "s/\/xrkmonitor/\/$sXrkmonitorHtmlPath/g" `ls cgi_fcgi/*.conf -1`
	sApacheDocRootCs=${APACHE_DOCUMENT_ROOT//\//\\\/}
	sed -i "/^CS_PATH/cCS_PATH $sApacheDocRootCs\/$sXrkmonitorHtmlPath/" `ls cgi_fcgi/*.conf -1`
	sed -i "s/\/xrkmonitor/\/$sXrkmonitorHtmlPath/g" html/index.html
fi
if [ "$APACHE_CGI_ACCESS_PATH" != "/cgi-bin/" ]; then
	sCgiAccessPath=${APACHE_CGI_ACCESS_PATH//\//\\\/}
	sed -i "s/\/cgi-bin\//$sCgiAccessPath/g" `ls cgi_fcgi/*.conf -1`
	sed -i "s/\/cgi-bin\//$sCgiAccessPath/g" html/index.html
fi
CUR_STEP=`expr 1 + $CUR_STEP`


echo ""
echo "STEP: ($CUR_STEP/$STEP_TOTAL) 开始安装字符云监控 web 控制台"
cp html/* $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH -fr
check_file $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH/dmt_login.html $LINENO 
cp $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH/index.html $APACHE_DOCUMENT_ROOT
check_file $APACHE_DOCUMENT_ROOT/index.html $LINENO 
cp cgi_fcgi/* $APACHE_CGI_PATH -fr
check_file $APACHE_CGI_PATH/mt_slog_monitor $LINENO 
CUR_STEP=`expr 1 + $CUR_STEP`


echo ""
echo "STEP: ($CUR_STEP/$STEP_TOTAL) 开始启动字符云监控后台服务, 请您耐心等待..."
check_file slog_write/slog_write.conf $LINENO
if [ "$SLOG_SERVER_FILE_PATH" != "/home/mtreport/slog/" ]; then
	sCurLogServerPath=${SLOG_SERVER_FILE_PATH//\//\\\/}
	sed -i "/^SLOG_SERVER_FILE_PATH=/cSLOG_SERVER_FILE_PATH ${sCurLogServerPath}" uninstall_xrkmonitor.sh
	sed -i "/^SLOG_SERVER_FILE_PATH/cSLOG_SERVER_FILE_PATH ${sCurLogServerPath}" slog_write/slog_write.conf
fi

cd tools_sh
check_file stop_all.sh $LINENO
ls -l /tmp/pid*slog*pid >/dev/null 2>&1
if [ $? -eq 0 ]; then
	./stop_all.sh
fi

check_file check_proc_monitor.sh $LINENO
./check_proc_monitor.sh 1
usleep 50000 > /dev/null 2>&1 || sleep 1
echo "开始检测确认字符云监控后台服务运行是否正常"
check_file /tmp/pid.slog_config.pid $LINENO
check_file /tmp/pid.slog_client.pid $LINENO 
check_file /tmp/pid.slog_check_warn.pid $LINENO 
check_file /tmp/pid.slog_deal_warn.pid $LINENO 
check_file /tmp/pid.slog_monitor_server.pid $LINENO 
check_file /tmp/pid.slog_mtreport_server.pid $LINENO 
check_file /tmp/pid.slog_server.pid $LINENO 
check_file /tmp/pid.slog_write.pid $LINENO 

check_file add_crontab.sh $LINENO
isyes=$(yn_continue "服务启动成功, 是否添加字符云监控服务自动拉起脚本到 crontab(y/n) ?")
if [ "$isyes" == "yes" ]; then
	./add_crontab.sh > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "安装字符云监控后台服务自动拉起脚本:add_crontab.sh 到 crontab 失败,请您手动安装"
	else
		echo "安装字符云监控后台服务自动拉起脚本: add_crontab.sh 到 crontab 成功"
	fi
fi
CUR_STEP=`expr 1 + $CUR_STEP`


echo ""
echo "本次安装信息如下: "
echo "---------------------------------------------------------------------------------"
echo "	apache 网站根目录: $APACHE_DOCUMENT_ROOT"
echo "	apache cgi 目录: $APACHE_CGI_PATH"
echo "	apache cgi 访问路径: $APACHE_CGI_ACCESS_PATH"
echo "	apache cgi 本地日志目录: $XRKMONITOR_CGI_LOG_PATH (cgi需要读写权限)"
echo "	监控系统 html/js 文件目录: $APACHE_DOCUMENT_ROOT/$XRKMONITOR_HTML_PATH"
echo "	监控系统日志中心日志目录: $SLOG_SERVER_FILE_PATH (cgi需要读权限)"
echo "	本机IP: $LOCAL_iP, 本机外网IP: $SERVER_OUT_IP"
echo "	监控系统动态链接库目录: $install_sh_home/xrkmonitor_lib"
echo "---------------------------------------------------------------------------------"
echo "如以上信息有误, 或者您想更改, 请先执行卸载脚本: uninstall_xrkmonitor.sh "
echo "然后修改安装脚本中的相关配置后再次执行安装脚本: online_install.sh"
echo "感谢您的使用, 联系我们: Q群(699014295), email(1820140912@qq.com)"
echo ""

apachectl -t -D DUMP_MODULES |grep cgi_module > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "未检测到 apache 模块: cgi_module, 请修改配置加载后重启 apache 服务. " 
else
	isyes=$(yn_continue "需要重启下 apache, 是否现在重启(y/n) ?")
	if [ "$isyes" == "yes" ]; then
		apachectl restart > /dev/null 2>&1
		[ $? -ne 0 ] && echo "重启 apache 服务失败, 请您手动重启下"
	else
		if [ -z "$SERVER_OUT_IP" ]; then
			echo "恭喜您, 在线安装完成, 现在您可以在浏览器中访问控制台了, 访问网址: http://$LOCAL_IP"
			echo "约 1 分钟左右, 您可以在字符云监控系统 web 控制台上查看监控系统本身的数据上报"
			echo " ---------------------------------------------------------------------------------"
			echo "特别提示: 如果您的服务器是云服务器, 且不能通过网址: http://$LOCAL_IP 访问web控制台"
			echo "您可以在本脚本中的配置: SERVER_OUT_IP 指定外网IP后, 再次执行本脚本"
		else
			echo "恭喜您, 在线安装完成, 现在您可以在浏览器中访问控制台了, 访问网址: http://$SERVER_OUT_IP"
			echo "约 1 分钟左右, 您可以在字符云监控系统 web 控制台上查看监控系统本身的数据上报"
		fi
	fi
fi
echo ""

cd $install_sh_home
check_file online_install.sh $LINENO
cp online_install.sh online_install.sh_bk
update_config online_install.sh_bk
mv online_install.sh_bk online_install.sh


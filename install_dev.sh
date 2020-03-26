#!/bin/bash

SUFFIX=xrkmonitor.com
cdir=`pwd`

STEP_TOTAL=5

# mysql 开发环境 check
MYSQL_INCLUDE=`cat make_env |grep ^MYSQL_INCLUDE|awk '{print $3}'`
MYSQL_LIB=`cat make_env |grep ^MYSQL_LIB|awk '{print $3}'`
MTLIB_INCLUDE_PATH=`cat make_env |grep ^MTLIB_INCLUDE_PATH|awk '{print $3}'`
MTLIB_LIB_PATH=`cat make_env |grep ^MTLIB_LIB_PATH|awk '{print $3}'`
# check mysql 头文件路径是否 OK
if [ ! -f "${MYSQL_INCLUDE}/mysql/mysql.h" ]; then
	echo "(1/$STEP_TOTAL) not find file:${MYSQL_INCLUDE}/mysql/mysql.h, check mysql include path failed !"
	exit 1
fi
# check mysql 库文件路径是否 OK
if [ ! -f "${MYSQL_LIB}/libmysqlclient.so" ]; then
	if [ -f /usr/lib64/libmysqlclient.so ]; then
		sed -i '/^MYSQL_LIB/cMYSQL_LIB = \/usr\/lib64' make_env
	elif [ -f /usr/lib/libmysqlclient.so ]; then
		sed -i '/^MYSQL_LIB/cMYSQL_LIB = \/usr\/lib' make_env
	elif [ -f /usr/lib/x86_64-linux-gnu/libmysqlclient.so ]; then
		sed -i '/^MYSQL_LIB/cMYSQL_LIB = \/usr\/lib\/x86_64-linux-gnu' make_env
	elif [ -f /usr/lib/i386-linux-gnu/libmysqlclient.so ]; then
		sed -i '/^MYSQL_LIB/cMYSQL_LIB = \/usr\/lib\/i386-linux-gnu' make_env
	else
		echo "(1/$STEP_TOTAL) not find file:${MYSQL_LIB}/libmysqlclient.so, check mysql lib path failed !"
		exit 2
	fi
fi
# 公共库/头文件安装路径
if [ ! -d "${MTLIB_INCLUDE_PATH}" ];then
	sed -i '/^MTLIB_INCLUDE_PATH/cMTLIB_INCLUDE_PATH = \/usr\/include' make_env
fi
if [ ! -d "${MTLIB_LIB_PATH}" ];then
	if [ -d /usr/lib64 -o -d /lib64 ]; then
		sed -i '/^MTLIB_LIB_PATH/cMTLIB_LIB_PATH = \/usr\/lib64' make_env
	else
		sed -i '/^MTLIB_LIB_PATH/cMTLIB_LIB_PATH = \/usr\/lib' make_env
	fi
fi

echo ""
echo "include dir:${MTLIB_INCLUDE_PATH}, lib dir:${MTLIB_LIB_PATH}"
echo "(1/$STEP_TOTAL) mysql devel check ok"

function yn_continue()
{
	read -p "$1" op
	while [ "$op" != "Y" -a "$op" != "y" -a "$op" != "N" -a "$op" != "n" ]; do
		read -p "please input (y/n): " op
	done
	if [ "$op" != "y" -a "$op" != "Y" ];then
		echo "no" 
	else
		echo "yes"
	fi
}

function InstallFailed()
{
	echo ""
	isyes=$(yn_continue "$1 failed, continue(y/n)? ")
	if [ "$isyes" != "yes" ];then
		exit -1 
	fi
}

# 安装 protobuf
function InstallProtobuf()
{
	echo ""
	isyes=$(yn_continue "(2/$STEP_TOTAL) install protobuf(y/n)? ")
	if [ "$isyes" != "yes" ];then
		return;
	fi

	if [ ! -f  ${MTLIB_LIB_PATH}/libprotobuf.a ]; then
		cd $cdir/lib/protobuf
		tar -zxf protobuf-2.3.0.tar.gz
		cd protobuf-2.3.0
		./configure --libdir=${MTLIB_LIB_PATH} --includedir=${MTLIB_INCLUDE_PATH}
		make 
		make install
	else
		echo "protobuf is already install ($SUFFIX)"
	fi

	
	if [ ! -f ${MTLIB_LIB_PATH}/libprotobuf.a ]; then
		echo "not find file:${MTLIB_LIB_PATH}/libprotobuf.a "
		InstallFailed "protobuf"
	fi
}

# fastcgi 编译环境
function InstallFastcgiDev()
{
	echo ""
	isyes=$(yn_continue "(3/$STEP_TOTAL) install fastcgi devel(y/n)? ")
	if [ "$isyes" != "yes" ];then
		return;
	fi

	if [ ! -f ${MTLIB_INCLUDE_PATH}/fastcgi/fastcgi.h -o ! -f ${MTLIB_LIB_PATH}/libfcgi.a ]; then
		echo "install fastcgi devel($SUFFIX)"
		cd $cdir/lib/fcgi
		tar -zxf fcgi-2.4.0.tar.gz
		cd fcgi-2.4.0
		./configure --libdir=${MTLIB_LIB_PATH} --includedir=${MTLIB_INCLUDE_PATH}/fastcgi
		make
		make install
	else
		echo "fastcgi devel is already install ($SUFFIX)"
	fi

	if [ ! -f ${MTLIB_INCLUDE_PATH}/fastcgi/fastcgi.h -o ! -f ${MTLIB_LIB_PATH}/libfcgi.a ]; then
		echo "not find file:${MTLIB_INCLUDE_PATH}/fastcgi/fastcgi.h or ${MTLIB_LIB_PATH}/libfcgi.a"
		InstallFailed "fastcgi devel"
	fi
}

# 0 - 交叉依赖预安装头文件
function InstallMysqlwrap()
{
	echo ""
	if [ ! -f ${MTLIB_INCLUDE_PATH}/mysqlwrapped/libmysqlwrapped.h ]; then
		# clearsilver cgi 模板引擎, 首次运行时需要执行下 configure
		cd $cdir/lib/clearsilver; ./configure ; touch _configure_check_
	
		cd $cdir/lib/mysqlwrapped
		echo "(4/$STEP_TOTAL) preinstall mysqlwrapped ($SUFFIX)"
		cat IError.h enum_t.h set_t.h Database.h Query.h > libmysqlwrapped.h
		mkdir -p ${MTLIB_INCLUDE_PATH}/mysqlwrapped
		cp libmysqlwrapped.h ${MTLIB_INCLUDE_PATH}/mysqlwrapped 
	else
		echo "(4/$STEP_TOTAL) mysqlwrapped is already preinstall ($SUFFIX)"
	fi

	if [ ! -f ${MTLIB_INCLUDE_PATH}/mysqlwrapped/libmysqlwrapped.h ]; then
		echo "not find file:${MTLIB_INCLUDE_PATH}/mysqlwrapped/libmysqlwrapped.h"
		InstallFailed "mysqlwrapped"
	fi
}

# 编译生成 memcached 可执行文件
function InstallMemcached()
{
	echo ""
	isyes=$(yn_continue "(5/$STEP_TOTAL) make and install memcached(y/n)? ")
	if [ "$isyes" != "yes" ];then
		return;
	fi

	if [ ! -f $cdir/slog_memcached/slog_memcached ]; then
		echo "make memcached ($SUFFIX)"
		cd $cdir/lib/memcached
		tar -zxf libevent-2.0.21-stable.tar.gz
		tar -zxf memcached-1.5.3.tar.gz
		cd libevent-2.0.21-stable
		./configure  --enable-static --disable-openssl --disable-shared 
		make
		make install

		cd ../memcached-1.5.3
		./configure --prefix=/usr/local/memcached --disable-option-checking --disable-sasl --disable-sasl-pwdb --disable-coverage 
		make
		make install
		cp /usr/local/memcached/bin/memcached $cdir/slog_memcached/slog_memcached
	else
		echo "memcached is already install ($SUFFIX)"
	fi

	if [ ! -f $cdir/slog_memcached/slog_memcached ]; then
		echo "not find file:$cdir/slog_memcached/slog_memcached"
		InstallFailed "memcached"
	fi
}

InstallProtobuf
InstallFastcgiDev
InstallMysqlwrap
InstallMemcached

cd $cdir
touch _install_dev_run_


#!/bin/bash

SUFFIX=xrkmonitor.com
cdir=`pwd`

STEP_TOTAL=6

# mysql 开发环境 check
MYSQL_INCLUDE=`cat make_env |grep MYSQL_INCLUDE|awk '{print $3}'`
MYSQL_LIB=`cat make_env |grep MYSQL_LIB|awk '{print $3}'`
# check mysql 头文件路径是否 OK
if [ ! -f ${MYSQL_INCLUDE}/mysql/mysql.h ]; then
	echo "(1/$STEP_TOTAL) not find file:${MYSQL_INCLUDE}/mysql/mysql.h, check mysql include path failed !"
	exit 1
fi
# check mysql 库文件路径是否 OK
if [ ! -f ${MYSQL_LIB}/libmysqlclient.so ]; then
	echo "(1/$STEP_TOTAL) not find file:${MYSQL_LIB}/libmysqlclient.so, check mysql lib path failed !"
	exit 2
fi
echo "(1/$STEP_TOTAL) mysql devel check ok"

function InstallFailed()
{
	echo ""
	soft=$1
	read -p "$soft failed, continue(y/n)? " op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do 
		read -p "please input y/n: " op
	done
	if [ $op != "y" -a $op != "Y" ];then
		exit -1
	fi
}

# 安装 protobuf
function InstallProtobuf()
{
	echo ""
	read -p "(2/$STEP_TOTAL) install protobuf(y/n)? " op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do 
		read -p "please input y/n: " op
	done
	if [ $op != "y" -a $op != "Y" ];then
		return;
	fi

	if [ ! -f  /usr/lib64/libprotobuf.a ]; then
		cd $cdir/lib/protobuf
		tar -zxf protobuf-2.3.0.tar.gz
		cd protobuf-2.3.0
		./configure --libdir=/usr/lib64 --includedir=/usr/include
		make 
		make install
	else
		echo "protobuf is already install ($SUFFIX)"
	fi

	
	if [ ! -f  /usr/lib64/libprotobuf.a ]; then
		echo "not find file:/usr/lib64/libprotobuf.a "
		InstallFailed "protobuf"
	fi
}

# fastcgi 编译环境
function InstallFastcgiDev()
{
	echo ""
	read -p "(3/$STEP_TOTAL) install fastcgi devel(y/n)? " op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do 
		read -p "please input y/n: " op
	done
	if [ $op != "y" -a $op != "Y" ];then
		return;
	fi

	if [ ! -f /usr/include/fastcgi/fastcgi.h -o ! -f /usr/lib64/libfcgi.a ]; then
		echo "install fastcgi devel($SUFFIX)"
		cd $cdir/lib/fcgi
		tar -zxf fcgi-2.4.0.tar.gz
		cd fcgi-2.4.0
		./configure --libdir=/usr/lib64 --includedir=/usr/include/fastcgi
		make
		make install
	else
		echo "fastcgi devel is already install ($SUFFIX)"
	fi

	if [ ! -f /usr/include/fastcgi/fastcgi.h -o ! -f /usr/lib64/libfcgi.a ]; then
		echo "not find file:/usr/include/fastcgi/fastcgi.h or /usr/lib64/libfcgi.a"
		InstallFailed "fastcgi devel"
	fi
}

# 安装 curl 编译环境
function InstallCurlDev()
{
	echo ""
	read -p "(4/$STEP_TOTAL) install curl devel(y/n)? " op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do 
		read -p "please input y/n: " op
	done
	if [ $op != "y" -a $op != "Y" ];then
		return;
	fi

	if [ ! -f /usr/include/curl/curl.h ]; then
		echo "install curl devel ($SUFFIX)"
		cd $cdir/lib/curl
		tar -zxf curl-7.54.1.tar.gz
		cd curl-7.54.1
		./configure --libdir=/usr/lib64 --includedir=/usr/include
		make
		make install
	else
		echo "curl devel is already install ($SUFFIX)"
	fi

	if [ ! -f /usr/include/curl/curl.h ]; then
		echo "not find file:/usr/include/curl/curl.h"
		InstallFailed "curl devel"
	fi
}

# 0 - 交叉依赖预安装头文件
function InstallMysqlwrap()
{
	echo ""
	if [ ! -f /usr/include/libmysqlwrapped.h ]; then
		# clearsilver cgi 模板引擎, 首次运行时需要执行下 configure
		cd $cdir/lib/clearsilver; ./configure
	
		cd $cdir/lib/mysqlwrapped
		echo "(5/$STEP_TOTAL) preinstall mysqlwrapped ($SUFFIX)"
		cat IError.h enum_t.h set_t.h Database.h Query.h > libmysqlwrapped.h
		cp libmysqlwrapped.h /usr/include/
	else
		echo "(5/$STEP_TOTAL) mysqlwrapped is already preinstall ($SUFFIX)"
	fi

	if [ ! -f /usr/include/libmysqlwrapped.h ]; then
		echo "not find file:/usr/include/libmysqlwrapped.h"
		InstallFailed "mysqlwrapped"
	fi
}

# 编译生成 memcached 可执行文件
function InstallMemcached()
{
	echo ""
	read -p "(6/$STEP_TOTAL) make and install memcached(y/n)? " op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do 
		read -p "please input y/n: " op
	done
	if [ $op != "y" -a $op != "Y" ];then
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
InstallCurlDev
InstallMysqlwrap
InstallMemcached


#!/bin/bash

SUFFIX=xrkmonitor.com
cdir=`pwd`

# mysql 开发环境 check
MYSQL_INCLUDE=`cat make_env |grep MYSQL_INCLUDE|awk '{print $3}'`
MYSQL_LIB=`cat make_env |grep MYSQL_LIB|awk '{print $3}'`
# check mysql 头文件路径是否 OK
if [ ! -f ${MYSQL_INCLUDE}/mysql/mysql.h ]; then
	echo "(1/5) not find file:${MYSQL_INCLUDE}/mysql/mysql.h, check mysql include path failed !"
	exit 1
fi
# check mysql 库文件路径是否 OK
if [ ! -f ${MYSQL_LIB}/libmysqlclient.so ]; then
	echo "(1/5) not find file:${MYSQL_LIB}/libmysqlclient.so, check mysql lib path failed !"
	exit 2
fi
echo "(1/5) mysql devel check ok"

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
	read -p "(2/5) install protobuf(y/n)? " op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do 
		read -p "please input y/n: " op
	done
	if [ $op != "y" -a $op != "Y" ];then
		return;
	fi

	if [ ! -f  /usr/lib64/libprotobuf.a ]; then
		cd $cdir/lib/protobuf-2.3.0
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
	read -p "(3/5) install fastcgi devel(y/n)? " op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do 
		read -p "please input y/n: " op
	done
	if [ $op != "y" -a $op != "Y" ];then
		return;
	fi

	if [ ! -f /usr/include/fastcgi/fastcgi.h -o ! -f /usr/lib64/libfcgi.a ]; then
		echo "install fastcgi devel($SUFFIX)"
		cd $cdir/lib/3rd_source_tar
		tar -zxf fcgi-2.4.0.tar.gz; tar -zxf fcgi-2.4.0.tar.gz
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
	read -p "(4/5) install curl devel(y/n)? " op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do 
		read -p "please input y/n: " op
	done
	if [ $op != "y" -a $op != "Y" ];then
		return;
	fi

	if [ ! -f /usr/include/curl/curl.h ]; then
		echo "install curl devel ($SUFFIX)"
		cd $cdir/lib/curl-7.54.1
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
		echo "(5/5) preinstall mysqlwrapped ($SUFFIX)"
		cat IError.h enum_t.h set_t.h Database.h Query.h > libmysqlwrapped.h
		cp libmysqlwrapped.h /usr/include/
	else
		echo "(5/5) mysqlwrapped is already preinstall ($SUFFIX)"
	fi

	if [ ! -f /usr/include/libmysqlwrapped.h ]; then
		echo "not find file:/usr/include/libmysqlwrapped.h"
		InstallFailed "mysqlwrapped"
	fi
}

InstallProtobuf
InstallFastcgiDev
InstallCurlDev
InstallMysqlwrap


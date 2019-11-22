#!/bin/bash
PATH=/bin:/sbin:/usr/bin/:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin
#
# 检测部署机相关信息, 以便下载相应版本的安装包
#
XRKMONITOR_HTTP=http://open.xrkmonitor.com/cgi-bin/mt_slog_open
ACTION=online_install
DEST_OS_ARC=
DEST_OS=
DEST_GLIBC_VER=

function GetDestOs()
{
	if [ -f /etc/issue ]; then
		OS_INFO_SRC=`cat /etc/issue`
	else
		OS_INFO_SRC=`cat /etc/*-release`
	fi
	OS_LIST=('CentOS' 'Slackware' 'Mint' 'Mageia' 'Arch' 'PCLinuxOS' 'FreeBSD' 'Red Hat' 'Aliyun' 'Fedora' 'Debian' 'openSUSE' 'Ubuntu')
	for os in ${OS_LIST[@]}; do
		echo $OS_INFO_SRC |grep "$os" >/dev/null 2>&1
		[ $? -eq 0 ] && echo "$os" && return
	done
	echo "unknow"
}

function GetDestOsBit()
{
	OS_NAME_INFO=`uname -a`
	OS_ARC_INFO=('x86_64' 'x86_32')

	for arc in ${OS_ARC_INFO[@]}; do
		echo $OS_ARC_INFO|grep "$arc" >/dev/null 2>&1
		[ $? -eq 0 ] && echo "$arc" && return
	done
	echo "unknow"
}

function GetLibcVer()
{
	if [ -d "/lib64" ];then
		echo `strings /lib64/libc.so.* |grep ^GLIBC_[0-9].|sort -r|awk '{if(NR==1) print $0}'`
		return
	elif [ -d "/lib" ]; then
		echo `strings /lib/libc.so.* |grep ^GLIBC_[0-9].|sort -r|awk '{if(NR==1) print $0}'`
		return
	fi
	echo "unknow"
}

DEST_OS_ARC=$(GetDestOsBit)
DEST_OS=$(GetDestOs)
DEST_GLIBC_VER=`strings /lib64/libc.so.6 |grep ^GLIBC_[0-9].|sort -r|awk '{if(NR==1) print $0}'`

wget "$XRKMONITOR_HTTP?action=$ACTION&os=$DEST_OS&os_arc=$DEST_OS_ARC&libcver=$DEST_GLIBC_VER" -O _wget_result



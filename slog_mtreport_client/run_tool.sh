#!/bin/bash
#
# 获取 agent 运行机器的一些系统信息
# 信息使用场景：在一键部署插件的时候云端需要根据这些信息派发与系统匹配的插件部署包
# add by rockdeng @2020-09-04
#

if [ $# -ne 1 ]; then
    echo 'invalid parameter!'
    exit 1
fi

function GetDestOsBit()
{
	OS_NAME_INFO=`uname -a`
	OS_ARC_INFO=('x86_64' 'x86_32')

	for arc in ${OS_ARC_INFO[@]}; do
		echo $OS_NAME_INFO|grep "$arc" >/dev/null 2>&1
		[ $? -eq 0 ] && echo "$arc" && return
	done

	echo ${OS_NAME_INFO} | grep "i*86" > /dev/null 2>&1
	[ $? -eq 0 ] && echo "x86_32" && return
	echo "unknow"
}

function GetDestOs()
{
    ls /etc/*-release > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        OS_INFO_SRC=`cat /etc/*-release`
    else
        OS_INFO_SRC=`cat /etc/issue`
    fi  
    OS_LIST=('CentOS' 'Slackware' 'Mint' 'Mageia' 'Arch' 
		'PCLinuxOS' 'FreeBSD' 'Red Hat' 'Aliyun' 'Fedora' 'Debian' 'openSUSE' 'Ubuntu')

	IFSBAK=$IFS
	IFS=':'
    for os in ${OS_LIST[@]}; do
        echo $OS_INFO_SRC |grep "$os" >/dev/null 2>&1
        [ $? -eq 0 ] && echo "$os" && return
    done
	IFS=$IFSBAK

    echo "unknow"
    exit 0
}

LIBVER_PATH_INFO=('/lib64' '/usr/lib64' '/lib' '/lib64/x86_64-linux-gnu' '/usr/lib' '/lib/i386-linux-gnu')
function GetLibcVer()
{
	for lpath in ${LIBVER_PATH_INFO[@]}; do
		if [ -d "$lpath" ];then
			ls -l $lpath/libc.so.* > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				echo `strings $lpath/libc.so.* |grep ^GLIBC_[0-9].|sort -rV|awk '{if(NR==1) print $0}'`
				return
			fi
		fi
	done
	echo "unknow"
}

function GetLibcppVer()
{
	for lpath in ${LIBVER_PATH_INFO[@]}; do
		if [ -d "$lpath" ];then
			ls -l $lpath/libstdc++.so.* > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				echo `strings $lpath/libstdc++.so.* |grep ^GLIBCXX_[0-9].|sort -rV|awk '{if(NR==1) print $0}'`
				return
			fi
		fi
	done
	echo "unknow"
}

case $1 in
    getos)
    GetDestOs
    exit 0
;;
    getarc)
    GetDestOsBit
    exit 0
;;
    getlibc)
    GetLibcVer
    exit 0
;;
    getlibcpp)
    GetLibcppVer
    exit 0
;;
    -h|--help)
    echo "use ./run_tool.sh - getos/getarc/getlibc/getlibcpp"
    exit 0
;;
*)
    echo 'unknow'
    exit 1
;;
esac



#!/bin/bash

STEP_TOTAL=5
CUR_STEP=1
XRKMONITOR_HTTP=http://open.xrkmonitor.com/xrkmonitor_down
cur_path=`pwd`

function failed_my_exit()
{
	exit $1
}

function check_file()
{
	if [ ! -f $1 ]; then
		echo "安装文件检查失败, 文件:$1 不存在!"
		failed_my_exit $2 
	fi
}

echo "开始自动安装: 字符云监控系统, 共 $STEP_TOTAL 步"
echo "($CUR_STEP/$STEP_TOTAL) 下载并解压库文件: xrkmonitor_lib.tar.gz"
if [ ! -f xrkmonitor_lib.tar.gz ]; then
	wget ${XRKMONITOR_HTTP}/xrkmonitor_lib.tar.gz 
else
	echo "库文件: xrkmonitor_lib.tar.gz 已存在"
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

echo "($CUR_STEP/$STEP_TOTAL) 下载运行测试文件: slog_tool"
if [ ! -f slog_tool ]; then
	wget ${XRKMONITOR_HTTP}/slog_tool
	chmod +x slog_tool
	check_file slog_tool 4
else
	echo "slog_tool 文件已存在"
fi
./slog_tool run_test > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "运行测试文件失败, 您可以手动安装或者加入Q群(699014295) 联系我们"
	failed_my_exit 5
fi
CUR_STEP=`expr 1 + $CUR_STEP`

echo "($CUR_STEP/$STEP_TOTAL) 下载并解压安装包文件: xrkmonitor.tar.gz, 请您耐心等待..."
if [ ! -f xrkmonitor.tar.gz ]; then
	wget ${XRKMONITOR_HTTP}/xrkmonitor.tar.gz
else
	echo "xrkmonitor.tar.gz 文件已存在"
fi

echo "恭喜您, 自动安装完成, 现在您可以在浏览器中访问控制台了"


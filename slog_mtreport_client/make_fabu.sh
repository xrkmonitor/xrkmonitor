#!/bin/bash
# 发布包 agent slog_mtreport_client 打包脚本

if [ ! -f slog_mtreport_client -o ! -f libmtreport/libmtreport_api.so.1 ]; then
	echo 'have no file: slog_mtreport_client or libmtreport/libmtreport_api.so.1'
	exit 1
fi

targetdir=xrkmonitor_agent
mkdir ${targetdir} 
cp slog_mtreport_client ./${targetdir}
cp slog_mtreport_client.conf ./${targetdir}/_slog_mtreport_client.conf
cp libmtreport ./${targetdir} -r
cp xrkmonitor_plus ./${targetdir} -r

tar zcvf  ${targetdir}.tar.gz ./${targetdir}
rm -fr ${targetdir}


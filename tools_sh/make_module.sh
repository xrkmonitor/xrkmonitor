#!/bin/bash
if [ $# -lt 1 ];then
	echo 'use ./make_module.sh module_name'
	exit 1
fi

cd ..
mname=$1
TarF=${mname}.tar.gz
BackupDir=release_pack

if [ ! -f ${mname}/${mname} ]; then
	echo "check file: ${mname}/${mname} failed !"
	exit 2
fi

echo $mname |grep slog_mtreport_client
if [ $? -eq 0 ]; then
	if [ ! -f slog_mtreport_client/libmtreport/libmtreport_api.so.1 ]; then 
		echo 'have no file: slog_mtreport_client/libmtreport/libmtreport_api.so.1' 
		exit 2 
	fi
	tar czf ${TarF} ${mname}/${mname} ${mname}/*.sh ${mname}/${mname}.conf ${mname}/libmtreport ${mname}/xrkmonitor_plus
else
	tar czf ${TarF} ${mname}/${mname} ${mname}/*.sh ${mname}/${mname}.conf
fi

cd tools_sh
mv ../${TarF} .
if [ ! -d ${BackupDir} ]; then
	mkdir ${BackupDir}
fi
CurDate=`date "+%Y%m%d"`
cp ${TarF} ${BackupDir}/${TarF}.$CurDate


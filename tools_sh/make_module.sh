#!/bin/bash
if [ $# -lt 1 ];then
	echo 'use ./make_module.sh module_name'
	exit 1
fi

cd ..
mname=$1
TarF=${mname}.tar.gz
BackupDir=release_pack

tar czf ${TarF} ${mname}/${mname} ${mname}/*.sh ${mname}/${mname}.conf

cd tools_sh
mv ../${TarF} .
if [ ! -d ${BackupDir} ]; then
	mkdir ${BackupDir}
fi
CurDate=`date "+%Y%m%d"`
cp ${TarF} ${BackupDir}/${TarF}.$CurDate


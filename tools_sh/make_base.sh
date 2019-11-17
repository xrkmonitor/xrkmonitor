#!/bin/bash
cd ..
Name=slog_base
TarF=${Name}.tar
TarP=$(pwd)/${TarF}
BackupDir=release_pack

tar cvf ${TarP} tools_sh/rm_zero.sh
tar rvf ${TarP} tools_sh/check_proc_monitor.sh
tar rvf ${TarP} tools_sh/start_all.sh
tar rvf ${TarP} tools_sh/add_crontab.sh
tar rvf ${TarP} tools_sh/stop_all.sh
tar rvf ${TarP} tools_sh/start_comm.sh
tar rvf ${TarP} tools_sh/stop_comm.sh

function make_module()
{
	mname=$1
	
	if [ ! -f ${mname}/${mname} ]; then
		echo "check file: ${mname}/${mname} failed !"
		rm -f ${TarP}
		exit 2
	fi
	tar rvf ${TarP} ${mname}/*.sh
	tar rvf ${TarP} ${mname}/${mname}.conf
	tar rvf ${TarP} ${mname}/${mname}

	echo $mname|grep slog_mtreport_client
	if [ $? -eq 0 ]; then
		tar rvf ${TarP} ${mname}/libmtreport ${mname}/xrkmonitor_plus
	fi
}

make_module slog_mtreport_client
make_module slog_client
make_module slog_config 

cd tools_sh
mkdir _tmp
mv ../${TarF} _tmp
cd _tmp
tar -xf ${TarF}
rm ${TarF}
tar -czf ${TarF}.gz *

cd ..
cp _tmp/${TarF}.gz .
rm -fr _tmp
if [ ! -d ${BackupDir} ]; then
	mkdir -p ${BackupDir}
fi
CurDate=`date "+%Y%m%d"`
cp ${TarF}.gz ${BackupDir}/${TarF}.gz.$CurDate


#!/bin/bash
cd ..
Name=slog_all
TarF=${Name}.tar
TarP=$(pwd)/${TarF}
BackupDir=release_pack

tar cvf ${TarP} tools_sh/rm_zero.sh
tar rvf ${TarP} tools_sh/check_proc_monitor.sh
tar rvf ${TarP} tools_sh/start_all.sh
tar rvf ${TarP} tools_sh/stop_all.sh
tar rvf ${TarP} tools_sh/start_comm.sh
tar rvf ${TarP} tools_sh/stop_comm.sh
tar rvf ${TarP} tools_sh/install_bin.sh

tar rvf ${TarP} cgi_fcgi/* --exclude *.cpp --exclude Makefile --exclude cgi_debug.txt 

dirlist=`find . -maxdepth 1 -type d`
for dr in $dirlist
do 
        if [ -f $dr/$dr ] ; then
                tar rvf ${TarP} $dr/*.sh
                tar rvf ${TarP} $dr/*.conf
                tar rvf ${TarP} $dr/$dr
        fi
done

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


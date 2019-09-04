#!/bin/bash
cd ..
Name=slog_all
TarF=${Name}.tar
TarP=$(pwd)/${TarF}
SaveDir=tools_sh

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

CurDate=`date "+%Y%m%d"`
tar zcvf $Name.tar.gz $Name.tar
cp $Name.tar.gz $SaveDir
rm $Name.tar
rm $Name.tar.gz


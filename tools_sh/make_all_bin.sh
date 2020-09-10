#!/bin/bash
cd ..
Name=slog_all_bin
TarF=${Name}.tar
TarP=$(pwd)/${TarF}
MD5F=slog_all_bin.md5
SaveDir=tools_sh
BackupDir=release_pack

tar cvf ${TarP} cgi_fcgi/slog_flogin
dirlist=`find . -maxdepth 1 -type d`
for dr in $dirlist
do 
	if [ -f $dr/$dr ] ; then
		tar rvf ${TarP} $dr/$dr
		echo $dr |grep slog_config
		if [ $? -eq 0 ]; then
			md5sum $dr/$dr >> $MD5F	
			echo >> $MD5F
			tar rvf ${TarP} $MD5F
		fi
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


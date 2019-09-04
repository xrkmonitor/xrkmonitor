#!/bin/bash
cd ..
Name=slog_all_bin
TarF=${Name}.tar
TarP=$(pwd)/${TarF}
MD5F=slog_all_bin.md5
SaveDir=tools_sh
BackupDir=tools_sh/release_pack

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

CurDate=`date "+%Y%m%d"`
mkdir -p $BackupDir
rm -f $BackupDir/$Name.tar.$CurDate
tar zcvf $Name.tar.gz $Name.tar
cp $Name.tar.gz $SaveDir
cp $Name.tar.gz $BackupDir
cp $Name.tar.gz $BackupDir/$Name.tar.gz.$CurDate
rm $Name.tar
rm $Name.tar.gz
rm $MD5F


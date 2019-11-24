#!/bin/bash
USE_DLL_COMM_LIB=`cat ../make_env |grep ^USE_DLL_COMM_LIB|awk '{print $3}'`
if [ "${USE_DLL_COMM_LIB}" != 'yes' ]; then
	exit 0
fi
MTLIB_LIB_PATH=`cat ../make_env |grep MTLIB_LIB_PATH|awk '{print $3}'`
LIB_DEST_PATH=./xrkmonitor_lib
cdir=`pwd`

# 运行测试文件
if [ ! -f ../slog_tool/slog_tool ]; then
	echo "file : ../slog_tool/slog_tool  not exist"
	exit 2
fi

if [ ! -d ${LIB_DEST_PATH} ]; then
	mkdir -p ${LIB_DEST_PATH}
fi
rm -fr ${LIB_DEST_PATH}/*
cp ../slog_tool/slog_tool ${LIB_DEST_PATH}

COPY_LIBS_INFO=('libmysqlclient' 'libssl' 'libcrypto' 'libz' 'libdl' 'libmysqlwrapped' 'libmtreport_api_open' 'libSockets' 'libmyproto' 'libprotobuf' 'libmtreport_api' 'libfcgi' 'libneo_cgi' 'libneo_cs' 'libneo_utl' 'libcgicomm')
cd ${LIB_DEST_PATH}; rm -fr *

function copy_lib_to_dest()
{
	ls -l $1 |grep "^-" > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		cp $1 .
	else 
		ls -l $1 |grep "^l" > /dev/null 2>&1
		if [ $? -eq 0 ]; then
			local ldstfile=`basename $1`
			local ldirname=`dirname $1`
			local lsrcfile=`ls -l $1 |awk '{print $NF}'`

			echo $lsrcfile |grep "^/" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				copy_lib_to_dest $lsrcfile
			else
				copy_lib_to_dest $ldirname/$lsrcfile
			fi
			if [ $? -ne 0 ]; then
				return 1;
			fi
			ln -s $lsrcfile $ldstfile
		else
			return 1
		fi
	fi
	return 0
}

for lib in ${COPY_LIBS_INFO[@]}; do
	dlib=`ldd ${cdir}/../cgi_fcgi/slog_flogin|grep $lib.so`
	if [ $? -ne 0 -o -z "$dlib" ]; then
		echo "copy lib: $lib failed, not find !"
		exit 2
	fi
 	dfile=`echo $dlib|awk '{print $3}'`
	if [ $? -eq 0 -a ! -z "$dfile" ]; then
		copy_lib_to_dest $dfile
	fi
done

cd $cdir
tar -czf xrkmonitor_lib.tar.gz $LIB_DEST_PATH


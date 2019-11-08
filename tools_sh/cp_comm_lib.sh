#!/bin/bash
USE_DLL_COMM_LIB=`cat ../make_env |grep ^USE_DLL_COMM_LIB|awk '{print $3}'`
if [ "${USE_DLL_COMM_LIB}" != 'yes' ]; then
	exit 0
fi
MTLIB_LIB_PATH=`cat ../make_env |grep MTLIB_LIB_PATH|awk '{print $3}'`
LIB_DEST_PATH=./xrkmonitor_lib

if [ ! -d ${LIB_DEST_PATH} ]; then
	mkdir -p ${LIB_DEST_PATH}
fi
cd ${LIB_DEST_PATH}
rm -fr *

function check_lib_exit()
{
	if [ ! -f $1 ]; then
		echo "copy lib failed, file:$1 not exit"
		exit 1
	fi
}
check_lib_exit ${MTLIB_LIB_PATH}/libmysqlclient.so.18.0.0
check_lib_exit ${MTLIB_LIB_PATH}/libmysqlwrapped-1.1.0.so
check_lib_exit ${MTLIB_LIB_PATH}/libmtreport_api_open-1.1.0.so
check_lib_exit ${MTLIB_LIB_PATH}/libSockets-1.1.0.so
check_lib_exit ${MTLIB_LIB_PATH}/libmyproto-1.1.0.so
check_lib_exit ${MTLIB_LIB_PATH}/libprotobuf.so.6.0.0
check_lib_exit ${MTLIB_LIB_PATH}/libmtreport_api-1.1.0.so
check_lib_exit ${MTLIB_LIB_PATH}/libfcgi.so.0.0.0
check_lib_exit ${MTLIB_LIB_PATH}/libneo_cgi-1.1.0.so
check_lib_exit ${MTLIB_LIB_PATH}/libneo_cs-1.1.0.so
check_lib_exit ${MTLIB_LIB_PATH}/libneo_utl-1.1.0.so
check_lib_exit ${MTLIB_LIB_PATH}/libcgicomm-1.1.0.so

cp ${MTLIB_LIB_PATH}/libfcgi.so.0.0.0 .
strip libfcgi.so.0.0.0
ln -s libfcgi.so.0.0.0 libfcgi.so.0
ln -s libfcgi.so.0 libfcgi.so

cp ${MTLIB_LIB_PATH}/libneo_cgi-1.1.0.so .
strip libneo_cgi-1.1.0.so
ln -s libneo_cgi-1.1.0.so libneo_cgi.so.1 
ln -s libneo_cgi.so.1 libneo_cgi.so 

cp ${MTLIB_LIB_PATH}/libneo_cs-1.1.0.so .
strip libneo_cs-1.1.0.so
ln -s libneo_cs-1.1.0.so libneo_cs.so.1 
ln -s libneo_cs.so.1 libneo_cs.so 

cp ${MTLIB_LIB_PATH}/libcgicomm-1.1.0.so .
strip libcgicomm-1.1.0.so
ln -s libcgicomm-1.1.0.so libcgicomm.so.1 
ln -s libcgicomm.so.1 libcgicomm.so

cp ${MTLIB_LIB_PATH}/libneo_utl-1.1.0.so .
strip libneo_utl-1.1.0.so
ln -s libneo_utl-1.1.0.so libneo_utl.so.1 
ln -s libneo_utl.so.1 libneo_utl.so 

cp ${MTLIB_LIB_PATH}/libmysqlclient.so.18.0.0 . 
strip libmysqlclient.so.18.0.0
ln -s libmysqlclient.so.18.0.0 libmysqlclient.so.18
ln -s libmysqlclient.so.18 libmysqlclient.so

cp ${MTLIB_LIB_PATH}/libmysqlwrapped-1.1.0.so .
strip libmysqlwrapped-1.1.0.so
ln -s libmysqlwrapped-1.1.0.so libmysqlwrapped.so.1
ln -s libmysqlwrapped.so.1 libmysqlwrapped.so

cp ${MTLIB_LIB_PATH}/libmtreport_api_open-1.1.0.so .
strip libmtreport_api_open-1.1.0.so
ln -s libmtreport_api_open-1.1.0.so libmtreport_api_open.so.1
ln -s libmtreport_api_open.so.1 libmtreport_api_open.so

cp ${MTLIB_LIB_PATH}/libSockets-1.1.0.so .
strip libSockets-1.1.0.so
ln -s libSockets-1.1.0.so libSockets.so.1
ln -s libSockets.so.1 libSockets.so

cp ${MTLIB_LIB_PATH}/libmyproto-1.1.0.so .
strip libmyproto-1.1.0.so
ln -s libmyproto-1.1.0.so libmyproto.so.1
ln -s libmyproto.so.1 libmyproto.so

cp ${MTLIB_LIB_PATH}/libprotobuf.so.6.0.0 .
strip libprotobuf.so.6.0.0
ln -s libprotobuf.so.6.0.0 libprotobuf.so.6
ln -s libprotobuf.so.6 libprotobuf.so

cp ${MTLIB_LIB_PATH}/libmtreport_api-1.1.0.so .
strip libmtreport_api-1.1.0.so
ln -s libmtreport_api-1.1.0.so libmtreport_api.so.1
ln -s libmtreport_api.so.1 libmtreport_api.so





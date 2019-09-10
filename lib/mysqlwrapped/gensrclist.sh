#!/bin/bash

rm -f libmysqlwrapped.h
cat IError.h enum_t.h set_t.h Database.h Query.h > libmysqlwrapped.h

MYLIB_INCLUDE=`cat ../../make_env |grep MTLIB_INCLUDE_PATH|awk '{print $3}'`
DIRLIST=". $MYLIB_INCLUDE/mtagent_api_open $MYLIB_INCLUDE/my_proto  $MYLIB_INCLUDE/Sockets $MYLIB_INCLUDE/mtreport_api" 
INSTALL_INC="libmysqlwrapped.h myparam_comm.h"

#if [ ! -d Dist ]; then
#	mkdir Dist
#fi

echo "VER_MAJOR = 1" > Makefile.srcs
echo "VER_MINOR = 1.0" >> Makefile.srcs
echo "INSTALL_INC = $INSTALL_INC" >> Makefile.srcs

echo -n "SRCS = " >> Makefile.srcs
find . -name "*.cpp" -print | xargs echo >> Makefile.srcs
echo >> Makefile.srcs

echo -n "INCLS = " >> Makefile.srcs
find . -name "*.h" -print | xargs echo >> Makefile.srcs
echo >> Makefile.srcs

echo -n "INCLUDE =" >> Makefile.srcs
for DIR in $DIRLIST; do
	echo -n " -I$DIR" >> Makefile.srcs
done
echo >> Makefile.srcs


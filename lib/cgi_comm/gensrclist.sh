#!/bin/sh

MYLIB_INCLUDE=`cat ../../make_env |grep MTLIB_INCLUDE_PATH|awk '{print $3}'`
INCDIRLIST="$MYLIB_INCLUDE/mtagent_api_open $MYLIB_INCLUDE/my_proto  $MYLIB_INCLUDE/Sockets $MYLIB_INCLUDE/cgi $MYLIB_INCLUDE/mtreport_api $MYLIB_INCLUDE/mysqlwrapped"

SRCDIRLIST="."
INSTALL_INC="*.h"

echo "VER_MAJOR = 1" > Makefile.srcs
echo "VER_MINOR = 1.0" >> Makefile.srcs
echo "INSTALL_INC = $INSTALL_INC" >> Makefile.srcs

echo -n "SRCS = " >> Makefile.srcs
for DIR in $SRCDIRLIST; do
	find $DIR -maxdepth 1 -name "*.cpp" -print |grep -v ".*.swp" | xargs echo >> Makefile.srcs
done
echo >> Makefile.srcs

echo -n "INCLS = " >> Makefile.srcs
for DIR in $SRCDIRLIST; do
	find $DIR -maxdepth 1 -name "*.h" -print |grep -v ".*.swp"| xargs echo >> Makefile.srcs
done
echo >> Makefile.srcs

echo -n "INCLUDE =" >> Makefile.srcs
for DIR in $INCDIRLIST; do
	echo -n " -I$DIR" >> Makefile.srcs
done
echo >> Makefile.srcs


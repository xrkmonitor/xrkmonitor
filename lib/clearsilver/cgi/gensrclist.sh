#!/bin/sh

MYLIB_INCLUDE=`cat ../../../make_env |grep MTLIB_INCLUDE_PATH|awk '{print $3}'`
INCDIRLIST=".. . $MYLIB_INCLUDE/mtagent_api_open" 
SRCDIRLIST="."
INSTALL_INC="*.h"

if [ ! -d Dist ]; then
    mkdir Dist
fi

echo "VER_MAJOR = 1" > Makefile.srcs
echo "VER_MINOR = 1.0" >> Makefile.srcs
echo "INSTALL_INC = $INSTALL_INC" >> Makefile.srcs

echo -n "SRCS = " >> Makefile.srcs
echo "cgiwrap.c cgi.c html.c date.c rfc2388.c" >> Makefile.srcs
echo >> Makefile.srcs

echo -n "INCLS = " >> Makefile.srcs
for DIR in $SRCDIRLIST; do
	find $DIR -maxdepth 1 -name "*.h" -print | xargs echo >> Makefile.srcs
done
echo >> Makefile.srcs

echo -n "INCLUDE =" >> Makefile.srcs
for DIR in $INCDIRLIST; do
	echo -n " -I$DIR" >> Makefile.srcs
done
echo >> Makefile.srcs


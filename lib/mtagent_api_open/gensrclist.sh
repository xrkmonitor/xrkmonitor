#!/bin/sh
 
INCDIRLIST="/usr/include/Sockets /usr/include/my_proto"
SRCDIRLIST="."
INSTALL_INC="*.h"

#if [ ! -d Dist ]; then
#    mkdir Dist
#fi

echo "VER_MAJOR = 1" > Makefile.srcs
echo "VER_MINOR = 1.0" >> Makefile.srcs
echo "INSTALL_INC = $INSTALL_INC" >> Makefile.srcs

echo -n "SRCS = " >> Makefile.srcs
for DIR in $SRCDIRLIST; do
	find $DIR -maxdepth 1 -name "*.c*" -print | xargs echo >> Makefile.srcs
done
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


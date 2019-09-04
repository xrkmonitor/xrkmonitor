#!/bin/sh

INCDIRLIST=". .." 
SRCDIRLIST="."
INSTALL_INC="*.h"

if [ ! -d Dist ]; then
    mkdir Dist
fi

echo "VER_MAJOR = 1" > Makefile.srcs
echo "VER_MINOR = 1.0" >> Makefile.srcs
echo "INSTALL_INC = $INSTALL_INC" >> Makefile.srcs

echo -n "SRCS = " >> Makefile.srcs
echo "neo_err.c neo_files.c neo_misc.c neo_rand.c ulist.c neo_hdf.c neo_str.c neo_date.c wildmat.c ulocks.c rcfs.c skiplist.c dict.c filter.c neo_net.c neo_server.c neo_hash.c" >> Makefile.srcs
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


#!/bin/bash

cd ..

dirlist=`find . -maxdepth 1 -type d`
for dr in $dirlist
do 
	if [ -f $dr/$dr -a -x $dr/start.sh -a -x $dr/stop.sh ] ; then
		cd $dr; ./stop.sh; cd ..
	fi
done


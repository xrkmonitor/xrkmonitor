#!/bin/bash

if [ ! -f _configure_check_ ]; then
	make clean; ./configure;
	touch _configure_check_
fi

cd cgi
./gensrclist.sh
cd ..

cd cs
./gensrclist.sh
cd ..

cd util
./gensrclist.sh
cd ..

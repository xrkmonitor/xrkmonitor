#!/bin/bash

# 检查是否已运行过 install_dev.sh 脚本

if [ ! -f _install_dev_run_ ]; then
	echo "you never run install_dev.sh, now run it first ."
	./install_dev.sh || exit 1
fi

if [ ! -f /usr/include/mysqlwrapped/libmysqlwrapped.h ]; then
	cd lib/mysqlwrapped
	cat IError.h enum_t.h set_t.h Database.h Query.h > libmysqlwrapped.h
	mkdir -p /usr/include/mysqlwrapped > /dev/null 2>&1
	cp libmysqlwrapped.h /usr/include/mysqlwrapped
fi



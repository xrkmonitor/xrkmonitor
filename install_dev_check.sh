#!/bin/bash

# 检查是否已运行过 install_dev.sh 脚本

if [ ! -f _install_dev_run_ ]; then
	echo "you never run install_dev.sh, now run it first ."
	./install_dev.sh
fi



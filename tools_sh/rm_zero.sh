#!/bin/bash

echo "搜索到系统中以下未使用的共享内存"
ipcs -m --human |awk 'BEGIN { printf "%-16s%-16s%-s\n", "key", "大小", "属主"; print "----------------------------------------------"; } {if($6==0) printf "%-16s%-16s%-s\n", $1, $5, $3;}' | tee _ipcs_not_use_list

function yn_continue()
{
	read -p "$1" op
	while [ "$op" != "Y" -a "$op" != "y" -a "$op" != "N" -a "$op" != "n" ]; do
		read -p "请输入 (y/n): " op
	done
	if [ "$op" != "y" -a "$op" != "Y" ];then
		echo "no" 
	else
		echo "yes"
	fi
}

isyes=$(yn_continue "是否全部清理(y/n) ?")
if [ "$isyes" != "yes" ];then
	rm _ipcs_not_use_list > /dev/null 2>&1
	exit 0
fi

cat _ipcs_not_use_list | awk '{ if(NR > 2) print "ipcrm -M "$1; }' > _zero.sh
chmod +x _zero.sh
./_zero.sh
rm -f _zero.sh


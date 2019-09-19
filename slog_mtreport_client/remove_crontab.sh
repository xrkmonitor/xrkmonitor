#!/bin/bash

crontab -l > _tmp 
grep "check_xrkmonitor_agent_client.sh" _tmp > /dev/null 2>&1
if [ ! $? -eq 0 ]; then
        rm _tmp
        echo "already remove"
        exit 0
fi
 
awk '{ if(!match($0, "check_xrkmonitor_agent_client")) print $0 }' _tmp > _remove_mtreport_client_check_tmp
crontab _remove_mtreport_client_check_tmp
if [ $? -eq 0 ];then
	rm _tmp
	rm _remove_mtreport_client_check_tmp
	echo "remove crontab ok"
	exit 0
fi

rm _tmp
rm _remove_mtreport_client_check_tmp
echo "remove crontab failed"


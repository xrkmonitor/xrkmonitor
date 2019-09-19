#!/bin/bash

crontab -l > _add_mtreport_client_check_tmp 
grep "check_xrkmonitor_agent_client.sh" _add_mtreport_client_check_tmp > /dev/null 2>&1
if [ $? -eq 0 ]; then
	rm _add_mtreport_client_check_tmp 
	echo "already add"
	exit 0
fi

curpwd=`pwd`
echo "*/1 * * * * cd ${curpwd}; ./check_xrkmonitor_agent_client.sh> /dev/null 2>&1" >> _add_mtreport_client_check_tmp 

crontab _add_mtreport_client_check_tmp
if [ $? -eq 0 ]; then
	rm _add_mtreport_client_check_tmp
	echo "add crontab ok"
	exit 0
fi

echo "add to crontab failed"
rm _add_mtreport_client_check_tmp 


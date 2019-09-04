#!/bin/sh
if [ $# -lt 3 ]; then
	echo "use ip server"
fi
ip=$1
server=$2
user=root
pass=drydlzxd

jmrun $ip "cd /usr/local/monitor/${server}; ./stop.sh" $user $pass 22
jmscp $server $ip /usr/local/monitor/${server} $user $pass 22
jmrun $ip "cd /usr/local/monitor/${server}; ./start.sh" $user $pass 22


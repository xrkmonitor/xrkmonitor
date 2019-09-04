#!/bin/bash
PATH=/bin:/sbin:/usr/bin/:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin

myUser=root
myPass=123

# mysql 服务器 IP
dbip='127.0.0.1'

# 本机 ip
selfip='172.16.0.109' 

cd ../slog_tool
dip=`./slog_tool show ips2d  ${selfip}|awk '{if(NF==3) print $1}'`
echo "get ip:$selfip:$dip"
cd -

exit 0

# import monitor db to mysql
mysql -u$myUser -p$myPass < mtreport_db.sql
mysql -u$myUser -p$myPass < attr_db.sql

MYSQL_CONTEXT="mysql -N -B -u$myUser -p$myPass"
echo "grant privileges to mtreport"
echo "grant all privileges on mtreport_db.* to 'mtreport'@'localhost' identified by 'mtreport875';" | ${MYSQL_CONTEXT}
echo "grant all privileges on attr_db.* to 'mtreport'@'localhost' identified by 'mtreport875';" | ${MYSQL_CONTEXT}

echo "init local machine to db"
echo "insert into mt_machine set name='local_config',ip1=$dip,user_add='sadmin',user_mod='sadmin',user_mod_id=1,user_add_id=1;" | ${MYSQL_CONTEXT}


#!/bin/bash

MAXCOUNTBACK=7
BACKUPPATH=/home/mtreport/mtreport_open/db
CURDATE=`date '+%Y%m%d-%H%M%S'`
cd ${BACKUPPATH}
mysqldump --default-character-set=utf8 -B mtreport_db > _mtreport_db.sql 

cp _mtreport_db.sql backup_sql/${CURDATE}.sql

cd backup_sql
COUNT=`ls -lt *.sql|wc -l`
if [ $COUNT -lt $MAXCOUNTBACK ]; then
        exit
fi
ls -lrt *.sql |awk '{if(NR==1) print $9}' |xargs rm -f {} \;


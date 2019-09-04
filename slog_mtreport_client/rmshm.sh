#!/bin/bash

shmkey=0x0c014eb3
timershmkey=0x01078561

grep MTREPORT_SHM_KEY slog_mtreport_client.conf > /dev/null 2>&1
if [ $? -eq 0 ]; then
	shmkey=`grep MTREPORT_SHM_KEY slog_mtreport_client.conf  |awk '{print $2}'`
fi

grep TIMER_HASH_SHM_KEY slog_mtreport_client.conf > /dev/null 2>&1
if [ $? -eq 0 ]; then
	timershmkey=`grep TIMER_HASH_SHM_KEY slog_mtreport_client.conf  |awk '{print $2}'`
fi

ipcrm -M $shmkey 
ipcrm -M $timershmkey 

echo "remove shm $shmkey $timershmkey ok"


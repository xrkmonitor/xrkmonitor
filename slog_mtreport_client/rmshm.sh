#!/bin/bash

shmkey=0x158CB3

grep MTREPORT_SHM_KEY slog_mtreport_client.conf > /dev/null 2>&1
if [ $? -eq 0 ]; then
	shmkey=`grep MTREPORT_SHM_KEY slog_mtreport_client.conf  |awk '{print $2}'`
fi
ipcrm -M $shmkey 
echo "remove shm $shmkey ok"


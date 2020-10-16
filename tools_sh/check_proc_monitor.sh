#!/bin/bash
#
#function: check process number 
#

# 进入脚本所在目录
script_path=''
script_name=''
function in_script_path()
{
    echo "/" > _tmp
    fc=`expr substr "$0" 1 1`
    echo "$fc" > _tmp_2
    cmp _tmp _tmp_2 > /dev/null 2>&1
    if [ $? -eq 0 ];then
        cscript=$0
    else
        cscript=$(pwd)/$0
    fi
    rm -f _tmp _tmp_2
    script_path=`dirname $cscript`
    script_name=`basename $cscript`
    cd "$script_path"
}
in_script_path

# 确保同一时间只运行一个文件目录监控脚本
function check_process_run()
{
    pname=$1
    if [ -f "${pname}.pid" ]; then
        oldpid=`cat ${pname}.pid`
        pgrep -f ${pname}|grep $oldpid > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo "already run, pid:$oldpid !"
			dinfo=`date`
			if [ -f "check_process-${pname}.log" ]; then
			    clogsize=`stat --format %s check_process-${pname}.log`
			    if [ $clogsize -lt 1048576 ]; then
			        rm "check_process-${pname}.log"
			        touch "check_process-${pname}.log"
			    fi
			fi
			echo "$dinfo: already run, pid:$oldpid ! - $$ -" >> "check_process-${pname}.log"
            exit $LINENO
        fi  
    fi  
    rm -f "${pname}.pid" 
    touch "${pname}.pid"
    echo $$ > "${pname}.pid"
}

function check_process_atexit()
{
	cd "$script_path"
	if [ -f "${script_name}.pid" ]; then
		rm -f "${script_name}.pid"
	fi
}
check_process_run "$script_name" 

cd ..
ppath=`pwd`
ACTION=0
TMPFILE=/tmp/check_proc_last.log
RESULTDIR=$ppath/slog_check_proc
MONITORBASEDIR=$ppath
MAILTOADDR=xx@qq.com
MAILSUBJECT='process restart ...'
RESTARTINFO=''
CHECK_STOP='yes';

if [ ! -d ${RESULTDIR} ]; then
	mkdir -p ${RESULTDIR}
fi

if [ $# -eq 1 ]; then
	CHECK_STOP='no'
fi

function restart_process()
{
	pdir=$1
	pname=$2
	pnum=$3

	if [ -z "$pdir" -o -z "$pname" -o -z "$pnum" ] ; then
		echo "ERROR! Argument Invalid!!!"; exit;
	fi

	if [ ${CHECK_STOP} == 'yes' ]; then
		if [ -f ${pdir}/_manual_stop_ ] ; then return ; fi
	fi

	Count=`pgrep -f "\./${pname}"|wc -l`
	if [ $Count -eq $pnum ] ; then return ; fi
	usleep 50000 > /dev/null 2>&1 || sleep 1
	Count=`pgrep -f "\./${pname}"|wc -l`
	if [ $Count -eq $pnum ] ; then return ; fi

	cd ${pdir}; ./stop.sh ; rm _manual_stop_ > /dev/null 2>&1; ./start.sh > /dev/null; cd - > /dev/null 
	echo "=========== log =========" >> $TMPFILE
	echo "${pname} restarted, Count is $Count, need $pnum." >> $TMPFILE
	RESTARTINFO="${RESTARTINFO}; ${pname} restarted, Count is $Count, need $pnum."
	echo "=========== log =========" >> $TMPFILE 
	if [ ${CHECK_STOP} == 'yes' ] ; then
		ACTION=1 
	fi
}

function restart_process_ex()
{
	pdir=$1
	pname=$2
	pnum_min=$3
	pnum_max=$4

	if [ -z "$pdir" -o -z "$pname" -o -z "$pnum_min" -o -z "$pnum_max" -o $pnum_min -gt $pnum_max ] ; then
		echo "ERROR! Argument Invalid!!!"; exit;
	fi

	if [ ${CHECK_STOP} == 'yes' ]; then
		if [ -f ${pdir}/_manual_stop_ ] ; then return ; fi
	fi

	Count=`pgrep -f "\./${pname}"|wc -l`
	if [ $Count -ge $pnum_min -a $Count -le $pnum_max ] ; then return ; fi
	usleep 50000 > /dev/null 2>&1 || sleep 1
	Count=`pgrep -f "\./${pname}"|wc -l`
	if [ $Count -ge $pnum_min -a $Count -le $pnum_max ] ; then return ; fi

	cd ${pdir}; ./stop.sh ; rm _manual_stop_; ./start.sh > /dev/null; cd - > /dev/null 
	echo "=========== log =========" >> $TMPFILE
	echo "${pname} restarted, Count is $Count, need ($pnum_min, $pnum_max)." >> $TMPFILE
	echo "=========== log =========" >> $TMPFILE
	RESTARTINFO="${RESTARTINFO}; ${pname} restarted, Count is $Count, need ($pnum_min, $pnum_max)."
	if [ ${CHECK_STOP} == 'yes' ] ; then
		ACTION=1 
	fi
}

CURDATE=`date '+%Y%m%d'`
DATETIME=`date '+%Y%m%d-%H%M%S'`
RESTARTFILELOG="restart_${CURDATE}.log"
echo "$DATETIME" > $TMPFILE
echo "Start check ..." >> $TMPFILE
echo "-------- ps and ipcs BEFORE check and restart process ----------" >> $TMPFILE
ps auxww >> $TMPFILE
ipcs >> $TMPFILE
echo "----------------------------------------------------------------" >> $TMPFILE

restart_process "${MONITORBASEDIR}/slog_memcached" 'slog_memcached' 1
restart_process "${MONITORBASEDIR}/slog_config" 'slog_config$' 1
while [ ! -f /tmp/_slog_config_read_ok ]  
do
	usleep 50000 > /dev/null 2>&1 || sleep 1
done

restart_process "${MONITORBASEDIR}/slog_client" 'slog_client$' 1
restart_process "${MONITORBASEDIR}/slog_server" 'slog_server$' 1
restart_process "${MONITORBASEDIR}/slog_check_warn" 'slog_check_warn$' 1
restart_process "${MONITORBASEDIR}/slog_write" 'slog_write$' 5
restart_process "${MONITORBASEDIR}/slog_monitor_server" 'slog_monitor_server$' 2
restart_process "${MONITORBASEDIR}/slog_mtreport_server" 'slog_mtreport_server$' 1
restart_process_ex "${MONITORBASEDIR}/slog_deal_warn" 'slog_deal_warn$' 1 8
restart_process "${MONITORBASEDIR}/slog_mtreport_client" 'slog_mtreport_client$' 2

if [ $ACTION -eq 1 ] ; then
	echo "-------- ps and ipcs AFTER check and restart process ----------" >> $TMPFILE 
	ps auxww >> $TMPFILE
	ipcs >> $TMPFILE
	echo "----------------------------------------------------------------" >> $TMPFILE
	echo "End check ..." >> $TMPFILE
	cp $TMPFILE $RESULTDIR/$RESTARTFILELOG

	ADDRESS=`echo $HOSTNAME`
	RESTARTINFO="${RESTARTINFO}; host name:${ADDRESS}"
	usleep 50000 > /dev/null 2>&1 || sleep 1
	#${MONITORBASEDIR}/slog_deal_warn/slog_deal_warn $MAILTOADDR "$MAILSUBJECT" "$RESTARTINFO"
	check_process_atexit
	exit 1
else
	check_process_atexit
	exit 0
fi


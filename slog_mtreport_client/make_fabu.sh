#!/bin/bash
# 发布包 agent slog_mtreport_client 打包脚本

TARGET=slog_mtreport_client.tar.gz
AGENT=slog_mtreport_client 

rm -f ${TARGET}
mv ${AGENT}.conf _${AGENT}.conf
cp fabu_mtreport_client.conf ${AGENT}.conf
tar -czf ${TARGET} restart.sh start.sh stop.sh add_crontab.sh remove_crontab.sh run_tool.sh check_xrkmonitor_agent_client.sh ${AGENT}.conf ${AGENT} rmshm.sh start_plugin.sh stop_plugin.sh
mv _${AGENT}.conf ${AGENT}.conf


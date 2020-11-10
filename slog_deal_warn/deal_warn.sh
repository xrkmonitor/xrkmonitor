#!/bin/bash
#
#  xrkmonitor 字符云监控系统开源版 告警回调脚本
#  您可以在此定制您自己的告警的处理方式, 该脚本可使用以下环境变量
#
#  warn_db_id: 告警记录的 DB id 
#  warn_start_time: 告警产生时间
#  warn_obj_type: 告警对象类型(view-视图, machine-单机, exception-异常监控点)
#  warn_obj_id: 告警对象类型ID(视图ID/上报机器ID/异常监控点ID)
#  warn_obj_name: 告警对象名称(视图名/上报机器名)
#  warn_type: 告警类型(max/min/wave/exception 分别表示最大值/最小值/波动值/异常监控点)
#  warn_attr_id: 触发告警的监控点ID
#  warn_attr_name: 触发告警的监控点名称
#  warn_report_val: 触发告警的监控点上报值
#  warn_config_val: 触发告警的监控点告警配置值
#  warn_desc: 告警描述
#
#  如您想使用自己的邮件服务发送告警，可以在该脚本中使用开源的 python 邮件发送脚本实现：sendEmail
#  推荐您使用云版本告警通道，无需开发可支持多种告警方式。 
#
#  xrkmonitor字符云监控系统, 开源、免费、高性能分布式监控系统
#	 官网地址：http://xrkmonitor.com
#	 演示地址：http://open.xrkmonitor.com
#

logfile=warn_log.txt

echo "收到监控告警" > $logfile
echo "告警记录的id：$warn_db_id" >> $logfile
echo "告警开始时间：$warn_start_time " >> $logfile
echo "告警对象类型：$warn_obj_type " >> $logfile
echo "告警对象类型ID：$warn_obj_id" >> $logfile
echo "告警对象名称：$warn_obj_name" >> $logfile
echo "告警类型：$warn_type" >> $logfile
echo "触发告警的监控点ID：$warn_attr_id" >> $logfile
echo "触发告警的监控点名称：$warn_attr_name" >> $logfile
echo "触发告警的监控点上报值：$warn_report_val" >> $logfile
echo "触发告警的监控点告警配置值：$warn_config_val" >> $logfile
echo "" >> $logfile
echo "告警描述: $warn_desc" >> $logfile


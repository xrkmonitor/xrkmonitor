/*** xrkmonitor license ***

   Copyright (c) 2019 by rockdeng

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


   字符云监控(xrkmonitor) 开源版 (c) 2019 by rockdeng
   当前版本：v1.0
   使用授权协议： apache license 2.0

   云版本主页：http://xrkmonitor.com

   云版本为开源版提供永久免费告警通道支持，告警通道支持短信、邮件、
   微信等多种方式，欢迎使用

   模块 slog_client 功能:
        slog_client 用于将字符云监控系统自身运行时产生的日志发送到日志中心，
		内部使用的协议是易于扩展的 protobuf 协议

****/

#ifndef _TOP_INCLUDE_COMM_20130110_H_
#define _TOP_INCLUDE_COMM_20130110_H_ 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <inttypes.h>
#include <sv_log.h>
#include <sv_struct.h>
#include <md5.h>
#include <sv_str.h>
#include <sv_cfg.h>
#include <sv_net.h>
#include <mt_report.h>
#ifdef encode
#undef encode
#endif 

#include <cassert>

#include <libmysqlwrapped.h>
#include <cassert>
#include <supper_log.h>

#endif


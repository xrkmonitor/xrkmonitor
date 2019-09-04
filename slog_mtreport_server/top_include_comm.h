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

   模块 slog_mtreport_server 功能:
        管理 agent slog_mtreport_client 的接入下发监控系统配置

****/

#ifndef _TOP_INCLUDE_COMM_20130110_H_
#define _TOP_INCLUDE_COMM_20130110_H_ 1

#include <inttypes.h>

extern "C" {
#include <sv_log.h>
#include <sv_struct.h>
#include <md5.h>
#include <sv_str.h>
#include <sv_cfg.h>
#include <sv_net.h>
#include <mt_report.h>
}

#ifdef encode
#undef encode
#endif 

#include <cassert>

#include <libmysqlwrapped.h>
#include <cassert>
#include <supper_log.h>
#include <basic_packet.h>

#endif


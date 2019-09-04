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

   模块 slog_deal_warn 功能:
        处理监控告警以及邮件发送

****/

#ifndef _TOP_INCLUDE_COMM_20130110_H_
#define _TOP_INCLUDE_COMM_20130110_H_ 1

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
#include <map>

#include "udp_sock.h"

struct TWarnMapKey{
	uint32_t seq;
	uint32_t dwSendTime;
	int32_t iSendTimes;
	bool operator < (const TWarnMapKey &o) const {
		return seq < o.seq;
	}
};

typedef struct
{
    char szLocalIp[20];

	int iSendWarnShmKey;
	int iValidSendWarnTimeSec;
	int iSkipSendWarn;
	TWarnSendInfo *pSendWarnShm;

	char szDbHost[32];
	char szUserName[32];
	char szPass[32];
	char szDbName[32];
	Database *db;
	Query *qu;
	TCommSendMailInfoShm *pShmMailInfo;
	int iClientShmKey;
	uint32_t dwCurrentTime;
	MtSystemConfig *psysConfig;
	int iEmailUseXrkmonitor;
	int iXrkmonitorUid;
	char szXrkmonitorKey[64];
	char szXrkmonitorSrv[32];
	int iXrkmonitorSrvPort;
	CUdpSock *pUdp;

	// 用于告警 udp 包重发逻辑
	std::map<TWarnMapKey, std::string> sWarnUdpPackList;
}CONFIG;

#endif


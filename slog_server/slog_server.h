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

   模块 slog_server 功能:
         接收日志客户端上报的日志，并将日志写入本机共享内存中

****/

#ifndef _SLOG_SERVER_H_
#define _SLOG_SERVER_H_ (1)

#define CONFIG_FILE "./slog_server.conf"
#define MAX_LOG_COUNT_PER_SEND 200

#include <top_proto.pb.h>
#include <set>
#include <map>
#include "udp_sock.h"

struct TGetAppLogSizeKey
{
	uint32_t dwAppLogSrv;
	uint16_t wAppLogSrvPort;
};

struct ReqAppLogSizeCmp {
	bool operator () (const TGetAppLogSizeKey & x, const TGetAppLogSizeKey & y) const {
		if(x.dwAppLogSrv > y.dwAppLogSrv)
			return 1;
		if(x.wAppLogSrvPort > y.wAppLogSrvPort)
			return 1;
		return 0;
	}
};

typedef struct
{
	uint32_t dwLastReqTime;
}TReqInfo;

class CUdpSock;
typedef struct
{
	char szListenIp[32];
	char szLocalIp[20];
	char szLogFile[256];
	uint32_t dwCurrentTime;
	int32_t iRecvPortMonitor;
	std::map<int, TSLogShm *> mapAppLogShm;
	std::map<int, SLogFile *> mapAppLogFileShm; 
	MtSystemConfig *psysConfig;

	int iLocalMachineId;
	MachineInfo *pLocalMachineInfo;
	CUdpSock *pstSock;
	SLogConfig * pShmConfig;
	int iCheckEachAppLogSpaceTime;
	int iCheckLogSpaceTime;
	std::map<TGetAppLogSizeKey, top::SlogGetAppLogSizeReq*, ReqAppLogSizeCmp> stMapLogSizeReqInfo;

	char szQuickToSlowIp[16];
	int iQuickToSlowPort;
}CONFIG;

extern CONFIG stConfig;

#endif


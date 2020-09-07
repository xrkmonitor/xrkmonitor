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

#ifndef _UDP_SOCK_H_
#define _UDP_SOCK_H_ 1

#include <Sockets/UdpSocket.h>
#include <Sockets/SocketHandler.h>
#include <basic_packet.h>

class CUdpSock: public UdpSocket, public CBasicPacket
{
	public:
		CUdpSock(ISocketHandler& h);
		~CUdpSock();
		void OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len);
		void SendLog(Ipv4Address &addr, const char *buf, size_t len);
		int32_t SendResponsePacket(const char *pkg, int len) { return 0; }
        void ReportQuickToSlowMsg(int iMachineId);

	private:
};

class TLogClientInfo
{
	public:
		TLogClientInfo() {
			pAppInfo = NULL;
			pstLogClient = NULL;
		}

		TLogClientInfo & operator=(const TLogClientInfo &clt)
		{
			pAppInfo = clt.pAppInfo;
			pstLogClient = clt.pstLogClient;
			return *this;
		}

		AppInfo * pAppInfo;
		CSLogClient *pstLogClient;
};

typedef std::map<int, TLogClientInfo> TMapLogClientInfo;
typedef std::map<int, TLogClientInfo>::iterator TMapLogClientInfoIt;
typedef std::map<uint32_t, uint32_t> TMapHeartToServerInfo;
typedef std::map<uint32_t, uint32_t>::iterator TMapHeartToServerInfoIt;

typedef struct _CONFIG
{
	char szLocalIp[20];

	int iLocalMachineId;
	MachineInfo *pLocalMachineInfo;

	TMapHeartToServerInfo stMapHeartInfo;

	uint32_t dwSendLogCount;
	uint32_t dwCurrentTime;
	TMapLogClientInfo mapLogClient;
	SLogAppInfo *pAppShmInfoList;
	AppInfo * pSelfApp;
	int32_t iSendHeartTimeSec;
	int32_t iCheckLogClientTimeSec;
    char szQuickToSlowIp[16];
    int iQuickToSlowPort;

	_CONFIG() {
		memset(szLocalIp, 0, sizeof(szLocalIp));

		dwSendLogCount = 0;
		dwCurrentTime = 0;
		pAppShmInfoList = NULL;
		pSelfApp = NULL;
		iSendHeartTimeSec = 0;
		iCheckLogClientTimeSec = 0;
	}
}CONFIG;

extern CONFIG stConfig;

#endif


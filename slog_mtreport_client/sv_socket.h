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

   模块 slog_mtreport_client 功能:
        用于上报除监控系统本身产生的监控点数据、日志，为减少部署上的依赖
		未引入任何第三方组件

****/

#ifndef __SV_SOCKET__
#define __SV_SOCKET__ (1)

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h> 
#include <arpa/inet.h>

#define MT_SOCKET_DEF_COUNT_MAX  100
#define MT_SOCKET_DEF_SELECT_TIME_US 10000

struct MtSocket;
struct MtReportSocket;
typedef void (* OnPkg_f)(struct MtSocket *psock, char * sBuf, int iLen, time_t uiTime);
typedef void (* OnLoop_f)(time_t uiTime);
typedef void (* OnTimeout_f)(time_t uiTime);
typedef void (* OnError_f)(struct MtReportSocket *pstSock, time_t uiTime);

struct MtSocket
{
	unsigned int iRecvPacks;
	unsigned int iSendPacks;
	int isock;
	int iMaxRespTimeMs;
	struct sockaddr_in remote;
	struct sockaddr_in last_recv_remote;
	char buf[8192];
	OnPkg_f onPkg;
	OnLoop_f onLoop;
};

#define MTSOCKET_TO_INDEX(psock) (((unsigned char*)psock)-((unsigned char*)g_socket.socks))/sizeof(MtSocket)
struct MtReportSocket
{
	char cIsInit;
	int iNumSock;
	int iLastFreeIdx;
	struct MtSocket socks[MT_SOCKET_DEF_COUNT_MAX];
	int iSockMax;
	fd_set socks_save;
	fd_set socks_use;
	OnTimeout_f onTimeout;
	OnError_f onError;
};

extern struct MtReportSocket g_socket;

int InitSocket(OnTimeout_f onTimeout, OnError_f onError);
int AddSocket(int isock, struct sockaddr_in *paddr, OnPkg_f onPkg, OnLoop_f onLoop);
int CheckSocket(time_t tmcur, uint32_t usSelect);
int SendPacket(int iSockIdx, struct sockaddr_in *pDestAddr, const char *pdata, int iDataLen);
int SendPacket(struct MtSocket *psock, const char *pdata, int iDataLen);
uint32_t GetSocketAddress(int iSockIdx);
void SetSocketAddress(int iSockIdx, uint32_t dwNetAddress, uint16_t wPort=0);
void InitSocketComm(int iSock);
int32_t GetMaxResponseTime(int iSockIdx);
void ModMaxResponseTime(int iSockIdx, int iRespTime);

#endif


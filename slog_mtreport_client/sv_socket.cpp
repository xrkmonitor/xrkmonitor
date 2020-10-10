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

#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "sv_socket.h"
#include "mtreport_client.h"

struct MtReportSocket g_socket;

int InitSocket(OnTimeout_f onTimeout, OnError_f onError)
{
	if(g_socket.cIsInit)
		return 1;
	memset(&g_socket, 0, MYSIZEOF(g_socket));
	FD_ZERO(&g_socket.socks_save);
	FD_ZERO(&g_socket.socks_use);
	g_socket.onTimeout = onTimeout;
	g_socket.onError = onError;
	g_socket.cIsInit = 1;
	return 0;
}

int AddSocket(int isock, struct sockaddr_in *paddr, OnPkg_f onPkg, OnLoop_f onLoop)
{
	if(!g_socket.cIsInit)
		return -1;
	if(g_socket.iNumSock >= MT_SOCKET_DEF_COUNT_MAX)
		return -2;

	int i=0, j=g_socket.iLastFreeIdx;
	for(i=0; i < MT_SOCKET_DEF_COUNT_MAX; i++) {
		if(g_socket.socks[j].isock == 0)
			break;
		else {
			j++;
			if(j >= MT_SOCKET_DEF_COUNT_MAX)
				j=0;
		}
	}
	if(i >= MT_SOCKET_DEF_COUNT_MAX)
		return -3;
	memset(g_socket.socks+j, 0, MYSIZEOF(struct MtSocket));
	g_socket.socks[j].isock = isock;
	if(paddr != NULL)
		memcpy(&(g_socket.socks[j].remote), paddr, MYSIZEOF(*paddr));
	else 
		memset(&(g_socket.socks[j].remote), 0, MYSIZEOF(struct sockaddr_in));
	memset(&(g_socket.socks[j].last_recv_remote), 0, MYSIZEOF(struct sockaddr_in));
	g_socket.socks[j].onPkg = onPkg;
	g_socket.socks[j].onLoop= onLoop;
	g_socket.socks[j].iMaxRespTimeMs = -1;

	if(j+1 >= MT_SOCKET_DEF_COUNT_MAX)
		g_socket.iLastFreeIdx = 0;
	else
		g_socket.iLastFreeIdx = j+1;
	if(isock+1 > g_socket.iSockMax)
		g_socket.iSockMax = isock+1;
	FD_SET(isock, &(g_socket.socks_save));
	g_socket.iNumSock++;
	return j;
}

int CheckSocket(time_t tmcur, uint32_t usSelet)
{
	int ret = 0, i = 0, j = 0;

	// onloop 
	for(j=0, i=0; i < MT_SOCKET_DEF_COUNT_MAX && j < g_socket.iNumSock; i++)
	{
		if(g_socket.socks[i].isock != 0)
			j++;
		if(g_socket.socks[i].isock != 0 && g_socket.socks[i].onLoop)
			g_socket.socks[i].onLoop(tmcur);
	}

	// check socket recv
	memcpy(&g_socket.socks_use, &g_socket.socks_save, MYSIZEOF(fd_set));
	struct timeval stTimeVal;
	stTimeVal.tv_sec = usSelet/1000000;
	stTimeVal.tv_usec = usSelet%1000000;
	ret = select(g_socket.iSockMax, &g_socket.socks_use, NULL, NULL, &stTimeVal);
	if(ret < 0) {
		if(g_socket.onError)
			g_socket.onError(&g_socket, tmcur);
	}
	else if (ret==0) {
		if(g_socket.onTimeout)
			g_socket.onTimeout(tmcur);
	}
	else {
		socklen_t iAddrLen = 0;
		for(j=ret, i=0; i < MT_SOCKET_DEF_COUNT_MAX && j > 0; i++)
		{
			if(g_socket.socks[i].isock != 0 && FD_ISSET(g_socket.socks[i].isock, &g_socket.socks_use)) {
				j--;
				iAddrLen = MYSIZEOF(struct sockaddr_in);
				int len = recvfrom(g_socket.socks[i].isock, g_socket.socks[i].buf, MYSIZEOF(g_socket.socks[i].buf), 
					0, (struct sockaddr *)&(g_socket.socks[i].last_recv_remote), &iAddrLen);
				if(len < 0) {
					if(g_socket.onError)
						g_socket.onError(&g_socket, tmcur);
				}
				else if(g_socket.socks[i].onPkg) {
					g_socket.socks[i].onPkg(g_socket.socks+i, g_socket.socks[i].buf, len, tmcur);
					g_socket.socks[i].iRecvPacks++;
				}
			
			}
		}
	}
	return ret;
}

void ModMaxResponseTime(int iSockIdx, int iRespTime)
{
	if(iSockIdx < 0 || iSockIdx >= MT_SOCKET_DEF_COUNT_MAX)
		return;

	if(g_socket.socks[iSockIdx].isock <= 0)
		return;
	if(iRespTime > g_socket.socks[iSockIdx].iMaxRespTimeMs) {
		g_socket.socks[iSockIdx].iMaxRespTimeMs = iRespTime;
		INFO_LOG("modify server:%s:%d max response time:%d",
			inet_ntoa(g_socket.socks[iSockIdx].remote.sin_addr), 
			ntohs(g_socket.socks[iSockIdx].remote.sin_port), iRespTime);
	}
}

int32_t GetMaxResponseTime(int iSockIdx)
{
	if(iSockIdx < 0 || iSockIdx >= MT_SOCKET_DEF_COUNT_MAX)
		return PKG_TIMEOUT_MS;

	if(g_socket.socks[iSockIdx].isock <= 0)
		return PKG_TIMEOUT_MS;
	if(g_socket.socks[iSockIdx].iMaxRespTimeMs <= 0)
		return PKG_TIMEOUT_MS-1000;
	return g_socket.socks[iSockIdx].iMaxRespTimeMs;
}

uint32_t GetSocketAddress(int iSockIdx)
{
	if(iSockIdx < 0 || iSockIdx >= MT_SOCKET_DEF_COUNT_MAX)
		return 0;

	if(g_socket.socks[iSockIdx].isock <= 0)
		return 0;
	return g_socket.socks[iSockIdx].remote.sin_addr.s_addr;
}

void SetSocketAddress(int iSockIdx, uint32_t dwNetAddress, uint16_t wPort)
{
	if(iSockIdx < 0 || iSockIdx >= MT_SOCKET_DEF_COUNT_MAX)
		return ;

	if(g_socket.socks[iSockIdx].isock <= 0)
		return ;
	if(g_socket.socks[iSockIdx].remote.sin_addr.s_addr != dwNetAddress)
		g_socket.socks[iSockIdx].iMaxRespTimeMs = -1;
	g_socket.socks[iSockIdx].remote.sin_addr.s_addr = dwNetAddress;
	if(wPort != 0)
		g_socket.socks[iSockIdx].remote.sin_port = htons(wPort);
}

int SendPacket(struct MtSocket *psock, const char *pdata, int iDataLen)
{
	if(psock->isock <= 0)
		return -2;

    socklen_t socklen = sizeof(struct sockaddr_in);
	int ret = sendto(psock->isock,
		pdata, iDataLen, 0, (struct sockaddr *)&(psock->remote), socklen);
	if(ret != iDataLen && g_socket.onError)
		g_socket.onError(&g_socket, time(NULL));
	psock->iSendPacks++;
	return ret;
}

int SendPacket(int iSockIdx, struct sockaddr_in *pDestAddr, const char *pdata, int iDataLen)
{
	if(iSockIdx < 0 || iSockIdx >= MT_SOCKET_DEF_COUNT_MAX || !pdata || iDataLen <= 0)
		return -1;

	if(g_socket.socks[iSockIdx].isock <= 0)
		return -2;

	int ret = 0;
	struct sockaddr_in remote_addr;
	socklen_t socklen = MYSIZEOF(struct sockaddr_in);

	if(pDestAddr != NULL 
		&& memcmp(pDestAddr, &g_socket.socks[iSockIdx].remote, MYSIZEOF(struct sockaddr_in)))
	{
		memcpy(&g_socket.socks[iSockIdx].remote, pDestAddr, MYSIZEOF(struct sockaddr_in));
		DEBUG_LOG("update socket:%d remote address to:%s", 
			iSockIdx, inet_ntoa(g_socket.socks[iSockIdx].remote.sin_addr));
	}

	int iSrcDataLen = iDataLen;
	memcpy(&remote_addr, &g_socket.socks[iSockIdx].remote, MYSIZEOF(remote_addr));

	ret = sendto(g_socket.socks[iSockIdx].isock, pdata, iDataLen, 0, (struct sockaddr *)&remote_addr, socklen);
	if(ret != iDataLen && g_socket.onError) {
		ERROR_LOG("sendto failed, packet len:%d to %s:%d, ret:%d, msg:%s", iDataLen, 
			inet_ntoa(g_socket.socks[iSockIdx].remote.sin_addr),
			ntohs(g_socket.socks[iSockIdx].remote.sin_port), ret, strerror(errno));
		g_socket.onError(&g_socket, time(NULL));
	}
	g_socket.socks[iSockIdx].iSendPacks++;
	DEBUG_LOG("sendpacket to:%s:%d", inet_ntoa(g_socket.socks[iSockIdx].remote.sin_addr),
		ntohs(g_socket.socks[iSockIdx].remote.sin_port));
	return iSrcDataLen;
}

void InitSocketComm(int iSock)
{
	int iFlags = 0; 
	iFlags = fcntl(iSock, F_GETFL, 0);
	iFlags |= O_NONBLOCK;
	iFlags |= O_NDELAY; 
	fcntl(iSock, F_SETFL, iFlags); 

	int iReuseAddr = 1;
	if(setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, &iReuseAddr, MYSIZEOF(iReuseAddr) != 0)) {
	}
	int iUdpSockBufLen = 512*1024;
	socklen_t optlen = MYSIZEOF(iUdpSockBufLen);
	if(setsockopt(iSock, SOL_SOCKET, SO_RCVBUF, &iUdpSockBufLen, optlen) != 0) {
	}
	iUdpSockBufLen = 1024*1024;
	if(setsockopt(iSock, SOL_SOCKET, SO_SNDBUF, &iUdpSockBufLen, optlen) != 0) {
	}
}


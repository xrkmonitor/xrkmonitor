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

   模块 slog_monitor_client 功能:
        用于上报监控系统自身相关模块的监控点数据，内部使用易于扩展的
        protobuf 协议

****/

#include <supper_log.h>
#include <sv_struct.h>

#include "udp_sock.h"

CUdpSock::CUdpSock(ISocketHandler&h): UdpSocket(h), CBasicPacket()
{
	Attach(CreateSocket(PF_INET, SOCK_DGRAM, "udp"));
}


void CUdpSock::SetServer(uint32_t dwIp, uint16_t wPort)
{
	m_dwSrvIP = dwIp;
	m_wSrvPort = wPort;
}

CUdpSock::~CUdpSock()
{
}

void CUdpSock::OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len)
{
	m_addrRemote.SetAddress(sa);
	DEBUG_LOG("receive ack from:%s, len:%d", m_addrRemote.Convert(true).c_str(), (int)len);
}

void CUdpSock::SendPacketPb(std::string &head, std::string &body)
{
	char *pack = NULL;
	int ipackLen = 0;
	if((ipackLen=SetPacketPb(head, body, &pack)) < 0)
	{
		ERR_LOG("SetPacketPb failed ret:%d", ipackLen);
		return;
	}

	Ipv4Address addr(m_dwSrvIP, m_wSrvPort);
	SendToBuf(addr, pack, ipackLen, 0);
	DEBUG_LOG("send attr report, packlen:%d, to:%s", ipackLen, addr.Convert(true).c_str());
}


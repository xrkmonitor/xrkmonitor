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
		void SetServer(uint32_t dwIp, uint16_t wPort);
		void OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len);
		void SendPacketPb(std::string &head, std::string &body);
		int32_t SendResponsePacket(const char *pkg, int len) { return 0; }
	private:
		uint32_t m_dwSrvIP;
		uint16_t m_wSrvPort;
};

#endif


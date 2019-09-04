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

#include <supper_log.h>
#include <sv_struct.h>

#include <mt_report.h>
#include "udp_sock.h"

void CUdpSock::SendLog(Ipv4Address &addr, const char *buf, size_t len)
{
	SendToBuf(addr, buf, len, 0);
	MtReport_Attr_Add(70, 1);
}

CUdpSock::CUdpSock(ISocketHandler&h): UdpSocket(h), CBasicPacket()
{
	Attach(CreateSocket(PF_INET, SOCK_DGRAM, "udp"));
	DEBUG_LOG("on CUdpSock construct");
}

CUdpSock::~CUdpSock()
{
	DEBUG_LOG("on CUdpSock destruct");
}

void CUdpSock::OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len)
{
	if(CheckBasicPacket(buf, len, sa) != NO_ERROR)
		MtReport_Attr_Add(64, 1);
	else
		MtReport_Attr_Add(65, 1);
}

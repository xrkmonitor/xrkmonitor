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

#include <supper_log.h>
#include <sv_struct.h>

#include <mt_report.h>
#include "top_include_comm.h"
#include "udp_sock.h"

extern CONFIG stConfig;

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
	slog.SetTestLog(false);
	slog.ClearAllCust();
	slog.SetCust_6(m_addrRemote.Convert(true).c_str());
	m_addrRemote.SetAddress(sa);

	int iRet = 0;
	if((iRet=CheckBasicPacket(buf, len)) != NO_ERROR)
	{
		REQERR_LOG("invalid packet, len:%d", (int)len);
		return ;
	}

	if(m_pbHead.en_cmd() == comm::CMD_SLOG_OPEN_SEND_WARN)
	{
		INFO_LOG("receive send warn ack packet, result:%d", m_pbHead.uint32_result());
	}
	else if(m_pbHead.en_cmd() == comm::CMD_SLOG_OPEN_SEND_EMAIL)
	{
		INFO_LOG("receive send email ack packet, result:%d", m_pbHead.uint32_result());
	}
	else
	{
		DEBUG_LOG("unknow packet, cmd:%d", m_pbHead.en_cmd());
	}
}

int CUdpSock::SendPacketPb(
	uint32_t seq, std::string &head, std::string &body, const char *psrv, int iPort)
{
	char *pack = NULL;
	int ipackLen = 0;
	if((ipackLen=SetPacketPb(head, body, &pack)) < 0)
	{   
		ERR_LOG("SetPacketPb failed ret:%d", ipackLen);
		return SLOG_ERROR_LINE;
	}

	SendToBuf(psrv, iPort, pack, ipackLen);

	// 暂存一下，以便可以超时重发 udp
	TWarnMapKey key;
	key.seq = seq;
	key.dwSendTime = stConfig.dwCurrentTime;
	key.iSendTimes = 1;
	std::string strPack(pack, ipackLen);
	stConfig.sWarnUdpPackList.insert(std::make_pair(key, strPack));

	return ipackLen;
}


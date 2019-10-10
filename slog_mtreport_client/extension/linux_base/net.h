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

   内置监控插件 linux_base 功能:
   		使用监控系统 api 实现 linux 基础信息监控上报, 包括 cpu/内存/磁盘/网络

****/

#ifndef __MTREPORT_NET_H__
#define __MTREPORT_NET_H__ 1

#define ETH0_IN_PACK 193 
#define ETH0_OUT_PACK 192 
#define ETH0_IN_BYTES 191 
#define ETH0_OUT_BYTES 190 

#define ETH1_IN_PACK 197 
#define ETH1_OUT_PACK 196 
#define ETH1_IN_BYTES 195 
#define ETH1_OUT_BYTES 194 

#define LO_IN_PACK 201 
#define LO_OUT_PACK 200 
#define LO_IN_BYTES 199 
#define LO_OUT_BYTES 198 

#define ETHTOTAL_IN_PACK 55
#define ETHTOTAL_OUT_PACK 56
#define ETHTOTAL_IN_BYTES 57
#define ETHTOTAL_OUT_BYTES 58

#define ETH_COMM0_IN_PACK 59
#define ETH_COMM0_OUT_PACK 60 
#define ETH_COMM0_IN_BYTES 71 
#define ETH_COMM0_OUT_BYTES 76 

#define ETH_COMM1_IN_PACK 77 
#define ETH_COMM1_OUT_PACK 79 
#define ETH_COMM1_IN_BYTES 80 
#define ETH_COMM1_OUT_BYTES 82

#define NETIF_DROP_PACK 202

typedef struct 
{
	char szInterName[32];
	uint64_t qwRecvPackets;
	uint64_t qwSendPackets;
	uint64_t qwRecvBytes;
	uint64_t qwSendBytes;
	uint32_t dwRecvDropPackets;
	uint32_t dwSendDropPackets;
}TNetIfInfo;

void InitGetNet();
void ReportNetInfo();

#endif


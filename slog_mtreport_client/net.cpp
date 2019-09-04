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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/shm.h>
#include "mtreport_client.h"
#include "net.h"

static TNetIfInfo s_eth0;
static TNetIfInfo s_eth1;
static TNetIfInfo s_lo;
static TNetIfInfo s_eth_comm0; // 网卡名不是 lo,eth0,eth1 的网卡
static TNetIfInfo s_eth_comm1; // 网卡名不是 lo,eth0,eth1 的网卡
static TNetIfInfo s_total; // 全部网卡统计

void InitGetNet()
{
	// cmd: /bin/cat /proc/net/dev |awk '{if(NR!=1 && NR!=2) print $0 }' |sed 's/:/ /g' |awk '{ print $1" "$2" "$3" "$5" "$10" "$11" "$13}'
	FILE *fp = popen("/bin/cat /proc/net/dev |awk \'{ if(NR!=1 && NR!=2) print $0 }\' |sed 's/:/ /g'| awk \'{ print $1\" \"$2\" \"$3\" \"$5\" \"$10\" \"$11\" \"$13}\'", "r");
	if(fp == NULL) {
		return ;
	}

	memset(&s_total, 0, sizeof(s_total));
	strncpy(s_total.szInterName, "eth_total", sizeof(s_total.szInterName));

	TNetIfInfo stTmp;
	bool bUseComm0 = false;
	bool bUseComm1 = false;
	while( fscanf(fp, "%s%" PRIu64 "%" PRIu64 "%u%" PRIu64 "%" PRIu64 "%u",
		stTmp.szInterName, &stTmp.qwRecvBytes, &stTmp.qwRecvPackets, &stTmp.dwRecvDropPackets, 
		&stTmp.qwSendBytes, &stTmp.qwSendPackets, &stTmp.dwSendDropPackets) == 7)
	{
		DEBUG_LOG("read inter:%s, recv - bytes:%" PRIu64 " packets:%" PRIu64 " drop:%u ; send - "
			"bytes:%" PRIu64 " packets:%" PRIu64 " drop:%u", 
			stTmp.szInterName, stTmp.qwRecvBytes,  stTmp.qwRecvPackets, stTmp.dwRecvDropPackets,
			stTmp.qwSendBytes, stTmp.qwSendPackets, stTmp.dwSendDropPackets);

		if(strstr(stTmp.szInterName, "eth0"))
			memcpy(&s_eth0, &stTmp, sizeof(stTmp));
		else if(strstr(stTmp.szInterName, "eth1"))
			memcpy(&s_eth1, &stTmp, sizeof(stTmp));
		else if(strstr(stTmp.szInterName, "lo"))
			memcpy(&s_lo, &stTmp, sizeof(stTmp));
		else {
			INFO_LOG("unknow net interface:%s", stTmp.szInterName);
			if(!bUseComm0) {
				memcpy(&s_eth_comm0, &stTmp, sizeof(stTmp));
				bUseComm0 = true;
			}else if(!bUseComm1) {
				memcpy(&s_eth_comm1, &stTmp, sizeof(stTmp));
				bUseComm1 = true;
			}
		}

		s_total.qwRecvPackets += stTmp.qwRecvPackets;
		s_total.qwSendPackets += stTmp.qwSendPackets;
		s_total.qwRecvBytes += stTmp.qwRecvBytes;
		s_total.qwSendBytes += stTmp.qwSendBytes;
		s_total.dwRecvDropPackets += stTmp.dwRecvDropPackets;
		s_total.dwSendDropPackets += stTmp.dwSendDropPackets;
	}
	pclose(fp);
}

void ReportNetInfo()
{
	// cmd: /bin/cat /proc/net/dev |awk '{if(NR!=1 && NR!=2) print $0 }' |sed 's/:/ /g' |awk '{ print $1" "$2" "$3" "$5" "$10" "$11" "$13}'
	FILE *fp = popen("/bin/cat /proc/net/dev |awk \'{ if(NR!=1 && NR!=2) print $0 }\' |sed 's/:/ /g'| awk \'{ print $1\" \"$2\" \"$3\" \"$5\" \"$10\" \"$11\" \"$13}\'", "r");
	if(fp == NULL) {
		return ;
	}

	TNetIfInfo stTmp_total;
	memset(&stTmp_total, 0, sizeof(stTmp_total));
	strncpy(stTmp_total.szInterName, "eth_total", sizeof(stTmp_total.szInterName));

	TNetIfInfo stTmp;
	bool bUseComm0 = false;
	bool bUseComm1 = false;
	while( fscanf(fp, "%s%" PRIu64 "%" PRIu64 "%u%" PRIu64 "%" PRIu64 "%u",
		stTmp.szInterName, &stTmp.qwRecvBytes, &stTmp.qwRecvPackets, &stTmp.dwRecvDropPackets, 
		&stTmp.qwSendBytes, &stTmp.qwSendPackets, &stTmp.dwSendDropPackets) == 7)
	{
		DEBUG_LOG("read inter:%s, recv - bytes:%" PRIu64 " packets:%" PRIu64 " drop:%u ; send - "
			"bytes:%" PRIu64 " packets:%" PRIu64 " drop:%u", 
			stTmp.szInterName, stTmp.qwRecvBytes,  stTmp.qwRecvPackets, stTmp.dwRecvDropPackets,
			stTmp.qwSendBytes, stTmp.qwSendPackets, stTmp.dwSendDropPackets);

		if(strstr(stTmp.szInterName, "eth0"))
		{
			MtReport_Attr_Add(ETH0_IN_PACK, stTmp.qwRecvPackets-s_eth0.qwRecvPackets);
			MtReport_Attr_Add(ETH0_IN_BYTES, stTmp.qwRecvBytes-s_eth0.qwRecvBytes);
			MtReport_Attr_Add(ETH0_OUT_PACK, stTmp.qwSendPackets-s_eth0.qwSendPackets);
			MtReport_Attr_Add(ETH0_OUT_BYTES, stTmp.qwSendBytes-s_eth0.qwSendBytes);
			memcpy(&s_eth0, &stTmp, sizeof(stTmp));
		}
		else if(strstr(stTmp.szInterName, "eth1"))
		{
			MtReport_Attr_Add(ETH1_IN_PACK, stTmp.qwRecvPackets-s_eth1.qwRecvPackets);
			MtReport_Attr_Add(ETH1_IN_BYTES, stTmp.qwRecvBytes-s_eth1.qwRecvBytes);
			MtReport_Attr_Add(ETH1_OUT_PACK, stTmp.qwSendPackets-s_eth1.qwSendPackets);
			MtReport_Attr_Add(ETH1_OUT_BYTES, stTmp.qwSendBytes-s_eth1.qwSendBytes);
			memcpy(&s_eth1, &stTmp, sizeof(stTmp));
		}
		else if(strstr(stTmp.szInterName, "lo"))
		{
			MtReport_Attr_Add(LO_IN_PACK, stTmp.qwRecvPackets-s_lo.qwRecvPackets);
			MtReport_Attr_Add(LO_IN_BYTES, stTmp.qwRecvBytes-s_lo.qwRecvBytes);
			MtReport_Attr_Add(LO_OUT_PACK, stTmp.qwSendPackets-s_lo.qwSendPackets);
			MtReport_Attr_Add(LO_OUT_BYTES, stTmp.qwSendBytes-s_lo.qwSendBytes);
			memcpy(&s_lo, &stTmp, sizeof(stTmp));
		}
		else {
			INFO_LOG("unknow net interface:%s", stTmp.szInterName);
			if(!bUseComm0) {
				MtReport_Attr_Add(ETH_COMM0_IN_PACK, stTmp.qwRecvPackets-s_eth_comm0.qwRecvPackets);
				MtReport_Attr_Add(ETH_COMM0_IN_BYTES, stTmp.qwRecvBytes-s_eth_comm0.qwRecvBytes);
				MtReport_Attr_Add(ETH_COMM0_OUT_PACK, stTmp.qwSendPackets-s_eth_comm0.qwSendPackets);
				MtReport_Attr_Add(ETH_COMM0_OUT_BYTES, stTmp.qwSendBytes-s_eth_comm0.qwSendBytes);
				memcpy(&s_eth_comm0, &stTmp, sizeof(stTmp));
				bUseComm0 = true;
			}else if(!bUseComm1) {
				MtReport_Attr_Add(ETH_COMM1_IN_PACK, stTmp.qwRecvPackets-s_eth_comm1.qwRecvPackets);
				MtReport_Attr_Add(ETH_COMM1_IN_BYTES, stTmp.qwRecvBytes-s_eth_comm1.qwRecvBytes);
				MtReport_Attr_Add(ETH_COMM1_OUT_PACK, stTmp.qwSendPackets-s_eth_comm1.qwSendPackets);
				MtReport_Attr_Add(ETH_COMM1_OUT_BYTES, stTmp.qwSendBytes-s_eth_comm1.qwSendBytes);
				memcpy(&s_eth_comm1, &stTmp, sizeof(stTmp));
				bUseComm1 = true;
			}
		}

		stTmp_total.qwRecvPackets += stTmp.qwRecvPackets;
		stTmp_total.qwSendPackets += stTmp.qwSendPackets;
		stTmp_total.qwRecvBytes += stTmp.qwRecvBytes;
		stTmp_total.qwSendBytes += stTmp.qwSendBytes;
		stTmp_total.dwRecvDropPackets += stTmp.dwRecvDropPackets;
		stTmp_total.dwSendDropPackets += stTmp.dwSendDropPackets;
	}
	pclose(fp);

	MtReport_Attr_Add(ETHTOTAL_IN_PACK, stTmp_total.qwRecvPackets-s_total.qwRecvPackets);
	MtReport_Attr_Add(ETHTOTAL_IN_BYTES, stTmp_total.qwRecvBytes-s_total.qwRecvBytes);
	MtReport_Attr_Add(ETHTOTAL_OUT_PACK, stTmp_total.qwSendPackets-s_total.qwSendPackets);
	MtReport_Attr_Add(ETHTOTAL_OUT_BYTES, stTmp_total.qwSendBytes-s_total.qwSendBytes);
	if(stTmp_total.dwSendDropPackets > s_total.dwSendDropPackets
		|| stTmp_total.dwRecvDropPackets > s_total.dwRecvDropPackets)
	{
		INFO_LOG("get drop packet send:%u, recv:%u", stTmp_total.dwSendDropPackets-s_total.dwSendDropPackets,
			stTmp_total.dwRecvDropPackets-s_total.dwRecvDropPackets);
		MtReport_Attr_Add(NETIF_DROP_PACK, 1);
	}
	memcpy(&s_total, &stTmp_total, sizeof(stTmp_total));
}


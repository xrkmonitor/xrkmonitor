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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <comm.pb.h>
#include <supper_log.h>
#include <math.h>

#include "top_include_comm.h"
#include "udp_sock.h"
#include "cpu.h"
#include "mem.h"
#include "disk.h"
#include "net.h"

typedef struct
{
	time_t tmNow;
	int32_t iSock;
	AttrNode *pAttrBaseReport;
	SharedHashTable stHash;
	SharedHashTable stStrHash;
	uint32_t dwServerIP;
	uint16_t wServerPort;
	char szServerIP[32];
	TcpuUse stCpuUse;
	TMemInfo stMemInfo;
	uint32_t dwMaxUsePer;
	int32_t iReportPerSecond;
	MtSystemConfig *psysConfig;
}CONFIG;

CONFIG stConfig;
CSupperLog slog;
const int g_cpuAttr[] = { 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179 };
const int g_iMemUseAttr = 183;
const int g_iDiskUseTotalAttr = 184;
const int g_iDiskUseMaxAttr = 189;
const int g_iDiskUseOverPer95 = 188;
const int g_iDiskUseOverPer90 = 187;
const int g_iDiskUseOverPer80 = 185;
const int g_iDiskUseOverPer70 = 186;

using namespace comm;

inline int str_attr_cmp(const void *pKey, const void *pNode)
{           
	if(((const StrAttrNode*)pKey)->iStrAttrId == ((const StrAttrNode*)pNode)->iStrAttrId
		&& !strcmp(((const StrAttrNode*)pKey)->szStrInfo, ((const StrAttrNode*)pNode)->szStrInfo))
		return 0;
	return 1;
}   

inline int attr_cmp(const void *pKey, const void *pNode)
{
	if(((const AttrNode*)pKey)->iAttrID == ((const AttrNode*)pNode)->iAttrID)
		return 0;
	return 1;
}

static int Init(const char *papp)
{
	int32_t iRet = 0;
	const char *pconf = "./slog_monitor_client.conf";
	if((iRet=slog.InitCommon(pconf)) < 0)
	{
		ERR_LOG("InitCommon failed ret:%d", iRet);
		return SLOG_ERROR_LINE;
	}

	int iServerPort = 0;
	if((iRet=LoadConfig(pconf,
		"SERVER_IP", CFG_STRING, stConfig.szServerIP, "", sizeof(stConfig.szServerIP),
		"SERVER_PORT", CFG_INT, &iServerPort, 16000,
		"REPORT_ATTR_PER_SECOND", CFG_INT, &stConfig.iReportPerSecond, 10,
		(void*)NULL)) < 0)
	{   
		ERR_LOG("LoadConfig:%s failed ! ret:%d", pconf, iRet);
		return SLOG_ERROR_LINE;
	} 

	if(InitHashTable(&stConfig.stHash, sizeof(AttrNode), MAX_ATTR_NODE, DEP_SHM_ID, attr_cmp, NULL) != 0)
	{
		ERR_LOG("InitHashTable failed -- key:%d node size:%lu count:%d",
			DEP_SHM_ID, sizeof(AttrNode), MAX_ATTR_NODE);
		return SLOG_ERROR_LINE;
	}
	stConfig.stHash.bAccessCheck = 1;

	if(InitHashTable(&stConfig.stStrHash, sizeof(StrAttrNode),
		MAX_STR_ATTR_NODE_COUNT, DEF_STR_ATTR_NODE_SHM_KEY, str_attr_cmp, NULL) != 0)
	{
		ERR_LOG("InitHashTable(attr str) failed -- key:%d node size:%lu count:%d",
			DEF_STR_ATTR_NODE_SHM_KEY, sizeof(StrAttrNode), MAX_STR_ATTR_NODE_COUNT);
		return SLOG_ERROR_LINE;
	}
	stConfig.stStrHash.bAccessCheck = 1;

	if(GetShm2((void**)&stConfig.pAttrBaseReport,
	    DEP_BASE_SHM_ID, sizeof(AttrNode)*MAX_BASE_ATTR_NODE, 0666|IPC_CREAT) < 0)
	{
		ERR_LOG("attache AttrBaseReport shm failed -- key:%d size:%lu", 
			DEP_BASE_SHM_ID, sizeof(AttrNode)*MAX_BASE_ATTR_NODE);
		return SLOG_ERROR_LINE;
	}

	stConfig.iSock = socket(PF_INET, SOCK_DGRAM, 0);
	stConfig.wServerPort = iServerPort;
	if('\0' != stConfig.szServerIP[0])
		stConfig.dwServerIP = inet_addr(stConfig.szServerIP);
	else
		stConfig.dwServerIP = 0;

	stConfig.psysConfig = slog.GetSystemCfg();
	if(stConfig.psysConfig == NULL) {
		FATAL_LOG("GetSystemCfg failed");
		return SLOG_ERROR_LINE;
	}

	if(InitGetCpuUse() < 0)
	{
		FATAL_LOG("InitGetCpuUse failed !");
		return SLOG_ERROR_LINE;
	}
	InitGetNet();

	memset(&(stConfig.stCpuUse), 0, sizeof(stConfig.stCpuUse));
	stConfig.stCpuUse.iCpuCount = 0;
	DEBUG_LOG("monitor server ip:%s port:%d attr key:%d", stConfig.szServerIP, iServerPort, DEP_SHM_ID);
	return 0;
}

void ReportAttrPerM(CUdpSock &stSock)
{
	static uint32_t s_dwSeq = rand();
	int32_t iRet = 0;

	ReportAttr stReport;
	AttrInfo *pattr = NULL;
	stReport.set_bytes_report_ip(slog.GetLocalIP());
	stReport.set_uint32_client_rep_time(stConfig.tmNow);

	bool bReverse = false;
	_HashTableHead *pTableHead = (_HashTableHead*)stConfig.stHash.pHash;
	AttrNode *pNode = (AttrNode*)GetFirstNode(&stConfig.stHash);
	if(pNode == NULL && pTableHead->dwNodeUseCount > 0) {
		bReverse = true;
		pNode = (AttrNode*)GetFirstNodeRevers(&stConfig.stHash);
		ERR_LOG("man by bug - has node:%u, bug get first null", pTableHead->dwNodeUseCount);
	}

	int iDealNodeCount = 0;
	while(pNode != NULL)
	{
		if(!HASH_NODE_IS_NODE_USED(pNode))
		{
			MtReport_Attr_Spec(63, 1);
			ERR_LOG("check node error in attr list [not use] -- attr:%d value:%d",
				pNode->iAttrID, pNode->iCurValue);
		}

		if(pNode->bSyncProcess != 1)
		{
			MtReport_Attr_Spec(63, 1);
			ERR_LOG("check node error in attr list [syncflag] -- attr:%d value:%d bSyncProcess:%d",
				pNode->iAttrID, pNode->iCurValue, pNode->bSyncProcess);
		}

		pattr = stReport.add_msg_attr_info();
		pattr->set_uint32_attr_id(pNode->iAttrID);
		pattr->set_uint32_attr_value(pNode->iCurValue);
		if((iRet=RemoveHashNode(&stConfig.stHash, pNode)) < 0)
		{
			MtReport_Attr_Spec(66, 1);
			ERR_LOG("check node error in attr list [remove] -- ret:%d", iRet);
			break;
		}
		pNode->bSyncProcess = 0;
		if(bReverse)
			pNode = (AttrNode*)GetCurNodeRevers(&stConfig.stHash);
		else
			pNode = (AttrNode*)GetCurNode(&stConfig.stHash);
		iDealNodeCount++;
	}

	if(bReverse) {
		INFO_LOG("link may bug, node use remain:%u, deal node:%d", pTableHead->dwNodeUseCount, iDealNodeCount);
	}

	if(bReverse && (iDealNodeCount == 0 || pTableHead->dwNodeUseCount <=5 ))  {
		ResetHashTable(&stConfig.stHash);
		stConfig.stHash.bAccessCheck = 1;
	}
	else if(!bReverse && pTableHead->dwNodeUseCount <= 0) {
		stConfig.stHash.bAccessCheck = 0;
	}

	for(int i=0; i < MAX_BASE_ATTR_NODE; i++)
	{
		if(stConfig.pAttrBaseReport[i].iAttrID != 0 && stConfig.pAttrBaseReport[i].iCurValue != 0) 
		{
			pattr = stReport.add_msg_attr_info();
			pattr->set_uint32_attr_id(stConfig.pAttrBaseReport[i].iAttrID);
			pattr->set_uint32_attr_value(stConfig.pAttrBaseReport[i].iCurValue);
		}
		stConfig.pAttrBaseReport[i].iAttrID = 0;
		stConfig.pAttrBaseReport[i].iCurValue = 0;
	}

	if(stReport.msg_attr_info_size() == 0)
	{
		DEBUG_LOG("have no attr report !");
		return;
	}

	PkgHead head;
	head.set_en_cmd(::comm::MONITOR_CLIENT_REPORT_ATTR);
	head.set_uint32_seq(s_dwSeq++);

	std::string strHead, strBody;
	if(!head.AppendToString(&strHead) || !stReport.AppendToString(&strBody))
	{
		ERR_LOG("AppendToString ReportAttr failed !");
		return;
	}
	stSock.SendPacketPb(strHead, strBody);
	DEBUG_LOG("report attr count:%d, info:%s", stReport.msg_attr_info_size(), stReport.ShortDebugString().c_str());
}

void ReportStrAttrPerM(CUdpSock &stSock)
{
	static uint32_t s_dwSeq = rand();
	int32_t iRet = 0;

	ReportAttr stReport;
	AttrInfo *pattr = NULL;
	stReport.set_bytes_report_ip(slog.GetLocalIP());
	stReport.set_uint32_client_rep_time(stConfig.tmNow);

	bool bReverse = false;
	_HashTableHead *pTableHead = (_HashTableHead*)stConfig.stStrHash.pHash;
	StrAttrNode *pNode = (StrAttrNode*)GetFirstNode(&stConfig.stStrHash);
	if(pNode == NULL && pTableHead->dwNodeUseCount > 0) {
		bReverse = true;
		pNode = (StrAttrNode*)GetFirstNodeRevers(&stConfig.stStrHash);
		ERR_LOG("man by bug - has node:%u, bug get first null", pTableHead->dwNodeUseCount);
	}

	int iDealNodeCount = 0;
	while(pNode != NULL)
	{
		if(!HASH_NODE_IS_NODE_USED(pNode))
		{
			MtReport_Attr_Spec(63, 1);
			ERR_LOG("check node error in str attr list [not use] -- attr:%d value:%d",
				pNode->iStrAttrId, pNode->iStrVal);
		}

		if(pNode->bSyncProcess != 1)
		{
			MtReport_Attr_Spec(63, 1);
			ERR_LOG("check node error in str attr list [syncflag] -- attr:%d value:%d bSyncProcess:%d",
				pNode->iStrAttrId, pNode->iStrVal, pNode->bSyncProcess);
		}

		pattr = stReport.add_msg_attr_info();
		pattr->set_uint32_attr_id(pNode->iStrAttrId);
		pattr->set_uint32_attr_value(pNode->iStrVal);
		pattr->set_str(pNode->szStrInfo);
		if((iRet=RemoveHashNode(&stConfig.stStrHash, pNode)) < 0)
		{
			MtReport_Attr_Spec(66, 1);
			ERR_LOG("check node error in str attr list [remove] -- ret:%d", iRet);
			break;
		}
		pNode->bSyncProcess = 0;
		if(bReverse)
			pNode = (StrAttrNode*)GetCurNodeRevers(&stConfig.stStrHash);
		else
			pNode = (StrAttrNode*)GetCurNode(&stConfig.stStrHash);
		iDealNodeCount++;
	}

	if(bReverse) {
		INFO_LOG("link may bug, node use remain:%u, deal node:%d", pTableHead->dwNodeUseCount, iDealNodeCount);
	}
	if(bReverse && (iDealNodeCount == 0 || pTableHead->dwNodeUseCount <=5 ))  {
		ResetHashTable(&stConfig.stStrHash);
		stConfig.stStrHash.bAccessCheck = 1;
	}
	else if(!bReverse && pTableHead->dwNodeUseCount <= 0) {
		stConfig.stStrHash.bAccessCheck = 0;
	}

	if(stReport.msg_attr_info_size() == 0)
	{
		DEBUG_LOG("have no str attr report !");
		return;
	}
	pattr = stReport.add_msg_attr_info();
	pattr->set_uint32_attr_id(456);
	pattr->set_uint32_attr_value(stReport.msg_attr_info_size());

	PkgHead head;
	head.set_en_cmd(::comm::MONITOR_CLIENT_REPORT_ATTR);
	head.set_uint32_seq(s_dwSeq++);

	std::string strHead, strBody;
	if(!head.AppendToString(&strHead) || !stReport.AppendToString(&strBody))
	{
		ERR_LOG("AppendToString ReportAttr failed !");
		return;
	}
	stSock.SendPacketPb(strHead, strBody);
	DEBUG_LOG("report str attr count:%d, info:%s", 
		stReport.msg_attr_info_size(), stReport.ShortDebugString().c_str());
}

void GetMemUse()
{
	memset(&stConfig.stMemInfo, 0, sizeof(stConfig.stMemInfo));
	if(GetMemInfo(stConfig.stMemInfo) < 0)
	{
		WARN_LOG("GetMemInfo failed !");
		return;
	}

	uint32_t dwFree = stConfig.stMemInfo.dwMemFree + stConfig.stMemInfo.dwCached
		- stConfig.stMemInfo.dwDirty - stConfig.stMemInfo.dwMapped;

	DEBUG_LOG("get meminfo - unit:%s total:%u free:%u buffers:%u cached:%u dirty:%u maped:%u",
		stConfig.stMemInfo.szUnit, stConfig.stMemInfo.dwMemTotal, 
		stConfig.stMemInfo.dwMemFree, stConfig.stMemInfo.dwBuffers,
		stConfig.stMemInfo.dwCached, stConfig.stMemInfo.dwDirty, stConfig.stMemInfo.dwMapped);

	uint32_t dwUsePer = (stConfig.stMemInfo.dwMemTotal-dwFree)*100/stConfig.stMemInfo.dwMemTotal;
	if(dwUsePer > stConfig.dwMaxUsePer)
		stConfig.dwMaxUsePer = dwUsePer;
	DEBUG_LOG("get mem use info -- real free:%u total:%u useper:%%%u", dwFree,
		stConfig.stMemInfo.dwMemTotal, dwUsePer);
}

void WriteMemInfo()
{
	if(stConfig.dwMaxUsePer > 0)
	{
		MtReport_Attr_Set(g_iMemUseAttr, stConfig.dwMaxUsePer);
		stConfig.dwMaxUsePer = 0;
	}
}

void WriteDiskInfo()
{
	uint64_t qwTotalSpace = 0, qwTotalUse = 0;
	uint32_t maxUsePer = 0;
	if(GetDiskInfo(qwTotalSpace, qwTotalUse, maxUsePer) < 0)
	{
		WARN_LOG("GetDiskInfo failed");
		return;
	}

	uint32_t dwUsePerTotal = (uint32_t)ceil((qwTotalUse*100.0/qwTotalSpace));
	MtReport_Attr_Set(g_iDiskUseTotalAttr, dwUsePerTotal);
	MtReport_Attr_Set(g_iDiskUseMaxAttr, maxUsePer);
	if(maxUsePer > 95)
		MtReport_Attr_Add(g_iDiskUseOverPer95, 1);
	else if(maxUsePer > 90)
		MtReport_Attr_Add(g_iDiskUseOverPer90, 1);
	else if(maxUsePer > 80)
		MtReport_Attr_Add(g_iDiskUseOverPer80, 1);
	else if(maxUsePer > 70)
		MtReport_Attr_Add(g_iDiskUseOverPer70, 1);
	DEBUG_LOG("get disk use total percent:%%%u, max use percent:%%%u", dwUsePerTotal, maxUsePer);
}

int main(int argc, char* argv[])
{
	static uint32_t s_dwLoopIp = inet_addr("127.0.0.1");

	int iRet = 0;
	if((iRet=Init(argv[0])) < 0)
	{
		ERR_LOG("Init Failed ret:%d !", iRet);
		return SLOG_ERROR_LINE;
	}

	slog.Daemon(1, 1);
	SocketHandler h(&slog);
	CUdpSock stSock(h);
	stSock.SetServer(stConfig.dwServerIP, stConfig.wServerPort);
	h.Add(&stSock);

	time_t tmLastReportPerM = 0;
	time_t tmLastGetBasicUse = 0;
	SLogServer *psrv_use = NULL;
	while(h.GetCount() && slog.Run())
	{
		stConfig.tmNow = slog.m_stNow.tv_sec;

		if('\0' == stConfig.szServerIP[0])
		{
			if(slog.CheckAttrServer() < 0)
			{
				ERR_LOG("CheckAttrServer failed");
				break;
			}

			SLogServer *psrv = slog.GetServerInfo(stConfig.psysConfig->iAttrSrvIndex);
			if(psrv != psrv_use || 
				psrv->dwIp != stConfig.dwServerIP || psrv->wPort != stConfig.wServerPort)
			{
				if(slog.IsIpMatchLocalMachine(psrv->dwIp))
					stConfig.dwServerIP = s_dwLoopIp;
				else
					stConfig.dwServerIP = psrv->dwIp;
				stConfig.wServerPort = psrv->wPort;
				stSock.SetServer(stConfig.dwServerIP, stConfig.wServerPort);
				INFO_LOG("update monitor server ip:%s port:%d attr key:%d", psrv->szIpV4, stConfig.wServerPort, DEP_SHM_ID);
				psrv_use = psrv;
			}
		}

		if(stConfig.tmNow >= tmLastReportPerM+stConfig.iReportPerSecond)
		{
			WriteCpuUse(stConfig.stCpuUse, g_cpuAttr);
			WriteMemInfo();
			WriteDiskInfo();
			ReportNetInfo();

			ReportAttrPerM(stSock);
			ReportStrAttrPerM(stSock);
			tmLastReportPerM = stConfig.tmNow+(slog.m_iRand%5);
		}

		h.Select(1, 1000);

		if(stConfig.tmNow >= 5+tmLastGetBasicUse)
		{
			GetMemUse();
			ReadCpuUse(stConfig.stCpuUse, sizeof(g_cpuAttr)/sizeof(int));
			tmLastGetBasicUse = stConfig.tmNow+(rand()%5);
		}
	}
	return 0;
}


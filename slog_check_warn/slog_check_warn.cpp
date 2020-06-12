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

   模块 slog_check_warn 功能:
       检查监控点上报的数据是否触发监控告警 

****/

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <myparam_comm.h>
#include <errno.h>
#include <list>
#include <map>
#include "top_include_comm.h"

using namespace std;

typedef struct
{
	int iPerCheck;
	int iSendWarnShmKey;
	int iValidSendWarnTimeSec;
	TWarnSendInfo *pSendWarnShm;
	MtSystemConfig *psysConfig;
	int iNotSendWarnAfterDealTimeCfg; // 开始处理告警后多少时间内告警不发送

	char szLocalIp[20];
	Database *db;
	Query *qu;
	uint32_t dwCurrentTime;
}CONFIG;	

CONFIG stConfig;
CSupperLog slog;

int Init(const char *pConfFile)
{
	int32_t iRet = 0;
	if((iRet=LoadConfig(pConfFile,
		"PER_SECOND_CHECK", CFG_INT, &stConfig.iPerCheck, 1, 
		"WARN_DEAL_TIME_SEC", CFG_INT, &stConfig.iNotSendWarnAfterDealTimeCfg, 7200, 
		"VALID_SEND_WARN_TIME_SEC", CFG_INT, &stConfig.iValidSendWarnTimeSec, DEF_WARN_SEND_INFO_VALID_TIME_SEC, 
		"DEF_SEND_WARN_SHM_KEY", CFG_INT, &stConfig.iSendWarnShmKey, DEF_SEND_WARN_SHM_KEY, 
		"LOCAL_IF_NAME", CFG_STRING, stConfig.szLocalIp, "eth0", MYSIZEOF(stConfig.szLocalIp),
		(void*)NULL)) < 0)
	{   
		ERR_LOG("LoadConfig:%s failed ! ret:%d", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	} 

	if(stConfig.szLocalIp[0] == '\0' || INADDR_NONE == inet_addr(stConfig.szLocalIp))
		GetCustLocalIP(stConfig.szLocalIp);
	if(stConfig.szLocalIp[0] == '\0' || INADDR_NONE == inet_addr(stConfig.szLocalIp))
	{
		ERR_LOG("get local ip failed, use LOCAL_IP to set !");
		return SLOG_ERROR_LINE;
	}

	if((iRet=slog.InitConfigByFile(pConfFile)) < 0)
		return SLOG_ERROR_LINE;
	if((iRet=slog.Init(stConfig.szLocalIp)) < 0)
		return SLOG_ERROR_LINE;

	if(slog.InitWarnAttrList() < 0)
	{
		FATAL_LOG("init warn attr shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitwarnConfig() < 0)
	{
		FATAL_LOG("init warn config shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitMachineViewConfig() < 0)
	{
		ERR_LOG("init InitwarnConfig info shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitAttrViewConfig() < 0)
	{
		ERR_LOG("init InitwarnConfig info shm failed !");
		return SLOG_ERROR_LINE;
	}

	stConfig.psysConfig = slog.GetSystemCfg();
	if(stConfig.psysConfig == NULL) 
	{
		ERR_LOG("GetSystemCfg failed");
		return SLOG_ERROR_LINE;
	}

	if(!(stConfig.pSendWarnShm=(TWarnSendInfo*)GetShm(stConfig.iSendWarnShmKey, 
		sizeof(TWarnSendInfo)*MAX_WARN_SEND_NODE_SHM, 0666|IPC_CREAT)))
	{
		ERR_LOG("init warn send shm failed, key:%d, size:%d", stConfig.iSendWarnShmKey,
			(int)sizeof(TWarnSendInfo)*MAX_WARN_SEND_NODE_SHM);
		return SLOG_ERROR_LINE;
	}

	if(stConfig.iPerCheck <= 0 || stConfig.iPerCheck >= 300)
		stConfig.iPerCheck = 1;
	return 0;
}

int GetDayOfMin(uint32_t timeSec)
{
	char * ptime_str = uitodate(timeSec);
	char *phour = strchr(ptime_str, ':');
	char *pmin = phour+1;
	if(phour == NULL)
		return 0;
	*phour = '\0';
	phour -= 2;
	pmin[2] = '\0';

	int i = atoi(phour)*60+atoi(pmin);
	if(i >= 1440)
		    return 1439;
	if(i <= 0)
		    return 0;
	return i;
}

void AdaptReportValue(
	TWarnAttrReportInfo *pNode, uint32_t now, int iDayMinIdx, uint32_t & dwLastVal, uint32_t & dwPreLastVal)
{
	dwLastVal = pNode->dwLastVal;
	dwPreLastVal = pNode->dwPreLastVal;

	if(pNode->bAttrDataType == SUM_REPORT_TOTAL)
	{
		// 历史累积数据类型监控点
		if(dwPreLastVal >= dwLastVal)
			dwLastVal = dwPreLastVal;
		return ;
	}
	// 考虑到可能是无上报或者丢失了的情况,需要修正下共享内存的上报数据
	if(now >= pNode->dwLastReportTime+1440*60)
	{
		dwLastVal = 0;
		dwPreLastVal = 0;
	}
	else if(iDayMinIdx != pNode->iMinIdx)
	{
		if(((pNode->iMinIdx+1)%1440) == iDayMinIdx) 
		{
			dwPreLastVal = dwLastVal;
			dwLastVal = pNode->dwCurVal;
		}
		else if(((pNode->iMinIdx+2)%1440) == iDayMinIdx)
		{
			dwPreLastVal = pNode->dwCurVal;
			dwLastVal = 0;
		}
		else 
		{
			dwLastVal = 0;
			dwPreLastVal = 0;
		}
		DEBUG_LOG("check report attr:%d, machine:%d, last report time:%s, value:%d|%d, dayOfMin:%d|%d", 
			pNode->iAttrId, pNode->iMachineId, uitodate(pNode->dwLastReportTime),
			dwLastVal, dwPreLastVal, pNode->iMinIdx, iDayMinIdx);
	}
}

void AdaptViewReportValue(TWarnConfig *pWarnConfig, uint32_t now, int iDayMinIdx)
{
	uint32_t dwPreLastValView = pWarnConfig->dwReserved1;
	uint32_t dwLastValView = pWarnConfig->dwReserved2;
	int32_t iDayMinIdxView = GetDayOfMin(pWarnConfig->dwReserved3);

	DEBUG_LOG("before adapt view report, attr:%d, view:%d, value:%u|%u, time:%s, iMinIdx:%d(%d)",
		pWarnConfig->iWarnId, pWarnConfig->iAttrId, dwLastValView, dwPreLastValView, 
		uitodate(pWarnConfig->dwReserved3), iDayMinIdxView, iDayMinIdx);
	if(now >= pWarnConfig->dwReserved3+1440*60)
	{
		dwLastValView = 0;
		dwPreLastValView = 0;
	}
	else if(iDayMinIdxView != iDayMinIdx)
	{
		if(((iDayMinIdxView+1)%1440) == iDayMinIdx)
		{
			dwPreLastValView = dwLastValView;
			dwLastValView = 0;
		}
		else
		{
			dwPreLastValView = 0;
			dwLastValView = 0;
		}
	}
	pWarnConfig->dwReserved1 = dwPreLastValView;
	pWarnConfig->dwReserved2 = dwLastValView;
	pWarnConfig->dwReserved3 = now;
	DEBUG_LOG("after adapt view report, attr:%d, view:%d, value:%u|%u",
		pWarnConfig->iWarnId, pWarnConfig->iAttrId, dwLastValView, dwPreLastValView);
}

void AddReportValForViewWarn(
	TWarnConfig *pWarnConfig, TWarnAttrReportInfo *pNode, uint32_t now, int iDayMinIdx)
{
	uint32_t dwLastVal=0, dwPreLastVal=0;
	AdaptReportValue(pNode, now, iDayMinIdx, dwLastVal, dwPreLastVal);
	pWarnConfig->dwReserved1 += dwPreLastVal;
	pWarnConfig->dwReserved2 += dwLastVal;
	DEBUG_LOG("add machine report to view, machine:%d, view:%d, attr:%d, value:%u|%u",
		pNode->iMachineId, pWarnConfig->iWarnId, pNode->iAttrId, pWarnConfig->dwReserved2, pWarnConfig->dwReserved1);
}

void CheckViewWarn(TWarnConfig *pWarnConfig, list<TAttrWarnInfo> &listWarn, uint32_t now)
{
	uint32_t dwPreLastVal=pWarnConfig->dwReserved1;
	uint32_t dwLastVal=pWarnConfig->dwReserved2;

	if((pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MIN) && dwLastVal < (uint32_t)pWarnConfig->iWarnMin)
	{
		TAttrWarnInfo wInfo;
		wInfo.iWarnFlag = ATTR_WARN_FLAG_MIN|ATTR_WARN_FLAG_TYPE_VIEW;
		if(pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MASK_WARN)
			wInfo.iWarnFlag |= ATTR_WARN_FLAG_MASK_WARN;

		wInfo.iWarnId = pWarnConfig->iWarnId;
		wInfo.iAttrId = pWarnConfig->iAttrId;
		wInfo.iWarnConfigValue = pWarnConfig->iWarnMin;
		wInfo.dwWarnValue = dwLastVal;
		wInfo.dwWarnTimeSec = now;
		listWarn.push_back(wInfo);
		INFO_LOG("get attr view min warn info - attr:%d, config:%d, value:%d, view:%d",
			wInfo.iAttrId, wInfo.iWarnConfigValue, wInfo.dwWarnValue, wInfo.iWarnId);
	}

	if((pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MAX) && dwLastVal > (uint32_t)pWarnConfig->iWarnMax)
	{
		TAttrWarnInfo wInfo;
		wInfo.iWarnFlag = ATTR_WARN_FLAG_MAX|ATTR_WARN_FLAG_TYPE_VIEW;
		if(pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MASK_WARN)
			wInfo.iWarnFlag |= ATTR_WARN_FLAG_MASK_WARN;

		wInfo.iWarnId = pWarnConfig->iWarnId;
		wInfo.iAttrId = pWarnConfig->iAttrId;
		wInfo.iWarnConfigValue = pWarnConfig->iWarnMax;
		wInfo.dwWarnValue = dwLastVal;
		wInfo.dwWarnTimeSec = now;
		listWarn.push_back(wInfo);
		INFO_LOG("get attr view max warn info - attr:%d, config:%d, value:%d, view:%d",
			wInfo.iAttrId, wInfo.iWarnConfigValue, wInfo.dwWarnValue, wInfo.iWarnId);
	}

	if((pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_WAVE) && dwPreLastVal > 0)
	{
		int w = (abs((int)(dwLastVal-dwPreLastVal))*100) / dwPreLastVal;
		if(w > pWarnConfig->iWarnWave) {
			TAttrWarnInfo wInfo;
			wInfo.iWarnFlag = ATTR_WARN_FLAG_WAVE|ATTR_WARN_FLAG_TYPE_VIEW;
			if(pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MASK_WARN)
				wInfo.iWarnFlag |= ATTR_WARN_FLAG_MASK_WARN;

			wInfo.iWarnId = pWarnConfig->iWarnId;
			wInfo.iAttrId = pWarnConfig->iAttrId;
			wInfo.iWarnConfigValue = pWarnConfig->iWarnWave;
			wInfo.dwWarnValue = w;
			wInfo.dwWarnTimeSec = now;
			listWarn.push_back(wInfo);
			INFO_LOG("get attr view wave warn info - attr:%d, config:%d, value:%d, view:%d",
				wInfo.iAttrId, wInfo.iWarnConfigValue, wInfo.dwWarnValue, wInfo.iWarnId);
		}
	}
}

inline bool IsBasicAttr(int iAttrId)
{
    return false;
}

void CheckMachineWarn(TWarnConfig *pWarnConfig, 
	TWarnAttrReportInfo *pNode, list<TAttrWarnInfo> &listWarn, uint32_t now, int iDayMinIdx)
{
	uint32_t dwLastVal=0, dwPreLastVal=0;
	AdaptReportValue(pNode, now, iDayMinIdx, dwLastVal, dwPreLastVal);
	MachineInfo *pMachineInfo = slog.GetMachineInfo(pWarnConfig->iWarnId, NULL);
	if(pMachineInfo == NULL) {
		ERR_LOG("not find machine:%d", pWarnConfig->iWarnId);
		return;
	}

	if((pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MIN) && dwLastVal < (uint32_t)pWarnConfig->iWarnMin)
	{
		TAttrWarnInfo wInfo;
		wInfo.iWarnFlag = ATTR_WARN_FLAG_MIN|ATTR_WARN_FLAG_TYPE_MACHINE;
		if((pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MASK_WARN)
			|| (pMachineInfo->bWarnFlag & MACH_WARN_DENY_ALL)
			|| ((pMachineInfo->bWarnFlag & MACH_WARN_DENY_EXCEPT) && (pWarnConfig->iWarnConfigFlag&ATTR_WARN_FLAG_EXCEPTION))
			|| ((pMachineInfo->bWarnFlag & MACH_WARN_DENY_BASIC) && IsBasicAttr(pNode->iAttrId)))
		{
			wInfo.iWarnFlag |= ATTR_WARN_FLAG_MASK_WARN;
		}

		wInfo.iWarnId = pNode->iMachineId;
		wInfo.iAttrId = pNode->iAttrId;
		wInfo.iWarnConfigValue = pWarnConfig->iWarnMin;
		wInfo.dwWarnValue = dwLastVal;
		wInfo.dwWarnTimeSec = now;
		listWarn.push_back(wInfo);
		INFO_LOG("get attr machine min warn info - attr:%d, config:%d, value:%d, machine:%d",
			wInfo.iAttrId, wInfo.iWarnConfigValue, wInfo.dwWarnValue, wInfo.iWarnId);
	}

	if((pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MAX) && dwLastVal > (uint32_t)pWarnConfig->iWarnMax)
	{
		TAttrWarnInfo wInfo;
		wInfo.iWarnFlag = ATTR_WARN_FLAG_MAX|ATTR_WARN_FLAG_TYPE_MACHINE;
		if((pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MASK_WARN)
			|| (pMachineInfo->bWarnFlag & MACH_WARN_DENY_ALL)
			|| ((pMachineInfo->bWarnFlag & MACH_WARN_DENY_EXCEPT) && (pWarnConfig->iWarnConfigFlag&ATTR_WARN_FLAG_EXCEPTION))
			|| ((pMachineInfo->bWarnFlag & MACH_WARN_DENY_BASIC) && IsBasicAttr(pNode->iAttrId)))
		{
			wInfo.iWarnFlag |= ATTR_WARN_FLAG_MASK_WARN;
		}

		wInfo.iWarnId = pNode->iMachineId;
		wInfo.iAttrId = pNode->iAttrId;
		wInfo.iWarnConfigValue = pWarnConfig->iWarnMax;
		wInfo.dwWarnValue = dwLastVal;
		wInfo.dwWarnTimeSec = now;
		listWarn.push_back(wInfo);
		INFO_LOG("get attr machine max warn info - attr:%d, config:%d, value:%d, machine:%d",
			wInfo.iAttrId, wInfo.iWarnConfigValue, wInfo.dwWarnValue, wInfo.iWarnId);
	}

	if(pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_WAVE)
	{
		int w = (abs((int)(dwLastVal-dwPreLastVal))*100) / dwPreLastVal;
		if(w > pWarnConfig->iWarnWave) {
			TAttrWarnInfo wInfo;
			wInfo.iWarnFlag = ATTR_WARN_FLAG_WAVE|ATTR_WARN_FLAG_TYPE_MACHINE;
			if((pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MASK_WARN)
				|| (pMachineInfo->bWarnFlag & MACH_WARN_DENY_ALL)
				|| ((pMachineInfo->bWarnFlag & MACH_WARN_DENY_EXCEPT) && (pWarnConfig->iWarnConfigFlag&ATTR_WARN_FLAG_EXCEPTION))
				|| ((pMachineInfo->bWarnFlag & MACH_WARN_DENY_BASIC) && IsBasicAttr(pNode->iAttrId)))
			{
				wInfo.iWarnFlag |= ATTR_WARN_FLAG_MASK_WARN;
			}

			wInfo.iWarnId = pNode->iMachineId;
			wInfo.iAttrId = pNode->iAttrId;
			wInfo.iWarnConfigValue = pWarnConfig->iWarnWave;
			wInfo.dwWarnValue = w;
			wInfo.dwWarnTimeSec = now;
			listWarn.push_back(wInfo);
			INFO_LOG("get attr machine wave warn info - attr:%d, config:%d, value:%d, machine:%d",
				wInfo.iAttrId, wInfo.iWarnConfigValue, wInfo.dwWarnValue, wInfo.iWarnId);
		}
	}
}

void GetReportViews(TWarnAttrReportInfo *pNode, list<int> &listView)
{
	TMachineViewConfigInfo *pMachineView = slog.GetMachineViewInfo(pNode->iMachineId, NULL);
	TAttrViewConfigInfo *pAttrView = slog.GetAttrViewInfo(pNode->iAttrId, NULL);
	if(pMachineView == NULL || pAttrView == NULL)
	{
		DEBUG_LOG("not find report view -- %p|%p, iMachineId:%d, iAttrId:%d", 
			pMachineView, pAttrView, pNode->iMachineId, pNode->iAttrId);
		return;
	}

	for(int i=0; i < pMachineView->iBindViewCount; i++)
	{
		for(int j=0; j < pAttrView->iBindViewCount; j++)
		{
			if(pMachineView->aryView[i] == pAttrView->aryView[j])
			{
				listView.push_back(pMachineView->aryView[i]);
				DEBUG_LOG("GetReportViews - view:%d, iMachineId:%d, iAttrId:%d", 
					pMachineView->aryView[i], pNode->iMachineId, pNode->iAttrId);
				break;
			}
		}
	}

	DEBUG_LOG("GetReportViews - iMachineId:%d, iAttrId:%d, view count:%d",
		pNode->iMachineId, pNode->iAttrId, (int)listView.size());
}

int DealDbConnect()
{
	static uint32_t s_dwLastCheckDbTime = 0;

	// 5 - 10 秒 check 一次
	if(stConfig.dwCurrentTime <= s_dwLastCheckDbTime)
		return 0;
	s_dwLastCheckDbTime = stConfig.dwCurrentTime+5+rand()%5;

	if(stConfig.db != NULL && stConfig.qu != NULL) {
		if(stConfig.qu->Connected())
			return 0;
		delete stConfig.db;
		stConfig.db = NULL;
		delete stConfig.qu;
		stConfig.qu = NULL;
	}

	stConfig.db = new Database(stConfig.psysConfig->szDbHost, stConfig.psysConfig->szUserName,
		stConfig.psysConfig->szPass, stConfig.psysConfig->szDbName, &slog);
	stConfig.qu = new Query(*stConfig.db);
	if(stConfig.db && stConfig.qu && stConfig.qu->Connected()) {
		INFO_LOG("connect to db:%s ok dbname:%s", stConfig.psysConfig->szDbHost, stConfig.psysConfig->szDbName);
		return 0;
	}
	ERR_LOG("connect to database failed ! dbhost:%s, dbname:%s", stConfig.psysConfig->szDbHost, stConfig.psysConfig->szDbName);
	delete stConfig.db;
	stConfig.db = NULL;
	delete stConfig.qu;
	stConfig.qu = NULL;
	return 0;
}

void AddSendWarnToShm(TAttrWarnInfo & warn, uint32_t dwWarnId, uint32_t dwTimeNow)
{
	static int s_iLastShmNodeIdx = 0;

	TWarnSendInfo *pSendNode = NULL, *pTimeoutNode = NULL;
	int i=0, j=0;
	for(i=s_iLastShmNodeIdx, j=0; j < MAX_WARN_SEND_NODE_SHM; j++)
	{
		pSendNode = stConfig.pSendWarnShm+i;
		if(0 == pSendNode->dwWarnId)
			break;
		if(NULL == pTimeoutNode && pSendNode->dwWarnAddTime+stConfig.iValidSendWarnTimeSec < dwTimeNow)
		{
			pTimeoutNode = pSendNode;
		}

		i++;
		if(i >= MAX_WARN_SEND_NODE_SHM)
			i = 0;
	}

	if(j >= MAX_WARN_SEND_NODE_SHM && pTimeoutNode != NULL)
	{
		INFO_LOG("use timeout node, wid:%u, add time:%s, for send warn:%u", 
			pTimeoutNode->dwWarnId, uitodate(pTimeoutNode->dwWarnAddTime), dwWarnId);
		pSendNode = pTimeoutNode;
		MtReport_Attr_Add(294, 1);
	}
	else if(j >= MAX_WARN_SEND_NODE_SHM || NULL == pSendNode)
	{
		WARN_LOG("add send warn info to shm failed - warn info id:%u, attr id:%d", dwWarnId, warn.iAttrId);
		return;
	}

	memcpy(&(pSendNode->stWarn), &warn, sizeof(TAttrWarnInfo));
	pSendNode->dwWarnAddTime = dwTimeNow;
	pSendNode->dwWarnId = dwWarnId;

	s_iLastShmNodeIdx = i+1;
	if(s_iLastShmNodeIdx >= MAX_WARN_SEND_NODE_SHM)
		s_iLastShmNodeIdx = 0;

	INFO_LOG("add send warn info to shm ok, warn id:%u, attr id:%d", dwWarnId, warn.iAttrId);
}
void CheckWarnConfig(list<TAttrWarnInfo> &listWarn)
{
	SharedHashTable *phash = slog.GetWarnConfigInfoHash();
	bool bReverse = false;
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	TWarnConfig *pNode = (TWarnConfig*)GetFirstNode(phash);
	if(pNode == NULL && pTableHead->dwNodeUseCount > 0) {
		bReverse = true;
		pNode = (TWarnConfig*)GetFirstNodeRevers(phash);
		ERR_LOG("man bug - has node:%u, bug get first null", pTableHead->dwNodeUseCount);
	}
	MachineInfo *pMachineInfo = NULL;
	while(pNode != NULL)
	{
		if(!(pNode->iWarnConfigFlag & ATTR_WARN_FLAG_MIN)
			|| slog.GetWarnAttrInfo(pNode->iAttrId, pNode->iWarnId, NULL) != NULL)
		{
			goto check_warn_next;
		}

		pMachineInfo = NULL;
		if(pNode->iWarnConfigFlag & ATTR_WARN_FLAG_TYPE_MACHINE) {
			pMachineInfo = slog.GetMachineInfo(pNode->iWarnId, NULL);
			if(pMachineInfo == NULL) {
				ERR_LOG("not find machine:%d", pNode->iWarnId);
				goto check_warn_next;
			}
		}
	
		TAttrWarnInfo wInfo;
		wInfo.iWarnFlag = ATTR_WARN_FLAG_MIN;
		if(pNode->iWarnConfigFlag & ATTR_WARN_FLAG_TYPE_MACHINE)
			wInfo.iWarnFlag |= ATTR_WARN_FLAG_TYPE_MACHINE;
		else
			wInfo.iWarnFlag |= ATTR_WARN_FLAG_TYPE_VIEW;
		if(pNode->iWarnConfigFlag & ATTR_WARN_FLAG_MASK_WARN)
			wInfo.iWarnFlag |= ATTR_WARN_FLAG_MASK_WARN;

		if(pMachineInfo != NULL 
			&& ((pMachineInfo->bWarnFlag & MACH_WARN_DENY_ALL) 
				|| ((pMachineInfo->bWarnFlag & MACH_WARN_DENY_EXCEPT) && (pNode->iWarnConfigFlag&ATTR_WARN_FLAG_EXCEPTION))
				|| ((pMachineInfo->bWarnFlag & MACH_WARN_DENY_BASIC) && IsBasicAttr(pNode->iAttrId))))
		{
			wInfo.iWarnFlag |= ATTR_WARN_FLAG_MASK_WARN;
			DEBUG_LOG("machine set warn mask, machine:%d, attr:%d, warn flag:%d|%d",
				pNode->iWarnId, pNode->iAttrId, pMachineInfo->bWarnFlag, pNode->iWarnConfigFlag);
		}

		wInfo.iWarnId = pNode->iWarnId;
		wInfo.iAttrId = pNode->iAttrId;
		wInfo.iWarnConfigValue = pNode->iWarnMin;
		wInfo.dwWarnValue = 0;
		wInfo.dwWarnTimeSec = slog.m_stNow.tv_sec;
		listWarn.push_back(wInfo);
		DEBUG_LOG("add min warn - attrid:%d, warnid:%d", pNode->iAttrId, pNode->iWarnId);

check_warn_next:
		if(bReverse)
			pNode = (TWarnConfig*)GetNextNodeRevers(phash);
		else
			pNode = (TWarnConfig*)GetNextNode(phash);
	}
}

void CheckAttrWarn(uint32_t now)
{
	static int s_iLastCheckMinIdx = 0;

	// 同一分钟只扫一次
	int iDayMinIdx = GetDayOfMin(now);
	if(s_iLastCheckMinIdx == iDayMinIdx)
		return;
	s_iLastCheckMinIdx = iDayMinIdx;

	list<TAttrWarnInfo> listWarn;
	map<int32_t, TWarnConfig *> mapViewWarnCfgs;

	SharedHashTable & hash = slog.GetWarnAttrHash();
	bool bReverse = false;
	_HashTableHead *pTableHead = (_HashTableHead*)hash.pHash;
	TWarnAttrReportInfo *pNode = (TWarnAttrReportInfo*)GetFirstNode(&hash);
	if(pNode == NULL && pTableHead->dwNodeUseCount > 0) {
		bReverse = true;
		pNode = (TWarnAttrReportInfo*)GetFirstNodeRevers(&hash);
		ERR_LOG("man by bug - has node:%u, bug get first null", pTableHead->dwNodeUseCount);
	}

	TWarnConfig *pWarnConfig = NULL;
	while(pNode != NULL)
	{
		// 单机告警 check
		pWarnConfig = slog.GetWarnConfigInfo(pNode->iMachineId, pNode->iAttrId, NULL);
		if(pWarnConfig != NULL) 
		{
			CheckMachineWarn(pWarnConfig, pNode, listWarn, now, iDayMinIdx);
		}

		// 异常属性告警 check
		if(pNode->bAttrDataType == EX_REPORT) 
		{
			pWarnConfig = slog.GetWarnConfigInfo(0, pNode->iAttrId, NULL);
			if(pWarnConfig == NULL) 
			{
				WARN_LOG("warn exception invalid - attr:%d, machine:%d", pNode->iAttrId, pNode->iMachineId);
			}
			else 
			{
				uint32_t dwLastVal=0, dwPreLastVal=0;
				AdaptReportValue(pNode, now, iDayMinIdx, dwLastVal, dwPreLastVal);
				if(dwLastVal > 0) {
					TAttrWarnInfo wInfo;
					// 异常属性告警,归类到单机告警,打上单机标记
					wInfo.iWarnFlag = ATTR_WARN_FLAG_TYPE_MACHINE|ATTR_WARN_FLAG_EXCEPTION;
					if(pWarnConfig->iWarnConfigFlag & ATTR_WARN_FLAG_MASK_WARN)
						wInfo.iWarnFlag |= ATTR_WARN_FLAG_MASK_WARN;

					wInfo.iWarnId = pNode->iMachineId;
					wInfo.iAttrId = pNode->iAttrId;
					wInfo.iWarnConfigValue = 0;
					wInfo.dwWarnValue = dwLastVal;
					wInfo.dwWarnTimeSec = now;
					listWarn.push_back(wInfo);
					INFO_LOG("get exception attr warn info - attr:%d, value:%d, machine:%d",
						wInfo.iAttrId, wInfo.dwWarnValue, wInfo.iWarnId);
				}
			}
		}

		// 视图告警, 先累计各机器上报的数据 -- 
		list<int32_t> listView;
		// 获取到同时绑定了属性:pNode->iAttrId, 机器:pNode->iMachineId 的视图
		GetReportViews(pNode, listView);
		list<int>::iterator it = listView.begin();
		for(; it != listView.end(); it++)
		{
			pWarnConfig = slog.GetWarnConfigInfo(*it, pNode->iAttrId, NULL);
			if(pWarnConfig != NULL)
			{
				// 存储告警配置列表,待扫描上上报后用于检查是否触发告警
				if(mapViewWarnCfgs.find(pWarnConfig->iWarnConfigId) == mapViewWarnCfgs.end())
				{
					mapViewWarnCfgs[ pWarnConfig->iWarnConfigId ] = pWarnConfig;
					AdaptViewReportValue(pWarnConfig, now, iDayMinIdx);
					DEBUG_LOG("add view warn info, attr:%d, view:%d, config id:%d", 
						pNode->iAttrId, *it, pWarnConfig->iWarnConfigId);
				}
				// 累计上报值到视图
				AddReportValForViewWarn(pWarnConfig, pNode, now, iDayMinIdx);
			}
		}
		listView.clear();

		if(bReverse)
			pNode = (TWarnAttrReportInfo*)GetNextNodeRevers(&hash);
		else
			pNode = (TWarnAttrReportInfo*)GetNextNode(&hash);
	}

	if(bReverse && pTableHead->dwNodeUseCount <=1 )  {
		ResetHashTable(&hash);
		hash.bAccessCheck = 1;
	}
	else if(!bReverse && pTableHead->dwNodeUseCount <= 0) {
		hash.bAccessCheck = 0;
	}

	// 扫描视图告警列表,看是否触发了视图告警
	map<int32_t, TWarnConfig *>::iterator it = mapViewWarnCfgs.begin();
	for(; it != mapViewWarnCfgs.end(); it++)
	{
		CheckViewWarn(it->second, listWarn, now);
	}

	// 扫描整个告警配置，处理没有上报值也可能触发告警的告警配置
	CheckWarnConfig(listWarn);

	if(listWarn.size() > 0)
	{
		MtReport_Attr_Add(284, listWarn.size());
		list<TAttrWarnInfo>::iterator it = listWarn.begin();
		char sSqlBuf[256] = {0};
		for(; it != listWarn.end(); it++)
		{
			bool bMaskWarn = false;
			if((*it).iWarnFlag & ATTR_WARN_FLAG_MASK_WARN)
			{
				// 屏蔽标记的配置可能改变, 影响合并记录逻辑,需要去掉该标记
				bMaskWarn = true;
				(*it).iWarnFlag &= ~ATTR_WARN_FLAG_MASK_WARN;
			}

			// 查看是否有相同且未处理完成的告警
			snprintf(sSqlBuf, sizeof(sSqlBuf), "select * from mt_warn_info where "
				" warn_id=%d and attr_id=%d and warn_flag=%d and deal_status!=%d "
				" order by warn_time_utc desc limit 1", 
				(*it).iWarnId, (*it).iAttrId, (*it).iWarnFlag, WARN_DEAL_STATUS_OK);

			// 24 小时内未处理完成的告警，自动合并为一条告警记录
			stConfig.qu->get_result(sSqlBuf);
			if(stConfig.qu->num_rows() > 0 && stConfig.qu->fetch_row() != NULL
				&& stConfig.qu->getuval("last_warn_time_utc")+24*3600 >= now)
			{
				uint32_t dwLastWarnTime = stConfig.qu->getuval("last_warn_time_utc");
				int32_t iWarnTimes = stConfig.qu->getval("warn_times");
				uint32_t dwWid = stConfig.qu->getuval("wid");
				uint32_t dwStartDealTime = stConfig.qu->getuval("start_deal_time_utc");
				int32_t iWarnStatus = stConfig.qu->getval("deal_status");

				// 不是处理完成状态的告警记录合并为一条数据库记录
				if(!bMaskWarn 
					&& (WARN_DEAL_STATUS_NONE == iWarnStatus || (WARN_DEAL_STATUS_DO == iWarnStatus 
						&& dwStartDealTime+stConfig.iNotSendWarnAfterDealTimeCfg < now)))
				{
					AddSendWarnToShm(*it, dwWid, now);
				}

				stConfig.qu->free_result();

				// 告警屏蔽标记改变了
				if(bMaskWarn != (WARN_DEAL_STATUS_MASK==iWarnStatus))
				{
					iWarnStatus = (bMaskWarn ? WARN_DEAL_STATUS_MASK : WARN_DEAL_STATUS_NONE);
					snprintf(sSqlBuf, sizeof(sSqlBuf), 
						"update mt_warn_info set warn_times=%d,last_warn_time_utc=%u,deal_status=%d,"
						"warn_val=%u where wid=%u", 
						iWarnTimes+1, (*it).dwWarnTimeSec, iWarnStatus, (*it).dwWarnValue, dwWid);
				}else {
					snprintf(sSqlBuf, sizeof(sSqlBuf), 
						"update mt_warn_info set warn_times=%d,last_warn_time_utc=%u,"
						"warn_val=%u where wid=%u", 
						iWarnTimes+1, (*it).dwWarnTimeSec, (*it).dwWarnValue, dwWid);
				}

				if(!stConfig.qu->execute(sSqlBuf))
				{
					ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, stConfig.qu->GetError().c_str());
					return;
				}
				DEBUG_LOG("update warn info - attr:%d, warn_id:%d, warn_val:%d|%d, flag:%d"
					", id:%d, wtimes:%d, lastwt:%u", 
					(*it).iAttrId, (*it).iWarnId, (*it).iWarnConfigValue, (*it).dwWarnValue, (*it).iWarnFlag, 
					dwWid, iWarnTimes, dwLastWarnTime);
				continue;
			}
			stConfig.qu->free_result();

			int iDealStatus = (bMaskWarn ? WARN_DEAL_STATUS_MASK : WARN_DEAL_STATUS_NONE);

			// 插入新告警记录
			snprintf(sSqlBuf, sizeof(sSqlBuf), "insert into mt_warn_info set warn_flag=%d,"
				"warn_id=%d,attr_id=%d,warn_config_val=%d,warn_val=%u,warn_time_utc=%u,"
				"warn_times=1,last_warn_time_utc=%u,deal_status=%d",
				(*it).iWarnFlag, (*it).iWarnId, (*it).iAttrId, (*it).iWarnConfigValue,(*it).dwWarnValue,
				(*it).dwWarnTimeSec, (*it).dwWarnTimeSec, iDealStatus);
			if(!stConfig.qu->execute(sSqlBuf))
			{
				ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, stConfig.qu->GetError().c_str());
				return ;
			}
			DEBUG_LOG("insert warn info - attr:%d, warn_id:%d, warn_val:%d|%d, flag:%d, id:%d",
				(*it).iAttrId, (*it).iWarnId, (*it).iWarnConfigValue,(*it).dwWarnValue, (*it).iWarnFlag,
				(uint32_t)(stConfig.qu->insert_id()));

			// 已屏蔽的监控不发送告警通知
			if(!bMaskWarn)
				AddSendWarnToShm(*it, (uint32_t)(stConfig.qu->insert_id()), now);
		}
	}
}

int main(int argc, char *argv[])
{
	int iRet = 0;
	const char *pconfig = "./slog_check_warn.conf";
	if((iRet=Init(pconfig)) < 0)
	{
		ERR_LOG("Init Failed ret:%d !", iRet);
		return SLOG_ERROR_LINE;
	}

	slog.Daemon(1, 1, 1);
	INFO_LOG("slog_check_warn start !");

	uint32_t dwLastCheckTime = 0;
	while(slog.Run())
	{
		stConfig.dwCurrentTime = slog.m_stNow.tv_sec;
		if(DealDbConnect() < 0)
			break;

		if(stConfig.dwCurrentTime < stConfig.iPerCheck+dwLastCheckTime)
		{
			usleep(1000);
			continue;
		}
		dwLastCheckTime = stConfig.dwCurrentTime;
		CheckAttrWarn(dwLastCheckTime);
	}
	INFO_LOG("slog_check_warn exist !");
	return 0;
}


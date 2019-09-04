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
#define __STDC_FORMAT_MACROS
#endif
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/time.h>
#include <unistd.h>

#include "mtreport_client.h"
#include "sv_timer.h"
#include "sv_list.h"
#include "sv_str.h"
#include "mt_shm.h"
#include "mt_vmem.h"

MtReportTimer g_mtTimer;
void SetCurTime();

int InitTimer(int iNodeSize, int iNodeCount, FunCompare cmp, FunWarning warn)
{
	if(g_mtTimer.cIsInit)
		return 1;

	//Init tvs
	int i;
	for(i = 0; i < TVN_SIZE; i++) {
		INIT_LIST_HEAD(g_mtTimer.tv5.vec + i);
		INIT_LIST_HEAD(g_mtTimer.tv4.vec + i);
		INIT_LIST_HEAD(g_mtTimer.tv3.vec + i);
		INIT_LIST_HEAD(g_mtTimer.tv2.vec + i);
	}
	g_mtTimer.tv5.index = 0;
	g_mtTimer.tv4.index = 0;
	g_mtTimer.tv3.index = 0;
	g_mtTimer.tv2.index = 0;
	for(i = 0; i < TVR_SIZE; i++) {
		INIT_LIST_HEAD(g_mtTimer.tv1.vec + i);
	}
	g_mtTimer.tv1.index = 0;

	g_mtTimer.tvecs[0] = (TimerVec*)&g_mtTimer.tv1;
	g_mtTimer.tvecs[1] = &g_mtTimer.tv2;
	g_mtTimer.tvecs[2] = &g_mtTimer.tv3;
	g_mtTimer.tvecs[3] = &g_mtTimer.tv4;
	g_mtTimer.tvecs[4] = &g_mtTimer.tv5;

	//Init time
	g_mtTimer.uiCheckInvlUs = MT_TIMER_CHECK_INTVALUS;
	gettimeofday(&g_mtTimer.stBaseTV, NULL);
	g_mtTimer.qwBaseTime = (uint64_t)g_mtTimer.stBaseTV.tv_sec*1000000 +g_mtTimer.stBaseTV.tv_usec;
	g_mtTimer.uiCheckTime = 0;
	g_mtTimer.uiCount = 0;
	g_mtTimer.uiMaxCount = 0;

	if(InitHashTable_NoList_Heap(&g_mtTimer.stHash, iNodeSize, iNodeCount, cmp, warn) != 0)
		return -101;

	g_mtTimer.cIsInit = 1;
	DEBUG_LOG("InitTimer node size:%d count:%d pid:%u", iNodeSize, iNodeCount, stConfig.dwProcessId);
	return 0;
}

static void InternalAddTimer(TimerNode * pstNode)
{
	unsigned expires = pstNode->uiExpire;
	unsigned idx = expires - g_mtTimer.uiCheckTime;
	int i=0;
	ListHead * vec;
	if (idx < TVR_SIZE) {
		i = (int)(expires & TVR_MASK);
		vec = g_mtTimer.tv1.vec + i;
	} else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
		i = (int)((expires >> TVR_BITS) & TVN_MASK);
		vec = g_mtTimer.tv2.vec + i;
	} else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
		i = (int)((expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK);
		vec = g_mtTimer.tv3.vec + i;
	} else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
		i = (int)((expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK);
		vec = g_mtTimer.tv4.vec + i;
	} else if ((signed) idx < 0) {
		vec = g_mtTimer.tv1.vec + g_mtTimer.tv1.index;
	} else {
		i = (int)((expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK);
		vec = g_mtTimer.tv5.vec + i;
	}
	pstNode->dwProcessId = stConfig.dwProcessId;
	LIST_ADD(&(pstNode->list), vec->pstPrev);
}

int UpdateTimer(TimerNode * pstNodeShm, unsigned uiDataLen, char * sData)
{
	if(!g_mtTimer.cIsInit)
		return -1;

	if(uiDataLen > 0) {
		pstNodeShm->pDataPtr = (char*)malloc(uiDataLen);
		if(NULL == pstNodeShm->pDataPtr)
			return -1011;
		memcpy(pstNodeShm->pDataPtr, sData, uiDataLen);
	}
	else
		pstNodeShm->pDataPtr = NULL;

	if(pstNodeShm->uiTimeOut < 1000)
		pstNodeShm->uiTimeOut = 1000;

	SetCurTime();
	pstNodeShm->qwExpireTime = TIME_MS_TO_US(pstNodeShm->uiTimeOut)+stConfig.qwCurTime;
	pstNodeShm->uiExpire = (unsigned)((stConfig.qwCurTime-g_mtTimer.qwBaseTime
				+ TIME_MS_TO_US(pstNodeShm->uiTimeOut)) / g_mtTimer.uiCheckInvlUs);
	pstNodeShm->dwNodeSetTime = stConfig.dwCurTime;

	InternalAddTimer(pstNodeShm);
	g_mtTimer.uiCount++;
	if(g_mtTimer.uiCount > g_mtTimer.uiMaxCount)
		g_mtTimer.uiMaxCount = g_mtTimer.uiCount;
	DEBUG_LOG("update timer - timeout:%u(ms) add data to vmem - idx:%d key:%u timer node count:%d",
		pstNodeShm->uiTimeOut, pstNodeShm->iDataIndex, pstNodeShm->uiKey, g_mtTimer.uiCount);
	return 0;
}

int AddTimer(unsigned uiKey, unsigned uiTimeOut, ExpireFunc OnExpire, 
	void *pSessShmData, int iSessDataLen, unsigned uiDataLen, char * sData)
{
	if(!g_mtTimer.cIsInit)
		return -1;

	if(iSessDataLen > TIMER_NODE_SESS_DATA_LEN)
		return -5;

	TimerNode * pstNode = NULL;
	TimerNode stNode;
	TimerNode stEmptyNode;

	stEmptyNode.uiKey = 0;
	stNode.uiKey = uiKey;
	unsigned int dwIsFind = 0;
	pstNode = (TimerNode*)HashTableSearchEx_NoList(
		&g_mtTimer.stHash, &stNode, &stEmptyNode, uiKey,  &dwIsFind);
	if(dwIsFind)
		return -4;

	if(pstNode == NULL)
		return -2;

	if(uiDataLen > 0) {
		pstNode->pDataPtr = (char*)malloc(uiDataLen);
		if(NULL == pstNode->pDataPtr)
			return -1011;
		memcpy(pstNode->pDataPtr, sData, uiDataLen);
	}
	else
		pstNode->pDataPtr = NULL;

	if(uiTimeOut < 1000)
		uiTimeOut = 1000;
	SetCurTime();
	pstNode->uiExpire = (unsigned)((stConfig.qwCurTime-g_mtTimer.qwBaseTime
				+ TIME_MS_TO_US(uiTimeOut)) / g_mtTimer.uiCheckInvlUs);
	pstNode->qwExpireTime = TIME_MS_TO_US(uiTimeOut)+stConfig.qwCurTime;
	pstNode->dwNodeSetTime = stConfig.dwCurTime;

	pstNode->OnExpire = OnExpire;
	pstNode->uiDataLen = uiDataLen;
	pstNode->uiTimeOut = uiTimeOut;
	pstNode->uiKey = stNode.uiKey;
	memcpy(pstNode->sSessData, pSessShmData, iSessDataLen);
	pstNode->iSessDataLen = iSessDataLen;
	InternalAddTimer(pstNode);
	g_mtTimer.uiCount++;
	if(g_mtTimer.uiCount > g_mtTimer.uiMaxCount)
		g_mtTimer.uiMaxCount = g_mtTimer.uiCount;
	DEBUG_LOG("add timer - data to vmem idx:%d|%u key:%u timer:%u ms, timer node count:%d",
		pstNode->iDataIndex, uiDataLen, uiKey, uiTimeOut, g_mtTimer.uiCount);
	return 0;
}

// 注意这里不能将 pNode 中的 uiKey 设置为0，因为可能要使用到 sSessData
int GetTimer(unsigned uiKey, TimerNode **pNode, unsigned *puiDataLen, char *psData)
{
	TimerNode * pstNode = NULL;
	TimerNode stNode;
	*pNode = NULL;
	if(!g_mtTimer.cIsInit)
		return -1;

	stNode.uiKey = uiKey;
	if((pstNode=(TimerNode*)HashTableSearch_NoList(&g_mtTimer.stHash, &stNode, stNode.uiKey)) == NULL)
		return -2;
	LIST_DEL(&(pstNode->list)); // 断开定时器 hash 链表
	g_mtTimer.uiCount--;

	if(pstNode->pDataPtr != NULL) {
		memcpy(psData, pstNode->pDataPtr,  pstNode->uiDataLen);
		free(pstNode->pDataPtr);
		pstNode->pDataPtr = NULL;
		*puiDataLen = pstNode->uiDataLen;
	}
	else
		*puiDataLen = 0;

	*pNode = pstNode;
	if(*puiDataLen != pstNode->uiDataLen)
		return -4;
	return 0;
}

int GetTimerData(unsigned uiKey, unsigned *puiDataLen, char *psData)
{
	TimerNode * pstNode = NULL;
	TimerNode stNode;

	if(puiDataLen == NULL || psData == NULL || *puiDataLen <= 0)
		return -1;

	stNode.uiKey = uiKey;
	if((pstNode=(TimerNode*)HashTableSearch_NoList(&g_mtTimer.stHash, &stNode, stNode.uiKey)) == NULL)
		return -2;

	if(pstNode->pDataPtr != NULL) {
		memcpy(psData, pstNode->pDataPtr,  pstNode->uiDataLen);
		free(pstNode->pDataPtr);
		pstNode->pDataPtr = NULL;
		*puiDataLen = pstNode->uiDataLen;
	}
	else
		*puiDataLen = 0;
	return 0;
}

/**
 * CascadeTimers and RunTimerList is same as timer.c in linux kernel 2.4.xx
 */
static inline int CascadeTimers(TimerVec * tv)
{
	ListHead *pstHead;
	ListHead *pstCurr;
	ListHead *pstNext;
	pstHead = tv->vec + tv->index;
	pstCurr = pstHead->pstNext;
	int iLoopCount = 0;
	for(; pstCurr != pstHead; iLoopCount++) 
	{
		TimerNode * pstCurNode = LIST_ENTRY(pstCurr, TimerNode, list);
		pstNext = pstCurr->pstNext;
		InternalAddTimer(pstCurNode);
		pstCurr = pstNext;
		if(iLoopCount >= 10000) {
			ERROR_LOG("timer run times over limit may have bug");
			stConfig.pReportShm->cIsAgentRun = 2;
			return -1;
		}
	}
	INIT_LIST_HEAD(pstHead);
	tv->index = (tv->index + 1) & TVN_MASK;
	return 0;
}

static void RunTimerList(unsigned uiCurTime)
{
	static char sDataBuf[TIMER_DATA_MAX_LENGTH] = {0};

	int iRet = 0;
	int iCheckNodeCount = 0;
	while (uiCurTime >= g_mtTimer.uiCheckTime && g_mtTimer.uiCount > 0)
	{
		ListHead *pstHead;
		ListHead *pstCurr;
		if (!g_mtTimer.tv1.index) {
			int n = 1;
			do {
				if(CascadeTimers(g_mtTimer.tvecs[n]) < 0)
					return ;
			} while (g_mtTimer.tvecs[n]->index == 1 && ++n < 5);
		}

repeat:
		pstHead = g_mtTimer.tv1.vec + g_mtTimer.tv1.index;
		pstCurr = pstHead->pstNext;
		if (pstCurr != pstHead) {
			TimerNode * pstCurNode = LIST_ENTRY(pstCurr, TimerNode, list);
			LIST_DEL(pstCurr); // 断开定时器 hash 链表
			g_mtTimer.uiCount--;
			iCheckNodeCount++;

			if(pstCurNode->qwExpireTime > stConfig.qwCurTime
				&& pstCurNode->qwExpireTime-stConfig.qwCurTime > g_mtTimer.uiCheckInvlUs)
			{
				FATAL_LOG("timer check failed - base:%lu, (%lu-%lu)(%lu > %u) "
					" (uiExpire:%u,uiTimeOut:%u)-(check:%u:%u)", 
					g_mtTimer.qwBaseTime, pstCurNode->qwExpireTime, stConfig.qwCurTime,
					pstCurNode->qwExpireTime-stConfig.qwCurTime, g_mtTimer.uiCheckInvlUs, 
					pstCurNode->uiExpire, pstCurNode->uiTimeOut, uiCurTime, g_mtTimer.uiCheckTime);
				stConfig.pReportShm->cIsAgentRun = 2;
				break;
			}

			unsigned uiDataLen = sizeof(sDataBuf);
			if((iRet=GetTimerData(pstCurNode->uiKey, &uiDataLen, sDataBuf)) < 0) {
				ERROR_LOG("GetTimerData failed, key:%u ret:%d", pstCurNode->uiKey, iRet);
				stConfig.pReportShm->cIsAgentRun = 2;
				break;
			}
			else {
				ExpireFunc OnExpire = pstCurNode->OnExpire;
				iRet = 0;
				if(OnExpire) {
					// OnExpire 返回码为 1，pstCurNode 又被使用了，并加入了定时器 hash, key 值保持不变
					iRet = OnExpire(pstCurNode, uiDataLen, sDataBuf);
				}
				if(iRet != 1)
					memset(pstCurNode, 0, sizeof(TimerNode));

				if(iCheckNodeCount >= MAX_TIMER_NODE_PER_TRY)
					break;
				goto repeat;
			}
		}
		g_mtTimer.uiCheckTime++;
		g_mtTimer.tv1.index = (g_mtTimer.tv1.index + 1) & TVR_MASK;
	}
}

int CheckTimer(void)
{
	unsigned uiCurTime = 0;
	uiCurTime = (unsigned)((stConfig.qwCurTime-g_mtTimer.qwBaseTime)/g_mtTimer.uiCheckInvlUs);
	if(uiCurTime >= g_mtTimer.uiCheckTime)
		RunTimerList(uiCurTime);
	return g_mtTimer.uiCheckInvlUs;
}


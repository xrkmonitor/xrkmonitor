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

#ifndef __SV_TIMER_H__
#define __SV_TIMER_H__

#include "sv_list.h"
#include "mt_shared_hash.h"

#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)
#define MT_TIMER_CHECK_INTVALUS 100000 // 10 ms
#define TIME_MS_TO_US(ms) ms*1000
#define TIMER_DATA_MAX_LENGTH 8192
#define TIMER_NODE_SESS_DATA_LEN 48 
#define MAX_TIMER_NODE_PER_TRY 10

/*
 * the expire callback function type
 * 
 */
struct TimerNode;
typedef int (*ExpireFunc)(TimerNode *pNodeShm, unsigned uiDataLen, char * sData);

typedef struct TimerNode
{
	ListHead list;
	unsigned uiKey;
	int iDataIndex;
	char *pDataPtr;
	unsigned uiExpire;
	unsigned uiTimeOut;
	uint64_t qwExpireTime;
	unsigned uiDataLen;
	ExpireFunc OnExpire;
	int iSessDataLen;
	uint32_t dwProcessId; 
	uint32_t dwNodeSetTime; 
	char sSessData[TIMER_NODE_SESS_DATA_LEN];
}TimerNode;

typedef struct
{
	int index;
	ListHead vec[TVR_SIZE];
}TimerVecRoot;

typedef struct
{
	int index;
	ListHead vec[TVN_SIZE];
}TimerVec;

typedef struct {
	char cIsInit;
	TimerVec tv5;
	TimerVec tv4;
	TimerVec tv3;
	TimerVec tv2;
	TimerVecRoot tv1;
	TimerVec *tvecs[5]; 
	struct timeval stBaseTV; 
	uint64_t qwBaseTime;
	unsigned uiCheckTime; 
	unsigned uiCheckInvlUs; 
	unsigned uiCount; // 当前定时节点数
	unsigned uiMaxCount; // 历史最大定时节点数
	SharedHashTableNoList stHash;
}MtReportTimer;

extern MtReportTimer g_mtTimer;

int InitTimer(int iNodeSize, int iNodeCount, FunCompare cmp, FunWarning warn);
int AddTimer(unsigned uiKey, unsigned uiTimeOut, ExpireFunc OnExpire, 
	void *pSessShmData, int iSessDataLen, unsigned uiDataLen, char * sData);
int UpdateTimer(TimerNode * pstNodeShm, unsigned uiDataLen, char * sData);
int GetTimer(unsigned uiKey, TimerNode **pNode, unsigned * puiDataLen, char * psData);
int CheckTimer(void);

#endif


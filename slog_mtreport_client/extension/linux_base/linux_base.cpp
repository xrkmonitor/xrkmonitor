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

#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <mt_report.h>
#include <mt_log.h>
#include <sv_cfg.h>
#include "net.h"
#include "cpu.h"
#include "disk.h"
#include "mem.h"

// 基础监控相关监控点 id
const int g_cpuAttr[] = {163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179 };
const int g_iMemUseAttr = 183; 
const int g_iDiskUseTotalAttr = 184; 
const int g_iDiskUseMaxAttr = 189; 
const int g_iDiskUseOverPer95 = 188; 
const int g_iDiskUseOverPer90 = 187; 
const int g_iDiskUseOverPer80 = 185; 
const int g_iDiskUseOverPer70 = 186;

const char *s_plusName = "linux_base";

const int PLUS_LINUX_BASE_CONFIG_ID = 184;

// cpu 使用率采集时间间隔
const int READ_CPU_PER_TIME_SEC = 4;
const int READ_MEM_PER_TIME_SEC = 6;

// 基础监控数据写入共享内存时间间隔
const int WRITE_BASE_PER_TIME_SEC = 8;

TcpuUse g_stCpuUse = {0};
int g_iMemMaxUsePer = 0;
TMemInfo g_stMemInfo = {0};

static void ReadMemUse(uint32_t dwTimeNow)
{
	static uint32_t s_dwLastReadMemTime;
	if(dwTimeNow < s_dwLastReadMemTime+READ_MEM_PER_TIME_SEC)
		return;
	s_dwLastReadMemTime = dwTimeNow;

    memset(&g_stMemInfo, 0, sizeof(g_stMemInfo));
    if(GetMemInfo(g_stMemInfo) < 0)
    {
        MtReport_Log_Warn("GetMemInfo failed !");
        return;
    }

    uint32_t dwFree = g_stMemInfo.dwMemFree + g_stMemInfo.dwCached
        - g_stMemInfo.dwDirty - g_stMemInfo.dwMapped;

    MtReport_Log_Debug("get meminfo - unit:%s total:%u free:%u buffers:%u cached:%u dirty:%u maped:%u",
        g_stMemInfo.szUnit, g_stMemInfo.dwMemTotal,
        g_stMemInfo.dwMemFree, g_stMemInfo.dwBuffers,
        g_stMemInfo.dwCached, g_stMemInfo.dwDirty, g_stMemInfo.dwMapped);

    int32_t iUsePer = (int)((g_stMemInfo.dwMemTotal-dwFree)*100/g_stMemInfo.dwMemTotal);
    if(iUsePer > g_iMemMaxUsePer)
        g_iMemMaxUsePer = iUsePer;
    MtReport_Log_Info("get mem use info -- real free:%u total:%u useper:%u%%",
        dwFree, g_stMemInfo.dwMemTotal, iUsePer);
}

static void WriteCpuUse()
{
    int iCount = sizeof(g_cpuAttr)/sizeof(int);
    if(iCount > g_stCpuUse.iCpuCount)
        iCount = g_stCpuUse.iCpuCount;
    int iValue = 0;
    MtReport_Log_Debug("write use cpu count:%d", iCount);
    for(int i=0; i < iCount; i++)
    {
        iValue = g_stCpuUse.iCpuUse[i]/10;
        if(g_stCpuUse.iCpuUse[i]%10 > 0)
            iValue++;
        MtReport_Log_Debug("get cpu%d use, attr:%d, use:%u%%(%d)",
            i, g_cpuAttr[i], iValue, g_stCpuUse.iCpuUse[i]);
        g_stCpuUse.iCpuUse[i] = 0;
        MtReport_Attr_Set(g_cpuAttr[i], iValue);
    }
    g_stCpuUse.iCpuCount = 0;
}

static void ReadCpuUse(uint32_t dwTimeNow)
{
    static TcpuUse stCpuUse;
	static uint32_t s_dwLastReadCpuTime;

	if(dwTimeNow < s_dwLastReadCpuTime+READ_CPU_PER_TIME_SEC)
		return;
	s_dwLastReadCpuTime = dwTimeNow;

    if(GetCpuUse(&stCpuUse) <= 0)
    {
        MtReport_Log_Warn("GetCpuUse failed !");
        return;      
    }            

    int iCount = sizeof(g_cpuAttr)/sizeof(int);
    if(iCount > stCpuUse.iCpuCount)
        iCount = stCpuUse.iCpuCount;
    MtReport_Log_Debug("use cpu count:%d", iCount);

    // 取使用率高的值
    g_stCpuUse.iCpuCount = stCpuUse.iCpuCount;
    for(int i=0; i < iCount; i++)
    {
        if(stCpuUse.iCpuUse[i] > g_stCpuUse.iCpuUse[i])
            g_stCpuUse.iCpuUse[i]=stCpuUse.iCpuUse[i];
        MtReport_Log_Debug("get cpu%d use:%d, now:%d", i, stCpuUse.iCpuUse[i], g_stCpuUse.iCpuUse[i]);
    }
}

static void WriteMemUse()
{   
	if(g_iMemMaxUsePer > 0)
	{
		MtReport_Log_Debug("write mem use:%d", g_iMemMaxUsePer);
		MtReport_Attr_Set(g_iMemUseAttr, g_iMemMaxUsePer);
		g_iMemMaxUsePer = 0;
	}
}

static void WriteDiskUse()
{   
    uint64_t qwTotalSpace = 0, qwTotalUse = 0;
    uint32_t maxUsePer = 0;
    if(GetDiskInfo(qwTotalSpace, qwTotalUse, maxUsePer) < 0)
    {
        MtReport_Log_Warn("GetDiskInfo failed");
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
    MtReport_Log_Info("get disk use total percent:%u%%, max use percent:%u%%", dwUsePerTotal, maxUsePer);
}

static void WriteBaseInfo(uint32_t dwTimeNow)
{
	static uint32_t s_dwLastWriteTime;
	if(dwTimeNow < s_dwLastWriteTime+WRITE_BASE_PER_TIME_SEC)
		return ;
	s_dwLastWriteTime = dwTimeNow;

	WriteCpuUse();
	WriteMemUse();
	WriteDiskUse();
	ReportNetInfo();	
}

extern "C" {

// 插件初始化函数
int SlogPlusInit()
{
	char szPlusName[256] = {0};
	char szLocalLogFile[256] = {0};
	int32_t iCfgId = 0, iRet = 0, iLocalLogType = 0;

	if((iRet=LoadConfig("./linux_base.conf",
		"LINUX_BASE_PLUS_NAME", CFG_STRING, szPlusName, s_plusName, sizeof(szPlusName),
		"LINUX_BASE_CONFIG_ID", CFG_INT, &iCfgId, PLUS_LINUX_BASE_CONFIG_ID,
		"LOCAL_LOG_TYPE", CFG_INT, &iLocalLogType, 0,
		"LOCAL_LOG_FILE", CFG_STRING, szLocalLogFile, "./linux_base.log", sizeof(szLocalLogFile),
		(void*)NULL)) < 0)
	{
		return ERROR_LINE;
	}

	if(MtReport_Plus_Init(szPlusName, iCfgId, szLocalLogFile, iLocalLogType) < 0)
		return ERROR_LINE;

	// cpu 监控 -- start 
	if(InitGetCpuUse() < 0)
	{
		MtReport_Log_Error("InitGetCpuUse failed !");
		return ERROR_LINE;
	}
	memset(&(g_stCpuUse), 0, sizeof(g_stCpuUse));
	InitGetNet();
	MtReport_Log_Info("plus:%s init ok, config id:%u, local log:%s:%d",
		szPlusName, iCfgId, szLocalLogFile, iLocalLogType);
	return 0;
}

// 插件逻辑实现函数，请勿在该函数中睡眠/阻塞
void SlogPlusOnLoop(uint32_t dwTimeNow)
{
	ReadCpuUse(dwTimeNow);
	ReadMemUse(dwTimeNow);
	WriteBaseInfo(dwTimeNow);
}

// agent 退出，插件资源清理函数
void SlogPlusEnd()
{

}

}


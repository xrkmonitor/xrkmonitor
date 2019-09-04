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

#define __STDC_FORMAT_MACROS
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/shm.h>
#include "cpu.h"
#include "mtreport_client.h"

void WriteCpuUse(TcpuUse &stCpuUse, const int *pcpuAttr)
{
    int iValue = 0;
    for(int i=0; i < stCpuUse.iCpuCount; i++)
    {
        iValue = stCpuUse.iCpuUse[i]/10;
        if(stCpuUse.iCpuUse[i]%10 > 0)
            iValue++;
        DEBUG_LOG("get cpu%d use, attr:%d, use:%%%d(%d)",
            i, pcpuAttr[i], iValue, stCpuUse.iCpuUse[i]);
        stCpuUse.iCpuUse[i] = 0; 
        MtReport_Attr_Set(pcpuAttr[i], iValue);
    }    
    stCpuUse.iCpuCount = 0; 
}

void ReadCpuUse(TcpuUse &cpuUse, int iCpuCountMax)
{
    static TcpuUse stCpuUse;

    if(GetCpuUse(&stCpuUse) <= 0)
    {    
        WARN_LOG("GetCpuUse failed !");
        return;
    }    

    if(iCpuCountMax > stCpuUse.iCpuCount)
        iCpuCountMax = stCpuUse.iCpuCount;
	else if(iCpuCountMax < stCpuUse.iCpuCount)
		WARN_LOG("cpu use not support all, support:%d, now:%d", iCpuCountMax, stCpuUse.iCpuCount);
    DEBUG_LOG("use cpu count:%d", iCpuCountMax);

    // 取使用率高的值
    cpuUse.iCpuCount = 0;
    for(int i=0; i < iCpuCountMax; i++) 
    {    
        if(stCpuUse.iCpuUse[i] > cpuUse.iCpuUse[i])
            cpuUse.iCpuUse[i]=stCpuUse.iCpuUse[i];
		cpuUse.iCpuCount++;
        DEBUG_LOG("get cpu%d use:%d, now:%d", i, stCpuUse.iCpuUse[i], cpuUse.iCpuUse[i]);
        if(iCpuCountMax <= 2)
            break;
    }    
}

int GetCpuStatis(TCpuStatic *pcp)
{
	TCpuInfo *plast = (TCpuInfo*)pcp->sInfo;
	FILE *fp = popen("/bin/cat /proc/stat |grep cpu|awk \'{print " CPU_AWK_FMT "}\'", "r");
	if(fp == NULL) {
		return -2;
	}

	int i=0;
	while( fscanf(fp, "%" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 ,
		&plast[i].qwUser, &plast[i].qwNice, &plast[i].qwSys, &plast[i].qwIdle,
		&plast[i].qwIowait, &plast[i].qwIrq, &plast[i].qwSoftIrq) == 7) {
		plast[i].qwTotal = plast[i].qwUser + plast[i].qwNice + plast[i].qwSys
			+ plast[i].qwIdle + plast[i].qwIowait + plast[i].qwIrq + plast[i].qwSoftIrq;
		i++;
		if(i >= MAX_CPU_SUPPORT)
			break;
	}
	pclose(fp);
	pcp->iCpuCount = i;
	return i;
}

static TCpuStatic s_cpuLastInfo;
int InitGetCpuUse()
{
	if(s_cpuLastInfo.iCpuCount > 0)
		return 0;
	if(GetCpuStatis(&s_cpuLastInfo) < 0)
		return -1;
	if(s_cpuLastInfo.iCpuCount <= 0)
		return -2;
	return s_cpuLastInfo.iCpuCount;
}

int GetCpuUse(TcpuUse *pCpuUse)
{
	TCpuStatic now;
	if(GetCpuStatis(&now) < 0)
		return -1;
	if(now.iCpuCount != s_cpuLastInfo.iCpuCount)
		return -2;
	TCpuStatic *ppre = &s_cpuLastInfo;
	TCpuStatic *pnowcp = &now;

	pCpuUse->iCpuCount = pnowcp->iCpuCount;
	if((ppre->sInfo[0]).qwTotal >= (pnowcp->sInfo[0]).qwTotal)
	{
		for(int i=0; i < pnowcp->iCpuCount; i++) {
			pCpuUse->iCpuUse[i] = 0;
		}
		memcpy(ppre, pnowcp, sizeof(TCpuStatic));
		return 0;
	}
	else if((ppre->sInfo[0]).qwIdle >= (pnowcp->sInfo[0]).qwIdle)
	{
		for(int i=0; i < pnowcp->iCpuCount; i++) {
			pCpuUse->iCpuUse[i] = 1000;
		}
		memcpy(ppre, pnowcp, sizeof(TCpuStatic));
		return 0;
	}

	for(int i=0; i < pnowcp->iCpuCount; i++) {
		pCpuUse->iCpuUse[i] = CPU_USE(ppre->sInfo[i], pnowcp->sInfo[i]);
		//dwCpuUser = CPU_USER(ppre->sInfo[i], pnowcp->sInfo[i]);
		//dwCpuSoftIrq = CPU_SOFTIRQ(ppre->sInfo[i], pnowcp->sInfo[i]);
		//dwCpuSys = CPU_SOFTIRQ(ppre->sInfo[i], pnowcp->sInfo[i]);
		//dwCpuIo = CPU_SOFTIRQ(ppre->sInfo[i], pnowcp->sInfo[i]);
	}
	memcpy(ppre, pnowcp, sizeof(TCpuStatic));
	return pCpuUse->iCpuCount;
}


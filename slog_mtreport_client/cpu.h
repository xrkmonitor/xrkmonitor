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

#ifndef __MTREPORT_CPU_H__
#define __MTREPORT_CPU_H__ 1

typedef struct {
	uint64_t qwUser;
	uint64_t qwNice;
	uint64_t qwSys;
	uint64_t qwIdle;
	uint64_t qwIowait;
	uint64_t qwIrq;
	uint64_t qwSoftIrq;
	uint64_t qwTotal;
}TCpuInfo;

#define MAX_CPU_SUPPORT 16

#define CPU_AWK_FMT "$2\" \"$3\" \"$4\" \"$5\" \"$6\" \"$7\" \"$8"
#define CPU_USE(c1, c2) (1000-((c2).qwIdle-(c1).qwIdle)*1000/((c2).qwTotal-(c1).qwTotal))
#define CPU_USER(c1, c2) (((c2).qwUser-(c1).qwUser)*1000/((c2).qwTotal-(c1).qwTotal))
#define CPU_SYS(c1, c2) (((c2).qwSys-(c1).qwSys)*1000/((c2).qwTotal-(c1).qwTotal))
#define CPU_IO(c1, c2) (((c2).qwIowait-(c1).qwIowait)*1000/((c2).qwTotal-(c1).qwTotal))
#define CPU_SOFTIRQ(c1, c2) (((c2).qwSoftIrq-(c1).qwSoftIrq)*1000/((c2).qwTotal-(c1).qwTotal))

typedef struct {
	int32_t iCpuCount;
	TCpuInfo sInfo[MAX_CPU_SUPPORT];
}TCpuStatic;

typedef struct {
	int32_t iCpuCount;
	int16_t iCpuUse[MAX_CPU_SUPPORT]; // 使用率在 0-1000 之间，千分制
}TcpuUse;

int InitGetCpuUse();

// 从 /proc/stat 中获取cpu 采样数据
int GetCpuStatis(TCpuStatic *pcp);

// 通过两次采样数据，计算cpu 使用率
int GetCpuUse(TcpuUse *pCpuUse);

void ReadCpuUse(TcpuUse &cpuUse, int iCpuCountMax);
void WriteCpuUse(TcpuUse &cpuUse, const int *pcpuAttr);

#endif


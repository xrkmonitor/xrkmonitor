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

   开发库  mtreport_api 说明:
         用户使用监控系统的c/c++ 开发库，本库使用 标准 c 开发无任何第
		 三方库依赖，用户可以在 c或者 c++ 项目中使用

****/

#ifndef _SV_VMEM_20141117_H_
#define _SV_VMEM_20141117_H_ (1) 

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
   *  以下接口支持多进程，多线程
   */

// 初始化 - 成功返回 大于等于0， 失败返回小于 0
int32_t MtReport_InitVmem();

// 存数据 - 成功返回 大于等于0， 失败返回小于 0
int32_t MtReport_SaveToVmem(const char *pdata, int32_t iDataLen);

// 取数据 - 成功返回 大于等于0， 失败返回小于 0
int32_t MtReport_GetFromVmem(int32_t index, char *pbuf, int32_t *piBufLen);

// 释放索引指向的节点 - 成功返回 大于等于0， 失败返回小于 0
int32_t MtReport_FreeVmem(int32_t index);

// 取数据并释放索引指向的节点 - 成功返回 大于等于0， 失败返回小于 0
int32_t MtReport_GetAndFreeVmem(int32_t index, char *pbuf, int32_t *piBufLen);

const char * MtReport_GetFromVmem_Local(int32_t index);

int32_t MtReport_CheckVmem();

void MtReport_show_shm();

// vmem 类型
#define VMEM_DEF_SHMKEY 2013046000
#define VMEM_SHM_CHECK_STR_16 "3dg2352332tsl*]YYYY"
#define VMEM_SHM_CHECK_STR_32 "3dgtlog_dk##Asyyyssl*]"
#define VMEM_SHM_CHECK_STR_64 "3@tlog_dk##ADlslgdjk*]"
#define VMEM_SHM_CHECK_STR_128 "@#)))))))%@#_mtlog]"
#define VMEM_SHM_CHECK_STR_255 "sdigdj#_mtlog_dk##AD___"

#define VMEM_SHM_COUNT 5 // 最大 120 

// 可变缓存, 适用于存储长度变化较大的数据缓存
// 用于存储长度在 1-VMEM_MAX_DATA_LEN 字节的数据
#define VMEM_MAX_DATA_LEN 65000 // 65KB 

// count 不能超过 65000
#define VMEM_16_NODE_COUNT 20000
#define VMEM_32_NODE_COUNT 20000
#define VMEM_64_NODE_COUNT 60000 // 这个要多点，日志的 cust 大部分应该会落这里
#define VMEM_128_NODE_COUNT 20000
#define VMEM_255_NODE_COUNT 15000

#pragma pack(1)

// vmem 第一个节点
typedef struct
{
	uint16_t wNodeCount;
	uint16_t wNodeUsed; // init must 1, first node always used
	uint16_t wFreeNodeIndex;
	volatile uint8_t bUseFlag;
	uint16_t wScanFreeIndex;
}VmemBufNodeFirst;

typedef struct
{
	uint16_t wNextNodeIndex;
	uint8_t bDataLen;
	uint8_t sDataBuf[16];
}VmemBufNode16;

typedef struct
{
	uint16_t wNextNodeIndex;
	uint8_t bDataLen;
	uint8_t sDataBuf[32];
}VmemBufNode32;

typedef struct
{
	uint16_t wNextNodeIndex;
	uint8_t bDataLen;
	uint8_t sDataBuf[64];
}VmemBufNode64;

typedef struct
{
	uint16_t wNextNodeIndex;
	uint8_t bDataLen;
	uint8_t sDataBuf[128];
}VmemBufNode128;

typedef struct
{
	uint16_t wNextNodeIndex;
	uint8_t bDataLen;
	uint8_t sDataBuf[255];
}VmemBufNode255;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif


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

   开发库 mtagent_api_open 说明:
        字符云监控系统内部公共库，提供各种公共的基础函数调用

****/

#ifndef _SV_VMEM_20141117_H_
#define _SV_VMEM_20141117_H_ (1) 

#include <inttypes.h>

#ifndef COMM_LIB_BUG
#define COMM_LIB_BUG ;
#endif

/*
   *  以下接口支持多进程，多线程
   */

// 初始化 - 成功返回 大于等于0， 失败返回小于 0
int32_t MtReport_InitVmem();
int32_t MtReport_InitVmem_ByFlag(int iFlag, int iShmKey);

// 存数据 - 成功返回 大于等于0， 失败返回小于 0
int32_t MtReport_SaveToVmem(const char *pdata, int32_t iDataLen);

// 取数据 - 
// 获取失败返回 NULL，*piBufLen 存返回码
// 获取成功返回指向存储数据的指针，数据长度存入*piBufLen, 返回的指针可能是 vmem 内部的也可能是 pbuf
const char *MtReport_GetFromVmemZeroCp(int32_t index, char *pbuf, int32_t *piBufLen);

// 取数据 - 成功返回 大于等于0， 失败返回小于 0
int32_t MtReport_GetFromVmem(int32_t index, char *pbuf, int32_t *piBufLen);

// 释放索引指向的节点 - 成功返回 大于等于0， 失败返回小于 0
int32_t MtReport_FreeVmem(int32_t index);

// 取数据并释放索引指向的节点 - 成功返回 大于等于0， 失败返回小于 0
int32_t MtReport_GetAndFreeVmem(int32_t index, char *pbuf, int32_t *piBufLen);

// 扫描指定小组中的节点，将其中的空闲节点加入到小组空闲链表中
// iNodeLen 节点长度 - 8, 16, 32 ... 255
// iArrIdx  节点小组索引 0 - VMEM_SHM_COUNT
// iNodeIdx  小组内节点索引 0 - xx_NODE_COUNT
// iScanCount 要扫描的节点数目 
// 返回码：-1 -- 未获取到竞争锁，0 -- 扫描完成， 1 -- 小组还有节点未扫描完成
int MtReport_ScanNode(int iNodeLen, int iArrIdx, int iNodeIdx, int iScanCount);
int MtReport_GetNextNodeLen(int iCurNodeLen);
int MtReport_GetNextArryIdx(int iCurArryIdx);

const char * MtReport_GetFromVmem_Local(int32_t index);
const char * MtReport_GetFromVmem_Local2(int32_t index, int32_t *piDataLen);

void MtReport_show_shm(int len, int idx);

// vmem 类型
#define VMEM_DEF_SHMKEY 2013046000
#define VMEM_SHM_CHECK_STR_8 "3dgtlog1234580l*"
#define VMEM_SHM_CHECK_STR_16 "3dg2352332tsl*]YYYY"
#define VMEM_SHM_CHECK_STR_32 "3dgtlog_dk##Asyyyssl*]"
#define VMEM_SHM_CHECK_STR_64 "3@tlog_dk##ADlslgdjk*]"
#define VMEM_SHM_CHECK_STR_128 "@#)))))))%@#_mtlog]"
#define VMEM_SHM_CHECK_STR_255 "sdigdj#_mtlog_dk##AD___"

#define VMEM_SHM_COUNT 10 // 最大 120 

// 可变缓存, 适用于存储长度变化较大的数据缓存
// 限制存储长度在 1-VMEM_MAX_DATA_LEN 字节的数据
#define VMEM_MAX_DATA_LEN 65000 // 65KB

#define VMEM_NODE_COUNT_MAX 65000

// count 不能超过 VMEM_NODE_COUNT_MAX 
#define VMEM_8_NODE_COUNT 64000
#define VMEM_16_NODE_COUNT 30000
#define VMEM_32_NODE_COUNT 30000
#define VMEM_64_NODE_COUNT 40000
#define VMEM_128_NODE_COUNT 40000
#define VMEM_255_NODE_COUNT 10000

#pragma pack(1)

// vmem 第一个节点, VmemBufNodeFirst 不能超过 VmemBufNode8 大小
typedef struct
{
	uint16_t wNodeCount;
	uint16_t wNodeUsed; // init must 1, first node always used
	uint16_t wDelayFree;
	uint16_t wFreeNodeIndex;
	volatile uint8_t bUseFlag;
	uint16_t wScanFreeIndex; // 空闲节点扫描索引
}VmemBufNodeFirst;

typedef struct
{
	uint16_t wNextNodeIndex;
	uint8_t bDataLen;
	uint8_t sDataBuf[8];
}VmemBufNode8;

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

typedef struct
{
	int8_t cIsInit;
	VmemBufNode8 *pV8Shm[VMEM_SHM_COUNT];
	VmemBufNode16 *pV16Shm[VMEM_SHM_COUNT];
	VmemBufNode32 *pV32Shm[VMEM_SHM_COUNT];
	VmemBufNode64 *pV64Shm[VMEM_SHM_COUNT];
	VmemBufNode128 *pV128Shm[VMEM_SHM_COUNT];
	VmemBufNode255 *pV255Shm[VMEM_SHM_COUNT];
}VmemBuf;

#pragma pack()

extern VmemBuf g_mtReportVmem;

#endif


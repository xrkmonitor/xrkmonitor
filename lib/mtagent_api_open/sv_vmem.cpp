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

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sv_vmem.h"
#include "sv_shm.h"
#include "sv_str.h"
#include "mt_report.h"

VmemBuf g_mtReportVmem;

static char s_sLocalBuf[VMEM_MAX_DATA_LEN+1];

int32_t MtReport_InitVmem_ByFlag(int iFlag, int iShmKey)
{
	if(g_mtReportVmem.cIsInit)
		return 1;

	memset(&g_mtReportVmem, 0, sizeof(g_mtReportVmem));

	int j = 0, iRet = 0, iCreate = 0;
	void *pshm = NULL;
	VmemBufNodeFirst *pfirst = NULL;
#define VMEM_CREATE_INIT_SHM(node, n_count, pshm) do { \
	node *pnode = (node*)pshm; \
	for(iCreate=0; iCreate < n_count; iCreate++) \
		pnode[iCreate].wNextNodeIndex = iCreate+1; \
	pfirst = (VmemBufNodeFirst*)pnode; \
	pfirst->wNodeCount = n_count; \
	pfirst->wNodeUsed = 1; \
	pfirst->wFreeNodeIndex = 1; \
}while(0)

#define VMEM_INIT_SHM(node, n_count, check_str, shm, shmKey) \
	if((iRet=MtReport_GetShm_comm(&pshm, shmKey, \
		sizeof(node)*n_count*VMEM_SHM_COUNT, iFlag, check_str)) >= 0) {  \
		for(j=0; j < VMEM_SHM_COUNT; j++) { \
			g_mtReportVmem.shm[j] = (node*)((char*)pshm+j*sizeof(node)*n_count);  \
			if(iRet > 0) \
				VMEM_CREATE_INIT_SHM(node, n_count, g_mtReportVmem.shm[j]); \
		} \
	} \
	else { \
		SetApiErrorMsg("attach vmem shm failed , key:%d, size:%d ret:%d\n", \
			shmKey, sizeof(node)*n_count, iRet); \
		return -1; \
	}

	VMEM_INIT_SHM(VmemBufNode8, VMEM_8_NODE_COUNT, VMEM_SHM_CHECK_STR_8, pV8Shm, iShmKey)
	VMEM_INIT_SHM(VmemBufNode16, VMEM_16_NODE_COUNT, VMEM_SHM_CHECK_STR_16, pV16Shm, iShmKey+1)
	VMEM_INIT_SHM(VmemBufNode32, VMEM_32_NODE_COUNT, VMEM_SHM_CHECK_STR_32, pV32Shm, iShmKey+2)
	VMEM_INIT_SHM(VmemBufNode64, VMEM_64_NODE_COUNT, VMEM_SHM_CHECK_STR_64, pV64Shm, iShmKey+3)
	VMEM_INIT_SHM(VmemBufNode128, VMEM_128_NODE_COUNT, VMEM_SHM_CHECK_STR_128, pV128Shm, iShmKey+4)
	VMEM_INIT_SHM(VmemBufNode255, VMEM_255_NODE_COUNT, VMEM_SHM_CHECK_STR_255, pV255Shm, iShmKey+5)

#undef VMEM_INIT_SHM

	g_mtReportVmem.cIsInit = 1;
	return 0;
}

int32_t MtReport_InitVmem()
{
	if(g_mtReportVmem.cIsInit)
		return 1;
	return MtReport_InitVmem_ByFlag(0666, VMEM_DEF_SHMKEY);
}

int MtReport_GetNextNodeLen(int iCurNodeLen)
{
	if(iCurNodeLen == 8)
		return 16;
	else if(iCurNodeLen == 16)
		return 32;
	else if(iCurNodeLen == 32)
		return 64;
	else if(iCurNodeLen == 64)
		return 128;
	else if(iCurNodeLen == 128)
		return 255;
	return 8;
}

int MtReport_GetNextArryIdx(int iCurArryIdx)
{
	iCurArryIdx++;
	if(iCurArryIdx >= VMEM_SHM_COUNT)
		return 0;
	return iCurArryIdx;
}

int MtReport_ScanNode(int iNodeLen, int iArrIdx, int iNodeIdx, int iScanCount)
{
#define MA_SCAN_FREE_NODE(type, pshm) do { \
	if(iArrIdx >= VMEM_SHM_COUNT) \
		return 0; \
	type *pNode = g_mtReportVmem.pshm[iArrIdx]; \
	pFirst = (VmemBufNodeFirst*)pNode; \
	if(!VARMEM_CAS_GET(&(pFirst->bUseFlag))) \
		return -1; \
	for(iScan = 0; iNodeIdx < pFirst->wNodeCount && iScan < iScanCount; iScan++) { \
		if(pNode[iNodeIdx].wNextNodeIndex == pFirst->wNodeCount+1) { \
			pNode[iNodeIdx].wNextNodeIndex = pFirst->wFreeNodeIndex; \
			pFirst->wFreeNodeIndex = iNodeIdx; \
			pFirst->wNodeUsed--; \
			pFirst->wDelayFree--; \
			if(pFirst->wNodeUsed <= 0) \
				MtReport_Attr_Add(104, 1); \
			MtReport_Attr_Add(106, 1); \
		}\
		iNodeIdx++; \
	} \
	VARMEM_CAS_FREE(pFirst->bUseFlag); \
	if(iNodeIdx >= pFirst->wNodeCount) \
		return 0; \
	return 1; \
}while(0)

	int iScan = 0;
	VmemBufNodeFirst *pFirst = NULL;
	switch(iNodeLen) {
		case 8:
			MA_SCAN_FREE_NODE(VmemBufNode8, pV8Shm);
			break;
		case 16:
			MA_SCAN_FREE_NODE(VmemBufNode16, pV16Shm);
			break;
		case 32:
			MA_SCAN_FREE_NODE(VmemBufNode32, pV32Shm);
			break;
		case 64:
			MA_SCAN_FREE_NODE(VmemBufNode64, pV64Shm);
			break;
		case 128:
			MA_SCAN_FREE_NODE(VmemBufNode128, pV128Shm);
			break;
		case 255:
			MA_SCAN_FREE_NODE(VmemBufNode255, pV255Shm);
			break;
		default:
			break;
	}
#undef MA_SCAN_FREE_NODE
	return 0;
}

#define VMEM_MAKE_IDEX32(iRet, len, t_idx, n_idx) do { \
	iRet = 0; \
	iRet |= t_idx; iRet <<= 8; \
	iRet |= len; iRet <<= 16; \
	iRet |= n_idx; \
}while(0)

#define SCAN_FREE_NODE do  { \
	for(iScan = 0; iScan < 10; iScan++) { \
		if(pFirst->wScanFreeIndex >= pFirst->wNodeCount) \
			pFirst->wScanFreeIndex = 1; \
		if(pNode[pFirst->wScanFreeIndex].wNextNodeIndex == pFirst->wNodeCount+1) { \
			pNode[pFirst->wScanFreeIndex].wNextNodeIndex = pFirst->wFreeNodeIndex; \
			pFirst->wFreeNodeIndex = pFirst->wScanFreeIndex; \
			pFirst->wNodeUsed--; \
			pFirst->wDelayFree--; \
			if(pFirst->wNodeUsed <= 0) \
				MtReport_Attr_Add(104, 1); \
			MtReport_Attr_Add(106, 1); \
		}\
		pFirst->wScanFreeIndex++; \
	} \
}while(0)

#define VMEM_GET_FREE_NODE do { \
	wFreeIndex = pFirst->wFreeNodeIndex; \
	pFirst->wFreeNodeIndex = pNode[wFreeIndex].wNextNodeIndex; \
}while(0)

#define __SAVE_DATA_TO_VMEM(fun, len, ntype, pshm) \
static int32_t fun(const char *pdata, int32_t iDataLen) \
{ \
	uint16_t wNeedCount = iDataLen/len + ((iDataLen%len) ? 1 : 0); \
	uint16_t wPreIndex = 0, wFreeIndex = 0, wFirstIndex = 0;\
	int32_t iReadBuf = 0, i = 0, iRet = -1, iScan = 0; \
\
	VmemBufNodeFirst *pFirst = NULL; \
	ntype *pNode = NULL; \
\
	for(i=0; i < VMEM_SHM_COUNT; i++) { \
		pNode = g_mtReportVmem.pshm[i]; \
		pFirst = (VmemBufNodeFirst*)pNode; \
		if(!VARMEM_CAS_GET(&(pFirst->bUseFlag))) \
			continue; \
\
		if(pFirst->wNodeCount < pFirst->wNodeUsed+wNeedCount) { \
			VARMEM_CAS_FREE(pFirst->bUseFlag); \
			continue; \
		} \
\
		SCAN_FREE_NODE; \
		VMEM_GET_FREE_NODE; \
		wFirstIndex = wFreeIndex;  \
		pFirst->wNodeUsed += wNeedCount; \
		while(wNeedCount > 1) { \
			memcpy(pNode[wFreeIndex].sDataBuf, pdata+iReadBuf, len); \
			pNode[wFreeIndex].bDataLen = len; \
			wPreIndex = wFreeIndex; \
			iReadBuf += len; \
			VMEM_GET_FREE_NODE; \
			pNode[wPreIndex].wNextNodeIndex = wFreeIndex; \
			wNeedCount--; \
		} \
		pNode[wFreeIndex].wNextNodeIndex = pFirst->wNodeCount; \
		memcpy(pNode[wFreeIndex].sDataBuf, pdata+iReadBuf, iDataLen-iReadBuf); \
		pNode[wFreeIndex].bDataLen = iDataLen-iReadBuf; \
		VARMEM_CAS_FREE(pFirst->bUseFlag); \
		VMEM_MAKE_IDEX32(iRet, len, i, wFirstIndex); \
		break; \
	} \
	return iRet; \
}

__SAVE_DATA_TO_VMEM(_SaveToVmem8, 8, VmemBufNode8, pV8Shm)
__SAVE_DATA_TO_VMEM(_SaveToVmem16, 16, VmemBufNode16, pV16Shm)
__SAVE_DATA_TO_VMEM(_SaveToVmem32, 32, VmemBufNode32, pV32Shm)
__SAVE_DATA_TO_VMEM(_SaveToVmem64, 64, VmemBufNode64, pV64Shm)
__SAVE_DATA_TO_VMEM(_SaveToVmem128, 128, VmemBufNode128, pV128Shm)
__SAVE_DATA_TO_VMEM(_SaveToVmem255, 255, VmemBufNode255, pV255Shm)

#undef __SAVE_DATA_TO_VMEM
#undef SCAN_FREE_NODE
#undef VMEM_GET_FREE_NODE
#undef VMEM_MAKE_IDEX32

int32_t MtReport_SaveToVmem(const char *pdata, int32_t iDataLen)
{
	if(!g_mtReportVmem.cIsInit)
		return -101;

	if(iDataLen <= 0 || iDataLen > VMEM_MAX_DATA_LEN)
		return -1;

#define VMEM_SAVE_CHECK_255 1
#define VMEM_SAVE_CHECK_128 2
#define VMEM_SAVE_CHECK_64 4
#define VMEM_SAVE_CHECK_32 8
#define VMEM_SAVE_CHECK_16 16 
#define VMEM_SAVE_CHECK_8 32 

	int iRet = -1, iCheckFlag = 0;

	// 优先尝试只使用一个节点保存，以便读取时可以不用拷贝数据
	if(iRet < 0 && iDataLen <= 8) {
		iRet = _SaveToVmem8(pdata, iDataLen);
	}
	if(iRet < 0 && iDataLen <= 16) {
		iRet = _SaveToVmem16(pdata, iDataLen);
	}
	if(iRet < 0 && iDataLen <= 32) {
		iRet = _SaveToVmem32(pdata, iDataLen);
	}
	if(iRet < 0 && iDataLen <= 32) {
		iRet = _SaveToVmem32(pdata, iDataLen);
	}
	if(iRet < 0 && iDataLen <= 64) {
		iRet = _SaveToVmem64(pdata, iDataLen);
	}
	if(iRet < 0 && iDataLen <= 128) {
		iRet = _SaveToVmem128(pdata, iDataLen);
	}
	if(iRet < 0 && iDataLen <= 255) {
		iRet = _SaveToVmem255(pdata, iDataLen);
	}
	// --- end zero copy

	// first try to best type
	if(iRet < 0 && iDataLen >= 255) {
		iRet = _SaveToVmem255(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_255;
	}
	if(iRet < 0 && iDataLen >= 128) {
		iRet = _SaveToVmem128(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_128;
	}
	if(iRet < 0 && iDataLen >= 64) {
		iRet = _SaveToVmem64(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_64;
	}
	if(iRet < 0 && iDataLen >= 32) {
		iRet = _SaveToVmem32(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_32;
	}
	if(iRet < 0 && iDataLen >= 16) {
		iRet = _SaveToVmem16(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_16;
	}
	if(iRet < 0 && iDataLen >= 8) {
		iRet = _SaveToVmem8(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_8;
	}

#undef VMEM_SAVE_CHECK_8
#undef VMEM_SAVE_CHECK_16
#undef VMEM_SAVE_CHECK_32
#undef VMEM_SAVE_CHECK_64
#undef VMEM_SAVE_CHECK_128
#undef VMEM_SAVE_CHECK_255

	if(iRet < 0)
		MtReport_Attr_Add(101, 1);
	else
		MtReport_Attr_Add(100, 1);
	return iRet;
}

#define VMEM_PARSE_IDEX32(idx, len, t_idx, n_idx) do { \
	n_idx = (idx & 0xffff); \
	t_idx = ((idx>>24) & 0xff); \
	len = ((idx>>16) & 0xff); \
}while(0)

int32_t MtReport_GetFromVmem(int32_t index, char *pbuf, int32_t *piBufLen)
{
	if(!g_mtReportVmem.cIsInit || index <= 0)
		return -101;

	uint16_t nIdx = 0;
	uint8_t tIdx = 0, len = 0;
	int32_t iBufLen = *piBufLen, iDataLen = 0;
	VMEM_PARSE_IDEX32(index, len, tIdx, nIdx);
	if(tIdx >= VMEM_SHM_COUNT) {
		MtReport_Attr_Add(103, 1);
		return -1;
	}

	VmemBufNodeFirst *pFirst = NULL;

#define VMEM_GET_DATA(type, pshm) do { \
	type *pNode = g_mtReportVmem.pshm[tIdx]; \
	pFirst = (VmemBufNodeFirst*)pNode; \
	if(nIdx >= pFirst->wNodeCount) { \
		MtReport_Attr_Add(103, 1); \
		return -2; \
	} \
	do { \
		if(pNode[nIdx].bDataLen > iBufLen) { \
			MtReport_Attr_Add(103, 1); \
			return -4; \
		} \
		if(pNode[nIdx].bDataLen <= 0) { \
			return -5; \
		} \
		memcpy(pbuf+iDataLen, pNode[nIdx].sDataBuf, pNode[nIdx].bDataLen); \
		iDataLen += pNode[nIdx].bDataLen; \
		iBufLen -= pNode[nIdx].bDataLen; \
		nIdx = pNode[nIdx].wNextNodeIndex; \
	}while(nIdx < pFirst->wNodeCount && iBufLen > 0); \
	*piBufLen = iDataLen; \
}while(0)

	switch(len) {
		case 8:
			VMEM_GET_DATA(VmemBufNode8, pV8Shm);
			break;
		case 16:
			VMEM_GET_DATA(VmemBufNode16, pV16Shm);
			break;
		case 32:
			VMEM_GET_DATA(VmemBufNode32, pV32Shm);
			break;
		case 64:
			VMEM_GET_DATA(VmemBufNode64, pV64Shm);
			break;
		case 128:
			VMEM_GET_DATA(VmemBufNode128, pV128Shm);
			break;
		case 255:
			VMEM_GET_DATA(VmemBufNode255, pV255Shm);
			break;
		default:
			MtReport_Attr_Add(103, 1); 
			return -3;
	}
#undef VMEM_GET_DATA
	MtReport_Attr_Add(102, 1);
	return 0;
}

const char *MtReport_GetFromVmemZeroCp(int32_t index, char *pbuf, int32_t *piBufLen)
{
	if(!g_mtReportVmem.cIsInit || index <= 0)
	{
		*piBufLen = -101;
		return NULL;
	}

	uint16_t nIdx = 0;
	uint8_t tIdx = 0, len = 0;
	int32_t iBufLen = *piBufLen, iDataLen = 0;
	VMEM_PARSE_IDEX32(index, len, tIdx, nIdx);
	if(tIdx >= VMEM_SHM_COUNT) {
		MtReport_Attr_Add(103, 1);
		*piBufLen = -1;
		return NULL;
	}

	VmemBufNodeFirst *pFirst = NULL;

#define VMEM_GET_DATA_ZERO_CP(type, pshm) do { \
	type *pNode = g_mtReportVmem.pshm[tIdx]; \
	pFirst = (VmemBufNodeFirst*)pNode; \
	if(nIdx >= pFirst->wNodeCount) { \
		MtReport_Attr_Add(103, 1); \
		*piBufLen = -3; \
		return NULL; \
	} \
	if(pNode[nIdx].bDataLen < len || pNode[nIdx].wNextNodeIndex >= pFirst->wNodeCount) \
	{ \
		*piBufLen = pNode[nIdx].bDataLen; \
		return (const char*)(pNode[nIdx].sDataBuf); \
	} \
	do { \
		if(pNode[nIdx].bDataLen > iBufLen) { \
			MtReport_Attr_Add(103, 1); \
			*piBufLen = -4; \
			return NULL; \
		} \
		if(pNode[nIdx].bDataLen <= 0) { \
			*piBufLen = -5; \
			return NULL; \
		} \
		memcpy(pbuf+iDataLen, pNode[nIdx].sDataBuf, pNode[nIdx].bDataLen); \
		iDataLen += pNode[nIdx].bDataLen; \
		iBufLen -= pNode[nIdx].bDataLen; \
		nIdx = pNode[nIdx].wNextNodeIndex; \
	}while(nIdx < pFirst->wNodeCount && iBufLen > 0); \
	*piBufLen = iDataLen; \
}while(0)

	switch(len) {
		case 8:
			VMEM_GET_DATA_ZERO_CP(VmemBufNode8, pV8Shm);
			break;
		case 16:
			VMEM_GET_DATA_ZERO_CP(VmemBufNode16, pV16Shm);
			break;
		case 32:
			VMEM_GET_DATA_ZERO_CP(VmemBufNode32, pV32Shm);
			break;
		case 64:
			VMEM_GET_DATA_ZERO_CP(VmemBufNode64, pV64Shm);
			break;
		case 128:
			VMEM_GET_DATA_ZERO_CP(VmemBufNode128, pV128Shm);
			break;
		case 255:
			VMEM_GET_DATA_ZERO_CP(VmemBufNode255, pV255Shm);
			break;
		default:
			MtReport_Attr_Add(103, 1); 
			*piBufLen = -2;
			return NULL;
	}
#undef VMEM_GET_DATA_ZERO_CP 
	MtReport_Attr_Add(102, 1);
	return pbuf;
}

const char * MtReport_GetFromVmem_Local2(int32_t index, int32_t *piDataLen)
{
	int32_t iBufLen = (int)sizeof(s_sLocalBuf);
	if(piDataLen != NULL)
	{
		*piDataLen = iBufLen;
		return MtReport_GetFromVmemZeroCp(index, s_sLocalBuf, piDataLen);
	}
	return MtReport_GetFromVmemZeroCp(index, s_sLocalBuf, &iBufLen);
}

const char * MtReport_GetFromVmem_Local(int32_t index)
{
	int32_t iBufLen = (int)sizeof(s_sLocalBuf);
	return MtReport_GetFromVmemZeroCp(index, s_sLocalBuf, &iBufLen);
}

int32_t MtReport_FreeVmem(int32_t index)
{
	if(!g_mtReportVmem.cIsInit || index <= 0)
		return -101;

	uint16_t nIdx = 0;
	uint8_t tIdx = 0, len = 0, bUseFlagGet = 0;
	int32_t iNextSave = 0;
	VMEM_PARSE_IDEX32(index, len, tIdx, nIdx);
	if(tIdx >= VMEM_SHM_COUNT)
		return -1;

	VmemBufNodeFirst *pFirst = NULL;
/*
   * 释放算法：如果能获取到 useFlag 则释放并加入到空闲链表节点中
   *           否则将 wNextNodeIndex 设置为 wNodeCount+1，此后由
   *           mtreport_agent 根据 wNextNodeIndex 扫描回收放入空闲链表
   */
#define VMEM_FREE_NODE(type, pshm) do { \
	type *pNode = g_mtReportVmem.pshm[tIdx]; \
	pFirst = (VmemBufNodeFirst*)pNode; \
	if(nIdx >= pFirst->wNodeCount) { \
		MtReport_Attr_Add(104, 1); \
		return -2; \
	} \
	if(VARMEM_CAS_GET(&(pFirst->bUseFlag))) \
		bUseFlagGet = 1; \
	do { \
		iNextSave = pNode[nIdx].wNextNodeIndex; \
		if(pNode[nIdx].bDataLen <= 0) { \
			MtReport_Attr_Add(103, 1); \
			break; \
		} \
		pNode[nIdx].bDataLen = 0; \
		if(bUseFlagGet) { \
			pNode[nIdx].wNextNodeIndex = pFirst->wFreeNodeIndex; \
			pFirst->wFreeNodeIndex = nIdx; \
			pFirst->wNodeUsed--; \
			if(pFirst->wNodeUsed <= 0) \
				MtReport_Attr_Add(104, 1); \
		} \
		else { \
			pNode[nIdx].wNextNodeIndex = pFirst->wNodeCount+1; \
			pFirst->wDelayFree++; \
			MtReport_Attr_Add(105, 1); \
		} \
		nIdx = iNextSave; \
	}while(nIdx < pFirst->wNodeCount); \
	if(bUseFlagGet) \
		VARMEM_CAS_FREE(pFirst->bUseFlag); \
}while(0)

	switch(len) {
		case 8:
			VMEM_FREE_NODE(VmemBufNode8, pV8Shm);
			break;
		case 16:
			VMEM_FREE_NODE(VmemBufNode16, pV16Shm);
			break;
		case 32:
			VMEM_FREE_NODE(VmemBufNode32, pV32Shm);
			break;
		case 64:
			VMEM_FREE_NODE(VmemBufNode64, pV64Shm);
			break;
		case 128:
			VMEM_FREE_NODE(VmemBufNode128, pV128Shm);
			break;
		case 255:
			VMEM_FREE_NODE(VmemBufNode255, pV255Shm);
			break;
		default:
			return -3;
	}
#undef VMEM_FREE_NODE
	return 0;
}

int32_t MtReport_GetAndFreeVmem(int32_t index, char *pbuf, int32_t *piBufLen)
{
	if(MtReport_GetFromVmem(index, pbuf, piBufLen) < 0)
		return -1;
	MtReport_FreeVmem(index);
	return 0;
}

void MtReport_show_shm(int len, int idx)
{
	int i =0; 
	VmemBufNodeFirst *pfirst = NULL;
	for(; i < VMEM_SHM_COUNT; i++) {
		printf("------------------- (%d) ------------------\n", i);
		if((len == 0 || len == 8) && (idx < 0 || idx == i))
		{
			VmemBufNode8 *pNode8 = g_mtReportVmem.pV8Shm[i];
			pfirst = (VmemBufNodeFirst*)pNode8;
			printf("node8 - %d\n", i);
			printf("node count:%d node use:%d delay free:%d free idx:%d uflag:%d scan:%d\n", pfirst->wNodeCount,
					pfirst->wNodeUsed, pfirst->wDelayFree, pfirst->wFreeNodeIndex, pfirst->bUseFlag, pfirst->wScanFreeIndex);
			printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode8[1].wNextNodeIndex, pNode8[2].wNextNodeIndex,
					pNode8[3].wNextNodeIndex, VMEM_8_NODE_COUNT-1, pNode8[VMEM_8_NODE_COUNT-1].wNextNodeIndex);
			printf("\n\n");
		}

		if((len == 0 || len == 16) && (idx < 0 || idx == i))
		{
			VmemBufNode16 *pNode16 = g_mtReportVmem.pV16Shm[i];
			pfirst = (VmemBufNodeFirst*)pNode16;
			printf("node16 - %d\n", i);
			printf("node count:%d node use:%d delay free:%d free idx:%d uflag:%d scan:%d\n", pfirst->wNodeCount,
					pfirst->wNodeUsed, pfirst->wDelayFree, pfirst->wFreeNodeIndex, pfirst->bUseFlag, pfirst->wScanFreeIndex);
			printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode16[1].wNextNodeIndex, pNode16[2].wNextNodeIndex,
					pNode16[3].wNextNodeIndex, VMEM_16_NODE_COUNT-1, pNode16[VMEM_16_NODE_COUNT-1].wNextNodeIndex);
			printf("\n\n");
		}

		if((len == 0 || len == 32) && (idx < 0 || idx == i))
		{
			VmemBufNode32 *pNode = g_mtReportVmem.pV32Shm[i];
			pfirst = (VmemBufNodeFirst*)pNode;
			printf("node32 - %d\n", i);
			printf("node count:%d node use:%d delay free:%d free idx:%d uflag:%d scan:%d\n", pfirst->wNodeCount,
					pfirst->wNodeUsed, pfirst->wDelayFree, pfirst->wFreeNodeIndex, pfirst->bUseFlag, pfirst->wScanFreeIndex);
			printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode[1].wNextNodeIndex, pNode[2].wNextNodeIndex,
					pNode[3].wNextNodeIndex, VMEM_32_NODE_COUNT-1, pNode[VMEM_32_NODE_COUNT-1].wNextNodeIndex);
			printf("\n\n");
		}

		if((len == 0 || len == 64) && (idx < 0 || idx == i))
		{
			VmemBufNode64 *pNode64 = g_mtReportVmem.pV64Shm[i];
			pfirst = (VmemBufNodeFirst*)pNode64;
			printf("node64 - %d\n", i);
			printf("node count:%d node use:%d delay free:%d free idx:%d uflag:%d scan:%d\n", pfirst->wNodeCount,
					pfirst->wNodeUsed, pfirst->wDelayFree, pfirst->wFreeNodeIndex, pfirst->bUseFlag, pfirst->wScanFreeIndex);
			printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode64[1].wNextNodeIndex, pNode64[2].wNextNodeIndex,
					pNode64[3].wNextNodeIndex, VMEM_64_NODE_COUNT-1, pNode64[VMEM_64_NODE_COUNT-1].wNextNodeIndex);
			printf("\n\n");
		}

		if((len == 0 || len == 128) && (idx < 0 || idx == i))
		{
			VmemBufNode128 *pNode128 = g_mtReportVmem.pV128Shm[i];
			pfirst = (VmemBufNodeFirst*)pNode128;
			printf("node128 - %d\n", i);
			printf("node count:%d node use:%d delay free:%d free idx:%d uflag:%d scan:%d\n", pfirst->wNodeCount,
					pfirst->wNodeUsed, pfirst->wDelayFree, pfirst->wFreeNodeIndex, pfirst->bUseFlag, pfirst->wScanFreeIndex);
			printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode128[1].wNextNodeIndex, pNode128[2].wNextNodeIndex,
					pNode128[3].wNextNodeIndex, VMEM_128_NODE_COUNT-1, pNode128[VMEM_128_NODE_COUNT-1].wNextNodeIndex);
			printf("\n\n");
		}

		if((len == 0 || len == 255) && (idx < 0 || idx == i))
		{
			VmemBufNode255 *pNode255 = g_mtReportVmem.pV255Shm[i];
			pfirst = (VmemBufNodeFirst*)pNode255;
			printf("node255 - %d\n", i);
			printf("node count:%d node use:%d delay free:%d free idx:%d uflag:%d scan:%d\n", pfirst->wNodeCount,
					pfirst->wNodeUsed, pfirst->wDelayFree, pfirst->wFreeNodeIndex, pfirst->bUseFlag, pfirst->wScanFreeIndex);
			printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode255[1].wNextNodeIndex, pNode255[2].wNextNodeIndex,
					pNode255[3].wNextNodeIndex, VMEM_255_NODE_COUNT-1, pNode255[VMEM_255_NODE_COUNT-1].wNextNodeIndex);
			printf("\n\n");
		}
	}
}


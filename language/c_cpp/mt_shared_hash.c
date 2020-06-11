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

#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include "mt_report.h"
#include "mt_shared_hash.h"
#include "mt_shm.h"
#include "mt_attr.h"

#define SHARED_HASH_ACCESS_CHECK_FAILED do { \
	pTableHead->bHashUseFlag = 0; ResetHashTable(phash); }while(0)
#define SHARED_HASH_ASSERT(p)  do { if(!(p)) { SHARED_HASH_ACCESS_CHECK_FAILED; }}while(0)

int IsProcExist(int pid)
{
	char szProcFile[64] = {0};
	snprintf(szProcFile, sizeof(szProcFile), "/proc/%d/cmdline", pid);
	FILE *fp = fopen(szProcFile, "r");
	if(fp != NULL)
	{
		fclose(fp);
		return 1;
	}
	return 0;
}

uint8_t IsValidIndex(SharedHashTable *phash, uint32_t dwIndex)
{
	return dwIndex < phash->dwRealNodeCount;
}

 uint32_t GetFirstIndex(SharedHashTable *phash)
{
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	return pTableHead->dwNodeStartIndex;
}

 uint32_t GetLastIndex(SharedHashTable *phash)
{
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	return pTableHead->dwNodeEndIndex;
}

 uint32_t GetCurIndex(SharedHashTable *phash)
{
	return phash->dwCurIndex;
}

 uint32_t GetCurIndexRevers(SharedHashTable *phash)
{
	return phash->dwCurIndexRevers;
}

 uint32_t GetNodeUsed(SharedHashTable *phash)
{
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	return pTableHead->dwNodeUseCount;
}

 uint32_t GetNodeTotal(SharedHashTable *phash)
{
	return phash->dwRealNodeCount;
}

 uint32_t GetSharedMemBytes(SharedHashTable *phash)
{
	return phash->dwShareMemBytes;
}	

 uint32_t GetNextIndex(SharedHashTable *phash, uint32_t dwIndex)
{
	if(dwIndex >= phash->dwRealNodeCount)
		return phash->dwRealNodeCount;
	_HashNodeHead *pNodeHead = INDEX_TO_NODE_HEAD(dwIndex);
	return pNodeHead->dwNodeNextIndex;
}



#define _ASSERT_CUR_NODE_USE_RET_VALUE(ret) do { \
	pNodeHead = INDEX_TO_NODE_HEAD(phash->dwCurIndex); \
	if(!pNodeHead->bIsUsed) { \
		phash->dwCurIndex = phash->dwRealNodeCount; \
		return ret; \
	}}while(0)

static int32_t IsPrime(uint32_t dwNum)
{
	uint32_t dwMax = (uint32_t)sqrt((double)dwNum);
	uint32_t i = 2;
	for(; i <= dwMax; i++)
		if(!(dwNum%i))
			return 0; 
	return 1; 
}

void ShowInnerSharedInfoByNode(SharedHashTable *phash, void *pNode)
{
	_HashNodeHead * pNodeHead = NULL;
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;

	printf("hash table info --- \n\tnode size:%u, node count:%u, node real count:%u\n",
		phash->dwNodeSize, phash->dwNodeCount, phash->dwRealNodeCount);
	printf("\tmemory size:%u, lastup:%u, init ok:%d, access check:%d\n",
		phash->dwShareMemBytes, phash->dwLastUpdateTime, phash->bInitSuccess, phash->bAccessCheck);
	printf("\tcur index:%u, cur index revers:%u, fun cmp:%p, fun war:%p, hash:%p\n",
		phash->dwCurIndex, phash->dwCurIndexRevers, phash->fun_cmp, phash->fun_war, phash->pHash);

	printf("\n\n");
	printf("inner table head info ---------\n");
	printf("\tstrar index:%u, end index:%u, node use:%u, node size:%u, node count:%u, wprocess id:%u\n",
		pTableHead->dwNodeStartIndex, pTableHead->dwNodeEndIndex, pTableHead->dwNodeUseCount,
		pTableHead->dwNodeSize, pTableHead->dwNodeCount, pTableHead->dwWriteProcessId);

	if(pNode != NULL)
	{
		printf("\n\n");
		pNodeHead = NODE_TO_NODE_HEAD(pNode);
		printf("inner node head info -----------------------\n");
		printf("\tis use:%d, pre index:%u, next index:%u\n",
			pNodeHead->bIsUsed, pNodeHead->dwNodePreIndex, pNodeHead->dwNodeNextIndex);
	}
	printf("\n\n\n");
}

void ShowInnerSharedInfoByIndex(SharedHashTable *phash, uint32_t dwIndex)
{
	if(dwIndex >= phash->dwRealNodeCount)
		ShowInnerSharedInfoByNode(phash, NULL);
	else
		ShowInnerSharedInfoByNode(phash, INDEX_TO_NODE(dwIndex));
}

 _HashTableHead * MtReport_GetHashTableHead(SharedHashTable *phash)
{
	 return (_HashTableHead*)phash->pHash;
}

static int CheckHashTable(SharedHashTable *phash)
{
	if(!phash->bInitSuccess)
		return -1;
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;

	uint32_t dwStartCheckTime = time(NULL);
	int i=0;

check_hash_try_next:
	for(i=0; i < 10; i++)
	{
		if(__sync_bool_compare_and_swap(&pTableHead->bHashUseFlag, 0, 1))
		{
			pTableHead->dwLastUseTimeSec = time(NULL);
			break;
		}
		usleep(1000);
	}
	if(i >= 10)
	{
		MtReport_Attr_Spec(91, 1);
		if(dwStartCheckTime+3 < (uint32_t)time(NULL))
		{
			pTableHead->bHashUseFlag = 0;
			exit(-1); 
		}

		if(dwStartCheckTime+2 < (uint32_t)time(NULL))
			pTableHead->bHashUseFlag = 0;
		goto check_hash_try_next;
	}

	int iCheckNodeCount = 0;
	_HashNodeHead *pNodeHead = NULL;
	for(i = pTableHead->dwNodeStartIndex; i >= 0 && i < (int)phash->dwRealNodeCount;)
	{
		pNodeHead = INDEX_TO_NODE_HEAD(i);
		if(pNodeHead->bIsUsed)
			iCheckNodeCount++;
		else
			break;
		i = pNodeHead->dwNodeNextIndex;
	}
	if(iCheckNodeCount != (int)pTableHead->dwNodeUseCount)
	{
		pTableHead->bHashUseFlag = 0;
		return -1;
	}

	iCheckNodeCount = 0;
	for(i = pTableHead->dwNodeEndIndex; i >= 0 && i < (int)phash->dwRealNodeCount;)
	{
		pNodeHead = INDEX_TO_NODE_HEAD(i);
		if(pNodeHead->bIsUsed)
			iCheckNodeCount++;
		else
			break;
		i = pNodeHead->dwNodePreIndex;
	}
	if(iCheckNodeCount != (int)pTableHead->dwNodeUseCount)
	{
		pTableHead->bHashUseFlag = 0;
		return -2;
	}

	pTableHead->bHashUseFlag = 0;
	return 0;
}

void ResetHashTable(SharedHashTable *phash)
{
	if(!phash->bInitSuccess)
		return;

	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;

	uint32_t dwStartCheckTime = time(NULL);
	int i=0;

check_hash_try_next:
	for(i=0; i < 10; i++)
	{
		if(__sync_bool_compare_and_swap(&pTableHead->bHashUseFlag, 0, 1))
		{
			pTableHead->dwLastUseTimeSec = time(NULL);
			break;
		}
		usleep(1000);
	}
	if(i >= 10)
	{
		MtReport_Attr_Spec(91, 1);
		if(dwStartCheckTime+3 < (uint32_t)time(NULL))
		{
			pTableHead->bHashUseFlag = 0;
			exit(-1); 
		}

		if(dwStartCheckTime+2 < (uint32_t)time(NULL))
			pTableHead->bHashUseFlag = 0;
		goto check_hash_try_next;
	}

	char *pNodes = (char*)(phash->pHash) + sizeof(_HashTableHead);
	memset(pNodes, 0, phash->dwShareMemBytes-sizeof(_HashTableHead));
	pTableHead->dwNodeStartIndex = phash->dwRealNodeCount;
	pTableHead->dwNodeEndIndex = phash->dwRealNodeCount;
	pTableHead->dwNodeUseCount = 0;
	pTableHead->dwLastUseTimeSec = 0;
	pTableHead->bHashUseFlag = 0;
	MtReport_Attr_Spec(239, 1);
}

int32_t MtReport_CreateHashTable(SharedHashTable *phash, void *pShm, char cShmInit,
	uint32_t dwNodeSize, uint32_t dwNodeCount, FunCompare cmp, FunWarning warn)
{
	if(phash->bInitSuccess)
		return 0;

	if(0 == dwNodeSize || 0 == dwNodeCount || NULL == cmp)
		return -1;

	phash->dwNodeSize = dwNodeSize;
	phash->dwNodeCount = dwNodeCount;
	phash->dwRealNodeCount = 0;
	uint32_t dwTmp = dwNodeCount;
	int32_t i = 0;
	for(; i < STATIC_HASH_ROW_MAX; i++)
	{
		for(dwTmp--; dwTmp >= 1002; dwTmp--) 
		{
			if(IsPrime(dwTmp))
				break;
		}
		phash->dwNodeNums[i] = dwTmp;
		phash->dwRealNodeCount += dwTmp;
	}
	phash->fun_cmp = cmp;
	phash->fun_war = warn;
	phash->dwShareMemBytes = sizeof(_HashTableHead)+dwNodeCount*(dwNodeSize+sizeof(_HashNodeHead));
	phash->pHash = pShm;

	if(cShmInit == 1) {
		_HashTableHead *pHead = (_HashTableHead*)phash->pHash;
		if(pHead->dwNodeSize != dwNodeSize || pHead->dwNodeCount != dwNodeCount)
			return -3;
	}
	else { 
		
		_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
		pTableHead->dwNodeStartIndex = phash->dwRealNodeCount;
		pTableHead->dwNodeEndIndex = phash->dwRealNodeCount;
		pTableHead->dwNodeSize = dwNodeSize;
		pTableHead->dwNodeCount = dwNodeCount;
	}
	
	phash->bInitSuccess = 1;
	phash->dwCurIndex = phash->dwRealNodeCount;
	phash->dwCurIndexRevers = phash->dwRealNodeCount;

	if(phash->bAccessCheck)
	{
		if(CheckHashTable(phash) != 0)
			ResetHashTable(phash);
	}
	return 0;
}

int32_t MtReport_InitHashTable(SharedHashTable *phash,
	uint32_t dwNodeSize, uint32_t dwNodeCount, int32_t *piSharedKey, FunCompare cmp, FunWarning warn)
{
	if(phash->bInitSuccess)
		return 0;

	if(0 == dwNodeSize || 0 == dwNodeCount || 0 == *piSharedKey || NULL == cmp)
		return -1;

	phash->dwNodeSize = dwNodeSize;
	phash->dwNodeCount = dwNodeCount;
	phash->dwRealNodeCount = 0;
	uint32_t dwTmp = dwNodeCount;
	int32_t i = 0;
	for(; i < STATIC_HASH_ROW_MAX; i++)
	{
		for(dwTmp--; dwTmp >= 1002; dwTmp--) 
		{
			if(IsPrime(dwTmp))
				break;
		}
		phash->dwNodeNums[i] = dwTmp;
		phash->dwRealNodeCount += dwTmp;
	}
	phash->fun_cmp = cmp;
	phash->fun_war = warn;
	phash->dwShareMemBytes = sizeof(_HashTableHead)+phash->dwRealNodeCount*(dwNodeSize+sizeof(_HashNodeHead));

	int iRet = 0;
	int iShmKey = *piSharedKey;
	for(i=0; i < MT_ATTACH_SHM_TRY_MAX; i++) {
		iRet = MtReport_GetShm((void**)(&phash->pHash),
			iShmKey+i, phash->dwShareMemBytes, 0666, MTREPORT_SHARED_HASH_CHECK_STR);
		if(iRet == 0)
			break;
	}

	if(i >= MT_ATTACH_SHM_TRY_MAX)
		return -1;
	*piSharedKey = iShmKey+i;

	_HashTableHead *pHead = (_HashTableHead*)phash->pHash;
	if(pHead->dwNodeSize != dwNodeSize || pHead->dwNodeCount != dwNodeCount)
		return -3;
	
	phash->bInitSuccess = 1;
	phash->dwCurIndex = phash->dwRealNodeCount;
	phash->dwCurIndexRevers = phash->dwRealNodeCount;
	return 0;
}

int32_t InitHashTable(SharedHashTable *phash,
	uint32_t dwNodeSize, uint32_t dwNodeCount, uint32_t dwSharedKey, FunCompare cmp, FunWarning warn)
{
	if(phash->bInitSuccess)
		return 0;

	if(0 == dwNodeSize || 0 == dwNodeCount || 0 == dwSharedKey || NULL == cmp)
		return -1;

	phash->dwNodeSize = dwNodeSize;
	phash->dwNodeCount = dwNodeCount;
	phash->dwRealNodeCount = 0;
	uint32_t dwTmp = dwNodeCount;
	int32_t i = 0;
	for(; i < STATIC_HASH_ROW_MAX; i++)
	{
		for(dwTmp--; dwTmp >= 1002; dwTmp--) 
		{
			if(IsPrime(dwTmp))
				break;
		}
		phash->dwNodeNums[i] = dwTmp;
		phash->dwRealNodeCount += dwTmp;
	}
	phash->fun_cmp = cmp;
	phash->fun_war = warn;
	phash->dwShareMemBytes = sizeof(_HashTableHead)+phash->dwRealNodeCount*(dwNodeSize+sizeof(_HashNodeHead));

	if(!(phash->pHash=MtReportGetShm(dwSharedKey, phash->dwShareMemBytes, 0666)))
	{
		if(!(phash->pHash=MtReportGetShm(dwSharedKey, phash->dwShareMemBytes, 0666|IPC_CREAT)))
			return -2;
		memset(phash->pHash, 0, phash->dwShareMemBytes);
		_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
		pTableHead->dwNodeStartIndex = phash->dwRealNodeCount;
		pTableHead->dwNodeEndIndex = phash->dwRealNodeCount;
		pTableHead->dwNodeSize = dwNodeSize;
		pTableHead->dwNodeCount = dwNodeCount;
	}

	_HashTableHead *pHead = (_HashTableHead*)phash->pHash;
	if(pHead->dwNodeSize != dwNodeSize || pHead->dwNodeCount != dwNodeCount)
		return -3;
	
	phash->bInitSuccess = 1;
	phash->dwCurIndex = phash->dwRealNodeCount;
	phash->dwCurIndexRevers = phash->dwRealNodeCount;
	return 0;
}

int32_t InitHashTableForWrite(SharedHashTable *phash,
	uint32_t dwNodeSize, uint32_t dwNodeCount, uint32_t dwSharedKey, FunCompare cmp, FunWarning warn)
{
	int32_t iRet = 0;

	if((iRet=InitHashTable(phash, dwNodeSize, dwNodeCount, dwSharedKey, cmp, warn)) != 0)
		return iRet;

	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	pTableHead->dwWriteProcessId = 0;
	return 0;
}

void * HashTableSearch(SharedHashTable *phash, const void *pKey, uint32_t dwShortKey)
{
	if(!phash->bInitSuccess)
		return NULL;

	uint32_t dwNodeSize = phash->dwNodeSize;
	uint32_t i = 0;
	void *pRow = phash->pHash + sizeof(_HashTableHead);
	void *pNode = NULL;
	_HashNodeHead *pNodeHead = NULL;
	for(; i < STATIC_HASH_ROW_MAX; i++) 
	{
	    pNodeHead = (_HashNodeHead*)(
			(char *)pRow+(dwNodeSize+sizeof(_HashNodeHead))*(dwShortKey%phash->dwNodeNums[i]));
		pNode = (char*)pNodeHead + sizeof(_HashNodeHead);
		if(pNodeHead->bIsUsed && phash->fun_cmp(pKey, pNode) == 0)
			return pNode;
		pRow = (char *)pRow + (dwNodeSize+sizeof(_HashNodeHead))*phash->dwNodeNums[i];
	}
	return NULL;
}

void * HashTableSearchEx(SharedHashTable *phash, const void *pKey,  uint32_t dwShortKey, uint32_t *pdwIsFind)
{
	if(!phash->bInitSuccess)
		return NULL;

	if(pdwIsFind) 
		*pdwIsFind = 0;

	uint32_t dwNodeSize = phash->dwNodeSize;
	uint32_t i = 0;
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	_HashNodeHead *pNodeHead = NULL;
	void *pRow = phash->pHash + sizeof(_HashTableHead);
	void *pNode = NULL, *pEmptyNode = NULL, *pFoundNode = NULL;
	for(i=0; i < STATIC_HASH_ROW_MAX; i++) 
	{
	    pNodeHead = (_HashNodeHead*)(
			(char *)pRow+(dwNodeSize+sizeof(_HashNodeHead))*(dwShortKey%phash->dwNodeNums[i]));
		pNode = (char*)pNodeHead + sizeof(_HashNodeHead);
		if(pNodeHead->bIsUsed && phash->fun_cmp(pKey, pNode) == 0)
		{
			if(pdwIsFind)
				*pdwIsFind = 1;
			pFoundNode = pNode;
			break;
		}

		if(pEmptyNode == NULL && !pNodeHead->bIsUsed)
		{
			pEmptyNode = pNode;
			if(i >= STATIC_HASH_ROW_WARN && phash->fun_war != NULL)
				phash->fun_war(pTableHead->dwNodeUseCount, phash->dwRealNodeCount);
		} 
		pRow = (char *)pRow + (dwNodeSize+sizeof(_HashNodeHead))*phash->dwNodeNums[i];
	}
	return (pFoundNode ? pFoundNode : pEmptyNode);
}

int32_t InsertHashNodeSort(SharedHashTable *phash, void *pNode)
{
	if(!phash->bInitSuccess)
		return -1;

	_HashNodeHead *pNewNodeHead = NODE_TO_NODE_HEAD(pNode); 
	uint32_t dwNodeIndex = NODE_TO_INDEX(pNode);
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	_HashNodeHead *pNodeLastHead = NULL;

	if(pTableHead->dwWriteProcessId != 0) {
		SHARED_HASH_ASSERT((uint32_t)getpid() == pTableHead->dwWriteProcessId);
	}
	else {
		int  i=0;
		for(; i < 10; i++)
		{
			if(__sync_bool_compare_and_swap(&pTableHead->bHashUseFlag, 0, 1))
			{
				pTableHead->dwLastUseTimeSec = time(NULL);
				break;
			}
			usleep(1000);
		}
		if(i >= 10)
		{
			if(pTableHead->dwLastUseTimeSec+2 < (uint32_t)time(NULL))
				pTableHead->bHashUseFlag = 0;
			MtReport_Attr_Spec(237, 1);
			return -33;
		}
	}

	void *pNodeInsert = NULL;
	if(pTableHead->dwNodeUseCount > 0)
	{
		pTableHead->dwNodeUseCount++;
		pNodeLastHead = INDEX_TO_NODE_HEAD(pTableHead->dwNodeEndIndex);
		do
		{
			pNodeInsert = ((char*)pNodeLastHead+sizeof(_HashNodeHead));
			if(!(phash->fun_cmp(pNodeInsert, pNode) > 0))
				break;
			if(pNodeLastHead->dwNodePreIndex == phash->dwRealNodeCount) 
			{
				pNodeInsert = NULL;
				break;
			}
			pNodeLastHead = INDEX_TO_NODE_HEAD(pNodeLastHead->dwNodePreIndex);
		}while(1);

		if(NULL == pNodeInsert)
		{
			if(INDEX_TO_NODE_HEAD(pTableHead->dwNodeStartIndex) != pNodeLastHead)
			{
				SHARED_HASH_ASSERT(0);
				pTableHead->bHashUseFlag = 0;
				ResetHashTable(phash);
				return -3;
			}
			
			pNodeLastHead->dwNodePreIndex = dwNodeIndex;
			pNewNodeHead->dwNodePreIndex = phash->dwRealNodeCount;
			pNewNodeHead->dwNodeNextIndex = pTableHead->dwNodeStartIndex; 
			pTableHead->dwNodeStartIndex = dwNodeIndex;
		}
		else  if(pNodeLastHead->dwNodeNextIndex == phash->dwRealNodeCount)
		{
			if(INDEX_TO_NODE_HEAD(pTableHead->dwNodeEndIndex) != pNodeLastHead)
			{
				SHARED_HASH_ASSERT(0);
				pTableHead->bHashUseFlag = 0;
				ResetHashTable(phash);
				return -4;
			}

			pNodeLastHead->dwNodeNextIndex = dwNodeIndex;
			pNewNodeHead->dwNodeNextIndex = phash->dwRealNodeCount;
			pNewNodeHead->dwNodePreIndex = pTableHead->dwNodeEndIndex;
			pTableHead->dwNodeEndIndex = dwNodeIndex;
		}
		else
		{
			pNewNodeHead->dwNodeNextIndex = pNodeLastHead->dwNodeNextIndex;
			pNewNodeHead->dwNodePreIndex = NODE_TO_INDEX(pNodeInsert);
			(INDEX_TO_NODE_HEAD(pNodeLastHead->dwNodeNextIndex))->dwNodePreIndex = dwNodeIndex;
			pNodeLastHead->dwNodeNextIndex = dwNodeIndex;
		}
	
	}
	else
	{
		pTableHead->dwNodeUseCount = 1;
		pNewNodeHead->dwNodePreIndex = phash->dwRealNodeCount;
		pNewNodeHead->dwNodeNextIndex = phash->dwRealNodeCount;
		pTableHead->dwNodeEndIndex = dwNodeIndex;
		pTableHead->dwNodeStartIndex = dwNodeIndex;
	}
	pNewNodeHead->bIsUsed = 1;
	pTableHead->bHashUseFlag = 0;
	pTableHead->dwLastUseTimeSec = 0;
	return 0;
}

int32_t InsertHashNodeAfter(SharedHashTable *phash, void *pNode, uint32_t dwIndex)
{
	if(!phash->bInitSuccess)
		return -1;

	if(dwIndex >= phash->dwRealNodeCount)
		return -2;

	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;

	if(pTableHead->dwWriteProcessId != 0) {
		SHARED_HASH_ASSERT((uint32_t)getpid() == pTableHead->dwWriteProcessId);
	}
	else {
		int  i=0;
		for(; i < 10; i++)
		{
			if(__sync_bool_compare_and_swap(&pTableHead->bHashUseFlag, 0, 1))
			{
				pTableHead->dwLastUseTimeSec = time(NULL);
				break;
			}
			usleep(1000);
		}
		if(i >= 10)
		{
			if(pTableHead->dwLastUseTimeSec+2 < (uint32_t)time(NULL))
				pTableHead->bHashUseFlag = 0;
			MtReport_Attr_Spec(237, 1);
			return -33;
		}
	}

	_HashNodeHead *pNewNodeHead = NODE_TO_NODE_HEAD(pNode); 
	_HashNodeHead *pNodeHead = INDEX_TO_NODE_HEAD(dwIndex); 

	pTableHead->dwNodeUseCount++;
	uint32_t dwOldNextIndex = pNodeHead->dwNodeNextIndex;
	pNewNodeHead->dwNodeNextIndex = dwOldNextIndex;
	pNewNodeHead->dwNodePreIndex = dwIndex;
	pNodeHead->dwNodeNextIndex = NODE_TO_INDEX(pNode); 

	if(dwOldNextIndex >= phash->dwRealNodeCount)
	{
		pTableHead->dwNodeEndIndex = dwIndex;
	}
	else
	{
		_HashNodeHead *pOldNextNodeHead = INDEX_TO_NODE_HEAD(dwOldNextIndex);
		pOldNextNodeHead->dwNodePreIndex = NODE_TO_INDEX(pNode);
	}
	pNewNodeHead->bIsUsed = 1;
	pTableHead->bHashUseFlag = 0;
	pTableHead->dwLastUseTimeSec = 0;
	return 0;
}

int32_t InsertHashNode(SharedHashTable *phash, void *pNode)
{
	if(!phash->bInitSuccess)
		return -1;

	_HashNodeHead *pNewNodeHead = NODE_TO_NODE_HEAD(pNode); 
	uint32_t dwNodeIndex = NODE_TO_INDEX(pNode);
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	_HashNodeHead *pNodeLastHead = NULL;

	if(pTableHead->dwWriteProcessId != 0) {
		SHARED_HASH_ASSERT((uint32_t)getpid() == pTableHead->dwWriteProcessId);
	}
	else {
		int  i=0;
		for(; i < 10; i++)
		{
			if(__sync_bool_compare_and_swap(&pTableHead->bHashUseFlag, 0, 1))
			{
				pTableHead->dwLastUseTimeSec = time(NULL);
				break;
			}
			usleep(1000);
		}
		if(i >= 10)
		{
			if(pTableHead->dwLastUseTimeSec+2 < (uint32_t)time(NULL))
				pTableHead->bHashUseFlag = 0;
			MtReport_Attr_Spec(237, 1);
			return -33;
		}
	}

	if(pTableHead->dwNodeUseCount > 0)
	{
		pTableHead->dwNodeUseCount++;
		pNodeLastHead = INDEX_TO_NODE_HEAD(pTableHead->dwNodeEndIndex);
		pNodeLastHead->dwNodeNextIndex = dwNodeIndex;
		pNewNodeHead->dwNodePreIndex = pTableHead->dwNodeEndIndex;
		pNewNodeHead->dwNodeNextIndex = phash->dwRealNodeCount;
		pTableHead->dwNodeEndIndex = dwNodeIndex;
	}
	else
	{
		pTableHead->dwNodeUseCount = 1;
		pNewNodeHead->dwNodePreIndex = phash->dwRealNodeCount;
		pNewNodeHead->dwNodeNextIndex = phash->dwRealNodeCount;
		pTableHead->dwNodeEndIndex = dwNodeIndex;
		pTableHead->dwNodeStartIndex = dwNodeIndex;
	}
	pNewNodeHead->bIsUsed = 1;
	pTableHead->bHashUseFlag = 0;
	pTableHead->dwLastUseTimeSec = 0;
	return 0;
}

int32_t RemoveHashNodeByIndex(SharedHashTable *phash, uint32_t dwIndex)
{
	if(dwIndex >= phash->dwRealNodeCount)
		return -101;
	void *pNode = INDEX_TO_NODE(dwIndex);
	return RemoveHashNode(phash, pNode);
}

int32_t RemoveHashNode(SharedHashTable *phash, void *pNode)
{
	if(!phash->bInitSuccess)
		return -1;

	_HashNodeHead *pNodeHead = NODE_TO_NODE_HEAD(pNode);
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	if(pTableHead->dwWriteProcessId != 0) {
		SHARED_HASH_ASSERT((uint32_t)getpid() == pTableHead->dwWriteProcessId);
	}
	else {
		int  i=0;
		for(; i < 10; i++)
		{
			if(__sync_bool_compare_and_swap(&pTableHead->bHashUseFlag, 0, 1))
			{
				pTableHead->dwLastUseTimeSec = time(NULL);
				break;
			}
			usleep(1000);
		}
		if(i >= 10)
		{
			if(pTableHead->dwLastUseTimeSec+2 < (uint32_t)time(NULL))
				pTableHead->bHashUseFlag = 0;
			MtReport_Attr_Spec(237, 1);
			return -33;
		}
	}

	uint32_t dwIndex = NODE_TO_INDEX(pNode);
	if(phash->dwCurIndex == dwIndex)
		GetNextNode(phash);
	if(phash->dwCurIndexRevers == dwIndex)
		GetNextNodeRevers(phash);

	pTableHead->dwNodeUseCount--;
	if(pTableHead->dwNodeUseCount <= 0)
	{
		pTableHead->dwNodeStartIndex = phash->dwRealNodeCount;
		pTableHead->dwNodeEndIndex = phash->dwRealNodeCount;
	}
	else if(pNodeHead->dwNodeNextIndex == phash->dwRealNodeCount)
	{
		_HashNodeHead *pNodePrev = INDEX_TO_NODE_HEAD(pNodeHead->dwNodePreIndex); 
		if(!pNodePrev->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return -2; 
		}
		pNodePrev->dwNodeNextIndex = phash->dwRealNodeCount;
		pTableHead->dwNodeEndIndex = pNodeHead->dwNodePreIndex;
	}
	else if(pNodeHead->dwNodePreIndex == phash->dwRealNodeCount)
	{
		_HashNodeHead *pNodeNext = INDEX_TO_NODE_HEAD(pNodeHead->dwNodeNextIndex); 
		if(!pNodeNext->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return -3; 
		}
		pNodeNext->dwNodePreIndex = phash->dwRealNodeCount;
		pTableHead->dwNodeStartIndex = pNodeHead->dwNodeNextIndex; 
	}
	else
	{
		_HashNodeHead *pNodePrev = INDEX_TO_NODE_HEAD(pNodeHead->dwNodePreIndex); 
		_HashNodeHead *pNodeNext = INDEX_TO_NODE_HEAD(pNodeHead->dwNodeNextIndex); 
		if(!pNodePrev->bIsUsed || !pNodeNext->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return -4; 
		}
		pNodePrev->dwNodeNextIndex = pNodeHead->dwNodeNextIndex; 
		pNodeNext->dwNodePreIndex = pNodeHead->dwNodePreIndex;
	}
	pNodeHead->bIsUsed = 0;
	pNodeHead->dwNodeNextIndex = phash->dwRealNodeCount;
	pNodeHead->dwNodePreIndex = phash->dwRealNodeCount;
	pTableHead->bHashUseFlag = 0;
	pTableHead->dwLastUseTimeSec = 0;
	return 0;
}

void * GetCurNode(SharedHashTable *phash)
{
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	_HashNodeHead *pNodeHead = NULL;
	if(phash->dwCurIndex == phash->dwRealNodeCount)
		return NULL;

	if(phash->bAccessCheck)
	{
		pNodeHead = INDEX_TO_NODE_HEAD(phash->dwCurIndex);
		if(!pNodeHead->bIsUsed)
		{
			phash->dwCurIndex = phash->dwRealNodeCount;
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
	}
	return INDEX_TO_NODE(phash->dwCurIndex);
}

void * GetFirstNode(SharedHashTable *phash)
{
	_HashNodeHead *pNodeHead = NULL;
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	phash->dwCurIndex = pTableHead->dwNodeStartIndex;
	if(phash->dwCurIndex == phash->dwRealNodeCount)
		return NULL;

	if(phash->bAccessCheck)
	{
		pNodeHead = INDEX_TO_NODE_HEAD(phash->dwCurIndex);
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
	}
	return INDEX_TO_NODE(phash->dwCurIndex);
}

void * GetNextNodeByIndex(SharedHashTable *phash, uint32_t dwIndex)
{
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	_HashNodeHead * pNodeHead = INDEX_TO_NODE_HEAD(dwIndex); 
	uint32_t dwNextIndex = pNodeHead->dwNodeNextIndex;
	if(dwNextIndex == phash->dwRealNodeCount)
		return NULL;

	if(phash->bAccessCheck)
	{
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
		pNodeHead = INDEX_TO_NODE_HEAD(dwNextIndex);
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
	}

	return INDEX_TO_NODE(dwNextIndex); 
}

void * GetNextNodeByNode(SharedHashTable *phash, void *pnode)
{
	_HashNodeHead * pNodeHead = (_HashNodeHead*)((char*)pnode-sizeof(_HashNodeHead)); 
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	uint32_t dwNextIndex = pNodeHead->dwNodeNextIndex;
	if(dwNextIndex == phash->dwRealNodeCount)
		return NULL;

	if(phash->bAccessCheck)
	{
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
		pNodeHead = INDEX_TO_NODE_HEAD(dwNextIndex);
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
	}

	return INDEX_TO_NODE(dwNextIndex); 
}

void * GetNextNode(SharedHashTable *phash)
{
	if(phash->dwCurIndex == phash->dwRealNodeCount)
		return NULL;
	_HashNodeHead * pNodeHead = INDEX_TO_NODE_HEAD(phash->dwCurIndex);
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	phash->dwCurIndex = pNodeHead->dwNodeNextIndex;
	if(phash->dwCurIndex == phash->dwRealNodeCount)
		return NULL;

	if(phash->bAccessCheck)
	{
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
		pNodeHead = INDEX_TO_NODE_HEAD(phash->dwCurIndex);
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
	}

	return INDEX_TO_NODE(phash->dwCurIndex); 
}

void * GetCurNodeRevers(SharedHashTable *phash)
{
	_HashNodeHead * pNodeHead = NULL; 
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	if(phash->dwCurIndexRevers == phash->dwRealNodeCount)
		return NULL;

	if(phash->bAccessCheck)
	{
		pNodeHead = INDEX_TO_NODE_HEAD(phash->dwCurIndexRevers);
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
	}
	return INDEX_TO_NODE(phash->dwCurIndexRevers);
}

void * GetFirstNodeRevers(SharedHashTable *phash)
{
	_HashNodeHead * pNodeHead = NULL; 
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	phash->dwCurIndexRevers = pTableHead->dwNodeEndIndex;
	if(phash->dwCurIndexRevers == phash->dwRealNodeCount)
		return NULL;

	if(phash->bAccessCheck)
	{
		pNodeHead = INDEX_TO_NODE_HEAD(phash->dwCurIndexRevers);
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
	}

	return INDEX_TO_NODE(phash->dwCurIndexRevers);
}

void * GetNextNodeByIndexRevers(SharedHashTable *phash, uint32_t dwIndex)
{
	_HashNodeHead * pNodeHead = INDEX_TO_NODE_HEAD(dwIndex); 
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	uint32_t dwPreIndex = pNodeHead->dwNodePreIndex;
	if(dwPreIndex == phash->dwRealNodeCount)
		return NULL;

	if(phash->bAccessCheck)
	{
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
		pNodeHead = INDEX_TO_NODE_HEAD(dwPreIndex);
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
	}

	return INDEX_TO_NODE(dwPreIndex); 
}

void * GetNextNodeByNodeRevers(SharedHashTable *phash, void *pnode)
{
	_HashNodeHead * pNodeHead = (_HashNodeHead*)((char*)pnode-sizeof(_HashNodeHead)); 
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	uint32_t dwPreIndex = pNodeHead->dwNodePreIndex;
	if(dwPreIndex == phash->dwRealNodeCount)
		return NULL;

	if(phash->bAccessCheck)
	{
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
		pNodeHead = INDEX_TO_NODE_HEAD(dwPreIndex);
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
	}

	return INDEX_TO_NODE(dwPreIndex); 
}

void * GetNextNodeRevers(SharedHashTable *phash)
{
	if(phash->dwCurIndexRevers == phash->dwRealNodeCount)
		return NULL;
	_HashNodeHead * pNodeHead = INDEX_TO_NODE_HEAD(phash->dwCurIndexRevers);
	_HashTableHead *pTableHead = (_HashTableHead*)phash->pHash;
	phash->dwCurIndexRevers = pNodeHead->dwNodePreIndex;
	if(phash->dwCurIndexRevers == phash->dwRealNodeCount)
		return NULL;

	if(phash->bAccessCheck)
	{
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
		pNodeHead = INDEX_TO_NODE_HEAD(phash->dwCurIndexRevers);
		if(!pNodeHead->bIsUsed)
		{
			SHARED_HASH_ACCESS_CHECK_FAILED;
			return NULL;
		}
	}

	return INDEX_TO_NODE(phash->dwCurIndexRevers); 
}



int32_t InitHashTable_NoList(SharedHashTableNoList *phash,
	uint32_t dwNodeSize, uint32_t dwNodeCount, uint32_t dwSharedKey, FunCompare cmp, FunWarning warn)
{
	if(phash->bInitSuccess)
		return 0;

	if(0 == dwNodeSize || 0 == dwNodeCount || 0 == dwSharedKey || NULL == cmp)
		return -1;

	memset(phash, 0, sizeof(SharedHashTableNoList));
	phash->dwNodeSize = dwNodeSize;
	phash->dwNodeCount = dwNodeCount;
	phash->dwRealNodeCount = 0;
	uint32_t dwTmp = dwNodeCount;
	int32_t i = 0;
	for(; i < STATIC_HASH_ROW_MAX; i++)
	{
		for(; dwTmp >= 1002; dwTmp--) 
		{
			if(IsPrime(dwTmp))
				break;
		}
		phash->dwNodeNums[i] = dwTmp;
		phash->dwRealNodeCount += dwTmp;
	}
	phash->fun_cmp = cmp;
	phash->fun_war = warn;
	phash->dwShareMemBytes = phash->dwRealNodeCount*dwNodeSize;

	if(!(phash->pHash=MtReportGetShm(dwSharedKey, phash->dwShareMemBytes, 0666)))
	{
		if(!(phash->pHash=MtReportGetShm(dwSharedKey, phash->dwShareMemBytes, 0666|IPC_CREAT)))
			return -2;
		memset(phash->pHash, 0, phash->dwShareMemBytes);
	}
	phash->bInitSuccess = 1;
	return 0;
}

int32_t InitHashTable_NoList_Heap(SharedHashTableNoList *phash,
	uint32_t dwNodeSize, uint32_t dwNodeCount, FunCompare cmp, FunWarning warn)
{
	if(phash->bInitSuccess)
		return 0;

	if(0 == dwNodeSize || 0 == dwNodeCount || NULL == cmp)
		return -1;

	memset(phash, 0, sizeof(SharedHashTableNoList));
	phash->dwNodeSize = dwNodeSize;
	phash->dwNodeCount = dwNodeCount;
	phash->dwRealNodeCount = 0;
	uint32_t dwTmp = dwNodeCount;
	int32_t i = 0;
	for(; i < STATIC_HASH_ROW_MAX; i++)
	{
		for(; dwTmp >= 1002; dwTmp--) 
		{
			if(IsPrime(dwTmp))
				break;
		}
		phash->dwNodeNums[i] = dwTmp;
		phash->dwRealNodeCount += dwTmp;
	}
	phash->fun_cmp = cmp;
	phash->fun_war = warn;
	phash->dwShareMemBytes = phash->dwRealNodeCount*dwNodeSize;

	phash->pHash = (char*)malloc(phash->dwShareMemBytes);
	assert(phash->pHash != NULL);
	memset(phash->pHash, 0, phash->dwShareMemBytes);
	phash->bInitSuccess = 1;
	return 0;
}

void * HashTableSearch_NoList(SharedHashTableNoList *phash, const void *pKey, uint32_t dwShortKey)
{
	if(!phash->bInitSuccess)
		return NULL;

	uint32_t dwNodeSize = phash->dwNodeSize;
	uint32_t i = 0;
	void *pRow = phash->pHash;
	void *pNode = NULL;
	for(; i < STATIC_HASH_ROW_MAX; i++) 
	{
	    pNode = (char *)pRow + dwNodeSize*(dwShortKey % phash->dwNodeNums[i]);
		if(phash->fun_cmp(pKey, pNode) == 0)
			return pNode;
		pRow = (char *)pRow + dwNodeSize*phash->dwNodeNums[i];
	}
	return NULL;
}

void * HashTableSearchEx_NoList(SharedHashTableNoList *phash, 
	const void *pKey, const void *pEmptyKey, uint32_t dwShortKey, uint32_t *pdwIsFind)
{
	if(pdwIsFind) 
		*pdwIsFind = 0;

	if(!phash->bInitSuccess)
		return NULL;
	uint32_t dwNodeSize = phash->dwNodeSize;
	uint32_t i = 0;
	void *pRow = phash->pHash;
	void *pNode = NULL, *pEmptyNode = NULL, *pFoundNode = NULL;
	for(i=0; i < STATIC_HASH_ROW_MAX; i++) 
	{
	    pNode = (char *)pRow + dwNodeSize*(dwShortKey % phash->dwNodeNums[i]);
		if(phash->fun_cmp(pKey, pNode) == 0)
		{
			if(pdwIsFind)
				*pdwIsFind = 1;
			pFoundNode = pNode;
			break;
		}

		if(pEmptyNode == NULL && phash->fun_cmp(pEmptyKey, pNode) == 0)
		{
			pEmptyNode = pNode;
			if(i >= STATIC_HASH_ROW_WARN && phash->fun_war != NULL)
				phash->fun_war(i, STATIC_HASH_ROW_MAX);
		} 
		pRow = (char *)pRow + dwNodeSize*phash->dwNodeNums[i];
	}
	return (pFoundNode ? pFoundNode : pEmptyNode);
}

static uint32_t s_iGetAllNodeRowIdx = 0;
static uint32_t s_iGetAllNodeColIdx = 0;
static char * s_pRow = NULL;
void InitGetAllHashNode()
{
    s_iGetAllNodeRowIdx = 0;
    s_iGetAllNodeColIdx = 0;
    s_pRow = NULL;
}

void * GetAllHashNode(SharedHashTable *phash)
{
    if(!phash->bInitSuccess)
        return NULL;

    uint32_t dwNodeSize = phash->dwNodeSize;
    _HashNodeHead *pNodeHead = NULL;
    if(s_iGetAllNodeRowIdx >= STATIC_HASH_ROW_MAX)
        return NULL;
    if(s_pRow == NULL)
        s_pRow = phash->pHash + sizeof(_HashTableHead);

    pNodeHead = (_HashNodeHead*)(s_pRow+(dwNodeSize+sizeof(_HashNodeHead))*s_iGetAllNodeColIdx);
    void * pNode = (char*)pNodeHead + sizeof(_HashNodeHead);
    s_iGetAllNodeColIdx++;
    if(s_iGetAllNodeColIdx >= phash->dwNodeNums[ s_iGetAllNodeRowIdx ])
    {
        s_pRow = s_pRow + (dwNodeSize+sizeof(_HashNodeHead))*phash->dwNodeNums[ s_iGetAllNodeRowIdx ];
        s_iGetAllNodeRowIdx++;
        s_iGetAllNodeColIdx = 0;
    }
    return pNode;
}

void * GetAllHashNode_NoList(SharedHashTableNoList *phash)
{
    if(!phash->bInitSuccess)
        return NULL;

    uint32_t dwNodeSize = phash->dwNodeSize;
    if(s_iGetAllNodeRowIdx >= STATIC_HASH_ROW_MAX)
        return NULL;
    if(s_pRow == NULL)
        s_pRow = phash->pHash;

    void * pNode =s_pRow + dwNodeSize*s_iGetAllNodeColIdx;
    s_iGetAllNodeColIdx++;
    if(s_iGetAllNodeColIdx >= phash->dwNodeNums[ s_iGetAllNodeRowIdx ])
    {
        s_pRow = s_pRow + dwNodeSize*phash->dwNodeNums[ s_iGetAllNodeRowIdx ];
        s_iGetAllNodeRowIdx++;
        s_iGetAllNodeColIdx = 0;
    }
    return pNode;
}

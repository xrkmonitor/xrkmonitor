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

#include <sys/sem.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mt_report.h"
#include "sv_shm.h"
#include "sv_vmem.h"

static SharedHashTable stStrHash;
static SharedHashTable stHash;
static AttrNode * pBaseAttrNode = NULL;

inline uint32_t GetShortKeyByStr(const char *pstr)
{
	uint32_t dwKey = 0;
	int iLen = strlen(pstr);
	while(iLen > 4) {
		dwKey += *(uint32_t*)pstr;
		iLen -= 4;
		pstr += 4;
	}
	while(iLen > 0) {
		dwKey += *pstr;
		pstr++;
		iLen--;
	}
	return dwKey;
}

void GetCustAttrSaveInfo()
{
	void deal_daemon_signal(int sig);

	FILE *fp = fopen("/tmp/hash.pid", "a+");
	if(fp != NULL)
	{
		fprintf(fp, "pid:%u\n", getpid());
		fclose(fp);
		deal_daemon_signal(255);
	}
}
#define CHECK_ATTR_CUST(cust) ; //do { if(cust == 90) GetCustAttrSaveInfo(); } while(0)

int MtReport_Attr_Spec(int32_t attr, int32_t iValue)
{
	CHECK_ATTR_CUST(attr);
	if(pBaseAttrNode == NULL){
		if(GetShm2((void**)&pBaseAttrNode,
			DEP_BASE_SHM_ID, sizeof(AttrNode)*MAX_BASE_ATTR_NODE, 0666|IPC_CREAT) < 0)
			return -1;
	}
	int i = 0, jFree = -1;
	for(; i < MAX_BASE_ATTR_NODE; i++)
	{
		if(pBaseAttrNode[i].iAttrID == attr)
		{
			pBaseAttrNode[i].iCurValue += iValue;
			return 0;
		}
		else if(pBaseAttrNode[i].iAttrID == 0 && jFree < 0)
			jFree = i;
	}

	if(jFree >= 0)
	{
		pBaseAttrNode[jFree].iAttrID = attr;
		pBaseAttrNode[jFree].iCurValue = iValue;
	}
	else
		return -3;
	return 0;
}

//pKey , pNode 相等 返回 0
inline int str_attr_cmp(const void *pKey, const void *pNode)
{
	if(((const StrAttrNode*)pKey)->iStrAttrId == ((const StrAttrNode*)pNode)->iStrAttrId
		&& !strcmp(((const StrAttrNode*)pKey)->szStrInfo, ((const StrAttrNode*)pNode)->szStrInfo))
		return 0;
	return 1;
}

inline int attr_cmp(const void *pKey, const void *pNode)
{
	if(((const AttrNode*)pKey)->iAttrID == ((const AttrNode*)pNode)->iAttrID)
		return 0;
	return 1;
}

inline static int ensureAttrList()
{
	if(stHash.bInitSuccess && stStrHash.bInitSuccess)
		return 0;

	if(InitHashTable(&stHash, sizeof(AttrNode), MAX_ATTR_NODE, DEP_SHM_ID, attr_cmp, NULL) != 0)
		return -1;
	if(InitHashTable(&stStrHash, sizeof(StrAttrNode), 
		MAX_STR_ATTR_NODE_COUNT, DEF_STR_ATTR_NODE_SHM_KEY, str_attr_cmp, NULL) != 0)
		return -2;
	
	return 0;
}

int MtReport_InitAttr()
{
	return ensureAttrList();
}

int ShowAllAttr()
{
	if(pBaseAttrNode == NULL){
		if(GetShm2((void**)&pBaseAttrNode,
			DEP_BASE_SHM_ID, sizeof(AttrNode)*MAX_BASE_ATTR_NODE, 0666|IPC_CREAT) < 0){
			printf("attach BaseAttrNode failed !");
			return -1;
		}
	}

	int i = 0;
	for(; i < MAX_BASE_ATTR_NODE; i++)
	{
		if(pBaseAttrNode[i].iAttrID != 0)
			printf("attr: %d value: %d\n", pBaseAttrNode[i].iAttrID, pBaseAttrNode[i].iCurValue);
	}

	if(ensureAttrList() < 0)
	{
		printf("ensureAttrList failed !");
		return -2;
	}

	AttrNode *pNode = (AttrNode*)GetFirstNode(&stHash);
	while(pNode != NULL)
	{
		printf("attr: %d value: %d\n", pNode->iAttrID, pNode->iCurValue);
		pNode = (AttrNode*)GetNextNode(&stHash);
	}

	StrAttrNode *pStrNode = (StrAttrNode*)GetFirstNode(&stStrHash);
	while(pStrNode != NULL)
	{
		printf("str attr: %d, str:%s value: %d\n",
			pStrNode->iStrAttrId, pStrNode->szStrInfo , pStrNode->iStrVal);
		pStrNode = (StrAttrNode*)GetNextNode(&stStrHash);
	}
	return 0;
}

int MtReport_Attr_Add(int32_t attr,int32_t iValue)
{
	CHECK_ATTR_CUST(attr);
	if (ensureAttrList() != 0)
		return -1;

	static AttrNode stRecord;

	uint32_t isFind = 0;
	uint8_t bCheckCount = 0;
	stRecord.iAttrID = attr;
	AttrNode *pNode = NULL;

check_again:
	if(bCheckCount > 100)
		return -2;

	pNode = (AttrNode*)HashTableSearchEx(&stHash, &stRecord, attr, &isFind);
	if(isFind)
		pNode->iCurValue += iValue;
	else if(pNode != NULL)
	{
		if(__sync_bool_compare_and_swap(&pNode->bSyncProcess, 0, 1))
		{
			pNode->iCurValue = iValue;
			pNode->iAttrID = attr;
			InsertHashNode(&stHash, pNode);
		}
		else
		{
			bCheckCount++;
			MtReport_Attr_Spec(236, 1);
			goto check_again;
		}
	}
	return 0;
}

int MtReport_Attr_Set(int32_t attr,int32_t iValue)
{
	CHECK_ATTR_CUST(attr);
	if (ensureAttrList() != 0)
		return -1;
	static AttrNode stRecord;

	uint32_t isFind = 0;
	uint8_t bCheckCount = 0;
	stRecord.iAttrID = attr;
	AttrNode *pNode = NULL;

check_again:
	if(bCheckCount > 100)
		return -2;

	pNode = (AttrNode*)HashTableSearchEx(&stHash, &stRecord, attr, &isFind);
	if(isFind)
	{
		pNode->iCurValue = iValue;
	}
	else if(pNode != NULL)
	{
		if(__sync_bool_compare_and_swap(&pNode->bSyncProcess, 0, 1))
		{
			pNode->iCurValue = iValue;
			pNode->iAttrID = attr;
			InsertHashNode(&stHash, pNode);
		}
		else
		{
			bCheckCount++;
			MtReport_Attr_Spec(236, 1);
			goto check_again;
		}
	}
	return 0;
}

int MtReport_Str_Attr_Add(int32_t attr, const char *pstr, int32_t iValue)
{
	CHECK_ATTR_CUST(attr);
	if (ensureAttrList() != 0)
		return -1;

	static StrAttrNode stRecord;

	uint32_t isFind = 0;
	uint8_t bCheckCount = 0;
	stRecord.iStrAttrId = attr;
	strncpy(stRecord.szStrInfo, pstr, sizeof(stRecord.szStrInfo)-1);
	StrAttrNode *pNode = NULL;
	uint32_t dwShortKey = attr+GetShortKeyByStr(stRecord.szStrInfo);

check_again:
	if(bCheckCount > 100)
		return -2;

	pNode = (StrAttrNode*)HashTableSearchEx(&stStrHash, &stRecord, dwShortKey, &isFind);
	if(isFind)
		pNode->iStrVal += iValue;
	else if(pNode != NULL)
	{
		if(__sync_bool_compare_and_swap(&pNode->bSyncProcess, 0, 1))
		{
			pNode->iStrVal = iValue;
			pNode->iStrAttrId = attr;
			strncpy(pNode->szStrInfo, pstr, sizeof(stRecord.szStrInfo)-1);
			InsertHashNode(&stStrHash, pNode);
		}
		else
		{
			bCheckCount++;
			MtReport_Attr_Spec(236, 1);
			goto check_again;
		}
	}
	return 0;
}

int MtReport_Str_Attr_Set(int32_t attr, const char *pstr, int32_t iValue)
{
	CHECK_ATTR_CUST(attr);
	if (ensureAttrList() != 0)
		return -1;

	static StrAttrNode stRecord;

	uint32_t isFind = 0;
	uint8_t bCheckCount = 0;
	stRecord.iStrAttrId = attr;
	strncpy(stRecord.szStrInfo, pstr, sizeof(stRecord.szStrInfo)-1);
	StrAttrNode *pNode = NULL;
	uint32_t dwShortKey = attr+GetShortKeyByStr(stRecord.szStrInfo);

check_again:
	if(bCheckCount > 100)
		return -2;

	pNode = (StrAttrNode*)HashTableSearchEx(&stStrHash, &stRecord, dwShortKey, &isFind);
	if(isFind)
		pNode->iStrVal = iValue;
	else if(pNode != NULL)
	{
		if(__sync_bool_compare_and_swap(&pNode->bSyncProcess, 0, 1))
		{
			pNode->iStrVal = iValue;
			pNode->iStrAttrId = attr;
			strncpy(pNode->szStrInfo, pstr, sizeof(stRecord.szStrInfo)-1);
			InsertHashNode(&stStrHash, pNode);
		}
		else
		{
			bCheckCount++;
			MtReport_Attr_Spec(236, 1);
			goto check_again;
		}
	}
	return 0;
}

void ShowStrAttr(int32_t attr, const char *pstr)
{
	if (ensureAttrList() != 0) {
		printf("init attr shm failed\n");
		return;
	}

	StrAttrNode stRecord;
	stRecord.iStrAttrId = attr;
	strncpy(stRecord.szStrInfo, pstr, sizeof(stRecord.szStrInfo)-1);
	uint32_t dwShortKey = attr+GetShortKeyByStr(stRecord.szStrInfo);
	StrAttrNode *pNode = (StrAttrNode*)HashTableSearch(&stStrHash, &stRecord, dwShortKey);
	if(pNode != NULL) {
		printf("attr:%d, str:%s, val:%d, sync:%d\n", 
			pNode->iStrAttrId, pNode->szStrInfo, pNode->iStrVal, pNode->bSyncProcess);
	}
	else {
		printf("not find str attr:%d, str:%s\n", attr, pstr);
	}
}

void ShowAttr(int32_t attr)
{
	if (ensureAttrList() != 0) {
		printf("init attr shm failed\n");
		return;
	}

	AttrNode stRecord;
	stRecord.iAttrID = attr;
	AttrNode *pNode = (AttrNode*)HashTableSearch(&stHash, &stRecord, attr);
	if(pNode != NULL) {
		printf("attr:%d, val:%d, sync:%d\n", pNode->iAttrID, pNode->iCurValue, pNode->bSyncProcess);
	}
	else {
		printf("not find attr:%d\n", attr);
	}
}


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

#include <sys/sem.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "mt_log.h"
#include "mt_attr.h"
#include "mt_shm.h"
#include "mt_shared_hash.h"

int MtReport_Attr_Spec(int32_t attr, int32_t iValue);

//pKey , pNode 相等 返回 0
inline int mtreport_str_attr_cmp(const void *pKey, const void *pNode)
{
	if(((const StrAttrNode*)pKey)->iStrAttrId == ((const StrAttrNode*)pNode)->iStrAttrId
		&& !strcmp(((const StrAttrNode*)pKey)->szStrInfo, ((const StrAttrNode*)pNode)->szStrInfo))
		return 0;
	return 1;
}

inline int mtreport_attr_cmp(const void *pKey, const void *pNode)
{
	if(((const AttrNode*)pKey)->iAttrID == ((const AttrNode*)pNode)->iAttrID)
		return 0;
	return 1;
}

int MtReport_InitAttr()
{
	if(g_mtReport.cIsAttrInit)
		return 1;

	if(g_mtReport.pMtShm == NULL)
		return -101;

	int i = 0;
	for(i=0; i < MTATTR_SHM_DEF_COUNT; i++) {
		if(MtReport_CreateHashTable(
			&g_mtReport.stAttrHash[i],
			g_mtReport.pMtShm->szAttrList
				+ i*(sizeof(_HashTableHead)
				+ MTATTR_HASH_NODE*STATIC_HASH_ROW_MAX*(sizeof(_HashNodeHead)+sizeof(AttrNode))),
			g_mtReport.pMtShm->cIsAttrInit, sizeof(AttrNode), MTATTR_HASH_NODE, mtreport_attr_cmp, NULL) < 0)
			break;
	}
	if(i < MTATTR_SHM_DEF_COUNT) {
		fprintf(stderr, "attach attr shm failed - count:%d\n", i);
		return -1;
	}
	g_mtReport.pMtShm->cIsAttrInit = 1;

	if(MtReport_CreateHashTable(
		&g_mtReport.stStrAttrHash,
		g_mtReport.pMtShm->szStrAttrBuf,
		g_mtReport.pMtShm->cIsStrAttrInit, sizeof(StrAttrNode), MTATTR_HASH_NODE, mtreport_str_attr_cmp, NULL) < 0)
	{
		fprintf(stderr, "attach str attr shm failed - size:%d\n", (int)sizeof(g_mtReport.pMtShm->szStrAttrBuf));
		return -2;
	}
	g_mtReport.pMtShm->cIsStrAttrInit = 1;

	g_mtReport.cIsAttrInit = 1;
	return 0;
}

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

int MtReport_Attr_Spec(int32_t attr, int32_t iValue)
{
	int i = 0;

	if(MtReport_InitAttr() < 0 || attr <= 0)
		return -1;

	g_mtReport.pMtShm->dwAttrReportBySpecCount++;
	for(i=0; i < g_mtReport.pMtShm->iAttrSpecialCount; i++) {
		if(g_mtReport.pMtShm->sArrtListSpec[i].iAttrID == attr)
			break;
	}
	if(i >= g_mtReport.pMtShm->iAttrSpecialCount) {
		i = g_mtReport.pMtShm->iAttrSpecialCount++;
		if(i >= MTATTR_SPECIAL_COUNT) {
			g_mtReport.pMtShm->wAttrReportFailed++;
			return -1;
		}
		g_mtReport.pMtShm->sArrtListSpec[i].iAttrID = attr;
		g_mtReport.pMtShm->sArrtListSpec[i].iCurValue = iValue;
	}
	else 
		g_mtReport.pMtShm->sArrtListSpec[i].iCurValue += iValue;
	return 0;
}

int MtReport_Attr_Add(int32_t attr, int32_t iValue)
{
	static AttrNode stRecord;

	if(MtReport_InitAttr() < 0 || attr <= 0)
		return -1;
	g_mtReport.pMtShm->dwAttrReportCount++;

	uint32_t isFind = 0;
	uint8_t bCheckCount = 0;
	stRecord.iAttrID = attr;
	AttrNode *pNode = NULL;
	int j=0;

check_again:
	if(bCheckCount > 10) {
		MtReport_Attr_Spec(238, 1);
		return -2;
	}

	for(; j < MTATTR_SHM_DEF_COUNT; j++)
	{
		pNode = (AttrNode*)HashTableSearchEx(&g_mtReport.stAttrHash[j], &stRecord, attr, &isFind);
		if(isFind) 
		{
			pNode->iCurValue += iValue;
			break;
		}
		else if(pNode != NULL) 
		{
			if(VARMEM_CAS_GET(&pNode->bSyncProcess) || pNode->bSyncProcess>=10)
			{
				pNode->iCurValue = iValue;
				pNode->iAttrID = attr;
				InsertHashNode(&g_mtReport.stAttrHash[j], pNode);
				break;
			}
			else
			{
				pNode->bSyncProcess++;
				bCheckCount++;
				j++;
				usleep(1000);
				goto check_again;
			}
		}
	}
	if(j < MTATTR_SHM_DEF_COUNT)
		return 0;
	return MtReport_Attr_Spec(attr, iValue);
}

int MtReport_Attr_Set(int32_t attr, int32_t iValue)
{
	static AttrNode stRecord;

	if(MtReport_InitAttr() < 0 || attr <= 0)
		return -1;
	g_mtReport.pMtShm->dwAttrReportCount++;

	uint32_t isFind = 0;
	uint8_t bCheckCount = 0;
	stRecord.iAttrID = attr;
	AttrNode *pNode = NULL;
	int j=0;

check_again:
	if(bCheckCount > 10) {
		MtReport_Attr_Spec(238, 1);
		return -2;
	}

	for(; j < MTATTR_SHM_DEF_COUNT; j++)
	{
		pNode = (AttrNode*)HashTableSearchEx(&g_mtReport.stAttrHash[j], &stRecord, attr, &isFind);
		if(isFind)
		{
			pNode->iCurValue = iValue;
			break;
		}
		else if(pNode != NULL)
		{
			if(VARMEM_CAS_GET(&pNode->bSyncProcess) || pNode->bSyncProcess >= 10)
			{
				pNode->iCurValue = iValue;
				pNode->iAttrID = attr;
				InsertHashNode(&g_mtReport.stAttrHash[j], pNode);
				break;
			}
			else
			{
				pNode->bSyncProcess++;
				bCheckCount++;
				j++;
				usleep(1000);
				goto check_again;
			}
		}
	}
	if(j < MTATTR_SHM_DEF_COUNT)
		return 0;
	return MtReport_Attr_Spec(attr, iValue);
}

int MtReport_Str_Attr_Add(int32_t attr, const char *pstr, int32_t iValue)
{
	static StrAttrNode stRecord;

	if(MtReport_InitAttr() < 0 || attr <= 0)
		return -1;
	g_mtReport.pMtShm->dwAttrReportCount++;

	uint32_t isFind = 0;
	uint8_t bCheckCount = 0;
	stRecord.iStrAttrId = attr;
	strncpy(stRecord.szStrInfo, pstr, sizeof(stRecord.szStrInfo)-1);
	StrAttrNode *pNode = NULL;
	uint32_t dwShortKey = attr+GetShortKeyByStr(stRecord.szStrInfo);

check_again:
	if(bCheckCount > 10) {
		MtReport_Attr_Spec(238, 1);
		return -2;
	}

	pNode = (StrAttrNode*)HashTableSearchEx(&g_mtReport.stStrAttrHash, &stRecord, dwShortKey, &isFind);
	if(isFind)
		pNode->iStrVal += iValue;
	else if(pNode != NULL)
	{
		if(VARMEM_CAS_GET(&pNode->bSyncProcess) || pNode->bSyncProcess >= 10)
		{
			pNode->iStrVal = iValue;
			pNode->iStrAttrId = attr;
			strncpy(pNode->szStrInfo, pstr, sizeof(stRecord.szStrInfo)-1);
			InsertHashNode(&g_mtReport.stStrAttrHash, pNode);
		}
		else
		{
			pNode->bSyncProcess++;
			bCheckCount++;
			usleep(1000);
			goto check_again;
		}
	}
	return 0;
}

int MtReport_Str_Attr_Set(int32_t attr, const char *pstr, int32_t iValue)
{
	static StrAttrNode stRecord;
	if (MtReport_InitAttr() < 0 || attr <= 0)
		return -1;

	uint32_t isFind = 0;
	uint8_t bCheckCount = 0;
	stRecord.iStrAttrId = attr;
	strncpy(stRecord.szStrInfo, pstr, sizeof(stRecord.szStrInfo)-1);
	StrAttrNode *pNode = NULL;
	uint32_t dwShortKey = attr+GetShortKeyByStr(stRecord.szStrInfo);

check_again:
	if(bCheckCount > 10) {
		MtReport_Attr_Spec(238, 1);
		return -2;
	}

	pNode = (StrAttrNode*)HashTableSearchEx(&g_mtReport.stStrAttrHash, &stRecord, dwShortKey, &isFind);
	if(isFind)
		pNode->iStrVal = iValue;
	else if(pNode != NULL)
	{
		if(VARMEM_CAS_GET(&pNode->bSyncProcess) || pNode->bSyncProcess >= 10)
		{
			pNode->iStrVal = iValue;
			pNode->iStrAttrId = attr;
			strncpy(pNode->szStrInfo, pstr, sizeof(stRecord.szStrInfo)-1);
			InsertHashNode(&g_mtReport.stStrAttrHash, pNode);
		}
		else
		{
			pNode->bSyncProcess++;
			bCheckCount++;
			usleep(1000);
			goto check_again;
		}
	}
	return 0;
}


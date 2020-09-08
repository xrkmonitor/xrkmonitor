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
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>

#include "mt_report.h"
#include "mt_log.h"
#include "sv_cfg.h"
#include "mt_shm.h"
#include "mt_vmem.h"

MtReport g_mtReport = {0};
char *uitodate(uint32_t dwTimeSec)
{
	static char sBuf[64];
	struct tm stTm;
	time_t tmnew = dwTimeSec;
	localtime_r(&tmnew, &stTm);
	strftime(sBuf, sizeof(sBuf), "%Y-%m-%d %H:%M:%S", &stTm);
	return sBuf;
}

uint32_t datetoui(const char *pdate)
{
	struct tm t;
	memset(&t, 0, sizeof(t));
	if(sscanf(pdate, "%d-%d-%d %d:%d:%d",
		&t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
		return 0;

	if(t.tm_year == 0)
		return 0;

	t.tm_year -= 1900;
	t.tm_mon -= 1;
	return mktime(&t);
}

int MtReport_Plus_Hello(uint32_t dwTime)
{
    if(g_mtReport.iPluginIndex >= 0 && g_mtReport.iPluginIndex < MAX_INNER_PLUS_COUNT) {
        TInnerPlusInfo *plugin = g_mtReport.pMtShm->stPluginInfo + g_mtReport.iPluginIndex;
        if(plugin->iPluginId != 0) {
            if(plugin->bCheckRet)
                return 1;
            if(!dwTime)
                plugin->dwLastHelloTime = time(NULL);
            else
                plugin->dwLastHelloTime = dwTime;
            return 0;
        }
        return 2;
    }
    return 3;
}

int MtReport_Plus_Init(const char *pConfigFile, const char *pBuildVersion)
{
    TInnerPlusInfo stPluginInfo;
	int iRet = 0, i = 0;
	char szLocalLogFile[256] = {0};
	int32_t iLocalLogType = 0;
	char szTypeString[300] = {0};
	int32_t iCfgId = 0;
    int32_t iShmKey = 0;

	if(g_mtReport.cIsInit)
		return 1;
    if(!pBuildVersion || pBuildVersion[0] == '\0') {
        fprintf(stderr, "invalid pBuildVersion\n");
        return ERROR_LINE;
    }

	char szCheckVer[16] = {0};
    memset(&stPluginInfo, 0, sizeof(stPluginInfo));
	if((iRet=LoadConfig(pConfigFile,
        "XRK_PLUGIN_NAME", CFG_STRING, stPluginInfo.szPlusName, "", sizeof(stPluginInfo.szPlusName),
		"XRK_PLUGIN_CONFIG_PLAT", CFG_STRING, szCheckVer, "", sizeof(szCheckVer)-1,
        "XRK_PLUGIN_ID", CFG_INT, &(stPluginInfo.iPluginId), 0,
        "XRK_PLUGIN_CONFIG_FILE_VER", CFG_STRING, stPluginInfo.szVersion, "", sizeof(stPluginInfo.szVersion),
		"XRK_PLUGIN_CONFIG_ID", CFG_INT, &iCfgId, 0,
		"XRK_CONFIG_SHM_KEY", CFG_INT, &iShmKey, MT_REPORT_DEF_SHM_KEY,
		"XRK_LOCAL_LOG_TYPE", CFG_STRING, szTypeString, "", sizeof(szTypeString),
		"XRK_LOCAL_LOG_FILE", CFG_STRING, szLocalLogFile, "", sizeof(szLocalLogFile),
		(void*)NULL)) < 0)
	{
		fprintf(stderr, "loadconfig from:%s failed, msg:%s !\n", pConfigFile, strerror(errno));
		return ERROR_LINE;
	}

	if(strcmp(szCheckVer, "cloud")) {
		fprintf(stderr, "check config version failed, %s != cloud", szCheckVer);
		return ERROR_LINE;
	}

    if(stPluginInfo.szPlusName[0] == '\0' || stPluginInfo.iPluginId == 0 || stPluginInfo.szVersion[0] == '\0') {
        fprintf(stderr, "invalid config\n");
        return ERROR_LINE;
    }

	iLocalLogType = GetLogTypeByStr(szTypeString);
	if((iRet=MtReport_Init(iCfgId, szLocalLogFile, iLocalLogType, iShmKey)) < 0 || !g_mtReport.pMtShm)
	{
		fprintf(stderr, "MtReport_Init failed, ret:%d\n", iRet);
		return ERROR_LINE;
	}
    strncpy(stPluginInfo.szBuildVer, pBuildVersion, sizeof(stPluginInfo.szBuildVer));
    stPluginInfo.dwPluginStartTime = time(NULL);
    stPluginInfo.iLibVerNum = XRKMONITOR_LIB_VER_NUM;

    MTREPORT_SHM *pshm = g_mtReport.pMtShm;

    // 是否已存在
    for(i=0; i < MAX_INNER_PLUS_COUNT; i++) {
        if(pshm->stPluginInfo[i].iPluginId == stPluginInfo.iPluginId) {
            memcpy(pshm->stPluginInfo+i, &stPluginInfo, sizeof(stPluginInfo));
            break;
        }
    }

    if(i >= MAX_INNER_PLUS_COUNT) {
        if(pshm->iPluginInfoCount >= MAX_INNER_PLUS_COUNT) {
            fprintf(stderr, "plugin count over limit:%d\n", MAX_INNER_PLUS_COUNT);
            return ERROR_LINE;
        }
        // 获取 flag 如果失败，可能是有插件初始化时异常退出导致
        for(i=0; i < 20; i++) {
            if(VARMEM_CAS_GET(&(pshm->bAddPluginInfoFlag)))
                break;
            usleep(2000);
        }

        for(i=0; i < MAX_INNER_PLUS_COUNT; i++) {
            if(pshm->stPluginInfo[i].iPluginId == 0)
                break;
        }
        if(i < MAX_INNER_PLUS_COUNT) {
            memcpy(pshm->stPluginInfo+i, &stPluginInfo, sizeof(stPluginInfo));
            pshm->iPluginInfoCount++;
            VARMEM_CAS_FREE(pshm->bAddPluginInfoFlag);
        }
        else {
            fprintf(stderr, "need space to save plugin info\n");
            return ERROR_LINE;
        }
    }
    g_mtReport.iPluginIndex = i;
	return 0;
}

int MtReport_Init_ByKey(unsigned int iConfigId, int iConfigShmKey, int iFlag)
{
	if(g_mtReport.cIsInit)
		return 1;

	// attach shm
	int i = 0, iRet = 0, j = 0;
	MTREPORT_SHM *pShm = NULL;
	if((iRet=MtReport_GetShm((void**)(&pShm),
		iConfigShmKey, MYSIZEOF(MTREPORT_SHM), iFlag, MT_MTREPORT_SHM_CHECK_STR)) >= 0) 
	{
		g_mtReport.pMtShm = pShm;
		if(iRet == 1) { // 创建
			g_mtReport.pMtShm->iLogSpecialReadIdx = -1;
			for(j=0; j < MTLOG_SHM_DEF_COUNT; j++)
				g_mtReport.pMtShm->stLogShm[j].iLogStarIndex = -1;
		}
	}
	else  {
		fprintf(stderr, "attach shm failed , key:%d, ret:%d, size:%u\n", iConfigShmKey, iRet, MYSIZEOF(MTREPORT_SHM));
		return -1;
	}

	if(iConfigId != 0) {
		// 这里可能查找失败，agent 有可能没有同步完，写日志的时候会检查
		g_mtReport.stLogInfo.dwLogCfgId = iConfigId;
		for(i=0; iConfigId != 0 && i < pShm->wLogConfigCount; i++) {
			if(pShm->stLogConfig[i].dwCfgId == iConfigId) {
				g_mtReport.stLogInfo.pCurConfigInfo = pShm->stLogConfig+i;
				break;
			}
		}
	}

	// vmem 
	if(MtReport_InitVmem() < 0) {
		fprintf(stderr, "init vmem shm failed key:%d\n", iConfigShmKey);
		return -4;
	}

	// 属性上报
	if(MtReport_InitAttr() < 0) {
		fprintf(stderr, "init attr shm failed key:%d\n", iConfigShmKey);
		return -5;
	}

	g_mtReport.cIsInit = 1;
	return iRet;
}

// 第三方程序埋点时初始化库
int MtReport_Init(int iConfigId, const char *pLocalLogFile, int iLocalLogType, int iConfigShmKey)
{
	int iRet = 0;
	if(g_mtReport.cIsInit)
		return 0;

	if(iConfigShmKey == 0)
		iRet = MtReport_Init_ByKey(iConfigId, MT_REPORT_DEF_SHM_KEY, 0666);
	else
		iRet = MtReport_Init_ByKey(iConfigId, iConfigShmKey, 0666);

	if(pLocalLogFile != NULL && iLocalLogType != 0)
	{
		strncpy(g_mtReport.stLogInfo.szLocalLogFile, pLocalLogFile, sizeof(g_mtReport.stLogInfo.szLocalLogFile));
		g_mtReport.stLogInfo.iLocalLogType = iLocalLogType;
	}
	else
	{
		g_mtReport.stLogInfo.szLocalLogFile[0] = '\0';
		g_mtReport.stLogInfo.iLocalLogType = 0;
	}
	return iRet;
}

inline void MtReport_Log_SetCust1(uint32_t dwCust)
{
	g_mtReport.stLogInfo.stCust.dwCust_1 = dwCust;
	SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C1_SET);
}

inline void MtReport_Log_ClearCust1()
{
	CLEAR_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C1_SET);
}

inline void MtReport_Log_SetCust2(uint32_t dwCust)
{
	g_mtReport.stLogInfo.stCust.dwCust_2 = dwCust;
	SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C2_SET);
}

inline void MtReport_Log_ClearCust2()
{
	CLEAR_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C2_SET);
}

inline void MtReport_Log_SetCust3(int32_t iCust)
{
	g_mtReport.stLogInfo.stCust.iCust_3 = iCust;
	SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C3_SET);
}

inline void MtReport_Log_ClearCust3()
{
	CLEAR_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C3_SET);
}

inline void MtReport_Log_SetCust4(int32_t iCust)
{
	g_mtReport.stLogInfo.stCust.iCust_4 = iCust;
	SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C4_SET);
}

inline void MtReport_Log_ClearCust4()
{
	CLEAR_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C4_SET);
}

inline void MtReport_Log_SetCust5(const char *pstrCust)
{
	strncpy(g_mtReport.stLogInfo.stCust.szCust_5, 
		pstrCust, MYSIZEOF(g_mtReport.stLogInfo.stCust.szCust_5)-1);
	SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C5_SET);
}

inline void MtReport_Log_ClearCust5()
{
	CLEAR_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C5_SET);
}

inline void MtReport_Log_SetCust6(const char *pstrCust)
{
	strncpy(g_mtReport.stLogInfo.stCust.szCust_6, 
		pstrCust, MYSIZEOF(g_mtReport.stLogInfo.stCust.szCust_6)-1);
	SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C6_SET);
}

inline void MtReport_Log_ClearCust6()
{
	CLEAR_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C6_SET);
}

// 返回码说明
// -1 -- 日志接口没有初始化
// -2 -- 日志类型不需要 log
// -3 -- 日志被频率限制
// -5 -- 写日志失败
// 0 -- 写日志成功
static int MtReport_Log_To_Spec(int iLogType, const char *pszFmt, va_list ap) 
{
	g_mtReport.pMtShm->dwLogBySpecCount++;

	struct timeval stNow;
	gettimeofday(&stNow, 0);

	int32_t iWrite = 0;
	char sLogBuf[MTREPORT_LOG_MAX_LENGTH+64] = {0};
	iWrite = vsnprintf(sLogBuf+iWrite, MYSIZEOF(sLogBuf)-iWrite-32, pszFmt, ap);

	if(iWrite >= (int)(MYSIZEOF(sLogBuf)-iWrite-32)) {
		strcpy(sLogBuf+MYSIZEOF(sLogBuf)-32, "[slog too long truncate ...]");
		g_mtReport.pMtShm->wLogTruncate++;
	}
	iWrite = strlen(sLogBuf);

	int32_t iIndex = g_mtReport.pMtShm->iLogSpecialWriteIdx;
	if(iIndex+1 >= MTLOG_LOG_SPECIAL_COUNT) 
		g_mtReport.pMtShm->iLogSpecialWriteIdx = 0;
	else
		g_mtReport.pMtShm->iLogSpecialWriteIdx++;
	if(g_mtReport.pMtShm->iLogSpecialReadIdx < 0)
		g_mtReport.pMtShm->iLogSpecialReadIdx = iIndex;
	else if(g_mtReport.pMtShm->iLogSpecialReadIdx == iIndex)
		g_mtReport.pMtShm->wLogInShmFull++;

	uint32_t dwSeq = g_mtReport.pMtShm->dwLogSeq++;
	if(0 == dwSeq)
	{
		dwSeq = 1;
		g_mtReport.pMtShm->dwLogSeq = 2;
	}

	if(iIndex >= MTLOG_LOG_SPECIAL_COUNT)
		iIndex = 0;
	g_mtReport.pMtShm->sLogListSpec[iIndex].qwLogTime = stNow.tv_sec*1000000ULL+stNow.tv_usec;
	g_mtReport.pMtShm->sLogListSpec[iIndex].dwLogSeq = 0;

	g_mtReport.pMtShm->sLogListSpec[iIndex].wLogType = iLogType;
	g_mtReport.pMtShm->sLogListSpec[iIndex].dwLogConfigId = g_mtReport.stLogInfo.dwLogCfgId;

	if(g_mtReport.pMtShm->dwFirstLogWriteTime == 0)
		g_mtReport.pMtShm->dwFirstLogWriteTime = stNow.tv_sec;
	g_mtReport.pMtShm->dwWriteLogCount++;
	g_mtReport.pMtShm->qwWriteLogBytes+= iWrite;

	const int iShmLogBufLen = (int)MYSIZEOF(g_mtReport.pMtShm->sLogListSpec[iIndex].sLogContent);
	if(iWrite < iShmLogBufLen)
	{
		g_mtReport.pMtShm->sLogListSpec[iIndex].iVarmemIndex = -1;
		strcpy(g_mtReport.pMtShm->sLogListSpec[iIndex].sLogContent, sLogBuf);
	}
	else
	{
		memcpy(g_mtReport.pMtShm->sLogListSpec[iIndex].sLogContent, sLogBuf, iShmLogBufLen);

		// 4 : 用于存储校验，即 sLogContent 的最后 4 字节要等于 varmem 中的前面 4 字节
		iWrite = MtReport_SaveToVmem(sLogBuf+iShmLogBufLen-4, iWrite-iShmLogBufLen+4);
		if(iWrite <= 0)
		{
			g_mtReport.pMtShm->wLogVmemFailed++;
			snprintf(g_mtReport.pMtShm->sLogListSpec[iIndex].sLogContent, 
				iShmLogBufLen-1, "write shm log failed, SaveDataToVarmem ret:%d", iWrite);
			g_mtReport.pMtShm->sLogListSpec[iIndex].wLogType = MTLOG_TYPE_ERROR;
			g_mtReport.pMtShm->sLogListSpec[iIndex].iVarmemIndex = -1;
		}
		else {
			g_mtReport.pMtShm->wLogWriteInVmem++;
			g_mtReport.pMtShm->sLogListSpec[iIndex].iVarmemIndex = iWrite;
		}
	}
	g_mtReport.pMtShm->sLogListSpec[iIndex].dwLogSeq = dwSeq;
	return 0;
}

static int MtReport_Save_LogCust()
{
	char sCustBuf[256] = {0};
	int iCustUseLen = 0;
	sCustBuf[iCustUseLen] = (char)g_mtReport.stLogInfo.stCust.bCustFlag;
	iCustUseLen++;
	if(IS_SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C1_SET)) {
		*(uint32_t*)(sCustBuf+iCustUseLen) = htonl(g_mtReport.stLogInfo.stCust.dwCust_1);
		iCustUseLen+=MYSIZEOF(uint32_t);
	}

	if(IS_SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C2_SET)) {
		*(uint32_t*)(sCustBuf+iCustUseLen) = htonl(g_mtReport.stLogInfo.stCust.dwCust_2);
		iCustUseLen+=MYSIZEOF(uint32_t);
	}

	if(IS_SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C3_SET)) {
		*(uint32_t*)(sCustBuf+iCustUseLen) = htonl(g_mtReport.stLogInfo.stCust.iCust_3);
		iCustUseLen+=MYSIZEOF(uint32_t);
	}

	if(IS_SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C4_SET)) {
		*(uint32_t*)(sCustBuf+iCustUseLen) = htonl(g_mtReport.stLogInfo.stCust.iCust_4);
		iCustUseLen+=MYSIZEOF(uint32_t);
	}

	if(IS_SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C5_SET)) {
		strncpy(sCustBuf+iCustUseLen, g_mtReport.stLogInfo.stCust.szCust_5,
			MYSIZEOF(g_mtReport.stLogInfo.stCust.szCust_5)-1);
		iCustUseLen += strlen(sCustBuf+iCustUseLen)+1;
	}

	if(IS_SET_BIT(g_mtReport.stLogInfo.stCust.bCustFlag, MTLOG_CUST_FLAG_C6_SET)) {
		strncpy(sCustBuf+iCustUseLen, g_mtReport.stLogInfo.stCust.szCust_6,
			MYSIZEOF(g_mtReport.stLogInfo.stCust.szCust_6)-1);
		iCustUseLen += strlen(sCustBuf+iCustUseLen)+1;
	}

	int iRet = MtReport_SaveToVmem(sCustBuf, iCustUseLen);
	if(iRet <= 0)
		g_mtReport.pMtShm->wLogCustVmemFailed++;
	else
		g_mtReport.pMtShm->wLogCustInVmem++;
	return iRet;
}

static void MtReport_Log_To_Local(int iLogType, const char *pszFmt, va_list ap)
{
	struct timeval stNow;
	char sBuf[32];
	FILE *fp = NULL;
	struct tm stTm;

	char sTypeStr[32] = {0};
	switch(iLogType)
	{
		case MTLOG_TYPE_OTHER: 
			strcpy(sTypeStr, "Other");
			break;
		case MTLOG_TYPE_DEBUG:
			strcpy(sTypeStr, "Debug");
			break;
		case MTLOG_TYPE_INFO:
			strcpy(sTypeStr, "Info");
			break;
		case MTLOG_TYPE_WARN:
			strcpy(sTypeStr, "Warn");
			break;
		case MTLOG_TYPE_REQERR:
			strcpy(sTypeStr, "Reqerr");
			break;
		case MTLOG_TYPE_ERROR:
			strcpy(sTypeStr, "Error");
			break;
		case MTLOG_TYPE_FATAL:
			strcpy(sTypeStr, "Fatal");
			break;
		default:
			strcpy(sTypeStr, "Unknow");
			break;
	}

	gettimeofday(&stNow, 0);
	localtime_r(&stNow.tv_sec, &stTm);
	strftime(sBuf, MYSIZEOF(sBuf), "%Y-%m-%d %H:%M:%S", &stTm);

	if(!strcmp(g_mtReport.stLogInfo.szLocalLogFile, "stdout")) {
		printf("%s.%06u - %s - ", sBuf, (uint32_t)stNow.tv_usec, sTypeStr);
		vprintf(pszFmt, ap);
		printf("\n");
		return;
	}

	fp = fopen(g_mtReport.stLogInfo.szLocalLogFile, "a+");
	if(fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		int iCurSize = ftell(fp);
		if(iCurSize >= 1024*1024*50) {
			fclose(fp);
			fp = fopen(g_mtReport.stLogInfo.szLocalLogFile, "w+");
		}
		if(fp != NULL) {
			fprintf(fp, "%s.%06u - %s - ", sBuf, (uint32_t)stNow.tv_usec, sTypeStr);
			vfprintf(fp, pszFmt, ap);
			fprintf(fp, "\n");
			fclose(fp);
		}
	}
	else
	{
		fp = fopen("/tmp/mtreport_write_error.log", "w+");
		if(fp) {
			fprintf(fp, "%s.%06u - open file:%s failed, for log - %s - \n\t",
				sBuf, (uint32_t)stNow.tv_usec, g_mtReport.stLogInfo.szLocalLogFile, sTypeStr);
			vfprintf(fp, pszFmt, ap); 
			fprintf(fp, "\n");
			fclose(fp);
		}
	}
}

void MtReport_Check_Test(FunCheckTestCallBack isTest, const void *pdata)
{
	int i = 0;
	if(g_mtReport.stLogInfo.pCurConfigInfo == NULL) 
		return;
	for(; i < g_mtReport.stLogInfo.pCurConfigInfo->wTestKeyCount; i++)
	{
		if(isTest(g_mtReport.stLogInfo.pCurConfigInfo->stTestKeys[i].bKeyType,
			g_mtReport.stLogInfo.pCurConfigInfo->stTestKeys[i].szKeyValue, pdata))
		{
			g_mtReport.stLogInfo.cIsTest = 1;
			return;
		}
	}
	g_mtReport.stLogInfo.cIsTest = 0;
}

void MtReport_Clear_Test()
{
	g_mtReport.stLogInfo.cIsTest = 0;
}

// 返回码说明
// -1 -- 日志接口没有初始化
// -2 -- 日志类型不需要 log
// -3 -- 日志被频率限制
// -4 -- 写日志多线程竞争资源失败
// -5 -- 写日志失败
// 0 -- 写日志成功
int MtReport_Log(int iLogType, const char *pszFmt, ...)
{
	int i=0, iRet=0, j=0;
	va_list ap;

	if(!g_mtReport.cIsInit)
		return -1;

	if('\0' != g_mtReport.stLogInfo.szLocalLogFile[0] 
		&& (iLogType & g_mtReport.stLogInfo.iLocalLogType))
	{
		va_start(ap, pszFmt);
		MtReport_Log_To_Local(iLogType, pszFmt, ap);
		va_end(ap);
	}

	// config id 为0， 不上报日志
	if(g_mtReport.stLogInfo.dwLogCfgId == 0 || g_mtReport.pMtShm == NULL)
		return 0;

	if(g_mtReport.stLogInfo.pCurConfigInfo == NULL) {
		if(g_mtReport.pMtShm != NULL)
		{
			for(i=0; i < g_mtReport.pMtShm->wLogConfigCount; i++) {
				if(g_mtReport.pMtShm->stLogConfig[i].dwCfgId 
					== g_mtReport.stLogInfo.dwLogCfgId) 
				{
					g_mtReport.stLogInfo.pCurConfigInfo = g_mtReport.pMtShm->stLogConfig+i;
					break;
				}
			}
		}

		if(g_mtReport.stLogInfo.pCurConfigInfo == NULL) {
			if(g_mtReport.pMtShm != NULL)
			{
				va_start(ap, pszFmt);
				iRet = MtReport_Log_To_Spec(iLogType, pszFmt, ap);
				va_end(ap);
			}
			return iRet;
		}
	}

	SLogConfig *pCurConfigInfo = g_mtReport.stLogInfo.pCurConfigInfo;

	// 染色标志如果设置了，则不检查日志类型
	if(!g_mtReport.stLogInfo.cIsTest && !(pCurConfigInfo->iLogType & iLogType))
	{
		g_mtReport.pMtShm->dwLogTypeLimited++;
		return -2;
	}

	struct timeval stNow;
	gettimeofday(&stNow, 0);

	if(g_mtReport.iPluginIndex >= 0 && g_mtReport.iPluginIndex < MAX_INNER_PLUS_COUNT) {
        TInnerPlusInfo *plugin = g_mtReport.pMtShm->stPluginInfo + g_mtReport.iPluginIndex;
        if(plugin->iPluginId != 0) {
            plugin->dwLastReportLogTime = stNow.tv_sec;
        }
    }

	// 日志频率限制
	if(pCurConfigInfo->dwSpeedFreq != 0) 
	{
		uint64_t qwTimeMs = stNow.tv_sec*1000 + stNow.tv_usec/1000;

		// 按1分钟计时频率
		if(pCurConfigInfo->qwLastFreqTime+60000 <= qwTimeMs)
		{
			if(VARMEM_CAS_GET(&(pCurConfigInfo->bLogFreqUseFlag))) {
				pCurConfigInfo->qwLastFreqTime = qwTimeMs;
				pCurConfigInfo->iWriteLogCount = 0;
				VARMEM_CAS_FREE(pCurConfigInfo->bLogFreqUseFlag);
			}
		}
		if(pCurConfigInfo->iWriteLogCount > (int)pCurConfigInfo->dwSpeedFreq)
		{
			g_mtReport.pMtShm->wLogFreqLimited++;
			return -3;
		}
	}
	pCurConfigInfo->iWriteLogCount++;

	int32_t iWrite = 0;
	char sLogBuf[MTREPORT_LOG_MAX_LENGTH+64] = {0};
	va_start(ap, pszFmt);
	iWrite = vsnprintf(sLogBuf+iWrite, MYSIZEOF(sLogBuf)-iWrite-32, pszFmt, ap);
	va_end(ap);

	if(iWrite >= (int)(MYSIZEOF(sLogBuf)-iWrite-32)){
		g_mtReport.pMtShm->wLogTruncate++;
		strcpy(sLogBuf+MYSIZEOF(sLogBuf)-32, "[slog too long truncate ...]");
	}
	iWrite = strlen(sLogBuf);

	// 写入共享内存中
	MTLogShm *pShmLog = NULL;
	for(j=0; j < 10; j++) {
		for(i=0; i < MTLOG_SHM_DEF_COUNT; i++) {
			if(VARMEM_CAS_GET(&g_mtReport.pMtShm->stLogShm[i].bTryGetLogIndex)
					|| stNow.tv_sec > g_mtReport.pMtShm->stLogShm[i].dwGetLogIndexStartTime+5) {
				pShmLog = g_mtReport.pMtShm->stLogShm+i;
				g_mtReport.pMtShm->stLogShm[i].dwGetLogIndexStartTime = stNow.tv_sec;
				break;
			}
		}
		if(i < MTLOG_SHM_DEF_COUNT)
		    break;
		usleep(1000);
	}

	if(pShmLog == NULL) {
		va_start(ap, pszFmt);
		iRet = MtReport_Log_To_Spec(iLogType, pszFmt, ap);
		va_end(ap);
		return iRet;
	}

	int32_t iIndex = pShmLog->iWriteIndex;
	if(iIndex+1 >= MTLOG_SHM_RECORDS_COUNT)
		pShmLog->iWriteIndex = 0;
	else
		pShmLog->iWriteIndex++;
	if(pShmLog->iLogStarIndex < 0)
		pShmLog->iLogStarIndex = iIndex;
	else if(iIndex == pShmLog->iLogStarIndex)
		g_mtReport.pMtShm->wLogInShmFull++;

	pShmLog->sLogList[iIndex].qwLogTime = stNow.tv_sec*1000000ULL+stNow.tv_usec;
	pShmLog->sLogList[iIndex].dwLogSeq = 0;
	VARMEM_CAS_FREE(pShmLog->bTryGetLogIndex);

	uint32_t dwSeq = g_mtReport.pMtShm->dwLogSeq++;
	if(0 == dwSeq)
	{
		dwSeq = 1;
		g_mtReport.pMtShm->dwLogSeq = 2;
	}

	pShmLog->sLogList[iIndex].iAppId = pCurConfigInfo->iAppId;
	pShmLog->sLogList[iIndex].iModuleId = pCurConfigInfo->iModuleId;
	pShmLog->sLogList[iIndex].wLogType = iLogType;
	pShmLog->sLogList[iIndex].dwLogConfigId = pCurConfigInfo->dwCfgId;

	if(g_mtReport.stLogInfo.stCust.bCustFlag != 0) 
		pShmLog->sLogList[iIndex].iCustVmemIndex = MtReport_Save_LogCust(); 
	else 
		pShmLog->sLogList[iIndex].iCustVmemIndex = 0;

	if(g_mtReport.pMtShm->dwFirstLogWriteTime == 0)
		g_mtReport.pMtShm->dwFirstLogWriteTime = stNow.tv_sec;
	g_mtReport.pMtShm->dwWriteLogCount++;
	g_mtReport.pMtShm->qwWriteLogBytes+= iWrite;

	const int iShmLogBufLen = (int)MYSIZEOF(pShmLog->sLogList[iIndex].sLogContent);
	if(iWrite < iShmLogBufLen)
	{
		pShmLog->sLogList[iIndex].iVarmemIndex = -1;
		strcpy(pShmLog->sLogList[iIndex].sLogContent, sLogBuf);
	}
	else
	{
		memcpy(pShmLog->sLogList[iIndex].sLogContent, sLogBuf, iShmLogBufLen);

		// 4 : 用于存储校验，即 sLogContent 的最后 4 字节要等于 varmem 中的前面 4 字节
		iWrite = MtReport_SaveToVmem(sLogBuf+iShmLogBufLen-4, iWrite-iShmLogBufLen+4);
		if(iWrite <= 0)
		{
			g_mtReport.pMtShm->wLogVmemFailed++;
			snprintf(pShmLog->sLogList[iIndex].sLogContent, 
				iShmLogBufLen-1, "write shm log failed, SaveDataToVarmem ret:%d", iWrite);
			pShmLog->sLogList[iIndex].wLogType = MTLOG_TYPE_ERROR;
			pShmLog->sLogList[iIndex].iVarmemIndex = -1;
		}
		else {
			pShmLog->sLogList[iIndex].iVarmemIndex = iWrite;
			g_mtReport.pMtShm->wLogWriteInVmem++;
		}
	}
	pShmLog->sLogList[iIndex].dwLogSeq = dwSeq;
	return 0;
}


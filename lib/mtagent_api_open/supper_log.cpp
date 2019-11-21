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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <assert.h>
#include "supper_log.h"
#include "Utility.h"
#include "Memcache.h"
#include "sv_vmem.h"
#include "mt_report.h"

#define SLOG_NEXT_INDEX(index, next) do { if(index+1 >= m_pShmLog->iLogMaxCount) next=0; \
	else next = index+1; }while(0)

#define TOO_LONG_TRUNC_STR " [log too long truncate, see history log !]"
#define TOO_LONG_TRUNC_STR_LEN 64

extern "C" {
	int MtReport_Init_ByKey(unsigned int iConfigId, int iConfigShmKey, int iFlag);
}

// class CLogTimeCur 
// ------------------------------------------------------------------------------------------------------
CLogTimeCur::CLogTimeCur()
{
	struct timeval stNow;
	gettimeofday(&stNow, 0);
	m_dwTimeSec = stNow.tv_sec;
	m_dwTimeUsec = stNow.tv_usec;
	m_qwTime = TIME_SEC_TO_USEC(m_dwTimeSec) + m_dwTimeUsec;
}


// class CSLogSearch 
// ------------------------------------------------------------------------------------------------------

CSLogSearch::CSLogSearch()
{
	m_qwRealTimeUsec = (uint64_t)(SHOW_REALTIME_LOG_TIME_SEC*1000000ULL);
	m_pShmLog = NULL;
	m_pstLogFileList = NULL;
	InitDefaultSearch();
	m_bInit = false;
}

int CSLogSearch::Init()
{
	int iRet = 0;
	if(slog.m_iVmemShmKey > 0) 
		iRet = MtReport_InitVmem_ByFlag(0666|IPC_CREAT, slog.m_iVmemShmKey);

	if(iRet < 0)
		m_bInit = false;
	else
	{
		m_bInit = true;
		return 0;
	}
	return SLOG_ERROR_LINE;
}

void CSLogSearch::InitDefaultSearch()
{
	m_iLogTypeFlag = 0; 
	m_iLogAppId = 0;
	m_iModuleIdCount = 0;
	memset(m_arrLogModuleId, 0, MYSIZEOF(m_arrLogModuleId));
	m_iIncKeyCount = 0;
	memset(m_stIncKey, 0, MYSIZEOF(m_stIncKey));
	memset(m_stIncJmpTab, 0, MYSIZEOF(m_stIncJmpTab));
	m_iExcpKeyCount = 0;
	memset(m_stExcpKey, 0, MYSIZEOF(m_stExcpKey));
	memset(m_stExcpJmpTab, 0, MYSIZEOF(m_stExcpJmpTab));
	m_qwTimeStart = 0;
	m_qwTimeEnd = 0;
	m_wFileNo = 0;
	m_dwFilePos = 0;
	m_wFileCount = 0;
	m_iLogField = 0;
	m_wFileIndexStar = 0;
	m_iReportMachine = 0;
	strncpy(m_sLogPath, DEF_SLOG_LOG_FILE_PATH, sizeof(m_sLogPath)-1);
	m_strLogFile.clear();
}

char * GetShmLog(TSLog *pLog, int32_t iLogIndex)
{
	static char sLogBuf[BWORLD_SLOG_MAX_LINE_LEN+TOO_LONG_TRUNC_STR_LEN];

	if(pLog->iContentIndex < 0)
		strncpy(sLogBuf, pLog->sLogContent, sizeof(sLogBuf)-1);
	else
	{
		// 日志存在变长内存中
		memcpy(sLogBuf, pLog->sLogContent, BWORLD_MEMLOG_BUF_LENGTH-4);
		int iLen = MYSIZEOF(sLogBuf)-BWORLD_MEMLOG_BUF_LENGTH+4;
		int iRet = MtReport_GetFromVmem(pLog->iContentIndex, sLogBuf+BWORLD_MEMLOG_BUF_LENGTH-4, &iLen);
		if(iRet < 0)
		{
			snprintf(sLogBuf, sizeof(sLogBuf), "bug: MtReport_GetFromVmem ret:%d logtime:%" PRIu64 
				" index:%d appid:%d module:%d loglevel:%d [some log content :%s]", iRet,
				pLog->qwLogTime, iLogIndex, pLog->iAppId, pLog->iModuleId, pLog->wLogType, pLog->sLogContent);
			MtReport_Attr_Add(234, 1);
			return sLogBuf;
		}

		// fix bug '\0' 结束符
		sLogBuf[BWORLD_MEMLOG_BUF_LENGTH-4+iLen] = '\0';

		// 4 字节校验
		uint32_t dwCheck = *(uint32_t*)(pLog->sLogContent+BWORLD_MEMLOG_BUF_LENGTH-4);
		if(dwCheck != *(uint32_t*)(sLogBuf+BWORLD_MEMLOG_BUF_LENGTH-4))
		{
			pLog->sLogContent[BWORLD_MEMLOG_BUF_LENGTH-5] = '\0';
			snprintf(sLogBuf, sizeof(sLogBuf), "bug: check num:%u != %u logtime:%" PRIu64 
				" index:%d appid:%d module:%d loglevel:%d [some log content :%s]", dwCheck, 
				*(uint32_t*)(sLogBuf+BWORLD_MEMLOG_BUF_LENGTH-4), pLog->qwLogTime,
				iLogIndex, pLog->iAppId, pLog->iModuleId, pLog->wLogType, pLog->sLogContent);
			MtReport_Attr_Add(235, 1);
		}
	}
	return sLogBuf;
}

int CSLogSearch::SetAppId(int iAppId)
{
	m_iLogAppId = iAppId;
	if(m_pShmLog != NULL) { 
		if(m_pShmLog->iAppId != iAppId)
		{
			shmdt(m_pShmLog);
			m_pShmLog = NULL;
		}
	}

	bool bTryCreateShm = false;
	AppInfo *pAppShmInfo = NULL;
	if(NULL == m_pShmLog) {
		pAppShmInfo = slog.GetAppInfo(iAppId);
		if(pAppShmInfo == NULL) {
			ERR_LOG("get appinfo failed, appid:%d", iAppId);
			return SLOG_ERROR_LINE;
		}

		m_pShmLog = slog.GetAppLogShm(pAppShmInfo, bTryCreateShm);
		if(m_pShmLog == NULL)
		{
			// 日志查询，检查下分发是否正确, 分发正确的情况下，创建并初始化日志相关共享内存
			SLogConfig *pcfg = slog.GetSlogConfig();
			MachineInfo *pmach = NULL;
			if(pcfg != NULL) {
				pmach = slog.GetMachineInfo(pcfg->stSysCfg.iMachineId, NULL);
				if(pmach != NULL && slog.IsIpMatchMachine(pmach, pAppShmInfo->dwAppSrvMaster) == 1) 
				{
					// 警告日志，这里应该跑不到，slog_write 程序会预先创建 app 相关的共享内存
					WARN_LOG("query app:%d log, try create app shm", pAppShmInfo->iAppId);
					bTryCreateShm = true;
					m_pShmLog = slog.GetAppLogShm(pAppShmInfo, bTryCreateShm); 
				}
			}
			
			if(m_pShmLog == NULL) {
				ERR_LOG("get app:%d log shm failed(%p, %p)", pAppShmInfo->iAppId, pcfg, pmach);
				return SLOG_ERROR_LINE;
			}
		}
	}

	if(m_pstLogFileList != NULL) {
		if(m_pstLogFileList->iAppId != iAppId)
		{
			shmdt(m_pstLogFileList);
			m_pstLogFileList = NULL;
		}
	}

	if(NULL == m_pstLogFileList) 
	{
		if(NULL == pAppShmInfo) {
			pAppShmInfo = slog.GetAppInfo(iAppId);
			if(pAppShmInfo == NULL) {
				ERR_LOG("get appinfo failed, appid:%d", iAppId);
				return SLOG_ERROR_LINE;
			}
		}
		m_pstLogFileList = CSLogServerWriteFile::GetAppLogFileShm(pAppShmInfo, bTryCreateShm);

		if(m_pstLogFileList == NULL)
		{
			ERR_LOG("get app:%d log file shm failed", pAppShmInfo->iAppId);
			return SLOG_ERROR_LINE;
		}

		if(bTryCreateShm)
			CSLogServerWriteFile logFile(pAppShmInfo, m_sLogPath, 0);

		// 预先读取文件头部
		for(int i=0; i < m_pstLogFileList->wLogFileCount; i++)
		{
			SLogFileHead &stFileHead = m_pstLogFileList->stFiles[i].stFileHead;

			// 文件头部还未读取
			if(stFileHead.iLogRecordsWrite <= 0)
			{
				FILE *fp = fopen(m_pstLogFileList->stFiles[i].szAbsFileName, "rb");
				if(NULL == fp)
				{
					ERR_LOG("open file:%s failed , msg:%s", 
						m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
					break;
				}
				if(fread(&stFileHead, MYSIZEOF(stFileHead), 1, fp) != 1)
				{
					ERR_LOG("read slog file head from file:%s failed, msg:%s !", 
						m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
					fclose(fp);
					continue;
				}
				fclose(fp);

				// 警告日志，这里应该跑不到，slog_write 程序会预先读取
				WARN_LOG("log search read slog file head, app:%d, file:%s",
					iAppId, m_pstLogFileList->stFiles[i].szAbsFileName);
			}
		}
	}
	return iAppId;
}

int CSLogSearch::SetModuleId(int iModuleId)
{
	if(m_iModuleIdCount+1 > SLOG_MODULE_COUNT_MAX_PER_APP)
	{
		WARN_LOG("invalid module id:%d , max:%d", iModuleId, SLOG_MODULE_COUNT_MAX_PER_APP);
		return -1;
	}
	m_arrLogModuleId[m_iModuleIdCount++] = iModuleId;
	return m_iModuleIdCount;
}

int CSLogSearch::SetAppModuleInfo(int iAppId, int iModuleIdCount, int *piModuleId)
{
	m_iLogAppId = iAppId;
	if(iModuleIdCount < 0)
		m_iModuleIdCount = 0;
	else if(m_iModuleIdCount > SLOG_MODULE_COUNT_MAX_PER_APP)
		m_iModuleIdCount = SLOG_MODULE_COUNT_MAX_PER_APP;
	else
		m_iModuleIdCount = iModuleIdCount; 
	for(int i=0; i < m_iModuleIdCount; i++)
		m_arrLogModuleId[i] = piModuleId[i];
	return m_iModuleIdCount;
}

void CSLogSearch::SetLogField(int iLogField)
{
	m_iLogField = iLogField;
}

void CSLogSearch::AddLogType(int iLogType)
{
	m_iLogTypeFlag |= iLogType;
}

void CSLogSearch::SetLogType(uint32_t dwLogTypeFlag)
{
	m_iLogTypeFlag = dwLogTypeFlag;
}

int CSLogSearch::AddIncludeKey(const char *pszKey)
{
	if(m_iIncKeyCount+1 > SLOG_MAX_KEY_COUNT)
	{
		WARN_LOG("logsearch too much search inkey :%d, max:%d", m_iIncKeyCount, SLOG_MAX_KEY_COUNT);
		return -1;
	}
	int i = m_iIncKeyCount;
	strncpy(m_stIncKey[i], pszKey, SLOG_KEY_MAX_LENGTH-1);
	bmh_get_jump_table(m_stIncKey[i], m_stIncJmpTab[i]);
	m_iIncKeyCount++;
	return m_iIncKeyCount;
}

int CSLogSearch::SetSearchIncKey(int iIncKeyCount, char (*sIncKeyList)[SLOG_KEY_MAX_LENGTH])
{
	if(iIncKeyCount > SLOG_MAX_KEY_COUNT)
		iIncKeyCount = SLOG_MAX_KEY_COUNT;

	int i = 0;
	memset(&m_stIncKey, 0, MYSIZEOF(m_stIncKey));
	for(; i < iIncKeyCount; i++)
	{
		strncpy(m_stIncKey[i], sIncKeyList[i], SLOG_KEY_MAX_LENGTH-1);
		bmh_get_jump_table(m_stIncKey[i], m_stIncJmpTab[i]);
	}
	m_iIncKeyCount = iIncKeyCount;
	return 0;
}

int CSLogSearch::AddExceptKey(const char *pszKey)
{
	if(m_iExcpKeyCount+1 > SLOG_MAX_KEY_COUNT)
	{
		WARN_LOG("logsearch too much except inkey :%d, max:%d", m_iExcpKeyCount, SLOG_MAX_KEY_COUNT);
		return -1;
	}
	int i = m_iExcpKeyCount;
	strncpy(m_stExcpKey[i], pszKey, SLOG_KEY_MAX_LENGTH-1);
	bmh_get_jump_table(m_stExcpKey[i], m_stExcpJmpTab[i]);
	m_iExcpKeyCount++;
	return m_iExcpKeyCount;
}

int CSLogSearch::SetSearchExcpKey(int iExcpKeyCount, char (*sExcpKeyList)[SLOG_KEY_MAX_LENGTH])
{
	if(iExcpKeyCount > SLOG_MAX_KEY_COUNT)
		iExcpKeyCount = SLOG_MAX_KEY_COUNT;

	int i = 0;
	for(i=0; i < iExcpKeyCount; i++)
	{
		strncpy(m_stExcpKey[i], sExcpKeyList[i], SLOG_KEY_MAX_LENGTH-1);
		bmh_get_jump_table(m_stExcpKey[i], m_stExcpJmpTab[i]);
	}
	m_iExcpKeyCount = iExcpKeyCount;
	return 0;
}

bool CSLogSearch::IsLogMatch(TSLog *pstLog, const char *pszLog)
{
	int i;

	if(pszLog == NULL)
	{
		// app module 是否匹配 ----
		if(m_iLogAppId != 0 && m_iLogAppId != pstLog->iAppId)
			return false;
		if(m_iModuleIdCount != 0)
		{
			for(i=0; i < m_iModuleIdCount; i++)
			{
				if(pstLog->iModuleId == m_arrLogModuleId[i])
					break;
			}
			if(i >= m_iModuleIdCount)
				return false;
		}

		// log 级别是否匹配 ----
		if(!(pstLog->wLogType & m_iLogTypeFlag))
			return false;
		
		// 上报机器是否匹配 ----
		if(m_iReportMachine != 0 && m_iReportMachine != (int)pstLog->dwLogHost)
			return false;
	}
	else
	{
		const char *phost = ipv4_addr_str(pstLog->dwLogHost);

		// 排除关键字优先 ------------
		for(i=0; i < m_iExcpKeyCount; i++)
		{
			if(strstr(phost, m_stExcpKey[i]))
				return false;
			if(bmh_is_match(pszLog, m_stExcpKey[i], m_stExcpJmpTab[i]) >= 0)
				return false;
		}

		// 包含关键字 [与]
		for(i=0; i < m_iIncKeyCount; i++)
		{
			if(strstr(phost, m_stIncKey[i]))	
				continue;
			if(bmh_is_match(pszLog, m_stIncKey[i], m_stIncJmpTab[i]) >= 0)
				continue;
			else
				return false;
		}
	}
	return true;
}

bool CSLogSearch::IsSearchHistoryComplete()
{
	// fileno, filepos 都是从 0 开始
	if(m_wFileCount <= 0 || (m_wFileNo+1 >= m_wFileCount && m_dwFilePos+1 >= m_dwCurFileHasRecords))
		return true;
	return false;
}

int CSLogSearch::GetSearchCurFilePercent()
{
	if(m_wFileCount <= 0)
		return 100;

	int iPer = m_dwFilePos*100 / m_dwCurFileHasRecords;
	if(iPer <= 0)
		return 1;
	return iPer;
}

TSLogOut * CSLogSearch::HistoryLog()
{
	static TSLogOut stLogOut;
	static char sLogBuf[BWORLD_SLOG_MAX_LINE_LEN+64];
	int i = 0, j = 0, k = 0;
	FILE *fp = NULL;

	if(!m_bInit || NULL==m_pstLogFileList)
	{
		FATAL_LOG("invalid, binit:%d, plogFileShm:%p", m_bInit, m_pstLogFileList);
		return NULL;
	}

	if(m_wFileCount <= 0) // 页面点击显示历史记录
	{
		DEBUG_LOG("total :%d slog files ------- ", m_pstLogFileList->wLogFileCount);
		m_wFileIndexStar = SLOG_LOG_FILES_COUNT_MAX;
		for(i=0; i < m_pstLogFileList->wLogFileCount; i++)
		{
			SLogFileHead &stFileHead = m_pstLogFileList->stFiles[i].stFileHead;

			// 文件头部还未读取
			if(stFileHead.iLogRecordsWrite <= 0)
			{
				fp = fopen(m_pstLogFileList->stFiles[i].szAbsFileName, "rb");
				if(NULL == fp)
				{
					ERR_LOG("open file:%s failed , msg:%s", 
						m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
					break;
				}
				if(fread(&stFileHead, MYSIZEOF(stFileHead), 1, fp) != 1)
				{
					ERR_LOG("read slog file head from file:%s failed, msg:%s !", 
						m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
					fclose(fp);
					continue;
				}
				fclose(fp);
			}

			// 指定文件查找
			if(m_strLogFile.size() > 0
				&& strstr(m_pstLogFileList->stFiles[i].szAbsFileName, m_strLogFile.c_str()))
			{
				m_wFileIndexStar = i;
				m_wFileNo = 0;
				m_dwFilePos = 0;
				m_wFileCount = 1;
				DEBUG_LOG("history log custom file - %s records:%d",
					m_strLogFile.c_str(), m_pstLogFileList->stFiles[i].stFileHead.iLogRecordsWrite);
				break;
			}
			else if(m_strLogFile.size() > 0)
				continue;

			DEBUG_LOG("%s records:%d time:%" PRIu64 "-%" PRIu64 "",
					m_pstLogFileList->stFiles[i].szAbsFileName, stFileHead.iLogRecordsWrite,
					stFileHead.qwLogTimeStart, stFileHead.qwLogTimeEnd);

			// 找符合时间间隔的起始
			if(m_wFileIndexStar >= SLOG_LOG_FILES_COUNT_MAX)
			{
				// end time 大于起始时间，则有符合时间的日志落在了这个日志文件
				if(stFileHead.qwLogTimeEnd >= m_qwTimeStart)
				{
					m_wFileIndexStar = i;
					m_wFileNo = 0;
					m_dwFilePos = 0;
					DEBUG_LOG("get history log, find start file index:%d, file name:%s", 
						i, m_pstLogFileList->stFiles[i].szAbsFileName);
				}
				else
					continue;
			}

			// start time 大于结束时间，说明没有符合查询时间的日志落在这个日志文件
			if(stFileHead.qwLogTimeStart > m_qwTimeEnd)
				break;
			m_wFileCount++;
		}
	}

	TSLog stLog;
	memset(&stLog, 0, MYSIZEOF(TSLog));
	m_dwCurFileHasRecords = 0;
	for(i=m_wFileIndexStar+m_wFileNo, k=m_wFileNo; i < SLOG_LOG_FILES_COUNT_MAX; i++, k++)
	{
		fp = fopen(m_pstLogFileList->stFiles[i].szAbsFileName, "rb");
		if(NULL == fp)
		{
			ERR_LOG("open file:%s failed , msg:%s", 
				m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
			break;
		}

		SLogFileHead &stFileHead = m_pstLogFileList->stFiles[i].stFileHead;
		SLogFileLogIndex stIndex;
		m_dwCurFileHasRecords = stFileHead.iLogRecordsWrite;
		for(j=m_dwFilePos; j < stFileHead.iLogRecordsWrite; j++)
		{
			if(fseek(fp, MYSIZEOF(SLogFileHead)+MYSIZEOF(stIndex)*j, SEEK_SET) < 0)
			{
				ERR_LOG("seek record:%d index failed msg:%s file:%s", 
					j, strerror(errno), m_pstLogFileList->stFiles[i].szAbsFileName);
				break;
			}
			if(fread(&stIndex, MYSIZEOF(SLogFileLogIndex), 1, fp) != 1)
			{
				ERR_LOG("read slog file index from file:%s failed, msg:%s !", 
					m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
				break;
			}

			// 检查时间, 注意指定文件查找 ----
			if(m_strLogFile.size() <= 0 && 
				(stIndex.qwLogTime < m_qwTimeStart || stIndex.qwLogTime > m_qwTimeEnd))
			{
				if(j < stFileHead.iLogRecordsWrite)
					m_dwFilePos = j+1;
				continue;
			}

			stLog.iAppId = stIndex.iAppId;
			stLog.iModuleId = stIndex.iModuleId;
			stLog.wLogType = stIndex.wLogType;
			stLog.dwLogHost = stIndex.dwLogHost;
			if(j < stFileHead.iLogRecordsWrite)
				m_dwFilePos = j+1;

			if(!IsLogMatch(&stLog, NULL))
				continue;

			if(fseek(fp, stIndex.dwLogContentPos, SEEK_SET) < 0)
			{
				ERR_LOG("seek record:%d content failed msg:%s file:%s", 
					j, strerror(errno), m_pstLogFileList->stFiles[i].szAbsFileName);
				break;
			}

			// 日志记录的 seq 校验
			uint32_t dwContentSeq = 0;
			if(fread(&dwContentSeq, 4, 1, fp) != 1)
			{
				ERR_LOG("read content seq failed from file:%s log pos:%d, msg:%s !",
					m_pstLogFileList->stFiles[i].szAbsFileName, j, strerror(errno));
				break;
			}
			if(dwContentSeq != stIndex.dwLogSeq)
			{
				ERR_LOG("from slog file:%s content seq:%u != %u check content failed !", 
					m_pstLogFileList->stFiles[i].szAbsFileName, dwContentSeq, stIndex.dwLogSeq);
				break;
			}
			
			if(fread(sLogBuf, 1, stIndex.dwLogContentLen, fp) != stIndex.dwLogContentLen)
			{
				ERR_LOG("read slog log contenet from file:%s failed, msg:%s !", 
					m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
				break;
			}
			if(IsLogMatch(&stLog, sLogBuf))
			{
				stLogOut.iAppId = stIndex.iAppId; 
				stLogOut.iModuleId = stIndex.iModuleId; 
				stLogOut.wLogType = stIndex.wLogType; 
				stLogOut.dwCust_1 = stIndex.dwCust_1;
				stLogOut.dwCust_2 = stIndex.dwCust_2;
				stLogOut.iCust_3 = stIndex.iCust_3;
				stLogOut.iCust_4 = stIndex.iCust_4;
				if(stIndex.szCust_5[0] != '\0') 
					memcpy(stLogOut.szCust_5, stIndex.szCust_5, sizeof(stLogOut.szCust_5));
				if(stIndex.szCust_6[0] != '\0') 
					memcpy(stLogOut.szCust_6, stIndex.szCust_6, sizeof(stLogOut.szCust_6));
				stLogOut.dwLogConfigId = stIndex.dwLogConfigId;
				stLogOut.dwLogHost = stIndex.dwLogHost;
				stLogOut.dwLogSeq = stIndex.dwLogSeq;
				stLogOut.qwLogTime = stIndex.qwLogTime;
				stLogOut.pszLog = sLogBuf;

				fclose(fp);
				return &stLogOut; 
			}
		}

		DEBUG_LOG("scan file:%s records:%u complete, fileno:%d, filepos:%u (i:%d, j:%d, k:%d)",
			m_pstLogFileList->stFiles[i].szAbsFileName, m_dwCurFileHasRecords, m_wFileNo, m_dwFilePos, i, j, k);

		fclose(fp);

		if(k+1 >= m_wFileCount) // 文件已经扫描完成
			break;

		m_wFileNo++; // 下一个文件索引
		m_dwFilePos = 0; // 下一个文件的日志记录起始索引
	}
	return NULL;
}

TSLogOut * CSLogSearch::RealTimeLog(int32_t & iLastLogIndex, uint32_t seq, uint64_t qwTimeNow)
{
	static TSLogOut stLogOut;
	if(m_pShmLog == NULL)
		return NULL;

	int32_t j = iLastLogIndex;

	if(!m_bInit || NULL==m_pShmLog)
		return NULL;

	// 先取最新的日志
	if(j < 0)
		j = m_pShmLog->iWriteIndex;

	int iCheckLoop = 0;
	char *pszLog = NULL;

	do{
		iCheckLoop++;

		// 显示最近 m_qwRealTimeUsec 时间内产生的log 
		// seq 为上次取的 seq, 首次时为 0
		j = (j <= 0 ? m_pShmLog->iLogMaxCount-1 : j-1);
		if(m_pShmLog->sLogList[j].qwLogTime+m_qwRealTimeUsec < qwTimeNow 
		  || m_pShmLog->sLogList[j].dwLogSeq == 0
		  || (IS_SEQ_BIGER(seq, m_pShmLog->sLogList[j].dwLogSeq) && seq != 0))
			break;

		if(IsLogMatch(m_pShmLog->sLogList+j, NULL)) {
			pszLog = GetShmLog(m_pShmLog->sLogList+j, j);
			if(pszLog != NULL && IsLogMatch(m_pShmLog->sLogList+j, pszLog)) {
				stLogOut.iAppId = m_pShmLog->sLogList[j].iAppId;
				stLogOut.iModuleId = m_pShmLog->sLogList[j].iModuleId;
				stLogOut.wLogType = m_pShmLog->sLogList[j].wLogType;
				stLogOut.pszLog = pszLog;
				stLogOut.dwLogSeq = m_pShmLog->sLogList[j].dwLogSeq;
				stLogOut.qwLogTime = m_pShmLog->sLogList[j].qwLogTime;
				stLogOut.dwLogConfigId = m_pShmLog->sLogList[j].dwLogConfigId;
				stLogOut.dwLogHost = m_pShmLog->sLogList[j].dwLogHost;
				stLogOut.dwCust_1 = m_pShmLog->sLogList[j].dwCust_1;
				stLogOut.dwCust_2 = m_pShmLog->sLogList[j].dwCust_2;
				stLogOut.iCust_3 = m_pShmLog->sLogList[j].iCust_3;
				stLogOut.iCust_4 = m_pShmLog->sLogList[j].iCust_4;
				memcpy(stLogOut.szCust_5, m_pShmLog->sLogList[j].szCust_5, sizeof(stLogOut.szCust_5));
				memcpy(stLogOut.szCust_6, m_pShmLog->sLogList[j].szCust_6, sizeof(stLogOut.szCust_6));
				iLastLogIndex = j;
				return &stLogOut; 
			}
		}
	}while(iCheckLoop < m_pShmLog->iLogMaxCount);
	return NULL;
}


// class CSupperLog
// ------------------------------------------------------------------------------------------------------

void ShowShmLogContent(TSLogShm *m_pShmLog, int iStartIndex, int iModuleId=0)
{
	char m_sLogBuf[BWORLD_SLOG_MAX_LINE_LEN+64] = {0}; 
	for(int i=0; iStartIndex != m_pShmLog->iWriteIndex; iStartIndex++) 
	{
		if(iStartIndex >= m_pShmLog->iLogMaxCount)
			iStartIndex = 0;

		if(iModuleId != 0 && m_pShmLog->sLogList[iStartIndex].iModuleId != iModuleId)
			continue;

		if(m_pShmLog->sLogList[iStartIndex].iContentIndex < 0)
		{
			// 日志存在固定长度字符数组中
			printf("log seq:%u, log time:%" PRIu64 " appid:%d, moduleid:%d, log type:%d --- \n\t[log ct] - %s\n",
					m_pShmLog->sLogList[iStartIndex].dwLogSeq,
					m_pShmLog->sLogList[iStartIndex].qwLogTime,
					m_pShmLog->sLogList[iStartIndex].iAppId,
					m_pShmLog->sLogList[iStartIndex].iModuleId,
					m_pShmLog->sLogList[iStartIndex].wLogType,
					m_pShmLog->sLogList[iStartIndex].sLogContent);
		}
		else
		{
			// 日志存在变长内存中
			memcpy(m_sLogBuf, m_pShmLog->sLogList[iStartIndex].sLogContent, BWORLD_MEMLOG_BUF_LENGTH-4);
			int iLen = MYSIZEOF(m_sLogBuf)-BWORLD_MEMLOG_BUF_LENGTH+4;
			int iRet = MtReport_GetFromVmem(m_pShmLog->sLogList[iStartIndex].iContentIndex,
					m_sLogBuf+BWORLD_MEMLOG_BUF_LENGTH-4, &iLen); 

			// 4 字节校验
			uint32_t dwCheck = *(uint32_t*)(
					m_pShmLog->sLogList[iStartIndex].sLogContent+BWORLD_MEMLOG_BUF_LENGTH-4);
			if(iRet < 0 || dwCheck != *(uint32_t*)(m_sLogBuf+BWORLD_MEMLOG_BUF_LENGTH-4))
				printf("ret:%d, log seq:%u, log time:%" PRIu64 ", appid:%d, moduleid:%d, log type:%d --- \n\t"
						"check slog failed! %#x != %#x index:%d, value:%s\n\n",
						iRet, m_pShmLog->sLogList[iStartIndex].dwLogSeq,
						m_pShmLog->sLogList[iStartIndex].qwLogTime,
						m_pShmLog->sLogList[iStartIndex].iAppId,
						m_pShmLog->sLogList[iStartIndex].iModuleId,
						m_pShmLog->sLogList[iStartIndex].wLogType,
						dwCheck, *(uint32_t*)(m_sLogBuf+BWORLD_MEMLOG_BUF_LENGTH-4),
						m_pShmLog->sLogList[iStartIndex].iContentIndex, m_sLogBuf);
			else
				printf("(vmem) log seq:%u, log time:%" PRIu64 ", appid:%d, moduleid:%d, log type:%d --- \n\t[log ct] - %s\n",
						m_pShmLog->sLogList[iStartIndex].dwLogSeq,
						m_pShmLog->sLogList[iStartIndex].qwLogTime,
						m_pShmLog->sLogList[iStartIndex].iAppId,
						m_pShmLog->sLogList[iStartIndex].iModuleId,
						m_pShmLog->sLogList[iStartIndex].wLogType, m_sLogBuf);
		}
		if(++i % 20 == 0)
			getchar();
	}
}

MtClientInfo * CSupperLog::GetMtClientInfo(int32_t iMachineId, uint32_t *piIsFind)
{
	MtClientInfo stKey;
	stKey.iMachineId = iMachineId;
	MtClientInfo *pNode = NULL;
	if(piIsFind != NULL) {
		*piIsFind = 0;
		MtClientInfo stEmpty;
		stEmpty.iMachineId = 0;
		pNode = (MtClientInfo*)HashTableSearchEx_NoList(
			&m_stHashMtClient, &stKey, &stEmpty, iMachineId, piIsFind);
	}
	else
		pNode = (MtClientInfo*)HashTableSearch_NoList(&m_stHashMtClient, &stKey, iMachineId);
	return pNode;
}

MtClientInfo * CSupperLog::GetMtClientInfo(int32_t iMachineId, int32_t index)
{
	MtClientInfo *pNode = (MtClientInfo*)NOLIST_HASH_INDEX_TO_NODE(&m_stHashMtClient, index);
	if(pNode->iMachineId != iMachineId)
		return NULL;
	return pNode;
}

void CSupperLog::ShowSystemConfig(const char *pfollow)
{
	if(m_pShmConfig == NULL) {
		printf("m_pShmConfig is NULL\n");
		return;
	}

	m_pShmConfig->stSysCfg.Show();
	printf("\n");
	MtSystemConfig *pInfo = &m_pShmConfig->stSysCfg;
	if(pfollow != NULL && strstr(pfollow, "attr"))
		FollowShowAttrList(pInfo->wAttrCount, pInfo->iAttrIndexStart);
	if(pfollow != NULL && strstr(pfollow, "app"))
		FollowShowAppList(pInfo->wAppInfoCount, pInfo->iAppInfoIndexStart);
	if(pfollow != NULL && strstr(pfollow, "logconfig"))
		FollowShowSlogConfigList(pInfo->wLogConfigCount, pInfo->iLogConfigIndexStart);
	if(pfollow != NULL && strstr(pfollow, "machine"))
		FollowShowMachineList(pInfo->wMachineCount, pInfo->iMachineListIndexStart);
	if(pfollow != NULL && strstr(pfollow, "view"))
		FollowShowViewList(pInfo->wViewCount, pInfo->iViewListIndexStart);
	if(pfollow != NULL && strstr(pfollow, "client"))
		FollowShowClientList(pInfo->wCurClientCount, pInfo->iMtClientListIndexStart);

	if(pfollow != NULL && strstr(pfollow, "remoteloginfo"))
	{
		user::UserRemoteAppLogInfo stRemoteAppInfoPb;
		slog.GetUserRemoteAppLogInfoPb(stRemoteAppInfoPb);
		printf("\n------- user remote app log info -----\n");
		printf("\n%s\n", stRemoteAppInfoPb.DebugString().c_str());
	}
}

int CSupperLog::IsIpMatchMachine(MachineInfo *pMach, uint32_t dwIp)
{
	if(dwIp == pMach->ip1 || dwIp == pMach->ip2 || dwIp == pMach->ip3 || dwIp == pMach->ip4)
		return 1;
	return 0;
}

int CSupperLog::IsIpMatchLocalMachine(uint32_t dwIp)
{
	if(NULL == m_pShmConfig)
	{
		WARN_LOG("m_pShmConfig is NULL");
		return SLOG_ERROR_LINE;
	}

	// slog_config 首次运行会跑到这里
	if(m_pShmConfig->stSysCfg.iMachineId == 0)
		return 1;
	return IsIpMatchMachine(dwIp, m_pShmConfig->stSysCfg.iMachineId);
}

int CSupperLog::IsIpMatchMachine(uint32_t dwIp, int32_t iMachineId)
{
	MachineInfo *pMach = slog.GetMachineInfo(iMachineId, NULL);
	if(pMach == NULL)
	{
		WARN_LOG("get machine failed, machine:%d", iMachineId);
		return SLOG_ERROR_LINE;
	}
	if(dwIp == pMach->ip1 || dwIp == pMach->ip2 || dwIp == pMach->ip3 || dwIp == pMach->ip4)
		return 1;
	return 0;
}

void CSupperLog::CheckTest(
	int (*isTest)(int iTestCfgCount, SLogTestKey *pstTestCfg, const void *pdata), 
	const void *pdata)
{
	slog.m_bIsTestLog = 0;
	if(isTest == NULL)
		return;

	if(isTest(m_pShmConfig->stConfig[m_iConfigIndex].wTestKeyCount, 
		m_pShmConfig->stConfig[m_iConfigIndex].stTestKeys, pdata))
	{
		slog.m_bIsTestLog = 1;
	}
}

void CSupperLog::ShowVarmemInfo()
{
}

void CSupperLog::ShowAppShmLog(int iAppId, int iModuleId)
{
	if(NULL == m_pShmAppInfo)
	{
		printf("SLogAppInfo shm not attache\n"); 
		return;
	}
	AppInfo *pAppInfo = NULL;
	TSLogShm *pLogShm = NULL;
	for(int j=0, i=0; j < MAX_SLOG_APP_COUNT; j++)
	{
		pAppInfo = m_pShmAppInfo->stInfo+j;
		if(m_pShmAppInfo->stInfo[j].iAppId == iAppId && iAppId != 0) {
			i++;
			printf("\napp:%d --- [%d] info --- \n", i, m_pShmAppInfo->stInfo[j].iAppId);
			pAppInfo->Show();
			pLogShm = GetAppLogShm(pAppInfo);
			if(pLogShm == NULL)
				printf("GetAppLogShm failed ! key[%u] flag[%u] \n", pAppInfo->iAppLogShmKey, pAppInfo->dwAppLogFlag);
			else {
				pLogShm->Show();
				if(pLogShm->iLogStarIndex >= 0)
					ShowShmLogContent(pLogShm, pLogShm->iLogStarIndex, iModuleId);
			}
			break;
		}

		// 指定了 app
		if(iAppId != 0) 
			continue;

		if(m_pShmAppInfo->stInfo[j].iAppId != 0)
		{
			i++;
			printf("\napp:%d --- [%d] info --- \n", i, m_pShmAppInfo->stInfo[j].iAppId);
			pAppInfo->Show();
			pLogShm = GetAppLogShm(pAppInfo);
			if(pLogShm == NULL)
				printf("GetAppLogShm failed ! key[%u] flag[%u] \n", pAppInfo->iAppLogShmKey, pAppInfo->dwAppLogFlag);
			else {
				pLogShm->Show();
				if(pLogShm->iLogStarIndex >= 0)
					ShowShmLogContent(pLogShm, pLogShm->iLogStarIndex, iModuleId);
			}
			printf("\n");
		}
	}	
}

void CSupperLog::ShowShmLogInfo(int iLogIndex)
{
	if(!m_bInit){
		printf("CSupperLog not init !\n");
		return;
	}

	printf("--- app config info \n");
	printf("\t--- config id:%u appid:%d moduleid:%d\n",
		m_dwConfigId, m_iLogAppId, m_iLogModuleId);
	printf("\t--- logtype:%u speed:%d testkeycount:%d\n", 
		m_iLogType, m_stParam.iWriteSpeed, m_pShmConfig->stConfig[m_iConfigIndex].wTestKeyCount);
	for(int i=0; i < MAX_SLOG_TEST_KEYS_PER_CONFIG; i++)
	{
		if(m_pShmConfig->stConfig[m_iConfigIndex].stTestKeys[i].bKeyType != 0)
			printf("\t\t--- testkey info, keytype:%d, key:%s\n", 
				m_pShmConfig->stConfig[m_iConfigIndex].stTestKeys[i].bKeyType, 
				m_pShmConfig->stConfig[m_iConfigIndex].stTestKeys[i].szKeyValue);
	}
	printf("\t--- shm info - log next seq:%u, sIndex:%d, wIndex:%d\n",
		m_pShmLog->dwLogSeq, m_pShmLog->iLogStarIndex, m_pShmLog->iWriteIndex);

	if(iLogIndex >= 0)
	{
		printf("\t\t --- show index:%d log\n", iLogIndex);
		ShowShmLogContent(m_pShmLog, iLogIndex);
	}

	if(m_pShmLog->iLogStarIndex < 0)
		return;
	printf("\t\t--- first log, index:%d\n", m_pShmLog->iLogStarIndex);
	ShowShmLogContent(m_pShmLog, m_pShmLog->iLogStarIndex);

	int iLast = m_pShmLog->iWriteIndex - 1;
	if(iLast < 0)
		iLast = 0;
	if(iLast == m_pShmLog->iLogStarIndex)
		return;
	printf("\t\t--- last log, index:%d\n", iLast);
	ShowShmLogContent(m_pShmLog, iLast);
}

int CSupperLog::InitCommon(const char *pConfFile) 
{
	int32_t iRet = 0;
	char szLocalEth[32] = {0};
	char szLogTypeStr[200] = {0};
	if((iRet=LoadConfig(pConfFile,
		"LOCAL_IF_NAME", CFG_STRING, szLocalEth, "eth0", MYSIZEOF(szLocalEth),
		"SLOG_CONFIG_ID", CFG_INT, &m_dwConfigId, 0,
		"FAST_CGI_MAX_HITS", CFG_INT, &m_iFastCgiHits, 0,
		"SLOG_OUT_TYPE", CFG_INT, &m_iLogOutType, 2,
		"SLOG_EXIT_MAX_WAIT_SEC", CFG_INT, &m_iMaxExitWaitTime, 3,
		"SLOG_TYPE", CFG_STRING, szLogTypeStr, "", MYSIZEOF(szLogTypeStr),
		"SLOG_EXIT_MAX_WAIT_SEC", CFG_INT, &m_iMaxExitWaitTime, 3,
		"SLOG_CHECK_PROC_RUN", CFG_INT, &m_iCheckProcExist, 1,
		"VMEM_SHM_KEY", CFG_INT, &m_iVmemShmKey, VMEM_DEF_SHMKEY,
		(void*)NULL)) < 0)
	{
		ERR_LOG("LoadConfig:%s failed ! ret:%d", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	}

	m_iLocalLogType = GetLogTypeByStr(szLogTypeStr);
	char szLocalIp[32] = {0};
	if(GetIpFromIf(szLocalIp, szLocalEth) != 0)
	{
	    ERR_LOG("GetIpFromIf failed ! local if name:%s", szLocalEth);
	    return SLOG_ERROR_LINE;
	}

	if(m_strLocalIP.size() > 0 && m_strLocalIP != szLocalIp) 
		WARN_LOG("check local ip failed ! %s != %s", m_strLocalIP.c_str(), szLocalIp);

	m_strLocalIP = szLocalIp;
	m_dwIpAddr = inet_addr(szLocalIp);

	if((iRet=InitConfigByFile(pConfFile)) < 0 || (iRet=Init(szLocalIp)) < 0)
	{
	    ERR_LOG("slog init failed file:%s ret:%d\n", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("local ip:%s", szLocalIp);
	return 0;
}

int CSupperLog::InitConfigByFile(const char *pszConfigFile, bool bCreateShm)
{
	m_strConfigFile = pszConfigFile;
	int32_t iRet = 0, iCfgShmKey = 0;

	char szLogTypeStr[200] = {0};
	if((iRet=LoadConfig(pszConfigFile,
		"SLOG_CONFIG_SHMKEY", CFG_INT, &m_iConfigShmKey, SLOG_CLIENT_CONFIG_DEF_SHMKEY, 
		"SLOG_APPINFO_SHMKEY", CFG_INT, &m_iAppInfoShmKey, SLOG_APP_INFO_DEF_SHMKEY, 
		"SLOG_CONFIG_ID", CFG_INT, &m_dwConfigId, 0,
		"FAST_CGI_MAX_HITS", CFG_INT, &m_iFastCgiHits, 0,
		"SLOG_OUT_TYPE", CFG_INT, &m_iLogOutType, 2,
		"SLOG_TYPE", CFG_STRING, szLogTypeStr, "", MYSIZEOF(szLogTypeStr),
		"SLOG_EXIT_MAX_WAIT_SEC", CFG_INT, &m_iMaxExitWaitTime, 3,
		"SLOG_CHECK_PROC_RUN", CFG_INT, &m_iCheckProcExist, 1,
		"VMEM_SHM_KEY", CFG_INT, &m_iVmemShmKey, VMEM_DEF_SHMKEY,
		"MTREPORT_SHM_KEY", CFG_INT, &iCfgShmKey, MT_REPORT_DEF_SHM_KEY,
		(void*)NULL)) < 0)
	{   
		ERR_LOG("LoadConfig:%s failed ! ret:%d", pszConfigFile, iRet);
		return -1; 
	}

	m_iLocalLogType = GetLogTypeByStr(szLogTypeStr);
	int32_t iFlag = 0666;
	if(bCreateShm)
		iFlag |= IPC_CREAT;

	iRet = GetShm2((void**)&m_pShmAppInfo, m_iAppInfoShmKey, (int)MYSIZEOF(SLogAppInfo), iFlag);
	if(iRet < 0)
	{
		ERR_LOG("attach SLogAppInfo shm failed, size:%u, key:%d", MYSIZEOF(SLogAppInfo), m_iAppInfoShmKey);
		return SLOG_ERROR_LINE;
	}

	iRet = GetShm2((void**)&m_pShmConfig, m_iConfigShmKey, MYSIZEOF(SLogConfig), iFlag);
	if(iRet < 0)
	{
		ERR_LOG("attach SLogConfig shm failed, size:%d, key:%d", MYSIZEOF(SLogConfig), m_iConfigShmKey);
		return SLOG_ERROR_LINE;
	}

	if((iRet=MtReport_Init_ByKey(0, iCfgShmKey, 0666|IPC_CREAT)) < 0)
	{
		ERR_LOG("init attr faile, ret:%d", iRet);
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static CMemCacheClient s_memcache;
CSupperLog::CSupperLog():memcache(s_memcache)
{
	assert(strlen(TOO_LONG_TRUNC_STR) < TOO_LONG_TRUNC_STR_LEN);

	m_bInit = false;
	m_bIsTestLog = false;
	m_strLogFile = "./supper_log";
	memset(&m_stLog, 0, MYSIZEOF(m_stLog));
	memset(&m_stLogNet, 0, MYSIZEOF(m_stLogNet));
	memset(&m_stParam, 0, MYSIZEOF(m_stParam));

	m_stParam.iIsLogToStd = -1;
	m_stParam.pszLogFullName = m_strLogFile.c_str();
	m_stParam.eLogLevel = LL_TRACE; 
	m_stParam.iIsLogTime = 1;
	m_stParam.iOldStyle = 1;
	m_stParam.dwMaxLogNum = 5;
	m_stParam.iWriteSpeed = 1;
	m_stParam.eShiftType = ST_SIZE;
	m_stParam.dwMax = 1024*1024*10;

	m_iLogOutType = 0;
	m_pShmLog = NULL;
	m_iLogAppId = 0;
	m_iLogModuleId = 0;
	memset(m_sLogBuf, 0, MYSIZEOF(m_sLogBuf));
	memset(m_szLogCommData, 0, MYSIZEOF(m_szLogCommData));

	m_iConfigShmKey = SLOG_CLIENT_CONFIG_DEF_SHMKEY;
	m_iAppInfoShmKey = SLOG_APP_INFO_DEF_SHMKEY;
	m_pShmConfig = NULL;
	m_pShmAppInfo = NULL;
	m_pAppInfo = NULL;
	m_dwConfigId = 0;
	m_iConfigIndex = MAX_SLOG_CONFIG_COUNT;
	m_iLogType = 0;
	m_iLocalLogType = 0;
	m_dwConfigSeq = 0;
	m_bExitProcess = false;
	m_iRemoteLogToStd = 0;
	m_bIsRemoteLog = 1;
	strcpy(m_szCoreFile, "/home/mtreport/slog_core/");

	m_iMachineShmKey = DEF_MACHINE_SHM_KEY;
	memset(&m_stHashMachine, 0, MYSIZEOF(m_stHashMachine));

	m_iMtClientInfoShmKey = MT_CLIENT_HASH_KEY;
	memset(&m_stHashMtClient, 0, MYSIZEOF(m_stHashMtClient));

	m_iAttrShmKey = DEF_ATTR_SHM_KEY;
	memset(&m_stHashAttr, 0, MYSIZEOF(m_stHashAttr));

	m_iWarnAttrShmKey = DEF_WARN_ATTR_SHM_KEY;
	memset(&m_stWarnHashAttr, 0, MYSIZEOF(m_stWarnHashAttr));

	m_iStrAttrShmKey = DEF_STR_ATTR_SHM_KEY;
	memset(&m_stStrAttrHash, 0, MYSIZEOF(m_stStrAttrHash));

	m_iWarnConfigShmKey = DEF_WARN_CONFIG_SHM_KEY;
	memset(&m_stHashWarnConfig, 0, MYSIZEOF(m_stHashWarnConfig));

	m_iMachineViewConfigShmKey = DEF_MACHINE_VIEW_CONFIG_SHM_KEY;
	memset(&m_stHashMachineViewConfig, 0, MYSIZEOF(m_stHashMachineViewConfig));

	m_iAttrViewConfigShmKey = DEF_ATTR_VIEW_CONFIG_SHM_KEY;
	memset(&m_stHashAttrViewConfig, 0, MYSIZEOF(m_stHashAttrViewConfig));

	m_iWarnInfoShmKey = DEF_WARN_INFO_SHM_KEY;
	memset(&m_stHashWarnInfo, 0, MYSIZEOF(m_stHashWarnInfo));

	m_iAttrTypeShmKey = DEF_ATTRTYPE_SHM_KEY;
	memset(&m_stHashAttrType, 0, MYSIZEOF(m_stHashAttrType));

	m_iMonitorMachineShmKey = DEF_MONITOR_MACHINE_SHM_KEY;
	m_pShmMonitorMachine = NULL;

	m_bEnableMemcache = true;
	m_iCheckProcExist = 1;

	m_iMaxExitWaitTime = 3;
	m_dwRecvExitSigTime = 0;

	gettimeofday(&m_stNow, 0);
	srand(m_stNow.tv_sec);
	m_iRand = rand();
	m_iPid = getpid();
}

int MtClientInfoHashWarn(uint32_t dwCurUse, uint32_t dwTotal)
{
	WARN_LOG("mtclient hash will full -- %u - %u", dwCurUse, dwTotal);
	if(dwCurUse == dwTotal)
		return 1;
	return 0;
}

int MtClientInfoHashCmp(const void *pKey, const void *pNode)
{
	if(((MtClientInfo*)(pKey))->iMachineId == ((MtClientInfo*)(pNode))->iMachineId)
		return 0;
	return 1;
}

int CSupperLog::SetUserRemoteAppLogInfoPb(const user::UserRemoteAppLogInfo &stPb)
{
	MtSystemConfig *pInfo = &m_pShmConfig->stSysCfg;
	int *piLen = (int*)pInfo->sUserRemoteAppLogInfo;
	char *pbuf = (char*)(pInfo->sUserRemoteAppLogInfo+4);
	std::string strval;
	if(!stPb.AppendToString(&strval))
	{
		ERR_LOG("UserRemoteAppLogInfo append failed!");
		MtReport_Attr_Add(248, 1);
		return SLOG_ERROR_LINE;
	}

	if(strval.size()+sizeof(int) > sizeof(pInfo->sUserRemoteAppLogInfo))
	{
		ERR_LOG("UserRemoteAppLogInfo need more space %lu > %lu", 
			strval.size()+sizeof(int), sizeof(pInfo->sUserRemoteAppLogInfo));
		MtReport_Attr_Add(248, 1);
		return SLOG_ERROR_LINE;
	}

	// 预警
	if(strval.size()+sizeof(int)+50 > sizeof(pInfo->sUserRemoteAppLogInfo))
	{
		WARN_LOG("UserRemoteAppLogInfo buffer will full  %lu > %lu",
			strval.size()+sizeof(int), sizeof(pInfo->sUserRemoteAppLogInfo));
		MtReport_Attr_Add(249, 1);
	}

	*piLen = (int)strval.size();
	memcpy(pbuf, strval.c_str(), strval.size());
	DEBUG_LOG("UserRemoteAppLogInfo set ok, len:%d, info:%s", *piLen, stPb.ShortDebugString().c_str());
	return 0;
}

int CSupperLog::GetUserRemoteAppLogInfoPb(user::UserRemoteAppLogInfo &stPb)
{
	MtSystemConfig *pInfo = &m_pShmConfig->stSysCfg;
	int *piLen = (int*)pInfo->sUserRemoteAppLogInfo;
	char *pbuf = (char*)(pInfo->sUserRemoteAppLogInfo+4);

	stPb.Clear();
	if(*piLen > 0 && !stPb.ParseFromArray(pbuf, *piLen))
	{
		ERR_LOG("get UserRemoteAppLogInfo failed, len:%d", *piLen);
		MtReport_Attr_Add(251, 1);
		return SLOG_ERROR_LINE;
	}
	return *piLen;
}

CSupperLog::~CSupperLog()
{
	if(m_bInit)
		C2_CloseLog(&m_stLog);
}

void CSupperLog::DealConfigChange()
{
	if(0 == m_dwConfigId || NULL == m_pShmConfig)
		return;

	if(m_iConfigIndex >= MAX_SLOG_CONFIG_COUNT || m_iConfigIndex < 0)
	{
		m_iConfigIndex = GetSlogConfig(m_dwConfigId, NULL);
		if(m_iConfigIndex < 0)
			return;
	}

	if(m_dwConfigId != m_pShmConfig->stConfig[m_iConfigIndex].dwCfgId)
	{
		DEBUG_LOG("check config change - config seq:%u, id:%u!=%u", 
			m_dwConfigSeq, m_dwConfigId, m_pShmConfig->stConfig[m_iConfigIndex].dwCfgId);
		m_bExitProcess = true; // 配置可能被删除了，设置重启进程信号
		return;
	}

	if(m_dwConfigSeq == m_pShmConfig->stConfig[m_iConfigIndex].dwConfigSeq)
	{
		return;
	}

	m_stParam.iWriteSpeed = m_pShmConfig->stConfig[m_iConfigIndex].dwSpeedFreq;

	if(m_stParam.iWriteSpeed == 0)
		m_stParam.iWriteSpeed = 10;

	// 日志频率限制
	int iWriteSpeed = m_stParam.iWriteSpeed;
	TokenBucket_Init(&m_stLog.stWriteSpeed, iWriteSpeed, iWriteSpeed);	//瞬间write量可以达到iWriteSpeed
	TokenBucket_Init(&m_stLogNet.stWriteSpeed, iWriteSpeed, iWriteSpeed);	//瞬间write量可以达到iWriteSpeed
	if(m_iLocalLogType != 0)
		m_iLogType = m_iLocalLogType;
	else
		m_iLogType = m_pShmConfig->stConfig[m_iConfigIndex].iLogType;

	uint32_t dwSeq = m_dwConfigSeq;
	m_dwConfigSeq = m_pShmConfig->stConfig[m_iConfigIndex].dwConfigSeq;

	DEBUG_LOG("slog config change from:%u to :%u - logtype:%d, logouttype:%d, speed:%u", 
		dwSeq, m_pShmConfig->stConfig[m_iConfigIndex].dwConfigSeq, m_iLogType, m_iLogOutType,
		iWriteSpeed);
}

int32_t CSupperLog::GetAppInfo(int32_t iAppId, int32_t *piFirstFree)
{
	if(piFirstFree != NULL)
		*piFirstFree = -1;

	if(NULL == m_pShmAppInfo)
	{
		ERR_LOG("SLogAppInfo shm not attache"); 
		return -1;
	}
	int32_t iFirstFree = -1;

	for(int i=0,j=0; j < MAX_SLOG_APP_COUNT; j++)
	{
		if(m_pShmAppInfo->stInfo[j].iAppId == iAppId)
			return j;
		if(m_pShmAppInfo->stInfo[j].iAppId != 0) {
			i++;
			if(i >= m_pShmAppInfo->iAppCount && (piFirstFree == NULL || iFirstFree >= 0))
				break;
		}
		else if(iFirstFree < 0)
			iFirstFree = j;
	}
	if(piFirstFree != NULL)
		*piFirstFree = iFirstFree;
	return -1;
}

void CSupperLog::FollowShowAppList(int iCount, int32_t iStartIdx)
{
	printf("-------- follow show app start ----------\n");
	if(NULL == m_pShmAppInfo)
	{
		printf("error - SLogAppInfo shm not attache\n"); 
		return ;
	}

	AppInfo *pInfo = NULL;
	int i = 0;
	while(iStartIdx >= 0 && i < iCount) {
		pInfo = m_pShmAppInfo->stInfo+iStartIdx;
		printf("\t ---- app:%d info ---- \n", pInfo->iAppId);
		pInfo->Show();
		iStartIdx = pInfo->iNextIndex;
		i++;
		printf("\n");
	}
	printf("-------- follow show app end, count:%d(next:%d)----------\n", i, iStartIdx);
}

void CSupperLog::ShowApp(int32_t iAppId)
{
	if(NULL == m_pShmAppInfo)
	{
		ERR_LOG("SLogAppInfo shm not attache"); 
		return ;
	}

	int i = 0;
	for(int j=0; j < MAX_SLOG_APP_COUNT; j++)
	{
		if(m_pShmAppInfo->stInfo[j].iAppId != 0) {
			i++;
			if(iAppId==0) {
				printf(" --- app :%d info --- \n", m_pShmAppInfo->stInfo[j].iAppId);
				m_pShmAppInfo->stInfo[j].Show();
			}
			else if(m_pShmAppInfo->stInfo[j].iAppId == iAppId) {
				m_pShmAppInfo->stInfo[j].Show();
				return;
			}
		}
	}
	printf(" --- app total count:%d --- \n", i);
}

AppInfo * CSupperLog::GetAppInfo(int32_t iAppId)
{
	int32_t iAppIndex = GetAppInfo(iAppId, (int32_t *)NULL);
	if(iAppIndex < 0)
		return NULL;
	return m_pShmAppInfo->stInfo+iAppIndex;
}

int32_t CSupperLog::GetMonitorMachineInfo(int32_t id, int32_t *piFirstFree)
{
	if(piFirstFree != NULL)
		*piFirstFree = -1;

	if(NULL == m_pShmMonitorMachine)
	{
		if(NULL == GetMonitorMachineList())
			return SLOG_ERROR_LINE;
	}
	int32_t iFirstFree = -1;

	for(int i=0,j=0; j < DEF_MONITOR_MACHINE_SHM_KEY; j++)
	{
		if(m_pShmMonitorMachine->stInfo[j].id == id)
			return j;

		if(m_pShmMonitorMachine->stInfo[j].id != 0){
			i++;
			if(i >= m_pShmMonitorMachine->count && (piFirstFree == NULL || iFirstFree >= 0))
				break;
		}
		else if(iFirstFree < 0)
			iFirstFree = j;
	}
	if(piFirstFree != NULL)
		*piFirstFree = iFirstFree;
	return -1;
}

void CSupperLog::ShowMonitorMachineList()
{
	if(NULL == m_pShmMonitorMachine)
	{
		if(GetMonitorMachineList() == NULL)
			return;
	}

	for(int j=0; j < MAX_MONITOR_MACHINE_COUNT; j++)
	{
		if(m_pShmMonitorMachine->stInfo[j].id == 0)
			continue;
		printf("monitor server info -- id:%d ip:%s port:%d weight:%u cur weight:%u\n",
			m_pShmMonitorMachine->stInfo[j].id,
			ipv4_addr_str(m_pShmMonitorMachine->stInfo[j].ip),
			m_pShmMonitorMachine->stInfo[j].port,
			m_pShmMonitorMachine->stInfo[j].weight,
			m_pShmMonitorMachine->stInfo[j].cur_weight);
	}
}

MonitorMachineList *CSupperLog::GetMonitorMachineList()
{
	if(NULL == m_pShmMonitorMachine)
	{
		int iRet = GetShm2((void**)&m_pShmMonitorMachine,
			m_iMonitorMachineShmKey, MYSIZEOF(MonitorMachineList), 0666|IPC_CREAT);
		if(iRet < 0)
		{
			ERR_LOG("get monitor Machine shm failed, key:%d size:%u",
				m_iMonitorMachineShmKey, MYSIZEOF(MonitorMachineList)); 
			return NULL;
		}
		INFO_LOG("get monitor machine info msg:%s shm size:%u",
			(iRet==0 ? "attached" : "create"), MYSIZEOF(MonitorMachineList));
	}
	return m_pShmMonitorMachine;
}

int AttrTypeHashCmp(const void *pKey, const void *pNode)
{
	if(((AttrTypeInfo*)(pKey))->id == ((AttrTypeInfo*)(pNode))->id)
		return 0;
	return 1;
}

int AttrTypeHashWarn(uint32_t dwCurUse, uint32_t dwTotal)
{
	MtReport_Attr_Add(180, 1);
	WARN_LOG("attrtype hash will full -- %u - %u", dwCurUse, dwTotal);
	if(dwCurUse == dwTotal)
	{
		MtReport_Attr_Add(181, 1);
		return 1;
	}
	return 0;
}

int AttrViewHashCmp(const void *pKey, const void *pNode)
{
	if(((TAttrViewConfigInfo*)(pKey))->iAttrId == ((TAttrViewConfigInfo*)(pNode))->iAttrId)
		return 0;
	return 1;
}

int MachineViewHashCmp(const void *pKey, const void *pNode)
{
	if(((TMachineViewConfigInfo*)(pKey))->iMachineId == ((TMachineViewConfigInfo*)(pNode))->iMachineId)
		return 0;
	return 1;
}

int WarnConfigHashCmp(const void *pKey, const void *pNode)
{
	if(((TWarnConfig*)(pKey))->iWarnId == ((TWarnConfig*)(pNode))->iWarnId
		&& ((TWarnConfig*)(pKey))->iAttrId == ((TWarnConfig*)(pNode))->iAttrId)
		return 0;
	return 1;
}

int WarnAttrHashCmp(const void *pKey, const void *pNode)
{
	if(((TWarnAttrReportInfo*)(pKey))->iAttrId == ((TWarnAttrReportInfo*)(pNode))->iAttrId
		&& ((TWarnAttrReportInfo*)(pKey))->iMachineId == ((TWarnAttrReportInfo*)(pNode))->iMachineId)
		return 0;
	return 1;
}

int AttrHashCmp(const void *pKey, const void *pNode)
{
	if(((AttrInfoBin*)(pKey))->id == ((AttrInfoBin*)(pNode))->id)
		return 0;
	return 1;
}

int AttrHashWarn(uint32_t dwCurUse, uint32_t dwTotal)
{
	MtReport_Attr_Add(180, 1);
	WARN_LOG("hash will full -- %u - %u", dwCurUse, dwTotal);
	if(dwCurUse == dwTotal)
	{
		MtReport_Attr_Add(181, 1);
		return 1;
	}
	return 0;
}

int MachineHashCmp(const void *pKey, const void *pNode)
{
	if(((MachineInfo*)(pKey))->id == ((MachineInfo*)(pNode))->id)
		return 0;
	return 1;
}

int MachineHashWarn(uint32_t dwCurUse, uint32_t dwTotal)
{
	WARN_LOG("machine hash will full -- %u - %u", dwCurUse, dwTotal);
	if(dwCurUse == dwTotal)
		return 1;
	return 0;
}


void CSupperLog::FollowShowClientList(int iCount, int32_t iStartIdx)
{
	InitMtClientInfo();
	printf("-------- follow show client start ----------\n");

	MtClientInfo *pInfo = NULL;
	int i = 0;
	while(iStartIdx >= 0 && i < iCount) {
		pInfo = GetMtClientInfo(iStartIdx);
		printf("\t ---- mtclient:%d info ---- \n", pInfo->iMachineId);
		pInfo->Show();
		iStartIdx = pInfo->iNextIndex;
		i++;
		printf("\n");
	}
	printf("-------- follow show mtclient end, count:%d(next:%d)----------\n", i, iStartIdx);
}

void CSupperLog::FollowShowAttrList(int iCount, int32_t iStartIdx)
{
	printf("-------- follow show attr start ----------\n");
	AttrInfoBin *pInfo = NULL;
	int i = 0;
	while(iStartIdx >= 0 && i < iCount) {
		i++;
		pInfo = GetAttrInfo(iStartIdx);
		printf("\t ---- attr:%d, id:%d info ---- \n", i, pInfo->id);
		pInfo->Show();
		iStartIdx = pInfo->iNextIndex;
		printf("\n");
	}
	printf("-------- follow show attr end, count:%d(next:%d)----------\n", i, iStartIdx);
}

void CSupperLog::ShowAttrList(int32_t id)
{
	AttrInfoBin *pInfo = NULL;
	if(id != 0) {
		pInfo = GetAttrInfo(id, (uint32_t*)NULL);
		if(pInfo != NULL) {
			pInfo->Show();
		}
	}
	else {
		InitGetAllHashNode();
		int i=0;
		do {
			pInfo = (AttrInfoBin*)GetAllHashNode_NoList(&m_stHashAttr);
			if(pInfo != NULL && pInfo->id != 0) {
				if(id != 0 && pInfo->id != id)
					continue;
				i++;
				printf(" --- attr:%d, id:%d, info --- \n", i, pInfo->id);
				pInfo->Show();
			}
		}while(pInfo != NULL);
		printf("------------ attr show total count:%d ---------------\n", i);
	}
}

void CSupperLog::ShowAttrTypeList(int32_t id)
{
	AttrTypeInfo *pInfo = NULL;
	if(id != 0) {
		pInfo = GetAttrTypeInfo(id, (uint32_t*)NULL);
		if(pInfo != NULL)
			pInfo->Show();
	}
	else {
		InitGetAllHashNode();
		int i=0;
		do {
			pInfo = (AttrTypeInfo*)GetAllHashNode_NoList(&m_stHashAttrType);
			if(pInfo != NULL && pInfo->id != 0) {
				if(id != 0 && pInfo->id != id)
					continue;
				i++;
				printf(" --- attr type:%d, id:%d info --- \n", i, pInfo->id);
				pInfo->Show();
			}
		}while(pInfo != NULL);
		printf("------------ attr type show total count:%d ---------------\n", i);
	}
}

int CSupperLog::InitAttrTypeList()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTable_NoList(&m_stHashAttrType, MYSIZEOF(AttrTypeInfo), 
		ATTRTYPE_HASH_NODE_COUNT, m_iAttrTypeShmKey, AttrTypeHashCmp, AttrTypeHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("attrtype hash shm init failed, key:%d ret:%d", m_iAttrTypeShmKey, iRet);
		return iRet;
	}
	INFO_LOG("init attrtype hash info success key:%d", m_iAttrTypeShmKey);
	s_bInit = true;
	return 0;
}

int WarnInfoHashCmp(const void *pKey, const void *pNode)
{
	if(((TWarnInfo*)(pKey))->id == ((TWarnInfo*)(pNode))->id)
		return 0;
	return 1;
}

TWarnInfo* CSupperLog::GetWarnInfo(uint32_t id, uint32_t *piIsFind)
{
	TWarnInfo stKey;
	stKey.id = id;
	TWarnInfo *pNode = NULL;
	if(piIsFind != NULL) {
		*piIsFind = 0;
		TWarnInfo stEmpty;
		stEmpty.id = 0;
		pNode = (TWarnInfo*)HashTableSearchEx_NoList(
			&m_stHashWarnInfo, &stKey, &stEmpty, id, piIsFind);
	}
	else
		pNode = (TWarnInfo*)HashTableSearch_NoList(&m_stHashWarnInfo, &stKey, id);
	return pNode;
}

void CSupperLog::ShowWarnInfo(uint32_t id)
{
	if(InitWarnInfo() < 0) {
		printf("InitWarnInfo failed \n");
		return ;
	}

	TWarnInfo *pInfo = NULL;
	if(id != 0) {
		pInfo = GetWarnInfo(id, (uint32_t*)NULL);
		if(pInfo != NULL)
			pInfo->Show();
	}
	else {
		InitGetAllHashNode();
		int i=0;
		do {
			pInfo = (TWarnInfo*)GetAllHashNode_NoList(&m_stHashWarnInfo);
			if(pInfo != NULL && pInfo->id != 0) {
				if(id != 0 && pInfo->id != id)
					continue;
				i++;
				printf(" --- warn : %d info --- \n", pInfo->id);
				pInfo->Show();
			}
		}while(pInfo != NULL);
		printf("------------ warn info show total count:%d ---------------\n", i);
	}
}

int CSupperLog::InitWarnInfo()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTable_NoList(&m_stHashWarnInfo, MYSIZEOF(TWarnInfo), 
		WARN_INFO_HASH_NODE_COUNT, m_iWarnInfoShmKey, WarnInfoHashCmp, AttrHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("warn info hash shm init failed, key:%d ret:%d", m_iWarnInfoShmKey, iRet);
		return iRet;
	}
	INFO_LOG("warn info hash success key:%d", m_iWarnInfoShmKey);
	s_bInit = true;
	return 0;
}

TWarnAttrReportInfo* CSupperLog::GetWarnAttrInfo(int32_t iAttrId, int32_t iMachineId, uint32_t *piIsFind)
{
	TWarnAttrReportInfo stKey;
	stKey.iAttrId = iAttrId;
	stKey.iMachineId = iMachineId;
	TWarnAttrReportInfo *pNode = NULL;
	if(piIsFind != NULL) {
		*piIsFind = 0;
		pNode = (TWarnAttrReportInfo*)HashTableSearchEx(
			&m_stWarnHashAttr, &stKey, iAttrId+iMachineId, piIsFind);
		if(pNode != NULL && *piIsFind == false)
		{
			// 插入新节点
			if(InsertHashNode(&m_stWarnHashAttr, pNode) < 0)
				return NULL;
		}
	}
	else
		pNode = (TWarnAttrReportInfo*)HashTableSearch(&m_stWarnHashAttr, &stKey, iAttrId+iMachineId);
	return pNode;
}

int CSupperLog::InitMachineViewConfig()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTable_NoList(&m_stHashMachineViewConfig, MYSIZEOF(TMachineViewConfigInfo), 
		MACHINE_VIEW_HASH_NODE_COUNT, m_iMachineViewConfigShmKey, MachineViewHashCmp, AttrHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("machine view config hash shm init failed, key:%d ret:%d", m_iMachineViewConfigShmKey, iRet);
		return iRet;
	}
	INFO_LOG("init machine view config hash info success key:%d", m_iMachineViewConfigShmKey);
	s_bInit = true;
	return 0;
}

int CSupperLog::InitAttrViewConfig()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTable_NoList(&m_stHashAttrViewConfig, MYSIZEOF(TAttrViewConfigInfo), 
		ATTR_VIEW_HASH_NODE_COUNT, m_iAttrViewConfigShmKey, AttrViewHashCmp, AttrHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("attr view config hash shm init failed, key:%d ret:%d", m_iAttrViewConfigShmKey, iRet);
		return iRet;
	}
	INFO_LOG("init attr view config hash info success key:%d", m_iAttrViewConfigShmKey);
	s_bInit = true;
	return 0;
}

int CSupperLog::AddViewBindMach(TViewInfo *pView, int iMachineId)
{
	if(pView->bBindMachineCount+1 > MAX_COUNT_BIND_MACHINES_PER_VIEW)
	{
		ERR_LOG("view bind machine over limit %d|%d", pView->bBindMachineCount, MAX_COUNT_BIND_MACHINES_PER_VIEW);
		return -1;
	}

	// check 下是否已绑
	int i = 0;
	for(; i < pView->bBindMachineCount; i++)
	{
		if(pView->aryBindMachines[i] == iMachineId)
			return 0;
	}

	i = pView->bBindMachineCount;
	pView->aryBindMachines[i] = iMachineId;
	pView->bBindMachineCount++;
	DEBUG_LOG("view bind machine - view:%d, machine:%d, count:%d", pView->iViewId, iMachineId, i+1);
	return 1;
}

void CSupperLog::AddViewBindMach(int iViewId, int iMachineId)
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return;
	}
	for(int j=0; j < MAX_VIEW_INFO_COUNT; j++)
	{
		if(m_pShmConfig->stViewInfo[j].iViewId == iViewId)
		{
			if(m_pShmConfig->stViewInfo[j].bBindMachineCount+1 > MAX_COUNT_BIND_MACHINES_PER_VIEW)
			{
				ERR_LOG("view bind machine over limit %d|%d", 
					m_pShmConfig->stViewInfo[j].bBindMachineCount, MAX_COUNT_BIND_MACHINES_PER_VIEW);
				break;
			}

			// check 下是否已绑
			int i = 0;
			for(; i < m_pShmConfig->stViewInfo[j].bBindMachineCount; i++)
			{
				if(m_pShmConfig->stViewInfo[j].aryBindMachines[i] == iMachineId)
					return;
			}

			i = m_pShmConfig->stViewInfo[j].bBindMachineCount;
			m_pShmConfig->stViewInfo[j].aryBindMachines[i] = iMachineId;
			m_pShmConfig->stViewInfo[j].bBindMachineCount++;
			DEBUG_LOG("view bind machine - view:%d, machine:%d, count:%d", iViewId, iMachineId, i+1);
			break;
		}
	}
}

void CSupperLog::DelViewBindMach(int iViewId, int iMachineId)
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return;
	}
	for(int j=0; j < MAX_VIEW_INFO_COUNT; j++)
	{
		if(m_pShmConfig->stViewInfo[j].iViewId == iViewId)
		{
            for(int i=0; i < m_pShmConfig->stViewInfo[j].bBindMachineCount; i++)
			{ 
				if(m_pShmConfig->stViewInfo[j].aryBindMachines[i] == iMachineId)
				{ 
					for(; i+1 < m_pShmConfig->stViewInfo[j].bBindMachineCount; i++)
        			{
						m_pShmConfig->stViewInfo[j].aryBindMachines[i] = m_pShmConfig->stViewInfo[j].aryBindMachines[i+1];
        			}
					m_pShmConfig->stViewInfo[j].bBindMachineCount--;
					break;
				}
			}
			break;
		}
	}
}

void CSupperLog::AddViewBindAttr(int iViewId, int iAttr)
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return;
	}
	for(int j=0; j < MAX_VIEW_INFO_COUNT; j++)
	{
		if(m_pShmConfig->stViewInfo[j].iViewId == iViewId)
		{
			if(m_pShmConfig->stViewInfo[j].bBindAttrCount+1 > MAX_COUNT_BIND_ATTRS_PER_VIEW)
			{
				ERR_LOG("view bind attr over limit %d|%d", 
					m_pShmConfig->stViewInfo[j].bBindAttrCount, MAX_COUNT_BIND_ATTRS_PER_VIEW);
				break;
			}

			// check 下是否已绑
			int i = 0;
			for(; i < m_pShmConfig->stViewInfo[j].bBindAttrCount; i++)
			{
				if(m_pShmConfig->stViewInfo[j].aryBindAttrs[i] == iAttr)
					return;
			}

			i = m_pShmConfig->stViewInfo[j].bBindAttrCount;
			m_pShmConfig->stViewInfo[j].aryBindAttrs[i] = iAttr;
			m_pShmConfig->stViewInfo[j].bBindAttrCount++;
			DEBUG_LOG("view bind attr - view:%d, attr:%d, count:%d", iViewId, iAttr, i+1);
			break;
		}
	}
}

void CSupperLog::DelViewBindAttr(int iViewId, int iAttr)
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return;
	}
	for(int j=0; j < MAX_VIEW_INFO_COUNT; j++)
	{
		if(m_pShmConfig->stViewInfo[j].iViewId == iViewId)
		{
            for(int i=0; i < m_pShmConfig->stViewInfo[j].bBindAttrCount; i++)
			{ 
				if(m_pShmConfig->stViewInfo[j].aryBindAttrs[i] == iAttr)
				{ 
					for(; i+1 < m_pShmConfig->stViewInfo[j].bBindAttrCount; i++)
        			{
						m_pShmConfig->stViewInfo[j].aryBindAttrs[i] = m_pShmConfig->stViewInfo[j].aryBindAttrs[i+1];
        			}
					m_pShmConfig->stViewInfo[j].bBindAttrCount--;
					break;
				}
			}
			break;
		}
	}
}

void CSupperLog::ShowViewInfo(int iView)
{
	if(NULL == m_pShmConfig)
	{
		printf("SLogConfig shm not attache\n"); 
		return;
	}
	for(int j=0; j < MAX_VIEW_INFO_COUNT; j++)
	{
		if((iView==0 || iView==(int)m_pShmConfig->stViewInfo[j].iViewId) 
			&& m_pShmConfig->stViewInfo[j].iViewId != 0)
		{
			printf("\n");
			m_pShmConfig->stViewInfo[j].Show();
			if(iView != 0)
				break;
		}
	}
}

void CSupperLog::AddViewInfoCount()
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return;
	}
	m_pShmConfig->dwViewConfigCount++;
	DEBUG_LOG("add view count, total:%u", m_pShmConfig->dwViewConfigCount);
}

void CSupperLog::SubViewInfoCount()
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return;
	}
	m_pShmConfig->dwViewConfigCount--;
	DEBUG_LOG("sub view count, total:%u", m_pShmConfig->dwViewConfigCount);
}

int CSupperLog::GetViewInfoIndex(int iViewId)
{
	MtSystemConfig *pInfo = GetSystemCfg();

	int idx = pInfo->iViewListIndexStart;
	TViewInfo *pView = NULL;
	for(int i=0; i < pInfo->wViewCount && idx >= 0; i++) {
		pView = GetViewInfo(idx);
		if(pView == NULL) {
			ERR_LOG("GetViewInfo failed, idx:%d", idx);
			break;
		}
		if(pView->iViewId == iViewId)
			return idx;
		idx = pView->iNextIndex;
	}
	return SLOG_ERROR_LINE;
}

int CSupperLog::GetViewInfoIndex(int iViewId, int32_t *piFirstFree)
{
	if(piFirstFree != NULL)
		*piFirstFree = -1;

	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return -1;
	}
	int32_t iFirstFree = -1;

	for(int i=0,j=0; j < MAX_VIEW_INFO_COUNT; j++)
	{
		if(m_pShmConfig->stViewInfo[j].iViewId == iViewId)
			return j;

		if(m_pShmConfig->stViewInfo[j].iViewId != 0) {
			i++;
			if(i >= (int)m_pShmConfig->dwViewConfigCount && (piFirstFree == NULL || iFirstFree >= 0))
				break;
		}
		else if(iFirstFree < 0)
			iFirstFree = j;
	}
	if(piFirstFree != NULL)
		*piFirstFree = iFirstFree;
	return -1;
}

TMachineViewConfigInfo * CSupperLog::GetMachineViewInfo(int32_t iMachineId, uint32_t *piIsFind)
{
	TMachineViewConfigInfo stKey;
	stKey.iMachineId = iMachineId;
	TMachineViewConfigInfo *pNode = NULL;
	if(piIsFind != NULL) {
		*piIsFind = 0;
		TMachineViewConfigInfo stEmpty;
		stEmpty.iMachineId = 0;
		pNode = (TMachineViewConfigInfo*)HashTableSearchEx_NoList(
			&m_stHashMachineViewConfig, &stKey, &stEmpty, iMachineId, piIsFind);
	}
	else
		pNode = (TMachineViewConfigInfo*)HashTableSearch_NoList(&m_stHashMachineViewConfig, &stKey, iMachineId);
	return pNode;
}

TAttrViewConfigInfo * CSupperLog::GetAttrViewInfo(int32_t iAttrId, uint32_t *piIsFind)
{
	if(InitAttrViewConfig() < 0)
		return NULL;

	TAttrViewConfigInfo stKey;
	stKey.iAttrId = iAttrId;
	TAttrViewConfigInfo *pNode = NULL;
	if(piIsFind != NULL) {
		*piIsFind = 0;
		TAttrViewConfigInfo stEmpty;
		stEmpty.iAttrId = 0;
		pNode = (TAttrViewConfigInfo*)HashTableSearchEx_NoList(
			&m_stHashAttrViewConfig, &stKey, &stEmpty, iAttrId, piIsFind);
	}
	else
		pNode = (TAttrViewConfigInfo*)HashTableSearch_NoList(&m_stHashAttrViewConfig, &stKey, iAttrId);
	return pNode;
}

void CSupperLog::ShowVarConfig()
{
	printf("--- all config var as followed: \n");

	printf("\n --- about log --- \n");
	printf("\tREMOTE_LOG_TO_STD\n");
	printf("\tSLOG_LOG_TO_STD\n");
	printf("\tSLOG_LOG_SIZE\n");
	printf("\tSLOG_CONFIG_ID\n");
	printf("\tSLOG_OUT_TYPE\n");
	printf("\tSLOG_TYPE\n");
	printf("\tSLOG_LOG_FILE\n");
	printf("\tSLOG_LOG_FILE_NUM\n");
	printf("\tSLOG_IS_REMOTE_LOG\n");
	printf("\tSLOG_SET_TEST\n");

	printf("\n --- about shm key --- \n");
	printf("\tSLOG_CONFIG_SHMKEY\n");
	printf("\tSLOG_APPINFO_SHMKEY\n");
	printf("\tMT_ATTR_SHM_KEY\n");
	printf("\tMT_WARN_ATTR_SHM_KEY\n");
	printf("\tMT_WARN_CONFIG_SHM_KEY\n");
	printf("\tMT_MACHINE_VIEW_SHM_KEY\n");
	printf("\tMT_ATTR_VIEW_SHM_KEY\n");
	printf("\tMT_ATTRTYPE_SHM_KEY\n");
	printf("\tMT_MACHINE_SHM_KEY\n");
	printf("\tMT_WARN_INFO_SHM_KEY\n");
	printf("\tMONITOR_MACHINE_SHM_KEY\n");
	printf("\tVMEM_SHM_KEY\n");

	printf("\n --- about memcache ---\n");
	printf("\tSLOG_MEMCACHE_BUF_LEN\n");
	printf("\tSLOG_MEMCACHE_IP\n");
	printf("\tSLOG_MEMCACHE_PORT\n");
	printf("\tSLOG_MEMCACHE_FLAG\n");
	printf("\tSLOG_MEMCACHE_RTIMEOUT\n");
	printf("\tSLOG_MEMCACHE_WTIMEOUT\n");

	printf("\n --- something other --- \n");
	printf("\tSLOG_COREDUMP_FILE\n");
	printf("\tSLOG_PROCESS_COUNT\n");
	printf("\tLOCAL_IF_NAME\n");
	printf("\tFAST_CGI_MAX_HITS\n");
	printf("\tSLOG_CHECK_PROC_RUN\n");
	printf("\tSLOG_EXIT_MAX_WAIT_SEC\n");
}

TWarnConfig* CSupperLog::GetWarnConfigInfo(int32_t iWarnId, int32_t iAttrId, uint32_t *piIsFind)
{
	if(InitwarnConfig() < 0)
		return NULL;

	TWarnConfig stKey;
	stKey.iWarnId = iWarnId;
	stKey.iAttrId = iAttrId;
	TWarnConfig *pNode = NULL;
	if(piIsFind != NULL) {
		*piIsFind = 0;
		pNode = (TWarnConfig*)HashTableSearchEx(
			&m_stHashWarnConfig, &stKey, iWarnId+iAttrId, piIsFind);
		if(pNode != NULL && *piIsFind == false) {
			if(InsertHashNode(&m_stHashWarnConfig, pNode) < 0)
				return NULL;
		}
	}
	else
		pNode = (TWarnConfig*)HashTableSearch(&m_stHashWarnConfig, &stKey, iWarnId+iAttrId);
	return pNode;
}

int CSupperLog::InitwarnConfig()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTable(&m_stHashWarnConfig, MYSIZEOF(TWarnConfig), 
		WARN_CONFIG_HASH_NODE_COUNT, m_iWarnConfigShmKey, WarnConfigHashCmp, AttrHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("warn config hash shm init failed, key:%d ret:%d", m_iWarnConfigShmKey, iRet);
		return iRet;
	}
	INFO_LOG("init warn config hash info success key:%d", m_iWarnConfigShmKey);
	s_bInit = true;
	return 0;
}

int CSupperLog::InitWarnAttrListForWrite()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTableForWrite(&m_stWarnHashAttr, MYSIZEOF(TWarnAttrReportInfo), 
		WARN_ATTR_HASH_NODE_COUNT, m_iWarnAttrShmKey, WarnAttrHashCmp, AttrHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("attr hash shm init failed, key:%d ret:%d", m_iWarnAttrShmKey, iRet);
		return iRet;
	}
	INFO_LOG("init warn attr hash info success key:%d", m_iWarnAttrShmKey);
	s_bInit = true;
	return 0;
}

int StrAttrHashCmp(const void *pKey, const void *pNode)
{
	if(((TStrAttrReportInfo*)(pKey))->iAttrId == ((TStrAttrReportInfo*)(pNode))->iAttrId
		&& ((TStrAttrReportInfo*)(pKey))->iMachineId == ((TStrAttrReportInfo*)(pNode))->iMachineId)
		return 0;
	return 1;
}

int CSupperLog::InitStrAttrHashForWrite()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTableForWrite(&m_stStrAttrHash, MYSIZEOF(TStrAttrReportInfo), 
		STR_ATTR_HASH_NODE_COUNT, m_iStrAttrShmKey, StrAttrHashCmp, AttrHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("str attr hash shm init failed, key:%d ret:%d", m_iStrAttrShmKey, iRet);
		return iRet;
	}
	INFO_LOG("init str attr hash info success key:%d", m_iStrAttrShmKey);
	s_bInit = true;
	return 0;
}

TStrAttrReportInfo* CSupperLog::GetStrAttrShmInfo(int32_t iAttrId, int32_t iMachineId, uint32_t *piIsFind)
{
	TStrAttrReportInfo stKey;
	stKey.iAttrId = iAttrId;
	stKey.iMachineId = iMachineId;
	TStrAttrReportInfo *pNode = NULL;
	if(piIsFind != NULL) {
		*piIsFind = 0;
		pNode = (TStrAttrReportInfo*)HashTableSearchEx(
			&m_stStrAttrHash, &stKey, iAttrId+iMachineId, piIsFind);
		if(pNode != NULL && *piIsFind == false)
		{
			// 插入新节点
			if(InsertHashNode(&m_stStrAttrHash, pNode) < 0)
				return NULL;
		}
	}
	else
		pNode = (TStrAttrReportInfo*)HashTableSearch(&m_stStrAttrHash, &stKey, iAttrId+iMachineId);
	return pNode;
}

int CSupperLog::InitStrAttrHash()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTable(&m_stStrAttrHash, MYSIZEOF(TStrAttrReportInfo), 
		STR_ATTR_HASH_NODE_COUNT, m_iStrAttrShmKey, StrAttrHashCmp, AttrHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("str attr hash shm init failed, key:%d ret:%d", m_iStrAttrShmKey, iRet);
		return iRet;
	}
	INFO_LOG("init str attr hash info success key:%d", m_iStrAttrShmKey);
	s_bInit = true;
	return 0;
}

int CSupperLog::InitWarnAttrList()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTable(&m_stWarnHashAttr, MYSIZEOF(TWarnAttrReportInfo), 
		WARN_ATTR_HASH_NODE_COUNT, m_iWarnAttrShmKey, WarnAttrHashCmp, AttrHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("attr hash shm init failed, key:%d ret:%d", m_iWarnAttrShmKey, iRet);
		return iRet;
	}
	INFO_LOG("init warn attr hash info success key:%d", m_iWarnAttrShmKey);
	s_bInit = true;
	return 0;
}

StrAttrNodeValShmInfo * CSupperLog::GetStrAttrNodeValShm(bool bCreate)
{
	StrAttrNodeValShmInfo *pshm = (StrAttrNodeValShmInfo*)GetShm(
		STR_ATTR_NODE_VAL_SHM_DEF_KEY, sizeof(StrAttrNodeValShmInfo), 0666);
	if(pshm == NULL && bCreate)
		pshm = (StrAttrNodeValShmInfo*)GetShm(
			STR_ATTR_NODE_VAL_SHM_DEF_KEY, sizeof(StrAttrNodeValShmInfo), 0666|IPC_CREAT);
	return pshm;
}

int CSupperLog::InitAttrList()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTable_NoList(&m_stHashAttr, MYSIZEOF(AttrInfoBin), 
		ATTR_HASH_NODE_COUNT, m_iAttrShmKey, AttrHashCmp, AttrHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("attr hash shm init failed, key:%d ret:%d", m_iAttrShmKey, iRet);
		return iRet;
	}
	INFO_LOG("init attr hash info success key:%d", m_iAttrShmKey);
	s_bInit = true;
	return 0;
}

int CSupperLog::InitMachineList()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTable_NoList(&m_stHashMachine, MYSIZEOF(MachineInfo), 
		MACHINE_HASH_NODE_COUNT, m_iMachineShmKey, MachineHashCmp, MachineHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("Machine hash shm init failed, key:%d ret:%d", m_iMachineShmKey, iRet);
		return iRet;
	}
	s_bInit = true;
	return 0;
}

void CSupperLog::FollowShowViewList(int iCount, int32_t iStartIdx)
{
	if(NULL == m_pShmConfig)
	{
		printf("SLogConfig shm not attache\n"); 
		return;
	}
	
	TViewInfo *pView = NULL;
	int i = 0;
	while(iStartIdx >= 0 && i < iCount) {
		pView = m_pShmConfig->stViewInfo+iStartIdx;
		printf("\t ---- view:%d, id:%d info ---- \n", i, pView->iViewId);
		pView->Show();
		iStartIdx = pView->iNextIndex;
		i++;
		printf("\n");
	}
	printf("-------- follow show view end, count:%d(next:%d)----------\n", i, iStartIdx);
}

void CSupperLog::FollowShowMachineList(int iCount, int32_t iStartIdx)
{
	printf("-------- follow show machine start ----------\n");
	InitMachineList();

	MachineInfo *pInfo = NULL;
	int i = 0;
	while(iStartIdx >= 0 && i < iCount) {
		pInfo = GetMachineInfo(iStartIdx);
		printf("\t ---- machine:%d, id:%d info ---- \n", i, pInfo->id);
		if(pInfo != NULL) {
			pInfo->Show();
			iStartIdx = pInfo->iNextIndex;
		}
		else  {
			printf("get machine failed, id:%d\n", pInfo->id);
			break;
		}
		i++;
		printf("\n");
	}
	printf("-------- follow show machine end, count:%d(next:%d)----------\n", i, iStartIdx);
}

void CSupperLog::ShowMachineList(int32_t id)
{
	InitMachineList();

	MachineInfo *pInfo = NULL;
	if(id != 0) {
		pInfo = slog.GetMachineInfo(id, NULL);
		if(pInfo != NULL)
			pInfo->Show();
		else
			printf("not find machine id:%d\n", id);
	}
	else {
		InitGetAllHashNode();
		int i=0;
		do {
			pInfo = (MachineInfo*)GetAllHashNode_NoList(&m_stHashMachine);
			if(pInfo != NULL && pInfo->id != 0) {
				if(id != 0 && pInfo->id != id)
					continue;
				i++;
				printf(" --- machine : %d info --- \n", pInfo->id);
				pInfo->Show();
			}
		}while(pInfo != NULL);
		printf("------------ machine show total count:%d ---------------\n", i);
	}
}

AttrTypeInfo* CSupperLog::GetAttrTypeInfo(int32_t id, uint32_t *piIsFind)
{
	if(InitAttrTypeList() < 0)
		return NULL;

	AttrTypeInfo stKey;
	stKey.id = id;
	AttrTypeInfo *pNode = NULL;
	if(piIsFind != NULL) {
		*piIsFind = 0;
		AttrTypeInfo stEmpty;
		stEmpty.id = 0;
		pNode = (AttrTypeInfo*)HashTableSearchEx_NoList(&m_stHashAttrType, &stKey, &stEmpty, id, piIsFind);
	}
	else
		pNode = (AttrTypeInfo*)HashTableSearch_NoList(&m_stHashAttrType, &stKey, id);
	return pNode;
}

int CSupperLog::GetAttrInfoFromShm(int32_t attr_id, Json &js)
{
	return -1;
}

const char * CSupperLog::GetAttrNameFromShm(int32_t attr_id)
{
    AttrInfoBin* pInfo = slog.GetAttrInfo(attr_id, NULL);
	int iNameIdx = 0;
	const char *pvname = "unknow";
	if(NULL != pInfo) {
	    iNameIdx = pInfo->iNameVmemIdx;
	}
	else {
	    ERR_LOG("get attr info from shm failed, attr:%d, info failed", attr_id);
	    return pvname;
	}
	if(iNameIdx > 0)
	    pvname = MtReport_GetFromVmem_Local(iNameIdx);
	return pvname;
}

AttrInfoBin* CSupperLog::GetAttrInfo(int32_t id, uint32_t *piIsFind)
{
	if(InitAttrList() < 0)
		return NULL;

	AttrInfoBin stKey;
	stKey.id = id;
	AttrInfoBin *pNode = NULL;
	if(piIsFind != NULL) {
		*piIsFind = 0;
		AttrInfoBin stEmpty;
		stEmpty.id = 0;
		pNode = (AttrInfoBin*)HashTableSearchEx_NoList(&m_stHashAttr, &stKey, &stEmpty, id, piIsFind);
	}
	else
		pNode = (AttrInfoBin*)HashTableSearch_NoList(&m_stHashAttr, &stKey, id);
	return pNode;
}

MachineInfo* CSupperLog::GetMachineInfo(uint32_t dwClientIp)
{
	MtSystemConfig *pUmInfo = GetSystemCfg();
	if(pUmInfo == NULL || pUmInfo->wMachineCount <= 0)
		return NULL;

	MachineInfo * pM = NULL;
	int idx = pUmInfo->iMachineListIndexStart;
	for(int i=0; i < pUmInfo->wMachineCount && idx >= 0; i++) {
		pM = GetMachineInfo(idx);
		if(pM == NULL) {
			ERR_LOG("get machine failed, idx:%d, count:%d", idx, pUmInfo->wMachineCount);
			continue;
		}
		if(pM->ip1 == dwClientIp || pM->ip2 == dwClientIp ||  pM->ip3 == dwClientIp ||  pM->ip4 == dwClientIp)
			return pM;
		idx = pM->iNextIndex;
	}
	return NULL;
}

MachineInfo* CSupperLog::GetMachineInfo(int32_t id, uint32_t *piIsFind)
{
	if(InitMachineList() < 0)
		return NULL;

	MachineInfo stKey;
	stKey.id = id;
	MachineInfo *pNode = NULL;
	if(piIsFind != NULL) {
		*piIsFind = 0;
		MachineInfo stEmpty;
		stEmpty.id = 0;
		pNode = (MachineInfo*)HashTableSearchEx_NoList(&m_stHashMachine, &stKey, &stEmpty, id, piIsFind);
	}
	else
		pNode = (MachineInfo*)HashTableSearch_NoList(&m_stHashMachine, &stKey, id);
	return pNode;
}

int SrvForWebCmp(const void *a, const void *b)
{
	return *(int32_t*)a - *(int32_t*)b;
}

SLogServer* CSupperLog::GetWebMasterSrv()
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return NULL;
	}
	for(int i=0; i < MAX_SERVICE_COUNT; i++) {
		if(m_pShmConfig->stServerList[i].dwServiceId != 0
			&& m_pShmConfig->stServerList[i].iWeightCur > 0
			&& m_pShmConfig->stServerList[i].wType == SRV_TYPE_WEB)
		{
			return m_pShmConfig->stServerList+i;
		}
	}
	return NULL;
}

int SrvForAttrCmp(const void *a, const void *b)
{
	return *(int32_t*)a - *(int32_t*)b;
}

SLogServer* CSupperLog::GetAttrSrv()
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return NULL;
	}
	for(int i=0; i < MAX_SERVICE_COUNT; i++) {
		if(m_pShmConfig->stServerList[i].dwServiceId != 0
			&& m_pShmConfig->stServerList[i].iWeightCur > 0
			&& m_pShmConfig->stServerList[i].wType == SRV_TYPE_ATTR)
		{
			return m_pShmConfig->stServerList+i;
		}
	}
	return NULL;
}

int SrvForAppLogCmp(const void *a, const void *b)
{
	return *(int32_t*)a - *(int32_t*)b;
}

SLogServer* CSupperLog::GetAppMasterSrv(int32_t id, bool bNeedActive)
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return NULL;
	}
	for(int i=0; i < MAX_SERVICE_COUNT; i++) {
		if(m_pShmConfig->stServerList[i].dwServiceId != 0
			&& (!bNeedActive || m_pShmConfig->stServerList[i].iWeightCur > 0)
			&& m_pShmConfig->stServerList[i].wType == SRV_TYPE_APP_LOG)
		{
			if(bsearch(&id, m_pShmConfig->stServerList[i].stForAppList.aiApp,
				m_pShmConfig->stServerList[i].stForAppList.wAppCount, sizeof(int32_t), SrvForAppLogCmp) != NULL)
			{
				return m_pShmConfig->stServerList+i;
			}
		}
	}
	return NULL;
}

// 获取可用的服务器, 用于路由存活服务 - iWeightCur > 0
SLogServer* CSupperLog::GetValidServerByType(int iType, int * piStartIdx)
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return NULL;
	}
	int i = (piStartIdx != NULL ? *piStartIdx : 0);
	for(; i < MAX_SERVICE_COUNT; i++) {
		if(m_pShmConfig->stServerList[i].dwServiceId != 0
			&& m_pShmConfig->stServerList[i].wType == iType 
			&& m_pShmConfig->stServerList[i].bSandBox == SAND_BOX_ENABLE_NEW
			&& m_pShmConfig->stServerList[i].iWeightCur > 0) 
		{
			if(i+1 >= MAX_SERVICE_COUNT && piStartIdx !=NULL)
				*piStartIdx = 0;
			else if(piStartIdx !=NULL)
				*piStartIdx = i+1;
			return m_pShmConfig->stServerList+i;
		}
	}
	if(piStartIdx !=NULL)
		*piStartIdx = 0;
	return NULL;
}

// 获取服务器, 一般用于遍历
SLogServer* CSupperLog::GetServerByType(int iType, int * piStartIdx)
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return NULL;
	}
	int i = (piStartIdx != NULL ? *piStartIdx : 0);
	for(; i < MAX_SERVICE_COUNT; i++) {
		if(m_pShmConfig->stServerList[i].dwServiceId != 0
			&& m_pShmConfig->stServerList[i].wType == iType)
		{
			if(i+1 >= MAX_SERVICE_COUNT && piStartIdx != NULL)
				*piStartIdx = 0;
			else if(piStartIdx != NULL)
				*piStartIdx = i+1;
			return m_pShmConfig->stServerList+i;
		}
	}
	if(piStartIdx != NULL)
		*piStartIdx = 0;
	return NULL;
}

SLogServer* CSupperLog::GetAppLogServer(const char *psrvIp)
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return NULL;
	}
	if(psrvIp == NULL)
		psrvIp = m_strLocalIP.c_str();
	for(int i=0; i < MAX_SERVICE_COUNT; i++) {
		if(m_pShmConfig->stServerList[i].wType ==SRV_TYPE_APP_LOG 
			&& !strcmp(m_pShmConfig->stServerList[i].szIpV4, psrvIp))
				return m_pShmConfig->stServerList+i;
	}
	return NULL;
}

SLogServer* CSupperLog::GetAttrServer(const char *psrvIp)
{
	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return NULL;
	}
	if(psrvIp == NULL)
		psrvIp = m_strLocalIP.c_str();

	for(int i=0; i < MAX_SERVICE_COUNT; i++) {
		if(m_pShmConfig->stServerList[i].wType == SRV_TYPE_ATTR
			&& !strcmp(m_pShmConfig->stServerList[i].szIpV4, psrvIp))
				return m_pShmConfig->stServerList+i;
	}
	return NULL;
}

void CSupperLog::ShowServerInfo(uint32_t srv_id)
{
	if(NULL == m_pShmConfig)
	{
		printf("ServerInfo shm not attache"); 
		return;
	}

	printf("show slogserver --- total count:%u\n", m_pShmConfig->wServerCount);
	for(int i=0,j=0; j < MAX_SERVICE_COUNT; j++)
	{
		if(m_pShmConfig->stServerList[j].dwServiceId == 0)
			continue;
		if(m_pShmConfig->stServerList[j].dwServiceId == srv_id || srv_id == 0)
		{
			printf("server id:%u info ------\n", m_pShmConfig->stServerList[j].dwServiceId);
			m_pShmConfig->stServerList[j].Show();
			if(srv_id != 0)
				break;
		}
		i++;
		if(i >= (int)m_pShmConfig->wServerCount)
			break;
	}
}

int CSupperLog::CheckAttrServer()
{
	MtSystemConfig *pMaster = GetSystemCfg();
	if(pMaster == NULL)
		return SLOG_ERROR_LINE;

	if(pMaster->iAttrSrvIndex < 0 || pMaster->iAttrSrvIndex >= MAX_SERVICE_COUNT
		|| m_pShmConfig->stServerList[ pMaster->iAttrSrvIndex ].dwServiceId == 0
		|| m_pShmConfig->stServerList[ pMaster->iAttrSrvIndex ].iWeightCur <= 0
		|| m_pShmConfig->stServerList[ pMaster->iAttrSrvIndex ].dwCfgSeq != pMaster->dwAttrSrvSeq
		|| m_pShmConfig->stServerList[ pMaster->iAttrSrvIndex ].dwIp != pMaster->dwAttrSrvMasterIp
		|| m_pShmConfig->stServerList[ pMaster->iAttrSrvIndex ].wPort != pMaster->wAttrSrvPort)
	{
		SLogServer *psrv = GetAttrSrv();
		if(psrv != NULL) {
			INFO_LOG("update user attr server :%s:%d server id:%u", psrv->szIpV4, psrv->wPort, psrv->dwServiceId);
			pMaster->dwAttrSrvMasterIp = psrv->dwIp;
			pMaster->wAttrSrvPort = psrv->wPort;
			pMaster->dwAttrSrvSeq = psrv->dwCfgSeq;
			pMaster->iAttrSrvIndex = slog.GetServerInfo(psrv->dwServiceId, NULL);
			return 0;
		}
		ERR_LOG("GetAttrSrv for failed !");
		return SLOG_ERROR_LINE;
	}
	return 0;
}

int CSupperLog::CheckAppLogServer(AppInfo *papp)
{
	if(NULL == m_pShmConfig || NULL == papp)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return SLOG_ERROR_LINE;
	}

	if(papp->iLogSrvIndex < 0 || papp->iLogSrvIndex >= MAX_SERVICE_COUNT
		|| m_pShmConfig->stServerList[ papp->iLogSrvIndex ].dwServiceId == 0
		|| m_pShmConfig->stServerList[ papp->iLogSrvIndex ].iWeightCur <= 0
		|| m_pShmConfig->stServerList[ papp->iLogSrvIndex ].dwCfgSeq != papp->dwAppSrvSeq
		|| m_pShmConfig->stServerList[ papp->iLogSrvIndex ].dwIp != papp->dwAppSrvMaster
		|| m_pShmConfig->stServerList[ papp->iLogSrvIndex ].wPort != papp->wLogSrvPort)
	{
		SLogServer *psrv = GetAppMasterSrv(papp->iAppId);
		if(psrv != NULL) {
			INFO_LOG("update app log server :%s:%d server id:%u for app:%d",
				psrv->szIpV4, psrv->wPort, psrv->dwServiceId, papp->iAppId);
			papp->dwAppSrvMaster = psrv->dwIp;
			papp->wLogSrvPort = psrv->wPort;
			papp->dwAppSrvSeq = psrv->dwCfgSeq;
			papp->iLogSrvIndex = slog.GetServerInfo(psrv->dwServiceId, NULL);
			papp->dwSeq++;
			return 0;
		}
		ERR_LOG("GetAppMasterSrv for app:%d failed !", papp->iAppId);
		return SLOG_ERROR_LINE;
	}
	return 0;
}

int32_t CSupperLog::GetServerInfo(uint32_t id, int32_t *piFirstFree)
{
	if(piFirstFree != NULL)
		*piFirstFree = -1;

	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return -1;
	}
	int32_t iFirstFree = -1;

	for(int i=0,j=0; j < MAX_SERVICE_COUNT; j++)
	{
		if(m_pShmConfig->stServerList[j].dwServiceId == id)
			return j;

		if(m_pShmConfig->stServerList[j].dwServiceId != 0){
			i++;
			if(i >= (int)m_pShmConfig->wServerCount && (piFirstFree == NULL || iFirstFree >= 0))
				break;
		}
		else if(iFirstFree < 0)
			iFirstFree = j;
	}
	if(piFirstFree != NULL)
		*piFirstFree = iFirstFree;
	return -1;
}

int32_t CSupperLog::GetSlogConfig(uint32_t iConfigId, int32_t *piFirstFree)
{
	if(piFirstFree != NULL)
		*piFirstFree = -1;

	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return -1;
	}
	int32_t iFirstFree = -1;

	for(int i=0,j=0; j < MAX_SLOG_CONFIG_COUNT; j++)
	{
		if(m_pShmConfig->stConfig[j].dwCfgId == iConfigId)
			return j;

		if(m_pShmConfig->stConfig[j].dwCfgId != 0) {
			i++;
			if(i >= (int)m_pShmConfig->dwSLogConfigCount && (piFirstFree == NULL || iFirstFree >= 0))
				break;
		}
		else if(iFirstFree < 0)
			iFirstFree = j;
	}
	if(piFirstFree != NULL)
		*piFirstFree = iFirstFree;
	return -1;
}

void CSupperLog::FollowShowSlogConfigList(int iCount, int32_t iStartIdx)
{
	printf("-------- follow show slog config start ----------\n");
	if(NULL == m_pShmConfig)
	{
		printf("error - SLogConfig shm not attache\n"); 
		return;
	}

	SLogClientConfig *pInfo = NULL;
	int i = 0;
	while(iStartIdx >= 0 && i < iCount) {
		pInfo = m_pShmConfig->stConfig+iStartIdx;
		printf("\t ---- log config:%u info ---- \n", pInfo->dwCfgId);
		pInfo->Show();
		iStartIdx = pInfo->iNextIndex;
		i++;
		printf("\n");
	}
	printf("-------- follow show log config end, count:%d(next:%d)----------\n", i, iStartIdx);
}

void CSupperLog::ShowSlogConfig(uint32_t iConfigId)
{
	if(NULL == m_pShmConfig)
	{
		printf("error - SLogConfig shm not attache\n"); 
		return;
	}

	int i=0;
	for(int j=0; j < MAX_SLOG_CONFIG_COUNT; j++)
	{
		if(m_pShmConfig->stConfig[j].dwCfgId == 0)
			continue;
		if(m_pShmConfig->stConfig[j].dwCfgId != iConfigId && iConfigId != 0)
			continue;
		printf("config :%u info ------\n", m_pShmConfig->stConfig[j].dwCfgId);
		m_pShmConfig->stConfig[j].Show();
		if(iConfigId != 0)
			break;
		i++;
		if(i >= (int)m_pShmConfig->dwSLogConfigCount)
			break;
	}
	printf("show config --- count:%d -- total:%u -- local log out type:%d\n",
		i, m_pShmConfig->dwSLogConfigCount, m_iLogOutType);
}

SLogClientConfig * CSupperLog::GetSlogConfig(uint32_t iConfigId)
{
	int iConfigIndex = GetSlogConfig(iConfigId, NULL);
	if(iConfigIndex < 0)
		return NULL;
	return m_pShmConfig->stConfig+iConfigIndex;
}

int32_t CSupperLog::GetAppModuleInfo(int32_t iAppIndex, int32_t iModuleId, int32_t *piFirstFree)
{
	if(piFirstFree != NULL)
		*piFirstFree = -1;

	if(iAppIndex < 0 || iAppIndex > MAX_SLOG_APP_COUNT)
	{
		DEBUG_LOG("invalid app index:%d(%d-%d-%d)", iAppIndex, iModuleId, 
			(piFirstFree!=NULL ? *piFirstFree : 0), MAX_SLOG_APP_COUNT);
		return -1;
	}

	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return -1;
	}
	int32_t iFirstFree = -1;

	for(int i=0,j=0; j < SLOG_MODULE_COUNT_MAX_PER_APP; j++)
	{
		if((int)m_pShmAppInfo->stInfo[iAppIndex].arrModuleList[j].iModuleId == iModuleId)
			return j;

		if(m_pShmAppInfo->stInfo[iAppIndex].arrModuleList[j].iModuleId != 0) {
			i++;
			if(i >= m_pShmAppInfo->stInfo[iAppIndex].wModuleCount && (piFirstFree == NULL || iFirstFree >= 0))
				break;
		}
		else if(iFirstFree < 0)
			iFirstFree = j;
	}

	if(piFirstFree != NULL)
		*piFirstFree = iFirstFree;
	return -1;
}

int32_t CSupperLog::GetTestKey(int iConfigIndex, uint8_t bKeyType, const char *pszKey, int32_t *piFirstFree)
{
	if(piFirstFree != NULL)
		*piFirstFree = -1;

	if(iConfigIndex < 0 || iConfigIndex >= MAX_SLOG_CONFIG_COUNT)
	{
		ERR_LOG("invalid config index:%d(0-%d)", iConfigIndex, MAX_SLOG_CONFIG_COUNT);
		return -1;
	}

	if(NULL == m_pShmConfig)
	{
		ERR_LOG("SLogConfig shm not attache"); 
		return -1;
	}
	int32_t iFirstFree = -1;

	for(int i=0,j=0; j < MAX_SLOG_TEST_KEYS_PER_CONFIG; j++)
	{
		if(m_pShmConfig->stConfig[iConfigIndex].stTestKeys[j].bKeyType == bKeyType
				&& !strcmp(m_pShmConfig->stConfig[iConfigIndex].stTestKeys[j].szKeyValue, pszKey))
			return j;

		if(m_pShmConfig->stConfig[iConfigIndex].stTestKeys[j].bKeyType != 0) {
			i++;
			if(i >= m_pShmConfig->stConfig[iConfigIndex].wTestKeyCount && (piFirstFree == NULL || iFirstFree >= 0))
				break;
		}
		else if(iFirstFree < 0)
			iFirstFree = j;
	}

	if(piFirstFree != NULL)
		*piFirstFree = iFirstFree;
	return -1;
}

int CSupperLog::InitMemcache()
{
	if(m_pShmConfig == NULL || m_pShmAppInfo == NULL)
	{
		ERR_LOG("m_pShmConfig == NULL || m_pShmAppInfo == NULL call InitConfigByFile first!");
		return SLOG_ERROR_LINE;
	}

	char szIp[20] = {0};
	int32_t iRet = 0, iBufLen = 0, iPort = 0, iRTimeout=0, iWTimeout=0, iDisable=0;
	if((iRet=LoadConfig(m_strConfigFile.c_str(),
		"SLOG_MEMCACHE_BUF_LEN", CFG_INT, &iBufLen, 1048576,
		"SLOG_MEMCACHE_IP", CFG_STRING, szIp, "127.0.0.1", MYSIZEOF(szIp),
		"SLOG_MEMCACHE_PORT", CFG_INT, &iPort, 12121,  
		"SLOG_MEMCACHE_RTIMEOUT", CFG_INT, &iRTimeout, 200,  
		"SLOG_MEMCACHE_WTIMEOUT", CFG_INT, &iWTimeout, 200,  
		"SLOG_MEMCACHE_DISABLE", CFG_INT, &iDisable, 0,  
		(void*)NULL)) < 0)
	{
		ERR_LOG("LoadConfig:%s failed ! ret:%d", m_strConfigFile.c_str(), iRet);
		return SLOG_ERROR_LINE;
	}

	if(iDisable)
	{
		INFO_LOG("disable memcache");
		memcache.InitDisable();
		m_bEnableMemcache = false;
		return 0;
	}

	char *pbuf = (char*)malloc(iBufLen);
	if(NULL == pbuf)
	{
		ERR_LOG("malloc failed , len:%d", iBufLen);
		return SLOG_ERROR_LINE;
	}

	memcache.Init(pbuf, iBufLen);
	if(memcache.AsyncConnect(szIp, iPort, iRTimeout, iWTimeout) < 0)
	{
		WARN_LOG("AsyncConnect failed, ip:%s, port:%d, rt:%d, wt:%d, msg:%s",
				szIp, iPort, iRTimeout, iWTimeout, memcache.GetLastErrMsg());
		return SLOG_ERROR_LINE;
	}
	else
	{
		INFO_LOG("init memcache ok, info- ip:%s, port:%d, rt:%d, wt:%d",
				szIp, iPort, iRTimeout, iWTimeout);
		m_bEnableMemcache = true;
	}
	return 0;
}

std::string CSupperLog::GetProcNameByConfFile()
{
	std::size_t iPos = m_strConfigFile.find_last_of('/');
	if(iPos == std::string::npos)
		iPos = 0;
	else 
		iPos++;
	std::size_t iPosEnd = m_strConfigFile.find_last_of('.');
	if(iPosEnd == std::string::npos)
		iPosEnd = m_strConfigFile.size();
	return  m_strConfigFile.substr(iPos, iPosEnd-iPos);
}

int CSupperLog::CheckProcExist()
{
	std::string proc = GetProcNameByConfFile();
	if(Utility::CheckProcExist(proc.c_str()) != 0)
	{
		WARN_LOG("start failed, proc:%s, config file:%s, already run, msg:%s", 
			proc.c_str(), m_strConfigFile.c_str(), strerror(errno));
		return 1;
	}
	return 0;
}

int CSupperLog::UpdateConfig(const char *pkey, const char *pval)
{
	std::string strFileTmp(m_strConfigFile);
	strFileTmp += "_tmp";
	FILE *fp = fopen(m_strConfigFile.c_str(), "r");
	if(fp == NULL)
	{
		ERR_LOG("open file failed(%s), msg:%s", m_strConfigFile.c_str(), strerror(errno));
		return SLOG_ERROR_LINE;
	}
	FILE *fp_w = fopen(strFileTmp.c_str(), "w+");
	if(fp_w == NULL)
	{
		ERR_LOG("open file failed(%s), msg:%s", strFileTmp.c_str(), strerror(errno));
		fclose(fp);
		return SLOG_ERROR_LINE;
	}

	char sLine[1024] = {0};
	while (1)
	{
		fgets(sLine, sizeof(sLine), fp);
		if (feof(fp))
		{
			break;
		}

		if(!strncmp(sLine, pkey, strlen(pkey)))
		{
			fprintf(fp_w, "%s %s\n", pkey, pval);
			INFO_LOG("update config file:%s, config:%s %s", m_strConfigFile.c_str(), pkey, pval);
		}
		else
			fputs(sLine, fp_w);
	}
	fclose(fp);
	fclose(fp_w);
	sprintf(sLine, "mv %s %s_bk; mv %s %s", m_strConfigFile.c_str(), m_strConfigFile.c_str(), 
		strFileTmp.c_str(), m_strConfigFile.c_str());
	int iRet = system(sLine);
	INFO_LOG("execute cmd:%s, ret:%d", sLine, iRet);
	return 0;
}

int CSupperLog::InitForUseLocalLog(const char *pszConfigFile)
{
	char szLogFile[256] = {0};
	int32_t iRet = 0;

	m_strConfigFile = pszConfigFile;
	m_iLogOutType = BWORLD_SLOG_TYPE_LOCAL;
	int iIsLogToStd = 0, iCfgShmKey = 0;
	char szLogTypeStr[200] = {0};
	if((iRet=LoadConfig(m_strConfigFile.c_str(),
		"SLOG_LOG_TO_STD", CFG_INT, &iIsLogToStd, 1,
		"SLOG_LOG_SIZE", CFG_INT, &m_stParam.dwMax, 10485760,
		"SLOG_LOG_FILE", CFG_STRING, szLogFile, "./slog.log", MYSIZEOF(szLogFile),
		"SLOG_LOG_FILE_NUM", CFG_INT, &m_stParam.dwMaxLogNum, 5,  
		"SLOG_LOG_FREQ", CFG_INT, &m_stParam.iWriteSpeed, 1,  
		"SLOG_SET_TEST", CFG_INT, &m_bIsTestLog, 0,
		"SLOG_COREDUMP_FILE", CFG_STRING, m_szCoreFile,  "/home/mtreport/slog_core/", MYSIZEOF(m_szCoreFile),
		"SLOG_PROCESS_COUNT", CFG_INT, &m_iProcessCount, 1,
		"SLOG_CONFIG_ID", CFG_INT, &m_dwConfigId, 0,
		"FAST_CGI_MAX_HITS", CFG_INT, &m_iFastCgiHits, 0,
		"SLOG_OUT_TYPE", CFG_INT, &m_iLogOutType, 2,
		"SLOG_TYPE", CFG_STRING, szLogTypeStr, "", MYSIZEOF(szLogTypeStr),
		"SLOG_EXIT_MAX_WAIT_SEC", CFG_INT, &m_iMaxExitWaitTime, 3,
		"SLOG_CHECK_PROC_RUN", CFG_INT, &m_iCheckProcExist, 1,
		"VMEM_SHM_KEY", CFG_INT, &m_iVmemShmKey, VMEM_DEF_SHMKEY,
		"MTREPORT_SHM_KEY", CFG_INT, &iCfgShmKey, MT_REPORT_DEF_SHM_KEY,
		(void*)NULL)) < 0)
	{   
		ERR_LOG("LoadConfig:%s failed ! ret:%d", m_strConfigFile.c_str(), iRet);
		return SLOG_ERROR_LINE;
	}

	m_iLocalLogType = GetLogTypeByStr(szLogTypeStr);
	if(m_stParam.iIsLogToStd < 0)
		m_stParam.iIsLogToStd = iIsLogToStd;

	SetLogFile(szLogFile);
	if(InitLocalLog() < 0)
		return SLOG_ERROR_LINE;

	if(m_iLocalLogType != 0)
		m_iLogType = m_iLocalLogType;

	if((iRet=MtReport_Init_ByKey(0, iCfgShmKey, 0666|IPC_CREAT)) < 0)
	{
		ERR_LOG("init attr faile, ret:%d", iRet);
		return SLOG_ERROR_LINE;
	}

	m_bInit = true;
	srand(time(NULL));
	INFO_LOG("local log only info - %s|%d|%s", szLogFile, m_iLogType, m_szCoreFile);
	return 0;
}

int CSupperLog::Init(const char *pszLocalIP)
{
	m_bInit = false; // 尝试用数据库配置初始
	char szLocalEth[32] = {0};

	if(m_pShmConfig == NULL || m_pShmAppInfo == NULL)
	{
		ERR_LOG("m_pShmConfig == NULL || m_pShmAppInfo == NULL call InitConfigByFile first!");
		return SLOG_ERROR_LINE;
	}

	char szLogFile[256] = {0};
	char szLogTypeStr[200] = {0};
	int32_t iRet = 0, iIsLogToStd = 0;
	if((iRet=LoadConfig(m_strConfigFile.c_str(),
		"REMOTE_LOG_TO_STD", CFG_INT, &m_iRemoteLogToStd, 0,
		"SLOG_LOG_TO_STD", CFG_INT, &iIsLogToStd, 1,
		"SLOG_LOG_SIZE", CFG_INT, &m_stParam.dwMax, 10485760,
		"SLOG_CONFIG_ID", CFG_INT, &m_dwConfigId, 0,  
		"FAST_CGI_MAX_HITS", CFG_INT, &m_iFastCgiHits, 0,
		"SLOG_LOG_FILE", CFG_STRING, szLogFile, "./slog.log", MYSIZEOF(szLogFile),
		"SLOG_LOG_FILE_NUM", CFG_INT, &m_stParam.dwMaxLogNum, 5,  
		"SLOG_IS_REMOTE_LOG", CFG_INT, &m_bIsRemoteLog, 1,
		"MT_ATTR_SHM_KEY", CFG_INT, &m_iAttrShmKey, DEF_ATTR_SHM_KEY,
		"MT_WARN_ATTR_SHM_KEY", CFG_INT, &m_iWarnAttrShmKey, DEF_WARN_ATTR_SHM_KEY,
		"MT_WARN_CONFIG_SHM_KEY", CFG_INT, &m_iWarnConfigShmKey, DEF_WARN_CONFIG_SHM_KEY,
		"MT_MACHINE_VIEW_SHM_KEY", CFG_INT, &m_iMachineViewConfigShmKey, DEF_MACHINE_VIEW_CONFIG_SHM_KEY,
		"MT_ATTR_VIEW_SHM_KEY", CFG_INT, &m_iAttrViewConfigShmKey, DEF_ATTR_VIEW_CONFIG_SHM_KEY,
		"MT_WARN_INFO_SHM_KEY", CFG_INT, &m_iWarnInfoShmKey, DEF_WARN_INFO_SHM_KEY,
		"SLOG_SET_TEST", CFG_INT, &m_bIsTestLog, 0,
		"SLOG_OUT_TYPE", CFG_INT, &m_iLogOutType, 2,
		"SLOG_TYPE", CFG_STRING, szLogTypeStr, "", MYSIZEOF(szLogTypeStr),
		"SLOG_EXIT_MAX_WAIT_SEC", CFG_INT, &m_iMaxExitWaitTime, 3,
		"SLOG_CHECK_PROC_RUN", CFG_INT, &m_iCheckProcExist, 1,
		"VMEM_SHM_KEY", CFG_INT, &m_iVmemShmKey, VMEM_DEF_SHMKEY,
		"MT_ATTRTYPE_SHM_KEY", CFG_INT, &m_iAttrTypeShmKey, DEF_ATTRTYPE_SHM_KEY,
		"MT_MACHINE_SHM_KEY", CFG_INT, &m_iMachineShmKey, DEF_MACHINE_SHM_KEY,
		"MONITOR_MACHINE_SHM_KEY", CFG_INT, &m_iMonitorMachineShmKey, DEF_MONITOR_MACHINE_SHM_KEY,
		"SLOG_COREDUMP_FILE", CFG_STRING, m_szCoreFile,  "/home/mtreport/slog_core/", MYSIZEOF(m_szCoreFile),
		"SLOG_PROCESS_COUNT", CFG_INT, &m_iProcessCount, 1,
		"LOCAL_IF_NAME", CFG_STRING, szLocalEth, "eth0", MYSIZEOF(szLocalEth),
		(void*)NULL)) < 0)
	{   
		ERR_LOG("LoadConfig:%s failed ! ret:%d", m_strConfigFile.c_str(), iRet);
		return -1; 
	}

	m_iLocalLogType = GetLogTypeByStr(szLogTypeStr);
	SetLogFile(szLogFile);
	
	if(m_stParam.iIsLogToStd < 0)
		m_stParam.iIsLogToStd = iIsLogToStd;

	if(pszLocalIP != NULL)
		m_strLocalIP = pszLocalIP;
	if(m_strLocalIP.size() <= 0)
	{
		char szLocalIp[32] = {0};
		if(GetIpFromIf(szLocalIp, szLocalEth) != 0)
		{
			ERR_LOG("GetIpFromIf failed ! local if name:%s", szLocalEth);
			return SLOG_ERROR_LINE;
		}
		m_strLocalIP = szLocalIp;
		pszLocalIP = m_strLocalIP.c_str();
	}

	m_dwIpAddr = inet_addr(pszLocalIP);

	m_iConfigIndex = -1;
	m_iConfigIndex = GetSlogConfig(m_dwConfigId, NULL);
	if(m_iConfigIndex < 0)
	{
		ERR_LOG("get config failed -config id:%u !", m_dwConfigId);
		return SLOG_ERROR_LINE;
	}

	// 日志级别的配置，网络日志使用共享内存中的配置
	if(m_iLocalLogType != 0)
		m_iLogType = m_iLocalLogType;
	else
		m_iLogType = m_pShmConfig->stConfig[m_iConfigIndex].iLogType;

	m_stParam.iWriteSpeed = m_pShmConfig->stConfig[m_iConfigIndex].dwSpeedFreq;
	m_dwConfigSeq = m_pShmConfig->stConfig[m_iConfigIndex].dwConfigSeq;
	m_iLogAppId = m_pShmConfig->stConfig[m_iConfigIndex].iAppId;
	m_iLogModuleId = m_pShmConfig->stConfig[m_iConfigIndex].iModuleId;

	m_pAppInfo = GetAppInfo(m_iLogAppId);
	if(m_pAppInfo == NULL) {
		ERR_LOG("GetAppInfo failed appid:%d !", m_iLogAppId);
		return SLOG_ERROR_LINE;
	}
	SET_BIT(m_pAppInfo->dwAppLogFlag, APPLOG_FLAG_LOG_WRITED);

	iRet = 0;
	if((iRet=InitSupperLog()) < 0)
	{   
		ERR_LOG("InitSupperLog failed, ret:%d", iRet);
		return SLOG_ERROR_LINE;
	} 

	m_bInit = true;
	srand(time(NULL));

	DEBUG_LOG("attach shm SLogConfig key:%d config id:%d size:%u config count:%d ok",
		m_iConfigShmKey, m_dwConfigId, MYSIZEOF(SLogConfig), m_pShmConfig->dwSLogConfigCount);
	DEBUG_LOG("attach shm SLogAppInfo key:%d size:%u app count:%d istest:%d ok",
		m_iAppInfoShmKey, MYSIZEOF(SLogAppInfo), m_pShmAppInfo->iAppCount, m_bIsTestLog);
	DEBUG_LOG("config - app:%d module:%d tostd:%d, logtype:%u logouttype:%d "
		" wspeed:%d logfile:%s core file:%s, localip:%s", 
		m_iLogAppId, m_iLogModuleId, m_stParam.iIsLogToStd, m_iLogType, m_iLogOutType, 
		m_stParam.iWriteSpeed, szLogFile, m_szCoreFile, m_strLocalIP.c_str());
	DEBUG_LOG("local log info - iIsLogToStd:%d, iOldStyle:%d, eShiftType:%d, dwMax:%u, "
		"dwMaxLogNum:%d, iWriteSpeed:%d, eLogLevel:%d, iIsLogTime:%d",
		m_stParam.iIsLogToStd, m_stParam.iOldStyle, m_stParam.eShiftType, m_stParam.dwMax, 
		m_stParam.dwMaxLogNum, m_stParam.iWriteSpeed, m_stParam.eLogLevel, m_stParam.iIsLogTime);
	DEBUG_LOG("process count :%d", m_iProcessCount);

	// fix bug slog_config 启动自动退出问题
	m_bExitProcess = false;
	return 0;
}

void deal_exist_sig(int sig)
{
	DEBUG_LOG("get exist signal:%d", sig);
	slog.m_bExitProcess = true;
	if(slog.m_dwRecvExitSigTime == 0)
		slog.m_dwRecvExitSigTime = time(NULL);
}

void CSupperLog::Daemon(int nochdir, int noclose, int noPreFork)
{
	daemon(nochdir, noclose);
	m_bInit = false;
	if(m_iCheckProcExist && CheckProcExist() != 0)
		exit(0);
	m_bInit = true;

	// core 文件目录&文件名，其中 fastcgi 不设置core 信号处理(fastcgi core 信号处理无法顺利输出堆栈，原因不知)
	if(m_iFastCgiHits <= 0) {
		char sBuf[512];
		snprintf(sBuf, MYSIZEOF(sBuf)-1, "mkdir -p %s", m_szCoreFile);
		system(sBuf);
		std::string proc = GetProcNameByConfFile();
		strcat(m_szCoreFile, proc.c_str());
		RegisterSignal(m_szCoreFile, NULL, NULL);
		signal(SIGUSR1, deal_exist_sig);
		signal(SIGCLD, SIG_IGN);
	}

	m_iProcessId = 0;
	if(noPreFork)
		return;
	for(int i=1; i < m_iProcessCount; i++)
	{
		if(fork() > 0)
			continue; // 父进程继续创建

		m_iProcessId = i;
		break;
	}
	m_iPid = getpid();
}

int CSupperLog::GetAppPort()
{
	if(m_iConfigIndex >= MAX_SLOG_CONFIG_COUNT || m_pShmConfig == NULL
		|| m_iLogAppId != m_pShmConfig->stConfig[m_iConfigIndex].iAppId)
	{
		FATAL_LOG("invalid config index:%d or pconfig:%p or appid:%d!=%d", 
			m_iConfigIndex, m_pShmConfig, m_iLogAppId, m_pShmConfig->stConfig[m_iConfigIndex].iAppId);
		return SLOG_ERROR_LINE;
	}
	return 0;
}

int CSupperLog::InitShmLog()
{
	int32_t iRet = 0;

	if(slog.m_iVmemShmKey > 0) {
		iRet = MtReport_InitVmem_ByFlag(0666|IPC_CREAT, slog.m_iVmemShmKey);
		if(iRet < 0) {
			ERR_LOG("Init vmem buf failed ! ret:%d", iRet);
			return SLOG_ERROR_LINE;
		}
	}

	if(m_pAppInfo == NULL) {
		ERR_LOG("m_pAppInfo not init, should call init fun !");
		return SLOG_ERROR_LINE;
	}

	if((m_pShmLog=GetAppLogShm(m_pAppInfo, true)) == NULL) {
		ERR_LOG("get applogshm failed appid :%d", m_pAppInfo->iAppId);
		return SLOG_ERROR_LINE;
	}

	if(m_stParam.iWriteSpeed == 0)
		m_stParam.iWriteSpeed = 10;

	// 日志频率限制
	int iWriteSpeed = m_stParam.iWriteSpeed; 
	TokenBucket_Init(&m_stLog.stWriteSpeed, iWriteSpeed, iWriteSpeed);	//瞬间write量可以达到iWriteSpeed
	TokenBucket_Init(&m_stLogNet.stWriteSpeed, iWriteSpeed, iWriteSpeed);	//瞬间write量可以达到iWriteSpeed
	return 0;
}

int CSupperLog::InitSupperLog()
{
	int iRet = -1;

	if(BWORLD_SLOG_TYPE_LOCAL & m_iLogOutType){
		iRet = InitLocalLog();
		if(iRet < 0) 
			return SLOG_ERROR_LINE;
	} 

	if(BWORLD_SLOG_TYPE_NET & m_iLogOutType){
		iRet = InitShmLog();
		if(iRet < 0) 
			return SLOG_ERROR_LINE;
	}
	return iRet;
}

int CSupperLog::InitLocalLog()
{
	int32_t iRet = 0;
	if((iRet=C2_InitLogFile(&m_stLog, &m_stParam)) != 0) {
		ERR_LOG("C2_InitLogFile failed, ret:%d, msg:%s", iRet, GetApiLastError());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

int CSupperLog::WriteAppLogToShm(TSLogShm* pShmLog, LogInfo *pLog, int32_t dwLogHost)
{
	int32_t iWrite = 0, iIndex = 0;
	bool bLogFull = false;
	uint32_t dwSeq = 0;

	// 提前判断缓冲区是否满
	if(InitGetShmLogIndex(pShmLog) < 0)
		return SLOG_ERROR_LINE;
	iIndex = pShmLog->iWriteIndex;
	// 缓存区满时，丢弃新日志，不改变 iWriteIndex
	if(iIndex == pShmLog->iLogStarIndex)
		bLogFull = true;
	else 
	{
		SLOG_NEXT_INDEX(iIndex, pShmLog->iWriteIndex);
		// seq 影响实时日志获取, 也要加锁
		dwSeq = pShmLog->dwLogSeq++;
		if(0 == dwSeq)
		{
			dwSeq = 1;
			pShmLog->dwLogSeq = 2;
		}
		pShmLog->sLogList[iIndex].dwLogSeq = 0;
	}
	EndGetShmLogIndex(pShmLog);

	if(bLogFull)
	{
		// 将要写的数组元素 seq 不为 0 或者条数超过最大值, 日志写满了
		SLOG_WARN_SHM_WRITE_FULL;

		// 注意这里需要检查下 app, 否则可能导致堆栈耗光, 递归了
		if(pShmLog->iAppId != m_iLogAppId)
		{
			ERR_LOG("slog exception(user) -- appid:%d moduleid:%d index:%d log lost may be full !", 
				pShmLog->iAppId, pLog->iModuleId, iIndex);
		}
		return SLOG_ERROR_LINE;
	}

	memset(m_sLogBuf, 0, MYSIZEOF(m_sLogBuf));
	struct timeval stNow;
	stNow.tv_sec = pLog->qwLogTime/1000000;
	stNow.tv_usec = pLog->qwLogTime%1000000;
	struct tm stTm;
	localtime_r(&stNow.tv_sec, &stTm);
	iWrite += strftime(m_sLogBuf, MYSIZEOF(m_sLogBuf), "%Y-%m-%d %H:%M:%S", &stTm);
	iWrite += snprintf(m_sLogBuf + iWrite, MYSIZEOF(m_sLogBuf) - iWrite, ".%06u ", (uint32_t)stNow.tv_usec);

	char *pLogContent = pLog->sLog + pLog->wCustDataLen;
	int iContentLen = pLog->wLogDataLen;
	if(iContentLen > BWORLD_SLOG_MAX_LINE_LEN) {
		if(iWrite+LOG_TRUNCATE_STR_LENGTH >= 64) {
			memcpy(m_sLogBuf+iWrite, pLogContent, BWORLD_SLOG_MAX_LINE_LEN-iWrite);
			iWrite = BWORLD_SLOG_MAX_LINE_LEN;
			memcpy(m_sLogBuf+iWrite, TOO_LONG_LOG_TRUNCATE_STR, LOG_TRUNCATE_STR_LENGTH);
			iWrite += LOG_TRUNCATE_STR_LENGTH;
		}
		else {
			memcpy(m_sLogBuf+iWrite, pLogContent, BWORLD_SLOG_MAX_LINE_LEN);
			iWrite += BWORLD_SLOG_MAX_LINE_LEN;
			memcpy(m_sLogBuf+iWrite, TOO_LONG_LOG_TRUNCATE_STR, LOG_TRUNCATE_STR_LENGTH);
			iWrite += LOG_TRUNCATE_STR_LENGTH;
		}
	}
	else {
		memcpy(m_sLogBuf+iWrite, pLogContent, iContentLen);
		iWrite += iContentLen;
	}
	if(m_sLogBuf[iWrite-1] != '\0') { // 字符串化
		m_sLogBuf[iWrite] = '\0';
		iWrite++;
	}

	pShmLog->sLogList[iIndex].iModuleId = pLog->iModuleId;
	pShmLog->sLogList[iIndex].iAppId = pLog->iAppId;

	// wCustDataLen 大于 0 才有定制日志
	if(pLog->wCustDataLen > 0) {
		uint8_t bCustFlag = pLog->sLog[0];
		char *pCust = pLog->sLog+1;
		if(IS_SET_BIT(bCustFlag, MTLOG_CUST_FLAG_C1_SET)) {
			pShmLog->sLogList[iIndex].dwCust_1 = *(uint32_t*)pCust;
			pCust += MYSIZEOF(uint32_t);
		}
		else
			pShmLog->sLogList[iIndex].dwCust_1 = 0;

		if(IS_SET_BIT(bCustFlag, MTLOG_CUST_FLAG_C2_SET)) {
			pShmLog->sLogList[iIndex].dwCust_2 = *(uint32_t*)pCust;
			pCust += MYSIZEOF(uint32_t);
		}
		else
			pShmLog->sLogList[iIndex].dwCust_2 = 0;

		if(IS_SET_BIT(bCustFlag, MTLOG_CUST_FLAG_C3_SET)) {
			pShmLog->sLogList[iIndex].iCust_3 = *(int32_t*)pCust;
			pCust += MYSIZEOF(int32_t);
		}
		else
			pShmLog->sLogList[iIndex].iCust_3 = 0;

		if(IS_SET_BIT(bCustFlag, MTLOG_CUST_FLAG_C4_SET)) {
			pShmLog->sLogList[iIndex].iCust_4 = *(int32_t*)pCust;
			pCust += MYSIZEOF(int32_t);
		}
		else
			pShmLog->sLogList[iIndex].iCust_4 = 0;

		if(!IS_SET_BIT(bCustFlag, MTLOG_CUST_FLAG_C5_SET))
			pShmLog->sLogList[iIndex].szCust_5[0] = '\0';
		else {
			memcpy(pShmLog->sLogList[iIndex].szCust_5, pCust, MYSIZEOF(pShmLog->sLogList[iIndex].szCust_5));
			pCust += MYSIZEOF(pShmLog->sLogList[iIndex].szCust_5);
		}

		if(!IS_SET_BIT(bCustFlag, MTLOG_CUST_FLAG_C6_SET)) 
			pShmLog->sLogList[iIndex].szCust_6[0] = '\0';
		else {
			memcpy(pShmLog->sLogList[iIndex].szCust_6, pCust, MYSIZEOF(pShmLog->sLogList[iIndex].szCust_6));
			pCust += MYSIZEOF(pShmLog->sLogList[iIndex].szCust_6);
		}
	}

	pShmLog->sLogList[iIndex].wLogType = pLog->wLogType; 
	pShmLog->sLogList[iIndex].qwLogTime = pLog->qwLogTime;
	pShmLog->sLogList[iIndex].dwLogHost = dwLogHost;
	pShmLog->sLogList[iIndex].dwLogConfigId = pLog->dwLogConfigId;
	
	if(iWrite < BWORLD_MEMLOG_BUF_LENGTH)
	{
		pShmLog->sLogList[iIndex].iContentIndex = -1; 
		strcpy(pShmLog->sLogList[iIndex].sLogContent, m_sLogBuf);
	}
	else
	{
		memcpy(pShmLog->sLogList[iIndex].sLogContent, m_sLogBuf, BWORLD_MEMLOG_BUF_LENGTH);

		if(m_iVmemShmKey <= 0) {
			// vmem 未启用
			pShmLog->sLogList[iIndex].sLogContent[BWORLD_MEMLOG_BUF_LENGTH-1] = '\0';
			pShmLog->sLogList[iIndex].iContentIndex = -1; 
		}
		else {
			// 4 : 用于存储校验，即 sLogContent 的最后 4 字节要等于 vmem 中的前面 4 字节
			iWrite = MtReport_SaveToVmem(m_sLogBuf+BWORLD_MEMLOG_BUF_LENGTH-4, iWrite-BWORLD_MEMLOG_BUF_LENGTH+4);
			if(iWrite < 0)
			{
				snprintf(pShmLog->sLogList[iIndex].sLogContent, BWORLD_MEMLOG_BUF_LENGTH-1,
						"write shm log failed, MtReport_SaveToVmem ret:%d(appid:%d, moduleid:%d, loglen:%u)", 
						iWrite, pShmLog->iAppId, pLog->iModuleId, MYSTRLEN(m_sLogBuf));
				pShmLog->sLogList[iIndex].wLogType = SLOG_TYPE_ERROR;
				pShmLog->sLogList[iIndex].iContentIndex = -1; 
			}
			else
				pShmLog->sLogList[iIndex].iContentIndex = iWrite;
		}
	}

	pShmLog->sLogList[iIndex].dwLogSeq = dwSeq;
	ModifyLogStartIndexCmpAndSwap(pShmLog, -1, iIndex);
	return 0;
}

void CSupperLog::ShowShmLog(int iCount)
{
	if(NULL == m_pShmLog)
	{
		ERR_LOG("slog not init with shm log !");
		return;
	}
	else if(iCount <= 0 || iCount > m_pShmLog->iLogMaxCount)
	{
		ERR_LOG("invalid parameter:%d %d", iCount, m_pShmLog->iLogMaxCount);
		return;
	}
	else if(m_pShmLog->iLogStarIndex < 0 || m_pShmLog->iLogStarIndex == m_pShmLog->iWriteIndex)
	{
		ERR_LOG("have no log in shm");
		return;
	}

	memset(m_sLogBuf, 0, MYSIZEOF(m_sLogBuf));
	int iStartIndex = m_pShmLog->iLogStarIndex;
	ERR_LOG("slog shm info ....................................");
	ERR_LOG("log next seq:%u, logstart index:%d, write index:%u", 
		m_pShmLog->dwLogSeq, m_pShmLog->iLogStarIndex, m_pShmLog->iWriteIndex);
	for(int i=0; i < iCount && iStartIndex != m_pShmLog->iWriteIndex; i++)
	{
		// 这里输出的不一定是时间顺序，因为写数组为回环数组
		ShowShmLogContent(m_pShmLog, iStartIndex);
		SLOG_NEXT_INDEX(iStartIndex, iStartIndex);
	}
}

// 获取 shmlog 索引， 失败返回-1
int CSupperLog::InitGetShmLogIndex(TSLogShm *pShmLog)
{
	for(int i = 0; i < 100; i++)
	{
		if(__sync_bool_compare_and_swap(&pShmLog->bTryGetLogIndex, 0, 1))
			return 0;
		usleep(10);
	}
	MtReport_Attr_Add(85, 1);
	return -1;
}

void CSupperLog::EndGetShmLogIndex(TSLogShm *pShmLog)
{
	pShmLog->bTryGetLogIndex = 0;
}

void CSupperLog::ModifyLogStartIndexCmpAndSwap(TSLogShm *pShmLog, int iOld, int iNew)
{
	// 改 iLogStarIndex 的前提 iOld 等于 pShmLog->iLogStarIndex
	while(iOld == pShmLog->iLogStarIndex)
	{
		if(InitGetShmLogIndex(pShmLog) < 0)
			continue;
		if(pShmLog->iLogStarIndex < 0)
			pShmLog->iLogStarIndex = iNew;
		else if(iNew == pShmLog->iWriteIndex)
			pShmLog->iLogStarIndex = -1;
		else
			pShmLog->iLogStarIndex = iNew;
		EndGetShmLogIndex(pShmLog);
		break;
	}
}

void CSupperLog::RemoteShmLog(TSLogOut &stLog, TSLogShm* pShmLog)
{
	if(NULL == pShmLog)
		pShmLog = m_pShmLog;

	int iIndex = 0;
	bool bLogFull = false;
	uint32_t dwSeq = 0;

	// 提前判断缓冲区是否满
	if(InitGetShmLogIndex(pShmLog) < 0)
		return;
	iIndex = pShmLog->iWriteIndex;
	// 缓存区满时，丢弃新日志，不改变 iWriteIndex
	if(iIndex == pShmLog->iLogStarIndex)
		bLogFull = true;
	else 
	{
		SLOG_NEXT_INDEX(iIndex, pShmLog->iWriteIndex);
		// dwSeq 用于实时日志获取、历史日志读取校验，用服务器上的
		dwSeq = pShmLog->dwLogSeq++;
		if(0 == dwSeq)
		{
			dwSeq = 1;
			pShmLog->dwLogSeq = 2;
		}
		// 先设置为 0
		pShmLog->sLogList[iIndex].dwLogSeq = 0;
	}
	EndGetShmLogIndex(pShmLog);

	if(bLogFull)
	{
		// 将要写的数组元素 seq 不为 0 或者条数超过最大值, 日志写满了
		SLOG_WARN_SHM_WRITE_FULL;

		// 注意这里需要检查下 app, 否则可能导致堆栈耗光, 递归了
		if(stLog.iAppId != m_iLogAppId)
		{
			ERR_LOG("slog exception(local remote) -- appid:%d moduleid:%d index:%d, log lost may be full !", 
				stLog.iAppId, stLog.iModuleId, iIndex);
		}
		return ;
	}

	pShmLog->sLogList[iIndex].iAppId = stLog.iAppId;
	pShmLog->sLogList[iIndex].iModuleId = stLog.iModuleId;
	pShmLog->sLogList[iIndex].dwCust_1 = stLog.dwCust_1;
	pShmLog->sLogList[iIndex].dwCust_2 = stLog.dwCust_2;
	pShmLog->sLogList[iIndex].iCust_3 = stLog.iCust_3;
	pShmLog->sLogList[iIndex].iCust_4 = stLog.iCust_4;
	if(stLog.szCust_5[0] == '\0')
		pShmLog->sLogList[iIndex].szCust_5[0] = '\0';
	else
		memcpy(pShmLog->sLogList[iIndex].szCust_5, stLog.szCust_5, MYSIZEOF(pShmLog->sLogList[iIndex].szCust_5));
	if(stLog.szCust_6[0] == '\0')
		pShmLog->sLogList[iIndex].szCust_6[0] = '\0';
	else
		memcpy(pShmLog->sLogList[iIndex].szCust_6, stLog.szCust_6, MYSIZEOF(pShmLog->sLogList[iIndex].szCust_6));

	pShmLog->sLogList[iIndex].qwLogTime = stLog.qwLogTime; 
	pShmLog->sLogList[iIndex].wLogType = stLog.wLogType; 
	pShmLog->sLogList[iIndex].dwLogConfigId = stLog.dwLogConfigId; 
	pShmLog->sLogList[iIndex].dwLogHost = stLog.dwLogHost; 

	int32_t iWrite = (int32_t)MYSTRLEN(stLog.pszLog);
	if(iWrite < BWORLD_MEMLOG_BUF_LENGTH)
	{
		pShmLog->sLogList[iIndex].iContentIndex = -1;
		strcpy(pShmLog->sLogList[iIndex].sLogContent, stLog.pszLog);
	}
	else
	{
		memcpy(pShmLog->sLogList[iIndex].sLogContent, stLog.pszLog, BWORLD_MEMLOG_BUF_LENGTH);

		if(m_iVmemShmKey <= 0) {
			// vmem 未启用
			pShmLog->sLogList[iIndex].sLogContent[BWORLD_MEMLOG_BUF_LENGTH-1] = '\0';
			pShmLog->sLogList[iIndex].iContentIndex = -1; 
		}
		else {
			// 4 : 用于存储校验，即 sLogContent 的最后 4 字节要等于 vmem 中的前面 4 字节
			iWrite = MtReport_SaveToVmem(stLog.pszLog+BWORLD_MEMLOG_BUF_LENGTH-4, iWrite-BWORLD_MEMLOG_BUF_LENGTH+4);
			if(iWrite < 0)
			{
				MtReport_Attr_Add(87, 1);
				ERR_LOG("MtReport_SaveToVmem failed, ret:%d, lost log info -- (appid:%d, moduleid:%d, loglen:%u)",
						iWrite, stLog.iAppId, stLog.iModuleId, (unsigned)MYSTRLEN(stLog.pszLog));

				// 超长 log 改为失败 log ，保留部分 log 信息
				pShmLog->sLogList[iIndex].wLogType = SLOG_TYPE_ERROR;
				pShmLog->sLogList[iIndex].sLogContent[BWORLD_MEMLOG_BUF_LENGTH-1] = '\0';
				pShmLog->sLogList[iIndex].iContentIndex = -1; 
			}
			else
				pShmLog->sLogList[iIndex].iContentIndex = iWrite;
		}
	}

	ModifyLogStartIndexCmpAndSwap(pShmLog, -1, iIndex);

	// modify by rock -- seq 最后设置，作为数据完全写入标志
	pShmLog->sLogList[iIndex].dwLogSeq = dwSeq;

	if(m_iRemoteLogToStd)
		printf("remote log info appid:%d module id:%d ConfigId:%u Remote addr:%s logtype:%d ---- \n\t\t%s\n",
			stLog.iAppId, stLog.iModuleId, stLog.dwLogConfigId, 
			ipv4_addr_str(stLog.dwLogHost), stLog.wLogType, stLog.pszLog);
}


TSLogShm* CSupperLog::GetAppLogShm(AppInfo *pAppShmInfo, bool bTryCreate)
{
	// 不属于本机的 app 不分派, 30 是监控系统本身的 appid 需要放开
	if(!IsIpMatchLocalMachine(pAppShmInfo->dwAppSrvMaster) && pAppShmInfo->iAppId != 30)
	{
		WARN_LOG("app:%d log server:%s, not match local machine", 
			pAppShmInfo->iAppId, ipv4_addr_str(pAppShmInfo->dwAppSrvMaster));
		return NULL;
	}

	TSLogShm * pShmLog = NULL;
	int iLogShmSize = BWORLD_MAX_SHM_SLOG_COUNT*MYSIZEOF(TSLog) + MYSIZEOF(TSLogShm);

	// shm log key 探测算法
	int32_t iShmKey = pAppShmInfo->iAppLogShmKey;
	if(bTryCreate && iShmKey == 0) 
	{
		if(!__sync_bool_compare_and_swap(&pAppShmInfo->bTryAppLogShmFlag, 0, 1))
		{
			usleep(1000);
			if(pAppShmInfo->iAppLogShmKey == 0 
				&& IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_ADD)
				&& IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_SUB)
				&& IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_MUL)
				&& IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_DEV)
				&& IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_MOD))
			{
				ERR_LOG("get app log shm failed - appid:%d, flag:%u", 
					pAppShmInfo->iAppId, pAppShmInfo->dwAppLogFlag);
				return NULL;
			}
			else if(pAppShmInfo->iAppLogShmKey != 0)
			{
				iShmKey = pAppShmInfo->iAppLogShmKey;
				INFO_LOG("get app log shm by other, appid:%d, flag:%u, iAppLogShmKey:%d",
					pAppShmInfo->iAppId, pAppShmInfo->dwAppLogFlag, iShmKey);
			}
			else {
				// 某个探测程序中途被终止了，可能跑到这里
				INFO_LOG("get app log shm failed, appid:%d, flag:%u, try:%d",
					pAppShmInfo->iAppId, pAppShmInfo->dwAppLogFlag, pAppShmInfo->bTryAppLogShmFlag);
				pAppShmInfo->bTryAppLogShmFlag = 0;
				return NULL;
			}
		}
		else
		{
			CLEAR_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_ADD);
			CLEAR_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_SUB);
			CLEAR_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_MUL);
			CLEAR_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_DEV);
			CLEAR_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_MOD);
		}
	}

	do {
		if(iShmKey==0 && bTryCreate && !(IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_ADD))) {
			SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_ADD);
			iShmKey = SLOG_APP_LOG_SHM_KEY_BASE + pAppShmInfo->iAppId;
		}
		else if(iShmKey==0 && bTryCreate && !(IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_SUB))) {
			SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_SUB);
			iShmKey = SLOG_APP_LOG_SHM_KEY_BASE - pAppShmInfo->iAppId;
		}
		else if(iShmKey==0 && bTryCreate && !(IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_MUL))) {
			SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_MUL);
			iShmKey = SLOG_APP_LOG_SHM_KEY_BASE * pAppShmInfo->iAppId;
		}
		else if(iShmKey==0 && bTryCreate && !(IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_DEV))) {
			SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_DEV);
			iShmKey = SLOG_APP_LOG_SHM_KEY_BASE / pAppShmInfo->iAppId;
		}
		else if(iShmKey==0 && bTryCreate && !(IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_MOD))) {
			SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FLAG_SHMKEY_USE_MOD);
			iShmKey = SLOG_APP_LOG_SHM_KEY_BASE % pAppShmInfo->iAppId;
		}

		if(iShmKey==0) {
			break;
		}

		if(!(pShmLog=(TSLogShm*)GetShm(iShmKey, iLogShmSize, 0666)) && bTryCreate) {
			// 尝试创建
			if(!(pShmLog=(TSLogShm*)GetShm(iShmKey, iLogShmSize, 0666|IPC_CREAT))) {
				WARN_LOG("try get log shm failed, shmkey:%d, size:%u, appid:%d",
					iShmKey, iLogShmSize, pAppShmInfo->iAppId);
				iShmKey = 0;
				continue;
			}
			else {
				memset(pShmLog, 0, iLogShmSize);

				// key 探测成功，设置key
				pAppShmInfo->iAppLogShmKey = iShmKey;
				pShmLog->iLogMaxCount = BWORLD_MAX_SHM_SLOG_COUNT;
				pShmLog->iLogStarIndex = -1;
				pShmLog->iAppId = pAppShmInfo->iAppId;
				if(slog.m_bInit) 
					INFO_LOG("create app log shm ok key:%d size:%d, appid:%d",
						iShmKey, iLogShmSize, pAppShmInfo->iAppId);
				pAppShmInfo->bTryAppLogShmFlag = 0;
				return pShmLog;
			}
		}

		if(pShmLog == NULL)
		{
			WARN_LOG("try get log shm failed, shmkey:%d, size:%u, flag:%#x, appid:%d",
				iShmKey, iLogShmSize, pAppShmInfo->dwAppLogFlag, pAppShmInfo->iAppId);
			break;
		}

		if(pShmLog != NULL) 
		{
			// attach 上已有的
			if(slog.m_bInit)  {
				INFO_LOG("attach app log shm ok key:%d size:%d appid:%d",
					iShmKey, iLogShmSize, pAppShmInfo->iAppId);
			}

			// app 删除后又从DB 启用会出现这种情况
			if(pAppShmInfo->iAppLogShmKey != iShmKey && bTryCreate)
			{
				pAppShmInfo->iAppLogShmKey = iShmKey;
				pShmLog->iLogMaxCount = BWORLD_MAX_SHM_SLOG_COUNT;
				pShmLog->iLogStarIndex = -1;
				pShmLog->iAppId = pAppShmInfo->iAppId;
			}
			pAppShmInfo->bTryAppLogShmFlag = 0;
			return pShmLog;
		}
	}while(true);
	pAppShmInfo->bTryAppLogShmFlag = 0;
	MtReport_Attr_Add(88, 1);
	return NULL;
}

void CSupperLog::ShmLog(int wLogType , const char *pszFmt, ...)
{
	int32_t iWrite = 0, iIndex = 0;
	bool bLogFull = false;
	uint32_t dwSeq = 0;

	// slog_config 首次初始化的时候会出现
	if(m_pShmLog == NULL)
		return;

	// 提前判断缓冲区是否满
	if(InitGetShmLogIndex(m_pShmLog) < 0)
		return;
	iIndex = m_pShmLog->iWriteIndex;
	// 缓存区满时，丢弃新日志，不改变 iWriteIndex
	if(iIndex == m_pShmLog->iLogStarIndex)
		bLogFull = true;
	else 
	{
		SLOG_NEXT_INDEX(iIndex, m_pShmLog->iWriteIndex);
		// dwSeq 用于实时日志获取、历史日志读取校验，用服务器上的
		dwSeq = m_pShmLog->dwLogSeq++;
		if(0 == dwSeq)
		{
			dwSeq = 1;
			m_pShmLog->dwLogSeq = 2;
		}
		// 先设置为 0
		m_pShmLog->sLogList[iIndex].dwLogSeq = 0;
	}
	EndGetShmLogIndex(m_pShmLog);

	if(bLogFull)
	{
		// 将要写的数组元素 seq 不为 0 或者条数超过最大值, 日志写满了
		SLOG_WARN_SHM_WRITE_FULL;
		return ;
	}

	memset(m_sLogBuf, 0, MYSIZEOF(m_sLogBuf));
	struct timeval stNow;
	gettimeofday(&stNow, 0);
	if(m_stParam.iIsLogTime)
	{
		struct tm stTm;
		localtime_r(&stNow.tv_sec, &stTm);
		iWrite += strftime(m_sLogBuf, MYSIZEOF(m_sLogBuf), "%Y-%m-%d %H:%M:%S", &stTm);
		iWrite += snprintf(m_sLogBuf + iWrite, MYSIZEOF(m_sLogBuf) - iWrite, ".%06u ", (uint32_t)stNow.tv_usec);
	}

	va_list ap;
	va_start(ap, pszFmt);
	int iTmp = vsnprintf(m_sLogBuf+iWrite, MYSIZEOF(m_sLogBuf)-iWrite-TOO_LONG_TRUNC_STR_LEN, pszFmt, ap);
	va_end(ap);

	if(iTmp < 0)
		strcat(m_sLogBuf+iWrite, "vsnprintf error !");
	else if(iTmp >= (int)(MYSIZEOF(m_sLogBuf)-iWrite-TOO_LONG_TRUNC_STR_LEN))
		strcat(m_sLogBuf+MYSIZEOF(m_sLogBuf)-TOO_LONG_TRUNC_STR_LEN, TOO_LONG_TRUNC_STR);
	iWrite = MYSTRLEN(m_sLogBuf);

	m_pShmLog->sLogList[iIndex].iAppId = m_iLogAppId;
	m_pShmLog->sLogList[iIndex].iModuleId = m_iLogModuleId;
	m_pShmLog->sLogList[iIndex].dwCust_1 = m_dwCust_1;
	m_pShmLog->sLogList[iIndex].dwCust_2 = m_dwCust_2;
	m_pShmLog->sLogList[iIndex].iCust_3 = m_iCust_3;
	m_pShmLog->sLogList[iIndex].iCust_4 = m_iCust_4;
	if(m_szCust_5[0] == '\0')
		m_pShmLog->sLogList[iIndex].szCust_5[0] = '\0';
	else
		memcpy(m_pShmLog->sLogList[iIndex].szCust_5, m_szCust_5, MYSIZEOF(m_szCust_5));
	if(m_szCust_6[0] == '\0')
		m_pShmLog->sLogList[iIndex].szCust_6[0] = '\0';
	else
		memcpy(m_pShmLog->sLogList[iIndex].szCust_6, m_szCust_6, MYSIZEOF(m_szCust_6));

	m_pShmLog->sLogList[iIndex].dwLogConfigId = m_dwConfigId;

	if(m_pShmConfig != NULL)
		m_pShmLog->sLogList[iIndex].dwLogHost = m_pShmConfig->stSysCfg.iMachineId;
	else
		m_pShmLog->sLogList[iIndex].dwLogHost = 0;

	m_pShmLog->sLogList[iIndex].wLogType = wLogType; 
	m_pShmLog->sLogList[iIndex].qwLogTime = stNow.tv_sec*1000000ULL+stNow.tv_usec;
	if(iWrite < BWORLD_MEMLOG_BUF_LENGTH)
	{
		m_pShmLog->sLogList[iIndex].iContentIndex = -1;
		strcpy(m_pShmLog->sLogList[iIndex].sLogContent, m_sLogBuf);
	}
	else
	{
		memcpy(m_pShmLog->sLogList[iIndex].sLogContent, m_sLogBuf, BWORLD_MEMLOG_BUF_LENGTH);
		if(m_iVmemShmKey <= 0) {
			// vmem 未启用
			m_pShmLog->sLogList[iIndex].sLogContent[BWORLD_MEMLOG_BUF_LENGTH-1] = '\0';
			m_pShmLog->sLogList[iIndex].iContentIndex = -1; 
		}
		else {
			// 4 : 用于存储校验，即 sLogContent 的最后 4 字节要等于 vmem 中的前面 4 字节
			iWrite = MtReport_SaveToVmem(m_sLogBuf+BWORLD_MEMLOG_BUF_LENGTH-4, iWrite-BWORLD_MEMLOG_BUF_LENGTH+4);
			if(iWrite < 0)
			{
				MtReport_Attr_Add(87, 1);
				snprintf(m_pShmLog->sLogList[iIndex].sLogContent, BWORLD_MEMLOG_BUF_LENGTH-1,
						BWORLD_SLOG_BASE_FMT"write shm log failed, MtReport_SaveToVmem ret:%d(appid:%d, moduleid:%d, loglen:%u)", 
						BWORLD_SLOG_BASE_VAL, iWrite, m_iLogAppId, m_iLogModuleId, MYSTRLEN(m_sLogBuf));
				m_pShmLog->sLogList[iIndex].wLogType = SLOG_TYPE_ERROR;
				m_pShmLog->sLogList[iIndex].iContentIndex = -1; 
			}
			else
				m_pShmLog->sLogList[iIndex].iContentIndex = iWrite;
		}
	}

	// modify by rock -- seq 最后设置，作为数据完全写入标志
	m_pShmLog->sLogList[iIndex].dwLogSeq = dwSeq;
	ModifyLogStartIndexCmpAndSwap(m_pShmLog, -1, iIndex);
}

void CSupperLog::error(ISocketHandler *h, Socket *sock, 
	const std::string& call, int err, const std::string& sys_err, loglevel_t lvl)
{
	if(lvl == LOG_LEVEL_INFO)
		return;

	switch (lvl)
	{   
		case LOG_LEVEL_WARNING:
			if(sock)
				WARN_LOG("fd %d :: %s: %d %s", sock->GetSocket(), call.c_str(), err, sys_err.c_str());
			else
				WARN_LOG("%s: %d %s", call.c_str(), err, sys_err.c_str());
			break;

		case LOG_LEVEL_ERROR:
			if(sock)
				ERR_LOG("fd %d :: %s: %d %s", 
					sock->GetSocket(), call.c_str(), err, sys_err.c_str());
			else
				ERR_LOG("%s: %d %s", call.c_str(), err, sys_err.c_str());
			break;

		case LOG_LEVEL_FATAL:
			if(sock)
				FATAL_LOG("fd %d :: %s: %d %s", 
					sock->GetSocket(), call.c_str(), err, sys_err.c_str());
			else
				FATAL_LOG("%s: %d %s", call.c_str(), err, sys_err.c_str());
			break;

		case LOG_LEVEL_INFO:
			if(sock)
				INFO_LOG("fd %d :: %s: %d %s", 
					sock->GetSocket(), call.c_str(), err, sys_err.c_str());
			else
				INFO_LOG("%s: %d %s", call.c_str(), err, sys_err.c_str());
			break;
	}
}

void CSupperLog::debug(Database& db, const std::string& errmsg)
{
	DEBUG_LOG("from database :%s", errmsg.c_str());
}

void CSupperLog::info(Database& db, const std::string& errmsg)
{
	INFO_LOG("from database :%s", errmsg.c_str());
}

void CSupperLog::error(Database& db, const std::string& errmsg)
{
	WARN_LOG("from database errmsg:%s", errmsg.c_str());
}

void CSupperLog::error(Database& db, Query& qu, const std::string& errmsg)
{
	WARN_LOG("from database query errmsg:%s", errmsg.c_str());
}

// class CSLogServerWriteFile 
// ------------------------------------------------------------------------------------------------------
CSLogServerWriteFile::CSLogServerWriteFile(AppInfo *pAppInfo, const char *pszLogPath, int iScanAgain)
{
	m_bInit = false;
	memset(m_szLogFilePath, 0, MYSIZEOF(m_szLogFilePath));
	snprintf(m_szLogFilePath, MYSIZEOF(m_szLogFilePath)-1, "%s%d/", pszLogPath, pAppInfo->iAppId);
	m_iScanAgain = iScanAgain;
	m_pAppInfo = pAppInfo;
	m_iAppId = pAppInfo->iAppId;

	if(Init() < 0)
		return;
	m_bInit = true;

	// 更新下时间--slog_write 写进程分发逻辑需要
	pAppInfo->dwLastTryWriteLogTime = time(NULL);
}

CSLogServerWriteFile::~CSLogServerWriteFile()
{
	if(m_pShmLog != NULL)
		shmdt(m_pShmLog);
	if(m_fpLogFile != NULL)
		fclose(m_fpLogFile);
	if(m_pstLogFileList != NULL)
		shmdt(m_pstLogFileList);
}

SLogFile * CSLogServerWriteFile::GetAppLogFileShm(AppInfo *pAppShmInfo, bool bTryCreate)
{
	SLogFile * pShmLogFile = NULL;

	if(!slog.IsIpMatchLocalMachine(pAppShmInfo->dwAppSrvMaster))
	{
		WARN_LOG("app id:%d, get log file shm, not match local machine", pAppShmInfo->iAppId);
		return NULL;
	}

	// shm log key 探测算法
	int32_t iShmKey = pAppShmInfo->iAppLogFileShmKey;
	if(bTryCreate && iShmKey == 0)
	{
		if(!__sync_bool_compare_and_swap(&pAppShmInfo->bTryAppLogFileShmFlag, 0, 1))
		{
			usleep(100);
			if(pAppShmInfo->iAppLogFileShmKey == 0 
				&& IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_ADD)
				&& IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_SUB)
				&& IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_MUL)
				&& IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_DEV)
				&& IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_MOD))
			{
				ERR_LOG("get app log file shm failed - appid:%d, flag:%u", 
					pAppShmInfo->iAppId, pAppShmInfo->dwAppLogFlag);
				return NULL;
			}
			else if(pAppShmInfo->iAppLogFileShmKey != 0)
			{
				iShmKey = pAppShmInfo->iAppLogFileShmKey;
				INFO_LOG("get app log file shm by other, appid:%d, flag:%u, iAppLogFileShmKey:%d",
					pAppShmInfo->iAppId, pAppShmInfo->dwAppLogFlag, iShmKey);
			}
			else {
				// 某个探测程序中途被终止了，可能跑到这里
				INFO_LOG("get app log file shm failed, appid:%d, flag:%u",
					pAppShmInfo->iAppId, pAppShmInfo->dwAppLogFlag);
				pAppShmInfo->bTryAppLogFileShmFlag = 0;
				return NULL;
			}
		}
		else
		{
			CLEAR_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_ADD);
			CLEAR_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_SUB);
			CLEAR_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_MUL);
			CLEAR_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_DEV);
			CLEAR_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_MOD);
		}
	}

	do {
		if(iShmKey==0 && bTryCreate 
			&& !(IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_ADD))) {
			SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_ADD);
			iShmKey = SLOG_APP_LOGFILE_SHM_KEY_BASE + pAppShmInfo->iAppId;
		}
		else if(iShmKey==0 && bTryCreate 
			&& !(IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_SUB))) {
			SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_SUB);
			iShmKey = SLOG_APP_LOGFILE_SHM_KEY_BASE - pAppShmInfo->iAppId;
		}
		else if(iShmKey==0 && bTryCreate 
			&& !(IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_MUL))) {
			SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_MUL);
			iShmKey = SLOG_APP_LOGFILE_SHM_KEY_BASE * pAppShmInfo->iAppId;
		}
		else if(iShmKey==0 && bTryCreate 
			&& !(IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_DEV))) {
			SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_DEV);
			iShmKey = SLOG_APP_LOGFILE_SHM_KEY_BASE / pAppShmInfo->iAppId;
		}
		else if(iShmKey==0 && bTryCreate 
			&& !(IS_SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_MOD))) {
			SET_BIT(pAppShmInfo->dwAppLogFlag, APPLOG_FILE_FLAG_SHMKEY_USE_MOD);
			iShmKey = SLOG_APP_LOGFILE_SHM_KEY_BASE % pAppShmInfo->iAppId;
		}

		if(iShmKey==0) {
			break;
		}

		if(!(pShmLogFile=(SLogFile*)GetShm(iShmKey, MYSIZEOF(SLogFile), 0666)) && bTryCreate) {
			// 尝试创建
			if(!(pShmLogFile=(SLogFile*)GetShm(iShmKey, MYSIZEOF(SLogFile), 0666|IPC_CREAT))) {
				WARN_LOG("try get log file shm failed, shmkey:%d, size:%u, appid:%d",
					iShmKey, MYSIZEOF(SLogFile), pAppShmInfo->iAppId);
				iShmKey = 0;
				continue;
			}
			else {
				memset(pShmLogFile, 0, MYSIZEOF(SLogFile));
				pAppShmInfo->iAppLogFileShmKey = iShmKey;
				pShmLogFile->iAppId = pAppShmInfo->iAppId;
				pAppShmInfo->bTryAppLogFileShmFlag = 0;
				INFO_LOG("create app log file shm ok key:%d size:%d appid:%d flag:%#x",
					iShmKey, MYSIZEOF(SLogFile), pAppShmInfo->iAppId, pAppShmInfo->dwAppLogFlag);
				return pShmLogFile;
			}
		}

		if(pShmLogFile == NULL)
		{
			WARN_LOG("try get log file shm failed, shmkey:%d, size:%u, flag:%#x, appid:%d",
				iShmKey, MYSIZEOF(SLogFile), pAppShmInfo->dwAppLogFlag, pAppShmInfo->iAppId);
			break;
		}
		
		if(pShmLogFile != NULL)
		{
			// attach 上已有的
			INFO_LOG("attach app log file shm ok key:%d size:%u appid:%d",
				iShmKey, MYSIZEOF(SLogFile), pAppShmInfo->iAppId);
			if(iShmKey != pAppShmInfo->iAppLogFileShmKey && bTryCreate)
			{
				pAppShmInfo->iAppLogFileShmKey = iShmKey;
				pShmLogFile->iAppId = pAppShmInfo->iAppId;
			}
			pAppShmInfo->bTryAppLogFileShmFlag = 0;
			return pShmLogFile;
		}
	}while(true);
	MtReport_Attr_Add(88, 1);
	pAppShmInfo->bTryAppLogFileShmFlag = 0;
	return NULL;
}

int CSLogServerWriteFile::Init()
{
	m_fpLogFile = NULL;
	m_wCurFpLogFileIndex = SLOG_LOG_FILES_COUNT_MAX;
	m_pShmLog = NULL;
	m_pstLogFileList = NULL;

	if(AttachShm() < 0)
		return -2;
	if(InitLogFiles() < 0)
		return -3;
	return 0;
}

void CSLogServerWriteFile::ShowFileShmInfo(bool bLogContent)
{
	static char sLogBuf[BWORLD_SLOG_MAX_LINE_LEN+64];
	FILE *fp = NULL;
	printf("total :%d slog files ------- \n", m_pstLogFileList->wLogFileCount);
	for(int i=0; i < m_pstLogFileList->wLogFileCount; i++)
	{
		printf("file:%d:%s records:%d time:%" PRIu64 "-%" PRIu64 "\n", i,
			m_pstLogFileList->stFiles[i].szAbsFileName, 
			m_pstLogFileList->stFiles[i].stFileHead.iLogRecordsWrite,
			m_pstLogFileList->stFiles[i].stFileHead.qwLogTimeStart,
			m_pstLogFileList->stFiles[i].stFileHead.qwLogTimeEnd);

		if(bLogContent)
		{
			fp = fopen(m_pstLogFileList->stFiles[i].szAbsFileName, "rb");
			if(NULL == fp)
			{
				printf("open file:%s failed , msg:%s\n", 
					m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
				continue;
			}
			SLogFileHead stFileHead;
			if(fread(&stFileHead, MYSIZEOF(stFileHead), 1, fp) != 1)
			{
				printf("read slog file head from file:%s failed, msg:%s !\n", 
					m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
				fclose(fp);
				continue;
			} 
			printf("file head info in file --- records:%d time:%" PRIu64 "-%" PRIu64 "\n",
				stFileHead.iLogRecordsWrite, stFileHead.qwLogTimeStart, stFileHead.qwLogTimeEnd);
			SLogFileLogIndex stIndex;
			for(int j=0;  j < stFileHead.iLogRecordsWrite; j++)
			{
				if(fseek(fp, MYSIZEOF(SLogFileHead)+MYSIZEOF(stIndex)*j, SEEK_SET) < 0)
				{
					printf("seek record:%d index failed msg:%s file:%s\n", 
						j, strerror(errno), m_pstLogFileList->stFiles[i].szAbsFileName);
					break;
				}
				if(fread(&stIndex, MYSIZEOF(SLogFileLogIndex), 1, fp) != 1)
				{
					printf("read slog file index from file:%s failed, msg:%s !\n", 
						m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
					break;
				}
	
				printf("log record info - appid:%d moduleid:%d logtype:%d seq:%u time:%" PRIu64 
					" contentlen:%u pos:%u\n", stIndex.iAppId, stIndex.iModuleId, stIndex.wLogType, 
					stIndex.dwLogSeq, stIndex.qwLogTime, stIndex.dwLogContentLen, stIndex.dwLogContentPos);

				if(fseek(fp, stIndex.dwLogContentPos, SEEK_SET) < 0)
				{
					printf("seek record:%d content failed msg:%s file:%s\n", 
						j, strerror(errno), m_pstLogFileList->stFiles[i].szAbsFileName);
					break;
				}

				uint32_t dwContentSeq = 0;
				if(fread(&dwContentSeq, 4, 1, fp) != 1)
				{
					printf("read content seq failed from file:%s , msg:%s !\n",
						m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
					break;
				}
				if(dwContentSeq != stIndex.dwLogSeq)
				{
					printf("from slog file:%s content seq:%u != %u check content failed !\n", 
						m_pstLogFileList->stFiles[i].szAbsFileName, dwContentSeq, stIndex.dwLogSeq);
					break;
				}

				if(fread(sLogBuf, 1, stIndex.dwLogContentLen, fp) != stIndex.dwLogContentLen)
				{
					printf("read slog log contenet from file:%s failed, msg:%s !\n", 
						m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
					break;
				}
				printf("log record content - seq:%u msg:%s\n", stIndex.dwLogSeq, sLogBuf);
			}
			fclose(fp);
		}
	}
}

static int LogFileCmp(const void *p1, const void *p2)
{
	const SLogFileInfo *plog1 = (const SLogFileInfo*)p1;
	const SLogFileInfo *plog2 = (const SLogFileInfo*)p2;
	return strcmp(plog1->szAbsFileName, plog2->szAbsFileName);
}

bool CSLogServerWriteFile::IsAppFileHeadReadAll()
{
	for(int i =0; i < m_pstLogFileList->wLogFileCount; i++)
	{
		if(m_pstLogFileList->stFiles[i].stFileHead.stLogStatInfo.qwLogSizeInfo <= 0)
			return false;
	}
	return true;
}

int CSLogServerWriteFile::InitLogFiles()
{
	if(m_szLogFilePath[0] == '\0' || NULL == m_pstLogFileList)
	{
		ERR_LOG("log file path or file shm is NULL !");
		return -101;
	}

	bool bReadAllFileHeadOk = IsAppFileHeadReadAll();

	// 共享内存中的文件列表已经初始化过了
	if(m_iScanAgain==0 && m_pstLogFileList->wLogFileCount > 0 
		&& m_pAppInfo != NULL && m_pAppInfo->bReadLogStatInfo && bReadAllFileHeadOk)
	{
		INFO_LOG("shm file count:%d, not scan again", m_pstLogFileList->wLogFileCount);
		m_wCurFpLogFileIndex = m_pstLogFileList->wLogFileCount-1;
		if(m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].stFileHead.iLogRecordsMax
				>= SLOG_LOG_RECORDS_COUNT_MAX){
			m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].stFileHead.iLogRecordsMax
				= SLOG_LOG_RECORDS_COUNT_MAX;
		}
		return m_pstLogFileList->wLogFileCount;
	}

	struct dirent **namelist = NULL;
	int iFileCount = scandir(m_szLogFilePath, &namelist, NULL, alphasort);
	if(iFileCount <= 0)
	{
		// 可能不存在，尝试创建目录
		WARN_LOG("scandir:%s failed ret:%d, msg:%s", m_szLogFilePath, iFileCount, strerror(errno));
		char sBuf[512] = {0};
		snprintf(sBuf, MYSIZEOF(sBuf)-1, "mkdir -p %s", m_szLogFilePath);
		system(sBuf);
		m_pstLogFileList->wLogFileCount = 0;
		return 0;
	}
	else
	{
		uint32_t dwStartSec, dwStartUsec;
		int i = 0, iNameLen = 0;
		for(int j=iFileCount-1; j >= 0 && i < SLOG_LOG_FILES_COUNT_MAX; j--)
		{
			if(!S_ISDIR(namelist[j]->d_type) && isdigit( namelist[j]->d_name[0] ) && namelist[j]->d_name[0]!='.')
			{
				if(sscanf(namelist[j]->d_name, "%u_%u", &dwStartSec, &dwStartUsec) != 2)
				{
					WARN_LOG("file:%s%s not slog file!", m_szLogFilePath, namelist[j]->d_name);
					free(namelist[j]);
					continue;
				}

				iNameLen = snprintf(m_pstLogFileList->stFiles[i].szAbsFileName, 
					MYSIZEOF(m_pstLogFileList->stFiles[i].szAbsFileName)-1, 
					"%s%s", m_szLogFilePath, namelist[j]->d_name);
				if(iNameLen > (int)(MYSIZEOF(m_pstLogFileList->stFiles[i].szAbsFileName)-1))
				{
					ERR_LOG("file:%s%s name len:%d too long or buff:%u too small",
						m_szLogFilePath, namelist[j]->d_name,
						iNameLen, MYSIZEOF(m_pstLogFileList->stFiles[i].szAbsFileName));
					free(namelist[j]);
					continue;
				}
				memset(&(m_pstLogFileList->stFiles[i].stFileHead), 0, MYSIZEOF(SLogFileHead));
				m_pstLogFileList->stFiles[i].qwLogTimeStart = TIME_SEC_TO_USEC(dwStartSec)+dwStartUsec;
				INFO_LOG("file: %s is slog file", m_pstLogFileList->stFiles[i].szAbsFileName);
				i++;
			}
			else
				DEBUG_LOG("file: %s%s is not slog file", m_szLogFilePath, namelist[j]->d_name);
			free(namelist[j]);
		}
		m_pstLogFileList->wLogFileCount = i;
		if(namelist)
			free(namelist);
		qsort(m_pstLogFileList->stFiles, m_pstLogFileList->wLogFileCount, sizeof(SLogFileInfo), LogFileCmp);

		// 读取全部文件头部, 文件统计信息写入 app 共享内存
		if(m_pAppInfo != NULL && (!m_pAppInfo->bReadLogStatInfo || !!bReadAllFileHeadOk))
		{
			memset(&m_pAppInfo->stLogStatInfo, 0, sizeof(TLogStatInfo));
			for(int i =0; i < m_pstLogFileList->wLogFileCount; i++)
			{
				SLogFileHead & stFileHead = m_pstLogFileList->stFiles[i].stFileHead;
				if(ReadLogFileHead(i) >= 0) {
					m_pAppInfo->stLogStatInfo.qwLogSizeInfo += stFileHead.stLogStatInfo.qwLogSizeInfo;
					m_pAppInfo->stLogStatInfo.dwDebugLogsCount += stFileHead.stLogStatInfo.dwDebugLogsCount;
					m_pAppInfo->stLogStatInfo.dwInfoLogsCount += stFileHead.stLogStatInfo.dwInfoLogsCount;
					m_pAppInfo->stLogStatInfo.dwWarnLogsCount += stFileHead.stLogStatInfo.dwWarnLogsCount;
					m_pAppInfo->stLogStatInfo.dwReqerrLogsCount += stFileHead.stLogStatInfo.dwReqerrLogsCount;
					m_pAppInfo->stLogStatInfo.dwErrorLogsCount += stFileHead.stLogStatInfo.dwErrorLogsCount;
					m_pAppInfo->stLogStatInfo.dwFatalLogsCount += stFileHead.stLogStatInfo.dwFatalLogsCount;
					m_pAppInfo->stLogStatInfo.dwOtherLogsCount += stFileHead.stLogStatInfo.dwOtherLogsCount;
				}
			}
			INFO_LOG("read app:%d, file count:%d, log stat info, " 
				"size:%lu, debug:%u, info:%u, warn:%u, reqerr:%u, err:%u, fatal:%u, other:%u",
				m_pstLogFileList->wLogFileCount, m_iAppId, 
				m_pAppInfo->stLogStatInfo.qwLogSizeInfo, m_pAppInfo->stLogStatInfo.dwDebugLogsCount,
				m_pAppInfo->stLogStatInfo.dwInfoLogsCount, m_pAppInfo->stLogStatInfo.dwWarnLogsCount,
				m_pAppInfo->stLogStatInfo.dwReqerrLogsCount, m_pAppInfo->stLogStatInfo.dwErrorLogsCount,
				m_pAppInfo->stLogStatInfo.dwFatalLogsCount, m_pAppInfo->stLogStatInfo.dwOtherLogsCount);
			m_pAppInfo->bReadLogStatInfo = true;
		}
		else if(m_pstLogFileList->wLogFileCount > 0)
		{
		    ReadLogFileHead(m_pstLogFileList->wLogFileCount-1);
		}
	}

	if(m_pstLogFileList->wLogFileCount > 0)
		    m_wCurFpLogFileIndex = m_pstLogFileList->wLogFileCount - 1;

	INFO_LOG("scandir:%s log file count:%d", m_szLogFilePath, m_pstLogFileList->wLogFileCount);
	return m_pstLogFileList->wLogFileCount;
}


void CSLogServerWriteFile::RemoveFileInfo(int iFileIndex)
{
    INFO_LOG("remove file shm info --  file:%s -- index:%d",
        m_pstLogFileList->stFiles[iFileIndex].szAbsFileName, iFileIndex);

    // 物理删除
    char szSysCmdRemove[256];
    snprintf(szSysCmdRemove, sizeof(szSysCmdRemove), "rm -f %s", m_pstLogFileList->stFiles[iFileIndex].szAbsFileName);
    system(szSysCmdRemove);

    // shm 统计信息中删除统计信息
    if(m_pAppInfo != NULL && m_pAppInfo->bReadLogStatInfo) {
        SLogFileHead & stFileHead = m_pstLogFileList->stFiles[iFileIndex].stFileHead;
        if(stFileHead.stLogStatInfo.qwLogSizeInfo > m_pAppInfo->stLogStatInfo.qwLogSizeInfo)
        {
            WARN_LOG("check app:%d log size failed:%lu > %lu", m_pAppInfo->iAppId,
                stFileHead.stLogStatInfo.qwLogSizeInfo, m_pAppInfo->stLogStatInfo.qwLogSizeInfo);
            m_pAppInfo->stLogStatInfo.qwLogSizeInfo = 0;
        }
        else {
            m_pAppInfo->stLogStatInfo.qwLogSizeInfo -= stFileHead.stLogStatInfo.qwLogSizeInfo;
        }
#define CHECK_LOG_STAT_INFO(ftype, typemsg) \
        if(m_pAppInfo->stLogStatInfo.ftype < stFileHead.stLogStatInfo.ftype) { \
            ERR_LOG("check app:%d log stat(%s) failed:%u < %u", m_pAppInfo->iAppId, \
                typemsg, m_pAppInfo->stLogStatInfo.ftype, stFileHead.stLogStatInfo.ftype); \
            m_pAppInfo->stLogStatInfo.ftype = 0;  \
        } \
        else  \
            m_pAppInfo->stLogStatInfo.ftype -= stFileHead.stLogStatInfo.ftype; 

        // fix bug -- 2019-04-17
        if(m_pAppInfo->stLogStatInfo.qwLogSizeInfo < stFileHead.stLogStatInfo.qwLogSizeInfo) {
            ERR_LOG("check app log size failed, info:%lu < %lu, app:%d",
                m_pAppInfo->stLogStatInfo.qwLogSizeInfo, stFileHead.stLogStatInfo.qwLogSizeInfo, m_pAppInfo->iAppId);
            m_pAppInfo->stLogStatInfo.qwLogSizeInfo = 0;
        }
        else {
            m_pAppInfo->stLogStatInfo.qwLogSizeInfo -= stFileHead.stLogStatInfo.qwLogSizeInfo;
        }

        CHECK_LOG_STAT_INFO(dwDebugLogsCount, "debug");
        CHECK_LOG_STAT_INFO(dwInfoLogsCount, "info");
        CHECK_LOG_STAT_INFO(dwWarnLogsCount, "warn");
        CHECK_LOG_STAT_INFO(dwReqerrLogsCount, "reqerr");
        CHECK_LOG_STAT_INFO(dwErrorLogsCount, "error");
        CHECK_LOG_STAT_INFO(dwFatalLogsCount, "fatal");
        CHECK_LOG_STAT_INFO(dwOtherLogsCount, "other");
#undef CHECK_LOG_STAT_INFO
        INFO_LOG("remove slog file:%s, app:%d, stat info - size:%lu|%lu, debug:%u|%u, info:%u|%u, warn:%u|%u"
            ", reqerr:%u|%u, error:%u|%u, fatal:%u|%u, other:%u|%u",
            m_pstLogFileList->stFiles[iFileIndex].szAbsFileName, m_pAppInfo->iAppId,
            m_pAppInfo->stLogStatInfo.qwLogSizeInfo, stFileHead.stLogStatInfo.qwLogSizeInfo,
            m_pAppInfo->stLogStatInfo.dwDebugLogsCount, stFileHead.stLogStatInfo.dwDebugLogsCount,
            m_pAppInfo->stLogStatInfo.dwInfoLogsCount, stFileHead.stLogStatInfo.dwInfoLogsCount,
            m_pAppInfo->stLogStatInfo.dwWarnLogsCount, stFileHead.stLogStatInfo.dwWarnLogsCount,
            m_pAppInfo->stLogStatInfo.dwReqerrLogsCount, stFileHead.stLogStatInfo.dwReqerrLogsCount,
            m_pAppInfo->stLogStatInfo.dwErrorLogsCount, stFileHead.stLogStatInfo.dwErrorLogsCount,
            m_pAppInfo->stLogStatInfo.dwFatalLogsCount, stFileHead.stLogStatInfo.dwFatalLogsCount,
            m_pAppInfo->stLogStatInfo.dwOtherLogsCount, stFileHead.stLogStatInfo.dwOtherLogsCount);
    }
    else {
        WARN_LOG("check app info shm failed");
    }

    int i = iFileIndex, j = iFileIndex+1;
    for(; j < m_pstLogFileList->wLogFileCount; i++, j++)
        memcpy(&(m_pstLogFileList->stFiles[i]), &(m_pstLogFileList->stFiles[j]), MYSIZEOF(SLogFileInfo));
    m_pstLogFileList->wLogFileCount--;
    if(m_pstLogFileList->wLogFileCount > 0)
        m_wCurFpLogFileIndex = m_pstLogFileList->wLogFileCount-1;
    else
        m_wCurFpLogFileIndex = SLOG_LOG_FILES_COUNT_MAX;
}

int CSLogServerWriteFile::AddNewSlogFile(uint64_t qwLogStartTime)
{
    if(m_pstLogFileList->wLogFileCount >= SLOG_LOG_FILES_COUNT_MAX)
    {
        INFO_LOG("slog file count :%d, remove first log time:%" PRIu64 "-%" PRIu64 " file:%s",
            m_pstLogFileList->wLogFileCount, m_pstLogFileList->stFiles[0].stFileHead.qwLogTimeStart,
            m_pstLogFileList->stFiles[0].stFileHead.qwLogTimeEnd, m_pstLogFileList->stFiles[0].szAbsFileName);

        if(remove(m_pstLogFileList->stFiles[0].szAbsFileName) < 0)
        {
            ERR_LOG("remove slog file:%s failed ! msg:%s",
                m_pstLogFileList->stFiles[0].szAbsFileName, strerror(errno));
        }
        RemoveFileInfo(0);
    }

    int i=m_pstLogFileList->wLogFileCount;

    sprintf(m_pstLogFileList->stFiles[i].szAbsFileName, "%s%u_%u",
        m_szLogFilePath, (uint32_t)(qwLogStartTime/SEC_USEC), (uint32_t)(qwLogStartTime%SEC_USEC));

    if(m_fpLogFile != NULL)
        fclose(m_fpLogFile);

    m_fpLogFile = fopen(m_pstLogFileList->stFiles[i].szAbsFileName, "wb+");
    if(NULL == m_fpLogFile)
    {
        ERR_LOG("create file: %s failed, msg:%s",
            m_pstLogFileList->stFiles[i].szAbsFileName, strerror(errno));
        RemoveFileInfo(i);
        return -1;
    }

    m_wCurFpLogFileIndex = i;
    m_pstLogFileList->wLogFileCount++;
    m_pstLogFileList->stFiles[i].qwLogTimeStart = qwLogStartTime;

    SLogFileHead &stFileHead = m_pstLogFileList->stFiles[i].stFileHead;
    memset(&stFileHead, 0, sizeof(stFileHead));
    strncpy(stFileHead.szCheckDigit, qwtoa(qwLogStartTime), MYSIZEOF(stFileHead.szCheckDigit));
    stFileHead.wCheckByteOrder = SLOG_CHECK_BYTES_ORDER_NUM;
    stFileHead.qwLogTimeStart = qwLogStartTime;
    stFileHead.qwLogTimeEnd = qwLogStartTime;
    stFileHead.iLogRecordsWrite = 0;
    stFileHead.iLogRecordsMax = SLOG_LOG_RECORDS_COUNT_MAX;
    stFileHead.bLogFileVersion = SLOG_FILE_VERSION_CUR;

    // 预先写入文件头和索引，以便日志内容写到所有日志索引的后面
    if(fwrite(&stFileHead, MYSIZEOF(stFileHead), 1, m_fpLogFile) != 1)
    {
        ERR_LOG("write file head failed msg:%s write size:%u file:%s",
            strerror(errno), MYSIZEOF(stFileHead), m_pstLogFileList->stFiles[i].szAbsFileName);
        return SLOG_ERROR_LINE;
    }
    SLogFileLogIndex stIndex;
    memset(&stIndex, 0, MYSIZEOF(stIndex));


    // 日志记录索引初始化 --- 共有 SLOG_LOG_RECORDS_COUNT_MAX 个索引
    for(int j = 0; j < SLOG_LOG_RECORDS_COUNT_MAX; j++)
    {
        if(fwrite(&stIndex, MYSIZEOF(SLogFileLogIndex), 1, m_fpLogFile) != 1)
        {
            ERR_LOG("first write log index(%d) failed msg:%s write size:%u file:%s",
                j, strerror(errno), MYSIZEOF(SLogFileLogIndex), m_pstLogFileList->stFiles[i].szAbsFileName);
            return SLOG_ERROR_LINE;
        }
    }
    return 0;
}


int CSLogServerWriteFile::AttachShm()
{
    int iRet = 0;
    if(slog.m_iVmemShmKey > 0) {
        iRet = MtReport_InitVmem_ByFlag(0666|IPC_CREAT, slog.m_iVmemShmKey);
        if(iRet < 0)
        {
            ERR_LOG("init vmem failed ret:%d", iRet);
            return -1;
        }
    }
    
	m_pShmLog = slog.GetAppLogShm(m_pAppInfo, true);
    if(m_pShmLog == NULL)
    {
        ERR_LOG("init app log shm failed ");
        return -2;
    }

    m_pstLogFileList = GetAppLogFileShm(m_pAppInfo, true);
    if(m_pstLogFileList == NULL)
    {
        ERR_LOG("init app log file shm failed");
        return SLOG_ERROR_LINE;
	}
    return 0;
}

int CSLogServerWriteFile::WriteLog(
	SLogFileHead &stFileHead, SLogFileLogIndex &stLogIndex, const char *pszLogTxt)
{
	if(NULL == m_fpLogFile)
	{
		FATAL_LOG("bug, m_fpLogFile is NULL !");
		return SLOG_ERROR_LINE;
	}

	// 日志长度
	stLogIndex.dwLogContentLen = MYSTRLEN(pszLogTxt) + 1; // 1 表示 0，字符串的结束符

	// 写日志文件头部, 日志统计信息同时写入共享内存中 ----------- 1
	stFileHead.iLogRecordsWrite++;
	stFileHead.stLogStatInfo.qwLogSizeInfo += stLogIndex.dwLogContentLen;
	if(m_pAppInfo != NULL)
		m_pAppInfo->stLogStatInfo.qwLogSizeInfo += stLogIndex.dwLogContentLen;
	switch(stLogIndex.wLogType)
	{
		case SLOG_LEVEL_DEBUG:
			stFileHead.stLogStatInfo.dwDebugLogsCount++;
			if(m_pAppInfo != NULL)
				m_pAppInfo->stLogStatInfo.dwDebugLogsCount++;
			break;
		case SLOG_LEVEL_INFO:
			stFileHead.stLogStatInfo.dwInfoLogsCount++;
			if(m_pAppInfo != NULL)
				m_pAppInfo->stLogStatInfo.dwInfoLogsCount++;
			break;
		case SLOG_LEVEL_WARNING:
			stFileHead.stLogStatInfo.dwWarnLogsCount++;
			if(m_pAppInfo != NULL)
				m_pAppInfo->stLogStatInfo.dwWarnLogsCount++;
			break;
		case SLOG_LEVEL_REQERROR:
			stFileHead.stLogStatInfo.dwReqerrLogsCount++;
			m_pAppInfo->stLogStatInfo.dwReqerrLogsCount++;
			break;
		case SLOG_LEVEL_ERROR:
			stFileHead.stLogStatInfo.dwErrorLogsCount++;
			m_pAppInfo->stLogStatInfo.dwErrorLogsCount++;
			break;
		case SLOG_LEVEL_FATAL:
			stFileHead.stLogStatInfo.dwFatalLogsCount++;
			m_pAppInfo->stLogStatInfo.dwFatalLogsCount++;
			break;
		default:
			stFileHead.stLogStatInfo.dwOtherLogsCount++;
			m_pAppInfo->stLogStatInfo.dwOtherLogsCount++;
			break;
	}

	if(fseek(m_fpLogFile, 0L, SEEK_SET) < 0)
	{
		ERR_LOG("seek file head failed msg:%s file:%s", 
			strerror(errno), m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName);
		RemoveFileInfo(m_wCurFpLogFileIndex);
		return SLOG_ERROR_LINE;
	}
	if(fwrite(&stFileHead, MYSIZEOF(stFileHead), 1, m_fpLogFile) != 1)
	{
		ERR_LOG("write file head failed msg:%s write size:%u file:%s", strerror(errno),
			MYSIZEOF(stFileHead), m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName);
		return SLOG_ERROR_LINE;
	}

	// 写日志内容 --------------- 2
	if(fseek(m_fpLogFile, 0L, SEEK_END) < 0)
	{
		ERR_LOG("seek file end failed msg:%s file:%s", 
			strerror(errno), m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName);
		return SLOG_ERROR_LINE;
	}

	int32_t iPos = 0;
	if((iPos=ftell(m_fpLogFile)) < 0)
	{
		ERR_LOG("ftell failed msg:%s file:%s", 
			strerror(errno), m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName);
		return SLOG_ERROR_LINE;
	}
	stLogIndex.dwLogContentPos = iPos;

	// 日志内容写个索引，以便数据对账
	if(fwrite(&(stLogIndex.dwLogSeq), MYSIZEOF(stLogIndex.dwLogSeq), 1, m_fpLogFile) != 1)
	{
		ERR_LOG("write log seq failed msg:%s write size:%u file:%s",
			strerror(errno), MYSIZEOF(stLogIndex.dwLogSeq),
			m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName);
		return SLOG_ERROR_LINE;
	}
	m_iWriteLogBytes += MYSIZEOF(stLogIndex.dwLogSeq);

	if(fwrite(pszLogTxt, stLogIndex.dwLogContentLen, 1, m_fpLogFile) != 1)
	{
		ERR_LOG("write log content failed msg:%s write size:%u file:%s",
			strerror(errno), MYSTRLEN(pszLogTxt)+1, m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName);
		return SLOG_ERROR_LINE;
	}
	m_iWriteLogBytes += stLogIndex.dwLogContentLen;

	// 写日志索引信息 ------------- 3
	iPos = (int32_t)(MYSIZEOF(SLogFileHead)+(stFileHead.iLogRecordsWrite-1)*MYSIZEOF(SLogFileLogIndex));
	if(fseek(m_fpLogFile, iPos, SEEK_SET) < 0)
	{
		ERR_LOG("seek log index failed msg:%s file:%s", 
			strerror(errno), m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName);
		return SLOG_ERROR_LINE;
	}
	if(fwrite(&stLogIndex, MYSIZEOF(SLogFileLogIndex), 1, m_fpLogFile) != 1)
	{
		ERR_LOG("write log index failed msg:%s write size:%u file:%s", strerror(errno),
			MYSIZEOF(SLogFileLogIndex), m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName);
		return SLOG_ERROR_LINE;
	}
	return 0;
}

int CSupperLog::InitMtClientInfo()
{
	static bool s_bInit = false;
	if(s_bInit)
		return 0;

	int iRet = InitHashTable_NoList(&m_stHashMtClient, MYSIZEOF(MtClientInfo), 
		MT_CLIENT_NODE_COUNT, m_iMtClientInfoShmKey, MtClientInfoHashCmp, MtClientInfoHashWarn);
	if(iRet < 0)
	{
		ERR_LOG("mtclient hash shm init failed, key:%d ret:%d", m_iMtClientInfoShmKey, iRet);
		return iRet;
	}
	INFO_LOG("init mtclient hash info success key:%d", m_iMtClientInfoShmKey);
	s_bInit = true;
	return 0;
}

int CSLogServerWriteFile::WriteLogRecord(int iLogIndex)
{
	const char *pszLogTxt = NULL;
	TSLog *pShmLog = m_pShmLog->sLogList+iLogIndex;

	// add check --- 可能数据未写入完整，记录日志报个错，下次再尝试 --
	if(0==pShmLog->dwLogSeq) {
		FATAL_LOG("WriteLogRecord check failed -- appid:%d module id:%d, index:%d",
			pShmLog->iAppId, pShmLog->iModuleId, iLogIndex);
		return SLOG_ERROR_LINE;
	}

	pszLogTxt = GetShmLog(pShmLog, iLogIndex);
	if(NULL == pszLogTxt)
	{
		WARN_LOG("slog shm index:%d, appid:%d, module id:%d get content failed !",
			iLogIndex, pShmLog->iAppId, pShmLog->iModuleId);

		// 重置下读索引
		m_pShmLog->iLogStarIndex = -1;
		return SLOG_ERROR_LINE;
	}

	if(m_pstLogFileList->wLogFileCount <= 0 
		|| m_pstLogFileList->stFiles[m_pstLogFileList->wLogFileCount-1].stFileHead.iLogRecordsWrite 
		   >= m_pstLogFileList->stFiles[m_pstLogFileList->wLogFileCount-1].stFileHead.iLogRecordsMax)
	{
		if(m_pstLogFileList->wLogFileCount > 0) {
			INFO_LOG("try add new log file - count:%d, records:%d, max:%d", m_pstLogFileList->wLogFileCount,
				m_pstLogFileList->stFiles[m_pstLogFileList->wLogFileCount-1].stFileHead.iLogRecordsWrite,
				m_pstLogFileList->stFiles[m_pstLogFileList->wLogFileCount-1].stFileHead.iLogRecordsMax);
		}
		if(AddNewSlogFile(pShmLog->qwLogTime) < 0)
		{
			if(pShmLog->iContentIndex > 0)
			{
				MtReport_FreeVmem(pShmLog->iContentIndex);
				pShmLog->iContentIndex = -1; 
			}
			return SLOG_ERROR_LINE;
		}
		INFO_LOG("add new log file - count:%d, new file:%s", m_pstLogFileList->wLogFileCount,
			m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName);
	}

	if(m_wCurFpLogFileIndex >= SLOG_LOG_FILES_COUNT_MAX)
	{
		ERR_LOG("bug: cur log file index:%u > %d", m_wCurFpLogFileIndex, SLOG_LOG_FILES_COUNT_MAX);
		m_wCurFpLogFileIndex = m_pstLogFileList->wLogFileCount-1;
	}
	if(m_wCurFpLogFileIndex >= SLOG_LOG_FILES_COUNT_MAX)
	{
		ERR_LOG("bug: cur log file index:%u|%d|%d", 
			m_wCurFpLogFileIndex, SLOG_LOG_FILES_COUNT_MAX, m_pstLogFileList->wLogFileCount);
		if(pShmLog->iContentIndex > 0)
		{
			MtReport_FreeVmem(pShmLog->iContentIndex);
			pShmLog->iContentIndex = -1; 
		}
		return SLOG_ERROR_LINE;
	}

	if(NULL == m_fpLogFile)
	{
		if(ReadLogFileHead(m_wCurFpLogFileIndex) < 0) 
			return SLOG_ERROR_LINE;

		m_fpLogFile = fopen(m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName, "rb+");
		if(NULL == m_fpLogFile)
		{
			ERR_LOG("open file:%s for write failed , msg:%s", 
				m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName, strerror(errno)); 
			RemoveFileInfo(m_wCurFpLogFileIndex);
			if(pShmLog->iContentIndex > 0)
			{
				MtReport_FreeVmem(pShmLog->iContentIndex);
				pShmLog->iContentIndex = -1; 
			}
			return SLOG_ERROR_LINE;
		}
	}

	SLogFileHead & stFileHead = m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].stFileHead;

	// 多条日志同时上报，可能先写的日志后到达 ---- add by rock - 2019-04-03
	// 日志文件有两个时间 
	// SLogFileHead: 中的时间用于历史日志查询、确定日志先后顺序 (qwLogTimeStart, qwLogTimeEnd)
	// SLogFileInfo: 中的时间用于文件删除, 文件名的命名 (qwLogTimeStart)
	if(stFileHead.qwLogTimeEnd < pShmLog->qwLogTime)
		stFileHead.qwLogTimeEnd = pShmLog->qwLogTime;
	if(stFileHead.qwLogTimeStart > pShmLog->qwLogTime)
		stFileHead.qwLogTimeStart = pShmLog->qwLogTime;

	SLogFileLogIndex stLogIndex; 
	memset(&stLogIndex, 0, MYSIZEOF(stLogIndex));
	stLogIndex.dwCust_1 = pShmLog->dwCust_1;
	stLogIndex.dwCust_2 = pShmLog->dwCust_2;
	stLogIndex.iCust_3 = pShmLog->iCust_3;
	stLogIndex.iCust_4 = pShmLog->iCust_4;
	memcpy(stLogIndex.szCust_5, pShmLog->szCust_5, MYSIZEOF(pShmLog->szCust_5));
	memcpy(stLogIndex.szCust_6, pShmLog->szCust_6, MYSIZEOF(pShmLog->szCust_6));
	stLogIndex.dwLogConfigId = pShmLog->dwLogConfigId;
	stLogIndex.dwLogHost = pShmLog->dwLogHost;
	stLogIndex.iAppId = pShmLog->iAppId;
	stLogIndex.iModuleId = pShmLog->iModuleId;

	if(pShmLog->wLogType != SLOG_LEVEL_DEBUG && pShmLog->wLogType != SLOG_LEVEL_INFO
		&& pShmLog->wLogType != SLOG_LEVEL_WARNING && pShmLog->wLogType != SLOG_LEVEL_REQERROR
		&& pShmLog->wLogType != SLOG_LEVEL_ERROR && pShmLog->wLogType != SLOG_LEVEL_FATAL)
	{
		stLogIndex.wLogType = SLOG_LEVEL_OTHER;
	}
	else
		stLogIndex.wLogType = pShmLog->wLogType;
	stLogIndex.dwLogSeq = pShmLog->dwLogSeq;
	stLogIndex.qwLogTime = pShmLog->qwLogTime;

	if(WriteLog(stFileHead, stLogIndex, pszLogTxt) < 0)
	{
		if(m_fpLogFile != NULL)
		{
			fclose(m_fpLogFile);
			m_fpLogFile = NULL;
		}

		if(pShmLog->iContentIndex > 0)
		{
			MtReport_FreeVmem(pShmLog->iContentIndex);
			pShmLog->iContentIndex = -1; 
		}
		exit(-1); // 进程退出，无法回滚
	}

	// 排除 slog_write 自身写入共享内存的log
	if(!(stLogIndex.iAppId==slog.m_iLogAppId && stLogIndex.iModuleId==slog.m_iLogModuleId))
	{
		DEBUG_LOG("write log -- appid:%d moduleid:%d logtype:%d logtime:%" PRIu64 " logseq:%u"
			" content len:%u content pos:%u to file:%s", stLogIndex.iAppId, stLogIndex.iModuleId, 
			stLogIndex.wLogType, stLogIndex.qwLogTime, stLogIndex.dwLogSeq, 
			stLogIndex.dwLogContentLen, stLogIndex.dwLogContentPos, 
			m_pstLogFileList->stFiles[m_wCurFpLogFileIndex].szAbsFileName);
	}

	// 使用了vmem 存储超长log
	if(pShmLog->iContentIndex > 0)
	{
		// 实时日志显示部分内容
		pShmLog->sLogContent[BWORLD_MEMLOG_BUF_LENGTH-TOO_LONG_TRUNC_STR_LEN] = '\0';
		strcat(pShmLog->sLogContent, TOO_LONG_TRUNC_STR);

		int iRet = 0;
		if((iRet=MtReport_FreeVmem(pShmLog->iContentIndex)) < 0)
			ERR_LOG("MtReport_FreeVmem failed ! ret:%d", iRet);
		pShmLog->iContentIndex = -1; 
	}
	return 0;
}


// 注意: 对于一个 app, 只能有一个写进程通过 m_pShmLog 将日志写入文件
int CSLogServerWriteFile::WriteFile(int iWriteRecords, uint32_t dwCurTime)
{
	if(!m_bInit)
	{
		ERR_LOG("not init !");
		return SLOG_ERROR_LINE;
	}

	// 日志文件占用空间过大自动删除逻辑
	if(m_pAppInfo != NULL
		&& IS_SET_BIT(m_pAppInfo->dwAppLogFlag, APPLOG_FILE_FLAG_DELETE_OLD_FILE)
		&& m_pAppInfo->dwDeleteLogFileTime == TIME_USEC_TO_SEC(m_pstLogFileList->stFiles[0].qwLogTimeStart)) 
	{
		INFO_LOG("delete log file by flag, app:%d, file time:%u, log file:%s", m_iAppId, 
			m_pAppInfo->dwDeleteLogFileTime, m_pstLogFileList->stFiles[0].szAbsFileName);
		CLEAR_BIT(m_pAppInfo->dwAppLogFlag, APPLOG_FILE_FLAG_DELETE_OLD_FILE);
		m_pAppInfo->dwDeleteLogFileTime = 0;
		RemoveFileInfo(0);
		MtReport_Attr_Add(263, 1);
	}
	m_pAppInfo->dwLastTryWriteLogTime = dwCurTime;

	// 从共享内存读取日志，写入文件，所以这里用读索引
	int32_t iLastWriteIndex = m_pShmLog->iLogStarIndex;
	if(iLastWriteIndex < 0)
		return 0;

	m_iWriteLogBytes = 0;
	int i, iNextIndex;
	for(i=0; i < iWriteRecords;)
	{
		if(WriteLogRecord(iLastWriteIndex) < 0)
			break;
		i++;

		SLOG_NEXT_INDEX(iLastWriteIndex, iNextIndex);

		// 是否有其它进程改过 iLogStarIndex
		if(iLastWriteIndex != m_pShmLog->iLogStarIndex || CSupperLog::InitGetShmLogIndex(m_pShmLog) < 0)
			break;

		if(iNextIndex == m_pShmLog->iWriteIndex)
		{
			m_pShmLog->iLogStarIndex = -1;
			CSupperLog::EndGetShmLogIndex(m_pShmLog);
			break;
		}
		m_pShmLog->iLogStarIndex = iNextIndex;
		CSupperLog::EndGetShmLogIndex(m_pShmLog);
		iLastWriteIndex = iNextIndex;
	}
	if(m_iWriteLogBytes > 0)
		MtReport_Attr_Add(285, m_iWriteLogBytes);

	if(m_fpLogFile != NULL)
	{
		fclose(m_fpLogFile);
		m_fpLogFile = NULL;
	}
	return i;
}

int CSLogServerWriteFile::ReadLogFileHead(int iLogFileIndex)
{
	if(iLogFileIndex >= m_pstLogFileList->wLogFileCount)
	{
		ERR_LOG("invalid parameter:%d > %d", iLogFileIndex, m_pstLogFileList->wLogFileCount);
		return -1;
	}

	FILE *fp = fopen(m_pstLogFileList->stFiles[iLogFileIndex].szAbsFileName, "rb+");
	if(NULL == fp)
	{
		ERR_LOG("open file:%s failed , msg:%s", 
			m_pstLogFileList->stFiles[iLogFileIndex].szAbsFileName, strerror(errno));
		RemoveFileInfo(iLogFileIndex);
		return -2;
	}
	fseek(fp, 0, SEEK_SET);

	SLogFileHead & stFileHead = m_pstLogFileList->stFiles[iLogFileIndex].stFileHead;
	if(fread(&stFileHead, MYSIZEOF(stFileHead), 1, fp) != 1)
	{
		ERR_LOG("read slog file head from file:%s failed, msg:%s !", 
			m_pstLogFileList->stFiles[iLogFileIndex].szAbsFileName, strerror(errno));
		fclose(fp);
		return -3;
	}

	// 日志统计的概要信息读取 --- 2019-02-07
	if(stFileHead.stLogStatInfo.qwLogSizeInfo <= 0)
	{
		// 老的日志文件没有写入类型等概要信息，重新生成并写入
		memset(&stFileHead.stLogStatInfo, 0, sizeof(stFileHead.stLogStatInfo));
		SLogFileLogIndex stIndex;
		int j = 0;
		for(j=0; j < stFileHead.iLogRecordsWrite; j++)
		{
			if(fread(&stIndex, MYSIZEOF(SLogFileLogIndex), 1, fp) != 1)
			{
				ERR_LOG("read slog file:%s failed, msg:%s !", 
					m_pstLogFileList->stFiles[iLogFileIndex].szAbsFileName, strerror(errno));
				break;
			}

			// check it
			if(stIndex.iAppId != m_iAppId) {
				ERR_LOG("check slog file:%s failed, appid: %d != %d",
					m_pstLogFileList->stFiles[iLogFileIndex].szAbsFileName, stIndex.iAppId, m_iAppId);
				break;
			}
			stFileHead.stLogStatInfo.qwLogSizeInfo += stIndex.dwLogContentLen;
			switch(stIndex.wLogType){
				case SLOG_LEVEL_DEBUG:
					stFileHead.stLogStatInfo.dwDebugLogsCount++;
					break;
				case SLOG_LEVEL_INFO:
					stFileHead.stLogStatInfo.dwInfoLogsCount++;
					break;
				case SLOG_LEVEL_WARNING:
					stFileHead.stLogStatInfo.dwWarnLogsCount++;
					break;
				case SLOG_LEVEL_REQERROR:
					stFileHead.stLogStatInfo.dwReqerrLogsCount++;
					break;
				case SLOG_LEVEL_ERROR:
					stFileHead.stLogStatInfo.dwErrorLogsCount++;
					break;
				case SLOG_LEVEL_FATAL:
					stFileHead.stLogStatInfo.dwFatalLogsCount++;
					break;
				default:
					stFileHead.stLogStatInfo.dwOtherLogsCount++;
					break;
			}
		}

		if(stFileHead.stLogStatInfo.qwLogSizeInfo > 0) {
			// 写入磁盘
			fseek(fp, 0, SEEK_SET);
			if(fwrite(&stFileHead, MYSIZEOF(stFileHead), 1, fp) != 1)
			{
				ERR_LOG("write file head failed msg:%s write size:%u file:%s",
					strerror(errno), MYSIZEOF(stFileHead), m_pstLogFileList->stFiles[iLogFileIndex].szAbsFileName); 
			}

			INFO_LOG("read app:%d log stat info from file:%s, " 
				"stat info - size:%lu, debug:%u, info:%u, warn:%u, reqerr:%u, err:%u, fatal:%u, other:%u",
				m_iAppId, m_pstLogFileList->stFiles[iLogFileIndex].szAbsFileName, 
				stFileHead.stLogStatInfo.qwLogSizeInfo, stFileHead.stLogStatInfo.dwDebugLogsCount,
				stFileHead.stLogStatInfo.dwInfoLogsCount, stFileHead.stLogStatInfo.dwWarnLogsCount,
				stFileHead.stLogStatInfo.dwReqerrLogsCount, stFileHead.stLogStatInfo.dwErrorLogsCount,
				stFileHead.stLogStatInfo.dwFatalLogsCount, stFileHead.stLogStatInfo.dwOtherLogsCount);
		}
	}
	fclose(fp);

	const char *pszCheckDigit = qwtoa(m_pstLogFileList->stFiles[iLogFileIndex].qwLogTimeStart);
	if(strcmp(stFileHead.szCheckDigit, pszCheckDigit))
	{
		WARN_LOG("check string failed %s != %s, file:%s", 
			stFileHead.szCheckDigit, pszCheckDigit, m_pstLogFileList->stFiles[iLogFileIndex].szAbsFileName);
		return SLOG_ERROR_LINE;
	}

	if(stFileHead.wCheckByteOrder != SLOG_CHECK_BYTES_ORDER_NUM)
	{
		// 当日志文件在不同的字节序机器间转移时，会出现字节序不一致问题，暂时不处理这种情况
		WARN_LOG("check bytes order failed %d != %d, file:%s",
			stFileHead.wCheckByteOrder, SLOG_CHECK_BYTES_ORDER_NUM, 
			m_pstLogFileList->stFiles[iLogFileIndex].szAbsFileName);
		return -44;
	}

	if(stFileHead.bLogFileVersion < SLOG_FILE_VERSION_MIN 
		|| stFileHead.bLogFileVersion > SLOG_FILE_VERSION_MAX)
	{
		WARN_LOG("invalid file version - %d(%d-%d)", 
			stFileHead.bLogFileVersion, SLOG_FILE_VERSION_MIN, SLOG_FILE_VERSION_MAX);
		return SLOG_ERROR_LINE;
	}

	INFO_LOG("read slog file head info: check str:%s time start:%" PRIu64 
		" time end:%" PRIu64 " records writed:%d record max:%d in file:%s", stFileHead.szCheckDigit, 
		stFileHead.qwLogTimeStart, stFileHead.qwLogTimeEnd, stFileHead.iLogRecordsWrite,
		stFileHead.iLogRecordsMax, m_pstLogFileList->stFiles[iLogFileIndex].szAbsFileName);
	return 0;
}

// class CSLogClient 
// ------------------------------------------------------------------------------------------------------
CSLogClient::CSLogClient(TSLogShm *pShmLog)
{
	m_bInit = false;
	m_pShmLog = pShmLog;
	if(slog.m_iVmemShmKey > 0) {
		int iRet = MtReport_InitVmem_ByFlag(0666|IPC_CREAT, slog.m_iVmemShmKey);
		if(iRet < 0)
		{
			ERR_LOG("Init vmem buf failed ! ret:%d", iRet);
			return ;
		}
	}
	m_bInit = true;
}

void CSLogClient::ShowShmInfo()
{
	if(!m_bInit){
		printf("CSLogClient not init !\n");
		return;
	}

	printf("--- shm info - log next seq:%u, sIndex:%d, wIndex:%d, bTryget:%d\n",
		m_pShmLog->dwLogSeq, m_pShmLog->iLogStarIndex, m_pShmLog->iWriteIndex, m_pShmLog->bTryGetLogIndex);

	if(m_pShmLog->iLogStarIndex < 0 || m_pShmLog->iLogStarIndex == m_pShmLog->iWriteIndex)
		return;
	printf("\t\t--- first log, index:%d\n", m_pShmLog->iLogStarIndex);
	ShowShmLogContent(m_pShmLog, m_pShmLog->iLogStarIndex);

	int iLast = m_pShmLog->iWriteIndex - 1;
	if(iLast < 0)
		iLast = 0;
	if(iLast == m_pShmLog->iLogStarIndex)
		return;
	printf("\t\t--- last log, index:%d\n", iLast);
	ShowShmLogContent(m_pShmLog, iLast);
}

TSLogOut * CSLogClient::GetLog()
{
	static TSLogOut stLogOut;
	int32_t iLastIndex = m_pShmLog->iLogStarIndex;
	if(iLastIndex >= 0)
	{
		stLogOut.iAppId = m_pShmLog->sLogList[iLastIndex].iAppId;
		stLogOut.iModuleId = m_pShmLog->sLogList[iLastIndex].iModuleId;
		stLogOut.wLogType = m_pShmLog->sLogList[iLastIndex].wLogType;
		stLogOut.dwLogSeq = m_pShmLog->sLogList[iLastIndex].dwLogSeq;
		stLogOut.qwLogTime = m_pShmLog->sLogList[iLastIndex].qwLogTime;
		stLogOut.dwCust_1 = m_pShmLog->sLogList[iLastIndex].dwCust_1;
		stLogOut.dwCust_2 = m_pShmLog->sLogList[iLastIndex].dwCust_2;
		stLogOut.iCust_3 = m_pShmLog->sLogList[iLastIndex].iCust_3;
		stLogOut.iCust_4 = m_pShmLog->sLogList[iLastIndex].iCust_4;
		stLogOut.dwLogConfigId = m_pShmLog->sLogList[iLastIndex].dwLogConfigId;
		stLogOut.dwLogHost = m_pShmLog->sLogList[iLastIndex].dwLogHost;
		memcpy(stLogOut.szCust_5, m_pShmLog->sLogList[iLastIndex].szCust_5, MYSIZEOF(stLogOut.szCust_5));
		memcpy(stLogOut.szCust_6, m_pShmLog->sLogList[iLastIndex].szCust_6, MYSIZEOF(stLogOut.szCust_6));
		stLogOut.wLogType = m_pShmLog->sLogList[iLastIndex].wLogType;
	
		stLogOut.pszLog = GetShmLog(m_pShmLog->sLogList+iLastIndex, iLastIndex);
		if(NULL == stLogOut.pszLog)
		{
			WARN_LOG("slog shm index:%d, appid:%d, module id:%d get content failed !",
				iLastIndex, m_pShmLog->sLogList[iLastIndex].iAppId, m_pShmLog->sLogList[iLastIndex].iModuleId);

			int iNewIndex;
			SLOG_NEXT_INDEX(iLastIndex, iNewIndex);
			CSupperLog::ModifyLogStartIndexCmpAndSwap(m_pShmLog, iLastIndex, iNewIndex);
			return NULL;
		}

		// 使用了vmem 存储超长log
		if(m_pShmLog->sLogList[iLastIndex].iContentIndex >= 0)
		{
			int iRet = 0;
			if((iRet=MtReport_FreeVmem(m_pShmLog->sLogList[iLastIndex].iContentIndex)) < 0)
				ERR_LOG("MtReport_FreeVmem failed ! slog index:%d cindex:%d ret:%d, log:%s", 
					iLastIndex, m_pShmLog->sLogList[iLastIndex].iContentIndex, iRet, stLogOut.pszLog);
			m_pShmLog->sLogList[iLastIndex].iContentIndex = -1; 

			m_pShmLog->sLogList[iLastIndex].sLogContent[BWORLD_MEMLOG_BUF_LENGTH-TOO_LONG_TRUNC_STR_LEN] = '\0';
			strcat(m_pShmLog->sLogList[iLastIndex].sLogContent+BWORLD_MEMLOG_BUF_LENGTH-TOO_LONG_TRUNC_STR_LEN,
				TOO_LONG_TRUNC_STR);
		}

		int iNewIndex;
		SLOG_NEXT_INDEX(iLastIndex, iNewIndex);
		CSupperLog::ModifyLogStartIndexCmpAndSwap(m_pShmLog, iLastIndex, iNewIndex);
		return &stLogOut; 
	}
	return NULL;
}

TCommSendMailInfoShm * CSupperLog::InitMailInfoShm(int iShmKey, bool bCreate)
{
	TCommSendMailInfoShm *pshm = NULL;
	int iFlag = 0666;
	if(bCreate)
		iFlag |= IPC_CREAT;

	int iRet = GetShm2((void**)&pshm, iShmKey, (int)MYSIZEOF(TCommSendMailInfoShm), iFlag);
	if(iRet < 0)
	{
		ERR_LOG("attach email shm info failed - key:%u, size:%u, create:%d",
			iShmKey, MYSIZEOF(TCommSendMailInfoShm), bCreate);
		return NULL;
	}
	DEBUG_LOG("attach email shm info ok - key:%u, size:%u, create:%d", 
			iShmKey, MYSIZEOF(TCommSendMailInfoShm), bCreate);
	return pshm;
}

int CSupperLog::AddMailToShm(TCommSendMailInfo &stMail)
{ 
	static TCommSendMailInfoShm *s_pshm = NULL;

	if(NULL == s_pshm) {
		s_pshm = InitMailInfoShm();
		if(NULL == s_pshm)
			return SLOG_ERROR_LINE;
	}
	return AddMailToShm(s_pshm, stMail);
}

int CSupperLog::InitGetMailShmLock(TCommSendMailInfoShm *pshm)
{
	int i=0;
	for(i=0; i < 100; i++)
	{
		if(__sync_bool_compare_and_swap(&pshm->bLockGetShm, 0, 1))
			break;
		usleep(10);
	}
	if(i >= 100)
	{
		MtReport_Attr_Add(222, 1);
		return SLOG_ERROR_LINE;
	}
	return 0;
}


int CSupperLog::AddMailToShm(TCommSendMailInfoShm *pshm, TCommSendMailInfo &stMail)
{
	int i=0, j=-1;
	uint32_t dwTimeNow = time(NULL);
	uint32_t dwModifySeq = 0;

	for(i=0; i < MAX_COMM_SEND_MAIL_NODE_COUNT; i++)
	{
		dwModifySeq = pshm->stInfo[i].dwModifySeq;
		if(pshm->stInfo[i].dwMailSeq == 0 || pshm->stInfo[i].dwValidTimeUtc <= dwTimeNow)
		{
			if(pshm->stInfo[i].dwMailSeq != 0 && pshm->stInfo[i].dwValidTimeUtc <= dwTimeNow)
				j = i;
			if(stMail.dwMailSeq == 0)
				stMail.dwMailSeq = 1;

			if(InitGetMailShmLock(pshm) >= 0)
			{
				if(dwModifySeq != pshm->stInfo[i].dwModifySeq)
				{
					EndGetMailShmLock(pshm);
					j = -1;
					continue;
				}

				memcpy(pshm->stInfo+i, &stMail, sizeof(stMail));
				pshm->stInfo[i].dwModifySeq++;
				EndGetMailShmLock(pshm);
				break;
			}
		}
	}

	if(j >= 0)
		MtReport_Attr_Add(223, 1);
	return 0;
}


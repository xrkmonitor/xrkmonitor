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

#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "sv_log.h"
#include "sv_str.h"


const uint32_t ONE_LINE = 256;			//average chars per line

#define MAX_LINE_LEN	(8192)		//max chars in a line
#define WRITE_SPEED_DEFAULT	50			//WARN & ERROR speed default(/s)

const uint32_t LOG_MIN_SZ = 64 << 10;	//minimal size of log file

const uint32_t LOG_MIN_LINES = 256;		//minimal lines of log file

const uint32_t LOG_MIN_INTERVAL = 10;	//minimal shift interval

//level names
#define LVL_NAME_LEN	9			//level name length
static const char g_aLevelName[][LVL_NAME_LEN] = {
	"[TRACE] ",
	"[DEBUG] ",
	"[INFO]  ",
	"[WARN]  ",
	"[ERROR] ",
	"[FATAL] "
};

const char * C2_ErrMsg(int iErrno=0)
{
	if(iErrno==0)
		iErrno = errno;
	static char sBuf[256];
	snprintf(sBuf, sizeof sBuf, " errno=%d(%s)", iErrno, strerror(iErrno));
	return sBuf;
}   

C2_ELogLevel C2_LogLvl(const char * pszLogLevel)
{
	if(!pszLogLevel)
		return LL_OFF;
	if(0 == strncmp("TRACE", pszLogLevel, 6))
		return LL_TRACE;
	if(0 == strncmp("DEBUG", pszLogLevel, 6))
		return LL_DEBUG;
	if(0 == strncmp("INFO", pszLogLevel, 5))
		return LL_INFO;
	if(0 == strncmp("WARN", pszLogLevel, 5))
		return LL_WARN;
	if(0 == strncmp("ERROR", pszLogLevel, 6))
		return LL_ERROR;
	if(0 == strncmp("FATAL", pszLogLevel, 6))
		return LL_FATAL;
	return LL_OFF;
}

static int setParams(C2_LogFile * pstLogFile, const C2_LogFileParam * pstLogFileParam)
{
	int iWriteSpeed;
	memset(pstLogFile, 0, sizeof(C2_LogFile));
	if(pstLogFileParam->pszLogBaseName){
		snprintf(pstLogFile->sLogFileName, sizeof pstLogFile->sLogFileName, "%s.log", pstLogFileParam->pszLogBaseName);
	}else if(pstLogFileParam->pszLogFullName){
		strncpy(pstLogFile->sLogFileName, pstLogFileParam->pszLogFullName, sizeof pstLogFile->sLogFileName);
	}else
		return -2;
	pstLogFile->dwMaxLogNum = pstLogFileParam->dwMaxLogNum;
	pstLogFile->iIsLogToStd = pstLogFileParam->iIsLogToStd;
	pstLogFile->iIsLogTime = pstLogFileParam->iIsLogTime;
	pstLogFile->eLogLevel = pstLogFileParam->eLogLevel;
	pstLogFile->eShiftType = pstLogFileParam->eShiftType;
	pstLogFile->dwMax = pstLogFileParam->dwMax;
	pstLogFile->iOldStyle = pstLogFileParam->iOldStyle;
	iWriteSpeed = (pstLogFileParam->iWriteSpeed ? pstLogFileParam->iWriteSpeed : WRITE_SPEED_DEFAULT);
	TokenBucket_Init(&pstLogFile->stWriteSpeed, iWriteSpeed, iWriteSpeed);
	switch(pstLogFile->eShiftType){	//set min value
		case ST_SIZE:
			if(pstLogFile->dwMax < LOG_MIN_SZ)
				pstLogFile->dwMax = LOG_MIN_SZ;
			break;
		case ST_LINES:
			if(pstLogFile->dwMax < LOG_MIN_LINES)
				pstLogFile->dwMax = LOG_MIN_LINES;
			break;
		case ST_INTERVAL:
			if(pstLogFile->dwMax < LOG_MIN_INTERVAL)
				pstLogFile->dwMax = LOG_MIN_INTERVAL;
			break;
		default:
			pstLogFile->eShiftType = ST_SIZE;
			if(pstLogFile->dwMax < LOG_MIN_SZ)
				pstLogFile->dwMax = LOG_MIN_SZ;
			;
	}
	return 0;
}

static int openFile(C2_LogFile * pstLogFile)
{
	char *pDir = strrchr(pstLogFile->sLogFileName, '/');
	if(pstLogFile->sLogFileName[0] != '.' && pDir != NULL) {

		*pDir = '\0'; 
		DIR * pstDir = opendir(pstLogFile->sLogFileName);
		if(pstDir != NULL)
			closedir(pstDir);
		else {
			char szCreateDir[512];
			snprintf(szCreateDir, sizeof(szCreateDir), "mkdir -p %s", pstLogFile->sLogFileName);
			system(szCreateDir);
			sleep(1);
		}
		*pDir = '/'; 
	}

	if(!pstLogFile->iFd){
		const int FILE_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		const int OPEN_FLAG = O_RDWR | O_CREAT | O_APPEND;
		pstLogFile->iFd = open(pstLogFile->sLogFileName, OPEN_FLAG, FILE_MODE);
		if(pstLogFile->iFd < 0){
			pstLogFile->iFd = 0;
			SetApiErrorMsg("cannot open log file '%s',%s\n", pstLogFile->sLogFileName, C2_ErrMsg());
			return -1;
		}
		++pstLogFile->iFd;
	}
	return 0;
}

static int getFileInfo(C2_LogFile * pstLogFile)
{
	struct stat stStat;
	if(pstLogFile->iFd > 0){	
		if(fstat(pstLogFile->iFd - 1, &stStat) < 0){
			SetApiErrorMsg("cannot stat log file '%s',%s\n", pstLogFile->sLogFileName,C2_ErrMsg());
			return -1;
		}
	}else if(access(pstLogFile->sLogFileName, F_OK) == 0){	
		if(stat(pstLogFile->sLogFileName, &stStat) < 0){
			SetApiErrorMsg("cannot stat log file '%s',%s\n", pstLogFile->sLogFileName,C2_ErrMsg());
			return -1;
		}
	}else{	
		pstLogFile->dwCur = 0;
		return 0;
	}
	pstLogFile->tLastShiftTime = stStat.st_ctime;
	pstLogFile->dwCur = (uint32_t)stStat.st_size;
	if(pstLogFile->eShiftType == ST_LINES)
		pstLogFile->dwCur /= ONE_LINE;
	return 0;
}

static int initFileLog(C2_LogFile * pstLogFile)
{
	int iRet = openFile(pstLogFile);
	if(iRet != 0)
		return iRet;
	return getFileInfo(pstLogFile);
}

inline static int needShift(C2_LogFile * pstLogFile)
{
	struct stat stStat;

	switch(pstLogFile->eShiftType){
		case ST_SIZE:
		case ST_LINES:
			if(stat(pstLogFile->sLogFileName, &stStat) < 0)
				return 0;
			if(stStat.st_size < pstLogFile->dwMax)
				return 0;
			return 1;
		case ST_INTERVAL:
			if(pstLogFile->tLastShiftTime + (time_t)pstLogFile->dwMax <= time(0))
		        return 1;
			break;
		case ST_DAY:
			if(pstLogFile->tLastShiftTime + 86400 <= time(0))
				return 1;
			break;
		case ST_HOUR:
			if(pstLogFile->tLastShiftTime + 3600 <= time(0))
				return 1;
			break;
		case ST_MIN:
			if(pstLogFile->tLastShiftTime + 60 <= time(0))
				return 1;
			break;
		default:
			break;
	}
	return 0;
}

static void closeFile(C2_LogFile * pstLogFile)
{
	if(pstLogFile->iFd){
		close(pstLogFile->iFd - 1);
		pstLogFile->iFd = 0;
	}
}

static int shiftFile(C2_LogFile * pstLogFile)
{
	const int PATH_LEN = sizeof pstLogFile->sLogFileName;
	closeFile(pstLogFile);
	if(!pstLogFile->dwMaxLogNum){
		remove(pstLogFile->sLogFileName);
	}else{
		char sNew[PATH_LEN+200], sOld[PATH_LEN+200];
		int i = pstLogFile->dwMaxLogNum - 1;
		for(;i > 0;--i){
			snprintf(sOld, PATH_LEN, "%s.%d", pstLogFile->sLogFileName, i);
			if(access(sOld, F_OK) == 0){
				snprintf(sNew, PATH_LEN, "%s.%d", pstLogFile->sLogFileName, i + 1);
				if(rename(sOld, sNew) < 0 )
					return -1;
			}
		}
		snprintf(sNew, PATH_LEN, "%s.%d", pstLogFile->sLogFileName, 1);
		rename(pstLogFile->sLogFileName, sNew);
	}
	time(&pstLogFile->tLastShiftTime);
	pstLogFile->dwCur = 0;
	return 0;
}

int C2_InitLogFile(C2_LogFile * pstLogFile, const C2_LogFileParam * pstLogFileParam)
{
	int iRet = setParams(pstLogFile, pstLogFileParam);
	if(iRet != 0)
		return iRet;
	if((iRet = initFileLog(pstLogFile)) != 0)
		return iRet;
	if(pstLogFile->iOldStyle)
		closeFile(pstLogFile);
	return 0;
}

int C2_Log_FreqControl(C2_LogFile * pstLogFile, C2_ELogLevel eLogLevel, const struct timeval *ptvNow)
{
	if(eLogLevel <= LL_INFO)
		return 1;	//no freq control
	TokenBucket_Gen(&pstLogFile->stWriteSpeed, ptvNow);
	if(0 == TokenBucket_Get(&pstLogFile->stWriteSpeed, 1))
		return 1;
	++pstLogFile->iSkipTimes;
	return 0;
}

inline static int32_t writeBuffer(char * pBuf, uint32_t dwLen, C2_LogFile * pstLogFile, C2_ELogLevel eLogLevel, const char * pszHeader, const char * pszFormat, va_list ap)
{
	int iRet;
	//time
	int32_t iWrite = 0;
	if(pstLogFile->iIsLogTime){
		struct timeval stNow;
		struct tm stTm;
		gettimeofday(&stNow, 0);
		localtime_r(&stNow.tv_sec, &stTm);
		iWrite += strftime(pBuf, dwLen, "%Y-%m-%d %H:%M:%S", &stTm);
		iWrite += snprintf(pBuf + iWrite, dwLen - iWrite, ".%06u ", (uint32_t)stNow.tv_usec);
	}
	//level & header
	iWrite += snprintf(pBuf + iWrite, LVL_NAME_LEN, "%s", g_aLevelName[eLogLevel]);
	if(pszHeader && pszHeader[0] != '\0') {
		iRet = snprintf(pBuf + iWrite, dwLen - iWrite, (pstLogFile->iSkipTimes ? "%s -%d- " : "%s - "), pszHeader, pstLogFile->iSkipTimes);	
		if(iRet < 0)
			return -1;
		iWrite += iRet;
		pstLogFile->iSkipTimes = 0;
	}else{
		memcpy(pBuf + iWrite, " - ", 3);
		iWrite += 3;
	}
	if((uint32_t)iWrite >= dwLen)
		return -1;
	//format msg
	iRet = vsnprintf(pBuf + iWrite, dwLen - iWrite, pszFormat, ap);
	if(iRet < 0)
		return -1;
	iWrite += iRet;
	if((uint32_t)iWrite < dwLen)
		pBuf[iWrite++] = '\n';
	else
		pBuf[dwLen - 1] = '\n';
	return iWrite;
}

int C2_Log(C2_LogFile * pstLogFile, C2_ELogLevel eLogLevel, const char * pszHeader, const char * pszFormat, ...)
{
	static char sBuf[MAX_LINE_LEN];	//slackware cannot handle too much stack space
	int32_t iWrite;
	va_list ap;
	int iRet;
	if(!pstLogFile || eLogLevel < pstLogFile->eLogLevel)
		return 0;
	if(!pstLogFile->iOldStyle){		//write file log
		if(!pstLogFile->iFd)
			return -1;
		if(needShift(pstLogFile) && (shiftFile(pstLogFile) != 0 || openFile(pstLogFile) != 0))
			return -2;
		va_start(ap, pszFormat);
		iWrite = writeBuffer(sBuf, sizeof sBuf, pstLogFile, eLogLevel, pszHeader, pszFormat, ap);
		va_end(ap);
		if(iWrite < 0)
			return -2;
		if((uint32_t)iWrite > sizeof sBuf)
			iWrite = sizeof sBuf;
		iWrite = write(pstLogFile->iFd - 1, sBuf, iWrite);
		if(iWrite < 0)
			return -2;
		//stats
		switch(pstLogFile->eShiftType){
			case ST_SIZE:
				pstLogFile->dwCur += iWrite;
				break;
			case ST_LINES:
				++pstLogFile->dwCur;
				break;
			default:;
		}
	}else{		//old style file log (multi-process)
		if(0 != (iRet = initFileLog(pstLogFile))){
			closeFile(pstLogFile);
			return iRet;
		}
		if(needShift(pstLogFile) && (shiftFile(pstLogFile) != 0 || openFile(pstLogFile) != 0)){
			closeFile(pstLogFile);
			return -2;
		}
		va_start(ap, pszFormat);
		iWrite = writeBuffer(sBuf, sizeof sBuf, pstLogFile, eLogLevel, pszHeader, pszFormat, ap);
		va_end(ap);
		if(iWrite < 0){
			closeFile(pstLogFile);
			return -2;
		}
		if((uint32_t)iWrite > sizeof sBuf)
			iWrite = sizeof sBuf;
		iWrite = write(pstLogFile->iFd - 1, sBuf, iWrite);
		if(iWrite < 0){
			closeFile(pstLogFile);
			return -2;
		}
		closeFile(pstLogFile);
	}
	return 0;
}

int C2_CloseLog(C2_LogFile * pstLogFile)
{
	closeFile(pstLogFile);
	return 0;
}

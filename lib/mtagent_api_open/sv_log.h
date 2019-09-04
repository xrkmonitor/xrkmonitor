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

#ifndef _SV_LOG_H
#define _SV_LOG_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>

#include "sv_freq_ctrl.h"

typedef enum {
	ST_SIZE,		// shift by size
	ST_LINES,		// shift by lines
	ST_INTERVAL,	// shift by interval
	ST_DAY,			// shift by day
	ST_HOUR,		// shift by hour
	ST_MIN			// shift by min
}C2_EShiftType;

typedef enum {
	LL_TRACE,
	LL_DEBUG,
	LL_INFO,
	LL_WARN,
	LL_ERROR,
	LL_FATAL,
	LL_OFF
}C2_ELogLevel;

typedef struct {
	//file log
	int32_t		iFd;			
	C2_EShiftType	eShiftType;
	uint32_t	dwMaxLogNum;	
	uint32_t	dwMax;			
	time_t		tLastShiftTime;
	uint32_t	dwCur;			
	int			iOldStyle;		
	//speed limit
	TokenBucket	stWriteSpeed;	
	int			iSkipTimes;
	//common
	int			iIsLogTime;
	int			iIsLogToStd;
	C2_ELogLevel	eLogLevel;
	char		sLogFileName[256];
}C2_LogFile;

typedef struct {
	int iIsLogToStd;
	const char * pszLogBaseName;
	const char * pszLogFullName;
	C2_ELogLevel eLogLevel;		
	int iIsLogTime;				
	C2_EShiftType eShiftType;	
	uint32_t dwMax;				
	uint32_t dwMaxLogNum;		
	int iOldStyle;
	int iWriteSpeed;			
}C2_LogFileParam;

C2_ELogLevel C2_LogLvl(const char * pszLogLevel);

int C2_InitLogFile(C2_LogFile * pstLogFile, const C2_LogFileParam * pstLogFileParam);

int C2_Log_FreqControl(C2_LogFile * pstLogFile, C2_ELogLevel eLogLevel, const struct timeval *ptvNow);

int C2_Log(C2_LogFile * pstLogFile, C2_ELogLevel eLogLevel, const char * pszHeader, const char * pszFormat, ...);

int C2_CloseLog(C2_LogFile * pstLogFile);

#define TRACE(pstLogFile, pszFormat, args...)	\
	do{	\
		if(pstLogFile && (pstLogFile)->eLogLevel <= LL_TRACE)	\
			C2_Log(pstLogFile, LL_TRACE, __FUNCTION__, pszFormat, ## args);	\
	} while(0)
#define DEBUG(pstLogFile, pszFormat, args...)	\
	do{	\
		if(pstLogFile && (pstLogFile)->eLogLevel <= LL_DEBUG)	\
			C2_Log(pstLogFile, LL_DEBUG, __FUNCTION__, pszFormat, ## args);	\
	} while(0)
#define INFO(pstLogFile, pszFormat, args...)	\
	do{	\
		if(pstLogFile && (pstLogFile)->eLogLevel <= LL_INFO)	\
			C2_Log(pstLogFile, LL_INFO, __FUNCTION__, pszFormat, ## args);	\
	} while(0)
#define WARN(pstLogFile, pszFormat, args...)	\
	do{	\
		if(pstLogFile && (pstLogFile)->eLogLevel <= LL_WARN){	\
			if(C2_Log_FreqControl(pstLogFile, LL_WARN, NULL))	\
				C2_Log(pstLogFile, LL_WARN, __FUNCTION__, pszFormat, ## args);	\
		}	\
	}while(0)
#define ERROR(pstLogFile, pszFormat, args...)	\
	do{	\
		if(pstLogFile && (pstLogFile)->eLogLevel <= LL_ERROR){	\
			if(C2_Log_FreqControl(pstLogFile, LL_ERROR, NULL))	\
				C2_Log(pstLogFile, LL_ERROR, __FUNCTION__, pszFormat, ## args);	\
		}	\
	} while(0)
#define FATAL(pstLogFile, pszFormat, args...)	\
	do {	\
		fprintf(stderr, pszFormat, ## args);	\
		fprintf(stderr, "\n");	\
		if(pstLogFile && (pstLogFile)->eLogLevel <= LL_FATAL){	\
			C2_Log (pstLogFile, LL_FATAL, __FUNCTION__, pszFormat, ## args);	\
		}	\
	} while(0)


#endif


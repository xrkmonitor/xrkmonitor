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

   模块 slog_write 功能:
        slog_write 用于将共享内存中的日志写入磁盘，支持多进程部署 

****/

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <errno.h>
#include "top_include_comm.h"
#include <set>

#define CONFIG_FILE "./slog_write.conf"
#define MAX_SLOG_SHM_FILE_PER_PROCESS 1000

#pragma pack(1)

// 用于向子进程新增写 app log 
typedef struct {
	int iAppId; // 要新增的 appid
	int iAppShmIndex; // 要新增的 appinfo shm index (因为是共享内存所有这里使用索引而不用指针)
	int iProcessId;
	uint32_t dwWriteRecordsAll; // 总共写入的日志记录数
	uint32_t dwWriteRecordsTotal; // 子进程写，父进程读，用于负载均衡
	uint32_t dwWriteRecordsPer;
	int iAppWriteTotal; // 总共负责写入的 app 数目
	int iArrAppIdList[MAX_SLOG_SHM_FILE_PER_PROCESS]; 
}TAddWriteAppLog;

#pragma pack()

typedef struct
{
	char szLocalIp[20];
	char szLogPath[MAX_PATH];
	time_t tmStart;
	int iLogSizeOutPerTime;
	int iPerShowTimeSec;
	int iScanAgain;
	int iCheckWriteProcessTime;
	SLogAppInfo *pAppShmInfoList;
	int iWriteRecordsPerLoop;

	int iAddWriteAppLogShmKey;
	TAddWriteAppLog *pAddWriteAppLogShm;
	TAddWriteAppLog *pAddWriteAppLogShmCur;

	int iLogFileUseCount;
	CSLogServerWriteFile *pstLogFile[MAX_SLOG_SHM_FILE_PER_PROCESS];
	int iLocalMachineId;
	MachineInfo *pLocalMachineInfo;
	std::set<int> stAppWrite;
}CONFIG;

CONFIG stConfig;
CSupperLog slog;

int Init(const char *pFile = NULL)
{
	const char *pConfFile = NULL;
	if(pFile != NULL)
		pConfFile = pFile;
	else
		pConfFile = CONFIG_FILE;

	int32_t iRet = 0;
	if((iRet=LoadConfig(pConfFile,
		"LOCAL_IP", CFG_STRING, stConfig.szLocalIp, "", sizeof(stConfig.szLocalIp),
		"SLOG_SERVER_FILE_PATH", CFG_STRING, stConfig.szLogPath, DEF_SLOG_LOG_FILE_PATH, sizeof(stConfig.szLogPath),
		"WRITE_SHOW_PER_SECS", CFG_INT, &stConfig.iPerShowTimeSec, 300,
		"SCAN_LOGFILE_AGAIN", CFG_INT, &stConfig.iScanAgain, 1,
		"LOCAL_MACHINE_ID", CFG_INT, &stConfig.iLocalMachineId, 0,
		"SLOG_PROCESS_COUNT", CFG_INT, &slog.m_iProcessCount, 4,
		"ADD_WRITE_APP_LOG_KEY", CFG_INT, &stConfig.iAddWriteAppLogShmKey, 20190203,
		"LOG_SIZE_PER_TIME", CFG_INT, &stConfig.iLogSizeOutPerTime, 5,
		"CHECK_WRITE_PROCESS_TIME", CFG_INT, &stConfig.iCheckWriteProcessTime, 30,
		"WRITE_RECORDS_PER_LOOP", CFG_INT, &stConfig.iWriteRecordsPerLoop, 200,
		(void*)NULL)) < 0)
	{   
		ERR_LOG("LoadConfig:%s failed ! ret:%d", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	} 

	// 不能太小，跟分发逻辑有关
	if(stConfig.iCheckWriteProcessTime < 10)
	{
		WARN_LOG("CHECK_WRITE_PROCESS_TIME too small, change from:%d to 10", stConfig.iCheckWriteProcessTime);
		stConfig.iCheckWriteProcessTime = 10;
	}

	if(stConfig.szLocalIp[0] == '\0' || INADDR_NONE == inet_addr(stConfig.szLocalIp))
		GetCustLocalIP(stConfig.szLocalIp);
	if(stConfig.szLocalIp[0] == '\0' || INADDR_NONE == inet_addr(stConfig.szLocalIp))
	{
		ERR_LOG("get local ip failed, use LOCAL_IP to set !");
		return SLOG_ERROR_LINE;
	}

	if((iRet=slog.InitConfigByFile(pConfFile)) < 0 || (iRet=slog.Init(stConfig.szLocalIp)) < 0)
	{ 
		ERR_LOG("slog init failed file:%s ret:%d\n", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	}

	if(stConfig.iPerShowTimeSec <= 0 || stConfig.iPerShowTimeSec > 300) 
	{
		WARN_LOG("invalid show time sec :%d should(1-300) set 6 sec", stConfig.iPerShowTimeSec);
		stConfig.iPerShowTimeSec = 6;
	}

	stConfig.pAppShmInfoList = slog.GetAppInfo();
	if(stConfig.pAppShmInfoList == NULL)
	{
		ERR_LOG("get app shm info list failed");
	    return SLOG_ERROR_LINE;
	}

	if(slog.InitMachineList() < 0)
	{
		FATAL_LOG("InitMachineList failed !");
		return SLOG_ERROR_LINE;
	}

	SLogConfig *pShmConfig = slog.GetSlogConfig();
	if(pShmConfig != NULL && stConfig.iLocalMachineId==0)
		stConfig.iLocalMachineId = pShmConfig->stSysCfg.iMachineId;

	if(stConfig.iLocalMachineId != 0) 
		stConfig.pLocalMachineInfo = slog.GetMachineInfo(stConfig.iLocalMachineId, NULL);
	else	
	{
		stConfig.pLocalMachineInfo = slog.GetMachineInfoByIp(stConfig.szLocalIp);
		if(stConfig.pLocalMachineInfo != NULL)
			stConfig.iLocalMachineId = stConfig.pLocalMachineInfo->id;
	}

	if(stConfig.iLocalMachineId == 0 || stConfig.pLocalMachineInfo==NULL)
	{
		ERR_LOG("local machine not set or get failed, machine:%d", stConfig.iLocalMachineId);
		return SLOG_ERROR_LINE;
	}
	INFO_LOG("local:%s, machine id:%d, log file path:%s", 
		stConfig.szLocalIp, stConfig.iLocalMachineId, stConfig.szLogPath);
	return 0;
}

void ShowAppWriteDispach()
{
	if(stConfig.pAddWriteAppLogShm == NULL) {
		fprintf(stderr, "failed , pAddWriteAppLogShm is NULL");
		return;
	}
	AppInfo * pAppInfo = NULL;
	for(int i=0; i < slog.m_iProcessCount-1; i++)
	{
		stConfig.pAddWriteAppLogShmCur = stConfig.pAddWriteAppLogShm + i;
		fprintf(stdout, "process id:%d write app info \n", stConfig.pAddWriteAppLogShmCur->iProcessId);
		fprintf(stdout, "write log records all:%u, total: %d, per min:%d\n",  
			stConfig.pAddWriteAppLogShmCur->dwWriteRecordsAll,
			stConfig.pAddWriteAppLogShmCur->dwWriteRecordsTotal, stConfig.pAddWriteAppLogShmCur->dwWriteRecordsPer);
		fprintf(stdout, "write app count:%d \n", stConfig.pAddWriteAppLogShmCur->iAppWriteTotal);
		for(int j=0; j < stConfig.pAddWriteAppLogShmCur->iAppWriteTotal; j++) {
			fprintf(stdout, "\t(%d) -- appid:%d\n", j+1, stConfig.pAddWriteAppLogShmCur->iArrAppIdList[j]);
			pAppInfo = slog.GetAppInfo(stConfig.pAddWriteAppLogShmCur->iArrAppIdList[j]);
			if(pAppInfo != NULL)  {
				fprintf(stdout, "\t --- appinfo -- \n");
				pAppInfo->Show();
			}
			else
				fprintf(stdout, "\t -- GetAppInfo failed !\n");
		}
		fprintf(stdout, "\n");
	}
}

// 参数：j - app 索引, tmTmp - 当前时间
void TryDispatchAppWrite(int j, uint32_t tmTmp)
{
	// 无日志可写或者最后一次写时间未过期 说明不需要指派
	if(!IS_SET_BIT(stConfig.pAppShmInfoList->stInfo[j].dwAppLogFlag, APPLOG_FLAG_LOG_WRITED) 
		|| tmTmp-(uint32_t)stConfig.iCheckWriteProcessTime < stConfig.pAppShmInfoList->stInfo[j].dwLastTryWriteLogTime) 
	{
		if(tmTmp-(uint32_t)stConfig.iCheckWriteProcessTime < stConfig.pAppShmInfoList->stInfo[j].dwLastTryWriteLogTime
			&& stConfig.stAppWrite.find(stConfig.pAppShmInfoList->stInfo[j].iAppId) == stConfig.stAppWrite.end()) 
		{
			stConfig.stAppWrite.insert(stConfig.pAppShmInfoList->stInfo[j].iAppId);
		}

		if(!stConfig.pAppShmInfoList->stInfo[j].bReadLogStatInfo)
		{
			CSLogServerWriteFile logFile(stConfig.pAppShmInfoList->stInfo+j, stConfig.szLogPath, 0);
			if(logFile.IsInit()) 
			{
				INFO_LOG("create shm for app:%d, logshm:%d, logfileshm:%d ok, logsize:%lu",
					stConfig.pAppShmInfoList->stInfo[j].iAppId,
					stConfig.pAppShmInfoList->stInfo[j].iAppLogShmKey,
					stConfig.pAppShmInfoList->stInfo[j].iAppLogFileShmKey,
					stConfig.pAppShmInfoList->stInfo[j].stLogStatInfo.qwLogSizeInfo);
				stConfig.pAppShmInfoList->stInfo[j].bReadLogStatInfo = true;
			}
		}
		return;
	}
	stConfig.pAppShmInfoList->stInfo[j].dwLastTryWriteLogTime = tmTmp;
	CLEAR_BIT(stConfig.pAppShmInfoList->stInfo[j].dwAppLogFlag, APPLOG_FLAG_LOG_WRITED);

	// 寻找一个相对空闲的进程，派发给其写日志
	int jj=0, kk=-1;
	for(kk=-1, jj=0; jj < slog.m_iProcessCount-1; jj++) 
	{
		if(stConfig.pAddWriteAppLogShm[jj].iAppId == 0) {
			if(kk >= 0
				&& stConfig.pAddWriteAppLogShm[jj].iAppWriteTotal 
					< MAX_SLOG_SHM_FILE_PER_PROCESS
				&& (stConfig.pAddWriteAppLogShm[kk].dwWriteRecordsPer >
				     stConfig.pAddWriteAppLogShm[jj].dwWriteRecordsPer))
				kk = jj;
			else if(kk < 0)
				kk = jj;
		}
	}

	if(kk >= 0) {
		stConfig.pAddWriteAppLogShm[kk].iAppId = stConfig.pAppShmInfoList->stInfo[j].iAppId;
		stConfig.pAddWriteAppLogShm[kk].iAppShmIndex = j;
		INFO_LOG("app log dispach to process:%d, appid:%d(cur:%u, check:%d, last try:%u)",
			stConfig.pAddWriteAppLogShm[kk].iProcessId,
			stConfig.pAppShmInfoList->stInfo[j].iAppId, tmTmp, stConfig.iCheckWriteProcessTime, 
			stConfig.pAppShmInfoList->stInfo[j].dwLastTryWriteLogTime);
	}
	else {
		WARN_LOG("dispach write log - for app:%d failed !",
			stConfig.pAppShmInfoList->stInfo[j].iAppId);
		usleep(10);
	}
}

void TryAcceptDispatchApp()
{
	if(stConfig.pAddWriteAppLogShmCur->iAppId == 0) 
		return ;

	if(stConfig.iLogFileUseCount >= MAX_SLOG_SHM_FILE_PER_PROCESS) {
		FATAL_LOG("bug - accept app log write failed - for appid:%d",
			stConfig.pAddWriteAppLogShmCur->iAppId);
		exit(-2);
	}

	CSLogServerWriteFile *pLogFile = NULL;
	pLogFile = new CSLogServerWriteFile(
		stConfig.pAppShmInfoList->stInfo+stConfig.pAddWriteAppLogShmCur->iAppShmIndex,
		stConfig.szLogPath, stConfig.iScanAgain);

	if(!pLogFile->IsInit()) 
	{
		WARN_LOG("create CSLogServerWriteFile init failed for appid:%d",
			stConfig.pAddWriteAppLogShmCur->iAppId);
		exit(-1);
	}
	stConfig.pstLogFile[stConfig.iLogFileUseCount] = pLogFile;
	stConfig.iLogFileUseCount++;

	stConfig.pAddWriteAppLogShmCur->iArrAppIdList[stConfig.pAddWriteAppLogShmCur->iAppWriteTotal]
		= stConfig.pAddWriteAppLogShmCur->iAppId;
	stConfig.pAddWriteAppLogShmCur->iAppWriteTotal++;

	INFO_LOG("accept app log write appid:%d, log file use count:%d, write total:%d",
		stConfig.pAddWriteAppLogShmCur->iAppId, stConfig.iLogFileUseCount, 
		stConfig.pAddWriteAppLogShmCur->iAppWriteTotal);
	stConfig.pAddWriteAppLogShmCur->iAppId = 0;
}

int main(int argc, char *argv[])
{
	int iRet = 0;
	stConfig.tmStart = time(NULL);
	if(argc > 1 && strcmp(argv[1], "show") && (iRet=Init(argv[1])) < 0)
	{
		ERR_LOG("Init Failed ret:%d !", iRet);
		return SLOG_ERROR_LINE;
	}

	if((iRet=Init(NULL)) < 0)
	{
		ERR_LOG("Init Failed ret:%d !", iRet);
		return SLOG_ERROR_LINE;
	}

	int iShmSize = sizeof(TAddWriteAppLog)*100;
	if(iShmSize < 8192)
		iShmSize = 8192;
	stConfig.pAddWriteAppLogShm = (TAddWriteAppLog*)GetShm(stConfig.iAddWriteAppLogShmKey, iShmSize, IPC_CREAT|0666);
	if(stConfig.pAddWriteAppLogShm == NULL) {
		FATAL_LOG("get pAddWriteAppLogShm failed, key:%d size:%u", stConfig.iAddWriteAppLogShmKey, iShmSize);
		return SLOG_ERROR_LINE;
	}

	//  for show some info 
	if(argc > 1 && strstr(argv[1], "show"))
	{
		if(argc <= 2 || !strcmp(argv[2], "help")) {
			printf("show cmd as followed: \n");
			printf("\t show logself\n");
			printf("\t show write\n");
			return 0;
		}
		else if(!strcmp(argv[2], "write"))
			ShowAppWriteDispach();
		else if(!strcmp(argv[2], "logself"))
			slog.ShowShmLogInfo();
		return 0;
	}

	INFO_LOG("slog_write start !");
	int32_t i=0, iWriteCount=0;
	time_t tmTmp = 0, tmLastLogTime = 0;
	slog.Daemon(1, 1, 0);

	// 子进程
	if(slog.m_iProcessId > 0) {
		stConfig.pAddWriteAppLogShmCur = stConfig.pAddWriteAppLogShm + slog.m_iProcessId-1;
		stConfig.pAddWriteAppLogShmCur->iProcessId = slog.m_iProcessId;
		stConfig.pAddWriteAppLogShmCur->dwWriteRecordsAll = 0;
		stConfig.pAddWriteAppLogShmCur->dwWriteRecordsTotal = 0;
		stConfig.pAddWriteAppLogShmCur->dwWriteRecordsPer = 0;
		stConfig.pAddWriteAppLogShmCur->iAppId = 0;
		stConfig.pAddWriteAppLogShmCur->iAppWriteTotal = 0;
	}

	while(slog.Run())
	{
		tmTmp = slog.m_stNow.tv_sec;
		if(slog.m_iProcessId != 0) 
		{
			for(i=0, iWriteCount=0; i < stConfig.iLogFileUseCount; i++) 
			{
				if(stConfig.pstLogFile[i] == NULL) {
					FATAL_LOG("check failed pstLogFile :%d is NULL -- process:%d", i, slog.m_iProcessId);
					exit(-1);
				}
				if((iRet=stConfig.pstLogFile[i]->WriteFile(stConfig.iWriteRecordsPerLoop, tmTmp)) < 0)
				{
					FATAL_LOG("WriteFile failed, ret:%d -- process:%d", iRet, slog.m_iProcessId);
					exit(-2);
				}
				if(iRet > 0) {
					MtReport_Attr_Add(67, iRet);
					MtReport_Attr_Add(332, iRet);
					stConfig.pAddWriteAppLogShmCur->dwWriteRecordsAll += iRet;
					stConfig.pAddWriteAppLogShmCur->dwWriteRecordsTotal += iRet;
					stConfig.pAddWriteAppLogShmCur->dwWriteRecordsPer += iRet;
					iWriteCount += iRet;
				}

				// 主进程分发逻辑，如果子进程写日志占用太多时间，可能导致某个app分发到两个子进程
				if(time(NULL) > tmTmp+stConfig.iCheckWriteProcessTime-2)
				{
					INFO_LOG("process:%d write log records use too much time-:%d", 
						slog.m_iProcessId, (int)(time(NULL)-tmTmp+stConfig.iCheckWriteProcessTime-2));
					break;
				}
			}
			if(iWriteCount >= stConfig.iWriteRecordsPerLoop)
				usleep(1000);
			else
				usleep(2000);
			TryAcceptDispatchApp();
		}
		else 
		{
			// 主进程定时写日志信息
			if(tmTmp >= tmLastLogTime+stConfig.iPerShowTimeSec) {
				for(i=0; i < slog.m_iProcessCount-1; i++) 
				{
					stConfig.pAddWriteAppLogShmCur = stConfig.pAddWriteAppLogShm + i;
					INFO_LOG("write log records total:%u, per min:%u, write app count:%d, process id:%d", 
						stConfig.pAddWriteAppLogShmCur->dwWriteRecordsTotal,
						stConfig.pAddWriteAppLogShmCur->dwWriteRecordsPer*60/stConfig.iPerShowTimeSec,
						stConfig.pAddWriteAppLogShmCur->iAppWriteTotal, stConfig.pAddWriteAppLogShmCur->iProcessId);
					tmLastLogTime = tmTmp;
					stConfig.pAddWriteAppLogShmCur->dwWriteRecordsPer = 0;
				}
			}

			// 扫描应用列表看是否有应用需要写日志
			for(int j=0; j < MAX_SLOG_APP_COUNT; j++)
			{
				if(stConfig.pAppShmInfoList->stInfo[j].iAppId != 0) 
				{
				    // log server 不是本机则不写日志
					if(slog.IsIpMatchMachine(stConfig.pLocalMachineInfo, 
						stConfig.pAppShmInfoList->stInfo[j].dwAppSrvMaster) != 1)
					{
						DEBUG_LOG("skip app:%d, server not match", stConfig.pAppShmInfoList->stInfo[j].iAppId);
						continue;
					}
					TryDispatchAppWrite(j, tmTmp);
				}
			} // check all app list

			int iSleepTime = stConfig.iPerShowTimeSec;
			if(iSleepTime > stConfig.iLogSizeOutPerTime)
				iSleepTime = stConfig.iLogSizeOutPerTime;
			if(iSleepTime > 10)
				sleep(10);
			else
				sleep(iSleepTime);
		} // process 0
	} // -- while
	INFO_LOG("slog_write exit ! - process id:%d", slog.m_iProcessId);
	return 0;
}


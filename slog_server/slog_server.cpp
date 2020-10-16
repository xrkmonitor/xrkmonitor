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

   模块 slog_server 功能:
         接收日志客户端上报的日志，并将日志写入本机共享内存中

****/

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <errno.h>
#include "top_include_comm.h"
#include "udp_sock.h"
#include "top_proto.pb.h"
#include "slog_server.h"

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
		"LOCAL_LISTEN_IP", CFG_STRING, stConfig.szListenIp, "0.0.0.0", sizeof(stConfig.szListenIp),
		"LOCAL_RECV_PORT", CFG_INT, &stConfig.iRecvPortMonitor, 28080, 
		"CHECK_EACH_APP_LOG_SIZE_TIME_SEC", CFG_INT, &stConfig.iCheckEachAppLogSpaceTime, 20,
		"CHECK_LOG_SPACE_TIME_SEC", CFG_INT, &stConfig.iCheckLogSpaceTime, 10,
		"LOCAL_MACHINE_ID", CFG_INT, &stConfig.iLocalMachineId, 0, 
		"QUICK_TO_SLOW_IP", CFG_STRING, stConfig.szQuickToSlowIp, "127.0.0.1", sizeof(stConfig.szQuickToSlowIp),
		"QUICK_TO_SLOW_PORT", CFG_INT, &stConfig.iQuickToSlowPort, 38081,
		(void*)NULL)) < 0)
	{   
		ERR_LOG("LoadConfig:%s failed ! ret:%d", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	} 

	if((iRet=slog.InitConfigByFile(pConfFile)) < 0 || (iRet=slog.Init()) < 0)
	{ 
		ERR_LOG("slog init failed file:%s ret:%d\n", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	}

	if(slog.InitMachineList() < 0)
	{
		ERR_LOG("init machine list shm failed !");
		return SLOG_ERROR_LINE;
	}

	stConfig.psysConfig = slog.GetSystemCfg();
	if(stConfig.psysConfig == NULL) {
		ERR_LOG("GetSystemCfg failed");
		return SLOG_ERROR_LINE;
	}

	SLogConfig *pShmConfig = slog.GetSlogConfig();
	if(pShmConfig != NULL && stConfig.iLocalMachineId==0)
		stConfig.iLocalMachineId = pShmConfig->stSysCfg.iMachineId;

	if(stConfig.iLocalMachineId != 0) 
		stConfig.pLocalMachineInfo = slog.GetMachineInfo(stConfig.iLocalMachineId, NULL);
	else 
	{
		stConfig.pLocalMachineInfo = slog.GetMachineInfoByIp((char*)(slog.GetLocalIP()));
		if(stConfig.pLocalMachineInfo != NULL)
			stConfig.iLocalMachineId = stConfig.pLocalMachineInfo->id;
	}

	if(stConfig.iLocalMachineId==0 || stConfig.pLocalMachineInfo==NULL)
	{
		ERR_LOG("local machine not set or get failed, machine:%d", stConfig.iLocalMachineId);
		return SLOG_ERROR_LINE;
	}

	stConfig.pShmConfig = slog.GetSlogConfig();
	if(stConfig.pShmConfig == NULL)
	{
		FATAL_LOG("get pShmConfig failed !");
		return SLOG_ERROR_LINE;
	}
	INFO_LOG("server listen ip:%s, base port:%d, local machine id:%d-ip:%s", 
		stConfig.szListenIp, stConfig.iRecvPortMonitor, stConfig.iLocalMachineId, slog.GetLocalIP());
	return 0;
}

void ScanAppLogSpaceByIdx(user::UserRemoteAppLogInfo & stPb, int iAppCount, int iAppInfoIdx)
{
	AppInfo * pAppInfo = NULL;
	int iTmpRemainAppCount = iAppCount;
	SLogFile * pLogFile = NULL;
	uint64_t qwTmpAppLogSize = 0;
	for(int i=0; i < iAppCount; i++)
	{
		if(iAppInfoIdx < 0 || iAppInfoIdx >= MAX_SLOG_APP_COUNT)
		{
			WARN_LOG("invalid app index:%d", iAppInfoIdx);
			MtReport_Attr_Add(252, 1);
			break;
		}
		pAppInfo = slog.GetAppInfoByIndex(iAppInfoIdx);
		if(pAppInfo->dwAppSrvMaster == 0 || pAppInfo->wLogSrvPort == 0)
		{
			ERR_LOG("invalid app log server, appid:%d", pAppInfo->iAppId);
			MtReport_Attr_Add(252, 1);
			continue;
		}

		if(slog.IsIpMatchMachine(pAppInfo->dwAppSrvMaster, stConfig.iLocalMachineId) == 1)
		{
			// app 日志在本机
			iTmpRemainAppCount--;
			std::map<int , SLogFile*>::iterator it = stConfig.mapAppLogFileShm.find(pAppInfo->iAppId);
			if(it != stConfig.mapAppLogFileShm.end())
			{
				pLogFile =  it->second;
			}
			else {
				pLogFile = CSLogServerWriteFile::GetAppLogFileShm(pAppInfo);
				if(NULL != pLogFile)
					stConfig.mapAppLogFileShm[pAppInfo->iAppId] = pLogFile;
			}

			if(NULL == pLogFile)
				DEBUG_LOG("get app log file failed, app:%d", pAppInfo->iAppId);
			else
			{
				qwTmpAppLogSize += pAppInfo->stLogStatInfo.qwLogSizeInfo;
				DEBUG_LOG("get app:%d(%d), log size:%lu, file count:%d, cur tmp total:%lu", 
					pAppInfo->iAppId, pLogFile->iAppId,
					pAppInfo->stLogStatInfo.qwLogSizeInfo, pLogFile->wLogFileCount, qwTmpAppLogSize);
				if(pLogFile->wLogFileCount > 0)
				{
					if(stPb.oldest_log_file_time() == 0 
						|| stPb.oldest_log_file_time() > (uint32_t)(pLogFile->stFiles[0].qwLogTimeStart/1000000))
					{
						stPb.set_oldest_log_file_time((uint32_t)(pLogFile->stFiles[0].qwLogTimeStart/1000000));
						stPb.set_oldest_log_file_app_id(pAppInfo->iAppId);
						DEBUG_LOG("get oldest log app info, app id:%d, time:%" PRIu64 ", %u(%s)",
							pAppInfo->iAppId, pLogFile->stFiles[0].qwLogTimeStart,
							stPb.oldest_log_file_time(), uitodate(stPb.oldest_log_file_time()));
					}
				}
			}
		}
		else
		{
			// app 日志在其它机器，调用接口查询
			TGetAppLogSizeKey key;
			key.dwAppLogSrv = pAppInfo->dwAppSrvMaster;
			key.wAppLogSrvPort = pAppInfo->wLogSrvPort;
			std::map<TGetAppLogSizeKey, top::SlogGetAppLogSizeReq*, ReqAppLogSizeCmp>::iterator it;
			if((it=stConfig.stMapLogSizeReqInfo.find(key)) != stConfig.stMapLogSizeReqInfo.end())
			{
				it->second->add_appid_list(pAppInfo->iAppId);
				DEBUG_LOG("add appid:%d, to req app log size info, ip:%s|%d", 
					pAppInfo->iAppId, ipv4_addr_str(key.dwAppLogSrv), key.wAppLogSrvPort);
			}
			else
			{
				top::SlogGetAppLogSizeReq *preq = new top::SlogGetAppLogSizeReq;
				preq->add_appid_list(pAppInfo->iAppId);
				stConfig.stMapLogSizeReqInfo[key] = preq;
				DEBUG_LOG("init appid:%d, to req app log size info, ip:%s|%d", 
					pAppInfo->iAppId, ipv4_addr_str(key.dwAppLogSrv), key.wAppLogSrvPort);
			}
		}
		iAppInfoIdx = pAppInfo->iNextIndex;
	}

	iTmpRemainAppCount += stPb.tmp_remain_app_count();
	if(iTmpRemainAppCount > 0) 
		stPb.set_tmp_remain_app_count(iTmpRemainAppCount);
	qwTmpAppLogSize += stPb.total_app_log_size();
	stPb.set_total_app_log_size(qwTmpAppLogSize);
}

void ClearMapLogSizeReq()
{
	DEBUG_LOG("clear MapLogSizeReq, size:%lu", stConfig.stMapLogSizeReqInfo.size());
	std::map<TGetAppLogSizeKey, top::SlogGetAppLogSizeReq*, ReqAppLogSizeCmp>::iterator it;
	for(it = stConfig.stMapLogSizeReqInfo.begin(); it != stConfig.stMapLogSizeReqInfo.end(); it++)
	{
		delete it->second;
	}
	stConfig.stMapLogSizeReqInfo.clear();
}

void ScanAppLogSpace()
{
	static uint32_t s_dwLastScanTime = 0;
	if(stConfig.dwCurrentTime < s_dwLastScanTime)
		return;
	if(stConfig.stMapLogSizeReqInfo.size() > 0)
	{
		ERR_LOG("bug map stMapLogSizeReqInfo size:%lu", stConfig.stMapLogSizeReqInfo.size());
		MtReport_Attr_Add(252, 1);
		ClearMapLogSizeReq();
	}
	s_dwLastScanTime = stConfig.dwCurrentTime+stConfig.iCheckLogSpaceTime+rand()%5;

	user::UserRemoteAppLogInfo stPb;
	slog.GetUserRemoteAppLogInfoPb(stPb);

	// 检查是否到达扫描日志时间
	if(stPb.next_check_log_space_time()+stConfig.iCheckEachAppLogSpaceTime > stConfig.dwCurrentTime)
	{
		DEBUG_LOG("skip check app log size, time:%u > %u",
			stPb.next_check_log_space_time()+stConfig.iCheckEachAppLogSpaceTime, stConfig.dwCurrentTime);
		return ;
	}
	stPb.set_next_check_log_space_time(stConfig.dwCurrentTime);

	// 上次统计 app 空间后，还剩余有未统计到的 app, 网络丢包会导致此种情况
	if(stPb.tmp_remain_app_count() > 0)
		WARN_LOG("remain app count:%d, may lost packet", stPb.tmp_remain_app_count());

	stPb.set_tmp_remain_app_count(0);
	stPb.set_oldest_log_file_app_id(0);
	stPb.set_oldest_log_file_time(0);
	stPb.set_total_app_log_size(0);

	ScanAppLogSpaceByIdx(stPb, stConfig.psysConfig->wAppInfoCount, stConfig.psysConfig->iAppInfoIndexStart);
	slog.SetUserRemoteAppLogInfoPb(stPb);
	DEBUG_LOG("app log info:%s", stPb.ShortDebugString().c_str());

	// 发起 app 查询日志文件占用磁盘空间大小请求
	if(stConfig.stMapLogSizeReqInfo.size() > 0)
	{
		std::map<TGetAppLogSizeKey, top::SlogGetAppLogSizeReq*, ReqAppLogSizeCmp>::iterator it;
		for(it = stConfig.stMapLogSizeReqInfo.begin(); it != stConfig.stMapLogSizeReqInfo.end(); it++)
		{
			stConfig.pstSock->SendGetAppLogSizeReq(it->first, it->second);
		}
		ClearMapLogSizeReq();
	}
}

int main(int argc, char *argv[])
{
	int iRet = 0;
	if((iRet=Init(NULL)) < 0)
	{
		ERR_LOG("Init Failed ret:%d !", iRet);
		return SLOG_ERROR_LINE;
	}

	INFO_LOG("slog server start !");
	slog.Daemon(1, 1, 0);

	SocketHandler h(&slog);
	CUdpSock stSock(h);

	const char *pszLocalIp = stConfig.szListenIp;
	if('\0' == pszLocalIp[0])
		pszLocalIp = stConfig.szLocalIp;
	Ipv4Address addr(std::string(pszLocalIp), stConfig.iRecvPortMonitor+slog.m_iProcessId);
	if(stSock.Bind(addr, 0) < 0)
	{
		FATAL_LOG("bind port:%d failed", stConfig.iRecvPortMonitor+slog.m_iProcessId);
		MtReport_Attr_Add(72, 1);
		return SLOG_ERROR_LINE;
	}
	h.Add(&stSock);
	stConfig.pstSock = &stSock;
	INFO_LOG("bind address at %s:%d ok ", pszLocalIp, stConfig.iRecvPortMonitor+slog.m_iProcessId);

	while(h.GetCount() && slog.TryRun())
	{
		if(slog.IsExitSet()) {
			stSock.SetCloseAndDelete();
			h.Select(0, 1000);
			continue;
		}

		stConfig.dwCurrentTime = slog.m_stNow.tv_sec;
		ScanAppLogSpace();
		h.Select(1, 100);
	}
	INFO_LOG("slog_server exit !");
	return 0;
}


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

   模块 slog_mtreport_server 功能:
        管理 agent slog_mtreport_client 的接入下发监控系统配置

****/

#include <errno.h>
#include "mtreport_server.h"
#include "top_include_comm.h"
#include "udp_sock.h"
#include "pid_guard.h"

CONFIG stConfig;
CSupperLog slog;


bool TryUseDbHost(const char *pdb_ip)
{
	if(stConfig.qu) {
		if(stConfig.qu->Connected())
			return true;

		delete stConfig.qu;
		stConfig.qu = NULL;
	}

	if(stConfig.db) 
	{
		stConfig.qu = new Query(*stConfig.db);
		if(false == stConfig.qu->Connected())
		{
			delete stConfig.db;
			stConfig.db = NULL;
			delete stConfig.qu;
			stConfig.qu = NULL;
		}
		else {
			return true;
		}
	}

	if(NULL == stConfig.db)
	{
		stConfig.db = new Database(pdb_ip,
			stConfig.pShmConfig->stSysCfg.szUserName,
			stConfig.pShmConfig->stSysCfg.szPass, 
			stConfig.pShmConfig->stSysCfg.szDbName,
			&slog);

		stConfig.qu = new Query(*stConfig.db);
		if(false == stConfig.qu->Connected())
		{
			delete stConfig.db;
			stConfig.db = NULL;
			delete stConfig.qu;
			stConfig.qu = NULL;
		}
		else {
			return true;
		}
	}
	return false;
}

int GetDatabaseServer()
{
	static uint32_t s_dwLastCheckDbTime = 0;

	// 5 - 10 秒 check 一次
	if(stConfig.dwCurrentTime <= s_dwLastCheckDbTime)
		return 0;
	s_dwLastCheckDbTime = stConfig.dwCurrentTime+5+slog.m_iRand%5;

	if(stConfig.qu != NULL && stConfig.qu->Connected())
		return 0;

	if(TryUseDbHost(stConfig.pShmConfig->stSysCfg.szDbHost))
		return 0;
	return SLOG_ERROR_LINE;
}

int Init(const char *pFile = NULL)
{
	const char *pConfFile = NULL;
	if(pFile != NULL)
		pConfFile = pFile;
	else
		pConfFile = CONFIG_FILE;

	int32_t iRet = 0;
	if((iRet=LoadConfig(pConfFile,
		"SERVER_PORT", CFG_INT, &stConfig.iRecvPort, 27000, 
		"LOCAL_IF_NAME", CFG_STRING, stConfig.szLocalIp, "eth0", MYSIZEOF(stConfig.szLocalIp),
		"LISTEN_IP", CFG_STRING, stConfig.szListenIp, "0.0.0.0", MYSIZEOF(stConfig.szListenIp),
		"TIMER_HASH_SHM_KEY", CFG_INT, &stConfig.iTimerHashKey, 2015031347,
		(void*)NULL)) < 0)
	{   
		FATAL_LOG("LoadConfig:%s failed ! ret:%d", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	} 

	if((iRet=slog.InitConfigByFile(pConfFile)) < 0 || (iRet=slog.Init(stConfig.szLocalIp)) < 0)
	{ 
		FATAL_LOG("slog init failed file:%s ret:%d", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	}

	if(stConfig.szListenIp[0] == '\0')
		strcpy(stConfig.szListenIp, stConfig.szLocalIp);

	stConfig.pShmConfig = slog.GetSlogConfig();
	if(stConfig.pShmConfig == NULL) {
		FATAL_LOG("get pShmConfig failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitMtClientInfo() < 0) 
		return SLOG_ERROR_LINE; 

	if(slog.InitMachineList() < 0)
		return SLOG_ERROR_LINE;

	stConfig.psysConfig = slog.GetSystemCfg();
	if(stConfig.psysConfig == NULL) {
		ERR_LOG("GetSystemCfg failed");
		return SLOG_ERROR_LINE;
	}
	stConfig.dwPkgSeq = slog.m_iRand;
	return 0;
}

int main(int argc, char *argv[])
{
	int iRet = 0;
	if(slog.IsShowVer(argc, argv))
		return 0;
	if((iRet=Init(NULL)) < 0)
	{
		ERR_LOG("Init Failed ret:%d !", iRet);
		return SLOG_ERROR_LINE;
	}

	slog.Daemon(1, 1, 1);
	INFO_LOG("slog_mtreport_server start !");

	SocketHandler h(&slog);
	CUdpSock stSock(h);
	Ipv4Address addr(std::string(stConfig.szListenIp), stConfig.iRecvPort);
	if(stSock.Bind(addr, 0) < 0)
	{
		FATAL_LOG("bind port:%d failed", stConfig.iRecvPort);
		return SLOG_ERROR_LINE;
	}
	h.Add(&stSock);

	while(h.GetCount() && slog.TryRun())
	{
		if(slog.IsExitSet())
		{
			stSock.SetCloseAndDelete();
			h.Select(0, 1000);
			continue;
		}
	
		stConfig.dwCurrentTime = slog.m_stNow.tv_sec;
		if((iRet=GetDatabaseServer()) < 0)
			break;
		h.Select(1, slog.m_iRand%SEC_USEC);
	}
	INFO_LOG("slog_mtreport_server exit !");
	return 0;
}


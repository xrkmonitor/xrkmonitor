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
#include "comm.pb.h"

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
		"WRITE_REANINFO_TO_DB_PER_TIME", CFG_INT, &stConfig.iRealinfoToDbPerTime, 30,
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

	stConfig.pCenterServer = slog.GetValidServerByType(SRV_TYPE_MT_CENTER);
	if(stConfig.pCenterServer == NULL) {
		ERR_LOG("Get center server failed !");
		return SLOG_ERROR_LINE;
	}
	if(slog.IsIpMatchLocalMachine(stConfig.pCenterServer->dwIp))
		stConfig.bSelfIsCenterServer = true;
	else
		stConfig.bSelfIsCenterServer = false;
	DEBUG_LOG("get center server seq:%u, addr:%s:%d, is self:%d", stConfig.pCenterServer->dwCfgSeq, 
		stConfig.pCenterServer->szIpV4, stConfig.pCenterServer->wPort, stConfig.bSelfIsCenterServer);
	return 0;
}

void ReadRealInfoFromDb()
{
	snprintf(stConfig.szSql, MYSIZEOF(stConfig.szSql), 
		"select other_info from flogin_user where xrk_status=0 and user_id=1");
	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();
	if(!qu.get_result(stConfig.szSql)) {
		ERR_LOG("execute sql:%s failed !", stConfig.szSql);
		return ;
	}
	if(qu.num_rows() > 0 && qu.fetch_row() != NULL)
	{
		qu.fetch_lengths();
		unsigned long* lengths = qu.fetch_lengths();
		const char *pval = qu.getstr("other_info");
		::comm::SysconfigInfo stInfo;
		if(lengths[0] <= 0) {
			WARN_LOG("get other info length 0");
			stConfig.psysConfig->stRealInfoShm.dwTotalAccTimes = 0;
			stConfig.psysConfig->stRealInfoShm.dwTodayAccTimes = 0;
			qu.free_result();
			return;
		}
		if(!stInfo.ParseFromArray(pval, lengths[0]))
		{
			WARN_LOG("ParseFromArray failed-%p-%lu", pval, lengths[0]);
			qu.free_result();
			return;
		}
		stConfig.psysConfig->stRealInfoShm.dwTotalAccTimes = stInfo.total_access_times();
		stConfig.psysConfig->stRealInfoShm.dwTodayAccTimes = stInfo.today_access_times();

		INFO_LOG("get real info from db- acc total:%u, acc today:%u",
			stInfo.total_access_times(), stInfo.today_access_times());
	}
	qu.free_result();
}

void TryUpdateRealinfo() 
{
	static uint32_t s_dwLastRealInfoSeq = 0;

	if(slog.m_stNow.tv_sec < stConfig.dwLastWriteRealinfoToDbTime+stConfig.iRealinfoToDbPerTime)
		return;
	stConfig.dwLastWriteRealinfoToDbTime = slog.m_stNow.tv_sec;

	SLogServer *pCenterServer = slog.GetValidServerByType(SRV_TYPE_MT_CENTER);
	if(pCenterServer != stConfig.pCenterServer) {
		stConfig.pCenterServer = pCenterServer;
		if(slog.IsIpMatchLocalMachine(stConfig.pCenterServer->dwIp))
			stConfig.bSelfIsCenterServer = true;
		else 
			stConfig.bSelfIsCenterServer = false;
		INFO_LOG("web server change new - seq:%u, addr:%s:%d, is self:%d", stConfig.pCenterServer->dwCfgSeq, 
			stConfig.pCenterServer->szIpV4, stConfig.pCenterServer->wPort, stConfig.bSelfIsCenterServer);
	}
	if(!stConfig.bSelfIsCenterServer)
		return;

	struct tm stTm;
	localtime_r(&slog.m_stNow.tv_sec, &stTm);
	if(stTm.tm_mday != stConfig.psysConfig->stRealInfoShm.bNewAccStartDay
		|| slog.m_stNow.tv_sec > stConfig.psysConfig->stRealInfoShm.dwNewAccStartTime+24*60*60)
	{
		if(slog.InitChangeRealInfoShm() >= 0) {
			stConfig.psysConfig->stRealInfoShm.bNewAccStartDay = stTm.tm_mday;
			stConfig.psysConfig->stRealInfoShm.dwNewAccStartTime = slog.m_stNow.tv_sec;
			stConfig.psysConfig->stRealInfoShm.dwReanInfoSeq++;
			stConfig.psysConfig->stRealInfoShm.dwTotalAccTimes += stConfig.psysConfig->stRealInfoShm.dwTodayAccTimes;
			stConfig.psysConfig->stRealInfoShm.dwTodayAccTimes = 0;
			stConfig.psysConfig->stRealInfoShm.dwTotalAccTimes += stConfig.psysConfig->stRealInfoShm.wNewAccTimes;
			stConfig.psysConfig->stRealInfoShm.wNewAccTimes = 0;
			slog.EndChangeRealInfoShm();
		}
	}

	if(s_dwLastRealInfoSeq == stConfig.psysConfig->stRealInfoShm.dwReanInfoSeq)
		return;

	::comm::SysconfigInfo stInfo;
	if(slog.GetRealInfoPb(stInfo) < 0)
		return;
	std::string strval;
	if(!stInfo.AppendToString(&strval)) {
		ERR_LOG("AppendToString failed !");
		return;
	}

	static char s_szBinSql[1024*10] = {0};
	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();
	int iTmpLen = snprintf(s_szBinSql, sizeof(s_szBinSql), "update flogin_user set other_info=");
	char *pbuf = (char*)s_szBinSql+iTmpLen;
	int32_t iBinaryDataLen = qu.SetBinaryData(pbuf, strval.c_str(), strval.size());
	if(iBinaryDataLen < 0)
		return;
	if(iTmpLen+iBinaryDataLen+256 > (int)sizeof(s_szBinSql)) {
		ERR_LOG("need more space %d < %d", iTmpLen+iBinaryDataLen, (int)sizeof(s_szBinSql));
		return ;
	}
	pbuf += iBinaryDataLen;
	int iSqlLen = (int32_t)(pbuf-s_szBinSql);
	iTmpLen = snprintf(pbuf, sizeof(s_szBinSql)-iSqlLen, " where xrk_status=0 and user_id=1");
	if(iTmpLen < (int)(sizeof(s_szBinSql)-iSqlLen))
		iSqlLen += iTmpLen;
	else
	{
		ERR_LOG("need more space %d < %d", iTmpLen, (int)(sizeof(s_szBinSql)-iSqlLen));
		return ;
	}
	if(qu.ExecuteBinary(s_szBinSql, iSqlLen) < 0)
		return ;

	if(stConfig.psysConfig->stRealInfoShm.wNewAccTimes != 0 && slog.InitChangeRealInfoShm() >= 0) 
	{
		stConfig.psysConfig->stRealInfoShm.dwTotalAccTimes += stConfig.psysConfig->stRealInfoShm.wNewAccTimes;
		stConfig.psysConfig->stRealInfoShm.dwTodayAccTimes += stConfig.psysConfig->stRealInfoShm.wNewAccTimes;
		stConfig.psysConfig->stRealInfoShm.wNewAccTimes = 0;
		slog.EndChangeRealInfoShm();
	}
	INFO_LOG("save realinfo to db, data length:%d, sql length:%d, total:%d, today:%d", 
		iBinaryDataLen, iSqlLen, stInfo.total_access_times(), stInfo.today_access_times());
}

void ClearOldMtClientInfo()
{
    static uint32_t s_dwLastClearTime = 0;
    if(s_dwLastClearTime+6*3600 >= slog.m_stNow.tv_sec)
        return;
    s_dwLastClearTime = slog.m_stNow.tv_sec;

    int idx = stConfig.psysConfig->iMtClientListIndexStart;
    uint16_t wClientCount = 0;
    MtClientInfo *pInfo = NULL, *pPrev = NULL, *pNext = NULL;
    for(int i=0; i < stConfig.psysConfig->wCurClientCount && idx >= 0; ++i) {
        pInfo = slog.GetMtClientInfo(idx);
        if(slog.IsMtClientValid(pInfo)) {
            idx = pInfo->iNextIndex;
            wClientCount++;
        }
        else {
            if(pInfo->iPreIndex >= 0)
                pPrev = slog.GetMtClientInfo(pInfo->iPreIndex);
            else if(pPrev)
                pPrev = NULL;
            if(pInfo->iNextIndex >= 0)
                pNext = slog.GetMtClientInfo(pInfo->iNextIndex);
            else
                pNext = NULL;
            ILINK_DELETE_NODE(stConfig.psysConfig, iMtClientListIndexStart, iMtClientListIndexEnd, pInfo,
                pPrev, pNext, iPreIndex, iNextIndex);
            INFO_LOG("clear mtclient machine id:%d, remote:%s:%d",
                pInfo->iMachineId, ipv4_addr_str(pInfo->dwAddress), pInfo->wBasePort);
            pInfo->iMachineId = 0;
        }
    }
    if(stConfig.psysConfig->wCurClientCount != wClientCount) {
        INFO_LOG("client count changed from:%d to %d", stConfig.psysConfig->wCurClientCount, wClientCount);
		stConfig.psysConfig->wCurClientCount = wClientCount;
    }
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

	stConfig.dwCurrentTime = time(NULL);
	if(stConfig.psysConfig->stRealInfoShm.dwReanInfoSeq == 0) {
		if((iRet=GetDatabaseServer()) < 0) {
			ERR_LOG("GetDatabaseServer failed !");
			return SLOG_ERROR_LINE;
		}
		ReadRealInfoFromDb();
	}

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
		stSock.CheckUdpSess(slog.m_stNow);
		stSock.DealEvent();
		h.Select(1, slog.m_iRand%SEC_USEC);
		TryUpdateRealinfo();
		ClearOldMtClientInfo();
	}
	INFO_LOG("slog_mtreport_server exit !");
	return 0;
}


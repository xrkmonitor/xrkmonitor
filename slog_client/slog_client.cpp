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

   模块 slog_client 功能:
        slog_client 用于将字符云监控系统自身运行时产生的日志发送到日志中心，
		内部使用的协议是易于扩展的 protobuf 协议

****/

#include <errno.h>
#include <map>
#include "top_include_comm.h"
#include "udp_sock.h"
#include "top_proto.pb.h"

#define CONFIG_FILE "./slog_client.conf"
#define MAX_LOG_COUNT_PER_SEND 1000

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
		"LOCAL_HEARTBEAT_TIME_SEC", CFG_INT, &stConfig.iSendHeartTimeSec, 5,
		"LOCAL_MACHINE_ID", CFG_INT, &stConfig.iLocalMachineId, 0,
		"LOCAL_CHECK_LOG_CLIENT_TIME_SEC", CFG_INT, &stConfig.iCheckLogClientTimeSec, 10,
        "QUICK_TO_SLOW_IP", CFG_STRING, stConfig.szQuickToSlowIp, "127.0.0.1", sizeof(stConfig.szQuickToSlowIp),
        "QUICK_TO_SLOW_PORT", CFG_INT, &stConfig.iQuickToSlowPort, 38081,
		"IS_XRKMONITOR_LOG_SERVER", CFG_INT, &stConfig.iSelfIsLogServer, 0,
		(void*)NULL)) < 0)
	{   
		ERR_LOG("LoadConfig:%s failed ! ret:%d", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	} 

	if((iRet=slog.InitConfigByFile(pConfFile)) < 0)
	{ 
		ERR_LOG("slog InitConfigByFile failed file:%s ret:%d\n", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	}

 	if((iRet=slog.Init()) < 0)
	{ 
		ERR_LOG("slog init failed ret:%d\n", iRet);
		return SLOG_ERROR_LINE;
	}

	if(slog.InitMachineList() < 0)
	{
		FATAL_LOG("InitMachineList failed !");
		return SLOG_ERROR_LINE;
	}

	SLogConfig *pShmConfig = slog.GetSlogConfig();
	if(pShmConfig != NULL && stConfig.iLocalMachineId==0)
	{
		stConfig.iLocalMachineId = pShmConfig->stSysCfg.iMachineId;
		DEBUG_LOG("get local machine id:%d", stConfig.iLocalMachineId);
	}

	if(stConfig.iLocalMachineId != 0) 
	{
		stConfig.pLocalMachineInfo = slog.GetMachineInfo(stConfig.iLocalMachineId, NULL);
		if(stConfig.pLocalMachineInfo != NULL)
			DEBUG_LOG("get local machine:%d, %s", stConfig.pLocalMachineInfo->id, ipv4_addr_str(stConfig.pLocalMachineInfo->ip1));
		else 
			WARN_LOG("get local machine failed, machine id:%d", stConfig.iLocalMachineId);
	}
	else	
	{
		stConfig.pLocalMachineInfo = slog.GetMachineInfoByIp((char*)(slog.GetLocalIP()));
		if(stConfig.pLocalMachineInfo != NULL)
		{
			stConfig.iLocalMachineId = stConfig.pLocalMachineInfo->id;
			DEBUG_LOG("get local machine by ip:%s -- machine:%d, %s", 
				slog.GetLocalIP(), stConfig.pLocalMachineInfo->id, ipv4_addr_str(stConfig.pLocalMachineInfo->ip1));
		}
	}

	if(stConfig.iLocalMachineId == 0 || stConfig.pLocalMachineInfo==NULL)
	{
		ERR_LOG("local machine not set or get failed, machine:%d", stConfig.iLocalMachineId);
		return SLOG_ERROR_LINE;
	}
	
	stConfig.pSelfApp = slog.GetAppInfoSelf();
	if(stConfig.pSelfApp == NULL)
	{
		ERR_LOG("get self app failed !");
		return SLOG_ERROR_LINE;
	}

	stConfig.pAppShmInfoList = slog.GetAppInfo();
	if(stConfig.pAppShmInfoList == NULL)
	{
		ERR_LOG("get app shm info list failed");
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int SendHeartInfo(CUdpSock &sock, std::string & strHead, std::string & strBody, AppInfo * pSelfApp)
{
	static char sBuf[1024];

	if(strHead.size()+strBody.size()+10 > 1024)
	{
		REQERR_LOG("packet too long over (%u > 1024)", (unsigned)(strHead.size()+strBody.size()+10));
		MtReport_Attr_Add(68, 1);
		return SLOG_ERROR_LINE;
	}

	char *pdata = sBuf, *pstart = sBuf;
	bool bIsMalloc = false;
	if(strHead.size()+strBody.size()+10 > MYSIZEOF(sBuf)) // 10 = stx+4(headlen)+4(bodylen)+etx
	{
		pdata = pstart = (char*)malloc(strHead.size()+strBody.size()+10);
		if(pstart == NULL)
		{
			ERR_LOG("malloc failed ! size:%u", (unsigned)(strHead.size()+strBody.size()+10));
			MtReport_Attr_Add(69, 1);
			return SLOG_ERROR_LINE;
		}
		bIsMalloc = true;
	}
	*pdata = SPKG_PB;
	pdata++;

	int32_t iHeadLen = strHead.size();
	*(int32_t*)pdata = htonl(iHeadLen);
	pdata += 4;
	memcpy(pdata, strHead.c_str(), strHead.size());
	pdata += strHead.size();

	int32_t iBodyLen = strBody.size();
	*(int32_t*)pdata = htonl(iBodyLen);
	pdata += 4;
	memcpy(pdata, strBody.c_str(), strBody.size());
	pdata += strBody.size();

	*pdata = EPKG_PB;
	pdata++;

	int32_t iPackLen = pdata-pstart;

	// 服务器在本机的，转为回环地址: 0x100007f (127.0.0.1)
	uint32_t dwAppSrvMaster = pSelfApp->dwAppSrvMaster;
	if(slog.IsIpMatchMachine(pSelfApp->dwAppSrvMaster, stConfig.iLocalMachineId) == 1)
		dwAppSrvMaster = 0x100007f;
	Ipv4Address addr(dwAppSrvMaster, pSelfApp->wLogSrvPort);

	sock.SendToBuf(addr, pstart, iPackLen, 0);
	if(bIsMalloc)
		free(pstart);
	DEBUG_LOG("send heartbeat packet len:%d head len:%d body len:%d to:%s, appid:%d",
		iPackLen, iHeadLen, iBodyLen, addr.Convert(true).c_str(), pSelfApp->iAppId);
	return iPackLen; 
}

static int SendLogInfo(
	CUdpSock &sock, std::string & strHead, std::string & strBody, AppInfo * pSelfApp)
{
	static char sBuf[8192];

	if(strHead.size()+strBody.size()+10 > 1024*1024)
	{
		REQERR_LOG("packet too long over (%u > 1024*1024)", (unsigned)(strHead.size()+strBody.size()+10));
		MtReport_Attr_Add(68, 1);
		return SLOG_ERROR_LINE;
	}

	char *pdata = sBuf, *pstart = sBuf;
	bool bIsMalloc = false;
	if(strHead.size()+strBody.size()+10 > MYSIZEOF(sBuf)) // 10 = stx+4(headlen)+4(bodylen)+etx
	{
		pdata = pstart = (char*)malloc(strHead.size()+strBody.size()+10);
		if(pstart == NULL)
		{
			ERR_LOG("malloc failed ! size:%u", (unsigned)(strHead.size()+strBody.size()+10));
			MtReport_Attr_Add(69, 1);
			return SLOG_ERROR_LINE;
		}
		bIsMalloc = true;
	}
	*pdata = SPKG_PB;
	pdata++;

	int32_t iHeadLen = strHead.size();
	*(int32_t*)pdata = htonl(iHeadLen);
	pdata += 4;
	memcpy(pdata, strHead.c_str(), strHead.size());
	pdata += strHead.size();

	int32_t iBodyLen = strBody.size();
	*(int32_t*)pdata = htonl(iBodyLen);
	pdata += 4;
	memcpy(pdata, strBody.c_str(), strBody.size());
	pdata += strBody.size();

	*pdata = EPKG_PB;
	pdata++;

	int32_t iPackLen = pdata-pstart;
	Ipv4Address addr(pSelfApp->dwAppSrvMaster, pSelfApp->wLogSrvPort);
	sock.SendLog(addr, pstart, iPackLen);
	if(bIsMalloc)
		free(pstart);
	
	DEBUG_LOG("send log packet len:%d head len:%d body len:%d to:%s",
		iPackLen, iHeadLen, iBodyLen, addr.Convert(true).c_str());
	return iPackLen; 
}

void SendHeart(CUdpSock &sock, TLogClientInfo & clt, uint32_t dwSrvIp)
{
	static uint32_t s_dwSeq = rand();

	TMapHeartToServerInfoIt it = stConfig.stMapHeartInfo.find(dwSrvIp);
	if(it != stConfig.stMapHeartInfo.end() 
	    && it->second+stConfig.iSendHeartTimeSec >= stConfig.dwCurrentTime)
		return;
	if(it == stConfig.stMapHeartInfo.end()) {
		stConfig.stMapHeartInfo[dwSrvIp] = stConfig.dwCurrentTime;
	}
	else {
		it->second = stConfig.dwCurrentTime;
	}

	::comm::PkgHead head;
	head.set_en_cmd(::comm::CMD_SLOG_CLIENT_HEART);
	head.set_uint32_seq(s_dwSeq);

	::comm::HeartInfo info;
	info.set_bytes_req_ip(slog.GetLocalIP());

	std::string strHead, strBody;
	if(!head.AppendToString(&strHead) || !info.AppendToString(&strBody))
	{
		ERR_LOG("protobuf AppendToString failed msg:%s", strerror(errno));
		return;
	}
	if(SendHeartInfo(sock, strHead, strBody, clt.pAppInfo) > 0)
		MtReport_Attr_Add(93, 1);
}

int SendSlog(CUdpSock &sock, TLogClientInfo & stClientInfo)
{
	static uint32_t s_dwSeq = rand();

	::top::SlogClientPkgBody log;
	log.set_uint32_app_id(stClientInfo.pAppInfo->iAppId);

	::top::SlogLogInfo *pstLogInfo = NULL;
	TSLogOut *pstLog = NULL;
	int32_t iLogLen = 0, j = 0, iRet = 0, iSelfCount = 0;
	uint8_t bCustFlag = 0;
	for(j=0; j < MAX_LOG_COUNT_PER_SEND && iLogLen < 1200; j++)
	{
		pstLog = stClientInfo.pstLogClient->GetLog();
		if(pstLog != NULL)
		{
			pstLogInfo = log.add_log();
			pstLogInfo->set_uint32_app_id(pstLog->iAppId);
			pstLogInfo->set_uint32_module_id(pstLog->iModuleId);
			pstLogInfo->set_uint64_log_time(pstLog->qwLogTime);
			pstLogInfo->set_uint32_log_seq(pstLog->dwLogSeq);
			pstLogInfo->set_uint32_log_type(pstLog->wLogType);
			pstLogInfo->set_uint32_log_config_id(pstLog->dwLogConfigId);
			pstLogInfo->set_uint32_log_host(pstLog->dwLogHost);
			iLogLen += 32;

			// 统计本身产生的日志数目
			if(pstLog->iAppId == slog.m_iLogAppId && pstLog->iModuleId == slog.m_iLogModuleId)
				iSelfCount++;

			pstLogInfo->set_bytes_log(pstLog->pszLog);
			iLogLen += pstLogInfo->bytes_log().size(); 

			bCustFlag = pstLog->bCustFlag;
			if(bCustFlag & MTLOG_CUST_FLAG_C1_SET) {
				pstLogInfo->set_uint32_cust_1(pstLog->dwCust_1);
				iLogLen += 4;
			}
			if(bCustFlag & MTLOG_CUST_FLAG_C2_SET) {
				pstLogInfo->set_uint32_cust_2(pstLog->dwCust_2);
				iLogLen += 4;
			}
			if(bCustFlag & MTLOG_CUST_FLAG_C3_SET) {
				pstLogInfo->set_int32_cust_3(pstLog->iCust_3);
				iLogLen += 4;
			}
			if(bCustFlag & MTLOG_CUST_FLAG_C4_SET) {
				pstLogInfo->set_int32_cust_4(pstLog->iCust_4);
				iLogLen += 4;
			}
			if((bCustFlag & MTLOG_CUST_FLAG_C5_SET) && pstLog->szCust_5[0] != '\0') {
				pstLogInfo->set_bytes_cust_5(pstLog->szCust_5);
				iLogLen += pstLogInfo->bytes_cust_5().size();
			}
			if((bCustFlag & MTLOG_CUST_FLAG_C6_SET) && pstLog->szCust_6[0] != '\0') {
				pstLogInfo->set_bytes_cust_6(pstLog->szCust_6);
				iLogLen += pstLogInfo->bytes_cust_6().size();
			}
			pstLogInfo->set_cust_flag(bCustFlag);
			iLogLen += 4;
		}
		else
			break;
	}

	// 全部是 slog_client 本身产生的日志则丢弃
	if(j <= 0 || iSelfCount >= j)
		return 0;

	::comm::PkgHead head;
	head.set_en_cmd(::comm::SLOG_CLIENT_SEND_LOG);
	head.set_uint32_seq(s_dwSeq);

	std::string strHead, strBody;
	if(!head.AppendToString(&strHead) || !log.AppendToString(&strBody))
	{
		ERR_LOG("protobuf AppendToString failed msg:%s, log count:%d, appid:%d",
				strerror(errno), j, stClientInfo.pAppInfo->iAppId);
		return SLOG_ERROR_LINE;
	}

	if((iRet=SendLogInfo(sock, strHead, strBody, stClientInfo.pAppInfo)) > 0)
	{
		stConfig.dwSendLogCount += j;
		DEBUG_LOG("send log packet log count:%d, appid:%d, pack seq:%u, packet len:%d", 
				j, stClientInfo.pAppInfo->iAppId, s_dwSeq, iRet);
		s_dwSeq++;
		MtReport_Attr_Add(62, j);
	}
	else
	{
		ERR_LOG("send log for appid:%d failed ret:%d, log count:%d", stClientInfo.pAppInfo->iAppId, iRet, j);
		MtReport_Attr_Add(61, 1);
		return SLOG_ERROR_LINE;
	}
	return j;
}

void SetLogClient()
{
	static time_t s_lastCheckTime = 0;
	assert(stConfig.pAppShmInfoList != NULL);
	if(stConfig.dwCurrentTime < s_lastCheckTime+stConfig.iCheckLogClientTimeSec)
		return;
	s_lastCheckTime = stConfig.dwCurrentTime;

	for(int i=0,j=0; i < stConfig.pAppShmInfoList->iAppCount && j < MAX_SLOG_APP_COUNT; j++)
	{
		if(0 == stConfig.pAppShmInfoList->stInfo[j].iAppId)
			continue;
		i++;

		if(!(IS_SET_BIT(stConfig.pAppShmInfoList->stInfo[j].dwAppLogFlag, APPLOG_FLAG_LOG_WRITED)))
			continue;
		if(stConfig.mapLogClient.find(stConfig.pAppShmInfoList->stInfo[j].iAppId)!=stConfig.mapLogClient.end())
			continue;

		TLogClientInfo clt;
		clt.pAppInfo = slog.GetAppInfo((int32_t)(stConfig.pAppShmInfoList->stInfo[j].iAppId));
		if(NULL == clt.pAppInfo)
		{
			WARN_LOG("get appinfo failed, appid:%d", stConfig.pAppShmInfoList->stInfo[j].iAppId);
			continue;
		}

		TSLogShm *plog_shm = slog.GetAppLogShm(clt.pAppInfo, false);
		if(NULL == plog_shm)
		{
			WARN_LOG("GetAppLogShm failed, appid:%d", stConfig.pAppShmInfoList->stInfo[j].iAppId);
			continue;
		}

		clt.pstLogClient = new CSLogClient(plog_shm);
		if(false == clt.pstLogClient->IsInit())
		{
			ERR_LOG("create CSLogClient or init failed, appid:%d", stConfig.pAppShmInfoList->stInfo[j].iAppId);
			delete clt.pstLogClient;
			continue;
		}
		stConfig.mapLogClient[stConfig.pAppShmInfoList->stInfo[j].iAppId] = clt;
		INFO_LOG("add logclient for app:%d", stConfig.pAppShmInfoList->stInfo[j].iAppId);
	}
}

int main(int argc, char *argv[])
{
	int iRet = 0;
	stConfig.dwCurrentTime = time(NULL);
	stConfig.dwSendLogCount = 0;

	if((iRet=Init(NULL)) < 0)
	{
		ERR_LOG("Init Failed ret:%d !", iRet);
		return SLOG_ERROR_LINE;
	}

	MachineInfo *pLocalMachineInfo = stConfig.pLocalMachineInfo;
	std::string strLocal("");
	if(pLocalMachineInfo->ip1 != 0) {
		strLocal += ipv4_addr_str(pLocalMachineInfo->ip1); 
		strLocal += "|";
	}
	if(pLocalMachineInfo->ip2 != 0) {
		strLocal += ipv4_addr_str(pLocalMachineInfo->ip2); 
		strLocal += "|";
	}
	if(pLocalMachineInfo->ip3 != 0) {
		strLocal += ipv4_addr_str(pLocalMachineInfo->ip3); 
		strLocal += "|";
	}
	if(pLocalMachineInfo->ip4 != 0) {
		strLocal += ipv4_addr_str(pLocalMachineInfo->ip4); 
		strLocal += "|";
	}
	INFO_LOG("get local machine info ip:[%s], machine id:%d", strLocal.c_str(), stConfig.iLocalMachineId);

	slog.Daemon(1, 1);

	// 单进程
	assert(1 == slog.m_iProcessCount);
	INFO_LOG("slog_client start !");

	SocketHandler h(&slog);
	CUdpSock stSock(h);
	h.Add(&stSock);
	while(h.GetCount() && slog.Run())
	{
		h.Select(1, 10000);
		slog.CheckTest(NULL);
		stConfig.dwCurrentTime = time(NULL);
		SetLogClient();

		TMapLogClientInfoIt it = stConfig.mapLogClient.begin();
		int iSendCount = 0;
		for(; it != stConfig.mapLogClient.end(); it++)
		{
			if((iRet=slog.CheckAppLogServer(it->second.pAppInfo)) < 0)
			{
				ERR_LOG("CheckAppLogServer failed, ret:%d, appid:%d", iRet, it->first);
				MtReport_Attr_Add(92, 1);
				continue;
			}

			// 与 app slog_server 同机部署的不用发送 slog 日志
			if(stConfig.iSelfIsLogServer
				|| it->second.pAppInfo->dwAppSrvMaster == pLocalMachineInfo->ip1
				|| it->second.pAppInfo->dwAppSrvMaster == pLocalMachineInfo->ip2 
				|| it->second.pAppInfo->dwAppSrvMaster == pLocalMachineInfo->ip3 
				|| it->second.pAppInfo->dwAppSrvMaster == pLocalMachineInfo->ip4) 
			{
				DEBUG_LOG("slog client and server is same, local(%u:%u:%u:%u), log server:%s|%u appid:%d", 
					pLocalMachineInfo->ip1, pLocalMachineInfo->ip2, pLocalMachineInfo->ip3,
					pLocalMachineInfo->ip4, ipv4_addr_str(it->second.pAppInfo->dwAppSrvMaster),
					it->second.pAppInfo->dwAppSrvMaster, it->first);
				pLocalMachineInfo->dwLastReportLogTime = slog.m_stNow.tv_sec;
				stSock.ReportQuickToSlowMsg(pLocalMachineInfo->id);
				//SendHeart(stSock, it->second, it->second.pAppInfo->dwAppSrvMaster);
				continue;
			}

			do{
				iRet = SendSlog(stSock, it->second);
				if(iRet > 0)
					iSendCount += iRet;
			}while(iRet >= MAX_LOG_COUNT_PER_SEND);
		}
	}
	INFO_LOG("slog_client exit !");
	return 0;
}


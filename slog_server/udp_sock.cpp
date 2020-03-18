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

#include <supper_log.h>
#include <errno.h>
#include <sv_struct.h>
#include <mt_report.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include "aes.h"
#include "slog_server.h"
#include "udp_sock.h"
#include "comm.pb.h"

using namespace comm;

CUdpSock::CUdpSock(ISocketHandler&h): UdpSocket(h), CBasicPacket() 
{
	Attach(CreateSocket(PF_INET, SOCK_DGRAM, "udp"));
	m_iPkgLen = 0;
	m_pcltMachine = NULL;
	m_dwReqSeqLocal = 1;
}

CUdpSock::~CUdpSock()
{
}

int32_t CUdpSock::SendResponsePacket(const char*pkg, int len) 
{
	SendToBuf(m_addrRemote, pkg, len, 0);
	DEBUG_LOG("send response packet to client:%s pkglen:%d", m_addrRemote.Convert(true).c_str(), len);
	return 0;
}

void CUdpSock::DealGetAppLogSizeRsp(top::SlogGetAppLogSizeRsp &rspinfo)
{
	if(rspinfo.req_app_count() != rspinfo.app_log_size_info_size())
	{
		WARN_LOG("app log size req/rsp not match, %d != %d", 
			rspinfo.req_app_count(), rspinfo.app_log_size_info_size());
	}

	user::UserRemoteAppLogInfo stPb;
	slog.GetUserRemoteAppLogInfoPb(stPb);

	int iTmpRemainAppCount = stPb.tmp_remain_app_count() - rspinfo.req_app_count();
	uint32_t qwTmpAppLogSize = stPb.total_app_log_size();
	uint32_t dwOldestLogFileTime = stPb.oldest_log_file_time();
	int32_t iOldestLogFileAppId = stPb.oldest_log_file_app_id();
	AppInfo * pAppInfo = NULL; 
	for(int i=0; i < rspinfo.app_log_size_info_size(); i++)
	{
		if(rspinfo.app_log_size_info(i).log_size() <= 0
			|| rspinfo.app_log_size_info(i).oldest_log_file_time() <= 0)
		{
			// app 没有日志上报会走到这里
			continue;
		}

		pAppInfo = slog.GetAppInfo(rspinfo.app_log_size_info(i).appid());
		if(pAppInfo == NULL) {
			WARN_LOG("get appinfo :%d failed", rspinfo.app_log_size_info(i).appid());
		}
		else 
		{
			qwTmpAppLogSize += rspinfo.app_log_size_info(i).log_size();
			if(dwOldestLogFileTime == 0
				|| dwOldestLogFileTime > rspinfo.app_log_size_info(i).oldest_log_file_time())
			{
				dwOldestLogFileTime = rspinfo.app_log_size_info(i).oldest_log_file_time();
				iOldestLogFileAppId = rspinfo.app_log_size_info(i).appid();
			}

			pAppInfo->stLogStatInfo.qwLogSizeInfo = rspinfo.app_log_size_info(i).log_size();
			pAppInfo->stLogStatInfo.dwDebugLogsCount = rspinfo.app_log_size_info(i).debug_logs_count();
			pAppInfo->stLogStatInfo.dwInfoLogsCount = rspinfo.app_log_size_info(i).info_logs_count();
			pAppInfo->stLogStatInfo.dwWarnLogsCount = rspinfo.app_log_size_info(i).warn_logs_count();
			pAppInfo->stLogStatInfo.dwReqerrLogsCount = rspinfo.app_log_size_info(i).reqerr_logs_count();
			pAppInfo->stLogStatInfo.dwErrorLogsCount = rspinfo.app_log_size_info(i).error_logs_count();
			pAppInfo->stLogStatInfo.dwFatalLogsCount = rspinfo.app_log_size_info(i).fatal_logs_count();
			pAppInfo->stLogStatInfo.dwOtherLogsCount = rspinfo.app_log_size_info(i).other_logs_count();
			pAppInfo->bReadLogStatInfo = true;
		}
	}

	stPb.set_tmp_remain_app_count(iTmpRemainAppCount);
	stPb.set_total_app_log_size(qwTmpAppLogSize);
	stPb.set_oldest_log_file_app_id(iOldestLogFileAppId);
	stPb.set_oldest_log_file_time(dwOldestLogFileTime);
	slog.SetUserRemoteAppLogInfoPb(stPb);

	if(iTmpRemainAppCount <= 0) {
		INFO_LOG("app log space:%u", (uint32_t)(qwTmpAppLogSize/(1024*1024)));
	}
	DEBUG_LOG("set app log info:%s", stPb.ShortDebugString().c_str());
}

void CUdpSock::GetAppLogSize(AppInfo *pAppInfo, top::SlogGetAppLogSizeRsp & rsp)
{
	SLogFile *pLogFile = NULL;
	std::map<int , SLogFile*>::iterator it = stConfig.mapAppLogFileShm.find(pAppInfo->iAppId);
	if(it != stConfig.mapAppLogFileShm.end())
	{
		pLogFile =  it->second;
	}
	else {
		pLogFile = CSLogServerWriteFile::GetAppLogFileShm(pAppInfo);
		if(NULL == pLogFile)
		{
			WARN_LOG("get app logfile failed, appid:%d", pAppInfo->iAppId);
			return;
		}
		stConfig.mapAppLogFileShm[pAppInfo->iAppId] = pLogFile;
	}

	top::AppLogSizeInfo *pAppLogSizeInfo = rsp.add_app_log_size_info();
	pAppLogSizeInfo->set_appid(pAppInfo->iAppId);
	pAppLogSizeInfo->set_log_size(pAppInfo->stLogStatInfo.qwLogSizeInfo);
	pAppLogSizeInfo->set_debug_logs_count(pAppInfo->stLogStatInfo.dwDebugLogsCount);
	pAppLogSizeInfo->set_info_logs_count(pAppInfo->stLogStatInfo.dwInfoLogsCount);
	pAppLogSizeInfo->set_warn_logs_count(pAppInfo->stLogStatInfo.dwWarnLogsCount);
	pAppLogSizeInfo->set_reqerr_logs_count(pAppInfo->stLogStatInfo.dwReqerrLogsCount);
	pAppLogSizeInfo->set_error_logs_count(pAppInfo->stLogStatInfo.dwErrorLogsCount);
	pAppLogSizeInfo->set_fatal_logs_count(pAppInfo->stLogStatInfo.dwFatalLogsCount);
	pAppLogSizeInfo->set_other_logs_count(pAppInfo->stLogStatInfo.dwOtherLogsCount);
	if(pLogFile->wLogFileCount > 0)
		pAppLogSizeInfo->set_oldest_log_file_time((uint32_t)(pLogFile->stFiles[0].qwLogTimeStart/1000000));
}

void CUdpSock::DealGetAppLogSizeReq(top::SlogGetAppLogSizeReq &reqinfo)
{
	top::SlogGetAppLogSizeRsp rsp;
	rsp.set_req_app_count(reqinfo.appid_list_size());

	AppInfo *pAppInfo = NULL;
	for(int i=0; i < reqinfo.appid_list_size(); i++)
	{
		pAppInfo = slog.GetAppInfo(reqinfo.appid_list(i));
		if(NULL == pAppInfo)
		{
			ERR_LOG("get appinfo failed, appid:%d", reqinfo.appid_list(i));
			continue;
		}

		// 检查分发是否正确
		if(slog.IsIpMatchMachine(stConfig.pLocalMachineInfo, pAppInfo->dwAppSrvMaster) != 1)
		{
			ERR_LOG("app log dispatch invalid, appsvrip:%s, appid:%d",
				ipv4_addr_str(pAppInfo->dwAppSrvMaster), pAppInfo->iAppId);
			MtReport_Attr_Add(260, 1);
			AckToReq(ERR_APP_LOG_DISPATCH_INVALID);
			return;
		}
		GetAppLogSize(pAppInfo, rsp);
	}
	DEBUG_LOG("response get app log size info:%s", rsp.ShortDebugString().c_str());
	SendGetAppLogSizeRsp(rsp);
}

int32_t CUdpSock::CheckSignature()
{
	MonitorCommSig *pcommSig = NULL;
	size_t iDecSigLen = 0;

	switch(m_pstSig->bSigType) {
		case MT_SIGNATURE_TYPE_COMMON:
			aes_decipher_data((const uint8_t*)m_pstSig->sSigValue, ntohs(m_pstSig->wSigLen),
			    (uint8_t*)m_sDecryptBuf, &iDecSigLen, 
				(const uint8_t*)stConfig.psysConfig->szAgentAccessKey, AES_128);
			if(iDecSigLen != sizeof(MonitorCommSig)) {
				REQERR_LOG("decrypt failed, %lu != %lu, siglen:%d, key:%s",
					iDecSigLen, sizeof(MonitorCommSig), ntohs(m_pstSig->wSigLen), 
					m_pcltMachine->sRandKey);
				return ERR_DECRYPT_FAILED;
			}
			pcommSig = (MonitorCommSig*)m_sDecryptBuf;
			if(ntohl(pcommSig->dwSeq) != m_dwReqSeq) {
				REQERR_LOG("invalid signature info - seq(%u,%u)", ntohl(pcommSig->dwSeq), m_dwReqSeq);
				return ERR_INVALID_SIGNATURE_INFO;
			}
			break; 

		default:
			break;
	}
	return 0;
}

int32_t CUdpSock::OnRawDataClientLog(const char *buf, size_t len)
{
	int iRet = 0;
	if(m_dwReqCmd != CMD_MONI_SEND_LOG || NULL == m_pstBody)
	{  
		REQERR_LOG("invalid packet cmd:%u != %u, or pbody:%p", m_dwReqCmd, CMD_MONI_SEND_LOG, m_pstBody);
		return AckToReq(ERR_INVALID_PACKET);
	} 

	TWTlv *pTlv = GetWTlvByType2(TLV_MONI_COMM_INFO, m_pstBody);
	if(pTlv == NULL || ntohs(pTlv->wLen) != MYSIZEOF(TlvMoniCommInfo)) {
		REQERR_LOG("get tlv(%d) failed, or invalid len(%d-%u)",
			TLV_MONI_COMM_INFO, (pTlv!=NULL ? ntohs(pTlv->wLen) : 0), MYSIZEOF(TlvMoniCommInfo));
		return AckToReq(ERR_CHECK_TLV);
	}
	TlvMoniCommInfo *pctinfo = (TlvMoniCommInfo*)pTlv->sValue;
	int iMachineId = ntohl(pctinfo->iMachineId);
	DEBUG_LOG("get request from :%s - machine id:%d ", m_addrRemote.Convert(true).c_str(), iMachineId);

	m_pcltMachine = slog.GetMachineInfo(iMachineId, NULL);
	if(m_pcltMachine == NULL) {
		REQERR_LOG("find client machine failed !");
		return AckToReq(ERR_INVALID_PACKET);
	}

	if((iRet=CheckSignature()) != NO_ERROR) {
		return AckToReq(iRet);
	}

	MonitorCommSig *pcommSig = (MonitorCommSig*)m_sDecryptBuf;

	// 是否启用了数据加密
	if(pcommSig->bEnableEncryptData)
	{
		static char sTmpBuf[MAX_APP_LOG_PKG_LENGTH+256];
		size_t iDecSigLen = 0;
		aes_decipher_data((const uint8_t*)m_pstCmdContent, m_wCmdContentLen,
			(uint8_t*)sTmpBuf, &iDecSigLen, (const uint8_t*)m_pcltMachine->sRandKey, AES_128);
		m_wCmdContentLen = (int)iDecSigLen;
		m_pstCmdContent = (void*)sTmpBuf;
	}

	int iWriteLogCount = 0;
	LogInfo *pInfo= NULL;
	TSLogShm *pLogShm = NULL;
	uint32_t tmNow = time(NULL);
	int iRead = 0;
	for(iRead=0; m_wCmdContentLen > iRead+sizeof(LogInfo); )  
	{
		pInfo = (LogInfo*)((char*)m_pstCmdContent+iRead);
		LogInfoNtoH(pInfo);
		if(iRead+sizeof(LogInfo)+pInfo->wCustDataLen+pInfo->wLogDataLen > m_wCmdContentLen) {
			REQERR_LOG("invalid log packet %d > %d (custlen:%d, loglen:%d)",
				(int)(iRead+sizeof(LogInfo)+pInfo->wCustDataLen+pInfo->wLogDataLen),
				m_wCmdContentLen, pInfo->wCustDataLen, pInfo->wLogDataLen);
			return AckToReq(ERR_INVALID_PACKET);
		}
		iRead += sizeof(LogInfo)+pInfo->wCustDataLen+pInfo->wLogDataLen;
		AppInfo *pAppInfo = slog.GetAppInfo(pInfo->iAppId);
		if(pAppInfo == NULL)
		{
			REQERR_LOG("get appinfo failed, appid:%d", pInfo->iAppId);
			continue;
		}

		// 检查分发是否正确
		if(slog.IsIpMatchMachine(stConfig.pLocalMachineInfo, pAppInfo->dwAppSrvMaster) != 1)
		{
			ERR_LOG("app req dispatch invalid, info appsvrip:%s, appid:%d",
					ipv4_addr_str(pAppInfo->dwAppSrvMaster), pAppInfo->iAppId);
			MtReport_Attr_Add(260, 1);
			return AckToReq(ERR_APP_LOG_DISPATCH_INVALID);
		}

		// 获取应用日志共享内存
		if(pLogShm == NULL || pLogShm->iAppId != pAppInfo->iAppId) 
		{
			std::map<int , TSLogShm *>::iterator it = stConfig.mapAppLogShm.find(pAppInfo->iAppId);
			if(it != stConfig.mapAppLogShm.end())
				pLogShm =  it->second;
			if(pLogShm == NULL)
			{
				pLogShm=slog.GetAppLogShm(pAppInfo, true);
				if(pLogShm == NULL) 
				{
					ERR_LOG("get applog shm failed, appid:%d", pAppInfo->iAppId);
					return AckToReq(ERR_INVALID_APPID);
				} 
				stConfig.mapAppLogShm[pAppInfo->iAppId] = pLogShm;
			}
		}
		SET_BIT(pAppInfo->dwAppLogFlag, APPLOG_FLAG_LOG_WRITED);

		// 时间校准 --- client 时间与server 时间相差超过2分钟则用server 的时间
		// 这里不直接使用 server 时间，原因是client上来的请求包可能乱序，如果直接使用server时间，日志的
		// 先后顺序就不能保证跟client 产生的顺序一样了
		if((pInfo->qwLogTime/1000000+120) < tmNow)
		{
			struct timeval stNow; 
			gettimeofday(&stNow, 0);
			pInfo->qwLogTime = stNow.tv_sec*1000000ULL+stNow.tv_usec;
		}

		// modify by rockdeng -- 使用上报机器id  - @2019-01-31
		if((iRet=slog.WriteAppLogToShm(pLogShm, pInfo, m_pcltMachine->id)) >= 0)
		{
			iWriteLogCount++;
		}
		else {
			ERR_LOG("WriteAppLogToShm failed - appid:%d, module id:%d, ret:%d",
				pInfo->iAppId, pInfo->iModuleId, iRet);
		}
	}

	MtReport_Attr_Add(330, 1);
	INFO_LOG("write user client log count: %d, from:%s", iWriteLogCount, m_addrRemote.Convert(true).c_str());
	return AckToReq(NO_ERROR);
}

void CUdpSock::OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len)
{
	SetConnected();

	slog.ClearAllCust();
	m_addrRemote.SetAddress(sa);
	slog.SetCust_6(m_addrRemote.Convert(true).c_str());

	// check packet
	int iRet = 0;
	if((iRet=CheckBasicPacket(buf, len)) != NO_ERROR)
	{
		if(iRet != ERR_NOT_ACK)
			CBasicPacket::AckToReq(iRet);
		MtReport_Attr_Add(74, 1);
		return ;
	}

	if(!PacketPb())
	{
		// 来自 slog_mtreport_client 的请求包
		OnRawDataClientLog(buf, len);
		return ;
	}

	if(m_pbHead.en_cmd() == comm::CMD_SLOG_CLIENT_HEART)
	{
		::comm::HeartInfo heart;
		const char *pBody = m_pReqPkg+1+4+m_iPbHeadLen+4;
		if(!heart.ParseFromArray(pBody, m_iPbBodyLen))
		{
			REQERR_LOG("ParseFromArray body failed ! bodylen:%d", m_iPbBodyLen);
			MtReport_Attr_Add(256, 1);
			return;
		}
		MtReport_Attr_Add(94, 1);
		DEBUG_LOG("get log heartbeat from:%s", heart.bytes_req_ip().c_str());
	}
	// 查询 app 占用的磁盘空间
	else if(m_pbHead.en_cmd() == comm::CMD_SLOG_GET_APP_LOG_SIZE_REQ)
	{
		top::SlogGetAppLogSizeReq reqsize;
		const char *pBody = m_pReqPkg+1+4+m_iPbHeadLen+4;
		if(!reqsize.ParseFromArray(pBody, m_iPbBodyLen))
		{
			REQERR_LOG("ParseFromArray body failed ! bodylen:%d", m_iPbBodyLen); 
			MtReport_Attr_Add(256, 1);
			return;
		}
		DEBUG_LOG("req app log size, info:%s", reqsize.ShortDebugString().c_str());
		MtReport_Attr_Add(257, 1);
		DealGetAppLogSizeReq(reqsize);
	}
	// 处理查询 app log file size 的响应
	else if(m_pbHead.en_cmd() == comm::CMD_SLOG_GET_APP_LOG_SIZE_RSP)
	{ 
		top::SlogGetAppLogSizeRsp rspsize;
		const char *pBody = m_pReqPkg+1+4+m_iPbHeadLen+4;
		if(!rspsize.ParseFromArray(pBody, m_iPbBodyLen))
		{
			REQERR_LOG("ParseFromArray body failed ! bodylen:%d", m_iPbBodyLen); 
			MtReport_Attr_Add(256, 1);
			return;
		}
		DEBUG_LOG("rsp app log size, info:%s", rspsize.ShortDebugString().c_str());
		MtReport_Attr_Add(258, 1);
		DealGetAppLogSizeRsp(rspsize);
	}
	// app 日志上报
	else if(m_pbHead.en_cmd() == comm::SLOG_CLIENT_SEND_LOG)
	{
		OnRawDataLocalPkg(buf, len, sa, sa_len);
	}
	else
	{
		REQERR_LOG("unknow cmd:%d", m_pbHead.en_cmd());
		AckToReq(ERR_UNKNOW_CMD);
	}
}

void CUdpSock::SendGetAppLogSizeRsp(top::SlogGetAppLogSizeRsp &rsp)
{
	char *pack = NULL;
	int ipackLen = 0;
	PkgHead head;
	head.set_en_cmd(::comm::CMD_SLOG_GET_APP_LOG_SIZE_RSP);
	head.set_uint32_seq(m_dwReqSeqLocal++);
	std::string strHead, strBody;
	if(head.AppendToString(&strHead) && rsp.AppendToString(&strBody))
	{
		if((ipackLen=SetPacketPb(strHead, strBody, &pack)) < 0)
		{
			ERR_LOG("SetPacketPb failed ret:%d", ipackLen);
			return ;
		}   
		SendToBuf(m_addrRemote, pack, ipackLen, 0);
		MtReport_Attr_Add(259, 1);
		DEBUG_LOG("send get app log file size rsp, packlen:%d, to:%s", ipackLen, m_addrRemote.Convert(true).c_str());
	}
	else
	{           
		ERR_LOG("AppendToString failed !");
		MtReport_Attr_Add(252, 1);
	}
}

void CUdpSock::SendGetAppLogSizeReq(const TGetAppLogSizeKey &info, top::SlogGetAppLogSizeReq *pApps)
{
	char *pack = NULL;
	int ipackLen = 0;
	PkgHead head;
	head.set_en_cmd(::comm::CMD_SLOG_GET_APP_LOG_SIZE_REQ);
	head.set_uint32_seq(m_dwReqSeqLocal++);
	std::string strHead, strBody;
	if(head.AppendToString(&strHead) && pApps->AppendToString(&strBody))
	{
		if((ipackLen=SetPacketPb(strHead, strBody, &pack)) < 0)
		{
			ERR_LOG("SetPacketPb failed ret:%d", ipackLen);
			return ;
		}   
		Ipv4Address addr(info.dwAppLogSrv, info.wAppLogSrvPort);
		SendToBuf(addr, pack, ipackLen, 0);
		MtReport_Attr_Add(255, 1);
		DEBUG_LOG("send get app log file size req, packlen:%d, to:%s", ipackLen, addr.Convert(true).c_str());
	}
	else
	{           
		ERR_LOG("AppendToString failed !");
		MtReport_Attr_Add(252, 1);
	}
}

void CUdpSock::OnRawDataLocalPkg(
	const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len)
{
	static char sLogBuf[BWORLD_SLOG_MAX_LINE_LEN+128];

	MtReport_Attr_Add(73, 1);

	slog.CheckTest(NULL);

	::top::SlogClientPkgBody logs;
	const char *pBody = m_pReqPkg+1+4+m_iPbHeadLen+4;
	if(!logs.ParseFromArray(pBody, m_iPbBodyLen))
	{
		REQERR_LOG("ParseFromArray body failed ! bodylen:%d", m_iPbBodyLen);
		AckToReq(ERR_INVALID_PACKET); 
		MtReport_Attr_Add(74, 1);
		return;
	}
	if(logs.log().size() <= 0)
	{
		REQERR_LOG("have no log in packet ");
		AckToReq(ERR_INVALID_PACKET); 
		MtReport_Attr_Add(74, 1);
		return;
	}

	AppInfo *pAppInfo = slog.GetAppInfo(logs.uint32_app_id());
	if(pAppInfo == NULL)
	{
		ERR_LOG("get app info failed, appid:%d", logs.uint32_app_id());
		AckToReq(ERR_INVALID_APPID);
		return;
	}

	// 检查分发是否正确
	if(slog.IsIpMatchMachine(stConfig.pLocalMachineInfo, pAppInfo->dwAppSrvMaster) != 1)
	{
		ERR_LOG("app req dispatch invalid, info appsvrip:%s, appid:%d",
			ipv4_addr_str(pAppInfo->dwAppSrvMaster), pAppInfo->iAppId);
		MtReport_Attr_Add(260, 1);
		AckToReq(ERR_APP_LOG_DISPATCH_INVALID);
		return;
	}

	TSLogShm * pLogShm = NULL;
	std::map<int , TSLogShm *>::iterator it = stConfig.mapAppLogShm.find(pAppInfo->iAppId);
	if(it != stConfig.mapAppLogShm.end())
		pLogShm =  it->second;
	if(pLogShm == NULL)
	{
		pLogShm=slog.GetAppLogShm(pAppInfo, true);
		if(pLogShm == NULL) 
		{
			ERR_LOG("get applog shm failed, appid:%d", pAppInfo->iAppId);
			AckToReq(ERR_INVALID_APPID);
			return ;
		} 
		stConfig.mapAppLogShm[pAppInfo->iAppId] = pLogShm;
	}
	SET_BIT(pAppInfo->dwAppLogFlag, APPLOG_FLAG_LOG_WRITED);

	MtReport_Attr_Add(75, logs.log().size());
	MtReport_Attr_Add(331, logs.log().size());
	TSLogOut stLog;
	stLog.pszLog = sLogBuf;
	uint32_t tmNow = time(NULL);
	int i = 0;
	for(i=0; i < logs.log().size(); i++)
	{
		const ::top::SlogLogInfo & log = logs.log(i);
		PB_LOG_TO_SLOG_OUT(log, stLog);
		// 时间校准 --- client 时间与server 时间相差超过2分钟则用server 的时间
		if((stLog.qwLogTime/1000000+120) < tmNow)
		{
			struct timeval stNow; 
			gettimeofday(&stNow, 0);
			stLog.qwLogTime = stNow.tv_sec*1000000ULL+stNow.tv_usec;
			MtReport_Attr_Add(220, 1);
		}
		slog.RemoteShmLog(stLog, pLogShm);
	}

	DEBUG_LOG("get recv log:%d appid:%d module id:%d head length:%d body length:%d", 
		logs.log().size(), logs.log(0).uint32_app_id(), logs.log(0).uint32_module_id(),
		m_iPbHeadLen, m_iPbBodyLen);
	AckToReq(NO_ERROR); 
}


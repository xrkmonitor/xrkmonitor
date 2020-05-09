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

#include <supper_log.h>
#include <basic_packet.h>
#include <errno.h>
#include <sv_struct.h>
#include <mt_report.h>
#include <aes.h>
#include <md5.h>
#include <supper_log.h>
#include <sv_struct.h>
#include "mtreport_server.h"
#include "udp_sock.h"
#include "comm.pb.h"


#define LOG_CHECK_COPY_CONFIG_INFO(pCfgInfo) \
	stLogConfig.dwSeq = htonl(pCfgInfo->dwConfigSeq); \
	stLogConfig.dwCfgId = htonl(pCfgInfo->dwCfgId); \
	stLogConfig.iAppId = htonl(pCfgInfo->iAppId); \
	stLogConfig.iModuleId = htonl(pCfgInfo->iModuleId); \
	stLogConfig.iLogType = htonl(pCfgInfo->iLogType); \
	stLogConfig.dwSpeedFreq = htonl(pCfgInfo->dwSpeedFreq); \
	stLogConfig.wTestKeyCount = htons(pCfgInfo->wTestKeyCount); \
    if(pCfgInfo->wTestKeyCount > 0) \
        memcpy(stLogConfig.stTestKeys, pCfgInfo->stTestKeys, MYSIZEOF(SLogTestKey)*pCfgInfo->wTestKeyCount); 

#define APPINFO_CHECK_COPY_INFO \
	stAppInfo.dwSeq = htonl(pAppInfo->dwSeq); \
	stAppInfo.iAppId = htonl(pAppInfo->iAppId); \
	stAppInfo.wModuleCount = htons(pAppInfo->wModuleCount); \
	stAppInfo.wLogSrvPort = htons(pAppInfo->wLogSrvPort); \
	stAppInfo.dwAppSrvMaster = htonl(pAppInfo->dwAppSrvMaster); 

void CUdpSock::Init() 
{
	slog.ClearAllCust();
	memset(m_sDecryptBuf, 0, MYSIZEOF(m_sDecryptBuf));
	m_pMtClient = NULL;
	m_pConfig = stConfig.pShmConfig;
	m_pAppInfo = slog.GetAppInfo();
	m_iUserMasterIndex = -1;
	m_iMtClientIndex = -1;
	m_dwAgentClientIp = 0;
	m_bIsFirstHello = false;
}

int CUdpSock::InitSignature(TSignature *psig, void *pdata, const char *pKey, int bSigType)
{
	return 0;
}

CUdpSock::CUdpSock(ISocketHandler&h): UdpSocket(h), CBasicPacket()
{
	Attach(CreateSocket(PF_INET, SOCK_DGRAM, "udp"));
	Init();

	// g++ -E -P xx > xx -- 宏扩展输出
	DEBUG_LOG("on CUdpSock construct");
}

CUdpSock::~CUdpSock()
{
	DEBUG_LOG("on CUdpSock destruct");
}

int32_t CUdpSock::SendResponsePacket(const char*pkg, int len)
{
	SendToBuf(m_addrRemote, pkg, len, 0);
	DEBUG_LOG("send response packet to client pkglen:%d, addr:%s", len, m_addrRemote.Convert(true).c_str());
	return 0;
}

int32_t CUdpSock::CheckSignature()
{
	MonitorCommSig *pcommSig = NULL;
	size_t iDecSigLen = 0;

	switch(m_pstSig->bSigType) {
		case MT_SIGNATURE_TYPE_HELLO_FIRST:
			aes_decipher_data((const uint8_t*)m_pstSig->sSigValue, ntohs(m_pstSig->wSigLen),
			    (uint8_t*)m_sDecryptBuf, &iDecSigLen, (const uint8_t*)stConfig.psysConfig->szAgentAccessKey, AES_256);
			if(iDecSigLen != sizeof(MonitorHelloSig)) {
				REQERR_LOG("decrypt failed, %lu != %lu, siglen:%d, key:%s",
					iDecSigLen, sizeof(MonitorHelloSig), ntohs(m_pstSig->wSigLen), 
					stConfig.psysConfig->szAgentAccessKey);
				return ERR_DECRYPT_FAILED;
			}
			break; 

		case MT_SIGNATURE_TYPE_COMMON:
			if(!m_pMtClient->bEnableEncryptData)
				break;
			aes_decipher_data((const uint8_t*)m_pstSig->sSigValue, ntohs(m_pstSig->wSigLen),
			    (uint8_t*)m_sDecryptBuf, &iDecSigLen, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
			if(iDecSigLen != sizeof(MonitorCommSig)) {
				REQERR_LOG("decrypt failed, %lu != %lu, siglen:%d, key:%s",
					iDecSigLen, sizeof(MonitorCommSig), ntohs(m_pstSig->wSigLen), 
					m_pMtClient->sRandKey);
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

void CUdpSock::InitMtClientInfo()
{
	if(m_pMtClient == NULL) {
		ERR_LOG("bug -- m_pMtClient(%p)", m_pMtClient);
		return;
	}

	m_pMtClient->dwLastHelloTime = stConfig.dwCurrentTime;
	m_pMtClient->dwFirstHelloTime = stConfig.dwCurrentTime;
	m_pMtClient->dwAddress = m_addrRemote.GetAddr();
	m_pMtClient->wBasePort = m_addrRemote.GetPort();
}

void CUdpSock::SendRealInfo()
{
	/*
	::comm::SysconfigInfo stInfo;
	std::string strContent;

	CBasicPacket pkg;

	// ReqPkgHead 
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_INNER_SEND_REALINFO, slog.m_iRand);
	*(int32_t*)(stHead.sReserved) = htonl(slog.GetLocalMachineId());

	// TSignature - empty
	// [cmd content]
	pkg.InitCmdContent((void*)strContent.c_str(), strContent.length());
	// TPkgBody - empty

	if(pkg.MakeReqPkg() > 0) {
		Ipv4Address addr;
		addr.SetAddress(stConfig.pCenterServer->dwIp, stConfig.pCenterServer->wPort);
		SendToBuf(addr, pkg, iPkgLen, 0);
		DEBUG_LOG("SendLogToServer ok, server:%s:%d", psrv->szIpV4, psrv->wPort);
	}
	*/
}

int CUdpSock::GetMtClientInfo()
{
	uint32_t isFind = 0;
	MachineInfo* pstMachInfo = NULL;

	// 非 first hello 一定存在相关索引
	if(!m_bIsFirstHello && (m_iRemoteMachineId <= 0 || m_iMtClientIndex < 0)) {
		REQERR_LOG("invalid packet - not first hello packet have no some cache !");
		return ERR_INVALID_PACKET;
	}

	if((m_dwReqCmd == CMD_MONI_SEND_HELLO || m_dwReqCmd == CMD_MONI_SEND_HELLO_FIRST)
		&& m_dwAgentClientIp == 0) 
	{
		REQERR_LOG("invalid packet - agent ip is 0");
		return ERR_INVALID_PACKET;
	}

	if(m_iRemoteMachineId <= 0) {
		// first hello
		pstMachInfo = slog.GetMachineInfo(m_dwAgentClientIp);
		if(pstMachInfo != NULL) {
			m_iRemoteMachineId = pstMachInfo->id;
			INFO_LOG("get mtclient machine id:%d", m_iRemoteMachineId);
		}
	}
	else {
		// not first hello
		pstMachInfo = slog.GetMachineInfo(m_iRemoteMachineId, NULL);
		if(pstMachInfo == NULL 
			|| (m_dwReqCmd == CMD_MONI_SEND_HELLO && !slog.IsIpMatchMachine(pstMachInfo, m_dwAgentClientIp))) 
		{
			REQERR_LOG("invalid, have no machine for id:%d", m_iRemoteMachineId);
			return ERR_NOT_FIND_MACHINE;
		}
	}
	
	// 新客户端接入，写入数据库自动注册
	if(m_iRemoteMachineId <= 0) {
		snprintf(stConfig.szSql, MYSIZEOF(stConfig.szSql), 
			"select xrk_id,xrk_status from mt_machine where (ip1=%u or ip2=%u or ip3=%u or ip4=%u)",
			m_dwAgentClientIp,m_dwAgentClientIp,m_dwAgentClientIp,m_dwAgentClientIp);

		MyQuery myqu(stConfig.qu, stConfig.db);
		Query & qu = myqu.GetQuery();
		if(!qu.get_result(stConfig.szSql)) {
			ERR_LOG("execute sql:%s failed !", stConfig.szSql);
			return ERR_SERVER;
		}

		if(qu.num_rows() <= 0 || !qu.fetch_row() || qu.getval("xrk_status") != 0)
		{
			int iDbMachineId = 0;
			if(qu.num_rows() > 0)
			    iDbMachineId = qu.getval("xrk_id");
			qu.free_result();

			if(iDbMachineId > 0) 
				snprintf(stConfig.szSql, MYSIZEOF(stConfig.szSql),
					"replace into mt_machine set xrk_name=\'%s\',ip1=%u,create_time=\'%s\',"
					"mod_time=now(),machine_desc=\'系统自动添加\',xrk_status=0,xrk_id=%d",
					ipv4_addr_str(m_dwAgentClientIp), m_dwAgentClientIp, uitodate(slog.m_stNow.tv_sec),
					iDbMachineId);
			else
				snprintf(stConfig.szSql, MYSIZEOF(stConfig.szSql),
					"insert into mt_machine set xrk_name=\'%s\',ip1=%u,create_time=\'%s\',"
					"mod_time=now(),machine_desc=\'系统自动添加\'",
					ipv4_addr_str(m_dwAgentClientIp), m_dwAgentClientIp, uitodate(slog.m_stNow.tv_sec));
			if(!qu.execute(stConfig.szSql)){
				ERR_LOG("execute sql:%s failed !", stConfig.szSql);
				return ERR_SERVER;
			}
			if(iDbMachineId > 0)
				m_iRemoteMachineId = iDbMachineId;
			else
				m_iRemoteMachineId = qu.insert_id();
			INFO_LOG("insert new machine for agent client ip:%s, machine id:%d",
				ipv4_addr_str(m_dwAgentClientIp), m_iRemoteMachineId);
		}
		else {
			qu.fetch_row();
			m_iRemoteMachineId = qu.getval("xrk_id");
			qu.free_result();
			INFO_LOG("get remote machine id:%d by sql ok", m_iRemoteMachineId);
		}
	}

	if(m_iMtClientIndex >= 0) {
		m_pMtClient = slog.GetMtClientInfo(m_iRemoteMachineId, m_iMtClientIndex);
		if(m_pMtClient != NULL) {
			DEBUG_LOG("GetMtClientInfo by cache index:%d ok ", m_iMtClientIndex);
		}
	}

	if(m_pMtClient == NULL) {
		m_pMtClient = slog.GetMtClientInfo(m_iRemoteMachineId, &isFind);
		if(m_pMtClient != NULL) {
			m_iMtClientIndex = slog.GetMtClientInfoIndex(m_pMtClient);
			if(!isFind) {
				// 只有 first hello 才可能找不到 client !
				if(!m_bIsFirstHello) {
					REQERR_LOG("not first hello, but have no mtclient !");
					return ERR_INVALID_PACKET;
				}
				m_pMtClient->dwAgentClientAddress = m_dwAgentClientIp;
				m_pMtClient->iMachineId = m_iRemoteMachineId;
				if(stConfig.psysConfig->wCurClientCount == 0) {
					ILINK_SET_FIRST(stConfig.psysConfig, iMtClientListIndexStart, iMtClientListIndexEnd,
						m_pMtClient, iPreIndex, iNextIndex, m_iMtClientIndex);
					stConfig.psysConfig->wCurClientCount = 1;
				}
				else {
					MtClientInfo *pfirst = slog.GetMtClientInfo(stConfig.psysConfig->iMtClientListIndexStart);
					ILINK_INSERT_FIRST(stConfig.psysConfig, iMtClientListIndexStart,
						m_pMtClient, iPreIndex, iNextIndex, pfirst, m_iMtClientIndex);
					stConfig.psysConfig->wCurClientCount++;
				}
				INFO_LOG("create MtClientInfo machine id:%d index:%d count:%d", 
					m_iRemoteMachineId, m_iMtClientIndex, stConfig.psysConfig->wCurClientCount);
			}
			else
				DEBUG_LOG("GetMtClientInfo by client address ok, index:%d", m_iMtClientIndex);
		}
	}

	if(m_pMtClient == NULL) {
		REQERR_LOG("get mtclient info failed !");
		return ERR_NOT_FIND_MACHINE;
	}
	return 0;
}

int CUdpSock::SetKeyToMachineTable()
{
	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	AddParameter(&ppara, "rand_key", m_pMtClient->sRandKey, NULL);
	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();

	std::string strSql;
	strSql = "update mt_machine set";
	JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
	strSql += " where id=";
	strSql += itoa(m_iRemoteMachineId);
	ReleaseParameter(&ppara);
	if(!qu.execute(strSql)) {
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("set machine key for machine id:%d ok", m_iRemoteMachineId);
	return 0;
}

int CUdpSock::DealCmdHelloFirst()
{
	if(m_wCmdContentLen != MYSIZEOF(MonitorHelloFirstContent)) {
		REQERR_LOG("invalid cmd content %u != %u", m_wCmdContentLen, MYSIZEOF(MonitorHelloFirstContent));
		return ERR_INVALID_CMD_CONTENT;
	}

	MonitorHelloFirstContent *pctinfo = (MonitorHelloFirstContent*)m_pstCmdContent;
	m_iMtClientIndex = ntohl(pctinfo->iMtClientIndex);
	m_iRemoteMachineId = ntohl(pctinfo->iMachineId);
	m_pstReqHead->sEchoBuf[ sizeof(m_pstReqHead->sEchoBuf) - 1 ] = '\0';
	DEBUG_LOG("get cmd first hello - client index:%d, machine id:%d, ver:%d, cmp time:%s",
		m_iMtClientIndex, m_iRemoteMachineId, ntohs(m_pstReqHead->wVersion), m_pstReqHead->sEchoBuf);

	// 签名部分处理
	int iRet = 0;
	if((iRet=CheckSignature()) != NO_ERROR)
		return iRet;
	MonitorHelloSig *psigInfo = (MonitorHelloSig*)m_sDecryptBuf;
	if(ntohl(psigInfo->dwPkgSeq) != m_dwReqSeq)
	{
		REQERR_LOG("invalid signature info - seq(%u,%u)", ntohl(psigInfo->dwPkgSeq), m_dwReqSeq);
		return ERR_INVALID_SIGNATURE_INFO;
	}
	m_dwAgentClientIp = ntohl(psigInfo->dwAgentClientIp);
	INFO_LOG("check packet ok remote client ip:%s, conn ip:%s",
		ipv4_addr_str(m_dwAgentClientIp), m_addrRemote.Convert().c_str());

	// 获取请求客户端
	if((iRet=GetMtClientInfo()) != 0)
		return iRet;

	// 首个 hello 重新初始化
	InitMtClientInfo(); 

	if(psigInfo->bEnableEncryptData) {
		m_pMtClient->bEnableEncryptData = 1;
		memcpy(m_pMtClient->sRandKey, psigInfo->sRespEncKey, sizeof(m_pMtClient->sRandKey));

		if(SetKeyToMachineTable() < 0)
			return AckToReq(ERR_SET_MACHINE_KEY_FAILED);
		DEBUG_LOG("set encrypt data key:%s", psigInfo->sRespEncKey);
	}
	else
		m_pMtClient->bEnableEncryptData = 0;

	// 响应处理
	MonitorHelloFirstContentResp stResp;
	memset(&stResp, 0, sizeof(stResp));
	stResp.iMtClientIndex = htonl(m_iMtClientIndex);
	stResp.iMachineId = htonl(m_iRemoteMachineId);
	stResp.dwConnServerIp = m_pMtClient->dwAddress;
	stResp.wAttrSrvPort = htons(stConfig.psysConfig->wAttrSrvPort);
	stResp.dwAttrSrvIp = stConfig.psysConfig->dwAttrSrvMasterIp;
	INFO_LOG("set attr server %s:%d", ipv4_addr_str(stResp.dwAttrSrvIp), stConfig.psysConfig->wAttrSrvPort);
	
	// 下发下最新的接入服务器 slog_mtreport_server 地址
	int iStartIdx = 0;
	SLogServer *psrv_m = slog.GetValidServerByType(SRV_TYPE_CONNECT, &iStartIdx);
	if(psrv_m != NULL)
	{
		memcpy(stResp.szNewMasterSrvIp, psrv_m->szIpV4, sizeof(stResp.szNewMasterSrvIp));
		stResp.wNewSrvPort = htons(psrv_m->wPort);
		DEBUG_LOG("set master svr:%s:%d", psrv_m->szIpV4, psrv_m->wPort);
	}

	char sEncBuf[1024] = {0};
	int iSigBufLen = ((sizeof(stResp)>>4)+1)<<4;
	if(iSigBufLen > MAX_SIGNATURE_LEN)
		ERR_LOG("need more space %d > %d", iSigBufLen, MAX_SIGNATURE_LEN);
	aes_cipher_data((const uint8_t*)&stResp,
		sizeof(stResp), (uint8_t*)sEncBuf, (const uint8_t*)psigInfo->sRespEncKey, AES_128);

	// 成功响应
	InitCmdContent(sEncBuf, iSigBufLen);
	AckToReq(NO_ERROR); 
	DEBUG_LOG("response first hello cmd content len:%d", iSigBufLen);
	return NO_ERROR;
}

int CUdpSock::DealCommInfo()
{
	TWTlv *pTlv = GetWTlvByType2(TLV_MONI_COMM_INFO, m_pstBody);
	if(pTlv == NULL || ntohs(pTlv->wLen) != MYSIZEOF(TlvMoniCommInfo)) {
		REQERR_LOG("get tlv(%d) failed, or invalid len(%d-%u)",
			TLV_MONI_COMM_INFO, (pTlv!=NULL ? ntohs(pTlv->wLen) : 0), MYSIZEOF(TlvMoniCommInfo));
		return ERR_CHECK_TLV;
	}
	TlvMoniCommInfo *pctinfo = (TlvMoniCommInfo*)pTlv->sValue;
	m_iMtClientIndex = ntohl(pctinfo->iMtClientIndex);
	m_iRemoteMachineId = ntohl(pctinfo->iMachineId);
	if(m_dwReqCmd == CMD_MONI_SEND_HELLO)
		m_dwAgentClientIp = ntohl(pctinfo->dwReserved_1);

	DEBUG_LOG("get comminfo by tlv MtClientIndex:%d", m_iMtClientIndex);

	// 获取请求客户端
	if(GetMtClientInfo() != 0)
		return ERR_INVALID_PACKET;

	// 签名部分处理
	int iRet = 0;
	if((iRet=CheckSignature()) != NO_ERROR)
		return iRet;
	return NO_ERROR;
}

int CUdpSock::DealCmdHello()
{
	if(m_wCmdContentLen != MYSIZEOF(MonitorHelloContent)) {
		REQERR_LOG("invalid cmd content %u != %u", m_wCmdContentLen, MYSIZEOF(MonitorHelloContent));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	MonitorHelloContent *pctinfo = (MonitorHelloContent*)m_pstCmdContent;
	m_pMtClient->dwLastHelloTime = stConfig.dwCurrentTime;
	DEBUG_LOG("get hello , server response time:%u ms , hello times:%u update last hello time -",
		ntohl(pctinfo->dwServerResponseTime), ntohl(pctinfo->dwHelloTimes));

	if((int)ntohl(pctinfo->dwServerResponseTime) > m_pMtClient->iServerResponseTimeMs) {
		m_pMtClient->iServerResponseTimeMs = ntohl(pctinfo->dwServerResponseTime);
		INFO_LOG("update max server response time to:%u", m_pMtClient->iServerResponseTimeMs);
	}

	// 成功响应
	MonitorHelloContentResp stResp;
	stResp.iMtClientIndex = htonl(m_iMtClientIndex);

	if(pctinfo->dwAttrSrvIp != stConfig.psysConfig->dwAttrSrvMasterIp
		|| pctinfo->wAttrServerPort != htons(stConfig.psysConfig->wAttrSrvPort))
	{
		stResp.bConfigChange = 1;
		stResp.wAttrServerPort = htons(stConfig.psysConfig->wAttrSrvPort);
		stResp.dwAttrSrvIp = stConfig.psysConfig->dwAttrSrvMasterIp;
		INFO_LOG("set new attr server :%s:%d", ipv4_addr_str(stResp.dwAttrSrvIp), stConfig.psysConfig->wAttrSrvPort);
	}
	else
	{
		stResp.bConfigChange = 0;
	}

	if(m_pMtClient->bEnableEncryptData) {
		char sEncBuf[128] = {0};
		int iSigBufLen = ((sizeof(stResp)>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sEncBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sEncBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)&stResp,
			sizeof(stResp), (uint8_t*)sEncBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sEncBuf, iSigBufLen);
	}
	else
		InitCmdContent(&stResp, sizeof(stResp));
	AckToReq(NO_ERROR); 
	return NO_ERROR;
}

int CUdpSock::InnerDealLogConfigCheck(int iFirstIdx, int iInfoCount, ContentCheckLogConfig *pctinfo,
	int &iAddCount, int &iModCount, int &iSameCount, int &iUseBufLen, int iMaxBufLen, TPkgBody *pRespTlvBody)
{
	if(iFirstIdx < 0 || iFirstIdx >= MAX_SLOG_CONFIG_COUNT)
	{
		WARN_LOG("invalid index:%d, max:%d", iFirstIdx, MAX_SLOG_CONFIG_COUNT);
		return SLOG_ERROR_LINE;
	}

	int iTmpLen = 0;
	SLogClientConfig *pCfgInfo = m_pConfig->stConfig+iFirstIdx;
	MtSLogConfig stLogConfig;
	for(int i=0,j=0; i < iInfoCount; i++) 
	{
		for(j=0; j < pctinfo->wLogConfigCount; j++) 
		{
			if(pCfgInfo->dwCfgId == pctinfo->stLogConfigList[j].dwCfgId) 
			{
				SET_BIT(pctinfo->stLogConfigList[j].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED);

				// check config seq
				if(pCfgInfo->dwConfigSeq != pctinfo->stLogConfigList[j].dwSeq) 
				{
					iTmpLen = MYSIZEOF(TWTlv) + MYSIZEOF(stLogConfig) - sizeof(SLogTestKey)*MAX_SLOG_TEST_KEYS_PER_CONFIG 
						+ pCfgInfo->wTestKeyCount*sizeof(SLogTestKey);
					if(iUseBufLen+iTmpLen > iMaxBufLen)
						break;

					INFO_LOG("log config change id:%u seq:%u(client:%u), tmpLen:%d, testkey:%d",
						pCfgInfo->dwCfgId, pCfgInfo->dwConfigSeq, 
						pctinfo->stLogConfigList[j].dwSeq, iTmpLen, pCfgInfo->wTestKeyCount);

					LOG_CHECK_COPY_CONFIG_INFO(pCfgInfo);

					iUseBufLen += SetWTlv((TWTlv*)((char*)pRespTlvBody+iUseBufLen), TLV_MONI_CONFIG_MOD, 
						iTmpLen-(int)MYSIZEOF(TWTlv), (const char*)(&stLogConfig));
					pRespTlvBody->bTlvNum++;
					iModCount++;
					iTmpLen = 0;
				}
				else 
					iSameCount++;
				break; 
			}
		}

		if(j >= pctinfo->wLogConfigCount) 
		{
			iTmpLen = MYSIZEOF(TWTlv) + MYSIZEOF(stLogConfig) - sizeof(SLogTestKey)*MAX_SLOG_TEST_KEYS_PER_CONFIG 
				+ pCfgInfo->wTestKeyCount*sizeof(SLogTestKey);
			if(iUseBufLen+iTmpLen > iMaxBufLen)
				break;

			INFO_LOG("add log config to client config id:%u seq:%u, tmpLen:%d, testkey:%d", 
				pCfgInfo->dwCfgId, pCfgInfo->dwConfigSeq, iTmpLen, pCfgInfo->wTestKeyCount);

			LOG_CHECK_COPY_CONFIG_INFO(pCfgInfo);

			iUseBufLen += SetWTlv((TWTlv*)((char*)pRespTlvBody+iUseBufLen), TLV_MONI_CONFIG_ADD,
				iTmpLen-(int)MYSIZEOF(TWTlv), (const char*)(&stLogConfig));
			pRespTlvBody->bTlvNum++;
			iAddCount++;
			iTmpLen = 0;
		}

		if(iTmpLen > 0) // 缓冲区不足了
			break;

		if(pCfgInfo->iNextIndex >= 0)
			pCfgInfo = m_pConfig->stConfig+pCfgInfo->iNextIndex;
		else
			pCfgInfo = NULL;
	}

	return iTmpLen;
}

int CUdpSock::SetLogConfigCheckInfo(ContentCheckLogConfig *pctinfo)
{
	static char sRespBuf[1200];
	memset(sRespBuf, 0, sizeof(sRespBuf));

	TPkgBody *pRespTlvBody = (TPkgBody*)sRespBuf;
	int iUseBufLen = MYSIZEOF(TPkgBody), i=0, j=0;

	pctinfo->wLogConfigCount = ntohs(pctinfo->wLogConfigCount);
	for(i=0; i < pctinfo->wLogConfigCount; i++) {
#define LOG_CHECK_32_TO_LOCAL(field) \
		pctinfo->stLogConfigList[i].field = ntohl(pctinfo->stLogConfigList[i].field);
		LOG_CHECK_32_TO_LOCAL(dwCfgId);
		LOG_CHECK_32_TO_LOCAL(dwSeq);
		LOG_CHECK_32_TO_LOCAL(dwCfgFlag);
#undef LOG_CHECK_32_TO_LOCAL 
		CLEAR_BIT(pctinfo->stLogConfigList[i].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED);
	}

	int iAddCount = 0, iModCount = 0, iDelCount = 0, iSameCount = 0, iTmpLen = 0;

	if(stConfig.psysConfig->wLogConfigCount > 0)
		iTmpLen = InnerDealLogConfigCheck(stConfig.psysConfig->iLogConfigIndexStart, stConfig.psysConfig->wLogConfigCount, 
			pctinfo, iAddCount, iModCount, iSameCount, iUseBufLen, (int)MYSIZEOF(sRespBuf), pRespTlvBody);

	// 检查是否有被删除的 log config, 通过检查处理标志是否设置实现
	uint32_t dwCfgIdDel = 0;
	for(j=0; iTmpLen <= 0 && j < pctinfo->wLogConfigCount; j++) 
	{
		if(IS_SET_BIT(pctinfo->stLogConfigList[j].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED))
			continue;

		iTmpLen = MYSIZEOF(uint32_t)+MYSIZEOF(TWTlv);
		if(iUseBufLen+iTmpLen > (int)MYSIZEOF(sRespBuf))
			break;

		dwCfgIdDel = htonl(pctinfo->stLogConfigList[j].dwCfgId);
		iUseBufLen += SetWTlv((TWTlv*)((char*)pRespTlvBody+iUseBufLen),
			TLV_MONI_CONFIG_DEL, MYSIZEOF(dwCfgIdDel), (const char*)(&dwCfgIdDel));
		pRespTlvBody->bTlvNum++;
		INFO_LOG("delete log config id:%u", pctinfo->stLogConfigList[j].dwCfgId);
		iDelCount++;
		iTmpLen = 0;
	}
	if(iTmpLen > 0)
		WARN_LOG("need more space %d+%d > %u", iUseBufLen, iTmpLen, MYSIZEOF(sRespBuf));

	if(m_pMtClient->bEnableEncryptData) {
		static char sContentBuf[2048+256];
		int iSigBufLen = ((iUseBufLen>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sContentBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sContentBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)pRespTlvBody, iUseBufLen,
			(uint8_t*)sContentBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sContentBuf, iSigBufLen);
	}
	else {
		InitCmdContent(pRespTlvBody, iUseBufLen);
	}
	INFO_LOG("check log config count add:%d mod:%d del:%d not change:%d, content len:%u, tlvnum:%d",
		iAddCount, iModCount, iDelCount, iSameCount, iUseBufLen, pRespTlvBody->bTlvNum);
	return NO_ERROR;
}

int CUdpSock::DealCmdCheckLogConfig()
{
	if(m_wCmdContentLen < MYSIZEOF(ContentCheckLogConfig)) {
		REQERR_LOG("invalid cmd content %u < %u", m_wCmdContentLen, MYSIZEOF(ContentCheckLogConfig));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	ContentCheckLogConfig *pctinfo = (ContentCheckLogConfig*)m_pstCmdContent;
	if(m_wCmdContentLen != MYSIZEOF(ContentCheckLogConfig)
		+ MYSIZEOF(pctinfo->stLogConfigList[0])*ntohs(pctinfo->wLogConfigCount)){
		REQERR_LOG("check cmd content failed %d != %u", m_wCmdContentLen,
			MYSIZEOF(ContentCheckLogConfig)+MYSIZEOF(uint32_t)*ntohs(pctinfo->wLogConfigCount));
		return ERR_INVALID_CMD_CONTENT;
	}

	DEBUG_LOG("get check log config, server response time:%u ms , log config count:%d",
		ntohl(pctinfo->dwServerResponseTime), ntohs(pctinfo->wLogConfigCount));
	if((int)ntohl(pctinfo->dwServerResponseTime) > m_pMtClient->iServerResponseTimeMs) {
		m_pMtClient->iServerResponseTimeMs = ntohl(pctinfo->dwServerResponseTime);
		INFO_LOG("update max server response time to:%u", m_pMtClient->iServerResponseTimeMs);
	}

	if((iRet=SetLogConfigCheckInfo(pctinfo)) != NO_ERROR)
		return iRet;
	AckToReq(NO_ERROR); 
	return NO_ERROR;
}

int CUdpSock::InnerDealAppInfoCheck(int iFirstAppIdx, int iAppInfoCount, ContentCheckAppInfo *pctinfo,
	int &iAddCount, int &iModCount, int &iSameCount, int &iUseBufLen, int iMaxBufLen, TPkgBody *pRespTlvBody)
{
	if(iFirstAppIdx < 0 || iFirstAppIdx >= MAX_SLOG_APP_COUNT)
	{
		WARN_LOG("invalid app index:%d, max:%d", iFirstAppIdx, MAX_SLOG_APP_COUNT);
		return SLOG_ERROR_LINE;
	}

	int iTmpLen = 0, iAppInfoLen = 0;
	AppInfo *pAppInfo = m_pAppInfo->stInfo+iFirstAppIdx;
	MtAppInfo stAppInfo;
	for(int i=0,j=0; i < iAppInfoCount; i++) 
	{
		for(j=0; j < pctinfo->wAppInfoCount; j++) 
		{
			if(pAppInfo->iAppId == pctinfo->stAppList[j].iAppId) {

				SET_BIT(pctinfo->stAppList[j].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED);

				// check config seq
				if(pAppInfo->dwSeq != pctinfo->stAppList[j].dwSeq) 
				{
					iAppInfoLen = MYSIZEOF(MtAppInfo);
					iTmpLen = MYSIZEOF(TWTlv) + iAppInfoLen;
					if(iUseBufLen+iTmpLen > iMaxBufLen)
						break;

					INFO_LOG("app info change app id:%u seq:%u(client:%u)",
						pAppInfo->iAppId, pAppInfo->dwSeq, pctinfo->stAppList[j].dwSeq);

					// copy new config to resp
					APPINFO_CHECK_COPY_INFO;

					iUseBufLen += SetWTlv((TWTlv*)(pRespTlvBody+iUseBufLen), 
						TLV_MONI_CONFIG_MOD, iAppInfoLen, (const char*)(&stAppInfo));
					pRespTlvBody->bTlvNum++;
					iModCount++;
					iTmpLen = 0;
				}
				else 
					iSameCount++;
				break;
			}
		}

		if(j >= pctinfo->wAppInfoCount) {
			iAppInfoLen = MYSIZEOF(MtAppInfo);
			iTmpLen = MYSIZEOF(TWTlv) + iAppInfoLen;
			if(iUseBufLen+iTmpLen > iMaxBufLen)
				break;

			INFO_LOG("add app info to client appid:%u seq:%u", pAppInfo->iAppId, pAppInfo->dwSeq);

			APPINFO_CHECK_COPY_INFO;

			iUseBufLen += SetWTlv((TWTlv*)(pRespTlvBody+iUseBufLen), TLV_MONI_CONFIG_ADD,
				iAppInfoLen, (const char*)(&stAppInfo));
			pRespTlvBody->bTlvNum++;
			iAddCount++;
			iTmpLen = 0;
		}

		if(iTmpLen > 0) // 缓冲区不足了
			break;

		if(pAppInfo->iNextIndex >= 0 && pAppInfo->iNextIndex < MAX_SLOG_APP_COUNT)
			pAppInfo = m_pAppInfo->stInfo+pAppInfo->iNextIndex;
		else
			pAppInfo = NULL;
	}

	return iTmpLen;
}

int CUdpSock::SetAppInfoCheck(ContentCheckAppInfo *pctinfo)
{
	static char sRespBuf[1200];
	memset(sRespBuf, 0, sizeof(sRespBuf));

	TPkgBody *pRespTlvBody = (TPkgBody*)sRespBuf;
	int iUseBufLen = MYSIZEOF(TPkgBody), i=0, j=0;

	pctinfo->wAppInfoCount = ntohs(pctinfo->wAppInfoCount);
	for(i=0; i < pctinfo->wAppInfoCount; i++) {
#define APP_INFO_CHECK_32_TO_LOCAL(field) \
		pctinfo->stAppList[i].field = ntohl(pctinfo->stAppList[i].field);
		APP_INFO_CHECK_32_TO_LOCAL(iAppId);
		APP_INFO_CHECK_32_TO_LOCAL(dwSeq);
		APP_INFO_CHECK_32_TO_LOCAL(dwCfgFlag);
#undef APP_INFO_CHECK_32_TO_LOCAL 
		CLEAR_BIT(pctinfo->stAppList[i].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED);
	}

	int iAddCount = 0, iModCount = 0, iDelCount = 0, iSameCount = 0, iTmpLen = 0;

	// 检查私有 app 配置情况
	if(stConfig.psysConfig->wAppInfoCount > 0)
		iTmpLen = InnerDealAppInfoCheck(stConfig.psysConfig->iAppInfoIndexStart, stConfig.psysConfig->wAppInfoCount, 
			pctinfo, iAddCount, iModCount, iSameCount, iUseBufLen, (int)MYSIZEOF(sRespBuf), pRespTlvBody);
	
	// 检查是否有被删除的 app, 通过检查处理标志是否设置实现
	uint32_t iAppIdDel = 0;
	for(j=0; iTmpLen <= 0 && j < pctinfo->wAppInfoCount; j++) 
	{
		if(IS_SET_BIT(pctinfo->stAppList[j].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED))
			continue;

		iTmpLen = MYSIZEOF(int32_t)+MYSIZEOF(TWTlv);
		if(iUseBufLen+iTmpLen > (int)MYSIZEOF(sRespBuf))
			break;

		iAppIdDel = htonl(pctinfo->stAppList[j].iAppId);
		iUseBufLen += SetWTlv((TWTlv*)((char*)pRespTlvBody+iUseBufLen),
			TLV_MONI_CONFIG_DEL, MYSIZEOF(iAppIdDel), (const char*)(&iAppIdDel));
		INFO_LOG("delete app info appid:%d", pctinfo->stAppList[j].iAppId);
		pRespTlvBody->bTlvNum++;
		iDelCount++;
		iTmpLen = 0;
	}

	if(iTmpLen > 0)
		WARN_LOG("need more space %d > %u", iUseBufLen+iTmpLen, MYSIZEOF(sRespBuf));

	if(m_pMtClient->bEnableEncryptData) {
		static char sContentBuf[2048+256];
		int iSigBufLen = ((iUseBufLen>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sContentBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sContentBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)pRespTlvBody, iUseBufLen,
			(uint8_t*)sContentBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sContentBuf, iSigBufLen);
	}
	else {
		InitCmdContent(pRespTlvBody, iUseBufLen);
	}

	INFO_LOG("check app config count add:%d mod:%d del:%d not change:%d", iAddCount, iModCount, iDelCount, iSameCount);
	return NO_ERROR;
}

int CUdpSock::DealCmdCheckAppInfo()
{
	if(m_wCmdContentLen < MYSIZEOF(ContentCheckAppInfo)) {
		REQERR_LOG("invalid cmd content %u < %u", m_wCmdContentLen, MYSIZEOF(ContentCheckAppInfo));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	ContentCheckAppInfo *pctinfo = (ContentCheckAppInfo*)m_pstCmdContent;
	if(m_wCmdContentLen != MYSIZEOF(ContentCheckAppInfo)
		+ MYSIZEOF(pctinfo->stAppList[0])*ntohs(pctinfo->wAppInfoCount)){
		REQERR_LOG("check cmd content failed %d != %u", m_wCmdContentLen,
			MYSIZEOF(ContentCheckAppInfo)+MYSIZEOF(uint32_t)*ntohs(pctinfo->wAppInfoCount));
		return ERR_INVALID_CMD_CONTENT;
	}

	DEBUG_LOG("get check app info, server response time:%u ms , app info count:%d",
		ntohl(pctinfo->dwServerResponseTime), ntohs(pctinfo->wAppInfoCount));
	if((int)ntohl(pctinfo->dwServerResponseTime) > m_pMtClient->iServerResponseTimeMs) {
		m_pMtClient->iServerResponseTimeMs = ntohl(pctinfo->dwServerResponseTime);
		INFO_LOG("update max server response time to:%u", m_pMtClient->iServerResponseTimeMs);
	}

	if((iRet=SetAppInfoCheck(pctinfo)) != NO_ERROR)
		return iRet;
	AckToReq(NO_ERROR); 
	return NO_ERROR;
}

int CUdpSock::SetSystemConfigCheck(ContentCheckSystemCfgReq *pctinfo)
{
	static char sRespBuf[1024];
	memset(sRespBuf, 0, sizeof(sRespBuf));

	MtSystemConfigClient *pResp = (MtSystemConfigClient*)sRespBuf;
	pResp->dwConfigSeq = htonl(stConfig.psysConfig->dwConfigSeq);

	// system config 
	if(stConfig.psysConfig->dwConfigSeq != ntohl(pctinfo->dwConfigSeq))
	{
		pResp->wHelloRetryTimes = htons(stConfig.psysConfig->wHelloRetryTimes);
		pResp->wHelloPerTimeSec = htons(stConfig.psysConfig->wHelloPerTimeSec);
		pResp->wCheckLogPerTimeSec = htons(stConfig.psysConfig->wCheckLogPerTimeSec);
		pResp->wCheckAppPerTimeSec = htons(stConfig.psysConfig->wCheckAppPerTimeSec);
		pResp->wCheckServerPerTimeSec = htons(stConfig.psysConfig->wCheckServerPerTimeSec);
		pResp->wCheckSysPerTimeSec = htons(stConfig.psysConfig->wCheckSysPerTimeSec);
		pResp->bAttrSendPerTimeSec = stConfig.psysConfig->bAttrSendPerTimeSec;
		pResp->bLogSendPerTimeSec = stConfig.psysConfig->bLogSendPerTimeSec;
		pResp->bReportCpuUseSec = stConfig.psysConfig->bReportCpuUseSec;
		INFO_LOG("system config change old seq:%u new seq:%u", 
			ntohl(pctinfo->dwConfigSeq), stConfig.psysConfig->dwConfigSeq);
	}
	else  {
		DEBUG_LOG("system config not change - seq:%u", stConfig.psysConfig->dwConfigSeq);
	}

	if(m_pMtClient->bEnableEncryptData) {
		static char sContentBuf[1024+256];
		int iSigBufLen = ((sizeof(*pResp)>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sContentBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sContentBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)pResp,
			sizeof(*pResp), (uint8_t*)sContentBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sContentBuf, iSigBufLen);
	}
	else {
		InitCmdContent(pResp, sizeof(*pResp));
	}
	return NO_ERROR;
}

int CUdpSock::DealCmdCheckSystemConfig()
{
	if(m_wCmdContentLen != MYSIZEOF(ContentCheckSystemCfgReq)) {
		REQERR_LOG("invalid cmd content %u < %u", m_wCmdContentLen, MYSIZEOF(ContentCheckSystemCfgReq));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	ContentCheckSystemCfgReq *pctinfo = (ContentCheckSystemCfgReq*)m_pstCmdContent;
	DEBUG_LOG("get check system config, seq:%u", ntohl(pctinfo->dwConfigSeq));

	if((int)ntohl(pctinfo->dwServerResponseTime) > m_pMtClient->iServerResponseTimeMs) {
		m_pMtClient->iServerResponseTimeMs = ntohl(pctinfo->dwServerResponseTime);
		INFO_LOG("update max server response time to:%u", m_pMtClient->iServerResponseTimeMs);
	}

	if((iRet=SetSystemConfigCheck(pctinfo)) != NO_ERROR)
		return iRet;

	AckToReq(NO_ERROR); 
	return NO_ERROR;
}

void CUdpSock::OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len)
{
	int iRet = 0;
	Init();
	if((iRet=CheckBasicPacket(buf, len, sa)) != NO_ERROR) {
		AckToReq(iRet);
		return;
	}

	if(PacketPb()) {

		return;
	}

	switch(m_dwReqCmd) {
		case CMD_MONI_SEND_HELLO_FIRST:
			// 这里加个版本限制，以方便以后升级
			if(ntohs(m_pstReqHead->wVersion) != 1) 
			{
				REQERR_LOG("check version failed ! (%d != 1)", ntohs(m_pstReqHead->wVersion));
				iRet = ERR_INVALID_PACKET;
				break;
			}
			m_bIsFirstHello = true;
			iRet = DealCmdHelloFirst();
			break;

		case CMD_MONI_SEND_HELLO:
			iRet = DealCmdHello();
			break;

		case CMD_MONI_CHECK_LOG_CONFIG:
			iRet = DealCmdCheckLogConfig();
			break;

		case CMD_MONI_CHECK_APP_CONFIG:
			iRet = DealCmdCheckAppInfo();
			break;

		case CMD_MONI_CHECK_SYSTEM_CONFIG:
			iRet = DealCmdCheckSystemConfig();
			break;

		default:
			REQERR_LOG("unknow cmd:%u", m_dwReqCmd);
			iRet = ERR_UNKNOW_CMD;
			break;
	}

	if(iRet != NO_ERROR)
		AckToReq(iRet);
}

#undef APPINFO_CHECK_COPY_INFO
#undef LOG_CHECK_COPY_CONFIG_INFO


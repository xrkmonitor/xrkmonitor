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

   模块 slog_deal_warn 功能:
        处理监控告警以及邮件发送

****/

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <errno.h>
#include <time.h>
#include <map>
#include <cstdlib>  
#include <Json.h>
#include <iostream>  
#include <aes.h>

#include "comm.pb.h"
#include "top_include_comm.h"
#include "sv_time.h"
#include "sv_str.h"
#include "Utility.h"
#include "sv_file.h"
#include "udp_sock.h"

#define CONFIG_FILE "./slog_deal_warn.conf"

CONFIG stConfig;
CSupperLog slog;

using namespace std;

int DealDbConnect();

int Init(const char *pFile = NULL)
{
    const char *pConfFile = NULL;
    if(pFile != NULL)
        pConfFile = pFile;
    else
        pConfFile = CONFIG_FILE;

    int32_t iRet = 0;
    if((iRet=LoadConfig(pConfFile,
        "LOCAL_IF_NAME", CFG_STRING, stConfig.szLocalIp, "eth0", MYSIZEOF(stConfig.szLocalIp),
		"VALID_SEND_WARN_TIME_SEC", CFG_INT, &stConfig.iValidSendWarnTimeSec, DEF_WARN_SEND_INFO_VALID_TIME_SEC, 
		"DEF_SEND_WARN_SHM_KEY", CFG_INT, &stConfig.iSendWarnShmKey, DEF_SEND_WARN_SHM_KEY, 
		"SKIP_SEND_WARN", CFG_INT, &stConfig.iSkipSendWarn, 0, 
		"EMAIL_USE_XRKMONITOR", CFG_INT, &stConfig.iEmailUseXrkmonitor, 1,
		// 云版本上的云账号ID 
		"XRKMONITOR_UID", CFG_INT, &stConfig.iXrkmonitorUid, 0,

		// 云版本云账号的数据上报 key，可在 账号中心 查看
		"XRKMONITOR_UKEY", CFG_STRING, stConfig.szXrkmonitorKey, "", MYSIZEOF(stConfig.szXrkmonitorKey),

		// 云版本服务地址，可以是域名或者 IP 地址
		"XRKMONITOR_OPEN_SRV", CFG_STRING, stConfig.szXrkmonitorSrv, "opensrv.xrkmonitor.com", MYSIZEOF(stConfig.szXrkmonitorSrv),
		// 云版本服务端口，一般不用修改
		"XRKMONITOR_OPEN_SRV_PORT", CFG_INT, &stConfig.iXrkmonitorSrvPort, 46000,

        (void*)NULL)) < 0)
    {   
        ERR_LOG("LoadConfig:%s failed ! ret:%d", pConfFile, iRet);
        return SLOG_ERROR_LINE;
    }   

    if(GetIpFromIf(stConfig.szLocalIp, stConfig.szLocalIp) != 0)
    {
        ERR_LOG("GetIpFromIf failed ! local if name:%s", stConfig.szLocalIp);
        return SLOG_ERROR_LINE;
    }

    if((iRet=slog.InitConfigByFile(pConfFile)) < 0)
    {
        ERR_LOG("slog InitConfigByFile failed file:%s ret:%d\n", pConfFile, iRet);
        return SLOG_ERROR_LINE;
    }

    if((iRet=slog.Init(stConfig.szLocalIp)) < 0)
    {
        ERR_LOG("slog init failed ret:%d\n", iRet);
        return SLOG_ERROR_LINE;
    }

	if(!(stConfig.pSendWarnShm=(TWarnSendInfo*)GetShm(stConfig.iSendWarnShmKey, 
		sizeof(TWarnSendInfo)*MAX_WARN_SEND_NODE_SHM, 0666|IPC_CREAT)))
	{
		ERR_LOG("init warn send shm failed, key:%d, size:%d", stConfig.iSendWarnShmKey,
			(int)sizeof(TWarnSendInfo)*MAX_WARN_SEND_NODE_SHM);
		return SLOG_ERROR_LINE;
	}

	if((stConfig.pShmMailInfo=slog.InitMailInfoShm(DEFAULT_MAILINFO_SHM_KEY, true)) == NULL)
	{
		ERR_LOG("InitMailInfoShm failed");
		return SLOG_ERROR_LINE;
	}

	stConfig.psysConfig = slog.GetSystemCfg();
	if(stConfig.psysConfig == NULL) 
	{
		ERR_LOG("GetSystemCfg failed");
		return SLOG_ERROR_LINE;
	}
    return 0;
}   

// 使用云版本提供的电子邮件发送接口发送邮件
// 注意：1、接收邮件的地址必须是在云版本上同账号关联过
//       2、免费账号有发送次数限制，限制发送次数同告警次数
int SendEmailByXrkmonitor(const string & strToAddr, const string & strSubject, const string & strTxtBody)
{
	static uint32_t s_dwSeq = rand();
	static char s_EncBodyBuf[512];

	// warn head
	::comm::PkgHead head;
	head.set_en_cmd(::comm::CMD_SLOG_OPEN_SEND_EMAIL);
	head.set_uint32_seq(s_dwSeq);
	head.set_uid(stConfig.iXrkmonitorUid);

	// warn info
	::comm::SendEmailInfo info;
	info.set_addr(strToAddr);
	info.set_subject(strSubject);
	info.set_text_body(strTxtBody);

	std::string strHead, strBody;
	if(!head.AppendToString(&strHead) || !info.AppendToString(&strBody))
	{
		ERR_LOG("protobuf AppendToString failed msg:%s", strerror(errno));
		return SLOG_ERROR_LINE;
	}

	// body 部分加密
	int iEncBodyLen = ((strBody.size()>>4)+1)<<4;
	if(iEncBodyLen > (int)sizeof(s_EncBodyBuf)) 
	{
		ERR_LOG("need more space, %d > %d", iEncBodyLen, (int)sizeof(s_EncBodyBuf));
		return SLOG_ERROR_LINE;
	}
	aes_cipher_data((const uint8_t*)strBody.c_str(), strBody.size(), 
		(uint8_t*)s_EncBodyBuf, (const uint8_t*)stConfig.szXrkmonitorKey, AES_256);
	strBody.assign(s_EncBodyBuf, iEncBodyLen);

	int iret = stConfig.pUdp->SendPacketPb(
		s_dwSeq++, strHead, strBody, stConfig.szXrkmonitorSrv, stConfig.iXrkmonitorSrvPort);
	if(iret > 0) {
		DEBUG_LOG("send email info to xrkmonitor(%s:%d), packet len:%d, enc body len:%d, info:%s",
			stConfig.szXrkmonitorSrv, stConfig.iXrkmonitorSrvPort,
			iret, iEncBodyLen, info.ShortDebugString().c_str());
	}
	else {
		WARN_LOG("send email info failed, ret:%d, info:%s", iret, info.ShortDebugString().c_str());
	}
	return 0;
}

// 电子邮件发送接口 
// 参数：strToAddr -- 邮件接收方地址
//		 strSubject -- 邮件标题
//		 strTxtBody -- 邮件内容
int SendEmail(const string & strToAddr, const string & strSubject, const string & strTxtBody)
{
	if(stConfig.iXrkmonitorUid != 0 && stConfig.szXrkmonitorKey[0] != '\0' && stConfig.iEmailUseXrkmonitor)
		return SendEmailByXrkmonitor(strToAddr, strSubject, strTxtBody);

	// 用户自行实现发送邮件接口

	return 0;
}

void GetSendWarnEmailAddrs(
	const TWarnSendInfo *pSendNode, list<string> & listAddrs)
{
	char sSql[128] = {0};
	snprintf(sSql , sizeof(sSql), "select email from flogin_user where status=0");
	if(stConfig.qu->get_result(sSql)) {
		while(stConfig.qu->fetch_row())
		{
			const char *paddr = stConfig.qu->getstr("email");
			if(paddr != NULL && paddr[0] != '\0') {
				listAddrs.push_back(paddr);
			}
		}
	}
	stConfig.qu->free_result();
}

int GetUserEmail(uint32_t uid, std::string &strMail)
{
	char sSql[256] = {0};
	snprintf(sSql, sizeof(sSql)-1, "select email from flogin_user where user_id=%u and status=0", uid);
	if(stConfig.qu->get_result(sSql) && stConfig.qu->fetch_row()) 
	{
		const char *ptmp = stConfig.qu->getstr("email");
		if(ptmp != NULL)
		{
			strMail = ptmp;
			DEBUG_LOG("get user:%u email:%s from db", uid, ptmp);
		}
		stConfig.qu->free_result();
		return 0;
	}

	stConfig.qu->free_result();
	return SLOG_ERROR_LINE;
}

void GetSendWarnTxt(TWarnSendInfo *pSendNode, string & strWarnTxt)
{
	if(pSendNode->stWarn.iWarnFlag & ATTR_WARN_FLAG_TYPE_VIEW)
	{
		strWarnTxt = "视图: [";
		strWarnTxt += itoa(pSendNode->stWarn.iWarnId);
		strWarnTxt += "] ";
		strWarnTxt += slog.GetViewName(pSendNode->stWarn.iWarnId);
	}
	else
	{
		strWarnTxt = "服务器: [";
		strWarnTxt += itoa(pSendNode->stWarn.iWarnId);
		strWarnTxt += "] ";

		MachineInfo *pMachinfo = slog.GetMachineInfo(pSendNode->stWarn.iWarnId, NULL);
		if(pMachinfo != NULL) {
			strWarnTxt += ipv4_addr_str(pMachinfo->ip1);
			strWarnTxt += "/";
			strWarnTxt += slog.GetMachineName(pSendNode->stWarn.iWarnId);
		}
	}
	strWarnTxt += " ; 产生";

	if(pSendNode->stWarn.iWarnFlag & 1)
		strWarnTxt += " 最大值告警";
	else if(pSendNode->stWarn.iWarnFlag & 2)
		strWarnTxt += " 最小值告警";
	else if(pSendNode->stWarn.iWarnFlag & 4)
		strWarnTxt += " 波动值告警";
	else 
		strWarnTxt += " 异常属性告警";

	strWarnTxt += "; 告警属性: [";
	strWarnTxt += itoa(pSendNode->stWarn.iAttrId);
	strWarnTxt += "] ";
	strWarnTxt += slog.GetAttrNameFromShm(pSendNode->stWarn.iAttrId);

	if(pSendNode->stWarn.iWarnFlag & 7)
	{
		strWarnTxt += "; 上报值:";
		strWarnTxt += uitoa(pSendNode->stWarn.dwWarnValue);
		if(pSendNode->stWarn.iWarnFlag & 2)
		{
			strWarnTxt += " 低于配置值: ";
			strWarnTxt += uitoa(pSendNode->stWarn.iWarnConfigValue);
		}
		else
		{
			strWarnTxt += " 超过配置值: ";
			strWarnTxt += uitoa(pSendNode->stWarn.iWarnConfigValue);
		}
	}

	DEBUG_LOG("get email warn txt, flag:%d, warn id:%d, attr id:%d, value:%u, config val:%d, txt:%s",
		pSendNode->stWarn.iWarnFlag, pSendNode->stWarn.iWarnId, pSendNode->stWarn.iAttrId,
		pSendNode->stWarn.dwWarnValue, pSendNode->stWarn.iWarnConfigValue, strWarnTxt.c_str());
}

int GetTimePeriod()
{
	struct tm stTm;
	localtime_r(&slog.m_stNow.tv_sec, &stTm);
	return stTm.tm_hour*60 + stTm.tm_min;
}

void SendWarnByXrkmonitor(TWarnSendInfo *pSendNode, int32_t iTimePeriod, uint32_t dwTimeNow)
{
	static uint32_t s_dwSeq = rand();
	static char s_EncBodyBuf[512];

	// warn head
	::comm::PkgHead head;
	head.set_en_cmd(::comm::CMD_SLOG_OPEN_SEND_WARN);
	head.set_uint32_seq(s_dwSeq);
	head.set_uid(stConfig.iXrkmonitorUid);

	// warn info
	::comm::SendWarnInfo info;
	info.set_warn_id(pSendNode->dwWarnId);
	info.set_attr_id(pSendNode->stWarn.iAttrId);
	info.set_attr_name(slog.GetAttrNameFromShm(pSendNode->stWarn.iAttrId));
	info.set_start_time(pSendNode->dwWarnAddTime);
	info.set_warn_flag(pSendNode->stWarn.iWarnFlag);
	info.set_warn_obj_type_id(pSendNode->stWarn.iWarnId);
	if(pSendNode->stWarn.iWarnFlag & ATTR_WARN_FLAG_TYPE_VIEW)
		info.set_warn_obj_type_name(slog.GetViewName(pSendNode->stWarn.iWarnId));
	else {
		std::string str;
		MachineInfo *pMachinfo = slog.GetMachineInfo(pSendNode->stWarn.iWarnId, NULL);
		if(pMachinfo != NULL) {
			str = ipv4_addr_str(pMachinfo->ip1);
			str += "/";
			str += slog.GetMachineName(pSendNode->stWarn.iWarnId);
		}
		else 
			str = "unknow";
		info.set_warn_obj_type_name(str);
	}
	std::string strInfo("当前上报值【");
	strInfo += uitoa(pSendNode->stWarn.dwWarnValue);
	strInfo += "】";
	if(pSendNode->stWarn.iWarnFlag & 2)
	    strInfo += " 低于配置值【";
	else
	    strInfo += " 超过配置值【";
	strInfo += uitoa(pSendNode->stWarn.iWarnConfigValue);
	strInfo += "】";
	info.set_warn_text(strInfo);

	std::string strHead, strBody;
	if(!head.AppendToString(&strHead) || !info.AppendToString(&strBody))
	{
		ERR_LOG("protobuf AppendToString failed msg:%s", strerror(errno));
		return;
	}

	// body 部分加密
	int iEncBodyLen = ((strBody.size()>>4)+1)<<4;
	if(iEncBodyLen > (int)sizeof(s_EncBodyBuf)) 
	{
		ERR_LOG("need more space, %d > %d", iEncBodyLen, (int)sizeof(s_EncBodyBuf));
		return;
	}
	aes_cipher_data((const uint8_t*)strBody.c_str(), strBody.size(), 
		(uint8_t*)s_EncBodyBuf, (const uint8_t*)stConfig.szXrkmonitorKey, AES_256);
	strBody.assign(s_EncBodyBuf, iEncBodyLen);

	int iret = stConfig.pUdp->SendPacketPb(
		s_dwSeq++, strHead, strBody, stConfig.szXrkmonitorSrv, stConfig.iXrkmonitorSrvPort);
	if(iret > 0) {
		DEBUG_LOG("send warn info to xrkmonitor(%s:%d), packet len:%d, enc body len:%d, info:%s",
			stConfig.szXrkmonitorSrv, stConfig.iXrkmonitorSrvPort,
			iret, iEncBodyLen, info.ShortDebugString().c_str());
	}
	else {
		WARN_LOG("send warn info failed, ret:%d, info:%s", iret, info.ShortDebugString().c_str());
	}
}

// 本地告警，以邮件通知作为示例
void SendWarn(TWarnSendInfo *pSendNode, int32_t iTimePeriod, uint32_t dwTimeNow)
{
	static const string strEmailWarnSubject("监控告警");
	string strWarnTxt;
	GetSendWarnTxt(pSendNode, strWarnTxt);
	int iSendMailCount = 0;

	list<string> listEmailAddr;
	GetSendWarnEmailAddrs(pSendNode, listEmailAddr);
	list<string>::iterator it = listEmailAddr.begin();
	for(; it != listEmailAddr.end(); it++)
	{
		if(SendEmail(*it, strEmailWarnSubject, strWarnTxt) != 0)
		{
			WARN_LOG("send email warn failed, info:%s, %s", (*it).c_str(), strWarnTxt.c_str());
		}
		else
		{
			iSendMailCount++;
			DEBUG_LOG("send email warn ok info|%s|%s", (*it).c_str(), strWarnTxt.c_str());
		}
	}
	INFO_LOG("get warn info - sendmail:%d, warn info:%s", iSendMailCount, strWarnTxt.c_str());
}

int DealDbConnect()
{
	static uint32_t s_dwLastCheckDbTime = 0;

	// 5 - 10 秒 check 一次
	if(stConfig.dwCurrentTime <= s_dwLastCheckDbTime)
		return 0;
	s_dwLastCheckDbTime = stConfig.dwCurrentTime+5+slog.m_iRand%5;

	if(stConfig.db != NULL && stConfig.qu != NULL) {
		if(stConfig.qu->Connected())
			return 0;
		delete stConfig.db;
		stConfig.db = NULL;
		delete stConfig.qu;
		stConfig.qu = NULL;
	}

	stConfig.db = new Database(stConfig.psysConfig->szDbHost, stConfig.psysConfig->szUserName,
		stConfig.psysConfig->szPass, stConfig.psysConfig->szDbName, &slog);
	stConfig.qu = new Query(*stConfig.db);
	if(stConfig.db && stConfig.qu && stConfig.qu->Connected()) {
		INFO_LOG("connect to db:%s ok dbname:%s", stConfig.psysConfig->szDbHost, stConfig.psysConfig->szDbName);
		return 0;
	}
	ERR_LOG("connect to database failed ! dbhost:%s, dbname:%s", stConfig.psysConfig->szDbHost, stConfig.psysConfig->szDbName);
	delete stConfig.db;
	stConfig.db = NULL;
	delete stConfig.qu;
	stConfig.qu = NULL;
	return SLOG_ERROR_LINE;
}

void ScanSendWarnShm()
{
	TWarnSendInfo *pSendNode = NULL;
	int j=0;
	uint32_t dwTimeNow = stConfig.dwCurrentTime;
	int32_t iTimePeriod = GetTimePeriod();

	for(j=0; j < MAX_WARN_SEND_NODE_SHM; j++)
	{
		pSendNode = stConfig.pSendWarnShm+j;
		if(0 == pSendNode->dwWarnId || pSendNode->dwWarnAddTime+stConfig.iValidSendWarnTimeSec <= dwTimeNow)
		{
			if(pSendNode->dwWarnId != 0 && pSendNode->dwWarnAddTime+stConfig.iValidSendWarnTimeSec <= dwTimeNow)
				WARN_LOG("warn :%d send timeout", pSendNode->dwWarnId);
			continue;
		}
		 
		if((int)(pSendNode->dwWarnId % slog.m_iProcessCount) != slog.m_iProcessId)
			continue;

		if(!stConfig.iSkipSendWarn)
		{
			if(stConfig.iXrkmonitorUid != 0 && stConfig.szXrkmonitorKey[0] != '\0')
			{
				// 云版本告警发送，接收方为云账号相关的邮箱、手机、微信号等
				SendWarnByXrkmonitor(pSendNode, iTimePeriod, dwTimeNow);
			}
			else
			{
				// 本地告警发送
				SendWarn(pSendNode, iTimePeriod, dwTimeNow);
			}
		}
		else
		{
			DEBUG_LOG("skip send warn, flag:%d, warn id:%u|%d, attr id:%d, value:%u, config val:%d",
				pSendNode->stWarn.iWarnFlag, pSendNode->dwWarnId, pSendNode->stWarn.iWarnId, 
				pSendNode->stWarn.iAttrId, pSendNode->stWarn.dwWarnValue, pSendNode->stWarn.iWarnConfigValue);
		}
		memset(pSendNode, 0, sizeof(TWarnSendInfo));
	}
}

void ScanShmMailInfo()
{
	TCommSendMailInfo stInfo;
	int i=0;
	uint32_t dwCurTime = time(NULL);
	uint32_t dwModifySeq = 0;
	for(; i < MAX_COMM_SEND_MAIL_NODE_COUNT; i++)
	{
		dwModifySeq = stConfig.pShmMailInfo->stInfo[i].dwModifySeq;

		if(stConfig.pShmMailInfo->stInfo[i].dwMailSeq == 0)
			continue;
		if((int)(stConfig.pShmMailInfo->stInfo[i].dwMailSeq % slog.m_iProcessCount) != slog.m_iProcessId)
			continue;
		if(stConfig.pShmMailInfo->stInfo[i].dwValidTimeUtc < dwCurTime)
		{
			WARN_LOG("email time check failed:%u < %u, uid:%u, mail:%s",
				stConfig.pShmMailInfo->stInfo[i].dwValidTimeUtc, dwCurTime, 
				stConfig.pShmMailInfo->stInfo[i].dwToUserId, stConfig.pShmMailInfo->stInfo[i].szEmailContent);
			// 异步多进程数据修改 --
			if(slog.InitGetMailShmLock(stConfig.pShmMailInfo) < 0)
				return;
			if(dwModifySeq == stConfig.pShmMailInfo->stInfo[i].dwModifySeq) {
				stConfig.pShmMailInfo->stInfo[i].dwMailSeq = 0;
				stConfig.pShmMailInfo->stInfo[i].dwModifySeq++;
			}
			slog.EndGetMailShmLock(stConfig.pShmMailInfo);
			MtReport_Attr_Add(223, 1);
			continue;
		}

		if(slog.InitGetMailShmLock(stConfig.pShmMailInfo) < 0)
			return;
		if(dwModifySeq == stConfig.pShmMailInfo->stInfo[i].dwModifySeq) {
			memcpy(&stInfo, stConfig.pShmMailInfo->stInfo+i, sizeof(stInfo));
			stConfig.pShmMailInfo->stInfo[i].dwMailSeq = 0;
			stConfig.pShmMailInfo->stInfo[i].dwModifySeq++;
		}
		slog.EndGetMailShmLock(stConfig.pShmMailInfo);
		break;
	}

	if(i < MAX_COMM_SEND_MAIL_NODE_COUNT)
	{
		std::string strMail;
		if(stInfo.szToEmailAddr[0] != '\0')
			strMail = stInfo.szToEmailAddr;
		else if(stInfo.dwToUserId == 0 || GetUserEmail(stInfo.dwToUserId, strMail) < 0)
		{
			WARN_LOG("send email failed info, not find email address, user:%d, mailseq:%u", 
				stInfo.dwToUserId, stInfo.dwMailSeq);
			return;
		}

		std::string strSubject(stInfo.szEmailSubject);
		std::string strTxt(stInfo.szEmailContent);
		SendEmail(strMail, strSubject, strTxt);

		DEBUG_LOG("send email info to user:%u, email:%s, subject:%s, txt:%s", 
			stInfo.dwToUserId, strMail.c_str(), strSubject.c_str(), strTxt.c_str());
	}
}

int main(int argc, char** argv)  
{
    int iRet = 0;
    if((iRet=Init(NULL)) < 0)
    {
        ERR_LOG("Init Failed ret:%d !", iRet);
        MtReport_Attr_Add(55, 1);
        return SLOG_ERROR_LINE;
    }   
    
    INFO_LOG("slog_mail start !");
    slog.Daemon(1, 1, 0);

	SocketHandler h(&slog);
	CUdpSock stSock(h);
	h.Add(&stSock);
	stConfig.pUdp = &stSock;
    while(h.GetCount() && slog.Run()) 
    {
		h.Select(1, 10000);
		stConfig.dwCurrentTime = slog.m_stNow.tv_sec;
		if((iRet=DealDbConnect()) < 0)
			break;
		ScanSendWarnShm();
		ScanShmMailInfo();
    }   
    INFO_LOG("slog_mail exit !");
    return 0;
} 


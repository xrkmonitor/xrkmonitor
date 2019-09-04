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

   模块 slog_mtreport_client 功能:
        用于上报除监控系统本身产生的监控点数据、日志，为减少部署上的依赖
		未引入任何第三方组件

****/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mtreport_client.h"
#include "mtreport_protoc.h"
#include "sv_struct.h"

void CBasicPacket::InitReqPkgHead(ReqPkgHead *pHead, uint32_t cmd)
{
	memset(pHead, 0, MYSIZEOF(ReqPkgHead));
	pHead->dwCmd = htonl(cmd);
	pHead->wVersion = htons(REQ_PKG_HEAD_VERSION);
	m_dwReqCmd = cmd;
	if(stConfig.pReportShm->dwPkgSeq == 0) {
		pHead->dwSeq = htonl(1);
		m_dwReqSeq = 1;
		stConfig.pReportShm->dwPkgSeq=2;
	}
	else {
		pHead->dwSeq = htonl(stConfig.pReportShm->dwPkgSeq);
		m_dwReqSeq = stConfig.pReportShm->dwPkgSeq;
		stConfig.pReportShm->dwPkgSeq++;
	}
	m_pstReqHead = pHead;
}

void CBasicPacket::InitSignature(TSignature *psig)
{
	m_pstRespSig = psig;
	m_wRespSigLen = MYSIZEOF(TSignature)+psig->wSigLen;
	psig->wSigLen = ntohs(psig->wSigLen);
}

void CBasicPacket::InitCmdContent(void *pCmdCt, uint16_t wCmdCtLen)
{
	m_pstRespCmdContent = pCmdCt;
	m_wRespCmdContentLen = wCmdCtLen;
}

void CBasicPacket::InitPkgBody(TPkgBody *pbody, uint16_t wBodyLen)
{
	m_pstRespBody = pbody;
	m_wRespBodyLen = wBodyLen;
}

const char * CBasicPacket::GetErrMsg(int32_t err)
{
	switch(err) {
		case NO_ERROR:
			return "no error";
		case ERR_SERVER:
			return "error server (bug) !";
		case ERR_INVALID_PACKET:
			return "error invalid packet !";
		case ERR_INVALID_PACKET_LEN:
			return "error invalid packet length !";
		case ERR_UNKNOW_CMD:
			return "error unknow cmd !";
		case ERR_CHECK_TLV:
			return "error check tlv !";
		case ERR_CHECK_SIGNATUR:
			return "error check signature failed";
		case ERR_INVALID_TLV_VALUE:
			return "error invalid tlv value !";
		case ERR_FIND_TLV:
			return "find tlv failed !";
		case ERR_RESP_SIZE_OVER_LIMIT:
			return "response size over limit !";
		case ERR_HTTP_KEEP_NOT_FIND:
			return "http keep socket not find !";
		case ERR_DECRYPT_FAILED:
			return "decrypt failed !";
		case ERR_FIND_SESSION_INFO:
			return "find session failed !";
		case ERR_INVALID_CMD_CONTENT:
			return "invalid cmd content !";
		case ERR_CHECK_DISTRIBUTE:
			return "check distribute failed !";
		case ERR_CHECK_BODY_MD5:
			return "check body md5 failed !";
		default:
			return "unknow error !";
	}
}

void CBasicPacket::ResetPacketInfo()
{
	m_pReqPkg = NULL;
	m_dwReqPkgLen = 0;
	m_pstReqHead = NULL;

	m_pstSig = NULL;
	m_pstRespSig = NULL;
	m_wRespSigLen = 0;

	m_pstCmdContent = NULL;
	m_wCmdContentLen = 0;
	m_pstRespCmdContent = NULL;
	m_wRespCmdContentLen = 0;

	m_pstBody = NULL;
	m_wBodyLen = 0;
	m_pstRespBody = NULL;
	m_wRespBodyLen = 0;

	m_dwReqCmd = 0;
	m_dwReqSeq = 0;
	m_bRetCode = 0;
}

int32_t CBasicPacket::CheckBasicPacket(const char *pPkg, int iPkgLen)
{
	ResetPacketInfo();

	const char *buf = pPkg;
	unsigned len = iPkgLen;

	m_pReqPkg = buf;
	m_dwReqPkgLen = len;

	if(len <= 2+MYSIZEOF(ReqPkgHead) 
		|| ntohs(((ReqPkgHead*)(1+buf))->wPkgLen) != len
		|| len > PKG_BUFF_LENGTH 
		|| buf[0] != SPKG || buf[len-1] != EPKG)
	{
		REQERR_LOG("invalid len pkglen:%u, pkglenInHeader:%u %c,%c",
			len, ntohs(((ReqPkgHead*)(1+buf))->wPkgLen), buf[0], buf[len-1]);
		return ERR_INVALID_PACKET;
	}

	m_pstReqHead = (ReqPkgHead*)(buf+1);
	uint16_t wToPkgBody = ntohs(m_pstReqHead->wToPkgBody);
	m_dwReqCmd = ntohl(m_pstReqHead->dwCmd);
	m_dwReqSeq = ntohl(m_pstReqHead->dwSeq);

	uint16_t wRemainLen = len;
	wRemainLen -= 2+MYSIZEOF(ReqPkgHead);
	m_pstSig = (TSignature*)(1+MYSIZEOF(ReqPkgHead)+buf);
	if(wRemainLen < MYSIZEOF(TSignature) || wRemainLen < MYSIZEOF(TSignature)+ntohs(m_pstSig->wSigLen))
	{
		REQERR_LOG("invalid signature len:%u < (%u or %u)",
			wRemainLen, MYSIZEOF(TSignature), MYSIZEOF(TSignature)+ntohs(m_pstSig->wSigLen));
		return ERR_INVALID_PACKET;
	}

	uint16_t wBasicLen = 1+MYSIZEOF(ReqPkgHead)+MYSIZEOF(TSignature)+ntohs(m_pstSig->wSigLen);
	if(wToPkgBody < wBasicLen || wToPkgBody+MYSIZEOF(TPkgBody) > len)
	{
		REQERR_LOG("invalid tobodylen:%u pkgLen:%u basiclen:%u", wToPkgBody, len, wBasicLen);
		return ERR_INVALID_PACKET;
	}

	// 业务相关部分提取
	if(wToPkgBody > wBasicLen)
	{
		m_wCmdContentLen = wToPkgBody-wBasicLen;
		m_pstCmdContent = (void*)(buf+wBasicLen);
	}
	else
	{
		m_wCmdContentLen = 0;
		m_pstCmdContent = NULL;
	}

	m_pstBody = (TPkgBody*)(buf+wToPkgBody);
	m_wBodyLen = len-wToPkgBody-1;
	if(CheckPkgBody(m_pstBody, len-wToPkgBody-1) < 0)
	{
		REQERR_LOG("tobodylen:%u tlvnum:%d invalid tlv", wToPkgBody, m_pstBody->bTlvNum);
		return ERR_CHECK_TLV;
	}

	// 是否是错误的响应包
	m_bRetCode = m_pstReqHead->bRetCode;
	if(m_bRetCode != NO_ERROR) {
		TWTlv *ptlv = (TWTlv*)GetWTlvByType2(TLV_ACK_ERROR, m_pstBody);
		if(ptlv != NULL)
		{
			uint16_t wErrLen = ntohs(ptlv->wLen);
			char *pszErrMsg = (char*)(ptlv->sValue);
			if(pszErrMsg[wErrLen-1] != '\0')
				pszErrMsg[wErrLen-1] = '\0';
			REQERR_LOG("response failed, ret:%d msg:%s", m_bRetCode, pszErrMsg);
		}
		else
			WARN_LOG("return failed ret:%d have no error msg !", m_bRetCode);
	}
	return NO_ERROR;
}

uint32_t CBasicPacket::MakeRespPkg(int iRetCode, char *pBuf, int *piBufLen)
{
	if(iRetCode != NO_ERROR)
		return MakeErrorRespPkg(iRetCode, pBuf, piBufLen);
	return MakePacket(pBuf, piBufLen, iRetCode, false);
}

uint32_t CBasicPacket::MakeErrorRespPkg(int iErrCode, char *pBuf, int *piBufLen)
{
	char sTmpBuf[128] = {0};
	m_pstRespBody = (TPkgBody*)sTmpBuf;
	m_wRespBodyLen = MYSIZEOF(TPkgBody);
	m_wRespBodyLen += SetWTlv(
		m_pstRespBody->stTlv, TLV_ACK_ERROR, strlen(GetErrMsg(iErrCode)), GetErrMsg(iErrCode));
	m_pstRespBody->bTlvNum = 1;
	return MakePacket(pBuf, piBufLen, iErrCode, false);
}

uint32_t CBasicPacket::MakePacket(char *pBuf, int *piBufLen, int iRetCode, bool bIsReqPkg)
{
	if(*piBufLen < (int)(2+MYSIZEOF(ReqPkgHead)+MYSIZEOF(TSignature)+MYSIZEOF(TPkgBody)
			+m_wRespSigLen+m_wRespCmdContentLen+m_wRespBodyLen)){
		ERROR_LOG("invalid buf len:%d < %d",
			*piBufLen, (int)(2+MYSIZEOF(ReqPkgHead)+MYSIZEOF(TSignature)+MYSIZEOF(TPkgBody)+
				+m_wRespSigLen+m_wRespCmdContentLen+m_wRespBodyLen));
		return 0;
	}
	char *pdata = pBuf;

	*pdata = SPKG;
	pdata++;

	ReqPkgHead *prespHead = (ReqPkgHead*)pdata;
	memcpy(pdata, m_pstReqHead, MYSIZEOF(ReqPkgHead));
	prespHead->bRetCode = (uint8_t)iRetCode;
	pdata += MYSIZEOF(ReqPkgHead);

	if(m_wRespSigLen > 0) {
		memcpy(pdata, m_pstRespSig, m_wRespSigLen);
		pdata += m_wRespSigLen;
	}
	else  {
		memset(pdata, 0, MYSIZEOF(TSignature));
		pdata += MYSIZEOF(TSignature); // empty signature
	}

	if(m_wRespCmdContentLen > 0) {
		memcpy(pdata, m_pstRespCmdContent, m_wRespCmdContentLen);
		pdata += m_wRespCmdContentLen;
	}
	prespHead->wToPkgBody = htons(pdata-pBuf);

	if(m_wRespBodyLen > 0) {
		memcpy(pdata, m_pstRespBody, m_wRespBodyLen);
		pdata += m_wRespBodyLen;
	}
	else {
		memset(pdata, 0, MYSIZEOF(TPkgBody));
		pdata += MYSIZEOF(TPkgBody); // empty body
	}

	*pdata = EPKG;
	pdata++;
	prespHead->wPkgLen = htons(pdata-pBuf);
	*piBufLen = pdata-pBuf;
	return m_dwReqSeq;
}


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


#include <errno.h>
#include "mt_report.h"
#include "basic_packet.h"

int CBasicPacket::SetPacketPb(std::string &strHead, std::string &strBody, char **ppack, int ibufLen)
{
	static char sBuf[8192];
	static char *pbuf = NULL;

	if(ibufLen <=0)
	{
		ibufLen = sizeof(sBuf);
		pbuf = sBuf;
		*ppack = sBuf;
	}
	else
		pbuf = *ppack;

	if((int)(strHead.size()+strBody.size()+10) > ibufLen) 
	{
		ERR_LOG("packet too long over (%u > %d)", (uint32_t)(strHead.size()+strBody.size()+10), ibufLen);
		return SLOG_ERROR_LINE;
	}

	char *pdata = pbuf; 
	*pdata = SPKG_PB;
	pdata++;

	int32_t iHeadLen = strHead.size();
	*(int32_t*)pdata = htonl(iHeadLen);
	pdata += sizeof(int32_t);
	memcpy(pdata, strHead.c_str(), strHead.size());
	pdata += strHead.size();

	int32_t iBodyLen = strBody.size();
	*(int32_t*)pdata = htonl(iBodyLen);
	pdata += sizeof(int32_t);
	memcpy(pdata, strBody.c_str(), strBody.size());
	pdata += strBody.size();

	*pdata = EPKG_PB;
	pdata++;
	DEBUG_LOG("set pb packet head len:%d body len:%d packet len:%d", iHeadLen, iBodyLen, (int)(pdata-pbuf));
	return pdata-pbuf;
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
		case ERR_UNKNOW_SIGTYPE:
			return "unknow signature type !";
		case ERR_NO_USER_MASTER:
			return "no user master find !";
		case ERR_DECRYPT_SIGNATURE:
			return "decrypt signature failed !";
		case ERR_INVALID_SIGNATURE_INFO:
			return "invalid signature decrypt info !";
		case ERR_CHECK_PACKET_CRC:
			return "check packet crc failed !";
		case ERR_CHECK_MACHINE_ACCESS_RIGHT:
			return "machine have no access right !";
		case ERR_NO_RESPONSE_CONTENT:
			return "have no response content !";
		case ERR_INVALID_APPID:
			return "invalid appid, not find !";
		case ERR_NOT_FIND_MACHINE:
			return "not find report machine !";
		case ERR_APP_LOG_DISPATCH_INVALID:
			return "dispatch log server invalid !";
		case ERR_ATTR_DISPATCH_INVALID:
			return "dispatch attr server invalid !";
		case ERR_INVALID_MAGIC:
			return "invalid magic, please change !";
		case ERR_LOG_FREQ_OVER_LIMIT:
			return "log freq over limit !";
		default:
			return "unknow error !";
	}
}

bool CBasicPacket::IsHttpKeepAliveReq()
{
	return (CMD_HTTP_KEEP_REQ == m_dwReqCmd || CMD_TOP_HTTP_STREAM_REQ == m_dwReqCmd);
}

bool CBasicPacket::IsHttpKeepNextReq()
{
	return CMD_TOP_HTTP_STREAM_DISCONN == m_dwReqCmd;
}

void CBasicPacket::InitReqPkgHead(ReqPkgHead *pHead, uint32_t cmd, uint32_t dwSeq)
{
    memset(pHead, 0, sizeof(ReqPkgHead));
    pHead->dwCmd = htonl(cmd);
    m_dwReqCmd = cmd;
    pHead->dwSeq = htonl(dwSeq);
	m_dwReqSeq = dwSeq;
	pHead->wVersion = htons(REQ_PKG_HEAD_VERSION);
    m_pstReqHead = pHead;
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
	m_iCommLen = 0;
	m_bRetCode = 0;
	m_bIsPbReq = false;
	m_bIsResponse = false;
}

int32_t CBasicPacket::MakeErrAckPacket(
	const char *reqPkg, size_t reqLen, char *pBuf, int32_t iBufLen, uint8_t bErrCode)
{
	return 0;
}

// pb packet: stx + dwHeadLen + sHead + dwbodyLen + sBody + etx
bool CBasicPacket::IsPacketPb(const char *buf, size_t len)
{
	if(len <= 10 || buf[0] != SPKG_PB || buf[len-1] != EPKG_PB)
	{
		return false;
	}

	m_iPbHeadLen = ntohl(*(int32_t*)(buf+1));
	if(m_iPbHeadLen+2+4 > (int)len)
	{
		m_iPbHeadLen = 0;
		m_bIsPbReq = false;
		return false;
	}

	m_iPbBodyLen = ntohl(*(int32_t*)(buf+1+4+m_iPbHeadLen));
	if(m_iPbHeadLen+m_iPbBodyLen+10 != (int)len)
	{
		m_iPbHeadLen = 0;
		m_iPbBodyLen = 0;
		m_bIsPbReq = false;
		return false;
	}
	m_bIsPbReq = true;
	return true;
}

int32_t CBasicPacket::CheckBasicPacketPb(const char *pPkg, size_t iPkgLen, struct sockaddr *sa, bool bUseSelfBuf)
{
	m_bIsPbReq = true;

	const char *buf = pPkg;
	size_t len = iPkgLen;
	if(bUseSelfBuf) {
		if(iPkgLen > sizeof(m_sCommBuf)) {
			REQERR_LOG( "invalid packent len:%lu > %lu", iPkgLen, sizeof(m_sCommBuf));
			m_pReqPkg = pPkg;
			m_dwReqPkgLen = len;
			return ERR_INVALID_PACKET_LEN;
		}
		memcpy(m_sCommBuf, buf, len);
		buf = m_sCommBuf;
		m_iCommLen = (int)len;
	}

	m_bParsePbHead = false;
	m_pReqPkg = buf;
	m_dwReqPkgLen = len;

	if(sa != NULL)
		m_addrRemote.SetAddress(sa);

	if(len <= 10 || buf[0] != SPKG_PB || buf[len-1] != EPKG_PB)
	{
		REQERR_LOG("invalid len pkglen:%lu, STX:%c,%c",
			len, buf[0], buf[len-1]);
		return ERR_INVALID_PACKET;
	}

	if(m_iPbHeadLen <= 0 || !m_pbHead.ParseFromArray(buf+5, m_iPbHeadLen))
	{
		REQERR_LOG("invalid len (headlen:%d pkglen:%lu) or Unserialize failed !", (int)m_iPbHeadLen, len);
		return ERR_INVALID_PACKET_LEN;
	}

	m_bParsePbHead = true;
	m_dwReqCmd = m_pbHead.en_cmd();
	m_dwReqSeq = m_pbHead.uint32_seq();
	m_bRetCode = m_pbHead.uint32_result();
	if(m_bRetCode != NO_ERROR)
	{
		REQERR_LOG("response failed, ret:%d msg:%s", m_bRetCode, m_pbHead.str_errmsg().c_str());
		return NO_ERROR;
	}

	if(m_iPbHeadLen+m_iPbBodyLen+10 != (int)len)
	{
		REQERR_LOG("invalid len (%d+%d+10 != %lu)", m_iPbHeadLen, m_iPbBodyLen, len);
		return ERR_INVALID_PACKET_LEN;
	}
	return NO_ERROR;
}

int32_t CBasicPacket::CheckBasicPacket(const char *pPkg, size_t iPkgLen, struct sockaddr *sa, bool bUseSelfBuf)
{
	ResetPacketInfo();
	if(IsPacketPb(pPkg, iPkgLen))
		return CheckBasicPacketPb(pPkg, iPkgLen, sa, bUseSelfBuf);

	const char *buf = pPkg;
	size_t len = iPkgLen;
	if(bUseSelfBuf) {
		if(iPkgLen > sizeof(m_sCommBuf)) {
			REQERR_LOG( "invalid packent len:%lu > %lu", iPkgLen, sizeof(m_sCommBuf));
			m_pReqPkg = pPkg;
			m_dwReqPkgLen = len;
			return ERR_INVALID_PACKET_LEN;
		}
		memcpy(m_sCommBuf, buf, len);
		buf = m_sCommBuf;
		m_iCommLen = (int)len;
	}
	m_pReqPkg = buf;
	m_dwReqPkgLen = len;

	if(sa != NULL) {
		m_addrRemote.SetAddress(sa);
		slog.SetCust_6(m_addrRemote.Convert(true).c_str());
	}

	DEBUG_LOG("recv packet from:%s - packet length:%lu", m_addrRemote.Convert(true).c_str(), len);

	if(len <= 2+sizeof(ReqPkgHead) 
		|| ntohs(((ReqPkgHead*)(1+buf))->wPkgLen) != len
		|| len > MAX_UDP_REQ_PACKET_SIZE 
		|| buf[0] != SPKG || buf[len-1] != EPKG)
	{
		REQERR_LOG("invalid pkg - %lu <= %lu, pkglenInHeader:%d!=%lu, %lu>%d, %c(!=%c),%c(!=%c), from:%s",
			len, 2+sizeof(ReqPkgHead), ntohs(((ReqPkgHead*)(1+buf))->wPkgLen), len, len, 
			MAX_UDP_REQ_PACKET_SIZE, buf[0], SPKG, buf[len-1], EPKG, m_addrRemote.Convert(true).c_str());
		return ERR_INVALID_PACKET;
	}

	m_pstReqHead = (ReqPkgHead*)(buf+1);
	uint16_t wToPkgBody = ntohs(m_pstReqHead->wToPkgBody);
	m_dwReqCmd = ntohl(m_pstReqHead->dwCmd);
	m_dwReqSeq = ntohl(m_pstReqHead->dwSeq);
	slog.SetCust_1(m_dwReqCmd);
	slog.SetCust_2(m_dwReqSeq);

	uint16_t wRemainLen = len;
	wRemainLen -= 2+sizeof(ReqPkgHead);
	m_pstSig = (TSignature*)(1+sizeof(ReqPkgHead)+buf);
	if(wRemainLen < sizeof(TSignature) || wRemainLen < sizeof(TSignature)+ntohs(m_pstSig->wSigLen)
		|| ntohs(m_pstSig->wSigLen) > MAX_SIGNATURE_LEN)
	{
		REQERR_LOG("invalid signature len:%u < (%lu or %lu)",
			wRemainLen, sizeof(TSignature), sizeof(TSignature)+ntohs(m_pstSig->wSigLen));
		return ERR_INVALID_PACKET;
	}

	uint16_t wBasicLen = 1+sizeof(ReqPkgHead)+sizeof(TSignature)+ntohs(m_pstSig->wSigLen);
	if(wToPkgBody < wBasicLen || wToPkgBody+sizeof(TPkgBody) > len)
	{
		REQERR_LOG("invalid tobodylen:%u pkgLen:%lu basiclen:%u", wToPkgBody, len, wBasicLen);
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
	int m_bRetCode = m_pstReqHead->bRetCode;
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
			WARN_LOG( "return failed ret:%d have no error msg !", m_bRetCode);
	}
	return NO_ERROR;
}

void CBasicPacket::AckToReqPb(std::string &strBody, int32_t iRetCode)
{
	static char sBuf[8192];
	if(!m_bParsePbHead)
	{
		ERR_LOG("pb not parse head, no packet ack");
		return;
	}
	if(m_pbHead.uint32_magic_response_num() == MAGIC_RESPONSE_NUM) {
		WARN_LOG("packet is response -- not ack again !");
		return;
	}

	m_bIsResponse = true;

	m_pbHead.set_uint32_result(iRetCode);
	m_pbHead.set_uint32_magic_response_num(MAGIC_RESPONSE_NUM);
	if(iRetCode != NO_ERROR)
		m_pbHead.set_str_errmsg(GetErrMsg(iRetCode));

	std::string strHead;
	if(!m_pbHead.AppendToString(&strHead))
	{
		ERR_LOG("protobuf AppendToString failed msg:%s", strerror(errno));
		return;
	}

	if(strHead.size()+strBody.size()+10 > sizeof(sBuf)) 
	{
		ERR_LOG("packet too long over (%lu > %lu)", strHead.size()+strBody.size()+10, sizeof(sBuf));
		return ;
	}

	char *pdata = sBuf; 
	*pdata = SPKG_PB;
	pdata++;

	int32_t iHeadLen = strHead.size();
	*(int32_t*)pdata = htonl(iHeadLen);
	pdata += sizeof(int32_t);
	memcpy(pdata, strHead.c_str(), strHead.size());
	pdata += strHead.size();

	int32_t iBodyLen = strBody.size();
	*(int32_t*)pdata = htonl(iBodyLen);
	pdata += sizeof(int32_t);
	memcpy(pdata, strBody.c_str(), strBody.size());
	pdata += strBody.size();

	*pdata = EPKG_PB;
	pdata++;
	SendResponsePacket(sBuf, pdata-sBuf);
}

void CBasicPacket::AckToReqPb(int32_t iRetCode)
{
	if(!m_bParsePbHead)
	{
		ERR_LOG("pb not parse head, no packet ack");
		return;
	}

	if(m_pbHead.uint32_magic_response_num() == MAGIC_RESPONSE_NUM) {
		WARN_LOG("packet is response -- not ack again !");
		return;
	}
	m_bIsResponse = true;

	m_pbHead.set_uint32_result(iRetCode);
	m_pbHead.set_uint32_magic_response_num(MAGIC_RESPONSE_NUM);
	if(iRetCode != NO_ERROR)
		m_pbHead.set_str_errmsg(GetErrMsg(iRetCode));

	std::string strResp;
	if(!m_pbHead.AppendToString(&strResp))
	{
		ERR_LOG("protobuf AppendToString failed msg:%s", strerror(errno));
		return;
	}

	char sBuf[1024] = {0};
	char *pdata = sBuf;
	*pdata = SPKG_PB;
	pdata++;
	*(int32_t*)pdata = htonl(strResp.size());
	pdata += sizeof(int32_t);
	memcpy(pdata, strResp.c_str(), strResp.size());
	pdata += strResp.size();

	*(int32_t*)pdata = 0;
	pdata += sizeof(int32_t);
	*pdata = EPKG_PB;
	pdata++;
	SendResponsePacket(sBuf, pdata-sBuf);
}

int32_t CBasicPacket::AckToReq(int iRetCode)
{
	if(m_bIsPbReq) {
		AckToReqPb(iRetCode);
		return NO_ERROR;
	}

	if(m_pstReqHead != NULL && ntohl(m_pstReqHead->dwRespMagicNum) == MAGIC_RESPONSE_NUM) 
	{
		WARN_LOG("packet is response -- not ack again !");
		return NO_ERROR;
	}

	m_bIsResponse = true;
	if(iRetCode != NO_ERROR) {
		static char sTmpBuf[128] = {0};
		m_pstRespBody = (TPkgBody*)sTmpBuf;
		m_wRespBodyLen = sizeof(TPkgBody);
		m_wRespBodyLen += SetWTlv(
			m_pstRespBody->stTlv, TLV_ACK_ERROR, strlen(GetErrMsg(iRetCode)), GetErrMsg(iRetCode));
		m_pstRespBody->bTlvNum = 1;
	}

	m_iCommLen = sizeof(m_sCommBuf);
	if((iRetCode=MakePacket(m_sCommBuf, &m_iCommLen, iRetCode)) > 0)
		return SendResponsePacket(m_sCommBuf, m_iCommLen);
	ERR_LOG("MakePacket failed, ret:%d", iRetCode);
	return NO_ERROR;
}

int32_t CBasicPacket::MakePacket(char *pBuf, int *piBufLen, int iRetCode)
{
    if(*piBufLen < (int)(2+sizeof(ReqPkgHead)+sizeof(TSignature)+sizeof(TPkgBody)
            +m_wRespSigLen+m_wRespCmdContentLen+m_wRespBodyLen)){
        ERR_LOG("invalid buf len:%d < %d",
            *piBufLen, (int)(2+sizeof(ReqPkgHead)+sizeof(TSignature)+sizeof(TPkgBody)+
                +m_wRespSigLen+m_wRespCmdContentLen+m_wRespBodyLen));
        return 0;
    }   
    char *pdata = pBuf;

    *pdata = SPKG;
    pdata++;

    ReqPkgHead *prespHead = (ReqPkgHead*)pdata;
	if(m_pstReqHead == NULL && m_pReqPkg != NULL)
		m_pstReqHead = (ReqPkgHead*)(m_pReqPkg+1);
	if(m_pstReqHead == NULL) {
		ERR_LOG("m_pstReqHead is NULL");
		return SLOG_ERROR_LINE;
	}
    memcpy(pdata, m_pstReqHead, sizeof(ReqPkgHead));
    prespHead->bRetCode = (uint8_t)iRetCode;
	if(m_bIsResponse)
		prespHead->dwRespMagicNum = htonl(MAGIC_RESPONSE_NUM);
    pdata += sizeof(ReqPkgHead);

    if(m_wRespSigLen > 0) {
        memcpy(pdata, m_pstRespSig, m_wRespSigLen);
        pdata += m_wRespSigLen;
    }   
    else {
		memset(pdata, 0, sizeof(TSignature));
        pdata += sizeof(TSignature); // empty signature
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
		memset(pdata, 0, sizeof(TPkgBody));
        pdata += sizeof(TPkgBody); // empty body
	}

    *pdata = EPKG;
    pdata++;
    prespHead->wPkgLen = htons(pdata-pBuf);
    *piBufLen = pdata-pBuf;
    return *piBufLen;
}

CTimeDiff::CTimeDiff(int iAttrId, int iStaticTimeMs)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	m_qwTimeStart = tv.tv_sec*1000000+tv.tv_usec;
	m_qwTimeEnd = 0;
	m_iAttrId = iAttrId;
	m_iStaticTimeMs = iStaticTimeMs;
	m_iAttrTime1000ms = 0;
	m_iAttrTime500ms = 0;
	m_iAttrTime200ms = 0;
	m_iAttrTime100ms = 0;
	m_iAttrTime50ms = 0;
	m_iAttrTime0ms = 0;
}

CTimeDiff::~CTimeDiff()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	m_qwTimeEnd = tv.tv_sec*1000000+tv.tv_usec;
	int iUseMs = (int)(m_qwTimeEnd-m_qwTimeStart)/1000;
	if(m_iAttrId != 0 && m_iStaticTimeMs != 0 && iUseMs > m_iStaticTimeMs) 
		MtReport_Attr_Add(m_iAttrId, 1);
	if(m_iAttrTime1000ms != 0 && iUseMs >= 1000)
		MtReport_Attr_Add(m_iAttrTime1000ms, 1);
	else if(m_iAttrTime500ms !=0 && iUseMs >= 500)
		MtReport_Attr_Add(m_iAttrTime500ms, 1);
	else if(m_iAttrTime200ms !=0 && iUseMs >= 200)
		MtReport_Attr_Add(m_iAttrTime200ms, 1);
	else if(m_iAttrTime100ms !=0 && iUseMs >= 100)
		MtReport_Attr_Add(m_iAttrTime100ms, 1);
	else if(m_iAttrTime50ms !=0 && iUseMs >= 50)
		MtReport_Attr_Add(m_iAttrTime50ms, 1);
	else if(m_iAttrTime0ms != 0)
		MtReport_Attr_Add(m_iAttrTime0ms, 1);
}

CTimeDiff::CTimeDiff()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	m_qwTimeStart = tv.tv_sec*1000000+tv.tv_usec;
	m_qwTimeEnd = 0;
	m_iAttrId = 0;
	m_iStaticTimeMs = 0;
	m_iAttrTime1000ms = 0;
	m_iAttrTime500ms = 0;
	m_iAttrTime200ms = 0;
	m_iAttrTime100ms = 0;
	m_iAttrTime50ms = 0;
	m_iAttrTime0ms = 0;
}

int CTimeDiff::GetTimeDiffMs()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	m_qwTimeEnd = tv.tv_sec*1000000+tv.tv_usec;
	return (m_qwTimeEnd-m_qwTimeStart)/1000;
}

int CTimeDiff::GetTimeDiffUs()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	m_qwTimeEnd = tv.tv_sec*1000000+tv.tv_usec;
	return (m_qwTimeEnd-m_qwTimeStart);
}


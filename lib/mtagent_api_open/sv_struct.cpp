#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "sv_struct.h"

int SetWTlv(TWTlv *ptlv, uint16_t wType, uint16_t wLen, const char *pValue)
{
	ptlv->wType = htons(wType);
	ptlv->wLen = htons(wLen);
	memcpy(ptlv->sValue, pValue, wLen);
	return sizeof(TWTlv)+wLen;
}

void* GetWTlvValueByType2(uint16_t wType, TPkgBody *pstTlv) 
{
	int i=0;
	TWTlv *ptlv = pstTlv->stTlv;
	for(i=0; i < pstTlv->bTlvNum; i++)
	{
		if(ntohs(ptlv->wType) == wType)
			break;
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	if(i >= pstTlv->bTlvNum)
		return NULL;
	return pstTlv->stTlv[i].sValue; 
}

TWTlv * GetWTlvType2_list(TPkgBody *pstTlv, int *piStartNum) 
{
	int i=0;
	TWTlv *ptlv = pstTlv->stTlv;
	if(*piStartNum >= pstTlv->bTlvNum || *piStartNum < 0)
		return NULL;
	for(i=0; i < pstTlv->bTlvNum; i++)
	{
		if(i >= *piStartNum) {
			*piStartNum = i+1;
			return ptlv;
		}
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	return NULL;
}

TWTlv * GetWTlvByType2_list(uint16_t wType, TPkgBody *pstTlv, int *piStartNum) 
{
	int i=0;
	TWTlv *ptlv = pstTlv->stTlv;
	if(*piStartNum >= pstTlv->bTlvNum || *piStartNum < 0)
		return NULL;
	for(i=0; i < pstTlv->bTlvNum; i++)
	{
		if(i >= *piStartNum && ntohs(ptlv->wType) == wType) {
			*piStartNum = i+1;
			return ptlv;
		}
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	return NULL;
}

TWTlv * GetWTlvByType2(uint16_t wType, TPkgBody *pstTlv) 
{
	int i=0;
	TWTlv *ptlv = pstTlv->stTlv;
	for(i=0; i < pstTlv->bTlvNum; i++)
	{
		if(ntohs(ptlv->wType) == wType)
			return ptlv;
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	return NULL;
}

void* GetWTlvValue(uint16_t wType, TWTlv *ptlv, int iTlvLen)
{
	int i=0;
	for(i=0; i < iTlvLen-1;)
	{
		if(ntohs(ptlv->wType) == wType)
			return ptlv->sValue;
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
		i += sizeof(TWTlv)+ntohs(ptlv->wLen);
	}
	return NULL; 
}

void* GetWTlvValueByType(uint16_t wType, uint16_t wTlvNum, TWTlv *ptlv)
{
	int i=0;
	for(i=0; i < wTlvNum; i++)
	{
		if(ntohs(ptlv->wType) == wType)
			break;
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	if(i >= wTlvNum)
		return NULL;
	return ptlv->sValue;
}

TWTlv * GetWTlvByType(uint16_t wType, uint16_t wTlvNum, TWTlv *ptlv)
{
	int i=0;
	for(i=0; i < wTlvNum; i++)
	{
		if(ntohs(ptlv->wType) == wType)
			return ptlv;
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	return NULL;
}

void LogInfoNtoH(LogInfo *pLog)
{
	pLog->dwLogSeq = ntohl(pLog->dwLogSeq);
	pLog->iAppId = ntohl(pLog->iAppId);
	pLog->iModuleId = ntohl(pLog->iModuleId);
	pLog->dwLogConfigId = ntohl(pLog->dwLogConfigId);
	pLog->wLogType = ntohs(pLog->wLogType);
	pLog->qwLogTime = ntohll(pLog->qwLogTime);
	pLog->wCustDataLen = ntohs(pLog->wCustDataLen);
	pLog->wLogDataLen = ntohs(pLog->wLogDataLen);
}

void TlvLogInfoNtoH(TlvLogInfo *pLog)
{
	pLog->dwCust_1 = ntohl(pLog->dwCust_1);
	pLog->dwCust_2 = ntohl(pLog->dwCust_2);
	pLog->iCust_3 = ntohl(pLog->iCust_3);
	pLog->iCust_4 = ntohl(pLog->iCust_4);
	pLog->dwLogConfigId = ntohl(pLog->dwLogConfigId);
	pLog->dwLogHostId = ntohl(pLog->dwLogHostId);
	pLog->iModuleId = ntohl(pLog->iModuleId);
	pLog->wLogType = ntohs(pLog->wLogType);
	pLog->qwLogTime = ntohll(pLog->qwLogTime);
}

void MonitorPkgLogInfoNtoH(MonitorPkgLogInfo *pinfo)
{
	pinfo->iAppId = ntohl(pinfo->iAppId);
	pinfo->dwLogHostId = ntohl(pinfo->dwLogHostId);
	pinfo->wCltReqLogCount = ntohs(pinfo->wCltReqLogCount);
	pinfo->wSrvWriteLogCount = ntohs(pinfo->wSrvWriteLogCount);
}

void MonitorPkgLogInfoHtoN(MonitorPkgLogInfo *pinfo)
{
	pinfo->iAppId = htonl(pinfo->iAppId);
	pinfo->dwLogHostId = htonl(pinfo->dwLogHostId);
	pinfo->wCltReqLogCount = htons(pinfo->wCltReqLogCount);
	pinfo->wSrvWriteLogCount = htons(pinfo->wSrvWriteLogCount);
}

void PkgHeadNtoH(TBworldPkgHead *pNPkgHead, TBworldPkgHead *pHPkgHead)
{
	pHPkgHead->wPkgLen = ntohs(pNPkgHead->wPkgLen);
	pHPkgHead->wMainCmd = ntohs(pNPkgHead->wMainCmd);
	pHPkgHead->wSubCmd = ntohs(pNPkgHead->wSubCmd);
	pHPkgHead->dwSeq = ntohl(pNPkgHead->dwSeq);
	pHPkgHead->wResultCode = ntohl(pNPkgHead->wResultCode);
	memcpy(pHPkgHead->bReserved, pNPkgHead->bReserved, sizeof(pNPkgHead->bReserved));
	memcpy(pHPkgHead->bEchoBuf,  pNPkgHead->bEchoBuf, sizeof(pNPkgHead->bEchoBuf));
}

void PkgHeadHtoN(TBworldPkgHead *pNPkgHead, TBworldPkgHead *pHPkgHead)
{
	pNPkgHead->wPkgLen = htons(pHPkgHead->wPkgLen);
	pNPkgHead->wMainCmd = htons(pHPkgHead->wMainCmd);
	pNPkgHead->wSubCmd = htons(pHPkgHead->wSubCmd);
	pNPkgHead->dwSeq = ntohl(pHPkgHead->dwSeq);
	pNPkgHead->wResultCode = ntohl(pHPkgHead->wResultCode);
	memcpy(pNPkgHead->bReserved, pHPkgHead->bReserved, sizeof(pNPkgHead->bReserved));
	memcpy(pNPkgHead->bEchoBuf, pHPkgHead->bEchoBuf, sizeof(pNPkgHead->bEchoBuf));
}

void PkgSigNtoH(TBworldSignature *pNSig, TBworldSignature *pHSig)
{
	pHSig->bSigType = pNSig->bSigType;
	pHSig->wSigLen = ntohs(pNSig->wSigLen);
	memcpy(pHSig->sSigValue, pNSig->sSigValue, pHSig->wSigLen);
}

void PkgSigHtoN(TBworldSignature *pNSig, TBworldSignature *pHSig)
{
	pNSig->bSigType = pHSig->bSigType;
	pNSig->wSigLen = htons(pHSig->wSigLen);
	memcpy(pNSig->sSigValue, pHSig->sSigValue, pHSig->wSigLen);
}

// pkgbody is valid ret 0
// pkgbody is invalid ret -1 
int CheckPkgBody(TPkgBody *pstBody, uint16_t wBodyLen)
{
	int i = 0;
	uint8_t *pdata = (uint8_t*)pstBody;
	TWTlv *ptlv = NULL;
	if(wBodyLen < sizeof(TPkgBody))
		return -1;
	wBodyLen -= sizeof(TPkgBody);
	pdata += sizeof(TPkgBody);
	for(i=0; i < pstBody->bTlvNum; i++)
	{
		if(wBodyLen < sizeof(TWTlv))
			return -1;
		ptlv = (TWTlv *)pdata; 
		if(ntohs(ptlv->wLen)+sizeof(TWTlv) > wBodyLen)
			return -1;
		wBodyLen -= ntohs(ptlv->wLen)+sizeof(TWTlv);
		pdata += ntohs(ptlv->wLen)+sizeof(TWTlv);
	}
	return 0;
}


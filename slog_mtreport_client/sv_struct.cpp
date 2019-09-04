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

#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mtreport_client.h"
#include "sv_str.h"
#include "sv_struct.h"
#include "aes.h"

int SetWTlv(TWTlv *ptlv, uint16_t wType, uint16_t wLen, const char *pValue)
{
	ptlv->wType = htons(wType);
	ptlv->wLen = htons(wLen);
	memcpy(ptlv->sValue, pValue, wLen);
	return sizeof(TWTlv)+wLen;
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

int InitSignature(TSignature *psig, void *pdata, const char *pKey, int bSigType)
{
	int32_t iSigBufLen = MAX_SIGNATURE_LEN;

	switch(bSigType) {
		case MT_SIGNATURE_TYPE_HELLO_FIRST:
			iSigBufLen = ((sizeof(MonitorHelloSig)>>4)+1)<<4;
			if(iSigBufLen > MAX_SIGNATURE_LEN)
			{
				ERROR_LOG("need more space %d > %d", iSigBufLen, MAX_SIGNATURE_LEN);
				return -1;
			}
			aes_cipher_data((const uint8_t*)pdata, 
				sizeof(MonitorHelloSig), (uint8_t*)psig+sizeof(TSignature), (const uint8_t*)pKey, AES_256);
			psig->bSigType = bSigType;
			psig->wSigLen = iSigBufLen;
			DEBUG_LOG("MtEncrypt by key: [ %s ] ok, key type:%d, encrypt len:%d",
				DumpStrByMask(pKey, 32), bSigType, iSigBufLen);
			break;

		case MT_SIGNATURE_TYPE_COMMON:
			iSigBufLen = ((sizeof(MonitorCommSig)>>4)+1)<<4;
			if(iSigBufLen > MAX_SIGNATURE_LEN)
			{
				ERROR_LOG("need more space %d > %d", iSigBufLen, MAX_SIGNATURE_LEN);
				return -1;
			}
			aes_cipher_data((const uint8_t*)pdata, 
				sizeof(MonitorCommSig), (uint8_t*)psig+sizeof(TSignature), (const uint8_t*)pKey, AES_128);
			psig->bSigType = bSigType;
			psig->wSigLen = iSigBufLen;
			DEBUG_LOG("MtEncrypt by key: [ %s ] ok, key type:%d, encrypt len:%d",
				DumpStrByMask(pKey, 16), bSigType, iSigBufLen);
			break;
		default:
			break;
	}
	return 0;
}


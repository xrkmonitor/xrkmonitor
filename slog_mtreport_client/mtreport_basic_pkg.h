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

#ifndef _BASIC_PKG_H_
#define _BASIC_PKG_H_ 1

#include "sv_errno.h"
#include "sv_struct.h"

#define PKG_BUFF_LENGTH 4096

// 以下类用于从数据包中分析出各个部分，其中 [业务相关] 可有可无，根据具体命令号有关
// pkg req: SPKG + ReqPkgHead + TSignature + [cmd content] + TPkgBody + EPKG
// pkg ack : SPKG + ReqPkgHead + TSignature + [cmd conent] + TPkgBody + EPKG
// success TPkgBody: 含有正确回复的 tlv 信息
// failed TPkgBody: 含有 TLV_ACK_ERROR
class CBasicPacket
{
	public:
		CBasicPacket() { ResetPacketInfo(); }
		int32_t CheckBasicPacket(const char *buf, int len);

		uint32_t MakeReqPkg(char *pBuf, int *piBufLen) { return MakePacket(pBuf, piBufLen, NO_ERROR, true); }
		uint32_t MakeRespPkg(int iErrCode, char *pBuf, int *piBufLen);

		// 以下接口用于组请求包
		void InitReqPkgHead(ReqPkgHead *pHead, uint32_t cmd);
		void InitSignature(TSignature *psig);
		void InitCmdContent(void *pCmdCt, uint16_t wCmdCtLen);
		void InitPkgBody(TPkgBody *pbody, uint16_t wBodyLen);

		static const char * GetErrMsg(int32_t err);

	private:
		void ResetPacketInfo();
		uint32_t MakePacket(char *pBuf, int *piBufLen, int iRetCode, bool bIsReqPkg);
		uint32_t MakeErrorRespPkg(int iErrCode, char *pBuf, int *piBufLen);

	public:
		const char *m_pReqPkg;
		uint32_t m_dwReqPkgLen;
		ReqPkgHead *m_pstReqHead;

		TSignature *m_pstSig;
		TSignature *m_pstRespSig;
		uint16_t m_wRespSigLen;

		void *m_pstCmdContent;
		uint16_t m_wCmdContentLen;
		void *m_pstRespCmdContent;
		uint16_t m_wRespCmdContentLen;

		TPkgBody *m_pstBody;
		uint16_t m_wBodyLen;
		TPkgBody *m_pstRespBody; // 响应包有包体 tlv 时设置到该参数中
		uint16_t m_wRespBodyLen; // 响应包有包体 tlv 时设置到该参数中

		uint32_t m_dwReqCmd;
		uint32_t m_dwReqSeq;
		uint8_t m_bRetCode;
};

#endif 


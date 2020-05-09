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


#ifndef __SOCK_BASIC_PACKET_H__
#define __SOCK_BASIC_PACKET_H__ 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include "UdpSocket.h"
#include "supper_log.h"
#include "top_proto.pb.h"
#include "Ipv4Address.h"
#include "sv_struct.h"

#define  MAX_UDP_REQ_PACKET_SIZE 4096

#define CHECK_LEN_RET_NULL(big, small) do { \
	if((int)(big) < (int)(small)) { ERR_LOG("check buff failed:%u < %u !", big, small); return;} }while(0)
#define CHECK_LEN_RET_VALUE(big, small, v) do { \
	if((int)(big) < (int)(small)) { ERR_LOG("check buff failed:%u < %u !", big, small); return v;} }while(0)
#define MAGIC_RESPONSE_NUM 1117250  // 响应包标记

enum
{
	NO_ERROR = 0,
	ERR_SERVER = 1,
	ERR_INVALID_PACKET = 2,
	ERR_INVALID_PACKET_LEN = 3,
	ERR_UNKNOW_CMD= 4,
	ERR_CHECK_TLV = 5,
	ERR_CHECK_SIGNATUR = 6,
	ERR_NOT_ACK = 7,
	ERR_INVALID_TLV_VALUE = 8,
	ERR_FIND_TLV = 9,
	ERR_RESP_SIZE_OVER_LIMIT = 10,
	ERR_HTTP_KEEP_NOT_FIND = 11,
	ERR_DECRYPT_FAILED = 12,
	ERR_FIND_SESSION_INFO = 13,
	ERR_INVALID_CMD_CONTENT = 14,
	ERR_CHECK_DISTRIBUTE = 15,
	ERR_CHECK_BODY_MD5 = 16,
	ERR_UNKNOW_SIGTYPE = 17,
	ERR_NO_USER_MASTER = 18,
	ERR_DECRYPT_SIGNATURE = 19,
	ERR_INVALID_SIGNATURE_INFO = 20,
	ERR_CHECK_PACKET_CRC = 21,
	ERR_CHECK_MACHINE_ACCESS_RIGHT = 22,
	ERR_NO_RESPONSE_CONTENT = 23,
	ERR_SET_MACHINE_KEY_FAILED = 24,
	ERR_INVALID_APPID = 25,
	ERR_NOT_FIND_MACHINE = 26,
	ERR_APP_LOG_DISPATCH_INVALID = 27,
	ERR_ATTR_DISPATCH_INVALID = 28,
	ERR_INVALID_MAGIC = 29,
	ERR_LOG_FREQ_OVER_LIMIT = 30,
	ERR_MAX=100,
};

#define MAX_ATTR_PKG_LENGTH 1200 
#ifndef MAX_ATTR_READ_PER_EACH
#define MAX_ATTR_READ_PER_EAC 120
#endif
#pragma pack(1)
typedef struct
{
	int32_t iAttrID;
	int32_t iCurValue;
} AttrNodeClient;

typedef struct
{
	int32_t iStrAttrId;
	int32_t iStrVal;
	int iStrLen; // include '\0'
	char szStrInfo[0];
}StrAttrNodeClient;

inline void AttrInfoNtoH(AttrNodeClient *pnode)
{
	pnode->iAttrID = ntohl(pnode->iAttrID);
	pnode->iCurValue = ntohl(pnode->iCurValue);
}
#pragma pack()

/*
   *  重要： 使用 CBasicPacket 注意事项
   * 
   *  1、如果接收到数据包后，还有其它的异步调用，使用 CheckBasicPacket() 函数的参数：bUseSelfBuf 要设置
   *     否则 CBasicPacket 类解析到的相关字段可能因为离开调用上下文而失效 !
   *
   *  2、回包函数 AckToReq() ，相关的包体字段要确保上下文有效，如：InitCmdContent(), InitSignature() 等的指向变量要同 AckToReq()
   *     上下文一致，或者使用 static 变量也可以
   *
   *  3、自动回包函数 AckToReq()，内部使用了 m_sCommBuf 变量，需要注意 CheckBasicPacket() 设置了 bUseSelfBuf 时是否可能导致冲突问题
   *
   *
   */

//
//log cust info
// - cust1: req cmd, cust2: req seq, cust6:remote address
//
// 以下类用于从数据包中分析出各个部分，其中 [业务相关] 可有可无，根据具体命令号有关
// pkg req: SPKG + ReqPkgHead + TSignature + [cmd content] + TPkgBody + EPKG
// pkg ack : SPKG + ReqPkgHead + TSignature + [cmd conent] + TPkgBody + EPKG
// success TPkgBody: 含有正确回复的 tlv 信息
// failed TPkgBody: 含有 TLV_ACK_ERROR
class CBasicPacket
{
	public:
		static const char * GetErrMsg(int32_t err);
		CBasicPacket(){ ResetPacketInfo(); }
		virtual ~CBasicPacket(){}
		virtual int32_t SendResponsePacket(const char *pkg, int len){ assert(0); return 0; }
		void ResetPacketInfo(); // 调用后可以再次用于分析数据包
		int32_t MakeReqPkg(char *pBuf=NULL, int *piBufLen=NULL) { return MakePacket(pBuf, piBufLen, NO_ERROR); }
		int32_t CheckBasicPacket(const char *buf, size_t len, struct sockaddr *sa=NULL, bool bUseSelfBuf=false);
		int32_t MakeErrAckPacket(const char *reqPkg, size_t reqLen, char *pBuf, int32_t iBufLen, uint8_t bErrCode);

		int SetPacketPb(std::string &head, std::string &body, char **ppack, int ibufLen=0);
		void AckToReqPb(std::string &body, int32_t iRetCode);
		int32_t AckToReq(int iRetCode);
		bool IsPacketPb(const char *pkg, size_t len);
		bool PacketPb() {return m_bIsPbReq; }

		void InitCmdContent(void *pCmdCt, uint16_t wCmdCtLen) {
			m_pstRespCmdContent = pCmdCt;
			m_wRespCmdContentLen = wCmdCtLen;
		}
		void InitSignature(TSignature *psig) {
			m_pstRespSig = psig;
			m_wRespSigLen = sizeof(TSignature)+psig->wSigLen;
			psig->wSigLen = ntohs(psig->wSigLen);
		}
		void InitPkgBody(TPkgBody *pbody, uint16_t wBodyLen) {
			m_pstRespBody = pbody;
			m_wRespBodyLen = wBodyLen;
		}
		void InitReqPkgHead(ReqPkgHead *pHead, uint32_t cmd, uint32_t dwSeq);
		int32_t MakePacket(char *pBuf, int *piBufLen, int iRetCode);
		void AckToReqPb(int32_t iRetCode);

	private:
		int32_t CheckBasicPacketPb(const char *buf, size_t len, struct sockaddr *sa=NULL, bool bUseSelfBuf=false);

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

		Ipv4Address m_addrRemote;
		uint32_t m_dwReqCmd;
		uint32_t m_dwReqSeq;
		char m_sCommBuf[MAX_UDP_REQ_PACKET_SIZE];
		int32_t m_iCommLen;
		uint8_t m_bRetCode;
		bool m_bIsPbReq;
		bool m_bIsResponse;

		// 以下字段用于关联请求信息，目前仅用于http 长连接
		uint32_t m_dwResponseSeq; 

		// protobuf  协议数据包
		::comm::PkgHead m_pbHead;
		bool m_bParsePbHead;
		int32_t m_iPbHeadLen;
		int32_t m_iPbBodyLen;
};

class CUdpSock_Sync: public UdpSocket, public CBasicPacket
{
	public:
		CUdpSock_Sync(ISocketHandler& h);
		~CUdpSock_Sync() {}
		void OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len);
		int32_t SendResponsePacket(const char *pkg, int len) { return 0; }

};

int SendUdpPacket(Ipv4Address *paddr, char *pkg, int iPkgLen, int iTimeoutMs, CSupperLog *pslog);

class CTimeDiff
{
	public:
		CTimeDiff();
		CTimeDiff(int iAttrId, int iStaticTimeMs);
		~CTimeDiff();
		int GetTimeDiffMs();
		int GetTimeDiffUs();
		void SetAttrInfo(int i1000, int i500, int i200, int i100, int i50, int i) {
			m_iAttrTime1000ms = i1000;
			m_iAttrTime500ms = i500;
			m_iAttrTime200ms = i200;
			m_iAttrTime100ms = i100;
			m_iAttrTime50ms = i50;
			m_iAttrTime0ms = i;
		}

	private:
		uint64_t m_qwTimeStart;
		uint64_t m_qwTimeEnd;
		int m_iAttrId;
		int m_iStaticTimeMs;
		int m_iAttrTime1000ms;
		int m_iAttrTime500ms;
		int m_iAttrTime200ms;
		int m_iAttrTime100ms;
		int m_iAttrTime50ms;
		int m_iAttrTime0ms;
};

#endif


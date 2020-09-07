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

#ifndef _UDP_SOCK_H_
#define _UDP_SOCK_H_ 1

#include <Sockets/UdpSocket.h>
#include <Sockets/SocketHandler.h>
#include <basic_packet.h>
#include <libmysqlwrapped.h>

#define MT_SIGNATURE_HELLO_KEY "#$@@ksdfk2313*("

#pragma pack(1)

// cmd first hello struct -------------------------------
typedef struct
{
	uint8_t bEnableEncryptData;
	uint32_t dwPkgSeq;
	char sRespEncKey[16+1];
	uint32_t dwAgentClientIp;
}MonitorHelloSig; // req

typedef struct 
{
	int32_t iMtClientIndex;
	int32_t iMachineId;
	char sCmpTime[32];
	char sVersion[12];
	char sOsInfo[32];
	char sOsArc[32];
	char sLibcVer[32];
	char sLibcppVer[32];
}MonitorHelloFirstContent; // req

typedef struct 
{
	int32_t iMtClientIndex;
	int32_t iMachineId;
	uint32_t dwConnServerIp;
	uint32_t dwAttrSrvIp;
	uint16_t wAttrSrvPort;
	char szNewMasterSrvIp[16];
	uint16_t wNewSrvPort;
}MonitorHelloFirstContentResp; // resp

// cmd hello struct -------------------------------
typedef struct
{
	uint32_t dwHelloTimes;
	uint32_t dwServerResponseTime;

	// user config check
	uint32_t dwAttrSrvIp;
	uint16_t wAttrServerPort;
}MonitorHelloContent; // req

typedef struct 
{
	int32_t iMtClientIndex;

	// user config check
	uint8_t bConfigChange;
	uint32_t dwAttrSrvIp;
	uint16_t wAttrServerPort;
}MonitorHelloContentResp; // resp

// cmd check log config struct -------------------------------
typedef struct
{
	uint32_t dwCfgId;
	uint32_t dwSeq;
	uint32_t dwCfgFlag;
}LogConfigReq;

typedef struct
{
	uint32_t dwServerResponseTime;
	uint16_t wLogConfigCount;
	LogConfigReq stLogConfigList[0];
}ContentCheckLogConfig; // req

// cmd check app info struct -------------------------------
typedef struct
{
	int32_t iAppId;
	uint32_t dwSeq;
	uint32_t dwCfgFlag;
}AppInfoReq;

typedef struct
{
	uint32_t dwServerResponseTime;
	uint16_t wAppInfoCount;
	AppInfoReq stAppList[0];
}ContentCheckAppInfo; // req

typedef struct
{
	int32_t iAppId;
	uint8_t bAppType;
	uint16_t wModuleCount;
	uint32_t dwSeq;
	uint16_t wLogSrvPort;
	uint32_t dwAppSrvMaster;
}MtAppInfo;

// ---------------- cmd check server config 
typedef struct 
{
	uint32_t dwServerResponseTime;
	uint32_t dwConfigSeq;
}ContentCheckSystemCfgReq; // req

#pragma pack()


class CUdpSock: public UdpSocket, public CBasicPacket 
{
	public:
		CUdpSock(ISocketHandler& h);
		~CUdpSock();
		int32_t SendResponsePacket(const char*pkg, int len);
		void OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len);
		void SendRealInfo();

	private:
		int InitSignature(TSignature *psig, void *pdata, const char *pKey, int bSigType);
		int32_t CheckSignature();
		int DealCmdHelloFirst();
		int DealCmdHello();
		int DealCommInfo();
		int DealCmdCheckLogConfig();
		int SetLogConfigCheckInfo(ContentCheckLogConfig *pctinfo);
		int DealCmdCheckAppInfo();

		int GetLogFreq(MtSLogConfig & stLogConfig, SLogClientConfig *pCfgInfo);
		int InnerDealAppInfoCheck(int iFirstAppIdx, int iAppInfoCount, ContentCheckAppInfo *pctinfo,
			int &iAddCount, int &iModCount, int &iSameCount, int &iUseBufLen, int iMaxBufLen, TPkgBody *pRespTlvBody);
		int InnerDealLogConfigCheck(int iFirstIdx, int iInfoCount, ContentCheckLogConfig *pctinfo,
			int &iAddCount, int &iModCount, int &iSameCount, int &iUseBufLen, int iMaxBufLen, TPkgBody *pRespTlvBody);
		int SetHelloTimeToMachineTable();

		int SetAppInfoCheck(ContentCheckAppInfo *pctinfo);
		int DealCmdCheckServerInfo();
		int DealCmdCheckSystemConfig();
		int SetSystemConfigCheck(ContentCheckSystemCfgReq *pctinfo);
		void Init();
		void InitMtClientInfo();
		int GetUserMasterInfo();
		int GetMtClientInfo();
		int SetKeyToMachineTable();
		int SaveMachineInfoToDb(MonitorHelloFirstContent *pctinfo);

		uint32_t m_dwUserMasterId;
		char m_sDecryptBuf[MAX_SIGNATURE_LEN+16];
		void *m_pUmInfo;
		MtClientInfo *m_pMtClient;
		SLogConfig *m_pConfig;
		SLogAppInfo *m_pAppInfo;
		MachineInfo* m_pstMachInfo;
		int32_t m_iUserMasterIndex;
		int32_t m_iMtClientIndex;
		int32_t m_iRemoteMachineId;
		uint32_t m_dwAgentClientIp;
		bool m_bIsFirstHello;
};

#endif


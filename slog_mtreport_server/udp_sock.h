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
	int32_t iBindCloudUserId;
	int32_t iReserved_1;
	int32_t iReserved_2;
	uint32_t dwReserved_1;
	uint32_t dwReserved_2;
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

// 一键部署进度上报, server 不回包
typedef struct {
    int32_t iPluginId;
    int32_t iMachineId;
    int32_t iDbId;
    int32_t iStatus;
	char sCheckStr[16];
	char sDevLang[12];
    int32_t iReserved_1;
    int32_t iReserved_2;
    uint32_t dwReserved_1;
    uint32_t dwReserved_2;
}CmdPreInstallReportContent;

// ---------------- cmd send plugin info 
typedef struct{
    int iPluginId; // 插件 id
    char szVersion[12]; // 配置文件中的插件版本
    char szBuildVer[12]; // 插件编译时的版本信息 
    int iLibVerNum; // 使用的开发库版本编号
    uint32_t dwLastReportAttrTime; // 最后一次 attr 上报时间
    uint32_t dwLastReportLogTime; // 最后一次 log 上报时间
    uint32_t dwLastHelloTime; // 最后一次 hello 时间
    uint32_t dwPluginStartTime; // 插件启动时间
    uint8_t bPluginNameLen; // 插件名长度
    char sPluginName[0];
}TRepPluginInfoFirst; // 首次上报结构

typedef struct{
    int iPluginId; // 插件 id
    uint32_t dwLastReportAttrTime; // 最后一次 attr 上报时间
    uint32_t dwLastReportLogTime; // 最后一次 log 上报时间
    uint32_t dwLastHelloTime; // 最后一次 hello 时间
}TRepPluginInfo; // 非首次上报结构

typedef struct
{
    uint8_t bPluginCount;
    char plugins[0];
}MonitorRepPluginInfoContent;

typedef struct {
    int32_t iPluginId;
    uint8_t bCheckResult;
}MonitorPluginCheckResult;

typedef struct
{
    uint8_t bPluginCount;
    MonitorPluginCheckResult sCheckResult[0];
}MonitorRepPluginInfoContentResp;

//
// 与 server 交互的数据结构相关定义 s2c ---
typedef struct {
    int32_t iPluginId;
    int32_t iMachineId;
    int32_t iDbId;
	char sCheckStr[16];
	char sDevLang[12];
	char sPluginName[32];
    int32_t iReserved_1;
    int32_t iReserved_2;
    uint32_t dwReserved_1;
    uint32_t dwReserved_2;
	int iUrlLen;
	char sLocalCfgUrl[0];
}CmdS2cPreInstallContentReq;

typedef struct {
    int32_t iPluginId;
    int32_t iMachineId;
    int32_t iDbId;
    char sPluginName[32];
    int32_t iReserved_1;
    int32_t iReserved_2;
    uint32_t dwReserved_1;
    uint32_t dwReserved_2;
}CmdS2cMachOprPluginReq;

typedef struct {
    uint32_t dwDownCfgTime;
    int32_t iPluginId;
    int32_t iMachineId;
    uint8_t bRestartPlugin;
    int iConfigLen;
    char strCfgs[0];
}CmdS2cModMachPluginCfgReq;

typedef struct {
    uint32_t dwDownCfgTime;
    int32_t iPluginId;
    int32_t iMachineId;
}CmdS2cModMachPluginCfgResp;

// 操作插件结果
enum {
    MACH_OPR_PLUGIN_SUCCESS = 0,
    MACH_OPR_PLUGIN_REMOVE_FAILED = 1,
    MACH_OPR_PLUGIN_PLUGIN_DIR_FAILED = 2,
    MACH_OPR_PLUGIN_ENABLE_FAILED = 3,
    MACH_OPR_PLUGIN_DISABLE_FAILED = 4,
    MACH_OPR_PLUGIN_UNKNOW_OPR_CMD = 5,
    MACH_OPR_PLUGIN_NOT_FIND = 6,
    MACH_OPR_PLUGIN_RET_MAX = 6,
};

typedef struct {
    int32_t iPluginId;
    int32_t iMachineId;
    int32_t iDbId;
    char bOprResult;
    int32_t iReserved_1;
    int32_t iReserved_2;
    uint32_t dwReserved_1;
    uint32_t dwReserved_2;
}CmdS2cMachOprPluginResp; 

#pragma pack()

class CUdpSock;
typedef struct _MachOprPluginSess
{
    int32_t iPluginId;
    int32_t iMachineId;
    int32_t iDbId;
    int iEventType;
    CUdpSock *psock;
}TMachOprPluginSess;
typedef TMachOprPluginSess TPreInstallPluginSess;

class CUdpSock: public UdpSocket, public CBasicPacket 
{
	public:
		CUdpSock(ISocketHandler& h);
		~CUdpSock();
		int32_t SendResponsePacket(const char*pkg, int len);
		void OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len);
		void SendRealInfo();
		void DealEvent();
        void OnMachOprPluginExpire(TMachOprPluginSess *psess_data);
        void OnPreInstallPluginExpire(TPreInstallPluginSess *psess_data);

	private:
		int GetLocalPlugin(Json &js_plugin, int iPluginId);
		int DealCmdPreInstallReport();

        void DealEventMachOprPlugin(TEventInfo &event);
        int MakeMachOprPluginNotifyPkg(TEventInfo &event, uint64_t & qwSessionId);
        int DealCmdMachOprPluginResp();
        int DealCmdModMachPluginCfgResp();
        int DealCmdReportPluginCfg();

		void DealEventPreInstall(TEventPreInstallPlugin &ev);
		int MakePreInstallNotifyPkg(TEventPreInstallPlugin &ev, std::ostringstream &sCfgUrl,uint64_t &sessid);

		int GetBindxrkmonitorUid();
		int InitSignature(TSignature *psig, void *pdata, const char *pKey, int bSigType);
		int32_t CheckSignature();
		int DealCmdHelloFirst();
		int DealCmdHello();
		int DealCommInfo();
		int DealCmdReportPluginInfo();
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


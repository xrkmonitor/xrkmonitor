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

#ifndef _MTREPORT_PROTOC_H_
#define _MTREPORT_PROTOC_H_ 1

#include "mtreport_basic_pkg.h"
#include "sv_timer.h"

#define HELLO_FLAG_USE_LAST_SRV 1
#define HELLO_FLAG_USE_MASTER_SRV 2
#define HELLO_FLAG_USE_BACKUP_SRV 4
#define HELLO_FLAG_RANDKEY_SETED 8
#define HELLO_FLAG_USE_URL_SRV 16

#pragma pack(1)

//
// 与 server 交互的数据结构相关定义 ---
//
// ---------------- cmd first hello
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

// ---------------- cmd hello
typedef struct
{
	uint32_t dwHelloTimes;
	uint32_t dwServerResponseTime;
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

// ---------------- cmd check log config 
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

typedef struct    
{ 
    uint32_t dwSeq;    
    uint32_t dwCfgId;    
    int32_t iAppId;    
    int32_t iModuleId;    
    int32_t iLogType;              
    uint32_t dwSpeedFreq;          
    uint16_t wTestKeyCount;
    SLogTestKey stTestKeys[MAX_SLOG_TEST_KEYS_PER_CONFIG];
}MtSLogConfig; // 同 mtagent_api_open 下的 sv_struct.h

// ---------------- cmd check app config 
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

// ---------------- cmd check system config 
typedef struct 
{
	uint32_t dwServerResponseTime;
	uint32_t dwConfigSeq;
}ContentCheckSystemCfgReq; // req

// ---------------- cmd send app log 
// cmd content dec - LogInfo[n] n >=1
typedef struct 
{
	uint32_t dwLogSeq;
	int32_t  iAppId;
	int32_t iModuleId;
	uint32_t dwLogConfigId; // 产生该条日志的配置编号
	uint16_t wLogType;
	uint64_t qwLogTime;
	uint16_t wCustDataLen;
	uint16_t wLogDataLen;
	char sLog[0];
}LogInfo; // -- 单条日志内容

// ---------------- cmd send attr 
// cmd content dec - AttrNodeClient[n] n >=1
typedef struct
{
	int32_t iAttrID;
	int32_t iCurValue;
}AttrNodeClient;

// ---------------- cmd send plugin info 
typedef struct{
    int iPluginId; // 插件 id
    char szVersion[12]; // 配置文件中的插件版本
    char szBuildVer[12]; // 插件编译时的版本信息 
    int iLibVerNum; // 使用的开发库版本编号
    uint32_t dwConfigFileTime; // 配置文件最后修改时间
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
    uint8_t bNeedReportCfg;
}MonitorPluginCheckResult;

typedef struct
{
    uint8_t bPluginCount;
    MonitorPluginCheckResult sCheckResult[0];
}MonitorRepPluginInfoContentResp;

// ---------------- cmd send str attr 
// cmd content dec - StrAttrNodeClient[n] n >=1
typedef struct
{
	int32_t iStrAttrId;
	int32_t iStrVal;
	int iStrLen; // include '\0'
	char szStrInfo[0];
}StrAttrNodeClient;

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

// 修改插件配置、启用、禁用插件等
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

typedef struct {
	uint8_t bHelloFlag;
	char sRespEncKey[16];
}TCmdHelloSess;

typedef struct {
	uint8_t bIsUseBackupSrv;
	int32_t iAppInfoIdx;
	uint32_t dwAppLogSrvIP;
}TCmdSendAppLogSess;

typedef struct {
	uint8_t bTrySrvCount;
	uint32_t dwAttrSrvIP;
}TCmdSendAttrSess;

typedef struct {
	uint8_t bTrySrvCount;
	uint32_t dwConfigSrvIP;
}TCmdSendPluginSess;

// (pkg) 注意 session 最长不超过 TIMER_NODE_SESS_DATA_LEN(目前为：48) 个字节
#define SESS_FLAG_WAIT_RESPONSE 1 // 数据包已发送，等待响应
#define SESS_FLAG_RESPONSED 2 // 收到响应数据包
#define SESS_FLAG_TIMEOUT_SENDPKG 3 // 等定时器到期再发送数据包

// (envent) 注意 session 最长不超过 TIMER_NODE_SESS_DATA_LEN(目前为：48) 个字节 
#define EVENT_TYPE_APP_LOG 1 // 定时读取 app log
#define EVENT_TYPE_M_ATTR 2 // 定时读取每分钟的属性上报 
#define EVENT_TYPE_REPORT_PLUGIN_INFO 3 // 插件信息上报

typedef struct {
	uint8_t bEventType;
	int32_t iSockIndex;
	int32_t iExpireTimeMs;
	uint32_t dwEventSetTimeSec;
	uint32_t dwEventSetTimeUsec;
}ENVSESSION;

typedef struct {
	int iSockIndex;
	uint32_t dwSendTimeSec;
	uint32_t dwSendTimeUsec;
	uint8_t bSessStatus;
	union {
		TCmdHelloSess hello;
		TCmdSendAppLogSess applog;
		TCmdSendAttrSess attr;
		TCmdSendPluginSess plugin;
	}stCmdSessData;
}PKGSESSION;

// 一键部署插件进度状态, 同 supper_log.h
enum {
    EV_PREINSTALL_TO_CLIENT_OK = 4,
    EV_PREINSTALL_CLIENT_GET_DOWN_URL = 5,
    EV_PREINSTALL_CLIENT_GET_PACKET = 6,
    EV_PREINSTALL_CLIENT_START_PLUGIN = 7,
	EV_PREINSTALL_CLIENT_INSTALL_PLUGIN_OK = 9,

    EV_PREINSTALL_ERR_GET_URL = 20, 
    EV_PREINSTALL_ERR_GET_URL_RET = 21, 
    EV_PREINSTALL_ERR_GET_URL_PARSE_RET = 22, 
    EV_PREINSTALL_ERR_DOWNLOAD_PACK = 23, 
    EV_PREINSTALL_ERR_UNPACK = 24, 
    EV_PREINSTALL_ERR_MKDIR = 25, 
    EV_PREINSTALL_ERR_START_PLUGIN = 26,
	EV_PREINSTALL_ERR_DOWNLOAD_OPEN_CFG = 27,
};

int DealLogConfigNotify(CBasicPacket &pkg);
int DealAppConfigNotify(CBasicPacket &pkg);
int IsHelloValid();

int OnPkgExpire(TimerNode *pNodeShm, unsigned uiDataLen, char *pData);

int DealResponseHelloFirst(CBasicPacket &pkg);
int DealResponseHello(CBasicPacket &pkg);
int DealRespCheckLogConfig(CBasicPacket &pkg);
int DealRespCheckAppConfig(CBasicPacket &pkg);
int DealRespCheckServerConfig(CBasicPacket &pkg);
int DealRespCheckSystemConfig(CBasicPacket &pkg);

uint32_t MakeFirstHelloPkg();
int MakeHelloToServer(PKGSESSION *psess);
void InitCheckLogConfig(PKGSESSION *psess, uint32_t dwTimeOutMs=0, uint32_t dwSrvRespTime=0);
void InitCheckAppConfig(PKGSESSION *psess, uint32_t dwTimeOutMs=0, uint32_t dwSrvRespTime=0);
void InitCheckServerConfig(PKGSESSION *psess, uint32_t dwTimeOutMs=0, uint32_t dwSrvRespTime=0);
void InitCheckSystemConfig(PKGSESSION *psess, uint32_t dwTimeOutMs=0, uint32_t dwSrvRespTime=0);

uint32_t MakeAppLogPkg(struct sockaddr_in & app_server, char *pAppLogContent, int iAppLogContentLen, int iAppId);
uint32_t MakeAttrPkg(struct sockaddr_in & app_server, char *pContent, int iContentLen, bool bIsStrAttr=false);

int MakeRepPluginInfoToServer();
int DealPreInstallNotify(CBasicPacket &pkg);
uint32_t MakeServerRespPkg(char *pRespContent, int iRespContentLen, CBasicPacket &req_pkg);
int DealRespRepPluginInfo(CBasicPacket &pkg);
int DealMachineOprPlugin(CBasicPacket &pkg);

void TryOpenPluginInstallLogFile(CmdS2cPreInstallContentReq *plug);
void TryOpenPluginInstallLogFile(CmdS2cMachOprPluginReq *plug);

const std::string g_strDevLangShell("linux shell");
const std::string g_strDevLangJs("javascript");

#endif


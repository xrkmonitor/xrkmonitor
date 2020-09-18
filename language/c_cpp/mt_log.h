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

   开发库  mtreport_api 说明:
         用户使用监控系统的c/c++ 开发库，本库使用 标准 c 开发无任何第
		 三方库依赖，用户可以在 c或者 c++ 项目中使用

****/

#ifndef _MTLOG_141117_H_
#define _MTLOG_141117_H_ (1)

#include <inttypes.h>
#include "mt_attr.h"
#include "mt_vmem.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef SET_BIT
// set bit b in value n
#define SET_BIT(n, b) n|=b 
// check is set bit b in value n
#define IS_SET_BIT(n, b) n&b
// clear bit b in value n
#define CLEAR_BIT(n, b) n&=~b
#endif

#define MT_MTREPORT_SHM_CHECK_STR "dksgks@#%@#%@#@SSSSyyyy"

// log type
#define MTLOG_TYPE_OTHER 1 // 
#define MTLOG_TYPE_DEBUG 2
#define MTLOG_TYPE_INFO 4
#define MTLOG_TYPE_WARN 8
#define MTLOG_TYPE_REQERR 16
#define MTLOG_TYPE_ERROR 32
#define MTLOG_TYPE_FATAL 64 

// log shm
#define MTLOG_SHM_RECORDS_COUNT 10000
#define MTLOG_SHM_DEF_COUNT 3 
#define MTLOG_LOG_SPECIAL_COUNT 2000 // 特殊日志缓冲（未获取到配置，没有竞争到共享内存的日志都写这里）
#define MTLOG_LOG_TIMEOUTMS_IN_SPEC 1000*60*2 // spec 日志过期时间

#define MTLOG_CUST_FLAG_C1_SET 1 // 自定义字段1设置
#define MTLOG_CUST_FLAG_C2_SET 2
#define MTLOG_CUST_FLAG_C3_SET 4
#define MTLOG_CUST_FLAG_C4_SET 8
#define MTLOG_CUST_FLAG_C5_SET 16
#define MTLOG_CUST_FLAG_C6_SET 32 

// 最多支持的内置监控插件数目
#define MAX_INNER_PLUS_COUNT 200

#pragma pack(1)

typedef struct 
{
	uint32_t dwCust_1; 
	uint32_t dwCust_2;
	int32_t iCust_3;
	int32_t iCust_4;
	char szCust_5[16];
	char szCust_6[32];
	uint8_t bCustFlag;
	char cReserved[8];
}MTLogCust;

typedef struct
{
	int32_t iCustVmemIndex; 
	uint32_t dwLogConfigId;
	uint32_t dwLogSeq;
	uint64_t qwLogTime;
	int32_t iAppId;
	int32_t iModuleId;
	uint16_t wLogType;
	int32_t iVarmemIndex; 
	char cReserved[16];
	char sLogContent[256];
}MTLog;

typedef struct
{
	volatile uint8_t bTryGetLogIndex; // 用于支持多进程多线程
	volatile int32_t iLogStarIndex; 
	volatile int32_t iWriteIndex;
	uint32_t dwGetLogIndexStartTime;
	char cReserved[28];
	MTLog sLogList[MTLOG_SHM_RECORDS_COUNT];
}MTLogShm;

// log config shm
#define MTLOG_MTREPORT_SHM_KEY 17001000 
#define MAX_LOG_CONFIG_COUNT 1050 // 私有1000 + 全局 50, log config 配置数目
#define MAX_APP_COUNT 120 
#define MAX_SERVICE_PER_SET 50 // 单个 set 最大包含的服务器数目(同 supper_log.h中的定义)

typedef struct
{
	int32_t iAppId;
	uint32_t dwAppSrvMaster; // app 日志主服务器, 主服务器有效则日志都发到主，否则随机发到备
	uint16_t wLogSrvPort;
	uint8_t bAppType; // 全局应用 bAppType 为 1， 否则为 0
	uint16_t wModuleCount;
	uint32_t dwSeq;

	// send log status
	uint32_t dwLastTrySendLogTime;
	uint32_t dwTrySendCount;
	uint32_t dwSendOkCount;
	uint32_t dwTrySendFailedCount;
	char cReserved[32];
}AppInfo;

#define SLOG_TEST_KEY_LEN 20 // 染色关键字最大长度
typedef struct
{
	uint8_t bKeyType;
	char szKeyValue[SLOG_TEST_KEY_LEN+1];
}SLogTestKey;

#define APP_LOG_READ_PER_TIME_MS 10 //  
#define MAX_SLOG_TEST_KEYS_PER_CONFIG 10 // 单个配置最大染色关键字数目
#define BWORLD_SLOG_TYPE_LOCAL 1 // 本地文件 log
#define BWORLD_SLOG_TYPE_NET 2 // 网络 log
#define BWORLD_SLOG_TYPE_TO_STD 4 // 标准输出
typedef struct
{
	uint32_t dwSeq;
	uint32_t dwCfgId;
	int32_t iAppId;
	int32_t iModuleId;
	int32_t iLogType;
	uint16_t wTestKeyCount;
	SLogTestKey stTestKeys[MAX_SLOG_TEST_KEYS_PER_CONFIG];

	// 日志频率限制, 策略: 当每分钟配置的限制超过 60 时，按秒计算，按秒计算后如果每秒钟
	// 配置超过 1000 进一步折算为每毫秒，折算粒度: 分钟，秒钟，100 毫秒，10 毫秒，1毫秒
	uint32_t dwSpeedFreq; // 配置的每分钟日志频率限制数

	volatile uint8_t bLogFreqUseFlag; // 用于支持多进程，多线程--日志频率限制
	char cReserved_1[8];
	int32_t iWriteLogCount; // 当前日志频率计算时间内已经写日志数目
	uint64_t qwLastFreqTime; // 上次日志频率开始计算的时间
	char cReserved[16];
}SLogConfig;

// 全局配置，还用于下发到 client, 同 supper_log.h 中的 MtSystemConfigClient
typedef struct 
{
    uint16_t wHelloRetryTimes; // hello 包重试次数
    uint16_t wHelloPerTimeSec; // hello 发送间隔
    uint16_t wCheckLogPerTimeSec; // 日志配置 check 时间
    uint16_t wCheckAppPerTimeSec; // app 配置 check 时间
    uint16_t wCheckServerPerTimeSec; // server 配置 check 时间
    uint16_t wCheckSysPerTimeSec; // system config 配置 check 时间
    uint32_t dwConfigSeq;
    uint8_t bAttrSendPerTimeSec; // attr 上报时间间隔
    uint8_t bLogSendPerTimeSec; // log 上报时间间隔
	uint8_t bReserved_2;
}MtSystemConfig;

// 插件信息
typedef struct _TInnerPlusInfo {
    char szPlusName[64]; // 插件名称
    int iPluginId; // 插件 id
    char szVersion[12]; // 配置文件中的插件版本
    char szBuildVer[12]; // 插件编译时的版本信息 
    int iLibVerNum; // 使用的开发库版本编号
    uint32_t dwLastReportAttrTime; // 最后一次 attr 上报时间
    uint32_t dwLastReportLogTime; // 最后一次 log 上报时间
    uint32_t dwPluginStartTime; // 插件启动时间

    uint32_t dwLastReportSelfInfoTime; // 插件自身最后一次信息上报时间
    uint32_t dwRep_LastReportLogTime;
    uint32_t dwRep_LastReportAttrTime;
    uint32_t dwLastHelloTime; // 存活校验
    uint32_t dwRep_LastHelloTime;
    uint8_t bCheckRet; // 服务端验证结果, 0 OK, 1 失败
    char sReserved[11];
}TInnerPlusInfo;

// agent client or api 共享内存结构
typedef struct
{
	// agent client 相关 --- local
	uint8_t bFirstHelloCheckOk;
	int32_t iMtClientIndex;
	int32_t iMachineId;
	uint32_t dwConnServerIp; // 接入　mtreport_server 的地址
	uint32_t dwReserved;
	char sRandKey[16];
	uint32_t dwPkgSeq;
	uint32_t dwLastHelloOkTime;
	uint32_t dwClientProcessStartTime;
	int32_t iAttrSrvMtClientIndex;
	int32_t iAppLogSrvMtClientIndex;

	// systen config
	MtSystemConfig stSysCfg;

	// app config
	uint32_t dwLastSyncAppConfigTime;
	uint16_t wAppConfigCount;
	AppInfo stAppConfigList[MAX_APP_COUNT];

	// app log config
	uint32_t dwLastSyncLogConfigTime;
	uint16_t wLogConfigCount;
	SLogConfig stLogConfig[MAX_LOG_CONFIG_COUNT];

	volatile uint8_t bModifyServerFlag; // for server sync

	// attr server config 
	uint16_t wAttrServerPort;
	uint32_t dwAttrSrvIp;

	// 日志上报相关 --- local
	volatile uint32_t dwLogSeq;
	int32_t iLogSpecialWriteIdx; // 日志临时存入 special 写索引
	int32_t iLogSpecialReadIdx; // 日志临时存入 special 读索引
	MTLog sLogListSpec[MTLOG_LOG_SPECIAL_COUNT];
	MTLogShm stLogShm[MTLOG_SHM_DEF_COUNT];
	uint16_t wSpecLogFailed; // 日志写 specail 失败，special 已满
	uint16_t wLogVmemFailed; // 日志超过预定义长度，超出部分写 vmem 失败
	uint16_t wLogFreqLimited; // 日志被频率限制
	uint16_t wLogTruncate; // 日志太长被截断 
	uint16_t wLogInShmFull; // 日志写满 shm 共享内存，可能是 agent client 读取日志太慢导致
	uint16_t wLogWriteInVmem; // 日志超过预定义长度，超出部分写 vmem 量
	uint16_t wLogCustInVmem; // 日志 cust 成功写入vmem 量
	uint16_t wLogCustVmemFailed; // 日志 cust写入vmem失败量
	uint32_t dwLogBySpecCount; // 日志尝试写入 special 的量
	uint32_t dwLogTypeLimited; // 日志类型不符合，日志丢弃量
	uint32_t dwFirstLogWriteTime; // 第一条日志时间
	uint32_t dwLastReportLogOkTime; // 最后一次上报日志成功的时间
	uint32_t dwWriteLogCount; // 总共写入的日志数
	uint64_t qwWriteLogBytes; // 总共写入的日志大小

	// 属性上报相关 --- local
	char cIsAttrInit;
	char szAttrList[(sizeof(_HashTableHead)+MTATTR_HASH_NODE*STATIC_HASH_ROW_MAX*(sizeof(_HashNodeHead)
		+sizeof(AttrNode)))*MTATTR_SHM_DEF_COUNT];
	int32_t iAttrSpecialCount; // 存入 spec 的 attr 数目
	AttrNode sArrtListSpec[MTATTR_SPECIAL_COUNT];

	// 字符串型监控点
	char cIsStrAttrInit;
	char szStrAttrBuf[(sizeof(_HashTableHead)+MTATTR_HASH_NODE*STATIC_HASH_ROW_MAX*(sizeof(_HashNodeHead)
		+sizeof(StrAttrNode)))];

	uint16_t wAttrReportFailed; // attr 上报失败量
	uint32_t dwAttrReportBySpecCount; // 总共尝试使用 spec 上报 attr 的次数
	uint32_t dwFirstAttrReportTime; // 首个属性上报时间
	uint32_t dwLastReportAttrOkTime; // 最后一次上报attr成功的时间
	uint32_t dwAttrReportCount; // 总共写入的属性上报数

	char cIsAgentRun;

	// vmem 相关 --- local
	char cIsVmemInit;
	VmemBufNode16 pV16Shm[VMEM_SHM_COUNT*VMEM_16_NODE_COUNT];
	VmemBufNode32 pV32Shm[VMEM_SHM_COUNT*VMEM_32_NODE_COUNT];
	VmemBufNode64 pV64Shm[VMEM_SHM_COUNT*VMEM_64_NODE_COUNT];
	VmemBufNode128 pV128Shm[VMEM_SHM_COUNT*VMEM_128_NODE_COUNT];
	VmemBufNode255 pV255Shm[VMEM_SHM_COUNT*VMEM_255_NODE_COUNT];

    // plugin info
    uint8_t bAddPluginInfoFlag;
    int iPluginInfoCount;
    TInnerPlusInfo stPluginInfo[MAX_INNER_PLUS_COUNT];

	int32_t iBindCloudUserId;
	uint32_t dwConnCfgServerIp; // 连接的远程服务器IP
	uint16_t wConnCfgServerPort; // 连接的远程服务器 port
	char cReserved[118];
}MTREPORT_SHM;

#pragma pack()

// 日志相关
typedef struct _TLogConfig{
	uint32_t dwLogCfgId; // 日志配置 id
	char cIsTest; // 日志染色标记
	MTLogCust stCust; // 日志自定义值
	SLogConfig *pCurConfigInfo; // 日志配置信息 
	char szLocalLogFile[256]; // 本地日志文件
	int iLocalLogType; // 本地日志记录类型, 为0表示不写本地日志
    char sReserved[128];
}TLogConfig;

// log struct 
typedef struct {
	char cIsInit;
	MTREPORT_SHM *pMtShm;
    int iPluginIndex; // 内置插件共享内存索引
	TLogConfig stLogInfo;
	char cIsAttrInit;
	SharedHashTable stAttrHash[MTATTR_SHM_DEF_COUNT];

	char cIsStrAttrInit;
	SharedHashTable stStrAttrHash;

	// vmem
	char cIsVmemInit;
	VmemBufNode16 *pV16Shm[VMEM_SHM_COUNT];
	VmemBufNode32 *pV32Shm[VMEM_SHM_COUNT];
	VmemBufNode64 *pV64Shm[VMEM_SHM_COUNT];
	VmemBufNode128 *pV128Shm[VMEM_SHM_COUNT];
	VmemBufNode255 *pV255Shm[VMEM_SHM_COUNT];
}MtReport;

extern MtReport g_mtReport;

uint32_t datetoui(const char *pdate);
char *uitodate(uint32_t dwTimeSec);
int MtReport_Init_ByKey(unsigned int iConfigId, int iShmKey, int iFlag);

#ifdef __cplusplus
}
#endif

#endif


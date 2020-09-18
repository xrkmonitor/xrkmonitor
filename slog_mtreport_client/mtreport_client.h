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

#ifndef _MTREPORT_CLIENT_H_
#define _MTREPORT_CLIENT_H_  (1)

#ifndef PRIu64 
#define PRIu64 "lu"
#endif 

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <string>
#include <map>
#include "sv_socket.h"
#include "sv_str.h"
#include "mt_attr.h"
#include "mt_log.h"
#include "mt_report.h"
#include "mtreport_protoc.h"
#include "mtreport_basic_pkg.h"

#ifndef MYSIZEOF
#define MYSIZEOF (unsigned)sizeof
#endif

// 输出 [C] 结构体成员信息 -- start
#define SHOW_FIELD_VALUE_INT(para_f) printf("\t " #para_f " :%d\n", pstr->para_f)
#define SHOW_FIELD_VALUE_UINT(para_f) printf("\t " #para_f " :%u\n", pstr->para_f)
#define SHOW_FIELD_VALUE_POINT(para_f) printf("\t " #para_f " :%p\n", pstr->para_f)
#define SHOW_FIELD_VALUE_UINT64(para_f) printf("\t " #para_f " :%" PRIu64 "\n", pstr->para_f)
#define SHOW_FIELD_VALUE_CSTR(para_f) printf("\t " #para_f " :%s\n", pstr->para_f)
#define SHOW_FIELD_VALUE_MASK_CSTR(para_len, para_f) printf("\t " #para_f " :%s\n", DumpStrByMask(pstr->para_f, para_len))
#define SHOW_FIELD_VALUE_BIN(para_len, para_f) printf("\t " #para_f "[%d] :%s\n", para_len, OI_DumpHex(pstr->para_f, 0, para_len))
#define SHOW_FIELD_VALUE_UINT_IP(para_f) printf("\t " #para_f ":%s\n", ipv4_addr_str(pstr->para_f))
#define SHOW_FIELD_VALUE_UINT_IP_COUNT(para_c, para_f) do { \
	printf("\t -------- " #para_f "[%d]:\n", pstr->para_c); \
	for(int i=0; i < pstr->para_c; i++) \
		printf("\t " #para_f ":%s\n", ipv4_addr_str(pstr->para_f[i])); \
}while(0)

#define SHOW_FIELD_VALUE_UINT_TIME(para_f) printf("\t " #para_f ":%s\n", uitodate(pstr->para_f))
#define SHOW_FIELD_VALUE_INT_COUNT(para_c, para_f) do { \
	printf("\t -------- " #para_f "[%d]:\n", pstr->para_c); \
	for(int i=0; i < pstr->para_c; i++) \
		printf("\t " #para_f "[%d]:%d\n", i, pstr->para_f[i]); \
}while(0)
#define SHOW_FIELD_VALUE_UINT_COUNT(para_c, para_f) do { \
	printf("\t -------- " #para_f "[%d]:\n", pstr->para_c); \
	for(int i=0; i < pstr->para_c; i++) \
		printf("\t " #para_f "[%d]:%u\n", i, pstr->para_f[i]); \
}while(0)
#define SHOW_FIELD_VALUE_UINT_COUNT_2(para_c, para_f) do { \
	printf("\t -------- " #para_f "[%d]:\n", para_c); \
	for(int i=0; i < para_c; i++) \
		printf("\t " #para_f "[%d]:%u\n", i, pstr->para_f[i]); \
}while(0)
#define SHOW_FIELD_VALUE_ARR_UINT_NOT_ZERO(para_c, para_f) do { \
	for(int i=0; i < para_c; i++) \
		if(para_f[i] != 0) \
			printf("\t " #para_f "[%d]:%u\n", i, pstr->para_f[i]); \
}while(0)
#define SHOW_FIELD_VALUE_FUN(para_f, para_fun) do { \
	printf("\t -------- " #para_f ": --------\n"); \
	para_fun(pstr->para_f); \
}while(0)
#define SHOW_FIELD_VALUE_FUN_COUNT(para_c, para_f, para_fun) do { \
	printf("\t -------- " #para_f "[%d]: --------\n", pstr->para_c); \
	for(int i=0; i < pstr->para_c; i++) { \
		printf("\t ------------ " #para_f "[%d]: \n", i); \
		para_fun(pstr->para_f[i]); \
	} \
}while(0)
#define SHOW_FIELD_VALUE_FUN_COUNT_2(para_c, para_f, para_fun) do { \
	printf("\t -------- " #para_f "[%d]: --------\n", para_c); \
	for(int i=0; i < para_c; i++) { \
		printf("\t ------------ " #para_f "[%d]: \n", i); \
		para_fun(pstr->para_f[i]); \
	} \
}while(0)
// 输出结构体成员信息 -- end --- 

bool LogFreqCheck();

#define ERROR_LOG(fmt, ...) do { if((32&stConfig.iLocalLogType) && stConfig.fpLogFile != NULL) fprintf(stConfig.fpLogFile, "[%s] err: (%s:%s:%d) "fmt"\n", stConfig.szTimeCur, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); fflush(stConfig.fpLogFile); } while(0)                        

#define FATAL_LOG(fmt, ...) do { if((64&stConfig.iLocalLogType) && stConfig.fpLogFile != NULL) fprintf(stConfig.fpLogFile, "[%s] fatal: (%s:%s:%d) "fmt"\n", stConfig.szTimeCur, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); fflush(stConfig.fpLogFile); } while(0)                        

#define REQERR_LOG(fmt, ...) do { if(LogFreqCheck() && (16&stConfig.iLocalLogType) && stConfig.fpLogFile != NULL) fprintf(stConfig.fpLogFile, "[%s] reqerr: (%s:%s:%d) "fmt"\n", stConfig.szTimeCur, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); fflush(stConfig.fpLogFile); } while(0)                        

#define WARN_LOG(fmt, ...) do { if(LogFreqCheck() && (8&stConfig.iLocalLogType) && stConfig.fpLogFile != NULL) fprintf(stConfig.fpLogFile, "[%s] warn: (%s:%s:%d) "fmt"\n", stConfig.szTimeCur, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); fflush(stConfig.fpLogFile); } while(0)                        

#define INFO_LOG(fmt, ...) do { if(LogFreqCheck() && (4&stConfig.iLocalLogType) && stConfig.fpLogFile != NULL) fprintf(stConfig.fpLogFile, "[%s] info: (%s:%s:%d) "fmt"\n", stConfig.szTimeCur, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); fflush(stConfig.fpLogFile); } while(0)                        

#define DEBUG_LOG(fmt, ...) do { if(LogFreqCheck() && (2&stConfig.iLocalLogType) && stConfig.fpLogFile != NULL) fprintf(stConfig.fpLogFile, "[%s] debug: (%s:%s:%d) "fmt"\n", stConfig.szTimeCur, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); fflush(stConfig.fpLogFile); } while(0)                        

#define PLUGIN_INST_LOG(fmt, ...) do { TryOpenPluginInstallLogFile(pct); if(stConfig.fpPluginInstallLogFile != NULL) { fprintf(stConfig.fpPluginInstallLogFile, "[%s] (%s:%s:%d) "fmt"\n", stConfig.szTimeCur, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); fclose(stConfig.fpPluginInstallLogFile); stConfig.fpPluginInstallLogFile = NULL; } } while(0) 

#define MTREPORT_CONFIG "./slog_mtreport_client.conf"
#define MTREPORT_CLIENT_CONFIG_ID 57 // monitor 系统中 mtreport_client 的log配置id
#define MAX_APP_LOG_PKG_LENGTH 1200  // 这个不能太大，整个udp 包长度超过 mtu(1500) 时有些系统发包会失败
#define MAX_ATTR_PKG_LENGTH 1200 
#define COUNT_APP_LOG_PACKET_SEND_PER 20
#define COUNT_ATTR_PACKET_SEND_PER 10

#define MAX_SERVER_COUNT 100
#define MTREPORT_ERROR_LINE -__LINE__

#define TIMER_HASH_NODE_COUNT 1117
#define TIMER_HASH_SHM_KEY_MAIN 17270113

#ifndef MT_DEBUG
#define PKG_TIMEOUT_MAX_MS 60000 // 最大超时时间
#else
#define PKG_TIMEOUT_MAX_MS 2000 // 最大超时时间
#endif


// 注意 PKG_TIMEOUT_MS 定义至少大于 1000
#ifndef MT_DEBUG
#define PKG_TIMEOUT_MS 5000 // 默认超时时间，单位毫秒(ms)
#else
#define PKG_TIMEOUT_MS 2000 // 默认超时时间，单位毫秒(ms)
#endif

#define RANDKEY_TIMEOUT_MINUTE 1440 // def, randkey 超时时间

#ifndef MT_DEBUG
#define TRY_MAX_TIMES_PER_SERVER 2  // def, 单台服务器数据包重试次数
#else
#define TRY_MAX_TIMES_PER_SERVER 1 // def, 单台服务器数据包重试次数
#endif

#define CMD_HELLO_SEND_TIME_MS 5*1000 // def, hello 包发送时间间隔

#ifndef MT_DEBUG
#define CMD_CHECK_CONFIG_TIME_MS  60*1000 // def, 配置check 间隔时间
#else
#define CMD_CHECK_CONFIG_TIME_MS  5*1000 // def, 配置check 间隔时间
#endif

#define MAX_SERVICE_PER_SET 50 // 单个 set 最大包含的服务器数目, 同 supper_log.h

#define MAX_APP_TYPE_READ_PER_EACH 10
typedef struct 
{
	int iReadAppId;
	int iReadAppLogBufLen;
	int iAppInfoIdx;
	char sAppLogBuf[MAX_APP_LOG_PKG_LENGTH]; 
}TAppLogRead;

//dwRestartFlag 相关处理标记
#define RESTART_FLAG_CHECK_SYSTEM_CONFIG 1
#define RESTART_FLAG_CHECK_LOG_CONFIG 2
#define RESTART_FLAG_CHECK_APP_CONFIG 4

#define MAX_ATTR_READ_PER_EACH 120 // 这个不能太大，否则会导致udp 包超过 1500 字节，某些系统可能发包会失败
#define MAX_STR_ATTR_READ_PER_EACH 15 
typedef struct
{
	// 与 server 交互用, server 地址信息
	char szSrvIp_master[32];
	int iSrvPort;

	// 数据包临时缓存
	char sSessBuf[TIMER_DATA_MAX_LENGTH+sizeof(PKGSESSION)];
	unsigned uiSessDataLen;

	// 当前工作目录
	char szCurPath[256];

	// 插件目录
	char szPlusPath[256];
	std::map<std::string, void *> mapPlus;

	// 创建新数据包之前需要设置以下值 使用 sSessBuf
	char *pPkg;
	int iPkgLen;
	PKGSESSION *pPkgSess;
	ENVSESSION *pEnvSess;

	// 用户配置
	char szUserKey[33];
	int iEnableEncryptData;

	// log 上报
	uint8_t bReadAppTypeCount;
	TAppLogRead stAppLogRead[MAX_APP_TYPE_READ_PER_EACH];

	// attr 上报
	uint16_t wReadAttrCount;
	AttrNode stAttrRead[MAX_ATTR_READ_PER_EACH];

	// str attr 上报
	uint16_t wReadStrAttrCount;
	StrAttrNode stStrAttrRead[MAX_STR_ATTR_READ_PER_EACH];

	// 在服务器上的配置相关
	MTREPORT_SHM *pReportShm;

	// 本地变量
	char szTimeCur[64];
	uint32_t dwCurTime;
	uint64_t qwCurTime;
	struct timeval stTimeCur;
	int iTimerHashKeyMain;
	char szLocalIP[32];
	uint32_t dwLocalIp;
	uint32_t dwProcessId;
	uint32_t uiMaxDataLenToVmem;

	char szCustAttrSrvIp[18];
	int iCustAttrSrvPort;

	FILE *fpLogFile;
	char szLocalLogFile[256];
	int iLocalLogType;
	int iMaxLocalLogFileSize;
	int iLogLimitPerSec;
	int iLogWriteCount;
	uint32_t dwLastCheckLogFreqTime;

	int iAttrSocketIndex;
	int iConfigSocketIndex;
	int iLogSocketIndex;

	int iMaxRunMins;
	int iDisablePlus;

	// 云端地址，用于一键部署插件等功能
	char szCloudUrl[256];
	char szOs[32];
	char szOsArc[20];
	char szLibcVer[32];
	char szLibcppVer[32];

	// 重启进程更新全部配置标记
	uint32_t dwRestartFlag;
	bool bCheckHelloStart;

	FILE *fpPluginInstallLogFile;
}CONFIG;


#define TIME_SEC_TO_MS(sec) sec*1000
#define GET_DIFF_TIME_MS(sec, usec) ((uint32_t)stConfig.stTimeCur.tv_sec > sec ? ((stConfig.stTimeCur.tv_sec-sec-1)*1000+(stConfig.stTimeCur.tv_usec+1000000-usec)/1000) : ((stConfig.stTimeCur.tv_sec-sec)*1000+(stConfig.stTimeCur.tv_usec-usec)/1000))

typedef struct {
	std::string strConfigName;
	std::string strConfigValue;
}TConfigItem;
typedef std::list<TConfigItem*> TConfigItemList;

void UpdateConfigFile(const char *pfile, TConfigItemList & list);
void ReleaseConfigList(TConfigItemList & list);
int get_cmd_result(const char *cmd, std::string &strResult);

extern CONFIG stConfig;
extern std::string g_strCmpTime;
extern const std::string g_strVersion;

#endif


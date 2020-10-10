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

#ifndef __SV_STRUCT__ 
#define __SV_STRUCT__ 1

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_APP_LOG_PKG_LENGTH 1400

// 输出 [C++] 结构体成员信息 -- start
#define SHOW_FIELD_VALUE_INT(para_f) printf("\t " #para_f " :%d\n", para_f)
#define SHOW_FIELD_VALUE_UINT(para_f) printf("\t " #para_f " :%u\n", para_f)
#define SHOW_FIELD_VALUE_UINT64(para_f) printf("\t " #para_f " :%" PRIu64 "\n", para_f)
#define SHOW_FIELD_VALUE_STR_MAX(chfld, max, para_str) do { \
	printf("\t --------- "#para_str"[%d]:\n", max); \
	for(int j=0; j < max; j++) { \
		if(para_str[j].chfld == 0) \
			continue; \
		printf("\t "#para_str"[%d]:\n", j); \
		para_str[j].Show(); \
		printf("\n"); \
	} \
}while(0)
#define SHOW_FIELD_VALUE_STR_COUNT(para_c, chfld, max, para_str) do { \
	printf("\t --------- "#para_str"[%d]:\n", para_c); \
	for(int i=0,j=0; i < para_c && j < max; j++) { \
		if(para_str[j].chfld == 0) \
			continue; \
		printf("\t "#para_str"[%d]:\n", j); \
		para_str[j].Show(); \
		i++; \
		printf("\n"); \
	} \
}while(0)
#define SHOW_FIELD_VALUE_CSTR(para_f) printf("\t " #para_f " :%s\n", para_f)
#define SHOW_FIELD_VALUE_MASK_CSTR(para_len, para_f) printf("\t " #para_f " :%s\n", DumpStrByMask(pstr->para_f, para_len))
#define SHOW_FIELD_VALUE_BIN(para_len, para_f) printf("\t " #para_f "[%d] :%s\n", para_len, OI_DumpHex(para_f, 0, para_len))
#define SHOW_FIELD_VALUE_UINT_IP(para_f) printf("\t " #para_f ":[%s]-[%u]\n", ipv4_addr_str(para_f), para_f)
#define SHOW_FIELD_VALUE_UINT_IP_COUNT(para_c, para_f) do { \
    printf("\t -------- " #para_f "[%d]:\n", para_c); \
    for(int i=0; i < para_c; i++) \
        printf("\t " #para_f ":%s\n", ipv4_addr_str(para_f[i])); \
}while(0)
#define SHOW_FIELD_VALUE_UINT_TIME(para_f) printf("\t " #para_f ":%s\n", uitodate(para_f))
#define SHOW_FIELD_VALUE_STR(para_str) do { \
	printf("\t --------- "#para_str":\n"); \
	para_str.Show(); \
}while(0)
#define SHOW_FIELD_VALUE_INT_COUNT(para_c, para_f) do { \
	printf("\t --------- " #para_f "[%d]:\n", para_c); \
	for(int i=0; i < para_c; i++) \
		printf("\t " #para_f "[%d]:%d\n", i, para_f[i]); \
}while(0)
#define SHOW_FIELD_VALUE_UINT_COUNT(para_c, para_f) do { \
	printf("\t --------- " #para_f "[%d]:\n", para_c); \
	for(int i=0; i < para_c; i++) \
		printf("\t " #para_f "[%d]:%u\n", i, para_f[i]); \
}while(0)
#define SHOW_FIELD_VALUE_ARR_INT_NOT_ZERO(para_c, para_f) do { \
	printf("\t --------- " #para_f "[%d]:\n", para_c); \
	for(int i=0; i < para_c; i++) \
		if(para_f[i] != 0) \
			printf("\t " #para_f "[%d]:%d\n", i, para_f[i]); \
}while(0)
#define SHOW_FIELD_VALUE_ARR_UINT_NOT_ZERO(para_c, para_f) do { \
	printf("\t --------- " #para_f "[%d]:\n", para_c); \
	for(int i=0; i < para_c; i++) \
		if(para_f[i] != 0) \
			printf("\t " #para_f "[%d]:%u\n", i, para_f[i]); \
}while(0)
#define SHOW_FIELD_VALUE_FUN(para_f, para_fun) do { \
	printf("\t -------- " #para_f ":\n"); \
    para_fun(para_f); \
}while(0)
#define SHOW_FIELD_VALUE_FUN_COUNT(para_c, para_f, para_fun) do { \
    printf("\t -------- " #para_f "[%d]:\n", para_c); \
    for(int i=0; i < para_c; i++) { \
		printf("\t ------------ " #para_f "[%d]: \n", i); \
        para_fun(para_f[i]); \
	}\
}while(0)
// 输出结构体成员信息 -- end --- 

#ifndef COMM_LIB_BUG
#define COMM_LIB_BUG ;
#endif

#define ntohll(n64) (((uint64_t)ntohl(n64)) << 32) | htonl(n64 >> 32)
#define htonll(h64) (((uint64_t)ntohl(h64)) << 32) | htonl(h64 >> 32)

// set bit b in value n
#define SET_BIT(n, b) n|=b 
// check is set bit b in value n
#define IS_SET_BIT(n, b) (n&b)
// clear bit b in value n
#define CLEAR_BIT(n, b) n&=~b

#define SPKG '['   // tcp, udp packet data start
#define EPKG ']'   // tcp, udp packet data start
#define SPKG_PB '{'   // tcp, udp packet data start
#define EPKG_PB '}'   // tcp, udp packet data start
#define S_REDIRECT_PKG '(' // local redirect pkg - ( + dstip + dstport + len + pkg[len]

#define MAX_SIGNATURE_LEN 128

// system macro
#define HTTP_HEADER_KV_CHAR ' '
#define HTTP_HEADER_PARSE_STR "\r\n"
#define MAX_UDP_REQ_PACKET_SIZE 4096
#define MAX_UDP_RESP_PACKET_SIZE 15000 
#define MAX_UDP_HTTP_RESP_HEADER_SIZE 4096 
#define MAX_UDP_HTTP_RESP_BODY_SIZE 8192 
#define TLV_HTTP_BODY_KV_CHAR '=' 

// ------------- cmd define start --------------------

// monitor system cmd: 201-400
#define CMD_MONI_SEND_HELLO_FIRST 201 // 首个 hello 命令
#define CMD_MONI_SEND_HELLO 202 // hello 命令
#define CMD_MONI_SEND_ATTR 203
#define CMD_MONI_CLIENT_INIT 204
#define CMD_MONI_PUSH_CONFIG 205 // 服务器 push 配置命令
#define CMD_MONI_CHECK_LOG_CONFIG 206
#define CMD_MONI_CHECK_APP_CONFIG 207
#define CMD_MONI_SEND_LOG 209
#define CMD_MONI_REPORT_SERVER_STATUS 211
#define CMD_MONI_GET_SERVICE_SERVER 212
#define CMD_MONI_CHECK_SYSTEM_CONFIG 213
#define CMD_MONI_SEND_STR_ATTR 214 // client send str attr
#define CMD_CGI_SEND_ATTR 215 
#define CMD_CGI_SEND_STR_ATTR 216 
#define CMD_CGI_SEND_LOG 217 
#define CMD_MONI_SEND_PLUGIN_INFO 218 // client send plugin info 
#define CMD_MONI_PREINSTALL_REPORT 219 // client send 一键部署进度 
#define CMD_SEND_PLUGIN_CONFIG 220

// monitor system cmd: 400 - 500 用于 pc 告警客户端
#define CMD_MONI_PC_DETECT_SERVER 400
#define CMD_MONI_PC_CLIENT_HELLO 401
#define CMD_MONI_PC_PUSH_WARN 402
#define CMD_MONI_PC_PUSH_WARN_NO_ACK 403

// cmd for protobuf 501-700 -- protobuf 协议使用命令范围
#define CMD_INNER_SEND_REALINFO 501 

// monitor system s2c cmd: 701-999
#define CMD_MONI_S2C_LOG_CONFIG_NOTIFY 701 
#define CMD_MONI_S2C_APP_CONFIG_NOTIFY 702 
#define CMD_MONI_S2C_PRE_INSTALL_NOTIFY 703 
#define CMD_MONI_S2C_MACH_ORP_PLUGIN_REMOVE 704 
#define CMD_MONI_S2C_MACH_ORP_PLUGIN_ENABLE 705
#define CMD_MONI_S2C_MACH_ORP_PLUGIN_DISABLE 706
#define CMD_MONI_S2C_MACH_ORP_PLUGIN_MOD_CFG 707

// monitor system s2c cmd: 1000-1100 用于 pc 告警客户端
#define CMD_MONI_PC_S2C_WARN_INFO 1000


// ------------- cmd define end --------------------
#define TLV_ACK_ERROR 5

// ------------------------------ tlv define start -------------------
// monitor system tlv : 201 --- 1000
#define TLV_MONI_SEND_LOG 201
#define TLV_MONI_SEND_ATTR 202
#define TLV_MONI_CLIENT_INIT 203
#define TLV_MONI_COMM_INFO 204
#define TLV_MONI_CONFIG_ADD 205
#define TLV_MONI_CONFIG_MOD 206
#define TLV_MONI_CONFIG_DEL 207

// ------------------------------ tlv define end -------------------


// ------------------------------ signature start  -------------------
// monitor signatur 1-64
#define PC_SIGNATURE_MAGIC_STRING "sdfsdfs@#@5ksdkxrk"
#define MT_SIGNATURE_TYPE_HELLO_FIRST 1
#define MT_SIGNATURE_TYPE_PC_DETECT 2
#define MT_SIGNATURE_TYPE_COMMON 101

// ------------------------------ signature end -------------------



// top --------------------------
#pragma pack(1)

typedef struct
{
	char cRedirectPkg;
	uint32_t dwDstIp;
	uint16_t wDstPort;
	uint16_t wPkgLen;
}TRedirectPkgHead;

// for top app packet format 
// udp req packet is : SPKG + ReqPkgHead + TSignature + TopApiPkgHead + TPkgBody + EPKG
//	req tlv: TOP_TLV_POST_HTTP_URL | TOP_TLV_POST_BODY_KV
//  req tlv: TOP_TLV_CONFIG_NOTIFY  --- for cmd: CMD_TOP_CONFIG_NOTIFY

// udp ack packet is : SPKG + ReqPkgHead + TSignature + TopApiPkgHead + TPkgBody + EPKG
//	ack tlv: TLV_HTTP_RESP_HEADER | TLV_HTTP_RESP_BODY
//  ack tlv: TOP_TLV_CONFIG_NOTIFY  --- for cmd: CMD_TOP_CONFIG_NOTIFY
//  if ack error have tlv: TLV_ACK_ERROR

#define SIZE_OF_SESS_USER_ID 20
#define SIZE_OF_SESS_USER_NICK 32 
#define SIZE_OF_SESS_TOKEN 64
#define SIZE_OF_SESS_TOKEN_TYPE 20

#define REQ_PKG_HEAD_VERSION 1
typedef struct
{
	uint32_t dwCmd;
	uint32_t dwSeq;
	uint16_t wPkgLen;
	uint16_t wToPkgBody;
	uint8_t bRetCode;
	uint8_t bResendTimes;
	uint16_t wVersion;
	uint32_t dwRespMagicNum;
	char sEchoBuf[32];
	uint64_t qwSessionId;
	char sReserved[8];
}ReqPkgHead;

typedef struct
{
	uint8_t bSigType;
	uint16_t wSigLen;
	char sSigValue[0];
}TSignature;

typedef struct
{
	uint16_t wType;
	uint16_t wLen;
	uint8_t sValue[0];
}TWTlv;

typedef struct
{
	uint8_t bTlvNum;
	TWTlv stTlv[0];
}TPkgBody;

// for sys monitor  --- start
// udp req packet is : SPKG + ReqPkgHead + TSignature + MonitorPkgLogInfo + vPkgBody + EPKG
//  req tlv: CMD_MONI_CLIENT_INIT --- for cmd: TLV_MONI_CLIENT_INIT 
//  req tlv: TLV_MONI_SEND_LOG --- for cmd: CMD_MONI_SEND_LOG 
//  req tlv: TLV_MONI_SEND_ATTR --- for cmd:CMD_MONI_SEND_ATTR 

// udp ack packet is : SPKG + ReqPkgHead + TSignature + MonitorPkgLogInfo + TPkgBody + EPKG
//  success : no tlv 
//  if ack error have tlv: TLV_ACK_ERROR

typedef struct
{
	uint32_t dwSeq;
	uint32_t dwCmd;
	uint8_t bEnableEncryptData;
	char szReserved[16];
}MonitorCommSig; // monitor 通用签名结构

typedef struct
{
	int32_t iMtClientIndex;
	int32_t iMachineId;
	uint32_t dwReserved_1;
	uint16_t wReserved_1;
	uint32_t dwReserved_2;
	uint16_t wReserved_2;
	char sReserved[32];
}TlvMoniCommInfo; // for TLV_MONI_COMM_INFO

#define MONI_CHECK_CONFIG_ADD 1
#define MONI_CHECK_CONFIG_DELETE 2
#define MONI_CHECK_CONFIG_UPDATE 3

// -----------------------for log config 
#define MONI_CONFIG_FLAG_USE 1 // client
#define MONI_CONFIG_FLAG_CHECKED 2 // server tmp
#define MONI_CONFIG_FLAG_CHECK_ADD 4 // server,client
#define MONI_CONFIG_FLAG_CHECK_UPDATE 8 // server,client
#define MONI_CONFIG_FLAG_CHECK_DELETE 16 // server,client

#define MAX_SLOG_TEST_KEYS_PER_CONFIG 10 // 单个配置最大染色关键字数目
#define SLOG_TEST_KEY_LEN 20 // 染色关键字最大长度
typedef struct
{
	uint8_t bKeyType;
	char szKeyValue[SLOG_TEST_KEY_LEN+1];

	void Show() {
		SHOW_FIELD_VALUE_UINT(bKeyType);
		SHOW_FIELD_VALUE_CSTR(szKeyValue);
	}
}SLogTestKey;

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
}MtSLogConfig;

typedef struct
{
	uint32_t dwCfgId;
	uint8_t bConfigCheck;
	MtSLogConfig stInfo[0];
}TlvLogConfigInfo;
// -------------------------- end for 

typedef struct 
{
	int32_t iAppId;
	uint32_t dwLogHostId;
	uint16_t wCltReqLogCount;
	uint16_t wSrvWriteLogCount;
	char sBodyTlvMd5[16];  // body tlv md5 防篡改
}MonitorPkgLogInfo;

typedef struct 
{
	uint32_t dwCust_1;
	uint32_t dwCust_2;
	int32_t iCust_3;
	int32_t iCust_4;
	char szCust_5[16];
	char szCust_6[32];
	uint32_t dwLogConfigId; // 产生该条日志的配置编号
	uint32_t dwLogHostId; // 产生该条日志的机器
	int32_t iModuleId;
	uint16_t wLogType;
	uint64_t qwLogTime;
	char sLog[0];
}TlvLogInfo; // fot TLV_MONI_SEND_LOG

// mtreport_client 上报的内容
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

// for sys monitor  --- end 

#pragma pack()


#define BWORLD_ERR_TLV 0x7fff

#define DIM(x) (sizeof(x) / sizeof((x)[0]))

int SetWTlv(TWTlv *ptlv, uint16_t wType, uint16_t wLen, const char *pValue);
TWTlv * GetWTlvByType(uint16_t wType, uint16_t wTlvNum, TWTlv *ptlv);
void* GetWTlvValueByType(uint16_t wType, uint16_t wTlvNum, TWTlv *ptlv);
void* GetWTlvValue(uint16_t wType, TWTlv *ptlv, int iTlvLen);
TWTlv * GetWTlvByType2(uint16_t wType, TPkgBody *pstTlv);
void* GetWTlvValueByType2(uint16_t wType, TPkgBody *pstTlv);
int CheckPkgBody(TPkgBody *pstBody, uint16_t wBodyLen);
TWTlv * GetWTlvByType2_list(uint16_t wType, TPkgBody *pstTlv, int *piStartNum);
TWTlv * GetWTlvType2_list(TPkgBody *pstTlv, int *piStartNum);
void TlvLogInfoNtoH(TlvLogInfo *pLog);
void MonitorPkgLogInfoHtoN(MonitorPkgLogInfo *pinfo);
void MonitorPkgLogInfoNtoH(MonitorPkgLogInfo *pinfo);
void LogInfoNtoH(LogInfo *pLog);

#endif


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

#ifndef _MTREPORT_20141117_H_
#define _MTREPORT_20141117_H_ (1)

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#ifndef ERROR_LINE 
#define ERROR_LINE -__LINE__
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// 共享内存默认 key
#define MT_REPORT_DEF_SHM_KEY 201412275

// 单条日志内容的最大长度
#define MTREPORT_LOG_MAX_LENGTH 1024 

#ifndef MTLOG_TYPE_OTHER
#define MTLOG_TYPE_OTHER 1 // 除以下类型以外的类型，用户可自行安排使用
#define MTLOG_TYPE_DEBUG 2 // 调试日志 -- 调试程序用，正式上线请不要打开
#define MTLOG_TYPE_INFO 4 // 信息日志 -- 用于关键信息的log
#define MTLOG_TYPE_WARN 8 // 警告日志 -- 程序非关键性的校验错误
#define MTLOG_TYPE_REQERR 16 // 请求错误 -- 由外部请求触发的错误，比如外部请求的参数不合法
#define MTLOG_TYPE_ERROR 32 // 错误日志 -- 程序自身校验错误，可能影响程序正常运行
#define MTLOG_TYPE_FATAL 64  // 严重错误 -- 程序不能继续运行时可以使用此日志类型记录日志
#endif

// 内置监控插件初始化函数, 成功返回码大于等于 0， 失败返回小于 0
// pPlusName 插件名，长度不超过 64 个字节
// iConfigId 日志配置id, 如果为0则不上报日志
// pszLocalLogFile 本地日志文件, 为 NULL 则不写本地日志
// iLocalLogType 本地日志文件记录的日志类型
int MtReport_Plus_Init(const char *pPlusName, int iConfigId, const char *pszLocalLogFile, int iLocalLogType);

// 日志和监控点 api 初始化, 成功返回码大于等于 0， 失败返回小于 0
// iConfigId 日志配置id, 如果为0则不上报日志
// pszLocalLogFile 本地日志文件, 为 NULL 则不写本地日志
// iLocalLogType 本地日志文件记录的日志类型
// iShmKey 共享内存 key， 如果为0表示使用默认值，注意要同 slog_mtreport_client 中使用的 key 一致
int MtReport_Init(int iConfigId, const char *pszLocalLogFile, int iLocalLogType, int iShmKey);

//  累加 attr 监控点值
int MtReport_Attr_Add(int32_t attr, int32_t iValue);

//  设置 attr 监控点值
int MtReport_Attr_Set(int32_t attr, int32_t iValue);

//  累加 字符串型监控点值 
int MtReport_Str_Attr_Add(int32_t attr, const char *pstr, int32_t iValue);

//  设置 字符串型监控点值 
int MtReport_Str_Attr_Set(int32_t attr, const char *pstr, int32_t iValue);

// 每条内存日志都记录的基本信息
#define SLOG_BASE_FMT " [%s:%s:%d] "
#define SLOG_BASE_VAL __FILE__, __FUNCTION__, __LINE__

#define MYSIZEOF (unsigned)sizeof
#define MYSTRLEN (unsigned)strlen

// 日志接口, 与 printf 函数一样
#define MtReport_Log_Other(fmt, ...) MtReport_Log(MTLOG_TYPE_OTHER, SLOG_BASE_FMT""fmt, SLOG_BASE_VAL, ##__VA_ARGS__)
#define MtReport_Log_Debug(fmt, ...) MtReport_Log(MTLOG_TYPE_DEBUG, SLOG_BASE_FMT""fmt, SLOG_BASE_VAL, ##__VA_ARGS__)
#define MtReport_Log_Info(fmt, ...) MtReport_Log(MTLOG_TYPE_INFO, SLOG_BASE_FMT""fmt, SLOG_BASE_VAL, ##__VA_ARGS__)
#define MtReport_Log_Warn(fmt, ...) MtReport_Log(MTLOG_TYPE_WARN, SLOG_BASE_FMT""fmt, SLOG_BASE_VAL, ##__VA_ARGS__)
#define MtReport_Log_Reqerr(fmt, ...) MtReport_Log(MTLOG_TYPE_REQERR, SLOG_BASE_FMT""fmt, SLOG_BASE_VAL, ##__VA_ARGS__)
#define MtReport_Log_Error(fmt, ...) MtReport_Log(MTLOG_TYPE_ERROR, SLOG_BASE_FMT""fmt, SLOG_BASE_VAL, ##__VA_ARGS__)
#define MtReport_Log_Fatal(fmt, ...) MtReport_Log(MTLOG_TYPE_FATAL, SLOG_BASE_FMT""fmt, SLOG_BASE_VAL, ##__VA_ARGS__)


#define MtReport_Log_Check(p) do { if(!p) \
	MtReport_Log(32, SLOG_BASE_FMT" check:"#p" failed", SLOG_BASE_VAL); }while(0)

// 日志裸接口，不建议使用，推荐用日志宏
int MtReport_Log(int iLogType, const char *pszFmt, ...);

// 日志高级功能：自定义字段
// 日志自定义字段 -- 设置后未改变前的日志都使用一样的值,直到下次改变
// 说明：支持多进程，不支持多线程，多个线程设置时只有一个线程的设置生效
void MtReport_Log_SetCust1(uint32_t dwCust);
void MtReport_Log_ClearCust1(); // 清除，cust1 不再使用，直到再次设置

void MtReport_Log_SetCust2(uint32_t dwCust);
void MtReport_Log_ClearCust2();

void MtReport_Log_SetCust3(int32_t iCust);
void MtReport_Log_ClearCust3();

void MtReport_Log_SetCust4(int32_t iCust);
void MtReport_Log_ClearCust4();

void MtReport_Log_SetCust5(const char *pstrCust); // 参数 pstrCust 指向的字符串小于16个字符
void MtReport_Log_ClearCust5(); // 参数 pstrCust 指向的字符串小于16个字符

void MtReport_Log_SetCust6(const char *pstrCust); // 参数 pstrCust 指向的字符串小于32个字符
void MtReport_Log_ClearCust6(); // 参数 pstrCust 指向的字符串小于32个字符
// 日志自定义字段 -- end

// 日志高级功能：染色日志
// 检查是否命中染色标记的回调接口
// 参数: bKeyType 染色关键字类型
// 参数: key 染色关键字
// 参数: pdata 用户上下文数据
// 该函数检查 pdata 数据是否命中 bKeyType, key 染色配置
typedef int (*FunCheckTestCallBack)(uint8_t bKeyType, const char *key, const void *pdata);
// 对参数 pdata 通过 isTest 回调遍历染色配置
// isTest 返回1,表示命中染色,设置染色标记; isTest 都返回 0,则清除染色标记
void MtReport_Check_Test(FunCheckTestCallBack isTest, const void *pdata);
// 清除染色标记
void MtReport_Clear_Test();


// 以下接口函数一般用不到 ----------------------------------

// 日志接口，推荐用定义的宏
// 返回码说明
// -1 -- 日志接口没有初始化
// -2 -- 日志类型不需要 log
// -3 -- 日志被频率限制
// -4 -- 写日志多线程竞争资源失败
// -5 -- 写日志失败
// 0 -- 写日志成功
int MtReport_Log(int iLogType, const char *pszFmt, ...);

#ifdef __cplusplus
}
#endif

#endif


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

#ifndef __SV_COREDUMP_H_21130505__
#define __SV_COREDUMP_H_21130505__ 1

#define BACKTRACE_DUMP_LEVELS 10
#define BACKTRACE_DEF_DUMP_FILE "/tmp/backtrace_dump.log"

typedef void (*SigCallBackFun)(int sig, const char *pszFile, void *pdata);

// 注册信号处理函数 (编译时请加上 -rdynamic 参数)
// pszDumpFile --- 堆栈信息转存的文件, 如果为 NULL 则输出到标准输出
// cb --- cb 信号处理回调函数，如果为 NULL, 收到信号后程序退出，否则调用 cb
void RegisterSignal(const char *pszDumpFile, void *pdata, SigCallBackFun cb);

#endif



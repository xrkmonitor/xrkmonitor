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

#ifndef __BMH_H_20170429__
#define __BMH_H_20170429__ 1

#define BMH_JUMP_TABLE_SIZE 256

// 获取跳表
void bmh_get_jump_table(char *pszPat, int *piJmpTab);

// bmh 匹配
// 如果 pszTxt 中含有 pszPat 子串，返回第一个匹配的索引，否则返回 -1
int bmh_is_match(const char *pszTxt, char *pszPat, int *piJmpTab);

#endif


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

#ifndef _SV_CFG_H
#define _SV_CFG_H

#define CFG_STRING		((int)1)
#define CFG_INT			((int)2)
#define CFG_LONG		((int)3)
#define CFG_DOUBLE		((int)4)
#define CFG_LINE		((int)5)
#define CFG_UINT32		((int)6)
#define CFG_UINT64		((int)7)

#ifdef __cplusplus
extern "C" {
#endif

int CheckPluginConfig(const char *pszConfigFilePath);
int IsVersionOk(const char *pbig_eq, const char *psmall);
int LoadConfig(const char *pszConfigFilePath, ...);
int GetConfigFile(const char *pAppStr, char **pconfOut);
int GetLogTypeByStr(const char *pstrType);

char * get_val(char* desc, char* src);
void get_config_val(char* desc, char* src); 

#ifdef __cplusplus
}
#endif

#endif // _SV_CFG_H


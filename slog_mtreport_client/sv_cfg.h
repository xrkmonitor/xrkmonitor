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

#ifndef _SV_CFG_H
#define _SV_CFG_H

#ifdef __cplusplus
extern "C" {
#endif


#define CFG_STRING		((int)1)
#define CFG_INT			((int)2)
#define CFG_LONG		((int)3)
#define CFG_DOUBLE		((int)4)
#define CFG_LINE		((int)5)
#define CFG_UINT32		((int)6)
#define CFG_UINT64		((int)7)

int LoadConfig(const char *pszConfigFilePath, ...);
int GetConfigFile(const char *pAppStr, char **pconfOut);

#ifdef __cplusplus
}
#endif

#endif // _SV_CFG_H

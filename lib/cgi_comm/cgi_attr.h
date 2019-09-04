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

   开发库 cgi_comm 说明:
        cgi/fcgi 的公共库，实现 cgi 的公共需求，比如登录态验证/cgi 初始化等

****/

#ifndef __CGI_ATTR_H__
#define __CGI_ATTR_H__ 

#include "cgi_head.h"
#include "des.h"
#include "user.pb.h"

int GetAttrTypeTreeFromVmem(CGIConfig &stConfig, MmapUserAttrTypeTree & stTypeTree);
const char *GetAttrTypeNameFromVmem(int iType, CGIConfig &stConfig);
void SetAttrTypeNameToVmem(int iType, const char* pname, CGIConfig &stConfig);

#endif


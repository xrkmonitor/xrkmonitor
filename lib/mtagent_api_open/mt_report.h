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

#ifndef _mt_report_H
#define _mt_report_H

#include <stdint.h>
#include <stddef.h>
#include "sv_shared_hash.h"

#define MAX_BASE_ATTR_NODE 10  
#define DEP_BASE_SHM_ID 24554 

#define MAX_ATTR_NODE 2100 
#define DEP_SHM_ID 24553 

#define DEF_STR_ATTR_NODE_SHM_KEY 2019060616 
#define MAX_STR_ATTR_NODE_COUNT 1179 
typedef struct
{
	volatile uint8_t bSyncProcess;
	char szStrInfo[65];
	int32_t iStrAttrId;
	int32_t iStrVal;
}StrAttrNode;

typedef struct
{
	volatile uint8_t bSyncProcess;
	int32_t iAttrID;
	int32_t iCurValue;
} AttrNode;

int MtReport_InitAttr();
int ShowAllAttr();
int MtReport_Attr_Add(int32_t attr, int32_t iValue);
int MtReport_Attr_Spec(int32_t attr, int32_t iValue);
int MtReport_Attr_Set(int32_t attr, int32_t iValue);

uint32_t GetShortKeyByStr(const char *pstr);
int MtReport_Str_Attr_Add(int32_t attr, const char *pstr, int32_t iValue);
int MtReport_Str_Attr_Set(int32_t attr, const char *pstr, int32_t iValue);

void ShowAttr(int32_t attr);
void ShowStrAttr(int32_t attr, const char *pstr);

#endif


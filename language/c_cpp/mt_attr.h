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

#ifndef __MT_ATTR_H__
#define __MT_ATTR_H__ (1) 

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "mt_shared_hash.h"

int MtReport_Attr_Spec(int32_t attr, int32_t iValue);

int MtReport_InitAttr();

#define MTATTR_HASH_NODE 2000 
#define MTATTR_SHM_DEF_COUNT 5 
#define MTATTR_SPECIAL_COUNT 200
#define APP_ATTR_READ_PER_TIME_SEC 60

uint32_t GetShortKeyByStr(const char *pstr);

#pragma pack(1)
typedef struct
{
	volatile uint8_t bSyncProcess;
	char szStrInfo[65];
	int32_t iStrAttrId;
	int32_t iStrVal;
}StrAttrNode;

typedef struct
{
	uint8_t bSyncProcess;
	int32_t iAttrID;
	int32_t iCurValue;
} AttrNode;
#pragma pack()


#ifdef __cplusplus
}
#endif

#endif


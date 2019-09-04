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


#ifndef _SV_SHM_H
#define _SV_SHM_H

#include <inttypes.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// 搜索共享内存 key 次数
#define MT_ATTACH_SHM_TRY_MAX 1000

// 原子操作
#define SYNC_FLAG_CAS_GET(f) __sync_bool_compare_and_swap(f, 0, 1)
#define SYNC_FLAG_CAS_FREE(f) f = 0

#define VARMEM_CAS_GET(f) __sync_bool_compare_and_swap(f, 0, 1)
#define VARMEM_CAS_FREE(f) f = 0

char* GetReadOnlyShm(int iKey, int iSize);
char* GetShm(int iKey, int iSize, int iFlag);
int GetShm2(void **pstShm, int iShmID, int iSize, int iFlag);

// 获取共享内存，并进行内存检查
// 返回值 -2 check 失败, -1 失败，1 不存在，已创建，0 存在已经 attach 上
int32_t MtReport_GetShm_comm(void **pstShm, int32_t iShmID, int32_t iSize, int32_t iFlag, const char *pcheckstr);

#endif


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

#ifndef _MT_SHM_20141117_H_
#define _MT_SHM_20141117_H_ (1) 

#include <inttypes.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define COMM_LIB_BUG assert(0)

// 搜索共享内存 key 次数
#define MT_ATTACH_SHM_TRY_MAX 100

// 原子操作
#define SYNC_CAS_GET(f) __sync_bool_compare_and_swap(f, 0, 1)
#define SYNC_CAS_FREE(f) f = 0
#define VARMEM_CAS_GET(f) __sync_bool_compare_and_swap(f, 0, 1)
#define VARMEM_CAS_FREE(f) f = 0


char* MtReportGetShm(int iKey, int iSize, int iFlag);

// 获取共享内存，并进行内存检查
// 返回值 -2 check 失败, -1 失败，1 不存在，已创建，0 存在已经 attach 上
int32_t MtReport_GetShm(void **pstShm, int32_t iShmID, int32_t iSize, int32_t iFlag, const char *pcheckstr);

#ifdef __cplusplus
}
#endif

#endif


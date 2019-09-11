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

#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "mt_shm.h"

char* MtReportGetShm(int iKey, int iSize, int iFlag)
{
	char* sShm = NULL;
	int iShmID = 0;
	if ((iShmID = shmget(iKey, iSize, iFlag)) < 0) 
		return NULL;

	if ((sShm = shmat(iShmID, NULL ,0)) == (char *)-1)
		return NULL;
	return sShm;
}

//  共享内存
// 返回值 -2 check 失败, -1 失败，1 不存在，已创建，0 存在已经 attach 上
int MtReport_GetShm(
	void **pstShm, int iShmID, int iSize, int iFlag, const char *pcheckstr)
{
	char* sShm = NULL;
	int iCheckSize = (int)strlen(pcheckstr);
	if ((sShm = MtReportGetShm(iShmID, iSize+iCheckSize*2, iFlag & (~IPC_CREAT))) == NULL) {
		if (!(iFlag & IPC_CREAT)) 
			return -1;

		if ((sShm = MtReportGetShm(iShmID, iSize+iCheckSize*2, iFlag)) == NULL)
			return -1;

		memcpy(sShm, pcheckstr, iCheckSize);
		memcpy(sShm+iCheckSize+iSize, pcheckstr, iCheckSize);
		memset(sShm+iCheckSize, 0, iSize);
		*pstShm = sShm+iCheckSize;
		return 1;
	}

	if(memcmp(sShm, pcheckstr, iCheckSize) || memcmp(sShm+iCheckSize+iSize, pcheckstr, iCheckSize))
		return -2;
	*pstShm = sShm+iCheckSize;
	return 0;
}


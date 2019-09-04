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

#include <sys/mman.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "sv_str.h"
#include "mt_report.h"
#include "sv_shm.h"

char* GetReadOnlyShm(int iKey, int iSize)
{
	int iShmID;
	char* sShm;
	if ((iShmID = shmget(iKey, iSize, 0)) < 0) {
		SetApiErrorMsg("shmget %d %d msg:%s", iKey, iSize, strerror(errno));
		return NULL;
	}
	if ((sShm = (char*)shmat(iShmID, NULL, SHM_RDONLY)) == (char *) -1) {
		SetApiErrorMsg("shmat failed msg:%s", strerror(errno));
		return NULL;
	}
	return sShm;
}

char* GetShm(int iKey, int iSize, int iFlag)
{
	int iShmID;
	char* sShm;

	if ((iShmID = shmget(iKey, iSize, iFlag)) < 0) {
		SetApiErrorMsg("shmget %d %d msg:%s", iKey, iSize, strerror(errno));
		return NULL;
	}
	if ((sShm = (char*)shmat(iShmID, NULL ,0)) == (char *) -1) {
		SetApiErrorMsg("shmat failed msg:%s", strerror(errno));
		return NULL;
	}
	return sShm;
}

//  共享内存
// 返回值 -2 check 失败, -1 失败，1 不存在，已创建，0 存在已经 attach 上
int MtReport_GetShm_comm(
	void **pstShm, int iShmID, int iSize, int iFlag, const char *pcheckstr)
{
	char* sShm = NULL;
	int iCheckSize = (int)strlen(pcheckstr);
	if ((sShm = GetShm(iShmID, iSize+iCheckSize*2, iFlag & (~IPC_CREAT))) == NULL) {
		if (!(iFlag & IPC_CREAT)) 
			return -1;

		if ((sShm = GetShm(iShmID, iSize+iCheckSize*2, iFlag)) == NULL)
		{
			MtReport_Attr_Add(97, 1);
			return -1;
		}

		memset(sShm, 0, iSize+iCheckSize*2);
		memcpy(sShm, pcheckstr, iCheckSize);
		memcpy(sShm+iCheckSize+iSize, pcheckstr, iCheckSize);
		*pstShm = sShm+iCheckSize;
		return 1;
	}

	if(memcmp(sShm, pcheckstr, iCheckSize) || memcmp(sShm+iCheckSize+iSize, pcheckstr, iCheckSize))
	{
		MtReport_Attr_Add(98, 1);
		return -2;
	}
	*pstShm = sShm+iCheckSize;
	return 0;
}

// 返回值 -1 失败，1 不存在，已创建，0 存在已经 attach 上
int GetShm2(void **pstShm, int iShmID, int iSize, int iFlag)
{
char* sShm;

       if (!(sShm = GetShm(iShmID, iSize, iFlag & (~IPC_CREAT)))) {
               if (!(iFlag & IPC_CREAT)) return -1;
               if (!(sShm = GetShm(iShmID, iSize, iFlag))) return -1;
               memset(sShm, 0, iSize);
               *pstShm = sShm;
               return 1;
       }
       *pstShm = sShm;
       return 0;
}

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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <errno.h>
#include <time.h>
#include <supper_log.h>
#include <assert.h>
#include "cgi_head.h"
#include "cgi_comm.h"
#include "sv_net.h"
#include "user.pb.h"
#include "mt_report.h"
#include "supper_log.h"

int GetAttrTypeTreeFromVmem(CGIConfig &stConfig, MmapUserAttrTypeTree & stTypeTree)
{
	int iVmemIdx = stConfig.stUser.pSysInfo->iAttrTypeTreeVmemIdx;
	if(iVmemIdx > 0)
	{
		char szShmValBuf[1024] = {0};
		int iShmValBufLen = sizeof(szShmValBuf);
		const char *pbuf = MtReport_GetFromVmemZeroCp(iVmemIdx, szShmValBuf, &iShmValBufLen);
		if(pbuf != NULL && iShmValBufLen > 0) {
			if(!stTypeTree.ParseFromArray(pbuf, iShmValBufLen)) {
				WARN_LOG("stTypeTree ParseFromArray failed, len:%d", iShmValBufLen);
			}
			else {
				DEBUG_LOG("get type tree info:%s", stTypeTree.ShortDebugString().c_str());
				return 0;
			}
		}
	}
	return SLOG_ERROR_LINE;
}

const char *GetAttrTypeNameFromVmem(int iType, CGIConfig &stConfig)
{
	AttrTypeInfo *pInfo = slog.GetAttrTypeInfo(iType, NULL);
	if(pInfo != NULL)
		return MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
	return NULL;
}

void SetAttrTypeNameToVmem(int iType, const char* pname, CGIConfig &stConfig)
{
	AttrTypeInfo *pInfo = NULL;
	pInfo = slog.GetAttrTypeInfo(iType, NULL);
	if(pInfo != NULL) {
		pInfo->iNameVmemIdx = MtReport_SaveToVmem(pname, strlen(pname)+1);;
	}
}


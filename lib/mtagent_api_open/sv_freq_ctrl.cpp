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

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#include "sv_freq_ctrl.h"

#define TV2US(ptv) ((ptv)->tv_sec * 1000000 + (ptv)->tv_usec)

int TokenBucket_Init(TokenBucket *pstTB, uint32_t dwFreqPerSec, uint32_t dwBucketSize)
{
	struct timeval tv;

	memset(pstTB, 0, sizeof(*pstTB));
	pstTB->dwFreqPerSec = dwFreqPerSec;
	pstTB->dwBucketSize = dwBucketSize;
	pstTB->llTokenCount = dwBucketSize;
	gettimeofday(&tv, NULL);
	pstTB->qwLastGenTime = TV2US(&tv);

	return 0;
}

int TokenBucket_Gen(TokenBucket *pstTB, const struct timeval *ptvNow)
{
	struct timeval tv;
	uint64_t qwNow, qwPastUs;
	uint64_t qwNewTokens, qwCalcDelta;
	int64_t llNewTokenCount;

	if (ptvNow == NULL) {
		gettimeofday(&tv, NULL);
		ptvNow = &tv;
	}
	qwNow = TV2US(ptvNow);
	if (qwNow < pstTB->qwLastGenTime) {
		pstTB->qwLastGenTime = qwNow;
		return -1;
	}

	qwPastUs = qwNow - pstTB->qwLastGenTime;
	qwNewTokens = (((uint64_t)pstTB->dwFreqPerSec * qwPastUs + pstTB->qwLastCalcDelta) / 1000000);
	qwCalcDelta = (((uint64_t)pstTB->dwFreqPerSec * qwPastUs + pstTB->qwLastCalcDelta) % 1000000);

	pstTB->qwLastGenTime = qwNow;
	pstTB->qwLastCalcDelta = qwCalcDelta;
	llNewTokenCount = pstTB->llTokenCount + qwNewTokens;
	if (llNewTokenCount < pstTB->llTokenCount) {
		pstTB->llTokenCount = pstTB->dwBucketSize;
		return -1;
	}
	if (llNewTokenCount > pstTB->dwBucketSize) {
		llNewTokenCount = pstTB->dwBucketSize;
	}
	pstTB->llTokenCount = llNewTokenCount;

	return 0;
}

int TokenBucket_Get(TokenBucket *pstTB, uint32_t dwNeedTokens)
{
	if (pstTB->llTokenCount < (int64_t)dwNeedTokens) {
		return -1;
	}
	pstTB->llTokenCount -= dwNeedTokens;
	return 0;
}

void TokenBucket_Mod(TokenBucket * pstTB, uint32_t dwFreqPerSec, uint32_t dwBucketSize)
{
	pstTB->dwFreqPerSec = dwFreqPerSec;
	pstTB->dwBucketSize = dwBucketSize;
}


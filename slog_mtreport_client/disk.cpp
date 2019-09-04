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

#define __STDC_FORMAT_MACROS
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/shm.h>
#include <math.h>
#include "mem.h"

int GetDiskInfo(uint64_t & qwTotalSpace, uint64_t & qwTotalUse, uint32_t &maxUsePer)
{
	FILE *fp = popen("df -P -l -x iso9660 -x iso9600 | awk \'{if(NR!=1 && NF==6) print $3\" \"$4 }\'", "r");
	if(fp == NULL) {
		return -1;
	}

	qwTotalSpace = 0;
	qwTotalUse = 0;
	maxUsePer = 0;
	uint64_t qwRemain=0, qwUse=0;
	uint32_t dwUsePer = 0;
	while( fscanf(fp, "%" PRIu64 " %" PRIu64 , &qwUse, &qwRemain) == 2) {
		qwTotalSpace += qwRemain+qwUse;
		qwTotalUse += qwUse;
		dwUsePer = (uint32_t)ceil(qwUse*100.0/(qwRemain+qwUse));
		if(dwUsePer > maxUsePer)
			maxUsePer = dwUsePer;
	}
	pclose(fp);
	return 0;
}


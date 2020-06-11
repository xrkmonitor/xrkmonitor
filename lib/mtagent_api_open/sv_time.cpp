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

#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "sv_time.h"

time_t  g_cmm_cur_time = 0;

void sv_SetCurTime(time_t now)
{
	g_cmm_cur_time = now;
}

time_t sv_GetCurTime()
{
	if(g_cmm_cur_time != 0)
		return g_cmm_cur_time;
	return time(NULL);
}

char * time_to_ISO8601(time_t t)
{
	static char s_ios[32];
	strftime(s_ios, sizeof(s_ios), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
	return (char*)s_ios;
}

int IsTimeInDayHour(time_t t, int h)
{
	struct tm *ptm = localtime(&t);
	if(ptm != NULL && ptm->tm_hour == h)
		return 1;

	return 0;
}

int32_t GetTimeDiffMs(char cIsStart)
{
	static struct timeval t_tmStart;
	struct timeval t_tmEnd;
	if(cIsStart) {
		gettimeofday(&t_tmStart, NULL);
		return 0;
	}
	gettimeofday(&t_tmEnd, NULL);
	return (int32_t)((t_tmEnd.tv_sec-t_tmStart.tv_sec)*1000000+t_tmEnd.tv_usec-t_tmStart.tv_usec)/1000;
}


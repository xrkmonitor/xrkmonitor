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
#include <execinfo.h>
#include <signal.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/types.h>
#include <dirent.h>
#include "sv_coredump.h"
#include "sv_str.h"

static SigCallBackFun s_cb;
static char *s_pszDumpFile;
static void *s_pdata;

void deal_daemon_signal(int sig)
{
	void *array[BACKTRACE_DUMP_LEVELS];
	size_t size = 0;
	size_t i = 0;
	char **strings = NULL;
	time_t tmCur = time(NULL);

	size = backtrace(array, BACKTRACE_DUMP_LEVELS);
	strings = backtrace_symbols(array, size);

	FILE *fp = NULL;
	if(s_pszDumpFile != NULL)
		fp = fopen(s_pszDumpFile, "a+");
	else
		fp = fopen(BACKTRACE_DEF_DUMP_FILE, "a+");
	if(fp != NULL)
	{
		fprintf(fp, "at :%s ------------------------------------ \n", uitodate(tmCur));
		fprintf(fp, "recevie signal: %d\n", sig);
		if(size <= 0 || NULL == strings)
			fprintf(fp, "backtrace or backtrace_symbols failed !\n");
		else
		{
			for (i = 0; i < size; i++)
				fprintf(fp, "%s \n", strings[i]);
		}
		fprintf(fp, "\n\n");
		fclose(fp);
		if(s_cb != NULL)
			s_cb(sig, s_pszDumpFile, s_pdata);
	}
	else if(s_cb != NULL)
		s_cb(sig, s_pszDumpFile, s_pdata);

	exit(-1);
}

void RegisterSignal(const char *pszDumpFile, void *pdata, SigCallBackFun cb)
{
	FILE *fp = NULL;
	char sDirBuf[256] = {0};
	char sMkdir[300] = {0};
	char *pdir = NULL;

	s_pszDumpFile = NULL;
	s_cb = cb;
	if(pszDumpFile != NULL)
		s_pszDumpFile = strdup(pszDumpFile);
	else
		s_pszDumpFile = (char*)BACKTRACE_DEF_DUMP_FILE;

	if(s_pszDumpFile != NULL)
	{
		fp = fopen(s_pszDumpFile, "a+");
		if(NULL == fp) {
			pdir = strrchr(s_pszDumpFile, '/');
			if(pdir != NULL) {
				assert((int)(pdir-s_pszDumpFile) < (int)sizeof(sDirBuf));
				strncpy(sDirBuf, s_pszDumpFile, pdir-s_pszDumpFile);
				snprintf(sMkdir, sizeof(sMkdir), "mkdir -p %s", sDirBuf);
				system(sMkdir);
				usleep(100000);
				fp = fopen(s_pszDumpFile, "a+");
			}
			if(NULL == fp)
			{
				fprintf(stderr, "open file : %s failed !\n", s_pszDumpFile);
				exit(-1);
			}
		}
		fclose(fp);
	}
	
	s_pdata = pdata;

	signal(SIGINT,  deal_daemon_signal);
	signal(SIGHUP,  deal_daemon_signal);
	signal(SIGQUIT, deal_daemon_signal);
	signal(SIGTERM, deal_daemon_signal);
	signal(SIGSEGV, deal_daemon_signal);
}


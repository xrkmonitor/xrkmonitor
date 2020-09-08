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
#include "mt_log.h"

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
			fprintf(fp, "use: c++filt and addr2line addr -e exe_file -f, to get detail info \n");
			free(strings);
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
		s_pszDumpFile = BACKTRACE_DEF_DUMP_FILE;

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


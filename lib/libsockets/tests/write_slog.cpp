#include <stdio.h>
#include <supper_log.h>

CSupperLog slog;
CSupperLog wslog;

void test(CSupperLog *pslog)
{
	CSLogServerWriteFile wlog(pslog);
	while(1)
	{
	//	if(wlog.WriteFile(2) < 0)
	//		break;
	//	getchar();
		wlog.ShowFileShmInfo(true);
		break;
	}
}

int main(int argc, char *argv[])
{
	int iRet = 0;

	slog.SetLogType(BWORLD_SLOG_TYPE_LOCAL);
	slog.SetLogLevel("SLOG_LEVEL_TRACE");
	slog.SetLogToStd(true);
	if((iRet=slog.InitSupperLog()) < 0)
	{
	    fprintf(stderr, "InitSupperLog failed, ret:%d\n", iRet);
	    return -1;
	}
	test(&slog);
	return 0;
}


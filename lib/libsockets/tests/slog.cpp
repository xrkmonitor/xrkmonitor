#include <stdio.h>
#include <supper_log.h>

CSupperLog slog;
CSupperLog wslog;

int main(int argc, char *argv[])
{
	int iRet = 0;

	slog.SetLogType(BWORLD_SLOG_TYPE_NET);
	slog.SetLogLevel("SLOG_LEVEL_TRACE");
	if((iRet=slog.InitSupperLog()) < 0)
	{
	    fprintf(stderr, "InitSupperLog failed, ret:%d\n", iRet);
	    return -1;
	}
	ERR_LOG("error log skdfjskdfjkdsk log appid:skdddddddddddf!iret:%d\n", iRet);

	/*
	wslog.SetLogLevel("SLOG_LEVEL_TRACE");
	wslog.SetLogSpeed(100000);
	wslog.SetLogType(BWORLD_SLOG_TYPE_NET);
	wslog.SetLogAppId(123);
	wslog.SetLogModuleId(888);
	wslog.SetLogToStd(0);

	slog.SetLogLevel("SLOG_LEVEL_TRACE");
	slog.SetLogSpeed(100000);
	slog.SetLogAppId(123);
	slog.SetLogModuleId(888);

	slog.SetLogType(BWORLD_SLOG_TYPE_NET);
	slog.SetLogToStd(1);

	//slog.SetLogType(BWORLD_SLOG_TYPE_LOCAL);

	if((iRet=slog.InitSupperLog()) < 0)
	{
	    fprintf(stderr, "InitSupperLog failed, ret:%d\n", iRet);
	    return -1;
	}

	WATER_LOG("water log skdfjskdfjkdsk log appid:skdddddddddddf!iret:%d\n", iRet);
	ERR_LOG("error log skdfjskdfjkdsk log appid:skdddddddddddf!iret:%d\n", iRet);
	REQERR_LOG("req error log skdfjskdfjkdsk log appid:skdddddddddddf!iret:%d\n", iRet);
	WARN_LOG("warning log skdfjskdfjkdsk log appid:skdddddddddddf!iret:%d\n", iRet);
	FATAL_LOG("fatal log skdfjskdfjkdsk log appid:skdddddddddddf!iret:%d\n", iRet);
	INFO_LOG("info log skdfjskdfjkdsk log appid:skdddddddddddf!iret:%d\n", iRet);
	DEBUG_LOG("test log skdfjskdfjkdsk log appid:skdddddddddddf!iret:%d\n", iRet);
	DEBUG_LOG("skdfjskdfjkdsk log appid:skdddddddddddf!iret:%d\n", iRet);
	DEBUG_LOG("skdfsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss_rock_deng_jskdfjkdsk log appid:skdddddddddddf!iret:%d, 12938385u38_8486,_k232k3@#%#@666\n", iRet);
	WATER_LOG("water skdfsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss_rock_deng_jskdfjkdsk log appid:skdddddddddddf!iret:%d, 12938385u38_8486,_k232k3@#%#@666\n", iRet);
	DEBUG_LOG("test skdfsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss_rock_deng_jskdfjkdsk log appid:skdddddddddddf!iret:%d, 12938385u38_8486,_k232k3@#%#@666\n", iRet);
	INFO_LOG("info skdfsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss_rock_deng_jskdfjkdsk log appid:skdddddddddddf!iret:%d, 12938385u38_8486,_k232k3@#%#@666\n", iRet);
//	*/
	slog.ShowShmLog(1000);
	return 0;
}


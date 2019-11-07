/*
   *   监控系统 api 使用示例
   */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "mt_report.h" // 监控系统 api 头文件
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
	if(argc <= 1) {
		printf("use test config_id\n");
		return -1;
	}

	// 使用属性监控和日志功能初始化，日志上报需要一个配置 id
	uint32_t dwCfgId = strtoul(argv[1], NULL, 10);

	// 本地日志文件记录的日志类型
	int iLogType = 255;

	// 调用初始化接口
	if(MtReport_Init(dwCfgId, "./local.log", iLogType, 0) < 0) {
		printf("MtReport_Init failed !\n");
		return -2;
	}

	int iCount = rand()%20;
	for(int i=0; i < iCount; i++) 
	{
		int iVal = rand()%1000;
		MtReport_Attr_Add(230, iVal);

		// 日志接口示例
		printf("debug - %d\n", MtReport_Log_Debug("debug log !"));
		printf("info - %d\n", MtReport_Log_Info("info log !"));
		printf("warn - %d\n", MtReport_Log_Warn("warning log !"));
		printf("reqerr - %d\n", MtReport_Log_Reqerr("request error log !"));
		printf("err - %d\n", MtReport_Log_Error("error log !"));
		printf("fatal - %d\n", MtReport_Log_Fatal("fatal log !"));

		// 使用日志自定义字段
		MtReport_Log_SetCust1(1024);
		MtReport_Log_SetCust5("cust5 set");

		// 以下几条日志 cust1, cust5 的值保持不变
		MtReport_Log_Debug("(cust) debug log - rand val:%d - %d - %d!", iCount, iVal, i);
		MtReport_Log_Info("(cust) info log - rand val:%d - %d - %d!", iCount, iVal, i);
		MtReport_Log_Warn("(cust) warning log - rand val:%d - %d - %d!", iCount, iVal, i);
		MtReport_Log_Reqerr("(cust) request error log - rand val:%d - %d - %d!", iCount, iVal, i);
		MtReport_Log_Error("(cust) error log - rand val:%d - %d - %d!", iCount, iVal, i);
		MtReport_Log_Fatal("(cust) fatal log - rand val:%d - %d - %d!", iCount, iVal, i);

		// 改变自定义字段值
		MtReport_Log_SetCust1(2048);
		MtReport_Log_SetCust5("cust5 change test");

		// 以下几条日志 cust1, cust5 的值为设置后的新值
		MtReport_Log_Debug("(cust change test) debug log - %u - rand val:%d - %d - %d!", time(NULL), iCount, iVal, i);
		MtReport_Log_Info("(cust change test) info log - rand val:%d - %d - %d!", iCount, iVal, i);
		MtReport_Log_Warn("(cust change test) warning log - rand val:%d - %d - %d!", iCount, iVal, i);
		MtReport_Log_Reqerr("(cust change test) request error log - rand val:%d - %d - %d!", iCount, iVal, i);
		MtReport_Log_Error("(cust change test) error log - rand val:%d - %d - %d!", iCount, iVal, i);
		MtReport_Log_Fatal("(cust change test) fatal log - rand val:%d - %d - %d !", iCount, iVal, i);
	}

	return 0;
}


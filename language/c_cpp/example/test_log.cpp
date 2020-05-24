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
	if(argc < 3) {
		// example: ./test_log xxx "hello, world"
		printf("use test_log config_id log\n");
		return -1;
	}

	// 日志上报需要一个日志配置 id, 可在控制台配置
	uint32_t dwCfgId = strtoul(argv[1], NULL, 10);

	// 本地日志文件记录的日志类型 -- 日志上报的api 能够同时写一份在本地，如不需要 iLogType 设置为0即可
	int iLogType = 255;

	// 调用初始化接口, 参数见 api 说明
	if(MtReport_Init(dwCfgId, "./local.log", iLogType, 0) < 0) {
		printf("MtReport_Init failed !\n");
		return -2;
	}

	// 使用日志自定义字段
	MtReport_Log_SetCust1(1024);
	MtReport_Log_SetCust5("cust5 set");
	MtReport_Log_SetCust6("cust6ddddddddd set");

	// cust 参数再没有再次设置的情况下在每条日志中输出的值不变
	// 以下几条日志 cust1, cust5 的值保持不变
	MtReport_Log_Debug("%s", argv[2]);
	MtReport_Log_Info("%s", argv[2]);
	MtReport_Log_Warn("%s", argv[2]);
	MtReport_Log_SetCust2(2024);
	MtReport_Log_SetCust3(3024);
	MtReport_Log_SetCust5("cust5 setssssss");
	MtReport_Log_Reqerr("%s", argv[2]);
	MtReport_Log_Error("%s", argv[2]);
	MtReport_Log_Fatal("%s", argv[2]);

	// 改变 cust 字段值
	// cust 参数值改变后，此后每条日志都是新值，直到再次变化
	MtReport_Log_SetCust1(2048);
	MtReport_Log_SetCust5("cust5 change test");

	// 以下几条日志 cust1, cust5 的值为设置后的新值
	MtReport_Log_Debug("%s", argv[2]);
	MtReport_Log_Info("%s", argv[2]);
	MtReport_Log_Warn("%s", argv[2]);
	MtReport_Log_Reqerr("%s", argv[2]);
	MtReport_Log_Error("%s", argv[2]);
	MtReport_Log_Fatal("%s", argv[2]);
	return 0;
}


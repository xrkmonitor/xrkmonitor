/*
   *   字符云监控系统开源版 使用示例
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
#include <iostream>

// 监控系统 api 头文件
#include "mt_report.h" 

using namespace std;

	/*
	   * 配置文件 pfile 可以包含0个或者多个配置项，使用工具 slog_tool(./slog_tool show varconfig)
	   * 可以列出所有的配置项，大部分配置项使用默认值即可，无需配置，基本配置项说明：
	   * SLOG_CONFIG_ID -- 模块日志配置 id，如您需要使用监控系统的日志功能则需要配置
	   * LOCAL_IF_NAME -- 本机IP 网口名，用于获取到本机在监控系统配置中的机器配置，以便将监控系统上报的
	   * 	日志/监控点数据归属到机器下
	   *
	   */

// 定义一个全局变量
CSupperLog slog;

int main(int argc, char *argv[])
{
	// 监控系统 api 库初始化，main.conf 为库初始化需要的配置文件
	if(slog.InitForUserApp("./main.conf") < 0)
	{
		printf("init failed\n");
		return -1;
	}

	return 0;
}


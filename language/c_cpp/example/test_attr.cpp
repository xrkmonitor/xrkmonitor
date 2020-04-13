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

// 监控系统 api 头文件
#include "mt_report.h" 
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
	if(argc < 3) {
		// example: ./test_attr  188 108
		// example: ./test_attr  188 strinfo 108
		printf("use test_attr attr_id [str] attr_val\n");
		return -1;
	}

	// 仅上报监控点数据，调用初始化接口不需要传日志配置id, 参数见 api 说明
	if(MtReport_Init(0, NULL, 0, 0) < 0) {
		printf("MtReport_Init failed !\n");
		return -2;
	}
	printf("init ok\n");
	if(argc >= 4)
		printf("%d\n", MtReport_Str_Attr_Add(atoi(argv[1]), argv[2], atoi(argv[3])));
	else
		printf("%d\n", MtReport_Attr_Add(atoi(argv[1]), atoi(argv[2])));
	return 0;
}


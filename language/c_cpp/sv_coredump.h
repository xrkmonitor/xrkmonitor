#ifndef __SV_COREDUMP_H_11130505__
#define __SV_COREDUMP_H_11130505__ 1

#ifdef __cplusplus
extern "C" {
#endif

#define BACKTRACE_DUMP_LEVELS 10
#define BACKTRACE_DEF_DUMP_FILE "/tmp/backtrace_dump.log"

typedef void (*SigCallBackFun)(int sig, const char *pszFile, void *pdata);

// 注册信号处理函数 (编译时请加上 -rdynamic 参数)
// pszDumpFile --- 堆栈信息转存的文件, 如果为 NULL 则输出到标准输出
// cb --- cb 信号处理回调函数，如果为 NULL, 收到信号后程序退出，否则调用 cb
void RegisterSignal(const char *pszDumpFile, void *pdata, SigCallBackFun cb);

#ifdef __cplusplus
}
#endif

#endif


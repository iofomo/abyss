/* This file is auto-generated, edit at your own risk.  */
#ifndef BUILD_H
#define BUILD_H
#undef VERSION
#define VERSION "v5.4.0-5f780cba"
//新的linux api,__ANDROID_API__ >= 23时可以
#define HAVE_PROCESS_VM
#define HAVE_SECCOMP_FILTER

//是否使用模版的loader程序来载入exe
//#define USE_LOADER_EXE

//是否处理系统调用
#define HANDLE_SYSCALL

//仅仅为了方便调试的代码(为了排查问题,可能会拖慢效率)
#define DEBUG_ONLY

//是否启用日志
#define ENABLE_LOG
//使用printf代替android_log_print
//#define LOG_PRINTF

#endif /* BUILD_H */

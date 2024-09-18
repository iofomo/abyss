//
// Created by mac on 2023/12/11.
//

#ifndef INTERCEPTSYSCALL_CONSTANT_H
#define INTERCEPTSYSCALL_CONSTANT_H
#include "build.h"
#include "library.h"

#include <android/log.h>
#include <stdio.h>

#define TAG "INTERCEPT"
#define TAG_SYS "INTERCEPT/SYS"
#define TAG_SYSW "INTERCEPT/SYSW"
#define TAG_SYSE "INTERCEPT/SYSE"

#if defined(ENABLE_LOG)

#if defined(LOG_PRINTF)

//set_seccomp_filters信息
#define LOGSECOMP(...) { printf(__VA_ARGS__);printf("\n");}
//#define LOGSECOMP(...) {}

#define LOGD(...) { printf(__VA_ARGS__);printf("\n");}
#define LOGI(...) { printf(__VA_ARGS__);printf("\n");}
#define LOGW(...) { printf(__VA_ARGS__);printf("\n");}
#define LOGE(...) { printf(__VA_ARGS__);printf("\n");}


#define LOGSYS(...) { printf(__VA_ARGS__);printf("\n");}
#define LOGSYSW(...) { printf(__VA_ARGS__);printf("\n");}
#define LOGSYSE(...) { printf(__VA_ARGS__);printf("\n");}
#else

//set_seccomp_filters信息
#define LOGSECOMP(...) { __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__);}
//#define LOGSECOMP(...) {}

#define LOGD(...) { __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__);}
#define LOGI(...) { __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__);}
#define LOGW(...) { __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__);}
#define LOGE(...) { __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__);}


#define LOGSYS(...) { __android_log_print(ANDROID_LOG_ERROR,TAG_SYS,__VA_ARGS__);}
#define LOGSYSW(...) { __android_log_print(ANDROID_LOG_ERROR,TAG_SYSW,__VA_ARGS__);}
#define LOGSYSE(...) { __android_log_print(ANDROID_LOG_ERROR,TAG_SYSE,__VA_ARGS__);}
#endif
#else

//set_seccomp_filters信息
#define LOGSECOMP(...) {}

#define LOGD(...) {}
#define LOGI(...) {}
#define LOGW(...) {}
#define LOGE(...) {}

#define LOGSYS(...) {}
#define LOGSYSW(...) {}
#define LOGSYSE(...) {}

#endif


#define __LIKELY(x)       __builtin_expect(!!(x), true)
#define __UNLIKELY(x)     __builtin_expect(!!(x), false)


typedef enum readlink_type{
    BUSINESS, //业务逻辑数据
    NORMAL, //常规的tracee调用
}readlink_type;

typedef struct readlink_context{
    readlink_type type;
    void* data;
}readlink_context;


typedef void (*on_sys_event_t)(syscall_data* data);

extern on_sys_event_t global_on_sysenter;
extern on_sys_event_t global_on_sysexit;

extern pthread_t work_tid; //工作线程pid
extern pid_t snew_attach_pid;
extern struct PCond_Context spctx;

#endif //INTERCEPTSYSCALL_CONSTANT_H

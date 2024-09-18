//
// Created by mac on 2023/12/15.
//

#include <string.h>
#include <android/log.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <dlfcn.h>
#include "cmn_back_call_stack.h"
#include "constant.h"
#include "mem.h"

#define LOG(...) { __android_log_print(ANDROID_LOG_ERROR,"Native_Stack",__VA_ARGS__);}

typedef struct TraceBuff {
    int depth;
    char* buffer[100];
} TraceBuffInfo;


void print_register_info(const ucontext_t *ucontext) {
#if defined(__aarch64__)
    for (int i = 0; i < 30; ++i) {
        if (i % 4 == 0) {
            LOG("%s", "     ");
        }
        char info[64] = {0};
        sprintf(info, "x%d %016lx ", i, ucontext->uc_mcontext.regs[i]);
        LOG(  "%s", info);
        if ((i + 1) % 4 == 0) {
            LOG( "%s", "\r\n");
        }
    }
    LOG(  "%s", "\r\n");

    char spInfo[64] = {0};
    sprintf(spInfo, "sp %016lx ", ucontext->uc_mcontext.sp);

    char lrInfo[64] = {0};
    sprintf(lrInfo, "lr %016lx ", ucontext->uc_mcontext.regs[30]);

    char pcInfo[64] = {0};
    sprintf(pcInfo, "pc %016lx ", ucontext->uc_mcontext.pc);

    LOG(  "%s%s%s%s\r\n", "     ",spInfo,lrInfo,pcInfo);
#elif defined(__arm__)
    char x0Info[64] = {0};
    sprintf(x0Info, "x0 %08lx ", ucontext->uc_mcontext.arm_r0);
    char x1Info[64] = {0};
    sprintf(x1Info, "x1 %08lx ", ucontext->uc_mcontext.arm_r1);
    char x2Info[64] = {0};
    sprintf(x2Info, "x2 %08lx ", ucontext->uc_mcontext.arm_r2);
    char x3Info[64] = {0};
    sprintf(x3Info, "x3 %08lx ", ucontext->uc_mcontext.arm_r3);
    char x4Info[64] = {0};
    sprintf(x4Info, "x4 %08lx ", ucontext->uc_mcontext.arm_r4);
    char x5Info[64] = {0};
    sprintf(x5Info, "x5 %08lx ", ucontext->uc_mcontext.arm_r5);
    char x6Info[64] = {0};
    sprintf(x6Info, "x6 %08lx ", ucontext->uc_mcontext.arm_r6);
    char x7Info[64] = {0};
    sprintf(x7Info, "x7 %08lx ", ucontext->uc_mcontext.arm_r7);
    char x8Info[64] = {0};
    sprintf(x8Info, "x8 %08lx ", ucontext->uc_mcontext.arm_r8);
    char x9Info[64] = {0};
    sprintf(x9Info, "x9 %08lx ", ucontext->uc_mcontext.arm_r9);
    char x10Info[64] = {0};
    sprintf(x10Info, "x10 %08lx ", ucontext->uc_mcontext.arm_r10);

    char ipInfo[64] = {0};
    sprintf(ipInfo, "ip %08lx ", ucontext->uc_mcontext.arm_ip);
    char spInfo[64] = {0};
    sprintf(spInfo, "sp %08lx ", ucontext->uc_mcontext.arm_sp);
    char lrInfo[64] = {0};
    sprintf(lrInfo, "lr %08lx ", ucontext->uc_mcontext.arm_lr);
    char pcInfo[64] = {0};
    sprintf(pcInfo, "pc %08lx ", ucontext->uc_mcontext.arm_pc);

    LOG("%s%s%s%s%s%s", "     ",x0Info, x1Info, x2Info, x3Info,
                    "\r\n");
            LOG("%s%s%s%s%s%s", "     ", x4Info, x5Info, x6Info, x7Info,
                    "\r\n");
            LOG("%s%s%s%s%s", "     ", x8Info, x9Info, x10Info, "\r\n");
            LOG("%s%s%s%s%s\r\n", "     ", ipInfo, spInfo, lrInfo, pcInfo);
#endif
    return ;
}


_Unwind_Reason_Code my_trace_back_stack(struct _Unwind_Context *context, void *hnd) {
    TraceBuffInfo *traceHandle = (TraceBuffInfo *) hnd;

    _Unwind_Word ip = _Unwind_GetIP(context);
    Dl_info info;
    int res = dladdr((void *) ip, &info);

    if (res == 0) {
        return _URC_END_OF_STACK;
    }

    char *desc = (char *) malloc(1024);
    memset(desc, 0, 1024);
    if (info.dli_fname != NULL) {
        char *symbol = (char *) malloc(256);

        if (info.dli_sname == NULL) {
            strcpy(symbol, "unknown");
        } else {
            sprintf(symbol, "%s+%ld", info.dli_sname,
                    ip - (_Unwind_Word) info.dli_saddr);
        }

#if defined(__arm__)
        sprintf(desc, "     #%02d pc %08lx  %s (%s) \r\n", traceHandle->depth,
                ip - (_Unwind_Word) info.dli_fbase,
                info.dli_fname, symbol);
#elif defined(__aarch64__)
        sprintf(desc, "     #%02d pc %016lx  %s (%s) \r\n", traceHandle->depth,
                ip - (_Unwind_Word) info.dli_fbase,
                info.dli_fname, symbol);
#endif
        free(symbol);
    }

    traceHandle->buffer[traceHandle->depth] = desc;
    ++traceHandle->depth;
    // FIXME: crash if call stack is over 5 on ARM32, unknown reason
#if !defined(__aarch64__) && defined(__arm__)
    if (traceHandle->depth == 5) {
        return _URC_END_OF_STACK;
    }
#endif
    if (traceHandle->depth == 99) {
        return _URC_END_OF_STACK;
    }

    return _URC_NO_REASON;
}

void sig_handler_call_stack(int sig, siginfo_t *info, void *context){
    TraceBuffInfo traceInfo;
    memset(&traceInfo, 0 , sizeof(traceInfo));
    _Unwind_Backtrace(my_trace_back_stack, &traceInfo);


    time_t timep;
    time(&timep);
    char tmp[64];
    char tmp_path[256];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S%z", localtime(&timep));

    strftime(tmp_path, sizeof(tmp_path), "%Y-%m-%d-%H-%M-%S.txt", localtime(&timep));
//    snprintf(path, sizeof(path), "%s/%s_%s", tombstone_file_path, __PRINTSTR(g_pkg_name), tmp_path);

//    if (elfFile) {
        char processName[256] = {0};
        char cmd[64] = {0};
        sprintf(cmd, "/proc/%d/cmdline", getpid());
        FILE *f = fopen(cmd, "r");
        if (f) {
            size_t size;
            size = fread(processName, sizeof(char), 256, f);
            if (size > 0 && '\n' == processName[size - 1]) {
                processName[size - 1] = '\0';
            }
            fclose(f);
        }

        LOG( "*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\r\n");

        LOG( "Timestamp: %s\r\n", tmp);
        LOG("pid: %d", getpid());
        LOG( ", uid: %d", getuid());
        LOG( ", tid: %d", gettid());
        LOG( ", process: >>> %s <<<\r\n", processName);
        LOG( "signal %d, code %d , fault addr %p\r\n",sig,  info->si_code, info->si_addr);

    print_register_info((const ucontext_t *) context);

        int i = 0;
        while(traceInfo.depth > i){
            LOG("%s", traceInfo.buffer[i]);
            free(traceInfo.buffer[i++]);
        }


        LOG("sig handler print success")
//    }
//    else {
//        __LOGI__("sigHandler_test write fail: %s", strerror(errno))
//    }
//    unregister_sig_handler();
    raise(sig);

    LOG("sig handler end")

}

#if defined(__aarch64__)
void print_remote_call_stack_arm64(const Tracee *tracee){
    struct user_regs_struct regs = tracee->_regs[CURRENT];
    uint64_t* fp = regs.regs[29]; //x29
    LOGD("exception pc:0x%lx,%p,x29:%lx",regs.pc,fp,regs.regs[29])

     while (fp != NULL)
    {
        word_t remote_data =peek_word(tracee,fp + 1);
        //  printf("stack:,fp:%p,lr_addr:%p lr:%lx\n",fp,(fp + 1),*((uint64_t *)(fp + 1)));
        LOGD("bt:0x%lx\n",remote_data);
        fp = peek_word(tracee,fp);
    }
    LOGD("exception print end--------");
}
#endif
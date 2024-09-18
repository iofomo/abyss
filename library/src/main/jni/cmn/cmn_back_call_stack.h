//
// Created by mac on 2023/12/15.
//

#ifndef CMN_BACK_CALL_STACK_H
#define CMN_BACK_CALL_STACK_H
#include <sys/ucontext.h>
#include <unwind.h>
#include <sys/user.h>
#include "tracee/tracee.h"


/**
 * 打印native异常栈
 */
void sig_handler_call_stack(int sig, siginfo_t *info, void *context);

#if defined(__aarch64__)
/**
 *  tracer打印tracee的堆栈
 */
void print_remote_call_stack_arm64(const Tracee *tracee);
#endif

#endif //CMN_BACK_CALL_STACK_H

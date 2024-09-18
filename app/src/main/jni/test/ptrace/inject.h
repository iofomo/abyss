/* Copyright (c) 2015, Simone 'evilsocket' Margaritelli
   Copyright (c) 2015-2019, Jorrit 'Chainfire' Jongma
   See LICENSE file for details */

#ifndef INJECT_H
#define INJECT_H

#include <unistd.h>
#include <android/log.h>
#include <stdio.h>


#if defined(__arm__)
#define CPSR_T_MASK ( 1u << 5 )
#define PARAMS_IN_REGS 4
#elif defined(__aarch64__)
#define CPSR_T_MASK ( 1u << 5 )
#define PARAMS_IN_REGS 8
#define pt_regs user_pt_regs
#define uregs regs
#define ARM_pc pc
#define ARM_sp sp
#define ARM_cpsr pstate
#define ARM_lr regs[30]
#define ARM_r0 regs[0]
#endif

#if defined(__LP64__)
#define PATH_LINKER_BIONIC "/bionic/bin/linker64"
#define PATH_LIBDL_BIONIC "/bionic/lib64/libdl.so"
#define PATH_LIBC_BIONIC "/bionic/lib64/libc.so"
#define PATH_LINKER "/system/bin/linker64"
#define PATH_LIBDL "/system/lib64/libdl.so"
#define PATH_LIBC "/system/lib64/libc.so"
#define PATH_LIBANDROID_RUNTIME "/system/lib64/libandroid_runtime.so"
#else
#define PATH_LINKER_BIONIC "/bionic/bin/linker"
#define PATH_LIBDL_BIONIC "/bionic/lib/libdl.so"
#define PATH_LIBC_BIONIC "/bionic/lib/libc.so"
#define PATH_LINKER "/system/bin/linker"
#define PATH_LIBDL "/system/lib/libdl.so"
#define PATH_LIBC "/system/lib/libc.so"
#define PATH_LIBANDROID_RUNTIME "/system/lib/libandroid_runtime.so"
#endif


// No need to reference manually, use HOOKLOG
//extern const char* _libinject_log_tag;
//extern int _libinject_log;

// Pass NULL to disable logging
//void libinject_log(const char* log_tag);


#define INJECTLOG(F,...)   { printf(F,##__VA_ARGS__);printf("\n");}
//#define INJECTLOG(F,...) {}

// Find pid for process
//pid_t libinject_find_pid_of(const char* process);

extern pid_t _pid;

// Load library in process pid, returns 0 on success
int libinject_injectvm(pid_t pid, char* library, char* param);

void trace_getregs(const char* debug, struct pt_regs * regs);

#endif

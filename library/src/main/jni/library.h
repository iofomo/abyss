//
// Created by mac on 2023/12/15.
//

#ifndef LIBRARY_H
#define LIBRARY_H

#include <unistd.h>
#include <stdbool.h>
#include "syscall/seccomp.h"



#define FILTERED_SYSNUM(SYSNUM) { PR_ ## SYSNUM, 0 }
#include "syscall/sysnum.h"
#include "syscall/seccomp.h"

typedef struct syscall_data {
    //tracee的进程id
    pid_t tracee_pid;
    //系统调用号
    Sysnum sysnum;
    //系统调用参数
//     unsigned long sysargs[6];
     //内部数据,请勿修改
     void* _internal;
     //上下文数据。使用者可以在系统调用进入前赋值,然后系统调用返回后自己使用
     void* user_context_data;
     //业务数据
}syscall_data;

//readlinkat传过来的业务数据
typedef struct business_data{
    //tracee的进程id
    pid_t tracee_pid;
    char data_in[PATH_MAX]; //传入的数据信息
    char result[PATH_MAX]; //响应结果
}business_data;

bool tracee_init(FilteredSysnum* add_filtered_sysnums,bool exclude_libc);

void tracer_init();
int trace_new_pid(int pid);
//系统调用进入时回调(在tracer进程)
void set_syscall_event_callback(void (*on_sysenter)(syscall_data* data),void (*on_sysexit)(syscall_data* data));
/**
 * 内存、寄存器操作
 */

word_t syscall_peek_reg(const syscall_data* sysdata,Reg reg);
void syscall_poke_reg(const syscall_data* sysdata,Reg reg, word_t value);
/**
 * 复制位于tracer中的tracer_ptr指向的data_size字节到tracee进程内。并让寄存器reg指向该buffer。
 */
int syscall_set_sysarg_data(const syscall_data* sysdata,const void *tracer_ptr, word_t data_size, Reg reg);

//赋值寄存器的值为c字符串
int syscall_set_sysarg_str(const syscall_data* sysdata,const char* tracer_ptr, Reg reg);
//从寄存器里获取数据缓冲区
int syscall_get_sysarg_data(const syscall_data* sysdata, char* dest_tracer,word_t max_size, Reg reg);
//从寄存器里获取c字符串值
int syscall_get_sysarg_str(const syscall_data* sysdata, char* dest_tracer,word_t max_size, Reg reg);

int syscall_write_data(const syscall_data* sysdata, word_t dest_tracee, const void *src_tracer, word_t size);

//int syscall_read_data(const syscall_data sysdata, void *dest_tracer, word_t src_tracee, word_t size);
//
//int syscall_read_string(const syscall_data sysdata, char *dest_tracer, word_t src_tracee, word_t max_size);

#endif //LIBRARY_H

//
// Created by mac on 2023/12/29.
//


//TODO 后面考虑 改成直接用印象笔记的那个demo
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ptrace.h>

#include <sys/wait.h>
#include <linux/uio.h>
#include <linux/elf.h>
#include "inject.h"

#define LOGD(...) { printf(__VA_ARGS__);printf("\n");}

static bool ignore_sigstop = false;
static pid_t child_pid = -1;

int syscall_status = 0;

#define IS_IN_SYSENTER() (syscall_status == 0)
#define IS_IN_SYSEXIT() (!IS_IN_SYSENTER())

void translate_syscall(int* restart_how){
    struct pt_regs regs;
    bool is_enter_stage = IS_IN_SYSENTER();
    long long sysnum = 0;
//    trace_getregs( "getregs", &regs );
#if defined(__arm__)
    //not impl!!!!
    sysnum = regs.uregs[7];
#elif  defined(__aarch64__)
    struct iovec ioVec;
    ioVec.iov_base = &regs;
    ioVec.iov_len = sizeof(regs);
    long status = ptrace(PTRACE_GETREGSET,child_pid,NT_PRSTATUS,&ioVec);
    if (status < 0){
        LOGD("PTRACE_GETREGSET,%d,%s",errno, strerror(errno))
    }
    sysnum = regs.uregs[8];
#endif

    LOGD("regs,r0:%llu,r1:%llu,r2:%llu,r3:%llu,sysnum:%llu",regs.uregs[1],regs.uregs[1],regs.uregs[2],regs.uregs[3],sysnum)
    if (is_enter_stage){
        syscall_status = 1;
        *restart_how = PTRACE_SYSCALL;
    }else {
        syscall_status = 0;
    }
}


void event_loop(){
    int status;
    while (1){
        pid_t pid;
        int signal;
        int tracee_status;
        pid = waitpid(-1, &tracee_status, __WALL);
        if (WIFSIGNALED(tracee_status)) { //Returns true if the process was terminated by a signal.
            LOGD("WIFSIGNALED")
            break;
        } else if (WIFSTOPPED(tracee_status)) { //tracee产生了一个一个信号 Returns true if the process was stopped by a signal.
            signal = (tracee_status & 0xfff00) >> 8;
            LOGD("WIFSTOPPED,%d",signal)
            int restart_how = PTRACE_CONT;
            switch (signal) {
                case SIGTRAP: {
                    static bool deliver_sigtrap = false;

                    const unsigned long default_ptrace_options = (
                            PTRACE_O_TRACESYSGOOD	|
                            PTRACE_O_TRACEFORK	|
                            PTRACE_O_TRACEVFORK	|
                            PTRACE_O_TRACEVFORKDONE	|
                            PTRACE_O_TRACEEXEC	|
                            PTRACE_O_TRACECLONE	|
                            PTRACE_O_TRACEEXIT);
                    if (deliver_sigtrap)
                        break;  /* Deliver this signal as-is.  */
                    deliver_sigtrap = true;
                    //TODO 启用seccomp mode 2
                    status = ptrace(PTRACE_SETOPTIONS, pid, NULL,
                                    default_ptrace_options);
                    if (status < 0) {
                        LOGD("ptrace(PTRACE_SETOPTIONS)")
                        exit(EXIT_FAILURE);
                    }
                }
                case SIGTRAP |0x80:
                    signal = 0;
                    translate_syscall(&restart_how);
                    break;
                case SIGSTOP:
                    if (!ignore_sigstop){
                        LOGD("ignore sigstop -----------------")
                        ignore_sigstop = true;
                        signal = 0;
                    }
                    break;
            }
            status =  ptrace(restart_how,pid,NULL,signal);
            //获取寄存器的值
            if (status < 0){
                LOGD("PTRACE_SYSCALL error,%d,%s",errno, strerror(errno))
                break;
            }
        } else if (WIFEXITED(status)) { //Returns true if the process exited normally.
            LOGD("WIFEXITED,%d",pid)
            break;
        }else{
//            LOGD("not support ----------------,%d",status)
        }
    }
}

int main(){
    LOGD("in ptracedemo,%d",getpid());
    int status;
    int ret;
    pid_t child = fork();
    if (child == 0){
        //child
        status = ptrace(PTRACE_TRACEME,0,NULL,NULL);
        if (status < 0){
            LOGD("ptrace error,%d,%s",errno, strerror(errno));
            return 0;
        }
        LOGD("before sigstop----")
        kill(getpid(),SIGSTOP);
        LOGD("after sigstop----")
        char *newargv[] = { NULL, "hello", "world", NULL };
        char *newenviron[] = { NULL };

        newargv[0] = "./myecho";

        execve("./myecho", newargv, newenviron);
        perror("execve");   /* execve() returns only on error */
        exit(EXIT_FAILURE);
    }else if (child > 0){
        LOGD("child pid:%d",child)
        //parent
        //waitpid 、wait
        //记录子进程的pid,方便inject.cpp使用
        _pid = child;
        child_pid = child;
        event_loop();
    }else{
        //error
        LOGD("fork error,%d,%s",errno,strerror(errno));
    }
    return 0;
}

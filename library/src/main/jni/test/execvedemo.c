//
// Created by mac on 2023/12/26.
//
/* execve.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include "constant.h"
#include "library.h"
#include "cmn/cmn_proc.h"
#include <jni.h>

//#define LOGD(...) { printf(__VA_ARGS__);printf("\n");}

static FilteredSysnum add_filtered_sysnums[] = {
        { PR_execve,		FILTER_SYSEXIT },
        { PR_execveat,		FILTER_SYSEXIT },
        {PR_openat,FILTER_SYSEXIT},
//        FILTERED_SYSNUM(read),
//        FILTERED_SYSNUM(close),
        FILTERED_SYSNUM_END
};

static void test(){
    char *newargv[] = { NULL, "hello", "world", NULL };
    char *newenviron[] = { NULL };

    newargv[0] = "./myecho";

    execve("./myecho", newargv, newenviron);
    perror("execve");   /* execve() returns only on error */
    exit(EXIT_FAILURE);
}


int new_child_tracee(int index){
    LOGD("new_child_tracee ----,%d",index)
    int pipefd_to_parent[2];
    int pipefd_to_child[2];
    pid_t ppid = getpid();

    if (pipe(pipefd_to_parent) < 0 ||
        pipe(pipefd_to_child) < 0){
        perror("failed to create pipe");
        return -1;
    };
    pid_t pid = fork();

    if (pid == 0){ //child tracee
        LOGD("new child------,cur pid %d,parent pid %d",getpid(),ppid)
        char message[20];
        close(pipefd_to_parent[0]);
        close(pipefd_to_child[1]);
        stpcpy(message,"trace me!");
        write(pipefd_to_parent[1],message, sizeof(message));

        LOGD("wait for trace-------%d",getpid())

        int size = read(pipefd_to_child[0],message, sizeof(message)); //TODO 第二个进程,到这出现了signal 5
        if (size <= 0){
            LOGD("child read error,%d,%d,%s",size,errno, strerror(errno))
            return 0;
        }
        LOGD("finish traced,[%s]",message)
        LOGD("child cont,%s-------------------------------------------",message);
        close(pipefd_to_parent[1]);
        close(pipefd_to_child[0]);
        if (index == 1 || index == 2){
            tracee_init(add_filtered_sysnums,false);
        }
        test();
        LOGD("child exit,%d",getpid());
        exit(0);
    }else if (pid > 0){ //parent tracer
        LOGD("new_child_tracee invoked,parent");
        char message[20] = "go!";
        close(pipefd_to_parent[1]);
        close(pipefd_to_child[0]);

        read(pipefd_to_parent[0],message, sizeof(message));
        LOGD("received trace request,[%s]",message)
        int ret = 0;
        if (index == 1 || index == 2){
            ret = trace_new_pid(pid);
        }
        LOGD("trace finished,notify child")
        sprintf(message,"%d",ret);
        write(pipefd_to_child[1],message, sizeof(message));
        LOGD("resume child:---------------%d",pid)
        close(pipefd_to_parent[0]);
        close(pipefd_to_child[1]);
    }else{
        printf("error\n");
    }
    return pid;
}

void* new_child_tracee_thread(void* data){
    LOGD("new_child_tracee_thread----%d",getpid())
    new_child_tracee(1);
    LOGD("new_child_tracee_thread end")
    return NULL;
}

//in tracer---
void on_sysenter(syscall_data* data){
    LOGD("on_sysenter syscall_num:%d",data->sysnum)
    switch (data->sysnum) {
        case PR_openat:{
            // int openat(int dirfd, const char *pathname, int flags, mode_t mode);
            LOGD("PR_openat ----%d,%d,sysarg2:0x%lx，0x%lx",getpid(),PR_openat,peek_reg(data->_internal, CURRENT, SYSARG_2),
                 syscall_peek_reg(data,SYSARG_2))
        }
            break;
    }
}

void on_sysexit(syscall_data* data){
    char path[PATH_MAX];
    LOGD("on_sysexit syscall_num:%d",data->sysnum)
    switch (data->sysnum) {
        case PR_openat:{
            if (syscall_get_sysarg_str(data,path,PATH_MAX,SYSARG_2) > 0 && !strcmp("/proc/self/maps",path)){
                LOGD("set result,raw %d", syscall_peek_reg(data,SYSARG_RESULT))
                syscall_poke_reg(data,SYSARG_RESULT,100);
            }else{
                LOGD("on_sysexit read error,%d,%s",errno, strerror(errno))
            }
        }
            break;
    }
}

int main(int argc,char * const argv[]){
    if (argc > 1){
       int fd =  open("/proc/self/maps6664",O_RDONLY);
        close(fd);
        test();
        return 0;
    }
    tracer_init();
    set_syscall_event_callback(on_sysenter,on_sysexit);

    //用新线程去fork
    pthread_t tid1;
    pthread_create(&tid1,NULL,new_child_tracee_thread,NULL);
    pthread_join(tid1,NULL);

    if (pthread_join(work_tid,NULL) != 0){
        printf("failed to join,cur_pid:%d\n",getpid());
    }
    LOGD("main exit");
    return 0;
}

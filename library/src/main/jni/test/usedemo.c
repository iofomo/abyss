//
// Created by mac on 2023/12/15.
//

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
#include <sys/mman.h>
#include "app/disguise.h"


#if defined(ARCH_X86_64)
#    include "loader/assembly-x86_64.h"
#elif defined(ARCH_ARM_EABI)
#    include "loader/assembly-arm.h"
#elif defined(ARCH_X86)
#    include "loader/assembly-x86.h"
#elif defined(ARCH_ARM64)
#    include "loader/assembly-arm64.h"
#else
#    error "Unsupported architecture"
#endif

void print_file(const char * path){
    printf("start print %s\n",path);
    FILE* in_fp = fopen(path,"r");
    if (in_fp == NULL){
        printf("fopen error,path:%s,%d,%s\n",path,errno, strerror(errno));
        return;
    }
    //CASE: fstat
    struct stat m_stat;
    int fd = fileno(in_fp);
    if (fstat(fd,&m_stat) != 0){
        printf("fstat read error,%d,%s\n",errno, strerror(errno));
        fclose(in_fp);
        return;
    };
    printf("start stat-----\n");
    //status raw:st_size:0,st_blocks:0,st_mode:33060,st_uid:2000,st_gid:2000,st_dev:4
    printf("st_size:%ld,st_blocks:%ld,st_mode:%d,st_uid:%d,st_gid:%d,st_dev:%lu\n",
           m_stat.st_size,m_stat.st_blocks,m_stat.st_mode,m_stat.st_uid,m_stat.st_gid,m_stat.st_dev);
    printf("end stat-----\n");

    //readlinkat
    char buf[128];
    sprintf(buf,"/proc/self/fd/%d",fd);
    ssize_t len = readlink(buf,buf, sizeof(buf));
    if (len > 0){
        buf[len] = '\0';
    }else {
        printf("readlink error,  fd %d, %d,%s\n",fd,errno, strerror(errno));
    }
    printf("readlink fd %d,len:%zd,result: %s\n",fd,len,buf);
    printf("in_fp:%p\n",in_fp);
    char buffer[PATH_MAX];
    while (!feof(in_fp)) {
        if (__UNLIKELY(!fgets(buffer, PATH_MAX, in_fp))) {
            printf("read %s finish,eof:%d,error:%d\n",path, feof(in_fp), ferror(in_fp));
            break;
        }
        printf("%s",buffer);
    }
    printf("end print %s\n",path);
}

void test_status(){
    print_file("/proc/self/status");
}

static void test(){
    //readlink mts-
    readlinkat(0,"/mts-1/com.demo,.ifma,ifmamts",NULL,0);
    readlinkat(0,"/mts-2//data/local/tmp/a.txt,/data/local/tmp/b.txt",NULL,0);
    test_status();
    FILE * fp = fopen("/data/local/tmp/a.txt","r");
    char buf[128];
    fgets(buf, sizeof(buf),fp);
    printf("open a:%d,%s\n", fileno(fp),buf);
    fclose(fp);
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
            tracee_init(get_add_filtered_sysnums(),false);
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
    if (argc == 2){
        test();
        return 0;
    }
    tracer_init();
    set_syscall_event_callback(disguise_on_sysenter,disguise_on_sysexit);
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
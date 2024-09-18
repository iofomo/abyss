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
#include "library.h"
#include "ptrace/ptrace.h"
#include "constant.h"
#include "event.h"
#include "cmn/cmn_pthread_cond.h"
#include "cmn/cmn_proc.h"


#define USE_PTRACE

//是否多tracee进程
//#define USE_MUL_TRACEE_PROCESS

static void test(){
    //test sigev
//    uint64_t*  fp = __builtin_frame_address(0);
//    LOGD("test cur fp %p",fp);
//     long* ptr = 10;
//    LOGD("val:%ld\n",*ptr);

    errno = 0;
    char file_path[40];
    sprintf(file_path,"/proc/%d/maps",getpid());
    LOGD("pid:%d,before open:%s",getpid(),file_path) //第二个线程从这里开始没有东西
    int fd =  open(file_path,O_RDONLY); //有open时 就会报栈破坏,没有就没事
    LOGD("pid:%d,after open:%s",getpid(),file_path)

//    LOGI("tracee open fd,%d,err:%d,%s",fd,errno, strerror(errno));
    close(fd); //这个貌似被过滤忽略掉了
//    char path[40];
//    for (int i = 0; i < 10; ++i) {
//        sprintf(path,"/proc/self/maps%d",i);
//        fd =  open(path,O_RDONLY); //有open时 就会报栈破坏,没有就没事
//         close(fd);
//        sleep(1);
//    }
}

static FilteredSysnum add_filtered_sysnums[] = {
        FILTERED_SYSNUM(openat),
        FILTERED_SYSNUM(close),
        FILTERED_SYSNUM_END
};
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
    //fuck https://blog.csdn.net/qq_42961603/article/details/129236882
    //fork后,会复制发起调用的线程
    pid_t pid = fork();

    if (pid == 0){ //child tracee
        LOGD("new child------,cur pid %d,parent pid %d",getpid(),ppid);
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
#if defined(USE_PTRACE)
            tracee_init(add_filtered_sysnums,false);
#endif
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
#if defined(USE_PTRACE)
            ret = trace_new_pid(pid);
#endif
        }
        LOGD("trace finished,notify child")
        sprintf(message,"%d",ret);
        write(pipefd_to_child[1],message, sizeof(message));
        LOGD("resume child:---------------%d",pid);

        close(pipefd_to_parent[0]);
        close(pipefd_to_child[1]);

    }else{
        printf("error\n");
    }
    return pid;
}

void* new_child_tracee_thread(void* data){
    LOGD("new_child_tracee_thread----%d",getpid())
    pid_t child1 = new_child_tracee(1);
    LOGD("new_child_tracee_thread end")
    return NULL;
}

int main(int argc,char * const argv[]){
    LOGD("sss  testsvc welcome,%d,argc:%d",getpid(),argc);
//    print_maps(NULL);
    if (argc > 1){
        test();
        printf("just test done----\n");
        return 0;
    }
#if defined(USE_PTRACE)
    tracer_init();
#endif
    LOGD("test before sleep,%d",getpid());
    sleep(3);
    LOGD("test after sleep,%d",getpid());

    //用新线程去fork
    pthread_t tid1;
    pthread_create(&tid1,NULL,new_child_tracee_thread,NULL);

    pthread_join(tid1,NULL);

#if defined(USE_MUL_TRACEE_PROCESS)
    LOGD("tid1 done,before child 2---------------------------,%d",getpid())
    pthread_t tid2;
    pthread_create(&tid2,NULL,new_child_tracee_thread,NULL);
#endif


    int * ret;
    if (pthread_join(work_tid,NULL) != 0){
        printf("failed to join,cur_pid:%d\n",getpid());
    }

    LOGD("main exit");

    return 0;
}
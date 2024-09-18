//
// Created by mac on 2023/12/22.
//
//pthread相关研究

#include "cmn/cmn_pthread_cond.h"
#include <stdio.h>
#include <unistd.h>
//#include "constant.h"

#define LOGD(...) { printf(__VA_ARGS__);printf("\n");}

static struct PCond_Context ctx;

void* work1(void* data){
    LOGD("work1 wait for lock----")
    cmn_pt_wait(&ctx);
    LOGD("work1 after lock-------")
    return NULL;
}

void* work2(void* data){
    LOGD("work2 begin sleep -----")
    sleep(3);
    LOGD("work2 after sleep,notify")
    cmn_pt_signal(&ctx);
    LOGD("work2 notify done")
    return NULL;
}
int main(int argc,const char * argv[]){
    LOGD("test %s",argv[0])
    cmn_pt_context_init(&ctx);
    pthread_t t1;
    pthread_create(&t1,NULL,work1,NULL);

    pthread_t t2;
    pthread_create(&t2,NULL,work2,NULL);

    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    LOGD("main after thread exit")
    cmn_pt_destroy(&ctx);
    LOGD("main exit")
    return 0;
}

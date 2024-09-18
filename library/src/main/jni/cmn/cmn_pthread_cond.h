//
// Created by mac on 2023/12/14.
//

#ifndef CMN_PTHREAD_COND_H
#define CMN_PTHREAD_COND_H
#include <pthread.h>
#include <stdbool.h>

struct PCond_Context{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

/**
 * 初始化
 */
void cmn_pt_context_init(struct PCond_Context* ctx);

/**
 * 等待事件发生
 */
void cmn_pt_wait(struct  PCond_Context* ctx);

void cmn_pt_wait2(struct  PCond_Context* ctx,bool (*check)());

/**
 * 通知所有等待线程
 */
int cmn_pt_broadcast(struct PCond_Context* ctx);

/**
 * 通知第一个等待的线程
 */
int cmn_pt_signal(struct  PCond_Context* ctx);

int cmn_pt_signal2(struct  PCond_Context* ctx,bool (*check)());


 /**
  * 清除资源
  */
 int cmn_pt_destroy(struct PCond_Context* ctx);


#endif //CMN_PTHREAD_COND_H

//
// Created by mac on 2023/12/14.
//


#include "cmn_pthread_cond.h"


/**
 * 初始化
 */
void cmn_pt_context_init(struct PCond_Context* ctx){
    pthread_mutex_init(&ctx->mutex, NULL);
    pthread_cond_init(&ctx->cond, NULL);
}

/**
 * 等待事件发生
 */
void cmn_pt_wait(struct  PCond_Context* ctx){
    pthread_mutex_lock(&ctx->mutex);
    pthread_cond_wait(&ctx->cond, &ctx->mutex);
    pthread_mutex_unlock(&ctx->mutex);
}

void cmn_pt_wait2(struct  PCond_Context* ctx,bool (*check)()){
    pthread_mutex_lock(&ctx->mutex);
    if (check()){
        pthread_cond_wait(&ctx->cond, &ctx->mutex);
    }
    pthread_mutex_unlock(&ctx->mutex);
}

/**
 * 通知所有等待线程
 */
int cmn_pt_broadcast(struct PCond_Context* ctx){
    return pthread_cond_broadcast(&ctx->cond);
}

/**
 * 通知第一个等待的线程
 */
int cmn_pt_signal(struct  PCond_Context* ctx){
    return pthread_cond_signal(&ctx->cond);
}

int cmn_pt_signal2(struct  PCond_Context* ctx,bool (*check)()){
    pthread_mutex_lock(&ctx->mutex);
    int ret = 0;
    if (check()){
        ret  =  pthread_cond_signal(&ctx->cond);
    }
    pthread_mutex_unlock(&ctx->mutex);
    return ret;
}


/**
 * 清除资源
 */
int cmn_pt_destroy(struct PCond_Context* ctx){
    return pthread_cond_destroy(&ctx->cond);
}
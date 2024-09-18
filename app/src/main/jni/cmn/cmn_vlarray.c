//
// Created by mac on 2023/12/13.
//

#include "cmn_vlarray.h"
#include <stdlib.h>

static int head_size(){
    return sizeof(int);
}

static void* org_ptr(void * ctx){
    return ctx - head_size();
}

static void* arr_ptr(void* org_ptr){
    return org_ptr + head_size();
}
static void set_arr_length(void* org_ptr,int size){
    int* size_ptr = org_ptr;
    *size_ptr = size;
}

void * vl_new_array(int ele_size,int count){
    int cap = ele_size * count + head_size();
    void * ptr = malloc(cap);
    if (!ptr) return NULL;
    set_arr_length(ptr,count);
    return arr_ptr(ptr);
}

//获取数组长度
int vl_array_length(void * ctx){
    int * ptr = org_ptr(ctx);
    return *ptr;
}

//改变数组长度
void * vl_array_realloc(void * ctx,int ele_size,int count){
    int except = ele_size * count + head_size();
    void *  ptr = realloc(org_ptr(ctx),except);
    if (!ptr) return NULL;
    set_arr_length(ptr,count);
    return arr_ptr(ptr);
}

void vl_array_free(void * ctx){
    free(org_ptr(ctx));
}

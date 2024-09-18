//
// Created by mac on 2023/12/13.
//

#ifndef CMN_VLARRAY_H
#define CMN_VLARRAY_H
/**
 * 内存结构"size(int) + [e1,e2...en]"
 eg.
     struct AAA{
        int a;
        int b;
    };
     void * ctx = vl_new_array(sizeof(struct AAA),0);
    printf("数组长度:%d\n", vl_array_length(ctx));
    ctx = vl_array_realloc(ctx, sizeof(struct AAA),5);
    printf("数组长度:%d\n", vl_array_length(ctx));
    struct AAA * arr = ctx;
    printf("数组3,a=%d,b=%d\n",arr[3].a,arr[3].b);
    for (int i = 0; i < vl_array_length(ctx); ++i) {
        arr[i].a = arr[i].b = i;
    }
    printf("数组3,a=%d,b=%d\n",arr[3].a,arr[3].b);
    vl_array_free(ctx);
 */

void * vl_new_array(int ele_size,int count);

int vl_array_length(void * ctx);


void * vl_array_realloc(void * ctx,int ele_size,int count);

void vl_array_free(void * ctx);

#endif //CMN_VLARRAY_H

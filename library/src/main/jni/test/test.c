//
// Created by mac on 2023/12/21.
//

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>

//堆栈打印
//ARM64

int def(){
    //x29
    //打印堆栈
    uint64_t* fp = __builtin_frame_address(0);
    //TODO 暂时忽略当前地址和寄存器

    //基于bp指针和lr指针的位置关系得到堆栈
    while (fp != NULL)
    {
        //  printf("stack:,fp:%p,lr_addr:%p lr:%lx\n",fp,(fp + 1),*((uint64_t *)(fp + 1)));
        printf("bt:0x%lx\n",*((uint64_t *)(fp + 1)));
        fp = *fp;
    }

    printf("bt finished");
    return 0;
}

void abc(){
    // long* ptr = 10;
    // printf("val:%ld\n",*ptr);

    long ptr = 10;
    printf("val:%ld\n",ptr);
    def();
    //printf("val:%ld\n",++ptr);
}



int main(){
    // printf("hi\n");
    abc();
}
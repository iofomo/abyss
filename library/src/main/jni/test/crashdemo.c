#include <stdio.h>

void abc(){
    long* ptr = 10;
    printf("val:%ld\n",*ptr);
}

int main(){
    printf("hi\n");
    abc();
    return 0;
}
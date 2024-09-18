//
// Created by mac on 2023/12/27.
//

#include <stdio.h>
#include <string.h>

static FILE * fp;

void write_line(char* line){
    if (fp == NULL){
        fp = fopen("./a.txt","a");
    }
    fwrite(line,strlen(line),1,fp);
    fwrite("\n",1,1,fp);
    fflush(fp);
}

int main(int argc,const char * argv[]){
    write_line("haha1");
    write_line("hahah2");

    char message_line[2000] = {0};
    sprintf(message_line,"%s%s","add","fff");
    write_line(message_line);
    return 0;
}
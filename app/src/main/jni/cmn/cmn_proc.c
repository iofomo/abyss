//
// Created by mac on 2023/12/19.
//

#include <unistd.h>
#include <string.h>
#include "cmn_proc.h"
#include "../constant.h"

void print_maps(char *filter){
    FILE * fp = fopen("/proc/self/maps","r");
    if (!fp){
        LOGE("fp is null")
        return;
    }
    LOGD("start print %d maps,filter:%s",getpid(),filter);
    size_t len = 0;
    ssize_t nread;
    char * line = NULL;

    while ((nread = getline(&line, &len, fp)) != -1) {
//        printf("Retrieved line of length %zu:\n", nread);
        line[nread] = '\0';
        if (filter != NULL && strlen(filter) > 0){
            if (strstr(line,filter) != NULL){
                LOGD("proc_maps:%s",line)
            }
        }else{
            LOGD("proc_maps:%s",line)
        }
    }
    fclose(fp);
    LOGD("end print %d maps",getpid());
}
//
// Created by mac on 2024/1/15.
//

#include "cmn_utils.h"
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h> //AT_*

bool cmn_utils_string_starts_With(const char* str,const char* sub_str){
    if (!str) return !sub_str;
    if(!sub_str) return false;
    size_t str_len = strlen(str);
    size_t sub_str_len = strlen(sub_str);
    if (!str_len) return !sub_str_len;
    return strstr(str,sub_str) == str;
}

bool cmn_utils_string_ends_With(const char* str,const char* sub_str){
    if (!str) return !sub_str;
    if(!sub_str) return false;
    size_t str_len = strlen(str);
    size_t sub_str_len = strlen(sub_str);
    if (!str_len) return !sub_str_len;
    if (str_len < sub_str_len) return false;
    return !strncmp(&str[str_len - sub_str_len],sub_str,str_len - sub_str_len);
}


char* cmn_utils_string_replace_With(const char* str,const char* sub_str,const char* rep_str){
    //不支持NULL的替换
    if (!sub_str || !rep_str) return NULL;
    size_t sub_str_len = strlen(sub_str);
    size_t rep_str_len = strlen(rep_str);
    size_t str_len = strlen(str);

    const char* find = strstr(str,sub_str);
    char *ret = NULL;
    if (!find){
        ret = malloc(sizeof(char)*sizeof(str_len + 1));
        strcpy(ret,str);
        return ret;
    }
    ret = malloc(sizeof(char)*sizeof(str_len + rep_str_len - sub_str_len));
    strncpy(ret,str,find - str);
    strncpy(&ret[find - str],rep_str,rep_str_len);
    strcpy(&ret[find - str + rep_str_len],&find[sub_str_len]);
    return ret;
}




ssize_t cmn_utils_readlink(pid_t pid,int fd, char* buf,size_t max_size) {
    int ret = 0;
    sprintf(buf, "/proc/%d/fd/%d",pid,fd);
    ret = readlinkat(AT_FDCWD, buf, buf, max_size);
    if (0 <= ret) {
        buf[ret] = '\0';
    }
    return ret;
}

int cmn_utils_string_split(char* src, char ch, char* items[], int items_len) {
    if (!src || !items) return 0;

    int i = 0, cnt = 0;
    memset(items, 0, items_len*sizeof(char*));
    while (i < items_len && *src) {
        if (!items[i]) {
            ++ cnt;
            items[i] = src;
        }
        if (*src == ch) {
            ++ i;
            *src = '\0';
        }
        ++ src;
    }
    return cnt;
}
//字符串hash函数
uint32_t cmn_utils_str_hash(const char* str){
    uint32_t h = 0, g;
    const char* ptr = str;
    while (*ptr) {
    h = (h << 4) + *ptr++;
    g = h & 0xf0000000;
    h ^= g;
    h ^= g >> 24;
    }
    return h;
}
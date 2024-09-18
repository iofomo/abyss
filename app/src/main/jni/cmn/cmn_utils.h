//
// Created by mac on 2024/1/15.
//

#ifndef  CMN_UTILS_H
#define CMN_UTILS_H
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

bool cmn_utils_string_starts_With(const char* str,const char* sub_str);
bool cmn_utils_string_ends_With(const char* str,const char* sub_str);
char* cmn_utils_string_replace_With(const char* str,const char* sub_str,const char* rep_str);
ssize_t cmn_utils_readlink(pid_t pid,int fd, char* buf,size_t max_size);
int cmn_utils_string_split(char* src, char ch, char* items[], int items_len);
uint32_t cmn_utils_str_hash(const char* str);

#endif //CMN_UTILS_H

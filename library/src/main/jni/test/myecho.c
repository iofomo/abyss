//
// Created by mac on 2023/12/26.
//

/* myecho.c */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{

    char * path = "/proc/self/mapsecho";
    printf("before open /proc/self/mapsecho,%p\n",path);
    int fd = open(path,O_RDONLY);
    printf("after open /proc/self/mapsecho,%d\n",fd);
    close(fd);

    int j;
    for (j = 0; j < argc; j++)
        printf("argv[%d]: %s\n", j, argv[j]);

    exit(EXIT_SUCCESS);
}
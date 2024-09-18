//
// Created by mac on 2023/12/22.
//
/*
 * This sample show how to use futex betwen two process, and use system v
 * shared memory to store data
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#if __GLIBC_PREREQ(2, 3)
#if defined FUTEX_WAIT || defined FUTEX_WAKE
#include <linux/futex.h>
#else
#define FUTEX_WAIT      0
#define FUTEX_WAKE      1
#endif

#ifndef __NR_futex
#define __NR_futex     202
#endif
#endif

#define FILE_MODE (S_IRUSR | S_IWUSR)

const char shmfile[] = "/tmp";
const int size = 100;

struct namelist
{
    int  id;
    char name[20];
};

int
main(void)
{
    int fd, pid, status;
    int *ptr;
    struct stat stat;

    // create a Posix shared memory
    int flags = O_RDWR | O_CREAT;
    fd = shm_open(shmfile, flags, FILE_MODE);
    if (fd < 0)
    {
        printf("shm_open failed, errormsg=%s errno=%d", strerror(errno), errno);
        return 0;
    }
    ftruncate(fd, size);
    ptr = (int *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    pid = fork();
    if (pid == 0) { // child process
        sleep(5);
        printf("Child %d: start/n", getpid());

        fd = shm_open(shmfile, flags, FILE_MODE);
        fstat(fd, &stat);
        ptr = (int *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        struct namelist tmp;

        // store total num in ptr[0];
        *ptr = 3;

        namelist *cur = (namelist *)(ptr+1);

        // store items
        tmp.id = 1;
        strcpy(tmp.name, "Nellson");
        *cur++ = tmp;
        tmp.id = 2;
        strcpy(tmp.name, "Daisy");
        *cur++ = tmp;
        tmp.id = 3;
        strcpy(tmp.name, "Robbie");
        *cur++ = tmp;

        printf("wake up parent/n");
        syscall(__NR_futex ,ptr, FUTEX_WAKE, 1, NULL );

        exit(0);
    } else{ // parent process
        printf("parent start waiting/n");
        syscall(__NR_futex , ptr, FUTEX_WAIT, *(int *)ptr, NULL );
        printf("parent end waiting/n");

        struct namelist tmp;

        int total = *ptr;
        printf("/nThere is %d item in the shm/n", total);

        ptr++;
        namelist *cur = (namelist *)ptr;

        for (int i = 0; i< total; i++) {
            tmp = *cur;
            printf("%d: %s/n", tmp.id, tmp.name);
            cur++;
        }

        printf("/n");
        waitpid(pid, &status, 0);
    }

    // remvoe a Posix shared memory from system
    printf("Parent %d get child status:%d/n", getpid(), status);
    return 0;
}

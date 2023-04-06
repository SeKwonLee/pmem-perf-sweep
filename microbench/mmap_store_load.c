#define _GNU_SOURCE
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>

#define FILE_SIZE           (4ULL*1024*1024*1024)
#define TOTAL_IO_SIZE       (2ULL*1024*1024*1024)
#define IO_SIZE             (256)

struct timeval start, end;

int main(int argc, char *argv[]){
    int fd = open("/mnt/pmem0/test.txt", O_CREAT | O_RDWR, 0666);
    if(fd < 0){
        printf("Open Failed\n");
        exit(1);
    }

    fallocate(fd, FALLOC_FL_ZERO_RANGE, 0, FILE_SIZE);

    char *ptr = mmap(NULL, FILE_SIZE,
            PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(ptr == MAP_FAILED){
        printf("Mapping Failed\n");
        return 1;
    }

    uint64_t t, i = 0;
    uint64_t numops = TOTAL_IO_SIZE / IO_SIZE;

    char *buf = NULL;
    uint64_t offset = 0;

    buf = (char *) malloc(sizeof(char) * IO_SIZE);
    memset(buf, 'V', sizeof(char) * IO_SIZE);

    for (i = 0; i < FILE_SIZE / IO_SIZE; i++) {
        memset((char *)((uint64_t)ptr + offset), 'A', IO_SIZE);
        offset += IO_SIZE;
    }

    int err = munmap(ptr, FILE_SIZE);
    if(err != 0){
        printf("UnMapping Failed\n");
        return 1;
    }

    ptr = mmap(NULL, FILE_SIZE,
            PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(ptr == MAP_FAILED){
        printf("Mapping Failed\n");
        return 1;
    }

#ifdef ENABLE_PREFAULT
    // prefault the mmap region first
    offset = 0;
    for (i = 0; i < numops; i++) {
        memset((char *)((uint64_t)ptr + offset), 'B', IO_SIZE);
        offset += IO_SIZE;
    }
#endif

    gettimeofday(&start, NULL);

    offset = 0;
    for (i = 0; i < numops; i++) {
#ifdef ENABLE_STORE
        memcpy((char *)((uint64_t)ptr + offset), buf, IO_SIZE);
#elif ENABLE_LOAD
        memcpy(buf, (char *)((uint64_t)ptr + offset), IO_SIZE);
#endif
        offset += IO_SIZE;
    }

    gettimeofday(&end, NULL);
    
    t = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);

    printf("\nThroughput: %0.2fM ops/sec\n", ((float)(numops)/t));

    int err = munmap(ptr, FILE_SIZE);
    if(err != 0){
        printf("UnMapping Failed\n");
        return 1;
    }

    err = munmap(ptr, FILE_SIZE);
    if(err != 0){
        printf("UnMapping Failed\n");
        return 1;
    }

    close(fd);
    return 0;
}


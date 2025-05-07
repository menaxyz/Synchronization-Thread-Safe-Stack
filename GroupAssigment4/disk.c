#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "disk.h"

#define DISK_BLOCKS 8192

static int disk_fd = -1;

int make_disk(const char *name) {
    int fd = open(name, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd < 0) return -1;

    char buf[BLOCK_SIZE];
    memset(buf, 0, BLOCK_SIZE);
    for (int i = 0; i < DISK_BLOCKS; i++) {
        if (write(fd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
            close(fd);
            return -1;
        }
    }
    close(fd);
    return 0;
}

int open_disk(const char *name) {
    if (disk_fd != -1) return -1;
    disk_fd = open(name, O_RDWR, 0666);
    return (disk_fd < 0) ? -1 : 0;
}

int close_disk() {
    if (disk_fd == -1) return -1;
    close(disk_fd);
    disk_fd = -1;
    return 0;
}

int read_block(int block_num, void *buf) {
    if (disk_fd == -1) return -1;
    if (block_num < 0 || block_num >= DISK_BLOCKS) return -1;
    off_t offset = block_num * BLOCK_SIZE;
    if (lseek(disk_fd, offset, SEEK_SET) < 0) return -1;
    if (read(disk_fd, buf, BLOCK_SIZE) != BLOCK_SIZE) return -1;
    return 0;
}

int write_block(int block_num, const void *buf) {
    if (disk_fd == -1) return -1;
    if (block_num < 0 || block_num >= DISK_BLOCKS) return -1;
    off_t offset = block_num * BLOCK_SIZE;
    if (lseek(disk_fd, offset, SEEK_SET) < 0) return -1;
    if (write(disk_fd, buf, BLOCK_SIZE) != BLOCK_SIZE) return -1;
    return 0;
}

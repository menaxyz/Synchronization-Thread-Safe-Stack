#ifndef DISK_H
#define DISK_H

#define BLOCK_SIZE 4096

int make_disk(const char *name);
int open_disk(const char *name);
int close_disk();
int read_block(int block_num, void *buf);
int write_block(int block_num, const void *buf);

#endif

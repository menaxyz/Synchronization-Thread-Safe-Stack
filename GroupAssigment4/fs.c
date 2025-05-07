//group 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "disk.h"

#define BLOCK_SIZE 4096
#define MAX_FILES 64
#define MAX_FILE_NAME 15
#define MAX_FD 32
#define FAT_EOF 0xFFFFFFFF
#define DATA_BLOCKS 4096
#define TOTAL_BLOCKS 8192

typedef struct {
    uint32_t fat_start;
    uint32_t dir_start;
    uint32_t data_start;
    uint32_t num_blocks;
    uint32_t num_data_blocks;
    uint32_t num_files;
} superblock_t;

typedef struct {
    char name[16];
    uint32_t size;
    uint32_t first_block;
    uint8_t used;
} dir_entry_t;

typedef struct {
    int used;
    int dir_index;
    uint32_t offset;
} file_descriptor_t;

int fs_close(int fildes);


// Global variables
superblock_t sb;
dir_entry_t directory[MAX_FILES];
uint32_t fat[DATA_BLOCKS];
file_descriptor_t fdt[MAX_FD];
int disk_mounted = 0;

// Helper Functions
void write_meta() {
    write_block(0, &sb);
    write_block(1, fat);
    write_block(2, directory);
}

void read_meta() {
    read_block(0, &sb);
    read_block(1, fat);
    read_block(2, directory);
}

int allocate_block() {
    for (int i = 0; i < DATA_BLOCKS; ++i) {
        if (fat[i] == 0) {
            fat[i] = FAT_EOF;
            return i;
        }
    }
    return -1;
}

int find_file(const char *name) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (directory[i].used && strcmp(directory[i].name, name) == 0)
            return i;
    }
    return -1;
}

// Core FS API
int make_fs(char *disk_name) {
    if (make_disk(disk_name) != 0 || open_disk(disk_name) != 0)
        return -1;

    memset(&sb, 0, sizeof(sb));
    sb.fat_start = 1;
    sb.dir_start = 2;
    sb.data_start = 3;
    sb.num_blocks = TOTAL_BLOCKS;
    sb.num_data_blocks = DATA_BLOCKS;
    sb.num_files = MAX_FILES;

    memset(fat, 0, sizeof(fat));
    memset(directory, 0, sizeof(directory));

    write_meta();
    close_disk();
    return 0;
}

int mount_fs(char *disk_name) {
    if (open_disk(disk_name) != 0)
        return -1;
    read_meta();
    memset(fdt, 0, sizeof(fdt));
    disk_mounted = 1;
    return 0;
}

int umount_fs(char *disk_name) {
    if (!disk_mounted)
        return -1;

    for (int i = 0; i < MAX_FD; ++i) {
        if (fdt[i].used)
            fs_close(i);
    }
    write_meta();
    close_disk();
    disk_mounted = 0;
    return 0;
}

int fs_create(char *name) {
    if (strlen(name) > MAX_FILE_NAME || find_file(name) != -1)
        return -1;

    for (int i = 0; i < MAX_FILES; ++i) {
        if (!directory[i].used) {
            strcpy(directory[i].name, name);
            directory[i].used = 1;
            directory[i].size = 0;
            directory[i].first_block = FAT_EOF;
            return 0;
        }
    }
    return -1;
}

int fs_delete(char *name) {
    int i = find_file(name);
    if (i == -1)
        return -1;

    for (int j = 0; j < MAX_FD; ++j)
        if (fdt[j].used && fdt[j].dir_index == i)
            return -1;

    uint32_t b = directory[i].first_block;
    while (b != FAT_EOF) {
        uint32_t next = fat[b];
        fat[b] = 0;
        b = next;
    }
    memset(&directory[i], 0, sizeof(dir_entry_t));
    return 0;
}

int fs_open(char *name) {
    int i = find_file(name);
    if (i == -1)
        return -1;

    for (int fd = 0; fd < MAX_FD; ++fd) {
        if (!fdt[fd].used) {
            fdt[fd].used = 1;
            fdt[fd].dir_index = i;
            fdt[fd].offset = 0;
            return fd;
        }
    }
    return -1;
}

int fs_close(int fildes) {
    if (fildes < 0 || fildes >= MAX_FD || !fdt[fildes].used)
        return -1;
    fdt[fildes].used = 0;
    return 0;
}

int fs_get_filesize(int fildes) {
    if (fildes < 0 || fildes >= MAX_FD || !fdt[fildes].used)
        return -1;
    return directory[fdt[fildes].dir_index].size;
}

int fs_lseek(int fildes, off_t offset) {
    if (fildes < 0 || fildes >= MAX_FD || !fdt[fildes].used)
        return -1;
    if (offset > directory[fdt[fildes].dir_index].size)
        return -1;
    fdt[fildes].offset = offset;
    return 0;
}

int fs_truncate(int fildes, off_t length) {
    if (fildes < 0 || fildes >= MAX_FD || !fdt[fildes].used)
        return -1;

    dir_entry_t *file = &directory[fdt[fildes].dir_index];
    if (length > file->size)
        return -1;
    file->size = length;
    if (fdt[fildes].offset > length)
        fdt[fildes].offset = length;
    return 0;
}

int fs_read(int fildes, void *buf, size_t nbyte) {
    if (fildes < 0 || fildes >= MAX_FD || !fdt[fildes].used)
        return -1;

    file_descriptor_t *fd = &fdt[fildes];
    dir_entry_t *file = &directory[fd->dir_index];
    if (fd->offset >= file->size)
        return 0;

    if (fd->offset + nbyte > file->size)
        nbyte = file->size - fd->offset;

    size_t bytes_read = 0;
    uint32_t current_block = file->first_block;
    uint32_t offset = fd->offset;

    // Traverse to the correct block based on offset
    size_t skip = offset / BLOCK_SIZE;
    size_t block_offset = offset % BLOCK_SIZE;

    while (skip > 0 && current_block != FAT_EOF) {
        current_block = fat[current_block];
        skip--;
    }

    char block_data[BLOCK_SIZE];
    char *dst = (char *)buf;
    while (current_block != FAT_EOF && bytes_read < nbyte) {
        read_block(sb.data_start + current_block, block_data);
        size_t to_copy = BLOCK_SIZE - block_offset;
        if (to_copy > nbyte - bytes_read)
            to_copy = nbyte - bytes_read;
        memcpy(dst + bytes_read, block_data + block_offset, to_copy);
        bytes_read += to_copy;
        block_offset = 0;
        current_block = fat[current_block];
    }

    fd->offset += bytes_read;
    return bytes_read;
}

int fs_write(int fildes, void *buf, size_t nbyte) {
    if (fildes < 0 || fildes >= MAX_FD || !fdt[fildes].used)
        return -1;

    file_descriptor_t *fd = &fdt[fildes];
    dir_entry_t *file = &directory[fd->dir_index];

    size_t new_size = fd->offset + nbyte;
    if (new_size > DATA_BLOCKS * BLOCK_SIZE)
        return -1;

    // Extend file if necessary
    if (file->first_block == FAT_EOF)
        file->first_block = allocate_block();

    uint32_t current_block = file->first_block;
    size_t offset = fd->offset;
    size_t skip = offset / BLOCK_SIZE;
    size_t block_offset = offset % BLOCK_SIZE;

    while (skip > 0) {
        if (fat[current_block] == FAT_EOF) {
            fat[current_block] = allocate_block();
        }
        current_block = fat[current_block];
        skip--;
    }

    char block_data[BLOCK_SIZE];
    char *src = (char *)buf;
    size_t bytes_written = 0;

    while (bytes_written < nbyte) {
        read_block(sb.data_start + current_block, block_data);
        size_t to_copy = BLOCK_SIZE - block_offset;
        if (to_copy > nbyte - bytes_written)
            to_copy = nbyte - bytes_written;
        memcpy(block_data + block_offset, src + bytes_written, to_copy);
        write_block(sb.data_start + current_block, block_data);
        bytes_written += to_copy;
        block_offset = 0;
        if (bytes_written < nbyte) {
            if (fat[current_block] == FAT_EOF)
                fat[current_block] = allocate_block();
            current_block = fat[current_block];
        }
    }

    fd->offset += bytes_written;
    if (fd->offset > file->size)
        file->size = fd->offset;

    return bytes_written;
}

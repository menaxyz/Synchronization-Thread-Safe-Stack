#include <stdio.h>
#include <string.h>
#include "disk.h"


int make_fs(char *disk_name);
int mount_fs(char *disk_name);
int umount_fs(char *disk_name);
int fs_create(char *name);
int fs_delete(char *name);
int fs_open(char *name);
int fs_close(int fildes);
int fs_get_filesize(int fildes);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);

int main() {
    char disk[] = "testdisk";
    char filename[] = "myfile";
    char write_buf[] = "Hello, this is a test of your file system!";
    char read_buf[100] = {0};

    // create disk
    if (make_fs(disk) == 0) printf("Disk created.\n");

    // mounted disk
    if (mount_fs(disk) == 0) printf("Disk mounted.\n");

    // create file
    if (fs_create(filename) == 0) printf("File created.\n");

    // open file 
    int fd = fs_open(filename);
    if (fd >= 0) printf("File opened. FD = %d\n", fd);

    
    int written = fs_write(fd, write_buf, strlen(write_buf));
    printf("Wrote %d bytes.\n", written);

    fs_close(fd);
    fd = fs_open(filename);


    int read = fs_read(fd, read_buf, sizeof(read_buf) - 1);
    printf("Read %d bytes: %s\n", read, read_buf);

    fs_close(fd);
    umount_fs(disk);
    return 0;
}

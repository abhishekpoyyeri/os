/* src/fs/vfs.h */
#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include "myfs.h"

#define MAX_OPEN_FILES 32

typedef struct {
    uint32_t file_index;
    uint32_t position;
    uint8_t locked;
    uint8_t used;
} VFSFile;

void vfs_init(void);
int vfs_open(const char* name);
int vfs_close(int fd);
int vfs_read(int fd, uint8_t* buffer, uint32_t length);
int vfs_write(int fd, const uint8_t* buffer, uint32_t length);
int vfs_delete(const char* name);
int vfs_exists(const char* name);
int vfs_create(const char* name, uint8_t type);
uint32_t vfs_get_size(int fd);
int vfs_seek(int fd, uint32_t offset);
uint32_t vfs_tell(int fd);
int vfs_lock(int fd);
int vfs_unlock(int fd);
int vfs_list(void (*callback)(const char* name, uint32_t size, uint8_t type));

#endif

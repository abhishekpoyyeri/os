/* src/fs/vfs.c */
#include "vfs.h"
#include "../kernel/error.h"
#include "../kernel/klog.h"
#include <stddef.h>

static VFSFile open_files[MAX_OPEN_FILES];

static void* local_memset(void* ptr, int value, size_t num) {
    uint8_t* p = (uint8_t*)ptr;
    while (num--) *p++ = (uint8_t)value;
    return ptr;
}

void vfs_init(void) {
    local_memset(open_files, 0, sizeof(open_files));
    klog_info("VFS initialized");
}

int vfs_open(const char* name) {
    uint32_t file_idx;
    if (myfs_find(name, &file_idx) != KERR_OK) {
        klog_warn("VFS open failed: not found");
        return KERR_NOT_FOUND;
    }
    
    int fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i].used) {
            fd = i;
            break;
        }
    }
    if (fd < 0) return KERR_LIMIT_REACHED;
    
    open_files[fd].file_index = file_idx;
    open_files[fd].position = 0;
    open_files[fd].locked = 0;
    open_files[fd].used = 1;
    klog_info_u32("VFS open fd", fd);
    klog_info_u32("VFS open file index", file_idx);
    return fd;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return KERR_INVALID_ARGUMENT;
    open_files[fd].used = 0;
    return KERR_OK;
}

int vfs_read(int fd, uint8_t* buffer, uint32_t length) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return KERR_INVALID_ARGUMENT;
    VFSFile* vf = &open_files[fd];
    
    int ret = myfs_read(vf->file_index, vf->position, buffer, length);
    if (ret > 0) {
        vf->position += ret;
    }
    return ret;
}

int vfs_write(int fd, const uint8_t* buffer, uint32_t length) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return KERR_INVALID_ARGUMENT;
    VFSFile* vf = &open_files[fd];
    
    klog_info_u32("VFS write fd", fd);
    klog_info_u32("VFS write length", length);
    klog_info_u32("VFS write position", vf->position);
    int ret = myfs_write(vf->file_index, vf->position, buffer, length);
    if (ret == KERR_OK) {
        vf->position += length;
    }
    return ret;
}

int vfs_truncate(int fd, uint32_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return KERR_INVALID_ARGUMENT;
    klog_info_u32("VFS truncate fd", fd);
    klog_info_u32("VFS truncate size", size);
    int ret = myfs_truncate(open_files[fd].file_index, size);
    if (ret == KERR_OK && open_files[fd].position > size) {
        open_files[fd].position = size;
    }
    return ret;
}

int vfs_delete(const char* name) {
    /* Optionally check if it's open */
    uint32_t file_idx;
    if (myfs_find(name, &file_idx) == KERR_OK) {
        for (int i = 0; i < MAX_OPEN_FILES; i++) {
            if (open_files[i].used && open_files[i].file_index == file_idx) {
                return KERR_ACCESS_DENIED; /* File is open */
            }
        }
    }
    return myfs_delete(name);
}

int vfs_exists(const char* name) {
    uint32_t file_idx;
    return (myfs_find(name, &file_idx) == KERR_OK) ? 1 : 0;
}

int vfs_create(const char* name, uint8_t type) {
    klog_info("VFS create");
    return myfs_create(name, type);
}

uint32_t vfs_get_size(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return 0;
    FileEntry* ent = myfs_get_entry(open_files[fd].file_index);
    if (!ent) return 0;
    return ent->size;
}

int vfs_seek(int fd, uint32_t offset) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return KERR_INVALID_ARGUMENT;
    FileEntry* ent = myfs_get_entry(open_files[fd].file_index);
    if (!ent) return KERR_NOT_FOUND;
    if (offset > ent->size) offset = ent->size;
    open_files[fd].position = offset;
    return KERR_OK;
}

uint32_t vfs_tell(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return 0;
    return open_files[fd].position;
}

int vfs_lock(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return KERR_INVALID_ARGUMENT;
    if (open_files[fd].locked) return KERR_ACCESS_DENIED;
    open_files[fd].locked = 1;
    return KERR_OK;
}

int vfs_unlock(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used) return KERR_INVALID_ARGUMENT;
    open_files[fd].locked = 0;
    return KERR_OK;
}

int vfs_list(void (*callback)(const char* name, uint32_t size, uint8_t type)) {
    for (int i = 0; i < MYFS_MAX_FILES; i++) {
        FileEntry* ent = myfs_get_entry(i);
        if (ent) {
            callback(ent->name, ent->size, ent->type);
        }
    }
    return KERR_OK;
}

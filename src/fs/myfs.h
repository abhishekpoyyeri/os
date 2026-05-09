/* src/fs/myfs.h */
#ifndef MYFS_H
#define MYFS_H

#include <stdint.h>

#define MYFS_MAGIC 0x4D594653
#define MYFS_MAX_FILES 128
#define MYFS_MAX_FILE_SIZE 65536
#define MAX_PATH_LENGTH 64

#define FILE_TYPE_TEXT   1
#define FILE_TYPE_CONFIG 2
#define FILE_TYPE_BINARY 3
#define FILE_TYPE_ASSET  4

typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t file_table_start;
    uint32_t data_start;
    uint32_t flags;
} MyFSSuperblock;

typedef struct {
    char name[MAX_PATH_LENGTH];
    uint32_t size;
    uint32_t start_sector;
    uint32_t created_time;
    uint32_t modified_time;
    uint8_t type;
    uint8_t flags;
    uint8_t used;
} FileEntry;

void myfs_init(void);
int myfs_format(void);
int myfs_create(const char* name, uint8_t type);
int myfs_delete(const char* name);
int myfs_read(uint32_t file_index, uint32_t offset, uint8_t* buffer, uint32_t length);
int myfs_write(uint32_t file_index, uint32_t offset, const uint8_t* buffer, uint32_t length);
int myfs_find(const char* name, uint32_t* file_index);
FileEntry* myfs_get_entry(uint32_t file_index);

#endif

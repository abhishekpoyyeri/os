/* src/fs/myfs.c */
#include "myfs.h"
#include "../drivers/ata.h"
#include "../drivers/rtc.h"
#include "../kernel/error.h"
#include "../kernel/klog.h"
#include "../kernel/panic.h"
#include <stddef.h>

static MyFSSuperblock superblock;
static FileEntry file_table[MYFS_MAX_FILES];
static uint8_t myfs_mounted = 0;

#define MYFS_FILE_TABLE_SECTORS ((sizeof(file_table) + 511) / 512)
#define MYFS_FILE_SECTORS ((MYFS_MAX_FILE_SIZE + 511) / 512)
#define MYFS_DEFAULT_TOTAL_BLOCKS 32768

static void* local_memset(void* ptr, int value, size_t num) {
    uint8_t* p = (uint8_t*)ptr;
    while (num--) *p++ = (uint8_t)value;
    return ptr;
}

static void* local_memcpy(void* dst, const void* src, size_t num) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    while (num--) *d++ = *s++;
    return dst;
}

static int local_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static void local_strncpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    if (max == 0) return;
    for (; i < max - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static int local_memcmp(const void* a, const void* b, size_t num) {
    const uint8_t* pa = (const uint8_t*)a;
    const uint8_t* pb = (const uint8_t*)b;
    for (size_t i = 0; i < num; i++) {
        if (pa[i] != pb[i]) return (int)pa[i] - (int)pb[i];
    }
    return 0;
}

static uint32_t get_current_timestamp(void) {
    uint8_t s, m, h, d, mo;
    uint32_t y;
    rtc_get_time(&s, &m, &h, &d, &mo, &y);
    return (uint32_t)s + (m * 60) + (h * 3600); /* Simple representation */
}

int myfs_format(void) {
    klog_info("Formatting MyFS...");
    local_memset(&superblock, 0, sizeof(MyFSSuperblock));
    superblock.magic = MYFS_MAGIC;
    superblock.total_blocks = MYFS_DEFAULT_TOTAL_BLOCKS;
    superblock.file_table_start = 2;
    superblock.data_start = superblock.file_table_start + MYFS_FILE_TABLE_SECTORS;
    superblock.flags = 0;
    klog_info_u32("MyFS total blocks", superblock.total_blocks);
    klog_info_u32("MyFS file table sectors", MYFS_FILE_TABLE_SECTORS);
    klog_info_u32("MyFS data start sector", superblock.data_start);

    uint8_t sec_buf[512];
    local_memset(sec_buf, 0, 512);
    local_memcpy(sec_buf, &superblock, sizeof(MyFSSuperblock));
    
    if (ata_write_sector(1, sec_buf) != KERR_OK) {
        klog_error("Format failed: could not write superblock");
        return KERR_IO_ERROR;
    }

    local_memset(file_table, 0, sizeof(file_table));
    uint8_t* ft_ptr = (uint8_t*)file_table;
    for (uint32_t i = 0; i < MYFS_FILE_TABLE_SECTORS; i++) {
        local_memset(sec_buf, 0, 512);
        size_t chunk_size = (sizeof(file_table) - i * 512) > 512 ? 512 : (sizeof(file_table) - i * 512);
        if (chunk_size > 0) local_memcpy(sec_buf, ft_ptr + i * 512, chunk_size);
        if (ata_write_sector(2 + i, sec_buf) != KERR_OK) return KERR_IO_ERROR;
    }

    myfs_mounted = 1;
    klog_info("Format complete");
    return KERR_OK;
}

static int flush_file_table(void) {
    uint8_t sec_buf[512];
    uint8_t* ft_ptr = (uint8_t*)file_table;
    klog_info("MyFS commit: flushing file table");
    for (uint32_t i = 0; i < MYFS_FILE_TABLE_SECTORS; i++) {
        local_memset(sec_buf, 0, 512);
        size_t chunk_size = (sizeof(file_table) - i * 512) > 512 ? 512 : (sizeof(file_table) - i * 512);
        if (chunk_size > 0) local_memcpy(sec_buf, ft_ptr + i * 512, chunk_size);
        klog_info_u32("MyFS commit table sector", 2 + i);
        if (ata_write_sector(2 + i, sec_buf) != KERR_OK) return KERR_IO_ERROR;
    }
    return KERR_OK;
}

void myfs_init(void) {
    uint8_t sec_buf[512];
    if (ata_read_sector(1, sec_buf) != KERR_OK) {
        klog_warn("Disk read failed. Attempting format.");
        myfs_format();
        return;
    }

    local_memcpy(&superblock, sec_buf, sizeof(MyFSSuperblock));
    if (superblock.magic != MYFS_MAGIC ||
        superblock.file_table_start != 2 ||
        superblock.data_start < superblock.file_table_start + MYFS_FILE_TABLE_SECTORS) {
        klog_warn("Invalid magic. Formatting disk.");
        myfs_format();
        return;
    }
    klog_info_u32("MyFS mount total blocks", superblock.total_blocks);
    klog_info_u32("MyFS mount data start", superblock.data_start);

    /* Read File Table */
    uint8_t* ft_ptr = (uint8_t*)file_table;
    for (uint32_t i = 0; i < MYFS_FILE_TABLE_SECTORS; i++) {
        if (ata_read_sector(2 + i, sec_buf) != KERR_OK) {
            panic("Failed to read file table during mount");
        }
        size_t chunk_size = (sizeof(file_table) - i * 512) > 512 ? 512 : (sizeof(file_table) - i * 512);
        if (chunk_size > 0) local_memcpy(ft_ptr + i * 512, sec_buf, chunk_size);
    }

    /* Mount Consistency Checks */
    for (int i = 0; i < MYFS_MAX_FILES; i++) {
        if (file_table[i].used) {
            if (file_table[i].size > MYFS_MAX_FILE_SIZE) {
                klog_error("Corrupted file size detected");
                file_table[i].used = 0;
            } else if (file_table[i].start_sector < superblock.data_start ||
                       file_table[i].start_sector + MYFS_FILE_SECTORS > superblock.total_blocks) {
                klog_error("Corrupted file sector range detected");
                file_table[i].used = 0;
            }
        }
    }

    myfs_mounted = 1;
    klog_info("MyFS mounted successfully");
}

int myfs_find(const char* name, uint32_t* file_index) {
    if (!myfs_mounted) return KERR_IO_ERROR;
    for (int i = 0; i < MYFS_MAX_FILES; i++) {
        if (file_table[i].used && local_strcmp(file_table[i].name, name) == 0) {
            *file_index = i;
            return KERR_OK;
        }
    }
    return KERR_NOT_FOUND;
}

FileEntry* myfs_get_entry(uint32_t file_index) {
    if (file_index >= MYFS_MAX_FILES || !file_table[file_index].used) return NULL;
    return &file_table[file_index];
}

static uint8_t ranges_overlap(uint32_t a_start, uint32_t a_count, uint32_t b_start, uint32_t b_count) {
    uint32_t a_end = a_start + a_count;
    uint32_t b_end = b_start + b_count;
    return a_start < b_end && b_start < a_end;
}

static int find_free_extent(uint32_t sector_count, uint32_t* start_sector) {
    if (sector_count == 0 || sector_count > MYFS_FILE_SECTORS) return KERR_INVALID_ARGUMENT;
    for (uint32_t candidate = superblock.data_start;
         candidate + sector_count <= superblock.total_blocks;
         candidate += sector_count) {
        uint8_t used = 0;
        for (int i = 0; i < MYFS_MAX_FILES; i++) {
            if (!file_table[i].used) continue;
            if (ranges_overlap(candidate, sector_count, file_table[i].start_sector, MYFS_FILE_SECTORS)) {
                used = 1;
                break;
            }
        }
        if (!used) {
            *start_sector = candidate;
            return KERR_OK;
        }
    }
    return KERR_LIMIT_REACHED;
}

int myfs_create(const char* name, uint8_t type) {
    if (!myfs_mounted) return KERR_IO_ERROR;
    uint32_t idx;
    if (myfs_find(name, &idx) == KERR_OK) return KERR_ACCESS_DENIED; /* Already exists */

    int free_idx = -1;
    for (int i = 0; i < MYFS_MAX_FILES; i++) {
        if (!file_table[i].used) {
            free_idx = i;
            break;
        }
    }
    if (free_idx < 0) return KERR_LIMIT_REACHED;

    uint32_t next_sector;
    int alloc_status = find_free_extent(MYFS_FILE_SECTORS, &next_sector);
    if (alloc_status != KERR_OK) return alloc_status;

    FileEntry* ent = &file_table[free_idx];
    local_memset(ent, 0, sizeof(FileEntry));
    local_strncpy(ent->name, name, MAX_PATH_LENGTH);
    ent->size = 0;
    ent->start_sector = next_sector;
    ent->created_time = get_current_timestamp();
    ent->modified_time = ent->created_time;
    ent->type = type;
    ent->used = 1;

    klog_info("MyFS create file");
    klog_info_u32("MyFS create index", free_idx);
    klog_info_u32("MyFS create start sector", ent->start_sector);
    klog_info_u32("MyFS create reserved sectors", MYFS_FILE_SECTORS);

    return flush_file_table();
}

int myfs_delete(const char* name) {
    uint32_t idx;
    if (myfs_find(name, &idx) != KERR_OK) return KERR_NOT_FOUND;
    
    file_table[idx].used = 0;
    return flush_file_table();
}

int myfs_read(uint32_t file_index, uint32_t offset, uint8_t* buffer, uint32_t length) {
    FileEntry* ent = myfs_get_entry(file_index);
    if (!ent) return KERR_NOT_FOUND;
    if (offset >= ent->size) return 0;
    if (offset + length > ent->size) length = ent->size - offset;

    uint8_t sec_buf[512];
    uint32_t start_sec = ent->start_sector + (offset / 512);
    uint32_t sec_offset = offset % 512;
    uint32_t bytes_read = 0;

    while (bytes_read < length) {
        if (start_sec >= ent->start_sector + MYFS_FILE_SECTORS ||
            start_sec >= superblock.total_blocks) return KERR_IO_ERROR;
        klog_info_u32("MyFS read sector", start_sec);
        if (ata_read_sector(start_sec, sec_buf) != KERR_OK) return KERR_IO_ERROR;
        
        uint32_t to_copy = 512 - sec_offset;
        if (to_copy > length - bytes_read) to_copy = length - bytes_read;
        
        local_memcpy(buffer + bytes_read, sec_buf + sec_offset, to_copy);
        bytes_read += to_copy;
        sec_offset = 0;
        start_sec++;
    }
    return bytes_read;
}

int myfs_write(uint32_t file_index, uint32_t offset, const uint8_t* buffer, uint32_t length) {
    FileEntry* ent = myfs_get_entry(file_index);
    if (!ent) return KERR_NOT_FOUND;
    if (offset + length > MYFS_MAX_FILE_SIZE) return KERR_LIMIT_REACHED;
    if (length == 0) return KERR_OK;

    uint8_t sec_buf[512];
    uint8_t verify_buf[512];
    uint32_t start_sec = ent->start_sector + (offset / 512);
    uint32_t sec_offset = offset % 512;
    uint32_t bytes_written = 0;

    klog_info("MyFS write begin");
    klog_info_u32("MyFS write file index", file_index);
    klog_info_u32("MyFS write offset", offset);
    klog_info_u32("MyFS write length", length);

    /* Safe write: Write sectors first */
    while (bytes_written < length) {
        if (start_sec >= ent->start_sector + MYFS_FILE_SECTORS ||
            start_sec >= superblock.total_blocks) return KERR_IO_ERROR;
        if (sec_offset != 0 || (length - bytes_written < 512)) {
            if (ata_read_sector(start_sec, sec_buf) != KERR_OK) return KERR_IO_ERROR;
        }
        
        uint32_t to_copy = 512 - sec_offset;
        if (to_copy > length - bytes_written) to_copy = length - bytes_written;
        
        local_memcpy(sec_buf + sec_offset, buffer + bytes_written, to_copy);
        klog_info_u32("MyFS write sector", start_sec);
        klog_info_u32("MyFS write sector offset", sec_offset);
        klog_info_u32("MyFS write bytes in sector", to_copy);
        if (ata_write_sector(start_sec, sec_buf) != KERR_OK) return KERR_IO_ERROR;
        if (ata_read_sector(start_sec, verify_buf) != KERR_OK) return KERR_IO_ERROR;
        if (local_memcmp(sec_buf, verify_buf, 512) != 0) {
            klog_error_u32("MyFS writeback verify failed sector", start_sec);
            return KERR_IO_ERROR;
        }
        
        bytes_written += to_copy;
        sec_offset = 0;
        start_sec++;
    }

    /* Then update metadata and flush */
    if (offset + length > ent->size) {
        ent->size = offset + length;
    }
    ent->modified_time = get_current_timestamp();
    return flush_file_table();
}

int myfs_truncate(uint32_t file_index, uint32_t size) {
    FileEntry* ent = myfs_get_entry(file_index);
    if (!ent) return KERR_NOT_FOUND;
    if (size > MYFS_MAX_FILE_SIZE) return KERR_LIMIT_REACHED;
    ent->size = size;
    ent->modified_time = get_current_timestamp();
    klog_info("MyFS truncate file");
    klog_info_u32("MyFS truncate index", file_index);
    klog_info_u32("MyFS truncate size", size);
    return flush_file_table();
}

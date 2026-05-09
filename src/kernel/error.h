/* src/kernel/error.h */
#ifndef ERROR_H
#define ERROR_H

typedef enum {
    KERR_OK = 0,
    KERR_DISK_TIMEOUT = -1,
    KERR_FS_CORRUPT = -2,
    KERR_INVALID_PATH = -3,
    KERR_OUT_OF_MEMORY = -4,
    KERR_NOT_FOUND = -5,
    KERR_LIMIT_REACHED = -6,
    KERR_IO_ERROR = -7,
    KERR_INVALID_ARGUMENT = -8,
    KERR_ACCESS_DENIED = -9
} kerror_t;

#endif

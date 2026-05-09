/* src/kernel/klog.h */
#ifndef KLOG_H
#define KLOG_H

void klog_init(void);
void klog_info(const char* msg);
void klog_warn(const char* msg);
void klog_error(const char* msg);
void klog_info_u32(const char* label, unsigned int value);
void klog_info_hex(const char* label, unsigned int value);
void klog_warn_u32(const char* label, unsigned int value);
void klog_error_u32(const char* label, unsigned int value);

#endif

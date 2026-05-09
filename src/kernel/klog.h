/* src/kernel/klog.h */
#ifndef KLOG_H
#define KLOG_H

void klog_init(void);
void klog_info(const char* msg);
void klog_warn(const char* msg);
void klog_error(const char* msg);

#endif

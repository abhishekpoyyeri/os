/* src/kernel/panic.h */
#ifndef PANIC_H
#define PANIC_H

void panic(const char* message);

#define ASSERT(condition, message) if (!(condition)) { panic(message); }

#endif

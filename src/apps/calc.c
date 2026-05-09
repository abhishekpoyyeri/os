/* src/apps/calc.c */
#include <stdint.h>

/* Simple calculator functions */

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int multiply(int a, int b) {
    return a * b;
}

int divide(int a, int b) {
    if (b == 0) return 0; // Error handling is basic
    return a / b;
}

/* In a real OS, we would have a shell that takes input.
   For now, we'll provide the functions that the shell can call. */

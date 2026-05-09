; boot.s
; Multiboot header and entry point

; Constants for Multiboot header
MB_MAGIC equ 0x1BADB002
MB_FLAGS equ (1 << 0) | (1 << 1)
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
    align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

section .bss
    align 16
stack_bottom:
    resb 16384 ; 16 KiB stack
stack_top:

section .text
global _start:function (_start.end - _start)
_start:
    ; Set up the stack
    mov esp, stack_top

    ; Call the global constructors (if any)
    ; extern _init
    ; call _init

    ; Call the kernel main function
    extern kernel_main
    call kernel_main

    ; If kernel returns, keep CPU alive with interrupts enabled
    sti
.hang:
    hlt
    jmp .hang
_start.end:

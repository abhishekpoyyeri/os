; src/drivers/interrupt.s
[EXTERN irq_handler]

%macro IRQ 2
  [GLOBAL irq%1]
  irq%1:
    cli
    push byte 0
    push byte %2
    jmp irq_common_stub
%endmacro

IRQ 0, 32   ; Timer (PIT)
IRQ 1, 33   ; Keyboard
IRQ 12, 44  ; PS/2 Mouse

irq_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp    ; Push pointer to registers_t
    call irq_handler
    add esp, 4  ; Pop pointer

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    sti
    iret

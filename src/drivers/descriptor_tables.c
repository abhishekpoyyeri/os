/* src/drivers/descriptor_tables.c */
#include "descriptor_tables.h"
#include "io.h"

extern void gdt_flush(uint32_t);
extern void idt_flush(uint32_t);

static void init_gdt();
static void gdt_set_gate(int32_t, uint32_t, uint32_t, uint8_t, uint8_t);
static void init_idt();
static void idt_set_gate(uint8_t, uint32_t, uint16_t, uint8_t);

gdt_entry_t gdt_entries[5];
gdt_ptr_t   gdt_ptr;
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

void init_descriptor_tables() {
    init_gdt();
    init_idt();
}

static void init_gdt() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

    gdt_flush((uint32_t)&gdt_ptr);
}

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

/* External IRQ stubs from interrupt.s */
extern void irq0();
extern void irq1();
extern void irq12();

typedef struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

void irq_handler(registers_t *regs) {
    /* Send EOI to slave PIC if needed */
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);

    /* Dispatch to specific handlers */
    switch (regs->int_no) {
        case 32: {
            /* IRQ0: Timer */
            extern void timer_handler(void);
            timer_handler();
            break;
        }
        case 33: {
            /* IRQ1: Keyboard */
            extern void keyboard_handler();
            keyboard_handler();
            break;
        }
        case 44: {
            /* IRQ12: Mouse */
            extern void mouse_handler(void);
            mouse_handler();
            break;
        }
    }
}

static void init_idt() {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0x08, 0x8E);
    }

    // PIC Remapping: IRQs 0-7 -> INT 32-39, IRQs 8-15 -> INT 40-47
    outb(0x20, 0x11);  // ICW1: init master
    outb(0xA0, 0x11);  // ICW1: init slave
    outb(0x21, 0x20);  // ICW2: master offset 32
    outb(0xA1, 0x28);  // ICW2: slave offset 40
    outb(0x21, 0x04);  // ICW3: slave on IRQ2
    outb(0xA1, 0x02);  // ICW3: cascade identity
    outb(0x21, 0x01);  // ICW4: 8086 mode
    outb(0xA1, 0x01);  // ICW4: 8086 mode
    outb(0x21, 0x0);   // Master: unmask all
    outb(0xA1, 0x0);   // Slave: unmask all

    /* Set IDT gates for our IRQ handlers */
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);  // Timer
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);  // Keyboard
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);  // Mouse

    idt_flush((uint32_t)&idt_ptr);
    asm volatile("sti");
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}

# Makefile for OS

AS = nasm
CC = gcc
LD = ld

ASFLAGS = -felf32
CFLAGS = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

SRCDIR = src
OBJDIR = obj

OBJS = $(OBJDIR)/boot.o \
       $(OBJDIR)/kernel.o \
       $(OBJDIR)/shell.o \
       $(OBJDIR)/clock.o \
       $(OBJDIR)/calc.o \
       $(OBJDIR)/gdt_idt.o \
       $(OBJDIR)/descriptor_tables.o \
       $(OBJDIR)/interrupt.o \
       $(OBJDIR)/keyboard.o \
       $(OBJDIR)/timer.o \
       $(OBJDIR)/mouse.o \
       $(OBJDIR)/vga.o \
       $(OBJDIR)/pmm.o \
       $(OBJDIR)/kheap.o \
       $(OBJDIR)/font.o \
       $(OBJDIR)/desktop.o

all: myos.bin

# ===== Boot & Kernel =====

$(OBJDIR)/boot.o: $(SRCDIR)/boot.s
	@mkdir -p $(OBJDIR)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJDIR)/kernel.o: $(SRCDIR)/kernel.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/shell.o: $(SRCDIR)/shell.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# ===== Apps =====

$(OBJDIR)/clock.o: $(SRCDIR)/apps/clock.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/calc.o: $(SRCDIR)/apps/calc.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# ===== Drivers =====

$(OBJDIR)/gdt_idt.o: $(SRCDIR)/drivers/gdt_idt.s
	@mkdir -p $(OBJDIR)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJDIR)/descriptor_tables.o: $(SRCDIR)/drivers/descriptor_tables.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/interrupt.o: $(SRCDIR)/drivers/interrupt.s
	@mkdir -p $(OBJDIR)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJDIR)/keyboard.o: $(SRCDIR)/drivers/keyboard.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/timer.o: $(SRCDIR)/drivers/timer.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/mouse.o: $(SRCDIR)/drivers/mouse.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/vga.o: $(SRCDIR)/drivers/vga.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# ===== Memory Management =====

$(OBJDIR)/pmm.o: $(SRCDIR)/mm/pmm.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/kheap.o: $(SRCDIR)/mm/kheap.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# ===== GUI =====

$(OBJDIR)/font.o: $(SRCDIR)/gui/font.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/desktop.o: $(SRCDIR)/gui/desktop.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# ===== Link =====

myos.bin: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) myos.bin

run: myos.bin
	qemu-system-i386 -kernel myos.bin

run-gui: myos.bin
	qemu-system-i386 -kernel myos.bin -m 32M

.PHONY: all clean run run-gui



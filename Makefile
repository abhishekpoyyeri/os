# Makefile for OS

AS = nasm
CC = gcc
LD = ld

ASFLAGS = -felf32
CFLAGS = -m32 -std=gnu99 -ffreestanding -fno-stack-protector -fno-pic -fno-pie -fno-asynchronous-unwind-tables -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld
DISK = myos.img

SRCDIR = src
OBJDIR = obj

OBJS = $(OBJDIR)/boot.o \
       $(OBJDIR)/kernel.o \
       $(OBJDIR)/shell.o \
       $(OBJDIR)/clock.o \
       $(OBJDIR)/calc.o \
       $(OBJDIR)/files_app.o \
       $(OBJDIR)/textedit_app.o \
       $(OBJDIR)/gdt_idt.o \
       $(OBJDIR)/descriptor_tables.o \
       $(OBJDIR)/interrupt.o \
       $(OBJDIR)/keyboard.o \
       $(OBJDIR)/timer.o \
       $(OBJDIR)/mouse.o \
       $(OBJDIR)/vga.o \
       $(OBJDIR)/ata.o \
       $(OBJDIR)/rtc.o \
       $(OBJDIR)/pmm.o \
       $(OBJDIR)/kheap.o \
       $(OBJDIR)/klog.o \
       $(OBJDIR)/panic.o \
       $(OBJDIR)/settings.o \
       $(OBJDIR)/vfs.o \
       $(OBJDIR)/myfs.o \
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

$(OBJDIR)/klog.o: $(SRCDIR)/kernel/klog.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/panic.o: $(SRCDIR)/kernel/panic.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/settings.o: $(SRCDIR)/settings.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# ===== Apps =====

$(OBJDIR)/clock.o: $(SRCDIR)/apps/clock.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/calc.o: $(SRCDIR)/apps/calc.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/files_app.o: $(SRCDIR)/apps/files_app.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/textedit_app.o: $(SRCDIR)/apps/textedit_app.c
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

$(OBJDIR)/ata.o: $(SRCDIR)/drivers/ata.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/rtc.o: $(SRCDIR)/drivers/rtc.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# ===== Memory Management =====

$(OBJDIR)/pmm.o: $(SRCDIR)/mm/pmm.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/kheap.o: $(SRCDIR)/mm/kheap.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# ===== Filesystem =====

$(OBJDIR)/vfs.o: $(SRCDIR)/fs/vfs.c
	@mkdir -p $(OBJDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/myfs.o: $(SRCDIR)/fs/myfs.c
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

$(DISK):
	dd if=/dev/zero of=$@ bs=1M count=16

run: myos.bin $(DISK)
	qemu-system-i386 -kernel myos.bin -drive file=$(DISK),format=raw,if=ide

run-gui: myos.bin $(DISK)
	qemu-system-i386 -kernel myos.bin -m 32M -drive file=$(DISK),format=raw,if=ide

.PHONY: all clean run run-gui



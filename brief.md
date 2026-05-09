# MyOS Project Brief

**Project Name:** MyOS
**Goal:** A custom 32-bit x86 bare-metal operating system featuring a text-mode shell and a fully functional VGA graphical user interface (Aurora Desktop).
**Tech Stack:** C, x86 Assembly (NASM), GCC (32-bit/i686-elf), Make, QEMU (Emulator).

## Architecture Map
- **Bootloader:** Multiboot compliant (`boot.s`).
- **Core Kernel:** GDT, IDT, PIC remapping, IRQ dispatch (`kernel.c`).
- **Memory Management:** Physical bitmap allocator (128MB), Kernel Heap (first-fit, 256KB).
- **Hardware Drivers:** PS/2 Keyboard & Mouse, PIT (Timer), VGA Mode 13h (320x200x256 colors with double buffering).
- **Shell:** Text-mode command line interface (`shell.c`).
- **GUI Desktop:** Aurora style window manager, start menu, desktop icons, taskbar, draggable windows.
- **Applications:** Clock, Calculator, System Monitor, Notes, Tasks.

## Dependency Map
- **Build Tools:** `nasm`, `gcc-multilib` (or `i686-elf-gcc`), `build-essential`.
- **Emulation:** `qemu-system-x86`.
- **Linker:** Custom linker script (`linker.ld`).

## Critical Modules
- `src/boot.s`: Kernel entry point and multiboot header.
- `src/kernel.c`: Kernel initialization and primary dispatch.
- `src/shell.c`: CLI implementation for text mode.
- `src/mm/`: Physical and heap memory managers.
- `src/drivers/`: Low-level hardware communication (VGA, input devices, timer).
- `src/gui/`: Window management, rendering, and UI events.
- `src/apps/`: Individual GUI applications.

## Pending Features (Roadmap)
- **Phase 5 (Networking):** PCI enumeration, Network stack (Ethernet/IP/TCP/UDP/DHCP), WiFi (Intel/Realtek) drivers, Bluetooth HCI stack.

*Status: v0.4 - Aurora desktop redesign active. Boot with `make run-gui`.*

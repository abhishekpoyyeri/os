
<img width="641" height="400" alt="image" src="https://github.com/user-attachments/assets/a6ea8bec-b840-4767-829e-c693e49772f5" />


# Detailed Guide: How to Run OS on Windows

Since you are building an operating system, you cannot use your standard Windows compiler. You need a **Cross-Compiler** that targets "bare metal" hardware.

## Option 1: Using WSL (Recommended)
This is the easiest and most reliable method for Windows users.

### 1. Install WSL
Open PowerShell as Administrator and run:
```powershell
wsl --install
```
Restart your computer after this.

### 2. Install Tools in WSL
Open the "Ubuntu" app and run:
```bash
sudo apt update
sudo apt install nasm gcc-multilib build-essential qemu-system-x86
```

I have updated the `Makefile` to use `gcc -m32` instead of `i686-elf-gcc`. This version is much easier to install on WSL/Ubuntu.

### 3. Build & Run
Navigate to your project folder (WSL maps your C: drive to `/mnt/c/`):
```bash
cd /mnt/c/Users/ACER/OneDrive/Desktop/myos
make
make run
```

---

## Option 2: Pure Windows (Manual)
If you don't want to use WSL, follow these steps:

### 1. Install NASM
- Download from [nasm.us](https://www.nasm.us/).
- Add it to your System PATH.

### 2. Install i686-elf-gcc
- Standard GCC for Windows (MinGW) won't work easily because it produces Windows-specific code.
- Download a prebuilt **i686-elf-gcc** toolchain (search for "i686-elf-gcc windows prebuilt").
- Extract it and add the `bin` folder to your System PATH.

### 3. Install QEMU
- Download the Windows installer from [qemu.org](https://www.qemu.org/download/#windows).
- Add it to your System PATH.

### 4. Compile & Launch
Open your terminal (CMD or PowerShell) in the `myos` folder:
```powershell
make
make run
```

---

## What happens when you run `make run`?
1.  **NASM** compiles `boot.s` into an ELF32 object file.
2.  **GCC** compiles all your C files into ELF32 object files.
3.  **The Linker** uses `linker.ld` to stitch them together into a single file called `myos.bin`. It ensures the "Multiboot Header" is at the very beginning so the computer can recognize it.
4.  **QEMU** starts a virtual computer. The `-kernel myos.bin` flag tells QEMU to act like a bootloader, load your binary into memory, and jump to the `_start` instruction in `boot.s`.

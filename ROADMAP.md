# OS Roadmap

## Completed Features

### Core Kernel
- Multiboot-compliant bootloader
- GDT, IDT, PIC remapping, IRQ dispatch
- Keyboard, Mouse (PS/2), Timer (PIT) drivers
- VGA Mode 13h (320x200x256) with double buffering
- Physical Memory Manager (bitmap allocator, 128MB support)
- Kernel Heap (first-fit linked list, 256KB)

### Shell (Text Mode)
- `help`, `clock`, `calc A+B`, `mem`, `status`, `apps`, `gui`, `clear`
- `uptime` - system uptime in minutes/seconds
- `ver` / `about` - OS version info
- `echo <text>` - echo text back
- Backspace editing in the command prompt

### GUI Desktop (Aurora Style)
- Window system: create, close, focus, and drag windows
- Start Menu launcher with 7 apps
- Clickable desktop icons and taskbar app buttons
- Live wallpaper widgets for time/date and RAM
- Mouse cursor with shadow

### GUI Apps
- **Clock** - live RTC time and date
- **Calculator** - 4-function clickable calculator
- **System Monitor** - live memory bars, KB used/free, uptime counter
- **Notes** - type text, word wrap, clear action, blinking cursor, char count
- **Tasks** - clickable local checklist
- **Help** - GUI shortcuts and usage tips
- **About** - OS version info

### UX
- ESC key exits GUI mode back to text shell
- Keyboard input forwarded to Notes when focused
- GUI shortcuts: `c`, `t`, `n`, `m`, `h`

---

## Future Phases

### Phase 5: Networking (WiFi & Bluetooth)
- PCI bus enumeration
- Network stack (Ethernet, IP, UDP/TCP, DHCP)
- Hardware drivers (Intel/Realtek WiFi)
- Wireless security (WPA2/WPA3)
- Bluetooth HCI stack

**Current Status**: v0.4 - Aurora desktop redesign with launcher, icons, 7 apps, and enhanced shell. Run with `make run-gui`.

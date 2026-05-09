/* kernel.c */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Check if the compiler thinks we are targeting the wrong operating system. */
/* Bypassing cross-compiler check for WSL compatibility with gcc -m32 */

/* Hardware text mode color constants. */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) 0xB8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll(void) {
    /* Scroll all rows up by one */
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[(y-1) * VGA_WIDTH + x] = terminal_buffer[y * VGA_WIDTH + x];
        }
    }
    /* Clear the last row */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[(VGA_HEIGHT-1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
}

void terminal_putchar(char c) {
    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
        } else if (terminal_row > 0) {
            terminal_row--;
            terminal_column = VGA_WIDTH - 1;
        }
        terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
        return;
    }

    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = VGA_HEIGHT - 1;
            terminal_scroll();
        }
        return;
    }
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = VGA_HEIGHT - 1;
            terminal_scroll();
        }
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

void itoa(int n, char s[]) {
    int i, sign;
    if ((sign = n) < 0) n = -n;
    i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0) s[i++] = '-';
    s[i] = '\0';
    
    /* Reverse the string */
    for (int j = 0, k = i-1; j < k; j++, k--) {
        char temp = s[j];
        s[j] = s[k];
        s[k] = temp;
    }
}

/* External functions from apps */
extern void read_rtc(uint8_t *second, uint8_t *minute, uint8_t *hour, uint8_t *day, uint8_t *month, uint32_t *year);

void display_time() {
    uint8_t s, m, h, d, mo;
    uint32_t y;
    read_rtc(&s, &m, &h, &d, &mo, &y);

    char buf[16];
    terminal_writestring("Current Time: ");
    
    itoa(h, buf);
    if (h < 10) terminal_putchar('0');
    terminal_writestring(buf);
    terminal_putchar(':');
    
    itoa(m, buf);
    if (m < 10) terminal_putchar('0');
    terminal_writestring(buf);
    terminal_putchar(':');
    
    itoa(s, buf);
    if (s < 10) terminal_putchar('0');
    terminal_writestring(buf);
    terminal_writestring("\n");
}

/* External subsystem initialization */
extern void init_descriptor_tables();
extern void shell_init();
extern void timer_init(uint32_t frequency);

/* Memory management */
extern void pmm_init(uint32_t mem_size_kb);
extern void kheap_init(void);

/* GUI desktop */
extern void desktop_init(void);
extern void desktop_update(void);
extern uint8_t desktop_is_running(void);

/* New core services */
extern void klog_init(void);
extern void ata_init(void);
extern void myfs_init(void);
extern void vfs_init(void);
extern void settings_init(void);
extern void settings_load(void);
extern void rtc_init(void);

/* Global flag: 0 = text shell, 1 = GUI mode */
volatile uint8_t gui_mode = 0;

void kernel_main(void) {
    /* Initialize GDT and IDT */
    init_descriptor_tables();

    /* Initialize terminal interface */
    terminal_initialize();
    timer_init(100);

    /* Initialize memory management (assume 32 MB RAM) */
    pmm_init(32 * 1024);  /* 32 MB in KB */
    kheap_init();

    /* Initialize storage and persistence */
    klog_init();
    ata_init();
    myfs_init();
    vfs_init();
    settings_init();
    settings_load();
    rtc_init();

    terminal_writestring("MyOS v0.4\n");
    terminal_writestring("----------\n");
    
    display_time();

    terminal_writestring("\nMemory: 32 MB initialized\n");
    terminal_writestring("Type 'gui' for the Aurora desktop\n");
    terminal_writestring("Type 'help' for commands and features\n");
    
    shell_init();

    /* Keep the kernel alive — halt the CPU but wake on interrupts.
     * Without this, kernel_main() returns to boot.s which does cli;hlt
     * and kills all interrupts including the keyboard. */
    for (;;) {
        if (gui_mode) {
            desktop_update();
        } else {
            asm volatile("hlt");
        }
    }
}

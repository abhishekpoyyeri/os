/* src/shell.c */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern void display_time();
extern size_t strlen(const char* str);
extern int add(int a, int b);
extern int subtract(int a, int b);
extern int multiply(int a, int b);
extern int divide(int a, int b);
extern void itoa(int n, char s[]);

/* GUI mode flag (defined in kernel.c) */
extern volatile uint8_t gui_mode;
extern void desktop_init(void);

/* Memory info */
extern uint32_t pmm_get_total_pages(void);
extern uint32_t pmm_get_free_pages(void);
extern uint32_t pmm_get_used_pages(void);

char command_buffer[256];
int buffer_index = 0;

void shell_init() {
    terminal_writestring("\nOS Shell\n");
    terminal_writestring("> ");
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static uint8_t starts_with(const char* text, const char* prefix) {
    while (*prefix) {
        if (*text++ != *prefix++) return 0;
    }
    return 1;
}

static char* skip_spaces(char* text) {
    while (*text == ' ') text++;
    return text;
}

static uint8_t parse_int(char** text, int* out) {
    char* p = skip_spaces(*text);
    int sign = 1;
    int value = 0;
    uint8_t found = 0;

    if (*p == '-') {
        sign = -1;
        p++;
    }

    while (*p >= '0' && *p <= '9') {
        value = value * 10 + (*p - '0');
        found = 1;
        p++;
    }

    if (!found) return 0;
    *out = value * sign;
    *text = p;
    return 1;
}

static void print_memory_status(void) {
    char buf[16];
    terminal_writestring("Memory Status:\n");

    terminal_writestring("  Total pages: ");
    itoa(pmm_get_total_pages(), buf);
    terminal_writestring(buf);
    terminal_writestring("\n");

    terminal_writestring("  Used pages:  ");
    itoa(pmm_get_used_pages(), buf);
    terminal_writestring(buf);
    terminal_writestring("\n");

    terminal_writestring("  Free pages:  ");
    itoa(pmm_get_free_pages(), buf);
    terminal_writestring(buf);
    terminal_writestring("\n");

    terminal_writestring("  Free memory: ");
    itoa(pmm_get_free_pages() * 4, buf);
    terminal_writestring(buf);
    terminal_writestring(" KB\n");
}

static void print_uptime(void) {
    extern uint32_t timer_get_ticks(void);
    uint32_t ticks = timer_get_ticks();
    uint32_t secs = ticks / 100;
    uint32_t mins = secs / 60;
    char buf[16];
    terminal_writestring("Uptime: ");
    itoa(mins, buf);
    terminal_writestring(buf);
    terminal_writestring("m ");
    itoa(secs % 60, buf);
    terminal_writestring(buf);
    terminal_writestring("s\n");
}

static void run_calc_expression(char* expression) {
    int left, right, result;
    char op;
    char* p = expression;

    if (!parse_int(&p, &left)) {
        terminal_writestring("Usage: calc 12 + 5\n");
        return;
    }

    p = skip_spaces(p);
    op = *p;
    if (op != '+' && op != '-' && op != '*' && op != '/') {
        terminal_writestring("Calculator supports + - * /\n");
        return;
    }
    p++;

    if (!parse_int(&p, &right)) {
        terminal_writestring("Usage: calc 12 + 5\n");
        return;
    }

    if (op == '+') result = add(left, right);
    else if (op == '-') result = subtract(left, right);
    else if (op == '*') result = multiply(left, right);
    else {
        if (right == 0) {
            terminal_writestring("Cannot divide by zero.\n");
            return;
        }
        result = divide(left, right);
    }

    char buf[16];
    terminal_writestring("Result: ");
    itoa(result, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
}

void execute_command(char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        terminal_writestring("\nAvailable commands:\n");
        terminal_writestring("  help       - Show this help\n");
        terminal_writestring("  clock      - Display current time\n");
        terminal_writestring("  calc A+B   - Calculate + - * /\n");
        terminal_writestring("  mem        - Show memory info\n");
        terminal_writestring("  status     - Memory + uptime summary\n");
        terminal_writestring("  uptime     - Show system uptime\n");
        terminal_writestring("  apps       - List desktop apps\n");
        terminal_writestring("  ver/about  - OS version info\n");
        terminal_writestring("  echo text  - Echo text back\n");
        terminal_writestring("  gui        - Launch Aurora desktop\n");
        terminal_writestring("  clear      - Clear screen\n");
    } else if (strcmp(cmd, "clock") == 0) {
        terminal_writestring("\n");
        display_time();
    } else if (strcmp(cmd, "calc") == 0) {
        terminal_writestring("\nUsage: calc 12 + 5\n");
        terminal_writestring("Operators: + - * /\n");
    } else if (starts_with(cmd, "calc ")) {
        terminal_writestring("\n");
        run_calc_expression(cmd + 5);
    } else if (strcmp(cmd, "mem") == 0) {
        terminal_writestring("\n");
        print_memory_status();
    } else if (strcmp(cmd, "status") == 0) {
        terminal_writestring("\nMyOS status\n");
        terminal_writestring("-----------\n");
        print_uptime();
        print_memory_status();
    } else if (strcmp(cmd, "uptime") == 0) {
        terminal_writestring("\n");
        print_uptime();
    } else if (strcmp(cmd, "apps") == 0) {
        terminal_writestring("\nDesktop apps:\n");
        terminal_writestring("  Clock, Calculator, System, Notes\n");
        terminal_writestring("  Tasks, Help, About\n");
        terminal_writestring("GUI shortcuts: c t n m h\n");
    } else if (strcmp(cmd, "ver") == 0 || strcmp(cmd, "about") == 0) {
        terminal_writestring("\nMyOS v0.4\n");
        terminal_writestring("32-bit x86 | VGA 320x200 | 32MB RAM\n");
        terminal_writestring("Aurora desktop with launcher, tasks, notes\n");
    } else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ') {
        terminal_writestring("\n");
        terminal_writestring(cmd + 5);
        terminal_writestring("\n");
    } else if (strcmp(cmd, "gui") == 0 || strcmp(cmd, "desktop") == 0) {
        terminal_writestring("\nLaunching GUI desktop...\n");
        desktop_init();
        gui_mode = 1;
        return;  /* Don't print prompt — we're in GUI mode now */
    } else if (strcmp(cmd, "clear") == 0) {
        extern void terminal_initialize();
        terminal_initialize();
    } else if (strlen(cmd) > 0) {
        terminal_writestring("\nUnknown command: ");
        terminal_writestring(cmd);
        terminal_writestring("\n");
    }
    terminal_writestring("> ");
}

void shell_input(char c) {
    /* Ignore keyboard input in GUI mode */
    if (gui_mode) return;
    
    if (c == '\n') {
        command_buffer[buffer_index] = '\0';
        execute_command(command_buffer);
        buffer_index = 0;
    } else if (c == '\b') {
        if (buffer_index > 0) {
            buffer_index--;
        }
    } else {
        if (buffer_index < 255) {
            command_buffer[buffer_index++] = c;
        }
    }
}

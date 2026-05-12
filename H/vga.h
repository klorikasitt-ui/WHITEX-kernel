#include <stdint.h>
#include <stddef.h>

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_BUFFER = (uint16_t*) 0xB8000;

enum vga_color {
   BLACK = 0,
   BLUE = 1,
   GREEN = 2,
   CYAN = 3,
   RED = 4,
   MAGENTA = 5,
   BROWN = 6,
   GREY = 7,
   WHITE = 8,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void scroll() {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t src_index = (y + 1) * VGA_WIDTH + x;
            const size_t dest_index = y * VGA_WIDTH + x;
            VGA_BUFFER[dest_index] = VGA_BUFFER[src_index];
        }
    }
    
    const size_t last_row_start = (VGA_HEIGHT - 1) * VGA_WIDTH;
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        VGA_BUFFER[last_row_start + x] = vga_entry(' ', terminal_color);
    }
    
    terminal_row = VGA_HEIGHT - 1;
}

void putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else {
        const size_t index = terminal_row * VGA_WIDTH + terminal_column;
        VGA_BUFFER[index] = vga_entry(c, terminal_color);
        
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            terminal_row++;
        }
    }

    if (terminal_row >= VGA_HEIGHT) {
        scroll();
    }
}

void print(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        putchar(data[i]);
    }
}

void init(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(WHITE, BLACK);
    
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            VGA_BUFFER[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_backspace() {
    if (terminal_column > 0) {
        terminal_column--;
    } else if (terminal_row > 0) {
        terminal_row--;
        terminal_column = VGA_WIDTH - 1;
    }
    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    VGA_BUFFER[index] = vga_entry(' ', terminal_color);
}

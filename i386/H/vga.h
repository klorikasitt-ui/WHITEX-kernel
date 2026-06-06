
#ifndef NOUVEAU_VGA_H
#define NOUVEAU_VGA_H

#include <stdint.h>
#include <stddef.h>

#define NV_REG_BASE 0xFD000000
#define NV_VGA_RAM_BASE 0x000A0000
#define NV_FB_BASE 0xE0000000

#define NV_PMC_INTR_0 0x00000100
#define NV_PMC_INTR_EN_0 0x00000140
#define NV_PFIFO_CACHE1_PUSH0 0x00003200
#define NV_PFIFO_CACHE1_PULL0 0x00003240
#define NV_PFIFO_CACHE1_GET 0x00003270
#define NV_PFIFO_CACHE1_PUT 0x00003274
#define NV_PGRAPH_STATUS 0x00400700
#define NV_PGRAPH_TRIGGER 0x00400800
#define NV_PCRTC_CONFIG 0x00600800
#define NV_PCRTC_RASTER 0x00600808
#define NV_PCRTC_CURSOR_CONFIG 0x00600300
#define NV_PCRTC_CURSOR_POS 0x00600308

#define NV_IO_CRTC_INDEX 0x006013D4
#define NV_IO_CRTC_DATA 0x006013D5

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
   GREY = 8,
   WHITE = 15,
};

struct nouveau_device {
    uint32_t chipset;
    uintptr_t mmio_addr;
    uintptr_t fb_addr;
    uint32_t ram_size;
    uint32_t crystal_khz;
    uint32_t pfifo_channels;
    uint8_t is_active;
};

struct nouveau_channel {
    uint32_t id;
    uint32_t* push_buffer;
    uint32_t put;
    uint32_t get;
    uint32_t size;
    uint32_t free_space;
};

static struct nouveau_device global_nv_dev;
static struct nouveau_channel global_nv_chan;

static inline uint32_t nv_read32(uint32_t reg) {
    volatile uint32_t* ptr = (volatile uint32_t*)(global_nv_dev.mmio_addr + reg);
    return *ptr;
}

static inline void nv_write32(uint32_t reg, uint32_t val) {
    volatile uint32_t* ptr = (volatile uint32_t*)(global_nv_dev.mmio_addr + reg);
    *ptr = val;
}

static inline uint8_t nv_read8(uint32_t reg) {
    volatile uint8_t* ptr = (volatile uint8_t*)(global_nv_dev.mmio_addr + reg);
    return *ptr;
}

static inline void nv_write8(uint32_t reg, uint8_t val) {
    volatile uint8_t* ptr = (volatile uint8_t*)(global_nv_dev.mmio_addr + reg);
    *ptr = val;
}

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

static void update_hardware_cursor(size_t x, size_t y) {
    uint16_t pos = y * VGA_WIDTH + x;
    nv_write8(NV_IO_CRTC_INDEX, 0x0F);
    nv_write8(NV_IO_CRTC_DATA, (uint8_t)(pos & 0xFF));
    nv_write8(NV_IO_CRTC_INDEX, 0x0E);
    nv_write8(NV_IO_CRTC_DATA, (uint8_t)((pos >> 8) & 0xFF));
    uint32_t nv_pos_val = ((uint32_t)y << 16) | ((uint32_t)x & 0xFFFF);
    nv_write32(NV_PCRTC_CURSOR_POS, nv_pos_val);
}

static void wait_for_fifo(uint32_t size) {
    while (global_nv_chan.free_space < size) {
        uint32_t get = nv_read32(NV_PFIFO_CACHE1_GET);
        uint32_t put = global_nv_chan.put;
        if (get <= put) {
            global_nv_chan.free_space = global_nv_chan.size - put + get - 4;
        } else {
            global_nv_chan.free_space = get - put - 4;
        }
        for (volatile int i = 0; i < 100; i++);
    }
}

static void push_to_channel(uint32_t val) {
    wait_for_fifo(1);
    global_nv_chan.push_buffer[global_nv_chan.put / 4] = val;
    global_nv_chan.put = (global_nv_chan.put + 4) % global_nv_chan.size;
    nv_write32(NV_PFIFO_CACHE1_PUT, global_nv_chan.put);
    global_nv_chan.free_space--;
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
    
    push_to_channel(0x00040000 | (0x100 * 4));
    push_to_channel(0x00000000);
    push_to_channel(VGA_HEIGHT - 1);
    
    update_hardware_cursor(terminal_column, terminal_row);
}

void putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else if (c == '\r') {
        terminal_column = 0;
    } else if (c == '\t') {
        terminal_column = (terminal_column + 8) & ~(8 - 1);
        if (terminal_column >= VGA_WIDTH) {
            terminal_column = 0;
            terminal_row++;
        }
    } else {
        const size_t index = terminal_row * VGA_WIDTH + terminal_column;
        uint8_t highlight_color = vga_entry_color(WHITE, BLUE);
        VGA_BUFFER[index] = vga_entry(c, highlight_color);
        
        push_to_channel(0x00080000 | (0x200 * 4));
        push_to_channel((uint32_t)c);
        push_to_channel((uint32_t)highlight_color);
        
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            terminal_row++;
        }
    }

    if (terminal_row >= VGA_HEIGHT) {
        scroll();
    }
    
    update_hardware_cursor(terminal_column, terminal_row);
}

void print(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        putchar(data[i]);
for (const char* c = data; *c != '\0'; c++) {
        outb(0xe9, *c); 
    }
    }
}

void init(void) {
    global_nv_dev.chipset = 0x50;
    global_nv_dev.mmio_addr = NV_REG_BASE;
    global_nv_dev.fb_addr = NV_FB_BASE;
    global_nv_dev.ram_size = 256 * 1024 * 1024;
    global_nv_dev.crystal_khz = 27000;
    global_nv_dev.pfifo_channels = 32;
    global_nv_dev.is_active = 1;

    global_nv_chan.id = 0;
    global_nv_chan.push_buffer = (uint32_t*)(global_nv_dev.mmio_addr + 0x800000);
    global_nv_chan.put = 0;
    global_nv_chan.get = 0;
    global_nv_chan.size = 4096;
    global_nv_chan.free_space = 4096;

    uint32_t intr_state = nv_read32(NV_PMC_INTR_0);
    nv_write32(NV_PMC_INTR_0, intr_state);
    nv_write32(NV_PMC_INTR_EN_0, 0xFFFFFFFF);

    nv_write32(NV_PFIFO_CACHE1_PUSH0, 1);
    nv_write32(NV_PFIFO_CACHE1_PULL0, 1);
    nv_write32(NV_PFIFO_CACHE1_GET, 0);
    nv_write32(NV_PFIFO_CACHE1_PUT, 0);

    uint32_t graph_status = nv_read32(NV_PGRAPH_STATUS);
    if (graph_status != 0) {
        nv_write32(NV_PGRAPH_TRIGGER, 1);
        while (nv_read32(NV_PGRAPH_STATUS) & 0x01);
    }

    nv_write32(NV_PCRTC_CONFIG, 0x00000001);
    nv_write32(NV_PCRTC_CURSOR_CONFIG, 0x05000000);

    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(WHITE, BLACK);
    
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            VGA_BUFFER[index] = vga_entry(' ', terminal_color);
        }
    }

    update_hardware_cursor(terminal_column, terminal_row);
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

    push_to_channel(0x000C0000 | (0x300 * 4));
    push_to_channel(terminal_column);
    push_to_channel(terminal_row);

    update_hardware_cursor(terminal_column, terminal_row);
}

#endif
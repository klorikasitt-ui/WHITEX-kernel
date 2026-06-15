
/*
 * WhiteX 
 * DEVELOPER BURAK YAKUB GÜÇER
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
void notgud() {
    print("            !=x* \n");
    print("      ~x-+-x/x++-/!!--+-! \n");
    print("      x----------------+--- \n");
    print("     v+-------------------x \n");
    print("    *----X------X------/ \n");
    print("  =/+++----x----x!+++--+++! \n");
    print("  !!-----+FAULT--+-----++/\n");
    print("    x--------------------+~ \n");
    print("    ~+------------------++~ \n");
    print("   xx-----------------+---+! \n");
    print("  -x-----------------------+* \n");
    print(" *x-----xxxxxxxx------------+ \n");
    print(" !x---+xxxxxxxxxx+x-----+----**+--v \n");
    print(" v+----+xxxxxxx+x------+----+=v+--+ \n");
    print(" xx-++++-xxxxxxx+----++++----+! x--+ \n");
    print("/xxx-+-x-xxxxxxxxxx-++++-------+v/---! \n");
    print("++----------xxxxxxxxxx-++--+------+++--+! \n");
    print(" /+------++--xxxxxxx+----------++++-~ \n");
    print("  *-+++++++++++++----++-------+= \n");
    print("                 x++x! \n");
}
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

static inline void io_port_write_8(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint8_t io_port_read_8(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_port_write_16(uint16_t port, uint16_t data) {
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint16_t io_port_read_16(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void secure_vga_print_panic(const char* message) {
    uint16_t* vga_buffer = (uint16_t*)0xB8000;
    uint32_t index = 0;
    uint16_t color_attribute = 0x4F00; 

    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t)0x4F20;
    }

    while (message[index] != '\0') {
        vga_buffer[index] = (uint16_t)message[index] | color_attribute;
        index++;
    }
}

static void enforce_strict_memory_protection(void) {
    uint32_t cr3_register_value;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3_register_value));
    
    uint32_t* page_directory = (uint32_t*)(cr3_register_value & 0xFFFFF000);
    
    for (uint32_t directory_index = 0; directory_index < 1024; directory_index++) {
        if (page_directory[directory_index] & 0x01) {
            page_directory[directory_index] &= ~0x02; 
            
            uint32_t* page_table = (uint32_t*)(page_directory[directory_index] & 0xFFFFF000);
            for (uint32_t table_index = 0; table_index < 1024; table_index++) {
                if (page_table[table_index] & 0x01) {
                    page_table[table_index] &= ~0x02;
                }
            }
        }
    }
    
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3_register_value));
}

static void ata_pio_force_disk_flush(uint32_t start_lba, uint8_t sector_count, uint32_t source_address) {
    uint16_t ata_base_port = 0x1F0;
    
    io_port_write_8(ata_base_port + 6, 0xE0 | ((start_lba >> 24) & 0x0F));
    io_port_write_8(ata_base_port + 2, sector_count);
    io_port_write_8(ata_base_port + 3, (uint8_t)start_lba);
    io_port_write_8(ata_base_port + 4, (uint8_t)(start_lba >> 8));
    io_port_write_8(ata_base_port + 5, (uint8_t)(start_lba >> 16));
    io_port_write_8(ata_base_port + 7, 0x30);

    uint8_t status;
    do {
        status = io_port_read_8(ata_base_port + 7);
    } while ((status & 0x80) && !(status & 0x08));

    uint16_t* data_pointer = (uint16_t*)source_address;
    uint32_t total_words = (sector_count * 512) / 2;

    for (uint32_t word_index = 0; word_index < total_words; word_index++) {
        io_port_write_16(ata_base_port + 0, data_pointer[word_index]);
    }
    
    io_port_write_8(ata_base_port + 7, 0xE7);
    
    do {
        status = io_port_read_8(ata_base_port + 7);
    } while (status & 0x80);
}

static void system_reboot_sequence(void) {
    uint8_t keyboard_status;
    do {
        keyboard_status = io_port_read_8(0x64);
        if (keyboard_status & 0x01) {
            io_port_read_8(0x60);
        }
    } while (keyboard_status & 0x02);
    
    io_port_write_8(0x64, 0xFE);
    
    while (1) {
        __asm__ volatile("hlt");
    }
}

void faulthandler() {
    __asm__ volatile("cli");

    enforce_strict_memory_protection();

    ata_pio_force_disk_flush(0x00000000, 1, 0x00100000); 
    ata_pio_force_disk_flush(0x00000001, 8, 0x00100200);

    secure_vga_print_panic("CRITICAL SYSTEM FAILURE. MEMORY LOCKED. DISK FLUSHED. PRESS ANY KEY TO REBOOT.");

    uint8_t initial_scancode = io_port_read_8(0x60);
    uint8_t current_scancode;

    while (1) {
        if (io_port_read_8(0x64) & 0x01) {
            current_scancode = io_port_read_8(0x60);
            if (current_scancode != initial_scancode && (current_scancode & 0x80) == 0) {
                break;
            }
        }
        __asm__ volatile("hlt");
    }

    system_reboot_sequence();
}
static void watch_stack() {
    uint32_t current_esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(current_esp));

    if (current_esp < 0x10000) {
        faulthandler();
    }
}



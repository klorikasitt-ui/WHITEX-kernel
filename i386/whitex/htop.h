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
#ifndef HTOP_H
#define HTOP_H

#include <stdint.h>
#include <stddef.h>

static inline void htop(void) {
    char kbd_buffer[256];
    
    size_t free_ram = get_free_ram();
    size_t total_ram = 65536;
    size_t used_ram = total_ram - free_ram;
    int ram_percent = (used_ram * 100) / total_ram;

    cls();
    print("========================================\n");
    print("        WhiteX Live System Monitor      \n");
    print("========================================\n\n");
    
    print("CPU Vendor: ");
    cpuid();
    print("\n");

    print("Uptime    : ");
    ram_print_number(ticks / 100);
    print(" seconds\n\n");

    print("MEMORY [");
    int bars = ram_percent / 5;
    for (int i = 0; i < 20; i++) {
        if (i < bars) putchar('#');
        else putchar('.');
    }
    print("] ");
    ram_print_number(ram_percent);
    print("%\n");

    print("Used/Total: ");
    ram_print_number(used_ram);
    print(" / ");
    ram_print_number(total_ram);
    print(" bytes\n\n");

    print("----------------------------------------\n");
    print("Press [Enter] to exit monitor...\n");

    scan(kbd_buffer);
}

#endif

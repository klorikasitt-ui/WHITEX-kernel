#ifndef HTOP_H
#define HTOP_H

void htop() {
    while (1) {
        cls();
        size_t free_ram = get_free_ram();
        size_t used_ram = 65536 - free_ram;
        int ram_percent = (used_ram * 100) / 65536;

        print("--- WhiteX Live Monitor  ---\n");
        
        print("CPU Vendor: ");
        cpuid();

        print("Uptime    : ");
        ram_print_number(ticks / 100);
        print(" seconds\n\n");

        print("MEM [");
        int bars = ram_percent / 5;
        for (int i = 0; i < 20; i++) {
            if (i < bars) putchar('#');
            else putchar('.');
        }
        print("] ");
        ram_print_number(ram_percent);
        print("%\n");

        print("Used: ");
        ram_print_number(used_ram);
        print(" bytes\n");
        print("Free: ");
        ram_print_number(free_ram);
        print(" bytes\n");

        
        for(volatile int i = 0; i < 5000000; i++);
        
        
    }
}

#endif

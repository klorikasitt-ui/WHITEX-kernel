void shutdown() {
    __asm__ __volatile__ (
        "mov $0x5301, %%ax\n\t"
        "xor %%bx, %%bx\n\t"
        "int $0x15\n\t"   // Bağlantı kur
        "mov $0x530e, %%ax\n\t"
        "mov $0x0102, %%cx\n\t"
        "int $0x15\n\t"   
        "mov $0x5307, %%ax\n\t"
        "mov $0x0001, %%bx\n\t"
        "mov $0x0003, %%cx\n\t"
        "int $0x15"    
        : : : "ax", "bx", "cx"
    );
}

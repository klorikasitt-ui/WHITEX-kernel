#ifndef GDT_H
#define GDT_H

#include <stdint.h>

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

static gdt_entry_t gdt[3] __attribute__((aligned(8)));
static gdt_ptr_t gdt_ptr;

static inline void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access      = access;
}

static inline void init_gdt() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    __asm__ __volatile__("lgdt %0" : : "m"(gdt_ptr));
    __asm__ __volatile__(
        "movw $0x10, %ax\n"
        "movw %ax, %ds\n"
        "movw %ax, %es\n"
        "movw %ax, %fs\n"
        "movw %ax, %gs\n"
        "movw %ax, %ss\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
    );
}

#endif

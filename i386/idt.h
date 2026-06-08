#ifndef IDT_H
#define IDT_H

#include <stdint.h>
typedef struct {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;


extern idt_entry_t idt[256];
extern idt_ptr_t idt_ptr;
 void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].sel     = sel;
    idt[num].always0 = 0;
    idt[num].flags   = flags;
    idt[num].base_hi = (base >> 16) & 0xFFFF;
}
static inline void __attribute__((interrupt)) default_handler(void* frame) {
    __asm__ __volatile__("cli; hlt");
}

#endif

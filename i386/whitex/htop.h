#ifndef HTOP_H
#define HTOP_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

extern volatile unsigned int ticks;
extern void cpuid(void);
extern void cls(void);
extern void print(const char* str);
extern void scan(char* buffer);
extern void putchar(char c);

static inline void htop(void) {
    char kbd_buffer[256];
    
    cls();
    cpuid();
    
    print("Uptime:");
    ram_print_number(ticks / 100);
    print("s\n");

    print("Kernel Ticks:");
    ram_print_number(g_system_scheduler.kernel_elapsed_ticks);
    print("\n");

    print("Scheduler Cycles:");
    ram_print_number(g_system_scheduler.scheduler_cycles);
    print("\n");

    print("Heap Free:");
    ram_print_number(get_free_ram());
    print("\n");

    print("OOM Kills:");
    ram_print_number(atomic_load(&g_sys_ctrl.total_oom_kills));
    print("\n");

    print("Zone DMA Free:");
    ram_print_number(atomic_load(&g_sys_ctrl.zones[ZONE_DMA].free_pages));
    print("\n");

    print("Active Tasks:");
    ram_print_number(g_system_scheduler.active_process_count);
    print("\n");

    for (uint32_t i = 0; i < g_system_scheduler.active_process_count; i++) {
        process_control_block_t* pcb = &g_system_scheduler.process_table[i];
        
        if (pcb->execution_state != TASK_STATE_DEAD) {
            char buf[16];
            
            ram_print_number(pcb->task_id);
            print(":");
            
            scheduler_integer_to_string(pcb->memory_footprint_pages, buf);
            print(buf);
            print("pages\n");
        }
    }

    scan(kbd_buffer);
}

#endif

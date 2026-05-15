#ifndef WHITEX_TASK_H
#define WHITEX_TASK_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t esp;
    uint32_t ebp;
    uint8_t active;
} wh_task_t;

static wh_task_t task_list[3];
static uint8_t current_task = 0;
static uint32_t task_stacks[3][1024];

extern void e1000_poll(uint32_t base);
extern void time();
extern void ram_print_number(size_t n);

void task_yield() {
    uint8_t next = (current_task + 1) % 3;
    while (!task_list[next].active) {
        next = (next + 1) % 3;
    }

    asm volatile("mov %%esp, %0" : "=m"(task_list[current_task].esp));
    asm volatile("mov %%ebp, %0" : "=m"(task_list[current_task].ebp));

    current_task = next;

    asm volatile("mov %0, %%esp" : : "m"(task_list[current_task].esp));
    asm volatile("mov %0, %%ebp" : : "m"(task_list[current_task].ebp));
}

void task_net_worker() {
    while(1) {
        e1000_poll(0xFEB00000);
        task_yield();
    }
}

void task_clock_worker() {
    while(1) {
        time();
        task_yield();
    }
}

void task_monitor_worker() {
    while(1) {
        if (net_api.rx_packets % 10 == 0 && net_api.rx_packets > 0) {
            print("\n[Monitor] 10 Packets Processed.\n");
        }
        task_yield();
    }
}

void start_triple_tasking() {
    for(int i=0; i<3; i++) {
        task_list[i].esp = (uint32_t)&task_stacks[i][1023];
        task_list[i].ebp = task_list[i].esp;
        task_list[i].active = 1;
    }
    
    task_net_worker();
}

#endif

#ifndef MULTITASKING_H
#define MULTITASKING_H
#define SCHED_MAX_PRIORITY          100
#define SCHED_DEFAULT_PRIORITY      20
#define SCHED_MIN_PRIORITY          1
#define SCHED_PRIORITY_AGING_LIMIT  500
#define SCHED_STACK_SIZE            4096
#define SCHED_IPC_MAX_MESSAGES      32
#define SCHED_IPC_MSG_SIZE          128
#define TASK_STATE_BLOCKED          0x10
#define TASK_STATE_STOPPED          0x20
#define SIGNAL_SIGKILL              9
#define SIGNAL_SIGTERM              15
#define SIGNAL_SIGSTOP              19
#define SIGNAL_SIGCONT              18
#define SIGNAL_SIGINT               2

typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eflags;
} context_frame_t;

typedef struct {
    char data[SCHED_IPC_MSG_SIZE];
    uint32_t sender_id;
    uint32_t length;
    uint32_t type;
} ipc_message_t;

typedef struct {
    ipc_message_t queue[SCHED_IPC_MAX_MESSAGES];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint32_t lock_state;
} ipc_channel_t;

typedef struct {
    uint32_t pending_signals;
    uint32_t blocked_signals;
    void (*handlers[32])(int);
} signal_context_t;

typedef struct {
    uint32_t execution_start_tick;
    uint32_t total_execution_ticks;
    uint32_t context_switches;
    uint32_t last_scheduled_tick;
    uint32_t syscall_count;
    uint32_t memory_allocations;
} performance_metrics_t;

typedef struct {
    context_frame_t context;
    uint32_t task_id;
    uint32_t parent_id;
    uint32_t execution_state;
    uint32_t memory_footprint_pages;
    uint32_t base_priority;
    uint32_t dynamic_priority;
    uint32_t accumulation_cycles;
    uint32_t allocation_flags;
    void* stack_memory_block;
    signal_context_t signal_subsystem;
    performance_metrics_t metrics;
    ipc_channel_t communication_channel;
    uint32_t waiting_on_pid;
    uint32_t sleep_until_tick;
} process_control_block_t;

typedef struct {
    process_control_block_t process_table[SYS_MAX_TASKS];
    uint32_t active_process_count;
    uint32_t running_process_index;
    uint32_t scheduler_cycles;
    uint32_t global_process_identifier_seed;
    uint32_t idle_ticks;
    uint32_t kernel_elapsed_ticks;
} scheduler_state_t;

static scheduler_state_t g_system_scheduler;

extern uint32_t auto_pid;
extern void* sys_alloc_pages(int zone_idx, size_t count);

static void* scheduler_allocate_memory(size_t size) {
    size_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    return sys_alloc_pages(ZONE_NORMAL, pages_needed);
}

static void scheduler_integer_to_string(uint32_t value, char* buffer) {
    uint32_t temporary = value;
    int length = 0;
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    while (temporary > 0) {
        length++;
        temporary /= 10;
    }
    buffer[length] = '\0';
    for (int i = length - 1; i >= 0; i--) {
        buffer[i] = (value % 10) + '0';
        value /= 10;
    }
}

static void scheduler_memory_copy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

static void scheduler_memory_set(void* dest, int val, size_t n) {
    char* temp = (char*)dest;
    for (size_t i = 0; i < n; i++) {
        temp[i] = (char)val;
    }
}

void scheduler_terminate_current(void);

void scheduler_exit_stub(void) {
    scheduler_terminate_current();
    while (1) {
        __asm__ volatile("hlt");
    }
}

void scheduler_initialize(void) {
    g_system_scheduler.active_process_count = 0;
    g_system_scheduler.running_process_index = 0;
    g_system_scheduler.scheduler_cycles = 0;
    g_system_scheduler.global_process_identifier_seed = 100;
    g_system_scheduler.idle_ticks = 0;
    g_system_scheduler.kernel_elapsed_ticks = 0;
    
    for (uint32_t i = 0; i < SYS_MAX_TASKS; i++) {
        g_system_scheduler.process_table[i].task_id = 0;
        g_system_scheduler.process_table[i].execution_state = TASK_STATE_DEAD;
        g_system_scheduler.process_table[i].context.esp = 0;
        g_system_scheduler.process_table[i].context.eflags = 0;
        g_system_scheduler.process_table[i].stack_memory_block = NULL;
        scheduler_memory_set(&g_system_scheduler.process_table[i].signal_subsystem, 0, sizeof(signal_context_t));
        scheduler_memory_set(&g_system_scheduler.process_table[i].metrics, 0, sizeof(performance_metrics_t));
        scheduler_memory_set(&g_system_scheduler.process_table[i].communication_channel, 0, sizeof(ipc_channel_t));
        g_system_scheduler.process_table[i].waiting_on_pid = 0;
        g_system_scheduler.process_table[i].sleep_until_tick = 0;
    }

    uint32_t target_index = g_system_scheduler.active_process_count;
    process_control_block_t* master_pcb = &g_system_scheduler.process_table[target_index];
    
    master_pcb->task_id = 100;
    master_pcb->parent_id = 0;
    master_pcb->execution_state = TASK_STATE_RUNNING;
    master_pcb->memory_footprint_pages = 512;
    master_pcb->base_priority = SCHED_DEFAULT_PRIORITY;
    master_pcb->dynamic_priority = SCHED_DEFAULT_PRIORITY;
    master_pcb->accumulation_cycles = 0;
    master_pcb->allocation_flags = OOM_FLAG_CRITICAL;
    master_pcb->stack_memory_block = NULL;
    master_pcb->metrics.execution_start_tick = 0;
    
    g_system_scheduler.active_process_count++;
}

void scheduler_apply_aging(void) {
    for (uint32_t i = 0; i < g_system_scheduler.active_process_count; i++) {
        process_control_block_t* pcb = &g_system_scheduler.process_table[i];
        if (pcb->execution_state == TASK_STATE_RUNNING && i != g_system_scheduler.running_process_index) {
            pcb->accumulation_cycles++;
            if (pcb->accumulation_cycles >= SCHED_PRIORITY_AGING_LIMIT) {
                if (pcb->dynamic_priority < SCHED_MAX_PRIORITY) {
                    pcb->dynamic_priority++;
                }
                pcb->accumulation_cycles = 0;
            }
        }
    }
}

uint32_t scheduler_select_next_candidate(void) {
    if (g_system_scheduler.active_process_count == 0) {
        return 0;
    }

    scheduler_apply_aging();
    
    uint32_t best_index = 0;
    uint32_t highest_priority = 0;
    uint32_t found = 0;

    for (uint32_t i = 0; i < g_system_scheduler.active_process_count; i++) {
        process_control_block_t* pcb = &g_system_scheduler.process_table[i];
        if (pcb->execution_state == TASK_STATE_RUNNING) {
            if (pcb->sleep_until_tick > 0 && g_system_scheduler.kernel_elapsed_ticks < pcb->sleep_until_tick) {
                continue;
            }
            if (pcb->sleep_until_tick > 0 && g_system_scheduler.kernel_elapsed_ticks >= pcb->sleep_until_tick) {
                pcb->sleep_until_tick = 0;
            }
            if (!found || pcb->dynamic_priority > highest_priority) {
                highest_priority = pcb->dynamic_priority;
                best_index = i;
                found = 1;
            }
        }
    }

    if (found) {
        g_system_scheduler.process_table[best_index].dynamic_priority = g_system_scheduler.process_table[best_index].base_priority;
        g_system_scheduler.process_table[best_index].accumulation_cycles = 0;
        return best_index;
    }

    return 0;
}

void scheduler_yield(void) {
    g_system_scheduler.kernel_elapsed_ticks++;
    if (g_system_scheduler.active_process_count <= 1) {
        return;
    }

    uint32_t current_index = g_system_scheduler.running_process_index;
    uint32_t next_index = scheduler_select_next_candidate();
    
    if (current_index == next_index) {
        return;
    }

    g_system_scheduler.running_process_index = next_index;
    g_system_scheduler.scheduler_cycles++;

    process_control_block_t* current_pcb = &g_system_scheduler.process_table[current_index];
    process_control_block_t* next_pcb = &g_system_scheduler.process_table[next_index];

    current_pcb->metrics.total_execution_ticks += (g_system_scheduler.kernel_elapsed_ticks - current_pcb->metrics.last_scheduled_tick);
    next_pcb->metrics.last_scheduled_tick = g_system_scheduler.kernel_elapsed_ticks;
    next_pcb->metrics.context_switches++;

    __asm__ volatile (
        "pushfl\n\t"
        "pushal\n\t"
        "movl %%esp, %0\n\t"
        "movl %1, %%esp\n\t"
        "popal\n\t"
        "popfl\n\t"
        : "=m" (current_pcb->context.esp)
        : "m" (next_pcb->context.esp)
        : "memory"
    );
}

uint32_t scheduler_register_process(void(*entry_point)(void), uint32_t priority, size_t initial_rss, uint32_t allocation_flags) {
    if (g_system_scheduler.active_process_count >= SYS_MAX_TASKS) {
        return ERR_MEM_OUT_OF_MEM;
    }

    uint32_t target_index = g_system_scheduler.active_process_count;
    uint32_t generated_pid = g_system_scheduler.global_process_identifier_seed++;
    auto_pid = g_system_scheduler.global_process_identifier_seed;

    sys_task_t* sys_task_node = (sys_task_t*)scheduler_allocate_memory(sizeof(sys_task_t));
    if (!sys_task_node) {
        return ERR_MEM_FAULT;
    }

    sys_task_node->pid = generated_pid;
    sys_task_node->ppid = g_system_scheduler.process_table[g_system_scheduler.running_process_index].task_id;
    sys_task_node->state = TASK_STATE_RUNNING;
    sys_task_node->rss_pages = initial_rss;
    sys_task_node->virtual_pages = initial_rss * 2;
    sys_task_node->flags = allocation_flags;
    sys_task_node->oom_score_adj = 0;
    sys_task_node->nice_value = 0;

    sys_register_task(sys_task_node);

    void* stack_raw = scheduler_allocate_memory(SCHED_STACK_SIZE);
    if (!stack_raw) {
        return ERR_MEM_FAULT;
    }

    process_control_block_t* pcb = &g_system_scheduler.process_table[target_index];
    pcb->task_id = generated_pid;
    pcb->parent_id = g_system_scheduler.process_table[g_system_scheduler.running_process_index].task_id;
    pcb->execution_state = TASK_STATE_RUNNING;
    pcb->memory_footprint_pages = initial_rss;
    pcb->base_priority = priority;
    pcb->dynamic_priority = priority;
    pcb->accumulation_cycles = 0;
    pcb->allocation_flags = allocation_flags;
    pcb->stack_memory_block = stack_raw;
    pcb->waiting_on_pid = 0;
    pcb->sleep_until_tick = 0;
    
    scheduler_memory_set(&pcb->signal_subsystem, 0, sizeof(signal_context_t));
    scheduler_memory_set(&pcb->metrics, 0, sizeof(performance_metrics_t));
    scheduler_memory_set(&pcb->communication_channel, 0, sizeof(ipc_channel_t));

    pcb->metrics.execution_start_tick = g_system_scheduler.kernel_elapsed_ticks;
    pcb->metrics.last_scheduled_tick = g_system_scheduler.kernel_elapsed_ticks;

    uint32_t* stack_pointer = (uint32_t*)((uintptr_t)stack_raw + SCHED_STACK_SIZE);
    
    *(--stack_pointer) = (uint32_t)scheduler_exit_stub;
    *(--stack_pointer) = (uint32_t)entry_point;
    *(--stack_pointer) = 0x202;
    *(--stack_pointer) = 0;
    *(--stack_pointer) = 0;
    *(--stack_pointer) = 0;
    *(--stack_pointer) = 0;
    *(--stack_pointer) = 0;
    *(--stack_pointer) = 0;
    *(--stack_pointer) = 0;
    *(--stack_pointer) = 0;

    pcb->context.esp = (uint32_t)stack_pointer;

    g_system_scheduler.active_process_count++;
    return ERR_OK;
}

void dummy_task_procedure(void) {
    while (1) {
        print(" [PROCESS] Background execution loop running...\n");
        scheduler_yield();
    }
}

void newtask2(void) {
    scheduler_register_process(dummy_task_procedure, SCHED_DEFAULT_PRIORITY, 128, 0);
}

void scheduler_sleep(uint32_t ticks) {
    uint32_t current_index = g_system_scheduler.running_process_index;
    g_system_scheduler.process_table[current_index].sleep_until_tick = g_system_scheduler.kernel_elapsed_ticks + ticks;
    scheduler_yield();
}

uint32_t scheduler_send_signal(uint32_t target_pid, uint32_t signal) {
    if (signal > 31) {
        return ERR_INVALID_OP;
    }
    for (uint32_t i = 0; i < g_system_scheduler.active_process_count; i++) {
        process_control_block_t* pcb = &g_system_scheduler.process_table[i];
        if (pcb->task_id == target_pid && pcb->execution_state != TASK_STATE_DEAD) {
            pcb->signal_subsystem.pending_signals |= (1 << signal);
            if (signal == SIGNAL_SIGKILL) {
                pcb->execution_state = TASK_STATE_DEAD;
                if (g_system_scheduler.running_process_index == i) {
                    scheduler_yield();
                }
            } else if (signal == SIGNAL_SIGSTOP) {
                pcb->execution_state = TASK_STATE_STOPPED;
                if (g_system_scheduler.running_process_index == i) {
                    scheduler_yield();
                }
            } else if (signal == SIGNAL_SIGCONT) {
                pcb->execution_state = TASK_STATE_RUNNING;
            }
            return ERR_OK;
        }
    }
    return ERR_IO_NOT_FOUND;
}

void scheduler_check_pending_signals(void) {
    uint32_t current_index = g_system_scheduler.running_process_index;
    process_control_block_t* pcb = &g_system_scheduler.process_table[current_index];
    uint32_t active_signals = pcb->signal_subsystem.pending_signals & ~pcb->signal_subsystem.blocked_signals;
    
    if (active_signals == 0) {
        return;
    }

    for (int i = 0; i < 32; i++) {
        if (active_signals & (1 << i)) {
            pcb->signal_subsystem.pending_signals &= ~(1 << i);
            if (pcb->signal_subsystem.handlers[i] != NULL) {
                pcb->signal_subsystem.handlers[i](i);
            }
        }
    }
}

uint32_t scheduler_register_signal_handler(uint32_t signal, void (*handler)(int)) {
    if (signal > 31 || signal == SIGNAL_SIGKILL || signal == SIGNAL_SIGSTOP) {
        return ERR_IO_PERMISSION;
    }
    uint32_t current_index = g_system_scheduler.running_process_index;
    g_system_scheduler.process_table[current_index].signal_subsystem.handlers[signal] = handler;
    return ERR_OK;
}

uint32_t scheduler_ipc_send(uint32_t destination_pid, const void* message_ptr, uint32_t length, uint32_t type) {
    if (length > SCHED_IPC_MSG_SIZE) {
        return ERR_OVERFLOW;
    }
    for (uint32_t i = 0; i < g_system_scheduler.active_process_count; i++) {
        process_control_block_t* dest_pcb = &g_system_scheduler.process_table[i];
        if (dest_pcb->task_id == destination_pid && dest_pcb->execution_state != TASK_STATE_DEAD) {
            ipc_channel_t* channel = &dest_pcb->communication_channel;
            if (channel->count >= SCHED_IPC_MAX_MESSAGES) {
                return ERR_BUSY;
            }
            ipc_message_t* msg = &channel->queue[channel->tail];
            msg->sender_id = g_system_scheduler.process_table[g_system_scheduler.running_process_index].task_id;
            msg->length = length;
            msg->type = type;
            scheduler_memory_copy(msg->data, message_ptr, length);
            channel->tail = (channel->tail + 1) % SCHED_IPC_MAX_MESSAGES;
            channel->count++;
            if (dest_pcb->execution_state == TASK_STATE_BLOCKED && dest_pcb->waiting_on_pid == 0) {
                dest_pcb->execution_state = TASK_STATE_RUNNING;
            }
            return ERR_OK;
        }
    }
    return ERR_IO_NOT_FOUND;
}

uint32_t scheduler_ipc_receive(void* buffer_ptr, uint32_t max_length, uint32_t* sender_id, uint32_t* type) {
    uint32_t current_index = g_system_scheduler.running_process_index;
    process_control_block_t* current_pcb = &g_system_scheduler.process_table[current_index];
    ipc_channel_t* channel = &current_pcb->communication_channel;

    while (channel->count == 0) {
        current_pcb->execution_state = TASK_STATE_BLOCKED;
        current_pcb->waiting_on_pid = 0;
        scheduler_yield();
    }

    ipc_message_t* msg = &channel->queue[channel->head];
    uint32_t copy_length = (msg->length > max_length) ? max_length : msg->length;
    scheduler_memory_copy(buffer_ptr, msg->data, copy_length);
    if (sender_id != NULL) {
        *sender_id = msg->sender_id;
    }
    if (type != NULL) {
        *type = msg->type;
    }
    channel->head = (channel->head + 1) % SCHED_IPC_MAX_MESSAGES;
    channel->count--;
    return copy_length;
}

uint32_t scheduler_terminate_process(uint32_t target_pid) {
    for (uint32_t i = 0; i < g_system_scheduler.active_process_count; i++) {
        if (g_system_scheduler.process_table[i].task_id == target_pid) {
            if (target_pid == 100 || (g_system_scheduler.process_table[i].allocation_flags & OOM_FLAG_CRITICAL)) {
                return ERR_IO_PERMISSION;
            }
            g_system_scheduler.process_table[i].execution_state = TASK_STATE_DEAD;
            for (uint32_t j = 0; j < g_system_scheduler.active_process_count; j++) {
                if (g_system_scheduler.process_table[j].execution_state == TASK_STATE_BLOCKED && 
                    g_system_scheduler.process_table[j].waiting_on_pid == target_pid) {
                    g_system_scheduler.process_table[j].execution_state = TASK_STATE_RUNNING;
                    g_system_scheduler.process_table[j].waiting_on_pid = 0;
                }
            }
            return ERR_OK;
        }
    }
    return ERR_IO_NOT_FOUND;
}

void scheduler_terminate_current(void) {
    uint32_t current_index = g_system_scheduler.running_process_index;
    uint32_t current_pid = g_system_scheduler.process_table[current_index].task_id;
    scheduler_terminate_process(current_pid);
    scheduler_yield();
}

uint32_t scheduler_wait_pid(uint32_t target_pid) {
    uint32_t current_index = g_system_scheduler.running_process_index;
    process_control_block_t* current_pcb = &g_system_scheduler.process_table[current_index];
    
    uint32_t target_exists = 0;
    for (uint32_t i = 0; i < g_system_scheduler.active_process_count; i++) {
        if (g_system_scheduler.process_table[i].task_id == target_pid && 
            g_system_scheduler.process_table[i].execution_state != TASK_STATE_DEAD) {
            target_exists = 1;
            break;
        }
    }

    if (!target_exists) {
        return ERR_IO_NOT_FOUND;
    }

    current_pcb->execution_state = TASK_STATE_BLOCKED;
    current_pcb->waiting_on_pid = target_pid;
    scheduler_yield();
    return ERR_OK;
}

void ps(void) {
    print("\n");
    print("PID       PPID      STATE       MEMORY (PAGES)    PRIORITY   CON-SW\n");
    print("-------------------------------------------------------------------\n");
    for (uint32_t i = 0; i < g_system_scheduler.active_process_count; i++) {
        process_control_block_t* pcb = &g_system_scheduler.process_table[i];
        if (pcb->execution_state != TASK_STATE_DEAD) {
            char conversion_buffer[32];
            
            scheduler_integer_to_string(pcb->task_id, conversion_buffer);
            print(conversion_buffer);
            print("       ");

            scheduler_integer_to_string(pcb->parent_id, conversion_buffer);
            print(conversion_buffer);
            print("       ");

            if (pcb->execution_state == TASK_STATE_RUNNING) {
                print("RUNNING     ");
            } else if (pcb->execution_state == TASK_STATE_BLOCKED) {
                print("BLOCKED     ");
            } else if (pcb->execution_state == TASK_STATE_STOPPED) {
                print("STOPPED     ");
            } else {
                print("SLEEPING    ");
            }

            scheduler_integer_to_string(pcb->memory_footprint_pages, conversion_buffer);
            print(conversion_buffer);
            print("               ");

            scheduler_integer_to_string(pcb->base_priority, conversion_buffer);
            print(conversion_buffer);
            print("         ");

            scheduler_integer_to_string(pcb->metrics.context_switches, conversion_buffer);
            print(conversion_buffer);
            print("\n");
        }
    }
}

#endif

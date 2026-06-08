#ifndef SYS_TOOLS_H
#define SYS_TOOLS_H
void* memset(void* dest, int val, uint32_t count) {
    unsigned char* ptr = (unsigned char*)dest;
    while (count--) {
        *ptr++ = (unsigned char)val;
    }
    return dest;
}

void print_int(uint32_t n) {
    if (n == 0) {
        print("0");
        return;
    }
    
    char buf[12];
    int i = 0;
    
  
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    
  
    char str[2] = {0, 0};
    while (--i >= 0) {
        str[0] = buf[i];
        print(str);
    }
}


typedef struct {
    uint32_t state;
    uint32_t owner_pid;
} mutex_t;
void mutex_init(mutex_t* mutex) {
    mutex->state = 0;
    mutex->owner_pid = 0;
}

void mutex_lock(mutex_t* mutex) {
    uint32_t current_pid = g_system_scheduler.process_table[g_system_scheduler.running_process_index].task_id;
    while (mutex->state == 1) {
        scheduler_yield();
    }
    mutex->state = 1;
    mutex->owner_pid = current_pid;
}

void mutex_unlock(mutex_t* mutex) {
    uint32_t current_pid = g_system_scheduler.process_table[g_system_scheduler.running_process_index].task_id;
    if (mutex->owner_pid == current_pid) {
        mutex->state = 0;
        mutex->owner_pid = 0;
    }
}

static uint32_t parse_integer(const char* str) {
    uint32_t result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

void cmd_kill(char* arg) {
    if (!arg || arg[0] == '\0') {
        print("Usage: kill <pid>\n");
        return;
    }
    uint32_t target_pid = parse_integer(arg);
    uint32_t status = scheduler_terminate_process(target_pid);
    if (status == ERR_OK) {
        print("Process terminated successfully.\n");
    } else if (status == ERR_IO_PERMISSION) {
        print("Error: System critical processes cannot be terminated!\n");
    } else {
        print("Error: Specified PID not found.\n");
    }
}

void cmd_top(void) {
    cls();
    while (1) {
        if (inb(0x64) & 0x01) {
            uint8_t key = inb(0x60);
            if (key == 0x10 || key == 0x01) {
                cls();
                break;
            }
        }
        
        terminal_row = 0;
        terminal_column = 0;
        
        print("================= WHITEX LIVE SYSTEM MONITOR =================\n");
        print("Press 'q' or 'ESC' to exit system monitor.\n");
        print("----------------------------------------------------------------\n");
        print("PID       PPID      STATE       MEMORY (PAGES)    PRIORITY   CON-SW\n");
        print("----------------------------------------------------------------\n");
        
        for (uint32_t i = 0; i < g_system_scheduler.active_process_count; i++) {
            process_control_block_t* pcb = &g_system_scheduler.process_table[i];
            if (pcb->execution_state != TASK_STATE_DEAD) {
                char buffer[32];
                
                scheduler_integer_to_string(pcb->task_id, buffer);
                print(buffer);
                print("       ");

                scheduler_integer_to_string(pcb->parent_id, buffer);
                print(buffer);
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

                scheduler_integer_to_string(pcb->memory_footprint_pages, buffer);
                print(buffer);
                print("               ");

                scheduler_integer_to_string(pcb->base_priority, buffer);
                print(buffer);
                print("         ");

                scheduler_integer_to_string(pcb->metrics.context_switches, buffer);
                print(buffer);
                print("\n");
            }
        }
        
        for (int delay = 0; delay < 10000; delay++) {
            scheduler_yield();
        }
    }
}

void cmd_msgsend(char* args) {
    if (!args || args[0] == '\0') {
        print("Usage: msgsend <target_pid> <message>\n");
        return;
    }
    
    char pid_str[16];
    int idx = 0;
    while (args[idx] != ' ' && args[idx] != '\0' && idx < 15) {
        pid_str[idx] = args[idx];
        idx++;
    }
    pid_str[idx] = '\0';
    
    if (args[idx] == ' ') {
        idx++;
    }
    
    uint32_t dest_pid = parse_integer(pid_str);
    char* message = args + idx;
    
    uint32_t len = 0;
    while (message[len] != '\0') {
        len++;
    }
    
    uint32_t status = scheduler_ipc_send(dest_pid, message, len + 1, 1);
    if (status == ERR_OK) {
        print("Message sent successfully.\n");
    } else {
        print("Error: Message could not be sent.\n");
    }
}

void cmd_msgrecv(void) {
    char buffer[128];
    uint32_t sender = 0;
    uint32_t type = 0;
    
    print("Waiting for message...\n");
    uint32_t received = scheduler_ipc_receive(buffer, 127, &sender, &type);
    
    if (received > 0) {
        char sender_str[16];
        scheduler_integer_to_string(sender, sender_str);
        print("Sender PID: ");
        print(sender_str);
        print("\nMessage: ");
        print(buffer);
        print("\n");
    } else {
        print("Error: Message could not be received.\n");
    }
}
void bot() {
    print("stress test\n");
    
    uint32_t start_time = ticks; 
    uint32_t msg_count = 0;
    uint32_t err_count = 0;
    
    char hex_buf[11];

    while ((ticks - start_time) < 3000) {
        uint32_t target_pid = (ticks % 20) + 1;
        
        if (scheduler_ipc_send(target_pid, "STRESS", 7, 1) != 0) {
            err_count++;
        }
        
        msg_count++;
        scheduler_yield();
    }
    
    print("Test ok!\n");
    print("Total msg: ");
    print_int(msg_count);
    
    print("\nError (Hex): ");
    to_hex(err_count, hex_buf); 
    print(hex_buf);
    print("\n");
}


#endif

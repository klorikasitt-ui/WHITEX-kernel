#ifndef SYS_TOOLS_H
#define SYS_TOOLS_H

#include <stdint.h>



void* memset(void* dest, int val, uint32_t count) {
    unsigned char* ptr = (unsigned char*)dest;
    while (count--) {
        *ptr++ = (unsigned char)val;
    }
    return dest;
}

static inline void print_int(uint32_t n) {
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

/*static inline void mutex_init(mutex_t* mutex) {
    mutex->state = 0;
    mutex->owner_pid = 0;
}
static inline void mutex_lock(mutex_t* mutex) {
    uint32_t current_pid = g_system_scheduler.process_table[g_system_scheduler.running_process_index].task_id;
    while (mutex->state == 1) {
        scheduler_yield();
    }
    mutex->state = 1;
    mutex->owner_pid = current_pid;
}

static inline void mutex_unlock(mutex_t* mutex) {
    uint32_t current_pid = g_system_scheduler.process_table[g_system_scheduler.running_process_index].task_id;
    if (mutex->owner_pid == current_pid) {
        mutex->state = 0;
        mutex->owner_pid = 0;
    }
}
*/
static inline uint32_t parse_integer(const char* str) {
    uint32_t result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

static inline void cmd_kill(char* arg) {
    if (!arg || arg[0] == '\0') {
        print("Usage: kill <pid>\n");
        return;
    }
    uint32_t target_pid = parse_integer(arg);
    uint32_t status = scheduler_terminate_process(target_pid);
    if (status == 0) {
        print("Process terminated successfully.\n");
    } else {
        print("Error: PID not found or permission denied.\n");
    }
}

static inline void cmd_top(void) {
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
        for (uint32_t i = 0; i < g_system_scheduler.active_process_count; i++) {
            process_control_block_t* pcb = &g_system_scheduler.process_table[i];
            if (pcb->execution_state != 0) {
                char buffer[32];
                scheduler_integer_to_string(pcb->task_id, buffer);
                print(buffer);
                print("\n");
            }
        }
        for (int delay = 0; delay < 10000; delay++) {
            scheduler_yield();
        }
    }
}

static inline void cmd_msgsend(char* args) {
    if (!args || args[0] == '\0') return;
    char pid_str[16];
    int idx = 0;
    while (args[idx] != ' ' && args[idx] != '\0') { pid_str[idx] = args[idx]; idx++; }
    pid_str[idx] = '\0';
    uint32_t dest_pid = parse_integer(pid_str);
    char* message = args + idx + 1;
    uint32_t len = 0;
    while (message[len] != '\0') len++;
    scheduler_ipc_send(dest_pid, message, len + 1, 1);
}

static inline void cmd_msgrecv(void) {
    char buffer[128];
    uint32_t sender = 0;
    uint32_t type = 0;
    uint32_t received = scheduler_ipc_receive(buffer, 127, &sender, &type);
    if (received > 0) {
        print("Message: ");
        print(buffer);
        print("\n");
    }
}

static inline void bot() {
    uint32_t start_time = ticks;
    while ((ticks - start_time) < 3000) {
        scheduler_ipc_send((ticks % 20) + 1, "STRESS", 7, 1);
        scheduler_yield();
    }
    print("Test ok!\n");
}
/*
static inline void wx_pkg_listen(void) {
}
*/
#endif
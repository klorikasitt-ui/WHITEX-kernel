/*
 * WhiteX 
 * DEVELOPER BURAK YAKUB GÜÇER
 * WARNING: THE CODE INCLUDED WITHIN THE headerlist.h FILE CALLED BY THIS FILE IS ALSO PROTECTED BY THE GPL.
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
#include "headerlist.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SYS_TICK_MAX 3000
#define SYS_CORE_MAGIC 0xC0DEBABE
#define SYS_STACK_GUARD 0xDEADBEEF
#define SYS_MAX_BUFFER 256
#define SYS_CMD_MAX 64
#define SYS_ARG_MAX 190
#define SYS_LOG_CAPACITY 128
#define FORTUNE_COUNT 9
#define VM_RAM_SIZE 0x10000
#define CANARY_VALUE 0xCAFEBABE0

extern void execute_cpu_stress_entropy(void);
//extern uint32_t read_hardware_entropy(void);

uint64_t g_tick_counter = 0;
char g_io_buffer[2048];
static uint32_t g_global_canary = CANARY_VALUE;

const char* g_fortunes[FORTUNE_COUNT] = {
    "I'm a kernel, not a debugger!",
    "I'm a kernel, not a dishwasher. Stop asking me to scrub memory!",
    "I'm a kernel, not a magician. I can't fix your segmentation fault.",
    "I'm a kernel, not a psychic. How should I know what the user wants?",
    "I'm a kernel, not an electrician. Stop trying to power cycle the CPU.",
    "I'm a kernel, not a therapist. Your code's emotional issues are your own.",
    "I'm a kernel, not a librarian. I don't know where you hid that pointer.",
    "What am I, a kernel or a garbage collector? Handle your own memory!",
    "I'm a kernel, not a coalminer. Don't dig through my paging tables!"
};

#pragma pack(push, 1)
typedef struct {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t useresp;
    uint32_t ss;
} cpu_state_t;

typedef struct {
    uint32_t log_level;
    uint64_t timestamp;
    char message[128];
    uint32_t integrity_checksum;
} log_entry_t;

typedef struct {
    log_entry_t entries[SYS_LOG_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint32_t structures_canary;
} sys_log_ring_t;

typedef struct {
    uint32_t base_address;
    uint32_t limit;
    uint32_t flags;
    uint8_t privilege_level;
    uint8_t is_present;
} memory_zone_t;

typedef struct {
    uint32_t system_state;
    uint32_t memory_guard;
    uint32_t execution_cycles;
    uint32_t fault_detected;
    cpu_state_t last_known_state;
    memory_zone_t kernel_zone;
    memory_zone_t user_zone;
    uint8_t is_root;
    uint8_t isolation_active;
    uint32_t aslr_offset;
    uint32_t structural_canary;
} sys_environment_t;
#pragma pack(pop)

typedef void (*routine_void_t)(void);
typedef void (*routine_arg_t)(char*);

typedef struct {
    const char *signature;
    routine_void_t exec_ptr;
    routine_arg_t exec_arg_ptr;
    uint8_t requires_args;
    uint8_t min_privilege;
} dispatch_entry_t;

static sys_environment_t g_env;
static sys_log_ring_t g_sys_log;
static VMState g_kernel_vm;
static uint8_t g_vm_ram[VM_RAM_SIZE];



    


static void sys_halt_cpu(void) {
    __asm__ volatile("cli");
    while (1) {
        __asm__ volatile("hlt");
    }
}


static void sys_memzero(void *dest, size_t count) {
    volatile uint8_t *ptr = (volatile uint8_t *)dest;
    while (count--) {
        *ptr++ = 0;
    }
}

static void sys_memcpy(void *dest, const void *src, size_t count) {
    volatile uint8_t *d = (volatile uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (count--) {
        *d++ = *s++;
    }
}

static uint32_t sys_calculate_checksum(const char *str, size_t len) {
    uint32_t hash = 5381;
    for (size_t i = 0; i < len && str[i] != '\0'; i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}

static uint8_t sys_constant_time_compare(const char *input, const char *expected, size_t max_len) {
    if (!input || !expected) return 0;
    size_t actual_len = 0;
    size_t expected_len = 0;
    while (input[actual_len] != '\0' && actual_len < max_len) actual_len++;
    while (expected[expected_len] != '\0' && expected_len < max_len) expected_len++;
    if (actual_len != expected_len) return 0;
    volatile uint8_t result = 0;
    for (size_t i = 0; i < actual_len; i++) {
        result |= (input[i] ^ expected[i]);
    }
    return result == 0 ? 1 : 0;
}

static void sys_safe_strncpy(char *dest, const char *src, size_t max_len) {
    if (!dest || !src || max_len == 0) return;
    size_t i = 0;
    while (i < (max_len - 1) && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void sys_sanitize_string(char *str, size_t max_len) {
    if (!str) return;
    for (size_t i = 0; i < max_len; i++) {
        if (str[i] == '\0') break;
        if (str[i] == '\n' || str[i] == '\r' || str[i] == ';' || str[i] == '&' || str[i] == '|') {
            str[i] = '\0';
            break;
        }
    }
}

static void sys_log_write(uint32_t level, const char *msg) {
    if (g_sys_log.structures_canary != g_global_canary) {
        sys_halt_cpu();
    }
    if (g_sys_log.count >= SYS_LOG_CAPACITY) {
        g_sys_log.head = (g_sys_log.head + 1) % SYS_LOG_CAPACITY;
        g_sys_log.count--;
    }
    g_sys_log.entries[g_sys_log.tail].log_level = level;
    g_sys_log.entries[g_sys_log.tail].timestamp = g_tick_counter;
    sys_safe_strncpy(g_sys_log.entries[g_sys_log.tail].message, msg, 128);
    g_sys_log.entries[g_sys_log.tail].integrity_checksum = sys_calculate_checksum(msg, 128);
    g_sys_log.tail = (g_sys_log.tail + 1) % SYS_LOG_CAPACITY;
    g_sys_log.count++;
}

void sys_panic(const char *error_code) {
    cls();
    __asm__ volatile("cli");
    notgud();
    g_env.fault_detected = 1;
    g_env.system_state = 0;
    print("\n[CRITICAL KERNEL PANIC] HARDWARE MUTEX LOCK APPLIED.\nCODE: ");
    hexdump();
    print((char*)error_code);
    print("\n");
    sys_log_write(3, error_code);
    sys_halt_cpu();
}

static void sys_verify_integrity(void) {
    if (g_env.structural_canary != g_global_canary || g_sys_log.structures_canary != g_global_canary) {
        sys_panic("ERR_INTEGRITY_VIOLATION_CANARY_BURST");
    }
    if (g_env.memory_guard != SYS_STACK_GUARD && g_env.isolation_active) {
        sys_panic("ERR_INTEGRITY_VIOLATION_STACK_SMASH");
    }
    if (g_env.kernel_zone.base_address != 0x00000000 || g_env.kernel_zone.limit != 0x000FFFFF) {
        sys_panic("ERR_INTEGRITY_VIOLATION_KERNEL_ZONE");
    }
}

static void sys_env_bootstrap(void) {
    g_global_canary ^= 0x55AA55AA;
    
    g_env.structural_canary = g_global_canary;
    g_sys_log.structures_canary = g_global_canary;
    
    g_env.system_state = 1;
    g_env.memory_guard = SYS_STACK_GUARD;
    g_env.execution_cycles = 0;
    g_env.fault_detected = 0;
    g_env.is_root = 0;
    g_env.isolation_active = 1;
    g_env.aslr_offset & 0xFFF;
    
    g_env.kernel_zone.base_address = 0x00000000;
    g_env.kernel_zone.limit = 0x000FFFFF;
    g_env.kernel_zone.flags = 0x9A;
    g_env.kernel_zone.privilege_level = 0;
    g_env.kernel_zone.is_present = 1;

    sys_memzero(&g_env.last_known_state, sizeof(cpu_state_t));
    sys_memzero(&g_sys_log, sizeof(sys_log_ring_t));
    g_sys_log.structures_canary = g_global_canary;
    
    sys_log_write(0, "KERNEL_BOOTSTRAP_INITIALIZED_WITH_ASLR");
    
}

static void sys_vm_init(void) {
    sys_verify_integrity();
    if (g_env.is_root == 0 && g_env.isolation_active) {
        print("SYS_ERR: Unauthorized virtualization request.\n");
        return;
    }
    VMState* local_vm = vm_create(VM_RAM_SIZE);
    if (!local_vm) {
        sys_log_write(2, "VM_ALLOCATION_FAILED");
        print("SYS_WARN: Virtual Machine allocation failed.\n");
        return;
    }
    sys_log_write(1, "VM_ENVIRONMENT_STARTED");
    vm_run(local_vm);
    vm_destroy(local_vm);
}

static void cmd_echo_handler(char *arg_buffer) {
    if (!arg_buffer || arg_buffer[0] == '\0') {
        print("\n");
        return;
    }
    echo(arg_buffer);
}

static void cmd_sudo_handler(char *arg_buffer) {
    sys_verify_integrity();
    char auth_query[SYS_MAX_BUFFER];
    char cred_entry[SYS_MAX_BUFFER];

    print("Do you have root privileges? (yes/no): ");
    sys_memzero(auth_query, SYS_MAX_BUFFER);
    scan(auth_query);
    __asm__ volatile("cli");
    sys_sanitize_string(auth_query, SYS_MAX_BUFFER);

    if (sys_constant_time_compare(auth_query, "yes", SYS_MAX_BUFFER) || sys_constant_time_compare(auth_query, "y", SYS_MAX_BUFFER)) {
        print("Enter root authentication password: ");
        sys_memzero(cred_entry, SYS_MAX_BUFFER);
        scan(cred_entry);
        __asm__ volatile("cli");
        sys_sanitize_string(cred_entry, SYS_MAX_BUFFER);

        if (sys_constant_time_compare(cred_entry, "root", SYS_MAX_BUFFER)) {
            g_env.is_root = 1;
            g_env.isolation_active = 0;
            g_env.memory_guard = 0;
            sys_log_write(1, "PRIVILEGE_ESCALATION_SUCCESS_RING0");
            print("Authentication successful. Ring 0 overrides active.\n");
        } else {
            g_env.is_root = 0;
            sys_log_write(2, "PRIVILEGE_ESCALATION_FAILED_INVALID_CREDENTIAL");
            print("Authentication rejected. Invalid credential matrix context.\n");
        }
    } else {
        g_env.is_root = 0;
        print("Access denied. Operational authority state remains unverified.\n");
    }
}

static void sys_kill_protect(void) {
    sys_verify_integrity();
    if (g_env.is_root == 0) {
        print("Access denied. Insufficient security clearance.\n");
        return;
    }
    g_env.is_root = 1;
    g_env.memory_guard = 0;
    g_env.isolation_active = 0;
    sys_log_write(2, "SECURITY_PROTECTION_DISABLED_WARNING");
    print("System protection disabled. Ring 0 override active.\n");
}

static void sys_open_safe(void) {
    g_env.memory_guard = SYS_STACK_GUARD;
    g_env.is_root = 0;
    g_env.isolation_active = 1;
    sys_log_write(0, "SECURITY_PROTECTION_RESTORED");
    print("WARNING: System integrity restored. Protection routines re-enabled.\n");
}

void sys_print_int(int n) {
    if (n == 0) {
        print("0");
        return;
    }
    char buffer[12];
    int i = 10;
    buffer[11] = '\0';
    while (n > 0 && i > 0) {
        buffer[--i] = (n % 10) + '0';
        n /= 10;
    }
    print(&buffer[i]);
}

void cmd_random_fortune(void) {
    static int index = 0;
    print(g_fortunes[index]);
    print("\n");
    index++;
    if (index >= FORTUNE_COUNT) {
        index = 0;
    }
}

void cmd_sys_logs(void) {
    sys_verify_integrity();
    if (g_sys_log.count == 0) {
        print("Log buffer is empty.\n");
        return;
    }
    uint32_t current = g_sys_log.head;
    for (uint32_t i = 0; i < g_sys_log.count; i++) {
        uint32_t calculated = sys_calculate_checksum(g_sys_log.entries[current].message, 128);
        if (calculated != g_sys_log.entries[current].integrity_checksum) {
            sys_panic("ERR_LOG_BUFFER_TAMPERED");
        }
        print("[T+");
        sys_print_int(g_sys_log.entries[current].timestamp);
        print("] LVL: ");
        sys_print_int(g_sys_log.entries[current].log_level);
        print(" | MSG: ");
        print(g_sys_log.entries[current].message);
        print("\n");
        current = (current + 1) % SYS_LOG_CAPACITY;
    }
}
// 1


static const dispatch_entry_t g_dispatch_table[] = {
    {"help", help, 0, 0},
    {"logo", logo, 0, 0},
    {"cls", cls, 0, 0},
    {"uname", uname, 0, 0},
    {"reboot", reboot, 0, 0},
    {"shutdown", shutdown, 0, 0},
    {"ls", ls, 0, 0},
    {"mkdir", mkdir, 0, 0},
    {"cd", cd, 0, 0},
    {"pwd", pwd, 0, 0},
    {"hexdump", hexdump, 0, 0},
    {"initfs", init_fs, 0, 0},
    {"cpuid", cpuid, 0, 0},
    {"notepad", notepad, 0, 0},
    {"kiskrnl", ant, 0, 0},
    {"time", time, 0, 0},
    {"internet", internetmain, 0, 0},
    {"sddshell", shellfs, 0, 0},
    {"ram", ram, 0, 0},
    {"htop", htop, 0, 0},
    {"song", melodi, 0, 0},
    {"echo", 0, cmd_echo_handler, 1},
    {"ntask", new_task, 0, 0},
    {"ps", ps, 0, 0},
    {"top", cmd_top, 0, 0},
    {"kill", 0, cmd_kill, 1},
    {"msgsend", 0, cmd_msgsend, 1},
    {"msgrecv", cmd_msgrecv, 0, 0},
    {"pkg", 0, wx_pkg_cli_handler, 1},
    {"calc", 0, cmd_calc_handler, 1},
    {"telemetry", cmd_lifecycle_telemetry, 0, 0},
    {"lcreset", 0, cmd_lifecycle_reset, 1},
    {"panic", 0, faulthandler, 0},
  //  {"cpu-stress", 0, execute_cpu_stress_entropy, 0},
    {"sudo", 0, cmd_sudo_handler, 1},
    {"nano", 0, execute_nano_terminal_simulator, 0},
    {"ping", ping, 0, 0},
    {"fortune", cmd_random_fortune, 0, 0},
    {"power", batt, 0, 0}
};



#define DISPATCH_TABLE_SIZE 40

// (sizeof(g_dispatch_table) / sizeof(dispatch_entry_t))

static void sys_execute_vector(const char *cmd_name, char *args) {
    sys_verify_integrity();
    if (!cmd_name || cmd_name[0] == '\0') return;

    for (size_t i = 0; i < DISPATCH_TABLE_SIZE; i++) {
        if (sys_constant_time_compare(cmd_name, g_dispatch_table[i].signature, SYS_CMD_MAX)) {
            if (g_dispatch_table[i].min_privilege > 0 && g_env.is_root == 0 && g_env.isolation_active) {
                sys_log_write(2, "UNAUTHORIZED_EXECUTION_ATTEMPT");
                print("Permission denied. System operation restricted to authorized context.\n");
                return;
            }
            
            g_env.execution_cycles++;
            sys_log_write(0, cmd_name);
            
            if (g_dispatch_table[i].requires_args) {
                if (g_dispatch_table[i].exec_arg_ptr) {
                    g_dispatch_table[i].exec_arg_ptr(args);
                }
            } else {
                if (g_dispatch_table[i].exec_ptr) {
                    g_dispatch_table[i].exec_ptr();
                }
            }
            return;
        }
    }

    print("Unknown command -> ");
    print((char*)cmd_name);
    print("\n");
}

static void sys_parse_input(char *raw_buffer) {
    sys_verify_integrity();
    if (!raw_buffer || raw_buffer[0] == '\0' || raw_buffer[0] == ' ') return;

    char extracted_cmd[SYS_CMD_MAX];
    char extracted_args[SYS_ARG_MAX];
    
    sys_memzero(extracted_cmd, SYS_CMD_MAX);
    sys_memzero(extracted_args, SYS_ARG_MAX);

    uint32_t idx = 0;
    uint32_t cmd_idx = 0;
    
    while (raw_buffer[idx] != '\0' && raw_buffer[idx] != ' ' && cmd_idx < (SYS_CMD_MAX - 1)) {
        extracted_cmd[cmd_idx++] = raw_buffer[idx++];
    }
    extracted_cmd[cmd_idx] = '\0';

    while (raw_buffer[idx] == ' ') {
        idx++;
    }

    if (raw_buffer[idx] != '\0') {
        uint32_t arg_idx = 0;
        while (raw_buffer[idx] != '\0' && arg_idx < (SYS_ARG_MAX - 1)) {
            extracted_args[arg_idx++] = raw_buffer[idx++];
        }
        extracted_args[arg_idx] = '\0';
    }

    sys_execute_vector(extracted_cmd, extracted_args);
}

void sys_execute_startup_routines() {
    sys_env_bootstrap();
    init();
    init_gdt();
    init_idt();
    init_fs();
    pit_init();
    ram();
    Sdd();
    melodi();
    cpuid();
    cls();
    login();
    scheduler_initialize();
    new_task();
    wx_pkg_system_bootstrap();
    logo();
}

void Kernel() {
    sys_execute_startup_routines();

    print("Welcome to WhiteX \n");
    print("Type 'help' for available system routines.\n");

    while(g_env.system_state == 1) {
        g_tick_counter++;
        if (g_tick_counter % 500 == 0) {
            watch_stack();
        }
        if (g_tick_counter >= SYS_TICK_MAX) {
            sys_oom_monitor_check();
            g_tick_counter = 0;    
            sys_verify_integrity();
        }
        
        char local_io_buffer[SYS_MAX_BUFFER];
        sys_memzero(local_io_buffer, SYS_MAX_BUFFER);
        
        if (g_env.is_root == 1) {
            print("\nroot@whitex~$ ");
        } else {
            print("\nwhitex~$ ");
        }
        
        scan(local_io_buffer);
        __asm__ volatile("cli");
        sys_sanitize_string(local_io_buffer, SYS_MAX_BUFFER);
        
        if (local_io_buffer[0] != '\0') {
            sys_parse_input(local_io_buffer);
        }
    }

    sys_panic("ERR_MAIN_LOOP_EXIT");
}

void execute_syscall(VMState *vm, uint64_t syscall_id) {
    sys_verify_integrity();
    if (g_env.memory_guard == 0 || g_env.isolation_active == 0) {
        switch (syscall_id) {
            case SYS_HELP:
                help();
                g_env.execution_cycles++;
                break;
            case SYS_RAM:
                ram();
                g_env.execution_cycles++;
                break;
            default:
                break;
        }
        return;
    }

    if (!vm || g_env.fault_detected) {
        sys_panic("ERR_SYSCALL_VIOLATION_INVALID_CONTEXT");
        return;
    }

    switch (syscall_id) {
        case SYS_HELP:
            help();
            g_env.execution_cycles++;
            break;
        case SYS_RAM:
            ram();
            g_env.execution_cycles++;
            break;
        default:
            sys_log_write(2, "INVALID_SYSCALL_EXECUTION_ATTEMPT");
            print("Invalid system call.\n");
            break;
    }
}

/*
 * WhiteX - i386 (32-bit) Stable Release
 * DEVELOPER BURAK YAKUB GÜÇER
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

#include "io.h"
#include "strcmp.h"
#include "../H/vga.h"
#include "../H/keyboard.h"
#include "logo.h"
#include "cls.h"
#include "help.h"
#include "uname.h"
#include "reboot.h"
#include "echo.h"
#include "fs.h"
#include "shutdown.h"
#include "hexdump.h"
#include "cpuid.h"
#include "notepad.h"
#include "Kisskrnl.h"
#include "clock.h"
#include "internet.h"
#include "fault.h"
#include "malloc.h"
#include "sdd.h"
#include "clk.h"
#include "fssdd.h"
#include "htop.h"
#include "melodi.h"
#include "idt.h"
#include "gdt.h"
#include "syscall.h"
#include "vm.h"
#include "esysc.h"

#define I386_CORE_MAGIC 0xC0DEBABE
#define I386_STACK_GUARD 0xDEADBEEF
#define I386_MAX_BUFFER 256
#define I386_CMD_MAX 64
#define I386_ARG_MAX 190

#pragma pack(push, 1)
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t eflags;
} i386_cpu_state_t;
#pragma pack(pop)

typedef void (*i386_routine_t)(void);
typedef void (*i386_args_routine_t)(char*);

typedef struct {
    const char *signature;
    i386_routine_t exec_ptr;
    i386_args_routine_t exec_arg_ptr;
    uint8_t requires_args;
} i386_dispatch_entry_t;

typedef struct {
    uint32_t system_state;
    uint32_t memory_guard;
    uint32_t execution_cycles;
    uint32_t fault_detected;
    i386_cpu_state_t last_known_state;
} i386_system_environment_t;

static i386_system_environment_t global_env;
static VMState kernel_vm;
static uint8_t vm_ram[0x10000];

static void safe_memzero_32(void *dest, size_t count) {
    volatile uint8_t *ptr = (volatile uint8_t *)dest;
    while (count--) {
        *ptr++ = 0;
    }
}

static void panic_handler_i386(const char *error_code) {
    __asm__ volatile("cli");
    global_env.fault_detected = 1;
    print("\n[i386 CRITICAL FAULT] SYSTEM HALTED.\n");
    print("CODE: ");
    print((char*)error_code);
    print("\n");
    while (1) {
        __asm__ volatile("hlt");
    }
}

static void verify_environment_integrity_i386(void) {
    if (global_env.memory_guard != I386_STACK_GUARD) {
        panic_handler_i386("ERR_STACK_SMASH");
    }
}

static void i386_env_bootstrap(void) {
    global_env.system_state = 1;
    global_env.memory_guard = I386_STACK_GUARD;
    global_env.execution_cycles = 0;
    global_env.fault_detected = 0;
    safe_memzero_32(&global_env.last_known_state, sizeof(i386_cpu_state_t));
}

static void safe_vm_init_i386(void) {
    VMState* local_vm = vm_create(0x10000);
    if (!local_vm) {
        print("SYS_WARN: Virtual Machine allocation failed.\n");
        return;
    }
    vm_run(local_vm);
    vm_destroy(local_vm);
}

static void cmd_echo_handler_i386(char *arg_buffer) {
    if (!arg_buffer || arg_buffer[0] == '\0') {
        print("\n");
        return;
    }
    echo(arg_buffer);
}

static const i386_dispatch_entry_t kernel_dispatch_table[] = {
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
    {"vm", safe_vm_init_i386, 0, 0},
    {"echo", 0, cmd_echo_handler_i386, 1}
};

#define DISPATCH_TABLE_SIZE (sizeof(kernel_dispatch_table) / sizeof(i386_dispatch_entry_t))

static void sanitize_string_i386(char *str, size_t max_len) {
    if (!str) return;
    for (size_t i = 0; i < max_len; i++) {
        if (str[i] == '\n' || str[i] == '\r') {
            str[i] = '\0';
            break;
        }
    }
}

static void execute_command_vector_i386(const char *cmd_name, char *args) {
    if (!cmd_name || cmd_name[0] == '\0') return;

    for (size_t i = 0; i < DISPATCH_TABLE_SIZE; i++) {
        if (strcmp((char*)cmd_name, (char*)kernel_dispatch_table[i].signature) == 0) {
            global_env.execution_cycles++;
            
            if (kernel_dispatch_table[i].requires_args) {
                if (kernel_dispatch_table[i].exec_arg_ptr) {
                    kernel_dispatch_table[i].exec_arg_ptr(args);
                }
            } else {
                if (kernel_dispatch_table[i].exec_ptr) {
                    kernel_dispatch_table[i].exec_ptr();
                }
            }
            return;
        }
    }

    print("WHITEX_CORE: Unknown vector -> ");
    print((char*)cmd_name);
    print("\n");
}

static void parse_input_buffer_i386(char *raw_buffer) {
    if (!raw_buffer || raw_buffer[0] == '\0' || raw_buffer[0] == ' ') return;

    char extracted_cmd[I386_CMD_MAX];
    char extracted_args[I386_ARG_MAX];
    
    safe_memzero_32(extracted_cmd, I386_CMD_MAX);
    safe_memzero_32(extracted_args, I386_ARG_MAX);

    uint32_t idx = 0;
    uint32_t cmd_idx = 0;
    
    while (raw_buffer[idx] != '\0' && raw_buffer[idx] != ' ' && cmd_idx < (I386_CMD_MAX - 1)) {
        extracted_cmd[cmd_idx++] = raw_buffer[idx++];
    }
    extracted_cmd[cmd_idx] = '\0';

    if (raw_buffer[idx] == ' ') {
        idx++;
        uint32_t arg_idx = 0;
        while (raw_buffer[idx] != '\0' && arg_idx < (I386_ARG_MAX - 1)) {
            extracted_args[arg_idx++] = raw_buffer[idx++];
        }
        extracted_args[arg_idx] = '\0';
    }

    execute_command_vector_i386(extracted_cmd, extracted_args);
}

void Kernel(void) {
    i386_env_bootstrap();

    init();
    init_gdt();
    init_idt();
    init_fs();
    pit_init();
    ram();
    Sdd();
    melodi();
    cpuid();

    char io_buffer[I386_MAX_BUFFER];
    logo();
    print("Type 'help' for available system routines.\n");

    while(global_env.system_state == 1) {
        verify_environment_integrity_i386();
        
        safe_memzero_32(io_buffer, I386_MAX_BUFFER);
        
        print("\nwhitex~$ ");
        
        
        
        scan(io_buffer);
        
        __asm__ volatile("cli");

        sanitize_string_i386(io_buffer, I386_MAX_BUFFER);
        
        if (io_buffer[0] != '\0') {
            parse_input_buffer_i386(io_buffer);
        }
    }

    panic_handler_i386("ERR_MAIN_LOOP_EXIT");
}

void execute_syscall(VMState *vm, uint64_t syscall_id) {
    if (!vm || global_env.fault_detected) {
        panic_handler_i386("ERR_SYSCALL_VIOLATION");
        return;
    }

    switch (syscall_id) {
        case SYS_HELP:
            help();
            global_env.execution_cycles++;
            break;
        case SYS_RAM:
            ram();
            global_env.execution_cycles++;
            break;
        default:
            print("WHITEX_CORE: Invalid i386 system call.\n");
            break;
    }
}
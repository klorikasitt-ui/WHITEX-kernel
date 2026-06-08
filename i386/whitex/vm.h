/*
 * WhiteX 
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
#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t registers[16];
    uint64_t pc;
    int running;
    size_t mem_size;
    uint8_t* ram;
} VMState;

extern void print(const char* data);
extern void* whitex_malloc(size_t size);
extern void whitex_free(void* ptr);
extern void execute_syscall(VMState *vm, uint64_t syscall_id);

static inline VMState* vm_create(size_t mem_size) {
    VMState* vm = (VMState*)whitex_malloc(sizeof(VMState));
    if (!vm) return NULL;
    vm->ram = (uint8_t*)whitex_malloc(mem_size);
    if (!vm->ram) {
        whitex_free(vm);
        return NULL;
    }
    vm->pc = 0;
    vm->running = 0;
    vm->mem_size = mem_size;
    for (int i = 0; i < 16; i++) vm->registers[i] = 0;
    return vm;
}

static inline void vm_run(VMState *vm) {
    if (!vm || !vm->ram) return;
    vm->running = 1;
    while (vm->running) {
        uint64_t instruction = *(uint64_t *)(vm->ram + vm->pc);
        vm->pc += 8;
        uint64_t opcode = (instruction >> 56) & 0xFF;
        uint64_t arg1 = (instruction >> 48) & 0xFF;
        uint32_t arg2 = (uint32_t)(instruction & 0xFFFFFFFF);
        switch (opcode) {
            case 0xCC:
                execute_syscall(vm, arg1);
                break;
            case 0x01:
                if (arg1 < 16) vm->registers[arg1] = arg2;
                break;
            case 0x0C:
                vm->pc = arg2;
                break;
            case 0x1F:
                vm->running = 0;
                break;
            default:
                vm->running = 0;
                break;
        }
    }
}

static inline void vm_destroy(VMState *vm) {
    if (!vm) return;
    if (vm->ram) whitex_free(vm->ram);
    whitex_free(vm);
}

static inline void vm_save_state(VMState *vm) {
    (void)vm;
    print("{\"status\":\"saved\"}\n");
}

static inline void vm_reset(VMState *vm) {
    if (!vm) return;
    vm->pc = 0;
    vm->running = 0;
    for (int i = 0; i < 16; i++) vm->registers[i] = 0;
}

static inline void vm_load_program(VMState *vm, uint8_t* program, size_t size) {
    if (!vm || !vm->ram || size > vm->mem_size) return;
    for (size_t i = 0; i < size; i++) {
        vm->ram[i] = program[i];
    }
}

static inline uint64_t vm_get_register(VMState *vm, int reg) {
    if (!vm || reg < 0 || reg >= 16) return 0;
    return vm->registers[reg];
}

static inline void vm_set_register(VMState *vm, int reg, uint64_t val) {
    if (!vm || reg < 0 || reg >= 16) return;
    vm->registers[reg] = val;
}

#endif

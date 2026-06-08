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
#ifndef ESYSC_H
#define ESYSC_H

extern void execute_syscall(VMState *vm, uint64_t syscall_id);
typedef struct {
    uint64_t code;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
} esyscall_packet_t;

static inline void esyscall_dispatch(VMState *vm, esyscall_packet_t *packet) {
    switch (packet->code) {
        case SYS_NOTEPAD:
            print("Notepad: ");
            print((char*)packet->arg0);
            print("\n");
            break;
            
        case SYS_CPUID:
            vm->registers[0] = 0x58544857; // 'WHTX'
            break;
            
        case SYS_MELODI:
            print("Playing system melody sequence...\n");
            break;
            
        case SYS_ANT:
            print("Ant-System initialized.\n");
            break;

        case SYS_INITFS:
            print("Filesystem mount request received.\n");
            break;

        default:
            execute_syscall(vm, packet->code);
            break;
    }
}

static inline void esyscall_trigger(VMState *vm, uint64_t id, uint64_t a, uint64_t b, uint64_t c) {
    esyscall_packet_t p = { .code = id, .arg0 = a, .arg1 = b, .arg2 = c };
    esyscall_dispatch(vm, &p);
}

#endif

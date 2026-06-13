section .multiboot
align 4
    dd 0x1BADB002
    dd 0x01
    dd -(0x1BADB002 + 0x01)

section .text
global _start
extern Kernel                 

_start:
    cli
    mov esp, stack_top
    
    push ebx
    push eax

    call Kernel

.halt:
    cli
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

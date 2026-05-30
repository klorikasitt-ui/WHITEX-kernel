
section .multiboot
align 4
    dd 0x1BADB002             ; Magic Number
    dd 0x01                   ; Flags (4KB)
    dd -(0x1BADB002 + 0x01)   ; Checksum

; --- Kod Başlangıcı ---
section .text
global _start
extern Kernel                 

_start:
    cli
    mov esp, stack_top

    
    ; EAX = 0x2BADB002 (Magic)

    
    call Kernel

   
.halt:
    hlt
    jmp .halt

; --- Stack Alanı ---
section .bss
align 16
stack_bottom:
    resb 16384                ; 16 KB 
stack_top:

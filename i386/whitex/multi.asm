[BITS 32]

global asm_switch_context
global asm_syscall_entry
global asm_read_cr0
global asm_write_cr0
global asm_read_cr2
global asm_write_cr2
global asm_read_cr3
global asm_write_cr3
global asm_read_cr4
global asm_write_cr4
global asm_invalidate_page
global asm_fast_memcpy
global asm_fast_memset
global asm_save_fpu_state
global asm_restore_fpu_state
global asm_get_stack_pointer
global asm_set_stack_pointer
global asm_enter_usermode
global asm_atomic_exchange

extern execute_syscall
extern scheduler_current_task
extern scheduler_next_task

section .text
align 16

asm_switch_context:
    push ebp
    mov ebp, esp
    pushf
    pusha
    mov eax, [ebp + 8]
    cmp eax, 0
    je .no_save
    mov [eax + 0], esp
    mov dword [eax + 4], .restore_point
.no_save:
    mov ebx, [ebp + 12]
    cmp ebx, 0
    je .halt_state
    mov esp, [ebx + 0]
    mov ecx, [ebx + 4]
    jmp ecx
.restore_point:
    popa
    popf
    pop ebp
    ret
.halt_state:
    cli
    hlt
    jmp .halt_state

align 16
asm_syscall_entry:
    cli
    push eax
    push ecx
    push edx
    push ebx
    push esp
    push ebp
    push esi
    push edi
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push eax
    push edx
    push ecx
    call execute_syscall
    add esp, 12
    pop gs
    pop fs
    pop es
    pop ds
    pop edi
    pop esi
    pop ebp
    pop esp
    pop ebx
    pop edx
    pop ecx
    pop eax
    sti
    sysexit

align 16
asm_read_cr0:
    mov eax, cr0
    ret

align 16
asm_write_cr0:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    mov cr0, eax
    pop ebp
    ret

align 16
asm_read_cr2:
    mov eax, cr2
    ret

align 16
asm_write_cr2:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    mov cr2, eax
    pop ebp
    ret

align 16
asm_read_cr3:
    mov eax, cr3
    ret

align 16
asm_write_cr3:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    mov cr3, eax
    pop ebp
    ret

align 16
asm_read_cr4:
    mov eax, cr4
    ret

align 16
asm_write_cr4:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    mov cr4, eax
    pop ebp
    ret

align 16
asm_invalidate_page:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    invlpg [eax]
    pop ebp
    ret

align 16
asm_fast_memcpy:
    push ebp
    mov ebp, esp
    push edi
    push esi
    mov edi, [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]
    mov edx, ecx
    shr ecx, 2
    cld
    rep movsd
    mov ecx, edx
    and ecx, 3
    rep movsb
    pop esi
    pop edi
    pop ebp
    ret

align 16
asm_fast_memset:
    push ebp
    mov ebp, esp
    push edi
    mov edi, [ebp + 8]
    mov eax, [ebp + 12]
    mov ecx, [ebp + 16]
    mov edx, ecx
    shr ecx, 2
    cld
    rep stosd
    mov ecx, edx
    and ecx, 3
    rep stosb
    pop edi
    pop ebp
    ret

align 16
asm_save_fpu_state:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    fnsave [eax]
    pop ebp
    ret

align 16
asm_restore_fpu_state:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    frstor [eax]
    pop ebp
    ret

align 16
asm_get_stack_pointer:
    mov eax, esp
    ret

align 16
asm_set_stack_pointer:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    mov esp, eax
    pop ebp
    ret

align 16
asm_enter_usermode:
    push ebp
    mov ebp, esp
    mov ebx, [ebp + 8]
    mov ecx, [ebp + 12]
    cli
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push 0x23
    push ecx
    pushf
    pop eax
    or eax, 0x200
    push eax
    push 0x1B
    push ebx
    iret

align 16
asm_atomic_exchange:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]
    mov eax, [ebp + 12]
    lock xchg [edx], eax
    pop ebp
    ret


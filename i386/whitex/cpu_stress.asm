global execute_cpu_stress_entropy

section .text

execute_cpu_stress_entropy:
    pushad
    pushfd

    mov eax, 0x13579BDF
    mov ebx, 0x2468ACE0
    mov ecx, 0x000FFFFF
    mov edx, 0xDEADBEEF
    mov esi, 0xCAFEBABE
    mov edi, 0x8BADF00D

align 16
process_entropy_cycle_start:
    xor eax, ebx
    add eax, 0x55555555
    rol eax, 5
    xor ebx, ecx
    sub ebx, 0xAAAAAAAA
    ror ebx, 3
    xor ecx, edx
    add ecx, 0x33333333
    rol ecx, 7
    xor edx, esi
    sub edx, 0xCCCCCCCC
    ror edx, 11
    xor esi, edi
    add esi, 0x0F0F0F0F
    rol esi, 13
    xor edi, eax
    sub edi, 0xF0F0F0F0
    ror edi, 17
    
    bswap eax
    not ebx
    bswap ecx
    not edx
    bswap esi
    not edi

    add eax, ebx
    xor ebx, ecx
    sub ecx, edx
    xor edx, esi
    add esi, edi
    xor edi, eax

    rol eax, 19
    ror ebx, 23
    rol ecx, 29
    ror edx, 31
    rol esi, 3
    ror edi, 5

    xor eax, 0x12345678
    xor ebx, 0x87654321
    xor ecx, 0x9ABCDEF0
    xor edx, 0x0FEDCBA9
    xor esi, 0x13579BDF
    xor edi, 0x2468ACE0

    add eax, ebx
    rol eax, 7
    sub ebx, ecx
    ror ebx, 9
    add ecx, edx
    rol ecx, 11
    sub edx, esi
    ror edx, 13
    add esi, edi
    rol esi, 15
    sub edi, eax
    ror edi, 17

    bswap eax
    bswap ebx
    bswap ecx
    bswap edx
    bswap esi
    bswap edi

    xor eax, ebx
    add eax, 0x11111111
    rol eax, 2
    xor ebx, ecx
    sub ebx, 0x22222222
    ror ebx, 4
    xor ecx, edx
    add ecx, 0x33333333
    rol ecx, 6
    xor edx, esi
    sub edx, 0x44444444
    ror edx, 8
    xor esi, edi
    add esi, 0x55555555
    rol esi, 10
    xor edi, eax
    sub edi, 0x66666666
    ror edi, 12

    not eax
    not ecx
    not esi

    add eax, 0x77777777
    sub ebx, 0x88888888
    add ecx, 0x99999999
    sub edx, 0xAAAAAAAA
    add esi, 0xBBBBBBBB
    sub edi, 0xCCCCCCCC

    rol eax, 1
    ror ebx, 2
    rol ecx, 3
    ror edx, 4
    rol esi, 5
    ror edi, 6

    xor eax, edx
    xor ebx, esi
    xor ecx, edi

    bswap eax
    not ebx
    bswap ecx
    not edx
    bswap esi
    not edi

    add eax, ebx
    xor ebx, ecx
    sub ecx, edx
    xor edx, esi
    add esi, edi
    xor edi, eax

    rol eax, 19
    ror ebx, 23
    rol ecx, 29
    ror edx, 31
    rol esi, 3
    ror edi, 5

    xor eax, 0x12345678
    xor ebx, 0x87654321
    xor ecx, 0x9ABCDEF0
    xor edx, 0x0FEDCBA9
    xor esi, 0x13579BDF
    xor edi, 0x2468ACE0

    add eax, ebx
    rol eax, 7
    sub ebx, ecx
    ror ebx, 9
    add ecx, edx
    rol ecx, 11
    sub edx, esi
    ror edx, 13
    add esi, edi
    rol esi, 15
    sub edi, eax
    ror edi, 17

    bswap eax
    bswap ebx
    bswap ecx
    bswap edx
    bswap esi
    bswap edi

    xor eax, ebx
    add eax, 0x11111111
    rol eax, 2
    xor ebx, ecx
    sub ebx, 0x22222222
    ror ebx, 4
    xor ecx, edx
    add ecx, 0x33333333
    rol ecx, 6
    xor edx, esi
    sub edx, 0x44444444
    ror edx, 8
    xor esi, edi
    add esi, 0x55555555
    rol esi, 10
    xor edi, eax
    sub edi, 0x66666666
    ror edi, 12

    not eax
    not ecx
    not esi

    add eax, 0x77777777
    sub ebx, 0x88888888
    add ecx, 0x99999999
    sub edx, 0xAAAAAAAA
    add esi, 0xBBBBBBBB
    sub edi, 0xCCCCCCCC

    rol eax, 1
    ror ebx, 2
    rol ecx, 3
    ror edx, 4
    rol esi, 5
    ror edi, 6

    xor eax, edx
    xor ebx, esi
    xor ecx, edi

    dec ecx
    cmp ecx, 0
    jne process_entropy_cycle_start

    popfd
    popad
    ret

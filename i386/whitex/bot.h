extern uint32_t asm_read_cr0(void);
extern uint32_t asm_read_cr3(void);
extern void asm_fast_memset(void* dest, uint32_t val, uint32_t count);
extern void asm_fast_memcpy(void* dest, const void* src, uint32_t count);
extern uint32_t asm_atomic_exchange(volatile uint32_t* addr, uint32_t val);

static void print_hex_32(uint32_t val) {
    char buf[9];
    char* hex = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[8] = '\0';
    print("0x");
    print(buf);
}

void bot(char* arg) {
    print("\n--- WHITEX ASM CORE TEST START ---\n");

    print("Testing CR0 Read... ");
    uint32_t cr0 = asm_read_cr0();
    print("CR0: ");
    print_hex_32(cr0);
    print("\n");

    print("Testing CR3 Read... ");
    uint32_t cr3 = asm_read_cr3();
    print("CR3 (Page Directory): ");
    print_hex_32(cr3);
    print("\n");

    print("Testing Fast Memset... ");
    uint32_t test_buffer[4];
    asm_fast_memset(test_buffer, 0xAA, sizeof(test_buffer));
    if (test_buffer[0] == 0xAAAAAAAA && test_buffer[3] == 0xAAAAAAAA) {
        print("[SUCCESS]\n");
    } else {
        print("[FAILED]\n");
    }

    print("Testing Fast Memcpy... ");
    uint32_t source_buffer[4] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
    uint32_t dest_buffer[4] = {0};
    asm_fast_memcpy(dest_buffer, source_buffer, sizeof(source_buffer));
    if (dest_buffer[1] == 0x22222222 && dest_buffer[3] == 0x44444444) {
        print("[SUCCESS]\n");
    } else {
        print("[FAILED]\n");
    }

    print("Testing Atomic Exchange... ");
    volatile uint32_t lock_val = 0x55555555;
    uint32_t old_val = asm_atomic_exchange(&lock_val, 0x99999999);
    if (old_val == 0x55555555 && lock_val == 0x99999999) {
        print("[SUCCESS]\n");
    } else {
        print("[FAILED]\n");
    }

    print("--- WHITEX ASM CORE TEST END ---\n");
}
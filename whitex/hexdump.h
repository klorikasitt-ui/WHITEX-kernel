void to_hex(unsigned int n, char* buf) {
    const char* hex_digits = "0123456789ABCDEF";
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 7; i >= 0; i--) {
        buf[i + 2] = hex_digits[(n >> ((7 - i) * 4)) & 0xF];
    }
    buf[10] = '\0';
}

void hexdump() {
    unsigned int _eax, _ebx, _esp;
    char hex_buf[11];

    asm volatile (
        "movl %%eax, %0\n\t"
        "movl %%ebx, %1\n\t"
        "movl %%esp, %2\n\t"
        : "=m"(_eax), "=m"(_ebx), "=m"(_esp)
        : 
        : "memory"
    );


    print("\n--- HEXDUMP---\n");
    print("STATUS CODE: ");
    print(hex_buf);
    print("\n");

   
    print("EAX: "); to_hex(_eax, hex_buf); print(hex_buf);
    print("\nEBX: "); to_hex(_ebx, hex_buf); print(hex_buf);
    print("\nESP: "); to_hex(_esp, hex_buf); print(hex_buf);
    print("\n--------------------\n");
    }


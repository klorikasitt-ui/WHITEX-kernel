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
        : "=r"(_eax), "=r"(_ebx), "=r"(_esp) 
        : 
        :
    );

    
    to_hex(_eax, hex_buf);
    print("EAX: "); print(hex_buf);
    print("\n");

    to_hex(_ebx, hex_buf);
    print("EBX: "); print(hex_buf);
    print("\n");

    to_hex(_esp, hex_buf);
    print("ESP: "); print(hex_buf);
    print("\n");
}


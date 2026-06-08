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

static inline unsigned long long get_cycles() {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}
void print_cycles() {
    char hex_buf[11];
    unsigned long long c = get_cycles();
    
    print("Cycle Count (High 32-bit): ");
    to_hex((unsigned int)(c >> 32), hex_buf);
    print(hex_buf); print("\n");

    print("Cycle Count (Low 32-bit):  ");
    to_hex((unsigned int)(c & 0xFFFFFFFF), hex_buf);
    print(hex_buf); print("\n");
}


int has_sse = 0;
int has_sse2 = 0;
int has_sep = 0;

void cpuid() {
    unsigned int eax, ebx, ecx, edx;
    char vendor[13];

  
    __asm__ __volatile__ ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    *((unsigned int*)(vendor))     = ebx;
    *((unsigned int*)(vendor + 4)) = edx;
    *((unsigned int*)(vendor + 8)) = ecx;
    vendor[12] = '\0';
    print("Vendor: "); print(vendor); print("\n");

   
    __asm__ __volatile__ ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));

    
    has_sse  = (edx & (1 << 25));
    has_sse2 = (edx & (1 << 26));
    has_sep  = (edx & (1 << 11));

   
    if (edx & (1 << 0))  print("FPU: yes\n");
    if (has_sep)         print("SEP: yes\n");
    if (has_sse)         print("SSE: yes\n");
    if (has_sse2)        print("SSE2: yes\n");
    if (ecx & (1 << 0))  print("SSE3: yes\n");
   

   
    if (has_sse) {
        __asm__ __volatile__ (
            "mov %%cr4, %%eax\n\t"
            "orl $0x200, %%eax\n\t" 
            "mov %%eax, %%cr4"
            ::: "eax"
        );
        
    }

print_cycles();

}


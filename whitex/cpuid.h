void cpuid() {
    unsigned int eax, ebx, ecx, edx;
    char vendor[13]; 

    __asm__ __volatile__ (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0) 
    );
    *((unsigned int*)(vendor))     = ebx;
    *((unsigned int*)(vendor + 4)) = edx;
    *((unsigned int*)(vendor + 8)) = ecx;
    vendor[12] = '\0'; 
    print(vendor);
    print("\n");
}

#ifndef CPUCRITICAL_H
#define CPUCRITICAL_H

#include <stdint.h>
#include "sys_tools.h"

#define MSR_IA32_TEMPERATURE_TARGET 0x1A2
#define MSR_IA32_PERF_STATUS        0x198
#define MSR_IA32_PERF_CTL           0x199

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi)); // RDTSC yerine RDMSR olmalı
    __asm__ __volatile__ ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    __asm__ __volatile__ ("wrmsr" : : "c"(msr), "a"((uint32_t)val), "d"((uint32_t)(val >> 32)));
}

static inline void get_thermal_data() {
    uint64_t target = rdmsr(MSR_IA32_TEMPERATURE_TARGET);
    uint32_t tcc_offset = (target >> 24) & 0xFF;
    
    uint64_t perf_status = rdmsr(MSR_IA32_PERF_STATUS);
    uint32_t current_vid = (perf_status >> 0) & 0x3F;

    print("TCC Activation Offset: "); 
    print_int(tcc_offset); 
    print("\n");
    
    print("Voltage ID (VID):      "); 
    print_int(current_vid); 
    print("\n");
}

static inline void apply_undervoltage() {
    wrmsr(MSR_IA32_PERF_CTL, 0x0800); 
    print("Voltage and Clock Dropped successfully.\n");
}

static inline void run_thermal_governor() {
    static uint64_t last_check = 0;
    const uint64_t INTERVAL = 60000000000ULL; 
    uint64_t current = get_cycles();

    if ((current - last_check) > INTERVAL) {
        last_check = current;
        
        uint64_t target = rdmsr(MSR_IA32_TEMPERATURE_TARGET);
        uint32_t tcc_offset = (target >> 24) & 0xFF;

        print("Governor: Monitoring thermal headroom...\n");
        if (tcc_offset < 10) {
            print("Governor: Thermal limit reached! Protection active.\n");
            apply_undervoltage();
        } else {
            print("Governor: Thermal status safe.\n");
        }
    }
}
#define MSR_PKG_ENERGY_STATUS 0x611

static inline uint32_t get_energy_raw() {
    return (uint32_t)(rdmsr(MSR_PKG_ENERGY_STATUS) & 0xFFFFFFFF);
}

static inline void print_power_watt() {
    static uint32_t last_energy = 0;
    static uint64_t last_time = 0;
    
    uint32_t current_energy = get_energy_raw();
    uint64_t current_time = get_cycles();
    
    if (last_energy != 0) {
        
        uint32_t energy_diff = current_energy - last_energy;
        uint64_t time_diff = current_time - last_time;
        
        print("Power Usage (Relative Units): ");
        print_int(energy_diff / 1000); 
        print(" units\n");
    }
    
    last_energy = current_energy;
    last_time = current_time;
}
 static inline void print_watt() {
    uint32_t raw_energy = get_energy_raw(); // Ham değeri oku
    
    print("Raw Energy Data: ");
    print_int(raw_energy); 
    print("\n");
}



#endif

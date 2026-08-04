// power.h

#ifndef POWER_H
#define POWER_H

extern void sys_print_int(int n);

#include <stdint.h>

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
    return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t low = val & 0xFFFFFFFF;
    uint32_t high = val >> 32;
    __asm__ volatile ("wrmsr" : : "c" (msr), "a" (low), "d" (high));
}

#define MSR_PLATFORM_INFO     0xCE
#define MSR_PKG_ENERGY_STATUS 0x611
#define MSR_RAPL_POWER_UNIT   0x606
#define MSR_PKG_POWER_LIMIT   0x610
#define MSR_FEATURE_CONFIG    0x13C

int is_run_on_battery(void) {
    uint64_t info = rdmsr(MSR_PLATFORM_INFO);
    return (info & (1ULL << 33)) != 0;
}

double get_energy_units(void) {
    uint64_t units = rdmsr(MSR_RAPL_POWER_UNIT);
    uint32_t energy_unit = (units >> 8) & 0x1F;
    return 1.0 / (1ULL << energy_unit);
}

double get_max_energy(void) {
    uint64_t limit = rdmsr(MSR_PKG_POWER_LIMIT);
    uint32_t max_range = (limit >> 32) & 0x1F;
    if (max_range == 0) return 0.0;
    return (double)(1ULL << max_range) * get_energy_units();
}

void batt() {
    if (!is_run_on_battery()) {
        return;
    }

    double energy_unit = get_energy_units();
    uint64_t raw_energy = rdmsr(MSR_PKG_ENERGY_STATUS);
    double current_energy = (double)(raw_energy & 0xFFFFFFFF) * energy_unit;
    double max_energy = get_max_energy();

    int percentage = 0;
    if (max_energy > 0.0) {
        double remaining = max_energy - current_energy;
        if (remaining < 0.0) remaining = 0.0;
        percentage = (int)((remaining / max_energy) * 100.0);
        if (percentage > 100) percentage = 100;
    }

    sys_print_int(percentage);

    if (percentage >= 100) {
        uint64_t val = rdmsr(MSR_FEATURE_CONFIG);
        val |= (1ULL << 0);
        wrmsr(MSR_FEATURE_CONFIG, val);
    }
}

#endif

#include <stdint.h>
#include <stddef.h>

#define MMIO_READ8(addr)     (*(volatile uint8_t *)(addr))
#define MMIO_READ16(addr)    (*(volatile uint16_t *)(addr))
#define MMIO_READ32(addr)    (*(volatile uint32_t *)(addr))
#define MMIO_READ64(addr)    (*(volatile uint64_t *)(addr))

#define MMIO_WRITE8(addr, val)   (*(volatile uint8_t *)(addr) = (val))
#define MMIO_WRITE16(addr, val)  (*(volatile uint16_t *)(addr) = (val))
#define MMIO_WRITE32(addr, val)  (*(volatile uint32_t *)(addr) = (val))
#define MMIO_WRITE64(addr, val)  (*(volatile uint64_t *)(addr) = (val))

typedef struct {
    uintptr_t base_phys;
    uintptr_t base_virt;
    size_t size;
    int flags;
} mmio_region_t;

static inline uint8_t mmio_read8(uintptr_t addr) {
    return MMIO_READ8(addr);
}

static inline void mmio_write8(uintptr_t addr, uint8_t value) {
    MMIO_WRITE8(addr, value);
}

static inline uint16_t mmio_read16(uintptr_t addr) {
    return MMIO_READ16(addr);
}

static inline void mmio_write16(uintptr_t addr, uint16_t value) {
    MMIO_WRITE16(addr, value);
}

static inline uint64_t mmio_read64(uintptr_t addr) {
    return MMIO_READ64(addr);
}

static inline void mmio_write64(uintptr_t addr, uint64_t value) {
    MMIO_WRITE64(addr, value);
}

int mmio_map_region(mmio_region_t *region, uintptr_t phys, uintptr_t virt, size_t size, int flags) {
    if (!region) {
        return -1;
    }

    region->base_phys = phys;
    region->base_virt = virt;
    region->size = size;
    region->flags = flags;

    return 0;
}

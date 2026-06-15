typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef struct registers registers_t;
typedef unsigned char uint8_t;
typedef struct registers registers_t;
typedef void (*isr_t)(registers_t*);

#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024
#define DIRECTORY_ENTRIES 1024
#define PHYSICAL_MEMORY_LIMIT 0x100000000ULL
#define BITMAP_SIZE (PHYSICAL_MEMORY_LIMIT / PAGE_SIZE / 32)

static inline void register_interrupt_handler(uint8_t n, void (*handler)(void*)) {
    static void** interrupt_handlers = (void**)0; 
    interrupt_handlers[n] = (void*)handler;
}


typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t unused     : 7;
    uint32_t frame      : 20;
} page_table_entry_t;

typedef struct {
    page_table_entry_t entries[PAGE_ENTRIES];
} page_table_t;

typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t write_thru : 1;
    uint32_t cache_dis  : 1;
    uint32_t accessed   : 1;
    uint32_t zero       : 1;
    uint32_t size       : 1;
    uint32_t ignored    : 4;
    uint32_t frame      : 20;
} page_directory_entry_t;

typedef struct {
    page_table_t* tables[DIRECTORY_ENTRIES];
    uint32_t physical_tables[DIRECTORY_ENTRIES];
    uint32_t physical_address;
} page_directory_t;

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} interrupt_registers_t;

extern void asm_write_cr3(uint32_t physical_addr);
extern uint32_t asm_read_cr3(void);
extern void asm_write_cr0(uint32_t cr0);
extern uint32_t asm_read_cr0(void);
extern uint32_t asm_read_cr2(void);
extern void asm_invalidate_page(uint32_t addr);
extern void panic_handler_i386(const char* msg);

static uint32_t physical_memory_bitmap[BITMAP_SIZE];
static uint32_t total_allocated_frames = 0;
static uint32_t total_system_frames = 0;
page_directory_t* current_page_directory = 0;
page_directory_t* kernel_page_directory = 0;

static void memzero_custom(void* dest, size_t len) {
    uint8_t* d = (uint8_t*)dest;
    while (len--) {
        *d++ = 0;
    }
}

static void memcpy_custom(void* dest, const void* src, size_t len) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (len--) {
        *d++ = *s++;
    }
}

static void bitmap_set_bit(uint32_t frame_address) {
    uint32_t frame = frame_address / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    physical_memory_bitmap[idx] |= (1 << off);
}

static void bitmap_clear_bit(uint32_t frame_address) {
    uint32_t frame = frame_address / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    physical_memory_bitmap[idx] &= ~(1 << off);
}

static uint32_t bitmap_test_bit(uint32_t frame_address) {
    uint32_t frame = frame_address / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    return (physical_memory_bitmap[idx] & (1 << off));
}

static uint32_t find_first_free_frame(void) {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (physical_memory_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                if (!(physical_memory_bitmap[i] & (1 << j))) {
                    return (i * 32 + j) * PAGE_SIZE;
                }
            }
        }
    }
    return 0xFFFFFFFF;
}

static uint32_t find_contiguous_free_frames(uint32_t count) {
    if (count == 0) return 0xFFFFFFFF;
    if (count == 1) return find_first_free_frame();

    uint32_t start_frame = 0;
    uint32_t free_count = 0;

    for (uint32_t i = 0; i < BITMAP_SIZE * 32; i++) {
        if (!bitmap_test_bit(i * PAGE_SIZE)) {
            if (free_count == 0) {
                start_frame = i;
            }
            free_count++;
            if (free_count == count) {
                return start_frame * PAGE_SIZE;
            }
        } else {
            free_count = 0;
        }
    }
    return 0xFFFFFFFF;
}

static void allocate_frame(page_table_entry_t* page, int is_kernel, int is_writable) {
    if (page->frame != 0) {
        return;
    }
    uint32_t frame_addr = find_first_free_frame();
    if (frame_addr == 0xFFFFFFFF) {
        panic_handler_i386("SYS_ERR_OUT_OF_PHYSICAL_MEMORY");
    }
    bitmap_set_bit(frame_addr);
    page->present = 1;
    page->rw = (is_writable == 1) ? 1 : 0;
    page->user = (is_kernel == 1) ? 0 : 1;
    page->frame = frame_addr / PAGE_SIZE;
    total_allocated_frames++;
}

static void free_frame(page_table_entry_t* page) {
    uint32_t frame_addr;
    if (!(frame_addr = page->frame)) {
        return;
    }
    bitmap_clear_bit(frame_addr * PAGE_SIZE);
    page->frame = 0;
    page->present = 0;
    total_allocated_frames--;
}

page_table_entry_t* get_page_entry(uint32_t address, int make, page_directory_t* dir) {
    address /= PAGE_SIZE;
    uint32_t table_idx = address / PAGE_ENTRIES;
    uint32_t page_idx = address % PAGE_ENTRIES;

    if (dir->tables[table_idx]) {
        return &dir->tables[table_idx]->entries[page_idx];
    } else if (make) {
        uint32_t physical_addr;
        dir->tables[table_idx] = (page_table_t*)find_first_free_frame();
        if (dir->tables[table_idx] == (page_table_t*)0xFFFFFFFF) {
            panic_handler_i386("SYS_ERR_PAGE_TABLE_ALLOCATION_FAILED");
        }
        physical_addr = (uint32_t)dir->tables[table_idx];
        bitmap_set_bit(physical_addr);
        memzero_custom(dir->tables[table_idx], sizeof(page_table_t));
        dir->physical_tables[table_idx] = physical_addr | 0x07; 
        return &dir->tables[table_idx]->entries[page_idx];
    }
    return 0;
}

void map_virtual_to_physical(uint32_t virtual_addr, uint32_t physical_addr, int is_kernel, int is_writable, page_directory_t* dir) {
    page_table_entry_t* page = get_page_entry(virtual_addr, 1, dir);
    if (!page) {
        panic_handler_i386("SYS_ERR_MAP_VIRTUAL_FAILED");
    }
    page->present = 1;
    page->rw = is_writable ? 1 : 0;
    page->user = is_kernel ? 0 : 1;
    page->frame = physical_addr / PAGE_SIZE;
    bitmap_set_bit(physical_addr);
}

void unmap_virtual_page(uint32_t virtual_addr, page_directory_t* dir) {
    page_table_entry_t* page = get_page_entry(virtual_addr, 0, dir);
    if (page && page->present) {
        free_frame(page);
        page->present = 0;
        asm_invalidate_page(virtual_addr);
    }
}

uint32_t get_physical_address(uint32_t virtual_addr, page_directory_t* dir) {
    page_table_entry_t* page = get_page_entry(virtual_addr, 0, dir);
    if (page && page->present) {
        return (page->frame * PAGE_SIZE) + (virtual_addr & 0xFFF);
    }
    return 0xFFFFFFFF;
}

void switch_page_directory(page_directory_t* dir) {
    current_page_directory = dir;
    asm_write_cr3(dir->physical_address);
    uint32_t cr0 = asm_read_cr0();
    cr0 |= 0x80000000;
    asm_write_cr0(cr0);
}

void page_fault_handler(interrupt_registers_t* regs) {
    uint32_t faulting_address = asm_read_cr2();
    int present = !(regs->err_code & 0x1);
    int rw = regs->err_code & 0x2;
    int us = regs->err_code & 0x4;
    int reserved = regs->err_code & 0x8;
    int id = regs->err_code & 0x10;

    if (present || reserved || id) {
        panic_handler_i386("SYS_ERR_UNRECOVERABLE_PAGE_FAULT");
    }

    page_table_entry_t* page = get_page_entry(faulting_address, 1, current_page_directory);
    if (!page->present) {
        allocate_frame(page, us ? 0 : 1, rw ? 1 : 0);
    } else {
        panic_handler_i386("SYS_ERR_PROTECTION_FAULT");
    }
}

page_table_t* clone_page_table(page_table_t* src, uint32_t* physical_addr) {
    page_table_t* new_table = (page_table_t*)find_first_free_frame();
    if (new_table == (page_table_t*)0xFFFFFFFF) return 0;
    
    *physical_addr = (uint32_t)new_table;
    bitmap_set_bit(*physical_addr);
    memzero_custom(new_table, sizeof(page_table_t));

    for (int i = 0; i < PAGE_ENTRIES; i++) {
        if (src->entries[i].present) {
            allocate_frame(&new_table->entries[i], 0, 0);
            new_table->entries[i].present = src->entries[i].present;
            new_table->entries[i].rw = src->entries[i].rw;
            new_table->entries[i].user = src->entries[i].user;
            new_table->entries[i].accessed = src->entries[i].accessed;
            new_table->entries[i].dirty = src->entries[i].dirty;
            
            uint32_t src_phys = src->entries[i].frame * PAGE_SIZE;
            uint32_t dest_phys = new_table->entries[i].frame * PAGE_SIZE;
            
            memcpy_custom((void*)dest_phys, (void*)src_phys, PAGE_SIZE);
        }
    }
    return new_table;
}

page_directory_t* clone_page_directory(page_directory_t* src) {
    page_directory_t* new_dir = (page_directory_t*)find_first_free_frame();
    if (new_dir == (page_directory_t*)0xFFFFFFFF) return 0;
    
    bitmap_set_bit((uint32_t)new_dir);
    memzero_custom(new_dir, sizeof(page_directory_t));
    uint32_t phys_offset = (uint32_t)new_dir->physical_tables - (uint32_t)new_dir;
    new_dir->physical_address = (uint32_t)new_dir + phys_offset;

    for (int i = 0; i < DIRECTORY_ENTRIES; i++) {
        if (!src->tables[i]) continue;

        if (kernel_page_directory->tables[i] == src->tables[i]) {
            new_dir->tables[i] = src->tables[i];
            new_dir->physical_tables[i] = src->physical_tables[i];
        } else {
            uint32_t phys;
            new_dir->tables[i] = clone_page_table(src->tables[i], &phys);
            new_dir->physical_tables[i] = phys | 0x07;
        }
    }
    return new_dir;
}

void allocate_memory_region(uint32_t start_addr, uint32_t size, int is_kernel, int is_writable, page_directory_t* dir) {
    uint32_t aligned_start = start_addr & ~(PAGE_SIZE - 1);
    uint32_t aligned_end = (start_addr + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    for (uint32_t current = aligned_start; current < aligned_end; current += PAGE_SIZE) {
        page_table_entry_t* page = get_page_entry(current, 1, dir);
        allocate_frame(page, is_kernel, is_writable);
    }
}

void free_memory_region(uint32_t start_addr, uint32_t size, page_directory_t* dir) {
    uint32_t aligned_start = start_addr & ~(PAGE_SIZE - 1);
    uint32_t aligned_end = (start_addr + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    for (uint32_t current = aligned_start; current < aligned_end; current += PAGE_SIZE) {
        unmap_virtual_page(current, dir);
    }
}

uint32_t check_region_availability(uint32_t start_addr, uint32_t size, page_directory_t* dir) {
    uint32_t aligned_start = start_addr & ~(PAGE_SIZE - 1);
    uint32_t aligned_end = (start_addr + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    for (uint32_t current = aligned_start; current < aligned_end; current += PAGE_SIZE) {
        page_table_entry_t* page = get_page_entry(current, 0, dir);
        if (page && page->present) {
            return 0;
        }
    }
    return 1;
}

void identity_map_region(uint32_t start_addr, uint32_t end_addr, page_directory_t* dir) {
    uint32_t aligned_start = start_addr & ~(PAGE_SIZE - 1);
    uint32_t aligned_end = (end_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    for (uint32_t current = aligned_start; current < aligned_end; current += PAGE_SIZE) {
        map_virtual_to_physical(current, current, 1, 1, dir);
    }
}

void flush_tlb(void) {
    uint32_t cr3_val = asm_read_cr3();
    asm_write_cr3(cr3_val);
}

void initialize_paging_system(uint32_t total_memory_kb) {
    total_system_frames = (total_memory_kb * 1024) / PAGE_SIZE;
    memzero_custom(physical_memory_bitmap, sizeof(physical_memory_bitmap));
    
    kernel_page_directory = (page_directory_t*)find_first_free_frame();
    if (kernel_page_directory == (page_directory_t*)0xFFFFFFFF) {
        panic_handler_i386("SYS_ERR_KERNEL_DIR_ALLOC_FAILED");
    }
    bitmap_set_bit((uint32_t)kernel_page_directory);
    memzero_custom(kernel_page_directory, sizeof(page_directory_t));
    
    uint32_t phys_offset = (uint32_t)kernel_page_directory->physical_tables - (uint32_t)kernel_page_directory;
    kernel_page_directory->physical_address = (uint32_t)kernel_page_directory + phys_offset;

    uint32_t kernel_end_ptr = 0x400000;
    for (uint32_t i = 0; i < kernel_end_ptr; i += PAGE_SIZE) {
        map_virtual_to_physical(i, i, 1, 1, kernel_page_directory);
    }
    
   register_interrupt_handler(14, (void (*)(void*))page_fault_handler);   //register_interrupt_handler(14, page_fault_handler);
    switch_page_directory(kernel_page_directory);
}

uint32_t get_total_allocated_memory(void) {
    return total_allocated_frames * PAGE_SIZE;
}

uint32_t get_total_free_memory(void) {
    return (total_system_frames - total_allocated_frames) * PAGE_SIZE;
}

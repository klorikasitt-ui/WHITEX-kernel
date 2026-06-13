#ifndef _WHITEX_IDT_FIX_H
#define _WHITEX_IDT_FIX_H

typedef _Bool bool;
#define true 1
#define false 0

#ifndef __initdata
#define __initdata
#endif
#ifndef __initconst
#define __initconst
#endif
#ifndef __page_aligned_bss
#define __page_aligned_bss
#endif
#ifndef __ro_after_init
#define __ro_after_init
#endif
#ifndef __init
#define __init
#endif

#define GATE_INTERRUPT  0xE
#define GATE_TASK       0x5
#define __KERNEL_CS     0x08
#define IDT_ENTRIES     256
#define NULL            ((void *)0)

#define CONFIG_X86_32   1

#define GDT_ENTRY_DOUBLEFAULT_TSS 3
#define IST_INDEX_NMI             1

#define IA32_SYSCALL_VECTOR               0x80
#define RESCHEDULE_VECTOR                 0xFD
#define CALL_FUNCTION_VECTOR              0xFC
#define CALL_FUNCTION_SINGLE_VECTOR       0xFB
#define REBOOT_VECTOR                     0xFA
#define THERMAL_APIC_VECTOR               0xF9
#define THRESHOLD_APIC_VECTOR             0xF8
#define DEFERRED_ERROR_VECTOR             0xF7
#define LOCAL_TIMER_VECTOR                0xEF
#define X86_PLATFORM_IPI_VECTOR           0xEE
#define POSTED_INTR_VECTOR                0xED
#define POSTED_INTR_WAKEUP_VECTOR         0xEC
#define POSTED_INTR_NESTED_VECTOR         0xEB
#define PERF_GUEST_MEDIATED_PMI_VECTOR    0xEA
#define IRQ_WORK_VECTOR                   0xF6
#define SPURIOUS_APIC_VECTOR              0xFF
#define ERROR_APIC_VECTOR                 0xFE
#define POSTED_MSI_NOTIFICATION_VECTOR    0xE9

#define X86_TRAP_DE         0
#define X86_TRAP_DB         1
#define X86_TRAP_NMI        2
#define X86_TRAP_BP         3
#define X86_TRAP_OF         4
#define X86_TRAP_BR         5
#define X86_TRAP_UD         6
#define X86_TRAP_NM         7
#define X86_TRAP_DF         8
#define X86_TRAP_OLD_MF     9
#define X86_TRAP_TS         10
#define X86_TRAP_NP         11
#define X86_TRAP_SS         12
#define X86_TRAP_GP         13
#define X86_TRAP_PF         14
#define X86_TRAP_SPURIOUS   15
#define X86_TRAP_MF         16
#define X86_TRAP_AC         17
#define X86_TRAP_MC         18
#define X86_TRAP_XF         19
#define X86_TRAP_VE         20
#define X86_TRAP_CP         21
#define X86_TRAP_VC         22

struct idt_bits {
    unsigned short ist  : 3,
                   zero : 5,
                   type : 4,
                   dpl  : 2,
                   p    : 1;
} __attribute__((packed));

struct idt_data {
    unsigned int vector;
    void (*addr)(void);
    unsigned int segment;
    struct idt_bits bits;
};

struct gate_struct {
    unsigned short offset_low;
    unsigned short segment;
    struct idt_bits bits;
    unsigned short offset_high;
} __attribute__((packed));

typedef struct gate_struct gate_desc;

struct desc_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

unsigned long system_vectors[8];

#define DEFINE_REAL_EXC(name) \
    __attribute__((naked)) static inline void name(void) { \
        __asm__ volatile("pushad\n\tpopad\n\tiretd\n\t"); \
    }

#define DEFINE_REAL_EXC_ERR(name) \
    __attribute__((naked)) static inline void name(void) { \
        __asm__ volatile("pushad\n\tpopad\n\tadd $4, %%esp\n\tiretd\n\t"); \
    }

DEFINE_REAL_EXC(asm_exc_divide_error)
DEFINE_REAL_EXC(asm_exc_debug)
DEFINE_REAL_EXC(asm_exc_nmi)
DEFINE_REAL_EXC(asm_exc_int3)
DEFINE_REAL_EXC(asm_exc_overflow)
DEFINE_REAL_EXC(asm_exc_bounds)
DEFINE_REAL_EXC(asm_exc_invalid_op)
DEFINE_REAL_EXC(asm_exc_device_not_available)
DEFINE_REAL_EXC_ERR(asm_exc_double_fault)
DEFINE_REAL_EXC(asm_exc_coproc_segment_overrun)
DEFINE_REAL_EXC_ERR(asm_exc_invalid_tss)
DEFINE_REAL_EXC_ERR(asm_exc_segment_not_present)
DEFINE_REAL_EXC_ERR(asm_exc_stack_segment)
DEFINE_REAL_EXC_ERR(asm_exc_general_protection)
DEFINE_REAL_EXC_ERR(asm_exc_page_fault)
DEFINE_REAL_EXC(asm_exc_spurious_interrupt_bug)
DEFINE_REAL_EXC(asm_exc_coprocessor_error)
DEFINE_REAL_EXC_ERR(asm_exc_alignment_check)
DEFINE_REAL_EXC(asm_exc_machine_check)
DEFINE_REAL_EXC(asm_exc_simd_coprocessor_error)
DEFINE_REAL_EXC(asm_exc_virtualization_exception)
DEFINE_REAL_EXC(asm_exc_control_protection)
DEFINE_REAL_EXC(asm_exc_vmm_communication)
DEFINE_REAL_EXC(entry_INT80_32)
DEFINE_REAL_EXC(asm_int80_emulation)

DEFINE_REAL_EXC(asm_sysvec_reschedule_ipi)
DEFINE_REAL_EXC(asm_sysvec_call_function)
DEFINE_REAL_EXC(asm_sysvec_call_function_single)
DEFINE_REAL_EXC(asm_sysvec_reboot)
DEFINE_REAL_EXC(asm_sysvec_thermal)
DEFINE_REAL_EXC(asm_sysvec_threshold)
DEFINE_REAL_EXC(asm_sysvec_deferred_error)
DEFINE_REAL_EXC(asm_sysvec_apic_timer_interrupt)
DEFINE_REAL_EXC(asm_sysvec_x86_platform_ipi)
DEFINE_REAL_EXC(asm_sysvec_kvm_posted_intr_ipi)
DEFINE_REAL_EXC(asm_sysvec_kvm_posted_intr_wakeup_ipi)
DEFINE_REAL_EXC(asm_sysvec_kvm_posted_intr_nested_ipi)
DEFINE_REAL_EXC(asm_sysvec_perf_guest_mediated_pmi_handler)
DEFINE_REAL_EXC(asm_sysvec_irq_work)
DEFINE_REAL_EXC(asm_sysvec_spurious_apic_interrupt)
DEFINE_REAL_EXC(asm_sysvec_error_interrupt)
DEFINE_REAL_EXC(asm_sysvec_posted_msi_notification)

static inline void load_idt(const struct desc_ptr *dtr) {
    __asm__ volatile("lidt %0" : : "m"(*dtr));
}

static inline void lockdep_assert_irqs_disabled(void) {}

static inline void set_bit(unsigned int nr, unsigned long *addr) {
    addr[nr / 32] |= (1UL << (nr % 32));
}

static inline void idt_init_desc(gate_desc *desc, const struct idt_data *t) {
    unsigned int addr = (unsigned int)t->addr;
    desc->offset_low = (unsigned short)(addr & 0xFFFF);
    desc->segment = (unsigned short)t->segment;
    desc->bits = t->bits;
    desc->offset_high = (unsigned short)((addr >> 16) & 0xFFFF);
}

static inline void write_idt_entry(gate_desc *idt, int entry, const gate_desc *desc) {
    idt[entry] = *desc;
}

static inline void init_idt_data(struct idt_data *data, unsigned int n, const void *addr) {
    data->vector = n;
    data->addr = (void(*)(void))addr;
    data->segment = __KERNEL_CS;
    data->bits.ist = 0;
    data->bits.zero = 0;
    data->bits.type = GATE_INTERRUPT;
    data->bits.dpl = 0;
    data->bits.p = 1;
}
typedef gate_desc idt_entry_t;
typedef struct desc_ptr idt_ptr_t;

typedef gate_desc idt_entry_t;
typedef struct desc_ptr idt_ptr_t;

extern idt_entry_t idt[256];
typedef gate_desc idt_entry_t;
typedef struct desc_ptr idt_ptr_t;

extern idt_entry_t idt[];

static inline void idt_set_gate(int vector, unsigned int handler_address, unsigned short selector, unsigned char attributes) {
    idt_entry_t desc;
    desc.offset_low = (unsigned short)(handler_address & 0xFFFF);
    desc.segment = selector;
    desc.bits.ist = 0;
    desc.bits.zero = 0;
    desc.bits.type = attributes & 0x0F;
    desc.bits.dpl = (attributes >> 5) & 0x03;
    desc.bits.p = (attributes >> 7) & 0x01;
    desc.offset_high = (unsigned short)((handler_address >> 16) & 0xFFFF);
    idt[vector] = desc;
}

__attribute__((naked)) static inline void default_handler(void) {
#ifdef __x86_64__
    __asm__ volatile(
        "push %%rax\n\t"
        "push %%rcx\n\t"
        "push %%rdx\n\t"
        "pop %%rdx\n\t"
        "pop %%rcx\n\t"
        "pop %%rax\n\t"
        "iretq\n\t"
        : : : "memory"
    );
#else
    __asm__ volatile(
        "pushl %%eax\n\t"
        "pushl %%ecx\n\t"
        "pushl %%edx\n\t"
        "popl %%edx\n\t"
        "popl %%ecx\n\t"
        "popl %%eax\n\t"
        "iret\n\t"
        : : : "memory"
    );
#endif
}



#endif

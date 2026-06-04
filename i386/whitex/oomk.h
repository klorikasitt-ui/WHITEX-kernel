#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdbool.h>

#define SYS_MAX_TASKS             1024
#define PAGE_SIZE                 4096
#define OOM_SCORE_ADJ_MIN         -1000
#define OOM_SCORE_ADJ_MAX         1000
#define OOM_SCORE_MAX             10000

#define ZONE_DMA                  0
#define ZONE_NORMAL               1
#define ZONE_HIGHMEM              2
#define MAX_NR_ZONES              3

#define WM_MIN                    0
#define WM_LOW                    1
#define WM_HIGH                   2

#define TASK_STATE_RUNNING        0x01
#define TASK_STATE_SLEEPING       0x02
#define TASK_STATE_ZOMBIE         0x04
#define TASK_STATE_DEAD           0x08

#define OOM_FLAG_CRITICAL         (1 << 0)
#define OOM_FLAG_UNKILLABLE       (1 << 1)
#define OOM_FLAG_RECLAIMING       (1 << 2)

#define REGS_BASE                 0x50000000

typedef struct {
    volatile uint32_t PANIC_CTRL;
    volatile uint32_t RECLAIM_TRIG;
    volatile uint32_t KILL_SIG_VEC;
    volatile uint32_t OOM_VICTIM_PID;
} hw_oom_regs_t;

#define OOM_HW_REGS ((hw_oom_regs_t*)REGS_BASE)

typedef struct {
    uint32_t pid;
    uint32_t ppid;
    uint32_t uid;
    uint8_t state;
    uint32_t flags;
    int16_t oom_score_adj;
    size_t rss_pages;
    size_t virtual_pages;
    size_t page_faults;
    uint64_t cpu_time_ms;
    int8_t nice_value;
} sys_task_t;

typedef struct {
    size_t managed_pages;
    atomic_size_t free_pages;
    size_t watermark[3];
    atomic_flag lock;
} sys_mem_zone_t;

typedef struct {
    sys_mem_zone_t zones[MAX_NR_ZONES];
    atomic_size_t total_reclaimed;
    atomic_size_t total_oom_kills;
    sys_task_t* task_array[SYS_MAX_TASKS];
    uint32_t active_task_count;
} sys_oom_ctrl_t;

static sys_oom_ctrl_t g_sys_ctrl;

static inline void sys_zone_lock(sys_mem_zone_t* zone) {
    while (atomic_flag_test_and_set_explicit(&zone->lock, memory_order_acquire)) {
        __asm__ volatile("pause" ::: "memory");
    }
}

static inline void sys_zone_unlock(sys_mem_zone_t* zone) {
    atomic_flag_clear_explicit(&zone->lock, memory_order_release);
}

void sys_oom_panic(const char* reason, uint32_t code) {
    OOM_HW_REGS->OOM_VICTIM_PID = code;
    OOM_HW_REGS->PANIC_CTRL = 0xDEAD0000 | code;
    while(1) {
        __asm__ volatile("cli; hlt" ::: "memory");
    }
}

void sys_oom_init(size_t dma_pages, size_t normal_pages, size_t highmem_pages) {
    g_sys_ctrl.zones[ZONE_DMA].managed_pages = dma_pages;
    atomic_init(&g_sys_ctrl.zones[ZONE_DMA].free_pages, dma_pages);
    g_sys_ctrl.zones[ZONE_DMA].watermark[WM_MIN] = dma_pages / 64;
    g_sys_ctrl.zones[ZONE_DMA].watermark[WM_LOW] = dma_pages / 32;
    g_sys_ctrl.zones[ZONE_DMA].watermark[WM_HIGH] = dma_pages / 16;
    atomic_flag_clear(&g_sys_ctrl.zones[ZONE_DMA].lock);

    g_sys_ctrl.zones[ZONE_NORMAL].managed_pages = normal_pages;
    atomic_init(&g_sys_ctrl.zones[ZONE_NORMAL].free_pages, normal_pages);
    g_sys_ctrl.zones[ZONE_NORMAL].watermark[WM_MIN] = normal_pages / 128;
    g_sys_ctrl.zones[ZONE_NORMAL].watermark[WM_LOW] = normal_pages / 64;
    g_sys_ctrl.zones[ZONE_NORMAL].watermark[WM_HIGH] = normal_pages / 32;
    atomic_flag_clear(&g_sys_ctrl.zones[ZONE_NORMAL].lock);

    g_sys_ctrl.zones[ZONE_HIGHMEM].managed_pages = highmem_pages;
    atomic_init(&g_sys_ctrl.zones[ZONE_HIGHMEM].free_pages, highmem_pages);
    g_sys_ctrl.zones[ZONE_HIGHMEM].watermark[WM_MIN] = highmem_pages / 256;
    g_sys_ctrl.zones[ZONE_HIGHMEM].watermark[WM_LOW] = highmem_pages / 128;
    g_sys_ctrl.zones[ZONE_HIGHMEM].watermark[WM_HIGH] = highmem_pages / 64;
    atomic_flag_clear(&g_sys_ctrl.zones[ZONE_HIGHMEM].lock);

    atomic_init(&g_sys_ctrl.total_reclaimed, 0);
    atomic_init(&g_sys_ctrl.total_oom_kills, 0);
    g_sys_ctrl.active_task_count = 0;
}

static size_t sys_shrink_slab_caches(void) {
    OOM_HW_REGS->RECLAIM_TRIG = 0x01;
    size_t reclaimed = 0;
    
    for (int i = 0; i < MAX_NR_ZONES; ++i) {
        size_t free_val = atomic_load(&g_sys_ctrl.zones[i].free_pages);
        if (free_val < g_sys_ctrl.zones[i].watermark[WM_LOW]) {
            size_t target_reclaim = g_sys_ctrl.zones[i].watermark[WM_HIGH] - free_val;
            
            size_t actual_reclaimed = (target_reclaim > 100) ? 100 : target_reclaim; 
            atomic_fetch_add(&g_sys_ctrl.zones[i].free_pages, actual_reclaimed);
            reclaimed += actual_reclaimed;
        }
    }
    
    atomic_fetch_add(&g_sys_ctrl.total_reclaimed, reclaimed);
    return reclaimed;
}

static size_t sys_shrink_page_cache(void) {
    OOM_HW_REGS->RECLAIM_TRIG = 0x02;
    size_t reclaimed = 0;
    
    size_t target = g_sys_ctrl.zones[ZONE_NORMAL].watermark[WM_HIGH];
    size_t current = atomic_load(&g_sys_ctrl.zones[ZONE_NORMAL].free_pages);
    
    if (current < target) {
        reclaimed = (target - current) / 2;
        atomic_fetch_add(&g_sys_ctrl.zones[ZONE_NORMAL].free_pages, reclaimed);
    }
    
    atomic_fetch_add(&g_sys_ctrl.total_reclaimed, reclaimed);
    return reclaimed;
}

static uint32_t sys_calculate_badness(sys_task_t* task, size_t total_pages) {
    if (!task) return 0;
    if (task->flags & OOM_FLAG_UNKILLABLE) return 0;
    if (task->state == TASK_STATE_DEAD || task->state == TASK_STATE_ZOMBIE) return 0;

    if (task->oom_score_adj == OOM_SCORE_ADJ_MIN) return 0;

    uint32_t score = (uint32_t)((task->rss_pages * OOM_SCORE_MAX) / total_pages);

    if (task->nice_value > 0) {
        score += (score * task->nice_value) / 20;
    } else if (task->nice_value < 0) {
        score -= (score * (-task->nice_value)) / 20;
    }

    if (task->flags & OOM_FLAG_CRITICAL) {
        score /= 4;
    }

    int32_t adjusted_score = (int32_t)score + (task->oom_score_adj * 10);
    
    if (adjusted_score <= 0) return 1;
    if (adjusted_score > OOM_SCORE_MAX) return OOM_SCORE_MAX;

    return (uint32_t)adjusted_score;
}

static sys_task_t* sys_select_bad_process(void) {
    sys_task_t* chosen_victim = NULL;
    uint32_t highest_score = 0;
    
    size_t total_sys_pages = g_sys_ctrl.zones[ZONE_DMA].managed_pages +
                             g_sys_ctrl.zones[ZONE_NORMAL].managed_pages +
                             g_sys_ctrl.zones[ZONE_HIGHMEM].managed_pages;

    for (uint32_t i = 0; i < g_sys_ctrl.active_task_count; ++i) {
        sys_task_t* task = g_sys_ctrl.task_array[i];
        if (!task) continue;

        uint32_t current_score = sys_calculate_badness(task, total_sys_pages);
        
        if (current_score > highest_score) {
            highest_score = current_score;
            chosen_victim = task;
        }
    }

    return chosen_victim;
}

static void sys_send_sigkill(sys_task_t* victim) {
    victim->state = TASK_STATE_DEAD;
    
    OOM_HW_REGS->OOM_VICTIM_PID = victim->pid;
    OOM_HW_REGS->KILL_SIG_VEC = 0x09; 
    
    size_t rss_reclaim = victim->rss_pages;
    
    sys_zone_lock(&g_sys_ctrl.zones[ZONE_NORMAL]);
    atomic_fetch_add(&g_sys_ctrl.zones[ZONE_NORMAL].free_pages, rss_reclaim);
    sys_zone_unlock(&g_sys_ctrl.zones[ZONE_NORMAL]);

    victim->rss_pages = 0;
    victim->virtual_pages = 0;
    
    atomic_fetch_add(&g_sys_ctrl.total_oom_kills, 1);
}

bool sys_trigger_oom_killer(void) {
    sys_task_t* victim = sys_select_bad_process();
    
    if (!victim) {
        sys_oom_panic("OOM_NO_VICTIM_FOUND", 0xDEAD0001);
        return false; 
    }

    if (victim->pid == 1 || (victim->flags & OOM_FLAG_CRITICAL)) {
        sys_oom_panic("OOM_KILLED_INIT_OR_CRITICAL", victim->pid);
    }

    sys_send_sigkill(victim);
    return true;
}

void* sys_alloc_pages(int zone_idx, size_t count) {
    if (zone_idx < 0 || zone_idx >= MAX_NR_ZONES) return NULL;
    
    sys_mem_zone_t* zone = &g_sys_ctrl.zones[zone_idx];
    size_t current_free;
    uint8_t retry_count = 0;

retry_allocation:
    current_free = atomic_load(&zone->free_pages);
    
    if (current_free >= count + zone->watermark[WM_MIN]) {
        sys_zone_lock(zone);
        if (atomic_load(&zone->free_pages) >= count + zone->watermark[WM_MIN]) {
            atomic_fetch_sub(&zone->free_pages, count);
            sys_zone_unlock(zone);
            return (void*)(uintptr_t)(0x10000000 + (atomic_load(&g_sys_ctrl.total_reclaimed) * PAGE_SIZE));
        }
        sys_zone_unlock(zone);
    }

    if (retry_count == 0) {
        sys_shrink_slab_caches();
        retry_count++;
        goto retry_allocation;
    } else if (retry_count == 1) {
        sys_shrink_page_cache();
        retry_count++;
        goto retry_allocation;
    } else if (retry_count == 2) {
        if (sys_trigger_oom_killer()) {
            retry_count++;
            goto retry_allocation;
        }
    }

    sys_oom_panic("OOM_ALLOC_FAILED_COMPLETELY", 0xDEAD0002);
    return NULL; 
}

void sys_register_task(sys_task_t* new_task) {
    if (g_sys_ctrl.active_task_count < SYS_MAX_TASKS) {
        g_sys_ctrl.task_array[g_sys_ctrl.active_task_count++] = new_task;
    } else {
        sys_oom_panic("TASK_LIMIT_REACHED", 0xDEAD0003);
    }
}
uint32_t auto_pid = 100;
static void new_task() {
    sys_task_t* task = (sys_task_t*)whitex_malloc(sizeof(sys_task_t));
    if (!task) return;

    task->pid = auto_pid++;
    task->ppid = 1;
    task->state = TASK_STATE_RUNNING;
    task->rss_pages = 100;
    task->flags = 0;
    task->oom_score_adj = 0;
    task->nice_value = 0;

    sys_register_task(task);
}
void sys_oom_monitor_check() {
    for (int i = 0; i < MAX_NR_ZONES; ++i) {
        if (atomic_load(&g_sys_ctrl.zones[i].free_pages) < g_sys_ctrl.zones[i].watermark[WM_MIN]) {
            sys_trigger_oom_killer();
        }
    }
}



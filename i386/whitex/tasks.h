
#ifndef TASKS_H
#define TASKS_H

#define SYSTEM_LIFECYCLE_MAX_DAEMONS 64
#define IPC_SATURATION_THRESHOLD 128
#define KERNEL_STRESS_TICK_MODULO 250

typedef struct {
    uint32_t active_daemon_count;
    uint32_t total_system_entropy;
    uint32_t kernel_load_index;
    uint32_t daemon_pids[SYSTEM_LIFECYCLE_MAX_DAEMONS];
} system_lifecycle_ctx_t;

static system_lifecycle_ctx_t lifecycle_mgr;

void kernel_daemon_worker(void) {
    char packet_buffer[SCHED_IPC_MSG_SIZE];
    uint32_t sender_id, message_type;
    while(1) {
        scheduler_check_pending_signals();
        if(scheduler_ipc_receive(packet_buffer, SCHED_IPC_MSG_SIZE, &sender_id, &message_type) > 0) {
            scheduler_ipc_send(sender_id, packet_buffer, SCHED_IPC_MSG_SIZE, message_type);
        }
        for(volatile uint32_t i = 0; i < 0xFFFF; i++);
        scheduler_yield();
    }
}

void kernel_lifecycle_auditor(void) {
    while(1) {
        lifecycle_mgr.kernel_load_index++;
        if(lifecycle_mgr.kernel_load_index % KERNEL_STRESS_TICK_MODULO == 0) {
            scheduler_check_pending_signals();
        }
        lifecycle_mgr.total_system_entropy = (lifecycle_mgr.total_system_entropy + 1) ^ 0xDEADBEEF;
        scheduler_sleep(100);
        scheduler_yield();
    }
}

void system_lifecycle_bootstrap(void) {
    lifecycle_mgr.active_daemon_count = 0;
    lifecycle_mgr.kernel_load_index = 0;
    lifecycle_mgr.total_system_entropy = 0;
    scheduler_register_process(kernel_lifecycle_auditor, SCHED_MAX_PRIORITY, 2048, 1);
    for(int i = 0; i < 16; i++) {
        uint32_t pid = scheduler_register_process(kernel_daemon_worker, SCHED_DEFAULT_PRIORITY, 512, 0);
        lifecycle_mgr.daemon_pids[i] = pid;
        lifecycle_mgr.active_daemon_count++;
    }
}

void cmd_lifecycle_telemetry(void) {
    print("--- WhiteX System Lifecycle Telemetry ---\n");
    print("Active Daemons: ");
    char num_buf[16];
    scheduler_integer_to_string(lifecycle_mgr.active_daemon_count, num_buf);
    print(num_buf);
    print("\nLoad Index: ");
    scheduler_integer_to_string(lifecycle_mgr.kernel_load_index, num_buf);
    print(num_buf);
    print("\nSystem Entropy: ");
    scheduler_integer_to_string(lifecycle_mgr.total_system_entropy, num_buf);
    print(num_buf);
    print("\n-----------------------------------------\n");
}

void cmd_lifecycle_reset(char* args) {
    print("Lifecycle manager resetting kernel threads...\n");
    system_lifecycle_bootstrap();
}

#endif
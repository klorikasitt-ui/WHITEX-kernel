#ifndef NANO_CORE_SYSTEM_H
#define NANO_CORE_SYSTEM_H
#define NANO_MAX_COLS 80
#define NANO_SYS_MAX_ROWS 25
#define NANO_MEM_POOL_SIZE 4096
#define NANO_MAX_LINE_LENGTH 256
#define NANO_MAX_TOTAL_LINES 1024
#define NANO_MAX_CMD_BUFFER 512
#define NANO_STATUS_OK 0x00
#define NANO_STATUS_ERR_MEM 0x01
#define NANO_STATUS_ERR_BOUNDS 0x02
#define NANO_STATUS_EXIT_SIG 0xFF
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef struct {
    char character_data[NANO_MAX_LINE_LENGTH];
    uint32_t occupied_length;
    uint32_t virtual_index;
} nano_text_line_t;
typedef struct {
    nano_text_line_t lines[NANO_MAX_TOTAL_LINES];
    uint32_t active_line_count;
    uint32_t global_cursor_x;
    uint32_t global_cursor_y;
    uint32_t viewport_offset_y;
    uint8_t document_modified_flag;
    uint8_t editor_running_state;
    char status_message_buffer[NANO_MAX_COLS];
} nano_global_context_t;
static nano_global_context_t core_editor_env;
static void nano_sys_memzero(void *destination, uint32_t byte_count) {
    volatile uint8_t *target_ptr = (volatile uint8_t *)destination;
    while (byte_count > 0) {
        *target_ptr = 0x00;
        target_ptr++;
        byte_count--;
    }
}

static uint32_t nano_sys_strlen(const char *string_ptr) {
    uint32_t length_accumulator = 0;
    if (!string_ptr) return 0;
    while (string_ptr[length_accumulator] != '\0') {
        length_accumulator++;
    }
    return length_accumulator;
}

static uint8_t nano_sys_strcmp(const char *string_alpha, const char *string_beta) {
    if (!string_alpha || !string_beta) return 0;
    uint32_t iterator = 0;
    while (string_alpha[iterator] != '\0' && string_beta[iterator] != '\0') {
        if (string_alpha[iterator] != string_beta[iterator]) {
            return 0;
        }
        iterator++;
    }
    if (string_alpha[iterator] != string_beta[iterator]) {
        return 0;
    }
    return 1;
}

static void nano_sys_strcpy(char *destination, const char *source) {
    if (!destination || !source) return;
    uint32_t iterator = 0;
    while (source[iterator] != '\0') {
        destination[iterator] = source[iterator];
        iterator++;
    }
    destination[iterator] = '\0';
}

static void nano_sys_strcat(char *destination, const char *source) {
    if (!destination || !source) return;
    uint32_t dest_len = nano_sys_strlen(destination);
    uint32_t iterator = 0;
    while (source[iterator] != '\0') {
        destination[dest_len + iterator] = source[iterator];
        iterator++;
    }
    destination[dest_len + iterator] = '\0';
}

static void nano_context_hard_reset(void) {
    nano_sys_memzero(&core_editor_env, sizeof(nano_global_context_t));
    core_editor_env.active_line_count = 1;
    core_editor_env.global_cursor_x = 0;
    core_editor_env.global_cursor_y = 0;
    core_editor_env.viewport_offset_y = 0;
    core_editor_env.document_modified_flag = 0;
    core_editor_env.editor_running_state = 1;
    nano_sys_strcpy(core_editor_env.status_message_buffer, "System Initialized. Awaiting Input.");
}

static void nano_set_status_message(const char *message) {
    nano_sys_memzero(core_editor_env.status_message_buffer, NANO_MAX_COLS);
    nano_sys_strcpy(core_editor_env.status_message_buffer, message);
}

static void nano_insert_character_at_cursor(char input_char) {
    uint32_t current_line_idx = core_editor_env.global_cursor_y;
    if (current_line_idx >= NANO_MAX_TOTAL_LINES) return;

    nano_text_line_t *target_line = &core_editor_env.lines[current_line_idx];
    
    if (target_line->occupied_length >= (NANO_MAX_LINE_LENGTH - 2)) {
        if (core_editor_env.active_line_count < NANO_MAX_TOTAL_LINES) {
            core_editor_env.global_cursor_y++;
            core_editor_env.global_cursor_x = 0;
            core_editor_env.active_line_count++;
            target_line = &core_editor_env.lines[core_editor_env.global_cursor_y];
        } else {
            nano_set_status_message("ERROR: Maximum document length reached.");
            return;
        }
    }

    uint32_t x_pos = core_editor_env.global_cursor_x;
    for (uint32_t i = target_line->occupied_length; i > x_pos; i--) {
        target_line->character_data[i] = target_line->character_data[i - 1];
    }

    target_line->character_data[x_pos] = input_char;
    target_line->occupied_length++;
    target_line->character_data[target_line->occupied_length] = '\0';
    
    core_editor_env.global_cursor_x++;
    core_editor_env.document_modified_flag = 1;
    nano_set_status_message("Document Modified.");
}

static void nano_process_raw_input_stream(const char *stream_buffer) {
    uint32_t buffer_index = 0;
    while (stream_buffer[buffer_index] != '\0') {
        nano_insert_character_at_cursor(stream_buffer[buffer_index]);
        buffer_index++;
    }
}

static void nano_render_header_component(void) {
    print("================================================================================\n");
    print(" GNU nano 7.2.X-Advanced       [ SYSTEM KERNEL BUFFER ]                         \n");
    print("================================================================================\n");
}

static void nano_render_status_component(void) {
    print("--------------------------------------------------------------------------------\n");
    print("[ STATUS ]: ");
    print(core_editor_env.status_message_buffer);
    print("\n");
}

static void nano_render_footer_component(void) {
    print("--------------------------------------------------------------------------------\n");
    print(" COMMAND MODE : Type '-cmdnanocmd'     |   EXIT SEQUENCE : Type 'exit'          \n");
    print("================================================================================\n");
}

static void nano_render_text_viewport(void) {
    uint32_t max_visible_lines = NANO_SYS_MAX_ROWS - 9;
    uint32_t render_y_iterator = 0;

    while (render_y_iterator < max_visible_lines) {
        uint32_t actual_line_index = core_editor_env.viewport_offset_y + render_y_iterator;
        
        if (actual_line_index < core_editor_env.active_line_count) {
            if (core_editor_env.lines[actual_line_index].occupied_length == 0) {
                print(" \n");
            } else {
                print(core_editor_env.lines[actual_line_index].character_data);
                print("\n");
            }
        } else {
            print("~\n");
        }
        render_y_iterator++;
    }
}

static void nano_force_display_refresh(void) {
    cls();
    nano_render_header_component();
    nano_render_text_viewport();
    nano_render_status_component();
    nano_render_footer_component();
}

static void nano_subsystem_execute_clear(void) {
    nano_sys_memzero(&core_editor_env.lines, sizeof(nano_text_line_t) * NANO_MAX_TOTAL_LINES);
    core_editor_env.active_line_count = 1;
    core_editor_env.global_cursor_x = 0;
    core_editor_env.global_cursor_y = 0;
    core_editor_env.viewport_offset_y = 0;
    nano_set_status_message("Buffer wiped clean successfully.");
}

static void nano_subsystem_execute_stats(void) {
    char stat_msg[NANO_MAX_COLS];
    nano_sys_memzero(stat_msg, NANO_MAX_COLS);
    nano_sys_strcpy(stat_msg, "Total Lines: ACTIVE | Memory: SECURE | Subsystem: ONLINE");
    nano_set_status_message(stat_msg);
}

static void nano_subsystem_terminal_dispatcher(void) {
    char terminal_command_buffer[NANO_MAX_CMD_BUFFER];
    uint8_t terminal_active_flag = 1;

    while (terminal_active_flag) {
        cls();
        print("================================================================================\n");
        print("                 [ NANO INTERNAL COMMAND SUBSYSTEM ]                            \n");
        print("================================================================================\n");
        print(" Available Directives:\n");
        print("   -> return    : Return to document editing\n");
        print("   -> clear     : Destroy all document contents\n");
        print("   -> stats     : Display buffer memory statistics\n");
        print("   -> save      : Commit buffer to virtual file system\n");
        print("   -> abort     : Terminate entire editor instance\n");
        print("--------------------------------------------------------------------------------\n");
        print("root@nano-internal:~# ");
        
        nano_sys_memzero(terminal_command_buffer, NANO_MAX_CMD_BUFFER);
        scan(terminal_command_buffer);

        if (nano_sys_strcmp(terminal_command_buffer, "return")) {
            terminal_active_flag = 0;
            nano_set_status_message("Returned to editor mode.");
        } 
        else if (nano_sys_strcmp(terminal_command_buffer, "abort")) {
            terminal_active_flag = 0;
            core_editor_env.editor_running_state = 0;
        }
        else if (nano_sys_strcmp(terminal_command_buffer, "clear")) {
            nano_subsystem_execute_clear();
            print("\n[SUCCESS] Document completely erased. Press any key...");
            char dummy[10];
            scan(dummy);
        }
        else if (nano_sys_strcmp(terminal_command_buffer, "stats")) {
            nano_subsystem_execute_stats();
            print("\n[SUCCESS] Statistics updated in status bar. Press any key...");
            char dummy[10];
            scan(dummy);
        }
        else if (nano_sys_strcmp(terminal_command_buffer, "save")) {
            nano_set_status_message("File saved to VFS (Simulated).");
            print("\n[SUCCESS] I/O Operation complete. Press any key...");
            char dummy[10];
            scan(dummy);
        }
        else {
            print("\n[FATAL] Unrecognized directive. Try again. Press any key...");
            char dummy[10];
            scan(dummy);
        }
    }
}

static void execute_nano_terminal_simulator() {
    char keyboard_input_sequence[NANO_MAX_CMD_BUFFER];
    nano_context_hard_reset();

    while (core_editor_env.editor_running_state == 1) {
        nano_force_display_refresh();
        
        print("\n[INPUT]> ");
        nano_sys_memzero(keyboard_input_sequence, NANO_MAX_CMD_BUFFER);
        scan(keyboard_input_sequence);

        if (nano_sys_strcmp(keyboard_input_sequence, "exit")) {
            core_editor_env.editor_running_state = 0;
        } 
        else if (nano_sys_strcmp(keyboard_input_sequence, "-cmdnanocmd")) {
            nano_subsystem_terminal_dispatcher();
        } 
        else if (keyboard_input_sequence[0] != '\0') {
            nano_process_raw_input_stream(keyboard_input_sequence);
        }
    }
    
    cls();
}

#endif

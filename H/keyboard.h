#include <stdint.h>
#include <stddef.h>

extern void terminal_backspace();
extern void putchar(char c);
extern uint8_t inb(uint16_t port);

static int shift_pressed = 0;
static int caps_lock = 0;
static int is_extended = 0;

unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

unsigned char kbd_us_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

char input_buffer[256];
int buffer_idx = 0;
int input_complete = 0;

void keyboard_handler() {
    uint8_t scancode = inb(0x60);

    if (scancode == 0xE0) { 
        is_extended = 1; 
        return; 
    }

    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) shift_pressed = 0;
        is_extended = 0;
        return;
    }

    if (scancode == 0x2A || scancode == 0x36) { 
        shift_pressed = 1; 
        return; 
    }

    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }

    if (is_extended) { 
        is_extended = 0; 
        return; 
    }

    if (scancode == 0x0E) {
        if (buffer_idx > 0) {
            buffer_idx--;
            input_buffer[buffer_idx] = '\0';
            terminal_backspace();
        }
        return;
    }

    if (scancode == 0x1C) {
        input_buffer[buffer_idx] = '\0';
        input_complete = 1;
        putchar('\n'); 
        return;
    }

    if (scancode < 128) {
        char c = 0;
        int use_upper = (shift_pressed ^ caps_lock);

        if (use_upper && kbd_us_shift[scancode] >= 'A' && kbd_us_shift[scancode] <= 'Z') {
            c = kbd_us_shift[scancode];
        } else if (shift_pressed) {
            c = kbd_us_shift[scancode];
        } else {
            c = kbd_us[scancode];
        }

        if (c != 0 && buffer_idx < 254) {
            input_buffer[buffer_idx++] = c;
            putchar(c);
        }
    }
}

void scan(char* buffer) {
    buffer_idx = 0;
    input_complete = 0;
    
    for (int i = 0; i < 256; i++) {
        input_buffer[i] = 0;
    }

    while (!input_complete) {
        if (inb(0x64) & 0x01) {
            keyboard_handler();
        }
    }

    int i = 0;
    while (input_buffer[i] != '\0') {
        buffer[i] = input_buffer[i];
        i++;
    }
    buffer[i] = '\0';
}

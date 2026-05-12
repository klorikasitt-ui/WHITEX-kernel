#include <stdint.h>
#include <stddef.h>

extern void terminal_backspace();
extern void putchar(char c);

static int shift_pressed = 0;
static int is_extended = 0;

unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

unsigned char kbd_us_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
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
        uint8_t released_code = scancode & 0x7F;
        if (released_code == 0x2A || released_code == 0x36) shift_pressed = 0;
        is_extended = 0;
        return;
    }

    
    if (scancode == 0x2A || scancode == 0x36) { 
        shift_pressed = 1; 
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

    char c = shift_pressed ? kbd_us_shift[scancode] : kbd_us[scancode];
    if (c != 0 && buffer_idx < 255) {
        input_buffer[buffer_idx++] = c;
        putchar(c);
    }
}

void scan(char* buffer) {
    buffer_idx = 0;
    input_complete = 0;

   
    while (!input_complete) {
        // PS/2 Controller Status Register (0x64 bit 0: Output buffer full)
        if (inb(0x64) & 1) {
            keyboard_handler();
        }
    }

 
    int i = 0;
    while (input_buffer[i] != '\0') {
        buffer[i] = input_buffer[i];
        i++;
    }
    buffer[i] = '\0'; // Null-terminator
}

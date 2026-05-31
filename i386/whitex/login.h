void scan_password(char* buffer) {
    buffer_idx = 0;
    input_complete = 0;
    
    for (int i = 0; i < 256; i++) {
        input_buffer[i] = 0;
    }

    while (!input_complete) {
        if (inb(0x64) & 0x01) {
            uint8_t scancode = inb(0x60);
            
            if (scancode == 0x1C) {
                input_buffer[buffer_idx] = '\0';
                input_complete = 1;
                putchar('\n');
            }
            else if (scancode == 0x0E) {
                if (buffer_idx > 0) {
                    buffer_idx--;
                    input_buffer[buffer_idx] = '\0';
                    terminal_backspace();
                }
            }
            else if (!(scancode & 0x80) && scancode < 128 && scancode != 0x2A && scancode != 0x36 && scancode != 0x3A) {
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
                    putchar('*');
                }
            }
            
            if (scancode == 0x2A || scancode == 0x36) shift_pressed = 1;
            if ((scancode & 0x80) && ((scancode & 0x7F) == 0x2A || (scancode & 0x7F) == 0x36)) shift_pressed = 0;
            if (scancode == 0x3A) caps_lock = !caps_lock;
        }
    }

    int i = 0;
    while (input_buffer[i] != '\0') {
        buffer[i] = input_buffer[i];
        i++;
    }
    buffer[i] = '\0';
}

void login() {
    char username[64];
    char password[64];
    int attempts = 3;

    while (attempts > 0) {
        
        
        print("Username: ");
        scan(username);

        print("Password: ");
        scan_password(password);

        if (strcmp(username, "nousr") == 0 && strcmp(password, "root") == 0) {
            print("\n[+] ACCESS GRANTED.\n\n");
            return;
        } else {
            attempts--;
            print("\n[-] INVALID CREDENTIALS. ");
            if (attempts > 0) {
                print("Please try again.\n");
            }
        }
    }

    print("\n[!!!] ACCESS DENIED. TRY AGAIN .\n");
}
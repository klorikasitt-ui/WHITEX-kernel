#define MAX_LINES 100
#define LINE_LEN 128

char text_storage[MAX_LINES][LINE_LEN];
int line_count = 0;

int str_compare(char* s1, char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void refresh_screen() {
    print("\033[2J\033[H"); 
    print("WhiteX Nano v1.0\n");
    print("------------------------------------------\n");
    for (int i = 0; i < line_count; i++) {
        print(text_storage[i]);
        print("\n");
    }
    for (int i = line_count; i < 20; i++) {
        print("~\n");
    }
    print("------------------------------------------\n");
    print("^Q: Quit | ^V: Refresh | System: :ls, :pwd\n");
}

void notepad() {
    char input[LINE_LEN];
    line_count = 0;

    for (int i = 0; i < MAX_LINES; i++) {
        text_storage[i][0] = '\0';
    }

    refresh_screen();

    while (1) {
        scan(input);

        if (str_compare(input, ":q") == 0) {
            break;
        } 
        else if (str_compare(input, ":v") == 0) {
            refresh_screen();
        }
        else if (str_compare(input, ":ls") == 0) {
            ls();
        }
        else if (str_compare(input, ":pwd") == 0) {
            pwd();
        }
        else if (str_compare(input, ":mkdir") == 0) {
            mkdir();
        }
        else if (str_compare(input, ":cd") == 0) {
            cd();
        }
        else {
            if (line_count < MAX_LINES) {
                int j = 0;
                while (input[j] != '\0' && j < LINE_LEN - 1) {
                    text_storage[line_count][j] = input[j];
                    j++;
                }
                text_storage[line_count][j] = '\0';
                line_count++;
                refresh_screen();
            } else {
                print("Error: Buffer full!\n");
            }
        }
    }
}

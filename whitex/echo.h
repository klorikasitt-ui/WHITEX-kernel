

void echo(const char* input) {
    if (input[4] == ' ' && input[5] != '\0') {
        print(input + 5);
        print("\n");
    } else {
        print("\n");
    }
}

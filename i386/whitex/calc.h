int power(int base, int exp) {
    if (exp < 0) return 0;
    int res = 1;
    for (int i = 0; i < exp; i++) {
        res *= base;
    }
    return res;
}

void itoa2(int num, char* str) {
    int i = 0;
    int is_neg = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    if (num < 0) {
        is_neg = 1;
        num = -num;
    }
    while (num != 0) {
        str[i++] = (num % 10) + '0';
        num = num / 10;
    }
    if (is_neg) {
        str[i++] = '-';
    }
    str[i] = '\0';

    int start = 0, end = i - 1;
    while (start < end) {
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++; 
        end--;
    }
}



int get_precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/' || op == '%' || op == 'x') return 2;
    if (op == '^') return 3;
    if (op == '&' || op == '|') return 0;
    return -1;
}

int apply_op(int a, int b, char op, int* err) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': 
        case 'x': return a * b;
        case '/': 
            if (b == 0) { *err = 1; return 0; }
            return a / b;
        case '%': 
            if (b == 0) { *err = 2; return 0; }
            return a % b;
        case '^': return power(a, b);
        case '&': return a & b;
        case '|': return a | b;
    }
    return 0;
}

void cmd_calc_handler(char *arg_buffer) {
    if (!arg_buffer || arg_buffer[0] == '\0') {
        print("Usage: calc <expression> (e.g., calc (15+72)*2)\n");
        return;
    }

    int values[40];
    char ops[40];
    int val_top = -1;
    int ops_top = -1;
    int err = 0;

    for (int i = 0; arg_buffer[i] != '\0'; i++) {
        if (arg_buffer[i] == ' ') continue;

        if (arg_buffer[i] == '(') {
            if (ops_top >= 39) { print("SHELL ERROR: Stack overflow\n"); return; }
            ops[++ops_top] = arg_buffer[i];
        }
        else if (arg_buffer[i] >= '0' && arg_buffer[i] <= '9') {
            int val = 0;
            while (arg_buffer[i] >= '0' && arg_buffer[i] <= '9') {
                val = (val * 10) + (arg_buffer[i] - '0');
                i++;
            }
            i--; 
            if (val_top >= 39) { print("SHELL ERROR: Stack overflow\n"); return; }
            values[++val_top] = val;
        }
        else if (arg_buffer[i] == ')') {
            while (ops_top >= 0 && ops[ops_top] != '(') {
                if (val_top < 1) { print("SHELL ERROR: Invalid syntax\n"); return; }
                int val2 = values[val_top--];
                int val1 = values[val_top--];
                char op = ops[ops_top--];
                
                int res = apply_op(val1, val2, op, &err);
                if (err == 1) { print("KERNEL PANIC: Division by zero\n"); return; }
                if (err == 2) { print("KERNEL PANIC: Modulo by zero\n"); return; }
                values[++val_top] = res;
            }
            if (ops_top >= 0) ops_top--; 
        }
        else {
            char current_op = arg_buffer[i];
            while (ops_top >= 0 && get_precedence(ops[ops_top]) >= get_precedence(current_op)) {
                if (val_top < 1) { print("SHELL ERROR: Invalid syntax\n"); return; }
                int val2 = values[val_top--];
                int val1 = values[val_top--];
                char op = ops[ops_top--];

                int res = apply_op(val1, val2, op, &err);
                if (err == 1) { print("KERNEL PANIC: Division by zero\n"); return; }
                if (err == 2) { print("KERNEL PANIC: Modulo by zero\n"); return; }
                values[++val_top] = res;
            }
            if (ops_top >= 39) { print("SHELL ERROR: Stack overflow\n"); return; }
            ops[++ops_top] = current_op;
        }
    }

    while (ops_top >= 0) {
        if (val_top < 1) { print("SHELL ERROR: Invalid syntax\n"); return; }
        int val2 = values[val_top--];
        int val1 = values[val_top--];
        char op = ops[ops_top--];

        int res = apply_op(val1, val2, op, &err);
        if (err == 1) { print(" Division by zero\n"); return; }
        if (err == 2) { print(" Modulo by zero\n"); return; }
        values[++val_top] = res;
    }

    if (val_top == 0) {
        char out[12];
        itoa2(values[val_top], out);
        print(out);
        print("\n");
    } else {
        print("SHELL ERROR: Invalid expression\n");
    }
}

void touch() {
    char name[NAME_LEN];
    print("File Name: ");
    scan(name);
    for (int i = 0; i < MAX_NODES; i++) {
        if (!storage[i].is_used) {
            storage[i].is_used = 1;
            storage[i].type = FILE_NODE;
            storage[i].parent_idx = current_dir;
            fs_strcpy(storage[i].name, name);
            print("File created.\n");
            return;
        }
    }
    print("FS Full!\n");
}

void rm() {
    char name[NAME_LEN];
    print("Remove: ");
    scan(name);
    for (int i = 0; i < MAX_NODES; i++) {
        if (storage[i].is_used && strcmp(storage[i].name, name) == 0 && storage[i].parent_idx == current_dir) {
            if (i == 0) {
                print("Cannot remove root!\n");
                return;
            }
            storage[i].is_used = 0;
            print("Deleted.\n");
            return;
        }
    }
    print("Not found.\n");
}

void Sdd_Sync() {
    for (int i = 0; i < (sizeof(storage) / 512) + 1; i++) {
        write_block(100 + i, (uint8_t*)storage + (i * 512));
    }
    print("FS Synced to Disk.\n");
}
void shellfs() {
    char cmd[16];
    while (1) {
        print("white$\n");
        print(storage[current_dir].name);
        print("> ");
        scan(cmd);

        if (strcmp(cmd, "ls") == 0) ls();
        else if (strcmp(cmd, "mkdir") == 0) mkdir();
        else if (strcmp(cmd, "cd") == 0) cd();
        else if (strcmp(cmd, "pwd") == 0) pwd();
        else if (strcmp(cmd, "touch") == 0) touch();
        else if (strcmp(cmd, "rm") == 0) rm();
        else if (strcmp(cmd, "sync") == 0) Sdd_Sync();
        else if (strcmp(cmd, "exit") == 0) break;
        else print("Unknown command.\n");
    }
}
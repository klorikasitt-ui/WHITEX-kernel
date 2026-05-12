#define MAX_NODES 128
#define NAME_LEN 32

typedef enum { 
    DIR_NODE, 
    FILE_NODE 
} NodeType;

typedef struct {
    char name[NAME_LEN];
    NodeType type;
    int parent_idx;
    int is_used;
} VFSNode;

static VFSNode storage[MAX_NODES];
static int current_dir = 0;

void fs_strcpy(char* dest, const char* src) {
    int i = 0;
    while (src[i] != '\0' && i < NAME_LEN - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void init_fs() {
    for (int i = 0; i < MAX_NODES; i++) {
        storage[i].is_used = 0;
    }
    storage[0].is_used = 1;
    storage[0].type = DIR_NODE;
    storage[0].parent_idx = 0;
    fs_strcpy(storage[0].name, "root");
    current_dir = 0;
    print("FS Ready.\n");
}

void ls() {
    print("\n");
    for (int i = 0; i < MAX_NODES; i++) {
        if (storage[i].is_used && storage[i].parent_idx == current_dir && i != current_dir) {
            if (storage[i].type == DIR_NODE) print("[D] ");
            else print("[F] ");
            print(storage[i].name);
            print("\n");
        }
    }
}

void mkdir() {
    char name[NAME_LEN];
    print("Name: ");
    scan(name);
    for (int i = 0; i < MAX_NODES; i++) {
        if (!storage[i].is_used) {
            storage[i].is_used = 1;
            storage[i].type = DIR_NODE;
            storage[i].parent_idx = current_dir;
            fs_strcpy(storage[i].name, name);
            return;
        }
    }
}

void cd() {
    char name[NAME_LEN];
    print("To: ");
    scan(name);
    if (strcmp(name, "..") == 0) {
        current_dir = storage[current_dir].parent_idx;
        return;
    }
    for (int i = 0; i < MAX_NODES; i++) {
        if (storage[i].is_used && storage[i].type == DIR_NODE && 
            strcmp(storage[i].name, name) == 0 && storage[i].parent_idx == current_dir) {
            current_dir = i;
            return;
        }
    }
}

void pwd() {
    print("/");
    print(storage[current_dir].name);
    print("\n");
}

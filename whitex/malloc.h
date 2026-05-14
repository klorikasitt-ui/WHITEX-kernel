
#define HEAP_SIZE 65536 
#define BLOCK_FREE 0
#define BLOCK_USED 1

void notgud(void);

typedef struct {
    size_t size;
    uint8_t status;
} block_header_t;

static uint8_t memory_heap[HEAP_SIZE];

static void ram_print_number(size_t n) {
    if (n == 0) {
        putchar('0');
        return;
    }
    char buf[12];
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (--i >= 0) {
        putchar(buf[i]);
    }
}

size_t get_free_ram() {
    size_t free_ram = HEAP_SIZE;
    size_t offset = 0;
    while (offset < HEAP_SIZE) {
        block_header_t* header = (block_header_t*)&memory_heap[offset];
        if (header->size == 0) break;
        if (header->status == BLOCK_USED) {
            free_ram -= (header->size + sizeof(block_header_t));
        }
        offset += sizeof(block_header_t) + header->size;
    }
    return free_ram;
}

void* whitex_malloc(size_t size) {
    size_t offset = 0;
    while (offset < HEAP_SIZE) {
        block_header_t* header = (block_header_t*)&memory_heap[offset];
        if (header->size == 0 || (header->status == BLOCK_FREE && header->size >= size)) {
            header->size = size;
            header->status = BLOCK_USED;
            return (void*)&memory_heap[offset + sizeof(block_header_t)];
        }
        offset += sizeof(block_header_t) + header->size;
    }
    print("Not available RAM!\n");
    notgud(); 
    return NULL;
}

void whitex_free(void* ptr) {
    if (!ptr) return;
    block_header_t* header = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    header->status = BLOCK_FREE;
}

void ram() {
    static int first = 1;
    if (first) {
        for (size_t i = 0; i < HEAP_SIZE; i++) {
            memory_heap[i] = 0;
        }
        first = 0;
    }
    print("Free RAM: ");
    ram_print_number(get_free_ram()); 
    print(" bytes\n");
}
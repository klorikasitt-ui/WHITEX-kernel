
#ifndef SDD_H
#define SDD_H
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));

}

void quit() {
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
}

#define ATA_DATA    0x1F0
#define ATA_STATUS  0x1F7

typedef enum { DEV_NONE, DEV_ATA } storage_type_t;

typedef struct {
    storage_type_t type;
    int (*read)(uint64_t lba, uint32_t count, void* buffer);
    int (*write)(uint64_t lba, uint32_t count, void* buffer);
} storage_device_t;

static storage_device_t primary_dev = {DEV_NONE, 0, 0};

int ata_read_sectors(uint64_t lba, uint32_t count, void* buffer) {
    uint16_t* buf = (uint16_t*)buffer;
    for(uint32_t i = 0; i < count; i++) {
        outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
        outb(0x1F2, 1);
        outb(0x1F3, (uint8_t)lba);
        outb(0x1F4, (uint8_t)(lba >> 8));
        outb(0x1F5, (uint8_t)(lba >> 16));
        outb(0x1F7, 0x20); 
        while (!(inb(ATA_STATUS) & 0x08));
        for (int j = 0; j < 256; j++) buf[j + (i * 256)] = inw(ATA_DATA);
        lba++;
    }
    return 0;
}

int ata_write_sectors(uint64_t lba, uint32_t count, void* buffer) {
    uint16_t* buf = (uint16_t*)buffer;
    for(uint32_t i = 0; i < count; i++) {
        outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
        outb(0x1F2, 1);
        outb(0x1F3, (uint8_t)lba);
        outb(0x1F4, (uint8_t)(lba >> 8));
        outb(0x1F5, (uint8_t)(lba >> 16));
        outb(0x1F7, 0x30); 
        while (!(inb(ATA_STATUS) & 0x08));
        for (int j = 0; j < 256; j++) outw(ATA_DATA, buf[j + (i * 256)]);
        lba++;
    }
    return 0;
}
int read_block(uint64_t block_id, void* buffer) {
    if (primary_dev.read) return primary_dev.read(block_id, 1, buffer);
    return -1;
}

int write_block(uint64_t block_id, void* buffer) {
    if (primary_dev.write) return primary_dev.write(block_id, 1, buffer);
    return -1;
}

void Sdd() {
    uint8_t status = inb(ATA_STATUS);
    if (status != 0xFF) {
        primary_dev.type = DEV_ATA;
        primary_dev.read = ata_read_sectors;
        primary_dev.write = ata_write_sectors;
        print("ATA Ready.\n");
    }
}
void fs_sync() {
    int sectors = (sizeof(storage) / 512) + 1;
    for (int i = 0; i < sectors; i++) {
        write_block(100 + i, (uint8_t*)storage + (i * 512));
    }
    print("Disk Synced.\n");
}

void whitex_touch() {
    char name[32];
    print("File: "); scan(name);
    for (int i = 0; i < 128; i++) {
        if (!storage[i].is_used) {
            storage[i].is_used = 1;
            storage[i].type = FILE_NODE;
            storage[i].parent_idx = current_dir;
            fs_strcpy(storage[i].name, name);
            return;
        }
    }
}

void whitex_rm() {
    char name[32];
    print("RM: "); scan(name);
    for (int i = 0; i < 128; i++) {
        if (storage[i].is_used && strcmp(storage[i].name, name) == 0 && storage[i].parent_idx == current_dir) {
            if (i != 0) storage[i].is_used = 0;
            return;
        }
    }
}

/* --- Shell --- */
void fsshell() {
    char cmd[32];
    while(1) {
        print("WhiteX:"); print(storage[current_dir].name); print("> ");
        scan(cmd);
         
        if (strcmp(cmd, "ls") == 0) ls();
        else if (strcmp(cmd, "quit") == 0) quit();
        else if (strcmp(cmd, "mkdir") == 0) mkdir();
        else if (strcmp(cmd, "cd") == 0) cd();
        else if (strcmp(cmd, "pwd") == 0) pwd();
        else if (strcmp(cmd, "touch") == 0) whitex_touch();
        else if (strcmp(cmd, "rm") == 0) whitex_rm();
        else if (strcmp(cmd, "sync") == 0) fs_sync();
        else if (strcmp(cmd, "help") == 0) print("ls, mkdir, cd, pwd, touch, rm, sync\n");
        else print("Error: Cmd not found.\n");
    }
}

#endif

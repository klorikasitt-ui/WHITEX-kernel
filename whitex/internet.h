
#define E1000_REG_CTRL      0x0000
#define E1000_REG_RCTL      0x0100
#define E1000_REG_RDBAL     0x2800
#define E1000_REG_RDLEN     0x2808
#define E1000_REG_RDH       0x2810
#define E1000_REG_RDT       0x2818

#define mmio_write32(addr, val) (*((volatile uint32_t*)(addr)) = (val))
#define mmio_read32(addr) (*((volatile uint32_t*)(addr)))

struct e1000_rx_desc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed));

static struct e1000_rx_desc rx_ring[32] __attribute__((aligned(16)));
static uint8_t rx_buffers[32][2048];
static int rx_ptr = 0;

void print_hex(uint32_t n) {
    const char *hex = "0123456789ABCDEF";
    print("0x");
    for (int i = 28; i >= 0; i -= 4) putchar(hex[(n >> i) & 0xF]);
}

void e1000_init(uint32_t base_addr) {
    print("NIC: started.. ");
    
    uint32_t ctrl = mmio_read32(base_addr + E1000_REG_CTRL);
    mmio_write32(base_addr + E1000_REG_CTRL, ctrl | (1 << 26));
    
    for(int i = 0; i < 32; i++) {
        rx_ring[i].addr = (uint64_t)(uintptr_t)rx_buffers[i];
        rx_ring[i].status = 0;
    }

    mmio_write32(base_addr + E1000_REG_RDBAL, (uint32_t)(uintptr_t)rx_ring);
    mmio_write32(base_addr + E1000_REG_RDLEN, 32 * sizeof(struct e1000_rx_desc));
    mmio_write32(base_addr + E1000_REG_RDH, 0);
    mmio_write32(base_addr + E1000_REG_RDT, 31);

    mmio_write32(base_addr + E1000_REG_RCTL, (1 << 1) | (1 << 15));
    
    print("[Ok]\n");
}

void e1000_check_packets(uint32_t base_addr) {
    if (rx_ring[rx_ptr].status & 0x01) {
        print("\n[!] New pkg. size:");
        print_hex(rx_ring[rx_ptr].length);
        print("\ndata: ");
        
        for(int i = 0; i < 6; i++) {
            print_hex(rx_buffers[rx_ptr][i]);
            print(" ");
        }

        rx_ring[rx_ptr].status = 0;
        mmio_write32(base_addr + E1000_REG_RDT, rx_ptr);
        rx_ptr = (rx_ptr + 1) % 32;
    }
}

void internetmain() {
    init(); 
    
    uint32_t nic_base = 0xFEB00000; 

    e1000_init(nic_base);
    
    print("System ready package awaited. .\n");

    while(1) {
        e1000_check_packets(nic_base);

        if (inb(0x64) & 1) {
            keyboard_handler(); 
        }
    }
}

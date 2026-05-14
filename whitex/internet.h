#ifndef WHITE_NET_FULL_H
#define WHITE_NET_FULL_H

#include <stdint.h>
#include <stddef.h>

#define E1000_REG_CTRL      0x0000
#define E1000_REG_RCTL      0x0100
#define E1000_REG_RDBAL     0x2800
#define E1000_REG_RDLEN     0x2808
#define E1000_REG_RDH       0x2810
#define E1000_REG_RDT       0x2818

#define mmio_write32(addr, val) (*((volatile uint32_t*)(addr)) = (val))
#define mmio_read32(addr) (*((volatile uint32_t*)(addr)))

static inline unsigned char inb_local(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static char get_input_local() {
    while(!(inb_local(0x64) & 1));
    return inb_local(0x60);
}

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

typedef struct {
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint8_t  local_ip[4];
} white_net_api_t;

static white_net_api_t net_api = {0, 0, {192, 168, 1, 10}};

void parse_net_packet(uint8_t* buffer, uint16_t len) {
    net_api.rx_packets++;
    print("\n[WhiteX Net] Packet Recv\n");
}

void e1000_init(uint32_t base_addr) {
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
}

void e1000_poll(uint32_t base_addr) {
    if (rx_ring[rx_ptr].status & 0x01) {
        parse_net_packet(rx_buffers[rx_ptr], rx_ring[rx_ptr].length);
        rx_ring[rx_ptr].status = 0;
        mmio_write32(base_addr + E1000_REG_RDT, rx_ptr);
        rx_ptr = (rx_ptr + 1) % 32;
    }
}

void internetmain() {
    uint32_t nic_base = 0xFEB00000; 
    e1000_init(nic_base);
    print("\n[WhiteX Online] Network interface is UP.\n");
    while(1) {
        e1000_poll(nic_base);
        if (inb_local(0x64) & 1) {
            char cmd = get_input_local();
            if (cmd == 0x10) break; // 'q' tusu ile cikis
        }
    }
}

#endif


#ifndef INTERNET_H
#define INTERNET_H

#include <stdint.h>
#include <stddef.h>

#define WX_HTONS(x) ((uint16_t)((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8)))
#define WX_HTONL(x) ((uint32_t)((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24)))
#define WX_NTOHS(x) WX_HTONS(x)
#define WX_NTOHL(x) WX_HTONL(x)

#define mmio_write32(addr, val) (*((volatile uint32_t*)(addr)) = (val))
#define mmio_read32(addr) (*((volatile uint32_t*)(addr)))

static inline unsigned char wx_inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void wx_memset(void *dest, uint8_t val, size_t len) {
    uint8_t *dst = (uint8_t *)dest;
    while (len--) *dst++ = val;
}

static void wx_memcpy(void *dest, const void *src, size_t len) {
    uint8_t *dst = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (len--) *dst++ = *s++;
}

static int wx_memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

typedef struct {
    uint8_t buffer[4096];
    uint16_t head;
    uint16_t data;
    uint16_t tail;
    uint16_t end;
    uint32_t network_header;
    uint32_t transport_header;
    uint32_t mac_header;
} wx_net_buf_t;

#define WX_NET_BUF_POOL_SIZE 256
static wx_net_buf_t buf_pool[WX_NET_BUF_POOL_SIZE];
static uint8_t buf_pool_used[WX_NET_BUF_POOL_SIZE];

wx_net_buf_t* wx_net_buf_alloc() {
    for (int i = 0; i < WX_NET_BUF_POOL_SIZE; i++) {
        if (!buf_pool_used[i]) {
            buf_pool_used[i] = 1;
            wx_memset(&buf_pool[i], 0, sizeof(wx_net_buf_t));
            buf_pool[i].head = 256;
            buf_pool[i].data = 256;
            buf_pool[i].tail = 256;
            buf_pool[i].end = 4096;
            return &buf_pool[i];
        }
    }
    return NULL;
}

void wx_net_buf_free(wx_net_buf_t* buf) {
    if (!buf) return;
    ptrdiff_t idx = buf - buf_pool;
    if (idx >= 0 && idx < WX_NET_BUF_POOL_SIZE) {
        buf_pool_used[idx] = 0;
    }
}

void wx_net_buf_reserve(wx_net_buf_t* buf, uint16_t len) {
    buf->data += len;
    buf->tail += len;
}

void* wx_net_buf_push(wx_net_buf_t* buf, uint16_t len) {
    buf->data -= len;
    return &buf->buffer[buf->data];
}

void* wx_net_buf_put(wx_net_buf_t* buf, uint16_t len) {
    void* tmp = &buf->buffer[buf->tail];
    buf->tail += len;
    return tmp;
}

void* wx_net_buf_pull(wx_net_buf_t* buf, uint16_t len) {
    void* tmp = &buf->buffer[buf->data];
    buf->data += len;
    return tmp;
}

typedef struct {
    uint8_t mac[6];
} __attribute__((packed)) wx_mac_t;

typedef struct {
    uint8_t ip[4];
} __attribute__((packed)) wx_ipv4_t;

typedef struct {
    wx_mac_t dest;
    wx_mac_t src;
    uint16_t ethertype;
} __attribute__((packed)) wx_eth_hdr_t;

typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_len;
    uint8_t proto_len;
    uint16_t opcode;
    wx_mac_t sender_mac;
    wx_ipv4_t sender_ip;
    wx_mac_t target_mac;
    wx_ipv4_t target_ip;
} __attribute__((packed)) wx_arp_hdr_t;

typedef struct {
    uint8_t ihl : 4;
    uint8_t version : 4;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    wx_ipv4_t src_ip;
    wx_ipv4_t dest_ip;
} __attribute__((packed)) wx_ipv4_hdr_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __attribute__((packed)) wx_icmp_hdr_t;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) wx_udp_hdr_t;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t ns : 1;
    uint8_t reserved : 3;
    uint8_t data_offset : 4;
    uint8_t fin : 1;
    uint8_t syn : 1;
    uint8_t rst : 1;
    uint8_t psh : 1;
    uint8_t ack : 1;
    uint8_t urg : 1;
    uint8_t ece : 1;
    uint8_t cwr : 1;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) wx_tcp_hdr_t;

typedef struct {
    uint32_t src_ip;
    uint32_t dest_ip;
    uint8_t zero;
    uint8_t protocol;
    uint16_t length;
} __attribute__((packed)) wx_pseudo_hdr_t;

typedef struct {
    wx_mac_t local_mac;
    wx_ipv4_t local_ip;
    wx_ipv4_t netmask;
    wx_ipv4_t gateway;
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint32_t rx_bytes;
    uint32_t tx_bytes;
    uint32_t errors;
} wx_net_device_t;

static wx_net_device_t core_net_dev;

uint16_t wx_checksum(void *data, size_t len) {
    uint32_t sum = 0;
    uint16_t *p = (uint16_t *)data;
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }
    if (len == 1) {
        sum += *(uint8_t *)p;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}

typedef struct {
    wx_ipv4_t ip;
    wx_mac_t mac;
    uint32_t state;
    uint32_t ttl;
} wx_arp_entry_t;

#define WX_ARP_CACHE_SIZE 256
static wx_arp_entry_t arp_cache[WX_ARP_CACHE_SIZE];

void wx_arp_init() {
    wx_memset(arp_cache, 0, sizeof(arp_cache));
}

void wx_arp_update(wx_ipv4_t ip, wx_mac_t mac) {
    for (int i = 0; i < WX_ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].state == 1 && wx_memcmp(&arp_cache[i].ip, &ip, 4) == 0) {
            wx_memcpy(&arp_cache[i].mac, &mac, 6);
            arp_cache[i].ttl = 3000;
            return;
        }
    }
    for (int i = 0; i < WX_ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].state == 0) {
            wx_memcpy(&arp_cache[i].ip, &ip, 4);
            wx_memcpy(&arp_cache[i].mac, &mac, 6);
            arp_cache[i].state = 1;
            arp_cache[i].ttl = 3000;
            return;
        }
    }
}

int wx_arp_lookup(wx_ipv4_t ip, wx_mac_t *mac_out) {
    for (int i = 0; i < WX_ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].state == 1 && wx_memcmp(&arp_cache[i].ip, &ip, 4) == 0) {
            wx_memcpy(mac_out, &arp_cache[i].mac, 6);
            return 1;
        }
    }
    return 0;
}

void wx_e1000_tx_raw(wx_net_buf_t *buf);

void wx_arp_send_request(wx_ipv4_t target_ip) {
    wx_net_buf_t *buf = wx_net_buf_alloc();
    if (!buf) return;
    wx_net_buf_reserve(buf, 64);
    wx_arp_hdr_t *arp = (wx_arp_hdr_t *)wx_net_buf_push(buf, sizeof(wx_arp_hdr_t));
    arp->hw_type = WX_HTONS(1);
    arp->proto_type = WX_HTONS(0x0800);
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->opcode = WX_HTONS(1);
    wx_memcpy(&arp->sender_mac, &core_net_dev.local_mac, 6);
    wx_memcpy(&arp->sender_ip, &core_net_dev.local_ip, 4);
    wx_memset(&arp->target_mac, 0xFF, 6);
    wx_memcpy(&arp->target_ip, &target_ip, 4);
    wx_eth_hdr_t *eth = (wx_eth_hdr_t *)wx_net_buf_push(buf, sizeof(wx_eth_hdr_t));
    wx_memset(&eth->dest, 0xFF, 6);
    wx_memcpy(&eth->src, &core_net_dev.local_mac, 6);
    eth->ethertype = WX_HTONS(0x0806);
    wx_e1000_tx_raw(buf);
}

void wx_arp_process(wx_net_buf_t *buf) {
    wx_arp_hdr_t *arp = (wx_arp_hdr_t *)&buf->buffer[buf->data];
    if (WX_NTOHS(arp->hw_type) != 1 || WX_NTOHS(arp->proto_type) != 0x0800) {
        wx_net_buf_free(buf);
        return;
    }
    wx_arp_update(arp->sender_ip, arp->sender_mac);
    if (WX_NTOHS(arp->opcode) == 1 && wx_memcmp(&arp->target_ip, &core_net_dev.local_ip, 4) == 0) {
        wx_net_buf_t *rep = wx_net_buf_alloc();
        if (rep) {
            wx_net_buf_reserve(rep, 64);
            wx_arp_hdr_t *r_arp = (wx_arp_hdr_t *)wx_net_buf_push(rep, sizeof(wx_arp_hdr_t));
            r_arp->hw_type = WX_HTONS(1);
            r_arp->proto_type = WX_HTONS(0x0800);
            r_arp->hw_len = 6;
            r_arp->proto_len = 4;
            r_arp->opcode = WX_HTONS(2);
            wx_memcpy(&r_arp->sender_mac, &core_net_dev.local_mac, 6);
            wx_memcpy(&r_arp->sender_ip, &core_net_dev.local_ip, 4);
            wx_memcpy(&r_arp->target_mac, &arp->sender_mac, 6);
            wx_memcpy(&r_arp->target_ip, &arp->sender_ip, 4);
            wx_eth_hdr_t *eth = (wx_eth_hdr_t *)wx_net_buf_push(rep, sizeof(wx_eth_hdr_t));
            wx_memcpy(&eth->dest, &arp->sender_mac, 6);
            wx_memcpy(&eth->src, &core_net_dev.local_mac, 6);
            eth->ethertype = WX_HTONS(0x0806);
            wx_e1000_tx_raw(rep);
        }
    }
    wx_net_buf_free(buf);
}

void wx_ip_send(wx_net_buf_t *buf, wx_ipv4_t dest_ip, uint8_t proto) {
    wx_mac_t dmac;
    int has_mac = 0;
    if (dest_ip.ip[0] == 255 && dest_ip.ip[1] == 255 && dest_ip.ip[2] == 255 && dest_ip.ip[3] == 255) {
        wx_memset(&dmac, 0xFF, 6);
        has_mac = 1;
    } else {
        has_mac = wx_arp_lookup(dest_ip, &dmac);
        if (!has_mac) {
            wx_arp_send_request(dest_ip);
            wx_net_buf_free(buf);
            return;
        }
    }
    wx_ipv4_hdr_t *ip = (wx_ipv4_hdr_t *)wx_net_buf_push(buf, sizeof(wx_ipv4_hdr_t));
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->total_len = WX_HTONS(buf->tail - buf->data);
    ip->id = WX_HTONS(0x1337);
    ip->frag_offset = 0;
    ip->ttl = 64;
    ip->protocol = proto;
    ip->checksum = 0;
    wx_memcpy(&ip->src_ip, &core_net_dev.local_ip, 4);
    wx_memcpy(&ip->dest_ip, &dest_ip, 4);
    ip->checksum = wx_checksum(ip, sizeof(wx_ipv4_hdr_t));
    wx_eth_hdr_t *eth = (wx_eth_hdr_t *)wx_net_buf_push(buf, sizeof(wx_eth_hdr_t));
    wx_memcpy(&eth->dest, &dmac, 6);
    wx_memcpy(&eth->src, &core_net_dev.local_mac, 6);
    eth->ethertype = WX_HTONS(0x0800);
    wx_e1000_tx_raw(buf);
}

void wx_icmp_process(wx_net_buf_t *buf, wx_ipv4_hdr_t *ip_hdr) {
    wx_icmp_hdr_t *icmp = (wx_icmp_hdr_t *)&buf->buffer[buf->data];
    if (icmp->type == 8 && icmp->code == 0) {
        uint16_t id = icmp->id;
        uint16_t seq = icmp->seq;
        uint16_t icmp_len = WX_NTOHS(ip_hdr->total_len) - (ip_hdr->ihl * 4);
        wx_net_buf_t *rep = wx_net_buf_alloc();
        if (!rep) {
            wx_net_buf_free(buf);
            return;
        }
        wx_net_buf_reserve(rep, 128);
        void *payload = wx_net_buf_put(rep, icmp_len - sizeof(wx_icmp_hdr_t));
        wx_memcpy(payload, &buf->buffer[buf->data + sizeof(wx_icmp_hdr_t)], icmp_len - sizeof(wx_icmp_hdr_t));
        wx_icmp_hdr_t *r_icmp = (wx_icmp_hdr_t *)wx_net_buf_push(rep, sizeof(wx_icmp_hdr_t));
        r_icmp->type = 0;
        r_icmp->code = 0;
        r_icmp->checksum = 0;
        r_icmp->id = id;
        r_icmp->seq = seq;
        r_icmp->checksum = wx_checksum(r_icmp, icmp_len);
        wx_ip_send(rep, ip_hdr->src_ip, 1);
    }
    wx_net_buf_free(buf);
}

typedef struct {
    uint32_t local_port;
    uint32_t remote_port;
    wx_ipv4_t remote_ip;
    uint32_t state;
    uint32_t snd_una;
    uint32_t snd_nxt;
    uint32_t rcv_nxt;
    uint32_t snd_wnd;
    uint32_t rcv_wnd;
    uint8_t used;
} wx_tcp_tcb_t;

#define WX_TCP_MAX_SOCKS 1024
static wx_tcp_tcb_t tcp_sockets[WX_TCP_MAX_SOCKS];

void wx_tcp_init() {
    wx_memset(tcp_sockets, 0, sizeof(tcp_sockets));
}

void wx_tcp_process(wx_net_buf_t *buf, wx_ipv4_hdr_t *ip_hdr) {
    wx_tcp_hdr_t *tcp = (wx_tcp_hdr_t *)&buf->buffer[buf->data];
    uint16_t src_port = WX_NTOHS(tcp->src_port);
    uint16_t dst_port = WX_NTOHS(tcp->dest_port);
    wx_tcp_tcb_t *tcb = NULL;
    for (int i = 0; i < WX_TCP_MAX_SOCKS; i++) {
        if (tcp_sockets[i].used && tcp_sockets[i].local_port == dst_port && tcp_sockets[i].remote_port == src_port && wx_memcmp(&tcp_sockets[i].remote_ip, &ip_hdr->src_ip, 4) == 0) {
            tcb = &tcp_sockets[i];
            break;
        }
    }
    if (!tcb) {
        for (int i = 0; i < WX_TCP_MAX_SOCKS; i++) {
            if (tcp_sockets[i].used && tcp_sockets[i].local_port == dst_port && tcp_sockets[i].state == 1) {
                tcb = &tcp_sockets[i];
                break;
            }
        }
    }
    if (tcb) {
        if (tcb->state == 1 && tcp->syn && !tcp->ack) {
            tcb->remote_port = src_port;
            wx_memcpy(&tcb->remote_ip, &ip_hdr->src_ip, 4);
            tcb->rcv_nxt = WX_NTOHL(tcp->seq_num) + 1;
            tcb->snd_nxt = 0x89ABCDEF;
            tcb->snd_una = tcb->snd_nxt;
            tcb->state = 2;
            wx_net_buf_t *resp = wx_net_buf_alloc();
            if (resp) {
                wx_net_buf_reserve(resp, 128);
                wx_tcp_hdr_t *r_tcp = (wx_tcp_hdr_t *)wx_net_buf_push(resp, sizeof(wx_tcp_hdr_t));
                wx_memset(r_tcp, 0, sizeof(wx_tcp_hdr_t));
                r_tcp->src_port = WX_HTONS(tcb->local_port);
                r_tcp->dest_port = WX_HTONS(tcb->remote_port);
                r_tcp->seq_num = WX_HTONL(tcb->snd_nxt);
                r_tcp->ack_num = WX_HTONL(tcb->rcv_nxt);
                r_tcp->data_offset = 5;
                r_tcp->syn = 1;
                r_tcp->ack = 1;
                r_tcp->window_size = WX_HTONS(8192);
                r_tcp->checksum = 0;
                wx_pseudo_hdr_t psh;
                wx_memcpy(&psh.src_ip, &core_net_dev.local_ip, 4);
                wx_memcpy(&psh.dest_ip, &tcb->remote_ip, 4);
                psh.zero = 0;
                psh.protocol = 6;
                psh.length = WX_HTONS(sizeof(wx_tcp_hdr_t));
                uint32_t csum = 0;
                uint16_t *p = (uint16_t *)&psh;
                for (size_t i = 0; i < sizeof(psh)/2; i++) csum += p[i];
                p = (uint16_t *)r_tcp;
                for (size_t i = 0; i < sizeof(wx_tcp_hdr_t)/2; i++) csum += p[i];
                while (csum >> 16) csum = (csum & 0xFFFF) + (csum >> 16);
                r_tcp->checksum = ~csum;
                wx_ip_send(resp, tcb->remote_ip, 6);
                tcb->snd_nxt++;
            }
        } else if (tcb->state == 2 && tcp->ack) {
            if (WX_NTOHL(tcp->ack_num) == tcb->snd_nxt) {
                tcb->state = 3;
            }
        }
    }
    wx_net_buf_free(buf);
}

void wx_udp_process(wx_net_buf_t *buf, wx_ipv4_hdr_t *ip_hdr) {
    wx_udp_hdr_t *udp = (wx_udp_hdr_t *)&buf->buffer[buf->data];
    uint16_t len = WX_NTOHS(udp->length);
    if (len >= 8) {
        wx_net_buf_pull(buf, 8);
    }
    wx_net_buf_free(buf);
}

void wx_ip_process(wx_net_buf_t *buf) {
    wx_ipv4_hdr_t *ip = (wx_ipv4_hdr_t *)&buf->buffer[buf->data];
    if (ip->version != 4 || ip->ihl < 5) {
        wx_net_buf_free(buf);
        return;
    }
    if (wx_memcmp(&ip->dest_ip, &core_net_dev.local_ip, 4) != 0 && ip->dest_ip.ip[3] != 255) {
        wx_net_buf_free(buf);
        return;
    }
    wx_net_buf_pull(buf, ip->ihl * 4);
    switch (ip->protocol) {
        case 1:
            wx_icmp_process(buf, ip);
            break;
        case 6:
            wx_tcp_process(buf, ip);
            break;
        case 17:
            wx_udp_process(buf, ip);
            break;
        default:
            wx_net_buf_free(buf);
            break;
    }
}

void wx_eth_process(wx_net_buf_t *buf) {
    wx_eth_hdr_t *eth = (wx_eth_hdr_t *)&buf->buffer[buf->data];
    uint16_t type = WX_NTOHS(eth->ethertype);
    wx_net_buf_pull(buf, sizeof(wx_eth_hdr_t));
    if (type == 0x0806) {
        wx_arp_process(buf);
    } else if (type == 0x0800) {
        wx_ip_process(buf);
    } else {
        wx_net_buf_free(buf);
    }
}

#define E1000_REG_CTRL      0x0000
#define E1000_REG_STATUS    0x0008
#define E1000_REG_EEPROM    0x0014
#define E1000_REG_CTRL_EXT  0x0018
#define E1000_REG_ICR       0x00C0
#define E1000_REG_RCTL      0x0100
#define E1000_REG_TCTL      0x0400
#define E1000_REG_RDBAL     0x2800
#define E1000_REG_RDBAH     0x2804
#define E1000_REG_RDLEN     0x2808
#define E1000_REG_RDH       0x2810
#define E1000_REG_RDT       0x2818
#define E1000_REG_TDBAL     0x3800
#define E1000_REG_TDBAH     0x3804
#define E1000_REG_TDLEN     0x3808
#define E1000_REG_TDH       0x3810
#define E1000_REG_TDT       0x3818
#define E1000_REG_RAL       0x5400
#define E1000_REG_RAH       0x5404

struct wx_e1000_rx_desc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed));

struct wx_e1000_tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  sta;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed));

#define WX_E1000_NUM_RX_DESC 128
#define WX_E1000_NUM_TX_DESC 128

static struct wx_e1000_rx_desc e1000_rx_ring[WX_E1000_NUM_RX_DESC] __attribute__((aligned(16)));
static struct wx_e1000_tx_desc e1000_tx_ring[WX_E1000_NUM_TX_DESC] __attribute__((aligned(16)));
static uint8_t e1000_rx_buffers[WX_E1000_NUM_RX_DESC][2048] __attribute__((aligned(16)));
static uint8_t e1000_tx_buffers[WX_E1000_NUM_TX_DESC][2048] __attribute__((aligned(16)));

static int e1000_rx_ptr = 0;
static int e1000_tx_ptr = 0;
static uint32_t e1000_base = 0;

void wx_e1000_write_mac() {
    uint32_t mac_low = mmio_read32(e1000_base + E1000_REG_RAL);
    uint32_t mac_high = mmio_read32(e1000_base + E1000_REG_RAH);
    if (mac_low != 0 || mac_high != 0) {
        core_net_dev.local_mac.mac[0] = (uint8_t)(mac_low & 0xFF);
        core_net_dev.local_mac.mac[1] = (uint8_t)((mac_low >> 8) & 0xFF);
        core_net_dev.local_mac.mac[2] = (uint8_t)((mac_low >> 16) & 0xFF);
        core_net_dev.local_mac.mac[3] = (uint8_t)((mac_low >> 24) & 0xFF);
        core_net_dev.local_mac.mac[4] = (uint8_t)(mac_high & 0xFF);
        core_net_dev.local_mac.mac[5] = (uint8_t)((mac_high >> 8) & 0xFF);
    } else {
        core_net_dev.local_mac.mac[0] = 0x52;
        core_net_dev.local_mac.mac[1] = 0x54;
        core_net_dev.local_mac.mac[2] = 0x00;
        core_net_dev.local_mac.mac[3] = 0x12;
        core_net_dev.local_mac.mac[4] = 0x34;
        core_net_dev.local_mac.mac[5] = 0x56;
    }
}

void wx_e1000_init(uint32_t base_addr) {
    e1000_base = base_addr;
    wx_e1000_write_mac();
    core_net_dev.local_ip.ip[0] = 192;
    core_net_dev.local_ip.ip[1] = 168;
    core_net_dev.local_ip.ip[2] = 1;
    core_net_dev.local_ip.ip[3] = 100;
    uint32_t ctrl = mmio_read32(e1000_base + E1000_REG_CTRL);
    mmio_write32(e1000_base + E1000_REG_CTRL, ctrl | (1 << 26));
    for (int i = 0; i < 10000; i++) {
        __asm__ volatile ("pause");
    }
    for (int i = 0; i < WX_E1000_NUM_RX_DESC; i++) {
        e1000_rx_ring[i].addr = (uint64_t)(uintptr_t)e1000_rx_buffers[i];
        e1000_rx_ring[i].status = 0;
    }
    mmio_write32(e1000_base + E1000_REG_RDBAL, (uint32_t)(uintptr_t)e1000_rx_ring);
    mmio_write32(e1000_base + E1000_REG_RDBAH, 0);
    mmio_write32(e1000_base + E1000_REG_RDLEN, WX_E1000_NUM_RX_DESC * sizeof(struct wx_e1000_rx_desc));
    mmio_write32(e1000_base + E1000_REG_RDH, 0);
    mmio_write32(e1000_base + E1000_REG_RDT, WX_E1000_NUM_RX_DESC - 1);
    mmio_write32(e1000_base + E1000_REG_RCTL, (1 << 1) | (1 << 15) | (1 << 4) | (0 << 16));
    for (int i = 0; i < WX_E1000_NUM_TX_DESC; i++) {
        e1000_tx_ring[i].addr = (uint64_t)(uintptr_t)e1000_tx_buffers[i];
        e1000_tx_ring[i].cmd = 0;
        e1000_tx_ring[i].sta = 1;
    }
    mmio_write32(e1000_base + E1000_REG_TDBAL, (uint32_t)(uintptr_t)e1000_tx_ring);
    mmio_write32(e1000_base + E1000_REG_TDBAH, 0);
    mmio_write32(e1000_base + E1000_REG_TDLEN, WX_E1000_NUM_TX_DESC * sizeof(struct wx_e1000_tx_desc));
    mmio_write32(e1000_base + E1000_REG_TDH, 0);
    mmio_write32(e1000_base + E1000_REG_TDT, 0);
    mmio_write32(e1000_base + E1000_REG_TCTL, (1 << 1) | (1 << 3) | (0x0F << 4) | (0x3F << 12));
    wx_arp_init();
    wx_tcp_init();
}

void wx_e1000_tx_raw(wx_net_buf_t *buf) {
    uint32_t len = buf->tail - buf->data;
    if (len > 2048) {
        wx_net_buf_free(buf);
        return;
    }
    wx_memcpy(e1000_tx_buffers[e1000_tx_ptr], &buf->buffer[buf->data], len);
    e1000_tx_ring[e1000_tx_ptr].length = len;
    e1000_tx_ring[e1000_tx_ptr].cmd = (1 << 3) | (1 << 1) | (1 << 0);
    e1000_tx_ring[e1000_tx_ptr].sta = 0;
    e1000_tx_ptr = (e1000_tx_ptr + 1) % WX_E1000_NUM_TX_DESC;
    mmio_write32(e1000_base + E1000_REG_TDT, e1000_tx_ptr);
    while (!(e1000_tx_ring[(e1000_tx_ptr - 1 + WX_E1000_NUM_TX_DESC) % WX_E1000_NUM_TX_DESC].sta & 0x0F)) {
        __asm__ volatile ("pause");
    }
    core_net_dev.tx_packets++;
    core_net_dev.tx_bytes += len;
    wx_net_buf_free(buf);
}

void wx_e1000_poll() {
    while (e1000_rx_ring[e1000_rx_ptr].status & 0x01) {
        uint16_t len = e1000_rx_ring[e1000_rx_ptr].length;
        wx_net_buf_t *buf = wx_net_buf_alloc();
        if (buf) {
            void *data = wx_net_buf_put(buf, len);
            wx_memcpy(data, e1000_rx_buffers[e1000_rx_ptr], len);
            core_net_dev.rx_packets++;
            core_net_dev.rx_bytes += len;
            wx_eth_process(buf);
        }
        e1000_rx_ring[e1000_rx_ptr].status = 0;
        mmio_write32(e1000_base + E1000_REG_RDT, e1000_rx_ptr);
        e1000_rx_ptr = (e1000_rx_ptr + 1) % WX_E1000_NUM_RX_DESC;
    }
}

int wx_socket(int domain, int type, int protocol) {
    if (domain != 2 || type != 1 || protocol != 6) return -1;
    for (int i = 0; i < WX_TCP_MAX_SOCKS; i++) {
        if (!tcp_sockets[i].used) {
            tcp_sockets[i].used = 1;
            tcp_sockets[i].state = 0;
            return i;
        }
    }
    return -1;
}

int wx_bind(int sockfd, uint32_t port) {
    if (sockfd < 0 || sockfd >= WX_TCP_MAX_SOCKS || !tcp_sockets[sockfd].used) return -1;
    tcp_sockets[sockfd].local_port = port;
    return 0;
}

int wx_listen(int sockfd, int backlog) {
    if (sockfd < 0 || sockfd >= WX_TCP_MAX_SOCKS || !tcp_sockets[sockfd].used) return -1;
    tcp_sockets[sockfd].state = 1;
    return 0;
}

void internetmain() {
    uint32_t nic_base = 0xFEB00000;
    wx_e1000_init(nic_base);
    int srv_sock = wx_socket(2, 1, 6);
    wx_bind(srv_sock, 80);
    wx_listen(srv_sock, 10);
    while (1) {
        wx_e1000_poll();
        if (wx_inb(0x64) & 1) {
            char cmd = wx_inb(0x60);
            if (cmd == 0x10) break;
        }
    }
}

#endif


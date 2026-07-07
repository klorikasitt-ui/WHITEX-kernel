#ifndef PING_H
#define PING_H

typedef struct {
    uint16_t seq;
    uint16_t id;
} wx_ping_t;

static void wx_ping_send(wx_ipv4_t target_ip, uint16_t id, uint16_t seq) {
    wx_net_buf_t *buf = wx_net_buf_alloc();
    if (!buf) return;

    wx_net_buf_reserve(buf, 128);

    uint8_t *payload = (uint8_t *)wx_net_buf_put(buf, 4);
    payload[0] = 0xDE; payload[1] = 0xAD; payload[2] = 0xBE; payload[3] = 0xEF;

    wx_icmp_hdr_t *icmp = (wx_icmp_hdr_t *)wx_net_buf_push(buf, sizeof(wx_icmp_hdr_t));
    icmp->type = 8;
    icmp->code = 0;
    icmp->id = id;
    icmp->seq = seq;
    icmp->checksum = 0;

    icmp->checksum = wx_checksum(icmp, sizeof(wx_icmp_hdr_t) + 4);

    wx_ip_send(buf, target_ip, 1);
}

void printnum(int n) {
    if (n == 0) {
        print("0");
        return;
    }
    char buffer[12];
    int i = 0;
    while (n > 0) {
        buffer[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (--i >= 0) {
        char s[2] = {buffer[i], '\0'};
        print(s);
    }
}

static inline void ping() {
    
    

        wx_ipv4_t target_ip;
        target_ip.ip[0] = 127;
        target_ip.ip[1] = 0;
        target_ip.ip[2] = 0;
        target_ip.ip[3] = 1;

        printnum(127);
        printnum(0);
        printnum(0);
        printnum(1);

        wx_ping_send(target_ip, 0x1337, 0x0001);
    }


#endif

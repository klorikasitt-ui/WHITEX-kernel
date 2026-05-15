
#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

uint8_t get_rtc_reg(int reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

uint8_t bcd_to_bin(uint8_t val) {
    return (val & 0x0F) + ((val / 16) * 10);
}

void time() {
    uint8_t s, m, h;
    unsigned short* vga = (unsigned short*)0xB8000;
    char t[8];

    while (1) {
        outb(CMOS_ADDR, 0x0A);
        if (!(inb(CMOS_DATA) & 0x80)) {
            s = bcd_to_bin(get_rtc_reg(0x00));
            m = bcd_to_bin(get_rtc_reg(0x02));
            h = bcd_to_bin(get_rtc_reg(0x04));

            t[0] = (h / 10) + '0';
            t[1] = (h % 10) + '0';
            t[2] = ':';
            t[3] = (m / 10) + '0';
            t[4] = (m % 10) + '0';
            t[5] = ':';
            t[6] = (s / 10) + '0';
            t[7] = (s % 10) + '0';

            for (int i = 0; i < 8; i++) {
                vga[72 + i] = (unsigned short)t[i] | (0x0E << 8);
            }
        }

        if (inb(0x64) & 0x01) {
            if (inb(0x60) == 0x10) {
                break;
            }
        }

        for (volatile int i = 0; i < 1000000; i++);
    }
}

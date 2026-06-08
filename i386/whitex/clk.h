/*
 * WhiteX 
 * DEVELOPER BURAK YAKUB GÜÇER
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define PIT_FREQ 1193182
#define TARGET_FREQ 100
#define PIT_COMMAND 0x43
#define PIT_DATA0 0x40

volatile unsigned int ticks = 0;
int seconds = 0, minutes = 0, hours = 12;

void itoa(int n, char* s) {
    s[0] = (n / 10) + '0';
    s[1] = (n % 10) + '0';
    s[2] = '\0';
}

void pit_init() {
    unsigned int divisor = PIT_FREQ / TARGET_FREQ;
    outb(PIT_COMMAND, 0x36);
    outb(PIT_DATA0, (unsigned char)(divisor & 0xFF));
    outb(PIT_DATA0, (unsigned char)((divisor >> 8) & 0xFF));
}

void timer_handler() {
    ticks++;
    if (ticks % 100 == 0) {
        seconds++;
        if (seconds >= 60) {
            seconds = 0;
            minutes++;
            if (minutes >= 60) {
                minutes = 0;
                hours++;
                if (hours >= 24) hours = 0;
            }
        }

        char h_str[3], m_str[3], s_str[3];
        itoa(hours, h_str);
        itoa(minutes, m_str);
        itoa(seconds, s_str);
        cls();

        print("\rWorld Clock: ");
        print(h_str);
        print(":");
        print(m_str);
        print(":");
        print(s_str);
    }
}

void clockmain() {
    pit_init();

    while (1) {
        timer_handler();
        for(volatile int i = 0; i < 1000000; i++); 
    }
}
  
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
static void delay(volatile unsigned long count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

void play_tone(unsigned int frequency) {
    if (frequency == 0) {
        outb(0x61, inb(0x61) & 0xFC);
        return;
    }
    unsigned int divisor = 1193180 / frequency;
    outb(0x43, 0xB6);
    outb(0x42, (unsigned char)(divisor & 0xFF));
    outb(0x42, (unsigned char)((divisor >> 8) & 0xFF));
    outb(0x61, inb(0x61) | 0x03);
}

void melodi() {
    
    static const unsigned int melody[] = {
        330, 392, 440, 523, 

        659, 659, 0, 659,   
        523, 523, 0, 587
    };
    
    static const unsigned int durations[] = {
        1, 1, 1, 1, 
        2, 1, 1, 2, 
        1, 1, 1, 4
    };

    
    unsigned long base_speed = 4000000; 

    for (int i = 0; i < 12; i++) {
        if (melody[i] != 0) {
            play_tone(melody[i]);
        } else {
            play_tone(0);
        }
        
        delay(base_speed * durations[i]);
     
        play_tone(0);
        delay(base_speed / 4);
    }
}
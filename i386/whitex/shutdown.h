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
void shutdown() {
    __asm__ __volatile__ (
        "mov $0x5301, %%ax\n\t"
        "xor %%bx, %%bx\n\t"
        "int $0x15\n\t"   // Bağlantı kur
        "mov $0x530e, %%ax\n\t"
        "mov $0x0102, %%cx\n\t"
        "int $0x15\n\t"   
        "mov $0x5307, %%ax\n\t"
        "mov $0x0001, %%bx\n\t"
        "mov $0x0003, %%cx\n\t"
        "int $0x15"    
        : : : "ax", "bx", "cx"
    );
}

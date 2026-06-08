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
#ifndef MALLOC_H
#define MALLOC_H

#include <stdint.h>
#include <stddef.h>

#define HEAP_SIZE 65536
#define BLOCK_FREE 0
#define BLOCK_USED 1

typedef struct {
    size_t size;
    uint8_t status;
} block_header_t;


static uint8_t memory_heap[HEAP_SIZE];
static int first = 1;

static void ram_print_number(size_t n) {
    if (n == 0) { putchar('0'); return; }
    char buf[12]; int i = 0;
    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
    while (--i >= 0) putchar(buf[i]);
}

static size_t get_free_ram(void) {
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

static void* whitex_malloc(size_t size) {
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

static void whitex_free(void* ptr) {
    if (!ptr) return;
    block_header_t* header = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    header->status = BLOCK_FREE;
}

static void ram(void) {
    if (first) {
        for (size_t i = 0; i < HEAP_SIZE; i++) memory_heap[i] = 0;
        first = 0;
    }
    print("Free RAM: ");
    ram_print_number(get_free_ram());
    print(" bytes\n");
}

#endif

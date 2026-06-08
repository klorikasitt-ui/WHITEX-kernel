
#ifndef ERRNU_H
#define ERRNU_H
#define ERR_OK              0x00
#define ERR_UNKNOWN         0x01
#define ERR_NOT_IMPLEMENTED 0x02
#define ERR_TIMEOUT         0x03
#define ERR_BUSY            0x04
#define ERR_DIV_BY_ZERO     0x10
#define ERR_OVERFLOW        0x11
#define ERR_UNDERFLOW       0x12
#define ERR_INVALID_OP      0x13
#define ERR_SQRT_NEG        0x14
#define ERR_MEM_FAULT       0x20
#define ERR_MEM_OUT_OF_MEM  0x21
#define ERR_MEM_PROTECTION  0x22
#define ERR_MEM_ALIGNMENT   0x23
#define ERR_MEM_INVALID     0x24
#define ERR_IO_READ_FAIL    0x30
#define ERR_IO_WRITE_FAIL   0x31
#define ERR_IO_NOT_FOUND    0x32
#define ERR_IO_DISK_FULL    0x33
#define ERR_IO_PERMISSION   0x34
#define ERR_CPU_INVALID_INS 0x40
#define ERR_CPU_GPF         0x41 
#define ERR_CPU_STACK_OVF   0x42
#define ERR_CPU_DIV_ERR     0x43
#define ERR_CPU_SSE_FAIL    0x44
#define ERR_HW_KEYBOARD     0x50
#define ERR_HW_DISPLAY      0x51
#define ERR_HW_MOUSE        0x52
#define ERR_HW_NET_FAIL     0x53
#endif

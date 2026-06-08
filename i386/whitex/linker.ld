ENTRY(_start)
PHDRS
{
    headers PT_PHDR PHDRS ;
    text PT_LOAD FILEHDR PHDRS ;
    data PT_LOAD ;
}

SECTIONS
{
   
    . = 0x100000;

    .text : 
    {
       
        KEEP(*(.multiboot))
        *(.text)
        *(.text.*)
    } :text

    .rodata : ALIGN(4K) { *(.rodata*) } :text
    .data   : ALIGN(4K) { *(.data) } :data
    .bss    : ALIGN(4K) { *(COMMON) *(.bss) } :data
}

#COMPILE.SH
#First, enter this.
clang --target=i686-elf -m32 -ffreestanding  -c *.c -o kernel.o -Wall
#Write this from now on
ld -m elf_i386 -T linker.ld -o kernelll boot.o kernel.o
#Write this to test
qemu-system-i386 -kernel kernelll -vnc :1
#The `-vnc :1` here is for things like Termux that don't use GTK, so if you have a desktop environment, you wouldn't use it.

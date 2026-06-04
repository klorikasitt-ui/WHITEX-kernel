#COMPILE.SH
#Run it inside the i386/whitex folder.
#Place the boot.asm file in the whitex folder; otherwise, it will not work. 
#First, enter this.
clang --target=i686-elf -m32 -ffreestanding  -c *.c -o kernel.o -Wall
nasm -f elf32 boot.asm -o boot.o
#Write this from now on
ld -m elf_i386 -T linker.ld -o kernelll boot.o kernel.o
#Write this to test
qemu-system-i386 -kernel kernelll -vnc :1
#The `-vnc :1` here is for things like Termux that don't use GTK, so if you have a desktop environment, you wouldn't use it.

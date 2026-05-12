#include "io.h"
#include "strcmp.h"
#include "../H/vga.h"
#include "../H/keyboard.h"
#include "logo.h"
#include "cls.h"
#include "help.h"
#include "uname.h"
#include "reboot.h"
#include "echo.h"
#include "fs.h"
#include "shutdown.h"
#include "hexdump.h"
void Kernel() {
init(); 
init_fs();

char input[256];
logo();
print("\n");
print(" Welcome to WhiteX\n");
print(" You can learn the commands by typing `help`. \n");


while(1) {
print("whitex$ \n");
scan(input);
if (strcmp(input, "help") == 0) 
{
help();
}
else if (strcmp(input, "logo") == 0)
{
logo();
}
else if (strcmp(input, "cls") == 0)
{
cls();
}
else if (strcmp(input, "uname") == 0)
{
uname();
}
else if (strcmp(input, "reboot") == 0)
{
reboot();
}
else if (strcmp(input, "echo") == 0)
{
echo(input); 
}

else if (strcmp(input," ") == 0)
{

}
else if (strcmp(input, "shutdown") == 0)
{
shutdown();
}
else if (strcmp(input, "ls") == 0)
{
ls();
}
else if (strcmp(input, "mkdir") == 0)
{
mkdir();
}
else if (strcmp(input, "ls") ==0)
{
ls();
}
else if (strcmp(input, "cd") == 0)
{
cd();
}
else if (strcmp(input, "pwd") == 0)
{
pwd();
}
else if (strcmp(input, "hexdump") == 0)
{
hexdump();
}
else if (strcmp(input, "initfs") == 0)
{
init_fs();
}
else {
            print("Unknown command: ");
            print(input);
            print("\n");
        }
}

while(1) {
__asm__ __volatile__("hlt" : : : "memory");
}
}
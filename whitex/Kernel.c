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
void Kernel() {
init(); 
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

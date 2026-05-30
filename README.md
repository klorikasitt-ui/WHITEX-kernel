
# WhiteX Kernel (i386)
WhiteX is a stable, modular kernel developed for the i386 (32-bit) architecture. This project implements core operating system functions and provides a shell-based interface for user interaction.
## Features
 * **Modular Command Structure:** Features an easily extensible command system via the kernel_dispatch_table.
 * **System Integrity Checks:** Includes runtime stack protection and integrity monitoring using I386_STACK_GUARD.
 * **Virtual Machine Support:** Built-in capability to execute virtual machines via the VMState structure.
 * **Fault Management:** Implements a panic_handler to halt the system and report error codes during critical failures.
 * **System Calls (Syscalls):** Provides an execute_syscall mechanism for controlled access to system services.
## Initialization Sequence
Upon startup, the Kernel() function performs the following steps:
 1. **Environment Bootstrap:** Initializes the kernel environment and the global_env structure.
 2. **Hardware Init:** Configures the Global Descriptor Table (GDT), Interrupt Descriptor Table (IDT), and Programmable Interval Timer (PIT).
 3. **File System & Memory:** Initializes the file system and memory management units (init_fs, ram, Sdd).
 4. **Utilities:** Loads peripheral services such as melody playback and CPU information reporting.
## Command Set
The following commands are available in the system shell (whitex~$):
| Command | Description |
|---|---|
| help | Lists available system routines. |
| logo | Displays the WhiteX splash logo. |
| cls | Clears the screen. |
| uname | Displays system information. |
| reboot / shutdown | Restarts or shuts down the system. |
| ls / mkdir / cd / pwd | File system management commands. |
| hexdump | Performs memory or file dumps. |
| cpuid | Reports CPU architectural information. |
| notepad | Opens the text editor. |
| htop | Opens the system monitor. |
| vm | Launches the virtual machine. |
| echo | Reflects text output to the screen. |
## License
This program is free software distributed under the terms of the **GNU General Public License (GPL) version 3** (or any later version) as published by the Free Software Foundation.
*Note: Developed by Burak Yakub GÜÇER.*

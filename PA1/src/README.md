# Programming Assignment 1
Author: Earl Potters: eapo2776@colorado.edu

## Building Helloworld
The following files need to be in their respective directories to be compiled as the default Linux kernel.
- ``arch/x86/kernel/helloworld.c``: add two numbers and return the result as a user level pointer
- ``arch/x86/kernel/Makefile``: instructions on how to build the kernel including the added source
- ``arch/x86/entry/syscalls/syscall_64.tbl``: system call table with two new calls at 333 and 334
- ``include/linux/syscalls.h``: prototype functions of the new system calls


## Building
The following files need to be in their respective directories to be compiled as the default Linux kernel.
- ``arch/x86/kernel/cs3753_add.c``: add two numbers and return the result as a user level pointer
- ``arch/x86/kernel/Makefile``: instructions on how to build the kernel including the added source
- ``arch/x86/entry/syscalls/syscall_64.tbl``: system call table with two new calls at 333 and 334
- ``include/linux/syscalls.h``: prototype functions of the new system calls

Additionally, this build requires a system call at 333 that requires the ``helloworld.c`` source in ``arch/x86/kernel/``.
And, this build requeires a system call at 334 that requires the ``cs3753_add.c`` source in `` arch/x86/kernel/``.

## Testing
To compile the helloworld test program run:
``` sh
gcc helloworld-test.c -o test
./test
```
To compile the cs3753_add test program run:
``` sh
gcc cs3753_test.c -o test
./test
```
## Test
If build helloworld was successful you should see "HELLO WORLD" in ``dmesg`` or ``/var/log/syslog``

If build cs3753_add was successful you should see "The result is 6"

You can see the generated ``printk`` messages by running ``dmesg`` or by examining ``/var/log/syslog`` which I have also attached in this repo.

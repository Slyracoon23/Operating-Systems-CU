# Programming Assignment 2
Author: Earl Potters , Email: eapo2776@colorado.edu

## Description
- ``simple_char_driver.c``: A device driver with read, write, seek, open, and close operations.

- ``test_driver.c``: Test code to the the file operations of the character device driver.

- ``Makefile``: Makefile for building the device driver. It creates a loadable kernel object.

## Build
Build the LKM by running the Makefile:
```
sudo make
```
_It requires sudo to access root folders and files. Check my file - Don't be a script kiddie_ 

Build test program with:
```
gcc test_driver.c -o test_driver
```

## Test

### Insert LKM
After you build you should see kernal object ``simple_char_driver.ko``, run the following command:
```bash
sudo insmod simple_char_driver.ko
```
Now use ``lsmod `` to find your inserted KLM. Use ``grep`` to filter it out is needed.

### Create Device File

Now run the following code:
```bash
sudo mknod –m 777 /dev/simple_character_device c 240 0
```
This command will create a character device file that will use the device driver just loaded.

Check that it has been created using ``ls /dev`` and see if filename ``simple_character_device`` exists.

### Test the driver
Run the comipiled ``test_driver.c``:
 ```
 ./test_driver
 ```
 Follow the prompts. When you are done just enter e to exit the test.

# HW2 | Timer Device Driver and Application

This project is the second assignment for the Embedded System Software course, focusing on developing a timer device driver module to control FPGA devices and timer, alongside an application program to control the timer device.

## Installing and Removing

The project is divided into two main parts: the application (app) and the module.

### Setting up the application

To set up the application, follow these steps:

1. Navigate to the ```app``` directory:

   ```bash
   cd app
   ```

2. Generate the binary file and push it to the FPGA board's ```/data/local/tmp``` directory:

   ```bash
   make push
   ```
### Setting up the module

To set up the module, follow these steps:

1. Navigate to the ```module``` directory:

   ```bash
   cd module
   ```

2. Generate the module program file and push it to the FPGA board's ```/data/local/tmp``` directory:

   ```bash
   make push
   ```

### Inserting the module

After pushing the module program file to the FPGA board, you need to insert the module program to the kernel:

1. Insert the module into the kernel:

   ```bash
   insmod dev_driver.ko
   ```

2. Create the device file:

   ```bash
   mknod /dev/dev_driver c 242 0
   ```

### Removing the module

If you wish to remove the module from the kernel, follow these steps:

1. (Optional) Remove the device file:
   ```bash
   rm /dev/dev_driver
   ```

2. Remove the module from the kernel:
   ```bash
   rmmod dev_driver
   ```

## Note

* To understand the program's operation, please read the /* comments */ written in the code carefully.

## Author

* Junyeong JANG, Dept. of CS&E, Sogang University.



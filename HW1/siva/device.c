#include "device.h"

void fnd_dd(int fnd_data)
{
    int dev = open("/dev/fpga_fnd", O_RDWR);
    if (dev < 0)
    {
        perror("ERROR(device.c) : fnd open failed.\n");
        _exit(-1);
    }

    unsigned char data[4];
    int i;
    for (i = 3; i >= 0; i--)
    {
        data[i] = fnd_data % 10;
        fnd_data /= 10;
    }

    write(dev, &data, 4);

    close(dev);
}

void led_mm(unsigned char led_data)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        perror("ERROR(device.c) : led mem open failed.\n");
        _exit(-1);
    }

    unsigned long *fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                                     MAP_SHARED, fd, FPGA_BASE_ADDRESS);
    if (fpga_addr == MAP_FAILED)
    {
        perror("ERROR(device.c) : led mmap error. fd closed.\n");
        close(fd);
    }
    unsigned char *led_addr = (unsigned char *)((void *)fpga_addr + LED_ADDR);
    *led_addr = led_data;

    munmap(led_addr, 4096);
    close(fd);

    usleep(1000);
    return;
}

void lcd_dd(char *line1, char *line2)
{
    int dev = open("/dev/fpga_text_lcd", O_WRONLY);
    if (dev < 0)
    {
        perror("ERROR(device.c) : lcd open error");
        _exit(-1);
    }
    // usleep(10000);

    unsigned char str[32];
    memset(str, ' ', 32);
    strncpy(str, line1, 16);
    strncpy(str + 16, line2, 16);
    write(dev, str, 32);

    close(dev);
}

void motor_dd(unsigned char action)
{
    int dev = open("/dev/fpga_step_motor", O_WRONLY);
    if (dev < 0)
    {
        perror("ERROR(device.c) : motor open failed.\n");
        _exit(-1);
    }

    unsigned char motor_state[3] = {action, 0, 200};
    write(dev, motor_state, 3);

    close(dev);
}

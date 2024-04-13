#include "device.h"

void fnd_dd(int fnd_data)
{
    int dev = open("/dev/fpga_fnd/", O_RDWR);
    if (dev < 0)
    {
        perror("ERROR(device.c) : fnd open failed.\n");
        _exit(-1);
    }

    unsigned char data[4];
    for (int i = 3; i >= 0; i--)
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

void dot_dd(int dot_data)
{
    unsigned char fpga_number[11][10] = {
        {0x3e, 0x7f, 0x63, 0x73, 0x73, 0x6f, 0x67, 0x63, 0x7f, 0x3e}, // 0
        {0x0c, 0x1c, 0x1c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x1e}, // 1
        {0x7e, 0x7f, 0x03, 0x03, 0x3f, 0x7e, 0x60, 0x60, 0x7f, 0x7f}, // 2
        {0xfe, 0x7f, 0x03, 0x03, 0x7f, 0x7f, 0x03, 0x03, 0x7f, 0x7e}, // 3
        {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7f, 0x7f, 0x06, 0x06}, // 4
        {0x7f, 0x7f, 0x60, 0x60, 0x7e, 0x7f, 0x03, 0x03, 0x7f, 0x7e}, // 5
        {0x60, 0x60, 0x60, 0x60, 0x7e, 0x7f, 0x63, 0x63, 0x7f, 0x3e}, // 6
        {0x7f, 0x7f, 0x63, 0x63, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03}, // 7
        {0x3e, 0x7f, 0x63, 0x63, 0x7f, 0x7f, 0x63, 0x63, 0x7f, 0x3e}, // 8
        {0x3e, 0x7f, 0x63, 0x63, 0x7f, 0x3f, 0x03, 0x03, 0x03, 0x03}, // 9
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // blank
    };

    int dev = open("/dev/fpga_dot", O_WRONLY);
    if (dev < 0)
    {
        perror("ERROR(device.c) : dot open error");
        _exit(-1);
    }

    write(dev, fpga_number[dot_data], sizeof(fpga_number[dot_data]));

    close(dev);
}

void lcd_dd(unsigned char *text_data)
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
    strncpy(str, text_data, 32);
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

    unsigned char motor_state[3] = {action, 0, 125};
    write(dev, motor_state, 3);

    close(dev);
}

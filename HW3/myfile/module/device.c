/*
 * device.c -
 *
 *
 * Author : 20211584 Junyeong JANG
 */

#include "core.h"

/* FPGA DEVICE DEFINITIONS */
#define DEV_NUM 2 /* number of devices supported */
/* FND */
#define FND 0
#define FND_PA 0x08000004 /* device physical address for FND */
/* DOT */
#define DOT 1
#define DOT_PA 0x08000210

/* DOT MATRIX FONT */
static const unsigned char dot_number[11][10] = {
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

/* Connect the physical address of the device to the virtual address of process
 * via the ioremap and store(map_device()).
 */
static unsigned char *dev_addr[DEV_NUM];

/* All interface functions following do what their names say! */
int map_device()
{
    int i;
    /* Map each device's physical address to a virtual address, */
    dev_addr[FND] = ioremap(FND_PA, 0x04);
    dev_addr[DOT] = ioremap(DOT_PA, 0x10);
    /* and do check routine for mapping error if any. */
    for (i = 0; i < DEV_NUM; i++)
    {
        if (dev_addr[i] == NULL)
        {
            printk("ERROR(device.c) : device %d ioremap failed\n", i);
            return -1;
        }
    }
    return 1;
}

void unmap_device()
{
    int i;
    /* Unmap all device virtual addresses. */
    for (i = 0; i < DEV_NUM; i++)
        iounmap(dev_addr[i]);
}

void fnd_write(const int elapsed)
{
    int min, sec;
    if (elapsed >= TIME_LIMIT)
    {
        min = 0;
        sec = 0;
    }
    else
    {
        min = elapsed / 600;
        sec = (elapsed / 10) % 60;
    }
    /* Concatenate min and sec to unsigned short value. */
    unsigned short int _s_value = ((min / 10) << 12) + ((min % 10) << 8) +
                                  ((sec / 10) << 4) + (sec % 10);
    /* Write value to FND. */
    outw(_s_value, (unsigned int)dev_addr[FND]);
}

void dot_write(const int elapsed)
{
    int i;
    int hms;
    if (elapsed >= TIME_LIMIT)
        hms = DOT_BLANK;
    else
        hms = elapsed % 10;
    /* Write pattern to DOT matrix. */
    for (i = 0; i < 10; i++)
        outw(dot_number[hms][i] & 0x7F, (unsigned int)dev_addr[DOT] + i * 2);
}
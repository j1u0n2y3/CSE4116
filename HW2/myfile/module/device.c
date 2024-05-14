/*
 * device.c -
 * This source file contains functions to map/unmap physical addresses of various devices
 * to virtual addresses and perform formatted read/write operations on these devices.
 *
 * Author : 20211584 Junyeong JANG
 */

#include "core.h"

/* FPGA DEVICE DEFINITIONS */
#define DEV_NUM 5 /* Number of devices supported */
/* SWITCH */
#define SWITCH 0
#define SWITCH_PA 0x08000000 /* Device physical address for SWITCH */
/* FND */
#define FND 1
#define FND_PA 0x08000004
/* LED */
#define LED 2
#define LED_PA 0x08000016
/* DOT */
#define DOT 3
#define DOT_PA 0x08000210
/* LCD */
#define LCD 4
#define LCD_PA 0x08000090

/* DOT MATRIX FONT */
static const unsigned char dot_number[10][10] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 0 (blank)
    {0x0c, 0x1c, 0x1c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x1e}, // 1
    {0x7e, 0x7f, 0x03, 0x03, 0x3f, 0x7e, 0x60, 0x60, 0x7f, 0x7f}, // 2
    {0xfe, 0x7f, 0x03, 0x03, 0x7f, 0x7f, 0x03, 0x03, 0x7f, 0x7e}, // 3
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7f, 0x7f, 0x06, 0x06}, // 4
    {0x7f, 0x7f, 0x60, 0x60, 0x7e, 0x7f, 0x03, 0x03, 0x7f, 0x7e}, // 5
    {0x60, 0x60, 0x60, 0x60, 0x7e, 0x7f, 0x63, 0x63, 0x7f, 0x3e}, // 6
    {0x7f, 0x7f, 0x63, 0x63, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03}, // 7
    {0x3e, 0x7f, 0x63, 0x63, 0x7f, 0x7f, 0x63, 0x63, 0x7f, 0x3e}, // 8
    {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f}  // 9 (full)
};

/* Connect the physical address of the device to the virtual address of process
 * via the 'ioremap' function(map_device()) and store.
 */
static unsigned char *dev_addr[DEV_NUM];

/* All interface functions following do what their names say! */
int map_device()
{
    int i;
    /* Map each device's physical address to a virtual address, */
    dev_addr[SWITCH] = ioremap(SWITCH_PA, 0x01);
    dev_addr[FND] = ioremap(FND_PA, 0x04);
    dev_addr[LED] = ioremap(LED_PA, 0X01);
    dev_addr[DOT] = ioremap(DOT_PA, 0x10);
    dev_addr[LCD] = ioremap(LCD_PA, 0x32);
    /* and do check routine for mapping error if any. */
    for (i = 0; i < DEV_NUM; i++)
    {
        if (dev_addr[i] == NULL)
        {
            printk("ERROR(device.c) : device [%d] ioremap failed\n", i);
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

int switch_read()
{
    unsigned char dip_sw_value;
    unsigned short _s_dip_sw_value;
    /* Read switch value, */
    _s_dip_sw_value = inw((unsigned int)dev_addr[SWITCH]);
    dip_sw_value = _s_dip_sw_value & 0xFF;
    /* and return 1 if switch is on, 0 otherwise. */
    if (!dip_sw_value)
        return 1;
    else
        return 0;
}

void fnd_write(const int idx, const int symbol)
{
    unsigned short int _s_value = 0 + (symbol << (12 - 4 * idx));
    /* Write value to FND. */
    outw(_s_value, (unsigned int)dev_addr[FND]);
}

void led_write(const int symbol)
{
    unsigned short _s_value = (symbol == 0 ? 0x00 : 0x01 << (8 - symbol));
    /* Write value to LED. */
    outw(_s_value, (unsigned int)dev_addr[LED]);
}

void dot_write(const int symbol)
{
    int i;
    /* Write symbol pattern to DOT matrix. */
    for (i = 0; i < 10; i++)
        outw(dot_number[symbol][i] & 0x7F, (unsigned int)dev_addr[DOT] + i * 2);
}

void lcd_write(const char *left_up, const int right_up,
               const char *down)
{
    int i;
    unsigned char value[33]; /* Buffer to hold the formatted string to be displayed on LCD. */
    /* value[33] :
     * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
     * | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 |
     * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
     * | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 |'\0'|
     * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
     *
     * left_up   : [00, 12] (string)
     * right_up  : [13, 15] (<=3 digit right-aligned integer)
     * down      : [16, 31] (string)
     */
    /* Initialize the buffer to empty. */
    memset(value, ' ', 32);
    value[32] = '\0';

    /* Copy the left_up string into the value buffer. */
    for (i = 0; i < 13 && left_up[i] != '\0'; i++)
        value[i] = left_up[i];
    /* Format the right_up integer and store in right_up_buf.
     * Copy the right_up_buf into the value buffer,
     * starting at position 13(<=3 digit right-aligned integer).
     */
    if (right_up != -1) /* If right_up is -1, it means the timer has ended,
                         * so nothing will be filled.
                         */
    {
        char right_up_buf[4]; /* temporary buffer to hold the formatted right_up integer */
        snprintf(right_up_buf, 4, "%3d", right_up);
        for (i = 13; i < 16; i++)
            value[i] = right_up_buf[i - 13];
    }
    /* Copy the down string into the value buffer, starting at position 16. */
    for (i = 16; i < 32 && down[i - 16] != '\0'; i++)
        value[i] = down[i - 16];

    /* Write value to LCD. */
    unsigned short int _s_value = 0;
    for (i = 0; i < 32; i += 2) /* Combine two adjacent characters into
                                 * a short int and write it to the LCD.
                                 */
    {
        _s_value = (value[i] & 0xFF) << 8 | value[i + 1] & 0xFF;
        outw(_s_value, (unsigned int)dev_addr[LCD] + i);
    }
}

/*
 * device.c -
 * This source file contains functions to map/unmap physical addresses of various devices
 * to virtual addresses and perform formatted read/write operations on these devices.
 *
 * Author : 20211584 Junyeong Jang
 */

#include "core.h"
#define INT_MAX (int)0x7FFFFFFE

#define DEV_NUM 5 /* number of devices supported */
/* SWITCH */
#define SWITCH 0
#define SWITCH_PA 0x08000000 /* device physical address for SWITCH */
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

/* indications */
const char *playing = "Playing Music...";
const char *notplaying = "Ready For Music!";

/* dot matrix */
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
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // 10(blank)
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

void fnd_write(const int rem /* remaining time (sec) */)
{
    int rem_min = (rem / 60) % 100;
    int rem_sec = rem % 60;
    if (rem >= INT_MAX || rem < 0)
    {
        rem_min = 0;
        rem_sec = 0;
    }
    /* Concatenate min and sec to unsigned short value. */
    unsigned short int _s_value = ((rem_min / 10) << 12) + ((rem_min % 10) << 8) +
                                  ((rem_sec / 10) << 4) + (rem_sec % 10);
    /* Write value to FND. */
    outw(_s_value, (unsigned int)dev_addr[FND]);
}

void led_write(const int cur /* current music number */)
{
    unsigned short _s_value = (cur == 0 ? 0x00 : 0x01 << (8 - cur));
    /* Write value to LED. */
    outw(_s_value, (unsigned int)dev_addr[LED]);
}

void dot_write(const char cur /* current music number */)
{
    int i;
    int playing;
    if (cur == '#')
        playing = 10;
    else
        playing = (int)((cur - '0' + 1) % 10);

#pragma unroll 5
    /* Write pattern to DOT matrix. */
    for (i = 0; i < 10; i++)
        outw(dot_number[playing][i] & 0x7F, (unsigned int)dev_addr[DOT] + i * 2);
}

void lcd_write(const int rem /* remaining time (sec) */,
               const int dur /* duration time (sec) */)
{
    int i;
    unsigned char value[33]; /* Buffer to hold the formatted string to be displayed on LCD. */
    /* value[33] :
     * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
     * | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 |
     * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
     * | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 |'\0'|
     * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
     * [00, 15] : indication whether music is currently playing or not
     * [16, 31] : played time / duration time (e.g. 02:13 / 04:42)
     */
    /* Initialize the buffer to empty. */
    memset(value, ' ', 32);
    value[32] = '\0';
    /* NOT PLAYING */
    if (rem < 0 && dur < 0)
        memcpy(value, notplaying, 16);
    else if (rem >= INT_MAX || dur >= INT_MAX)
        memcpy(value, notplaying, 16);
    /* PLAYING */
    else
    {
        memcpy(value, playing, 16);

        int prog = dur - rem; /* played time (sec) */
        int prog_min = (prog / 60) % 100;
        int prog_sec = prog % 60;
        int dur_min = (dur / 60) % 100;
        int dur_sec = dur % 60;
        value[16] = (char)(prog_min / 10 + '0');
        value[17] = (char)(prog_min % 10 + '0');
        value[18] = ':';
        value[19] = (char)(prog_sec / 10 + '0');
        value[20] = (char)(prog_sec % 10 + '0');
        value[21] = ' ';
        value[22] = '/';
        value[23] = ' ';
        value[24] = (char)(dur_min / 10 + '0');
        value[25] = (char)(dur_min % 10 + '0');
        value[26] = ':';
        value[27] = (char)(dur_sec / 10 + '0');
        value[28] = (char)(dur_sec % 10 + '0');
    }

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

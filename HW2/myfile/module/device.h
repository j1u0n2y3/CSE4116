#ifndef _DEVICE_H_
#define _DEVICE_H_

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

int map_device();
void unmap_device();
int switch_read();
void fnd_write(const int, const int);
void led_write(const int);
void dot_write(const int);
void lcd_write(const char *, const int, const char *);

#endif /* _DEVICE_H_ */
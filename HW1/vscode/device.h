#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "main.h"

#define FPGA_BASE_ADDRESS 0x08000000
#define LED_ADDR 0x16

#define LED_BLANK (unsigned char)0x00
#define LCD_BLANK (unsigned char *)" "
#define MOTOR_OFF (unsigned char)0
#define MOTOR_ON (unsigned char)1

void fnd_dd(int);
void led_mm(unsigned char);
void lcd_dd(unsigned char *, unsigned char *);
void motor_dd(unsigned char);

#endif /* _DEVICE_H_ */
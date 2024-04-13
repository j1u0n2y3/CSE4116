#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "main.h"

#define FPGA_BASE_ADDRESS 0x08000000
#define LED_ADDR 0x16

void fnd_dd(int);
void led_mm(unsigned char);
void lcd_dd(unsigned char *, unsigned char *);
void motor_dd(unsigned char);

#endif /* _DEVICE_H_ */
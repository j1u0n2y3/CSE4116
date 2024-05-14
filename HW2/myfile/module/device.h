#ifndef _DEVICE_H_
#define _DEVICE_H_

/* FPGA DEVICE INTERFACES */
int map_device();
void unmap_device();
int switch_read();
void fnd_write(const int, const int);
void led_write(const int);
void dot_write(const int);
void lcd_write(const char *, const int, const char *);

#endif /* _DEVICE_H_ */
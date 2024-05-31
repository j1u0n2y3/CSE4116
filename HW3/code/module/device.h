#ifndef _DEVICE_H_
#define _DEVICE_H_

#define DOT_BLANK 10 /* index of blank font */

/* FPGA DEVICE INTERFACES */
int map_device();
void unmap_device();
void fnd_write(const int);
void dot_write(const int);

#endif /* _DEVICE_H_ */
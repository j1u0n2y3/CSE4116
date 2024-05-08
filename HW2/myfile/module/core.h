#ifndef _CORE_H_
#define _CORE_H_

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/version.h>

#define MAJOR_NUM 242
#define DEV_NAME "dev_driver"
#define DEV_FILE_LOC "/dev/dev_driver"

#define IOCTL_SET_OPTION _IOW(MAJOR_NUM, 1, char *)
#define IOCTL_COMMAND _IO(MAJOR_NUM, 2);

#include "timer.h"
#include "device.h"

#endif /* _CORE_H_ */
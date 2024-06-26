/*
 * module.c -
 * This source code is for a Linux kernel module that implements the integrated FPGA device driver.
 * It includes (1)replaced fop functions for device file,
 *             (2)functions for module init/exit.
 *
 * To install this module, the following steps need to be taken:
 *             $ insmod music_driver.ko
 *             $ mknod /dev/music_driver c 242 0
 *
 * To remove this module, the following steps need to be taken:
 * (optional)  $ rm /dev/music_driver
 *             $ rmmod music_driver
 *
 * Author : 20211584 Junyeong Jang
 */

#include "core.h"
#define INT_MAX (int)0x7FFFFFFE

/* 'already_open' flag for I/O(open) blocking */
enum
{
    NOT_USED = 0,
    EXCLUSIVE_OPEN = 1
};
static atomic_t already_open = ATOMIC_INIT(NOT_USED);

/* function headers */
static int m_open(struct inode *, struct file *);
static int m_close(struct inode *, struct file *);
static int m_read(struct file *, char __user *, size_t, loff_t *);
static int m_write(struct file *, char __user *, size_t, loff_t *);
static long m_ioctl(struct file *, unsigned int, unsigned long);
static struct file_operations m_fops = {
    .owner = THIS_MODULE,
    .open = m_open,
    .release = m_close,
    .read = m_read,
    .write = m_write,
    .unlocked_ioctl = m_ioctl,
};
static void m_display_init();

/* 1. Replaced fop functions for device file -
 * When a file operation is executed on the device file,
 * the corresponding (replaced) fop function is executed.
 */
/* m_open - Replace 'open' fop on device file. */
static int m_open(struct inode *inode, struct file *file)
{
    /* Compare already_open flag with NOT_USED and set it to EXCLUSIVE_OPEN if equal;
     * Otherwise, meaning that file is already opened, return error(-EBUSY).
     */
    if (atomic_cmpxchg(&already_open, NOT_USED, EXCLUSIVE_OPEN))
        return -EBUSY;
    /* Increase module usage. */
    try_module_get(THIS_MODULE);
    return 0;
}

/* m_close - Replace 'close' fop on device file. */
static int m_close(struct inode *inode, struct file *file)
{
    /* Set already_open flag to NOT_USED */
    atomic_set(&already_open, NOT_USED);
    /* Decrease module usage. */
    module_put(THIS_MODULE);

    return 0;
}

/* m_read - Replace 'read' fop on device file. */
static int m_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    /* Read the input from the RESET switch and return it. */
    return switch_read();
}

/* m_write - Replace 'write' fop on device file. */
static int m_write(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    /* Initialize display devices. */
    m_display_init();
    return 0;
}

/* m_ioctl - Replace 'ioctl' fop on device file. */
static long m_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
    int i, j;
    if (ioctl_num == IOCTL_OPTION)
    {
        /* Cast the ioctl parameter to a char pointer. */
        char *u_option = (char *)ioctl_param;
        char opt_buf[34] = {0}; /* buffer to hold the option string */
        /* Copy the user option string to the buffer. */
        if (strncpy_from_user(opt_buf, u_option, 33) < 0)
        {
            printk("ERROR(module.c) : strncpy_from_user failed\n");
            return -1;
        }

        char queue[4] = {'#', '#', '#', '#'}; /* array to hold the queue */
        int queue_top = 0;                    /* the number of elements of the queue */
        /* Loop to parse the queue from the option string. */
        for (i = 0; opt_buf[i] != '|'; i = i + 2)
            queue[queue_top++] = opt_buf[i];
        i++;

        int rem = 0; /* remainingTime */
        /* Loop to parse the remaining time from the option string. */
        for (; opt_buf[i] != '|'; i++)
            rem = rem * 10 + (int)(opt_buf[i] - '0');
        i++;

        int dur = 0; /* durationTime */
        /* Loop to parse the duration from the option string. */
        for (; opt_buf[i] != '\0'; i++)
            dur = dur * 10 + (int)(opt_buf[i] - '0');

        /* Write the first element of the queue to the dot matrix. */
        dot_write(queue[0]);
        /* Write the playing time(dur - rem) and duration to the LCD */
        lcd_write(rem, dur);
        /* Write the remaining time to the FND */
        fnd_write(rem);

        return 0;
    }
    else
    {
        /* If the ioctl number does not match IOCTL_OPTION,
         * print an error message and return -1.
         */
        printk("ERROR(module.c) : unknown ioctl number\n");
        return -1;
    }
    return -1;
}

/* m_display_init - Initialize the output devices. */
static void m_display_init()
{
    fnd_write(0);
    led_write(0);
    dot_write('#');
    lcd_write(-1, -1);
}

/* 2. Functions for module init/exit -
 * These functions are executed when this module(timer device driver) is installed/removed.
 */
/* module_init - insmod */
static int __init m_init()
{
    /* Register device driver module. */
    int res = register_chrdev(MAJOR_NUM, DEV_NAME, &m_fops);
    if (res < 0)
    {
        printk("ERROR(timer.c) : register_chrdev failed\n");
        return -1;
    }
    /* Map all FPGA devies. */
    map_device();
    /* Initialize display devices. */
    m_display_init();

    return 0;
}

/* module_exit - rmmod */
static void __exit m_exit()
{
    /* Initialize display devices. */
    m_display_init();
    /* Unmap all FPGA devies. */
    unmap_device();
    /* Unregister device driver module. */
    unregister_chrdev(MAJOR_NUM, DEV_NAME);
}

module_init(m_init);
module_exit(m_exit);

/* Module license and author */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junyeong JANG");

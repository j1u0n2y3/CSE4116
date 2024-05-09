/*
 * timer.c -
 * This source code is for a Linux kernel module that implements a timer device driver.
 * It includes (1)replaced fop functions for device file,
 *             (2)interface/utility functions for managing timer,
 *             (3)functions for module init/exit.
 *
 * To install this module, the following steps need to be taken:
 *             $ insmod dev_driver.ko
 *             $ mknod /dev/dev_driver c 242 0
 *
 * To remove this module, the following steps need to be taken:
 * (optional)  $ rm /dev/dev_driver
 *             $ rmmod dev_driver
 *
 * Author : 20211584 Junyeong JANG
 */

#include "core.h"

static TIMER timer_data;
/* */
enum
{
    NOT_USED = 0,
    EXCLUSIVE_OPEN = 1
};
static atomic_t already_open = ATOMIC_INIT(NOT_USED);
struct semaphore TIMER_END;

/* Replaced fop functions for device file HEADER */
static int timer_open(struct inode *, struct file *);
static int timer_close(struct inode *, struct file *);
static int timer_read(struct file *, char __user *, size_t, loff_t *);
static long timer_ioctl(struct file *, unsigned int, unsigned long);
static struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .release = timer_close,
    .read = timer_read,
    .unlocked_ioctl = timer_ioctl,
};

/* Interface/Utility functions for managing timer HEADER */
static int timer_atoi(const char *);
static void timer_metadata_init(const char *);
static void timer_display();
static void timer_add();
static void timer_handler(unsigned long);

/* 1. Replaced fop functions for device file -
 * When a file operation is executed on the timer device file,
 * the corresponding (replaced) fop function is executed.
 */
static int timer_open(struct inode *inode, struct file *file)
{
    if (atomic_cmpxchg(&already_open, NOT_USED, EXCLUSIVE_OPEN))
        return -EBUSY;
    try_module_get(THIS_MODULE);
    return 0;
}

static int timer_close(struct inode *inode, struct file *file)
{
    atomic_set(&already_open, NOT_USED);
    module_put(THIS_MODULE);
    return 0;
}

static int timer_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    return switch_read();
}

static long timer_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
    int i;
    switch (ioctl_num)
    {
    case IOCTL_SET_OPTION:
    {
        char *u_option = (char *)ioctl_param;
        char tmp_buf[11] = {0};
        if (strncpy_from_user(tmp_buf, u_option, 10) < 0)
        {
            printk("ERROR(timer.c) : strncpy_from_user failed\n");
            return -1;
        }
        timer_metadata_init(tmp_buf);
        timer_display();
    }
    case IOCTL_COMMAND:
    {
        del_timer_sync(&(timer_data.timer));
        timer_add();
        down_interruptible(&TIMER_END);
    }
    default:
    {
        printk("ERROR(timer.c) : unknown ioctl command\n");
        return -1;
    }
    }
    return 0;
}

/* 2. Interface/Utility functions for managing timer -
 * There are some interface functions to manage timer device.
 */
static int timer_atoi(const char *str)
{
    int i;
    int res = 0;
    for (i = 0; str[i] != '\0'; i++)
        res = res * 10 + str[i] - '0';
    return res;
}

static void timer_metadata_init(const char *option)
{
    int i;
    char buf[5] = {0};
    int u_interval, u_cnt;
    char u_init[5] = {0};
    /* interval */
    memcpy(buf, option, 3);
    u_interval = timer_atoi(buf);
    /* cnt */
    memcpy(buf, option + 3, 3);
    u_cnt = timer_atoi(buf);
    /* init */
    memcpy(u_init, option + 6, 4);

    struct metadata tmp;
    /* interval, cnt, elapsed */
    tmp.interval = u_interval;
    tmp.cnt = u_cnt;
    tmp.elapsed = 0;
    /* LCD fields */
    snprintf(tmp.left_up, 8, "20211584");
    tmp.right_up = tmp.cnt - tmp.elapsed;
    snprintf(tmp.down, 3, "JJY");
    /* FND fields */
    for (i = 0; i < 4; i++)
    {
        if (u_init[i] != '0')
        {
            tmp.fnd_idx = i;
            tmp.symbol = u_init[i] - '0';
        }
    }

    timer_data.info = tmp;
}

static void timer_display()
{
    fnd_write(timer_data.info.fnd_idx, timer_data.info.symbol);
    led_write(timer_data.info.symbol);
    dot_write(timer_data.info.symbol);
    lcd_write(timer_data.info.left_up, timer_data.info.right_up, timer_data.info.down);
}

static void timer_add()
{
    timer_data.timer.expires = get_jiffies_64() + (timer_data.info.interval * HZ / 10);
    timer_data.timer.data = (unsigned long)&timer_data;
    timer_data.timer.function = timer_handler;
    add_timer(&(timer_data.timer));
}

static void timer_handler(unsigned long timeout)
{
    TIMER *cur_t = (TIMER *)timeout;
    struct metadata tmp = cur_t->info;
    tmp.elapsed++;

    if (tmp.elapsed < tmp.cnt)
    {
        tmp.right_up = tmp.cnt - tmp.elapsed;
        if (++tmp.symbol == 9)
            tmp.symbol = 1;
        if (tmp.elapsed % 8 == 0)
            tmp.fnd_idx = (tmp.fnd_idx + 1) % 4;

        timer_data.info = tmp;
        timer_display();
        timer_add();
    }
    else if (tmp.cnt <= tmp.elapsed &&
             tmp.elapsed < tmp.cnt + 3)
    {
        snprintf(tmp.left_up, 10, "Time's up!");
        tmp.right_up = 0;
        snprintf(tmp.down, 16, "Shutdown in %d...", 3 - (tmp.elapsed - tmp.cnt));
        tmp.fnd_idx = 0;
        tmp.symbol = 0;

        timer_data.info = tmp;
        timer_display();
        timer_add();
    }
    else
    {
        snprintf(tmp.left_up, 1, " ");
        tmp.right_up = -1;
        snprintf(tmp.down, 1, " ");
        tmp.fnd_idx = 0;
        tmp.symbol = 0;

        timer_data.info = tmp;
        timer_display();
        up(&TIMER_END);
    }
}

/* 3. Functions for module init/exit -
 * These functions are executed when this module(timer device driver) is installed/removed.
 */
static int __init timer_init()
{
    int res = register_chrdev(MAJOR_NUM, DEV_NAME, &timer_fops);
    if (res < 0)
    {
        printk("ERROR(timer.c) : register_chrdev failed\n");
        return -1;
    }
    sema_init(&TIMER_END, 0);
    init_timer(&(timer_data.timer));
    map_device();

    return 0;
}

static void __exit timer_exit()
{
    unmap_device();
    del_timer_sync(&(timer_data.timer));
    unregister_chrdev(MAJOR_NUM, DEV_NAME);
}

module_init(timer_init);
module_exit(timer_exit);

/* Module license and author */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junyeong JANG");
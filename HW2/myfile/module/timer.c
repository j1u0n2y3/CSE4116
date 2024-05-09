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

/* TIMER */
static TIMER timer_data;
/* 'already_open' flag for I/O(open) blocking */
enum
{
    NOT_USED = 0,
    EXCLUSIVE_OPEN = 1
};
static atomic_t already_open = ATOMIC_INIT(NOT_USED);
/* Mutex semaphore to prevent ioctl command from exiting before the timer expires */
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
/* timer_open - Replace 'open' fop on device file. */
static int timer_open(struct inode *inode, struct file *file)
{
    if (atomic_cmpxchg(&already_open, NOT_USED, EXCLUSIVE_OPEN))
        return -EBUSY;
    try_module_get(THIS_MODULE);
    return 0;
}

/* timer_close - Replace 'close' fop on device file. */
static int timer_close(struct inode *inode, struct file *file)
{
    atomic_set(&already_open, NOT_USED);
    module_put(THIS_MODULE);
    return 0;
}

/* timer_read - Replace 'read' fop on device file. */
static int timer_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    /* Read the input from the RESET switch and return it. */
    return switch_read();
}

/* timer_ioctl - Replace 'ioctl' fop on device file. */
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
        /* Reset and register timer. */
        del_timer_sync(&(timer_data.timer));
        timer_add();
        down_interruptible(&TIMER_END); /* blocked until timer expires */
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
/* timer_atoi - Simple kernel atoi. [ASCII string -> (unsigned) int] */
static int timer_atoi(const char *str)
{
    int i;
    int res = 0;
    for (i = 0; str[i] != '\0'; i++)
        res = res * 10 + str[i] - '0';
    return res;
}

/* timer_metadata_init - Initialize timer's metadata(info field) based on option. */
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

/* timer_display - Display timer's metadata on FPGA devices. */
static void timer_display()
{
    fnd_write(timer_data.info.fnd_idx, timer_data.info.symbol);
    led_write(timer_data.info.symbol);
    dot_write(timer_data.info.symbol);
    lcd_write(timer_data.info.left_up, timer_data.info.right_up, timer_data.info.down);
}

/* timer_add - Set timer field and register timer. */
static void timer_add()
{
    timer_data.timer.expires = get_jiffies_64() +
                               (timer_data.info.interval * HZ / 10); /* 0.1 * interval (sec) */
    timer_data.timer.data = (unsigned long)&timer_data;
    timer_data.timer.function = timer_handler;
    add_timer(&(timer_data.timer));
}

/* timer_handler - Timer handler called whenever the timer expires(timer->expires == get_jiffies_64()). */
static void timer_handler(unsigned long timeout)
{
    TIMER *cur_t = (TIMER *)timeout;
    struct metadata tmp = cur_t->info;

    /* elapsed proceeds :
     * [1              ][2    ][3       )
     * +----------------+------+-------->
     * 0                cnt    cnt+3    INF
     *
     * Interval 1 : Section proceeding by user-specified number 'TIMER_CNT'.
     * Interval 2 : 3-second notification section indicating that the timer will expire.
     * Interval 3 : Expire timer. End routine.
     */
    tmp.elapsed++;
    /* Interval 1 */
    if (tmp.elapsed < tmp.cnt)
    {
        /* Update metadata (INCREASING TO TIMER_CNT). */
        tmp.right_up = tmp.cnt - tmp.elapsed;
        if (++tmp.symbol == 9) /* If symbol is out of range, rotate it. */
            tmp.symbol = 1;
        if (tmp.elapsed % 8 == 0) /* When symbol has completed one cycle,
                                   * index moves on to the next.
                                   */
            tmp.fnd_idx = (tmp.fnd_idx + 1) % 4;
        /* Display the updated information and add the timer again. */
        timer_data.info = tmp;
        timer_display();
        timer_add();
    }
    /* Interval 2 */
    else if (tmp.cnt <= tmp.elapsed &&
             tmp.elapsed < tmp.cnt + 3)
    {
        /* Update metadata (EXPIRATION NOTIFICATION). */
        snprintf(tmp.left_up, 10, "Time's up!");
        tmp.right_up = 0;
        snprintf(tmp.down, 16, "Shutdown in %d...", 3 - (tmp.elapsed - tmp.cnt));
        tmp.fnd_idx = 0;
        tmp.symbol = 0;
        /* Display the updated information and add the timer again. */
        timer_data.info = tmp;
        timer_display();
        timer_add();
    }
    /* Interval 3 */
    else
    {
        /* Update metadata (TURN-OFF ALL FPGA DEVICES). */
        snprintf(tmp.left_up, 1, " ");
        tmp.right_up = -1;
        snprintf(tmp.down, 1, " ");
        tmp.fnd_idx = 0;
        tmp.symbol = 0;
        /* Display the updated information and do semaphore-up-operation
         * to notify that the timer has expired completely and unblock module process.
         */
        timer_data.info = tmp;
        timer_display();
        up(&TIMER_END);
    }
}

/* 3. Functions for module init/exit -
 * These functions are executed when this module(timer device driver) is installed/removed.
 */
/* timer_init - insmod */
static int __init timer_init()
{
    /* Register device driver module. */
    int res = register_chrdev(MAJOR_NUM, DEV_NAME, &timer_fops);
    if (res < 0)
    {
        printk("ERROR(timer.c) : register_chrdev failed\n");
        return -1;
    }
    /* Initialize TIMER_END semaphore to 0. */
    sema_init(&TIMER_END, 0);
    /* Initialize timer. */
    init_timer(&(timer_data.timer));
    /* Map all FPGA devies. */
    map_device();

    return 0;
}

/* timer_exit - rmmod */
static void __exit timer_exit()
{
    /* Unmap all FPGA devies. */
    unmap_device();
    /* Remove timer. */
    del_timer_sync(&(timer_data.timer));
    /* Unregister device driver module. */
    unregister_chrdev(MAJOR_NUM, DEV_NAME);
}

module_init(timer_init);
module_exit(timer_exit);

/* Module license and author */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junyeong JANG");
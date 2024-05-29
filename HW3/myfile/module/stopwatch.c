/*
 * timer.c -
 *
 *
 * Author : 20211584 Junyeong JANG
 */

#include "core.h"

/* 'already_open' flag for I/O(open) blocking */
enum
{
    NOT_USED = 0,
    EXCLUSIVE_OPEN = 1
};
static atomic_t already_open = ATOMIC_INIT(NOT_USED);
/* mutex semaphore to prevent user process from exiting before the stopwatch ends */
struct semaphore STOPWATCH_QUIT;

/* HEADER: replaced fop functions for device file */
static int stopwatch_open(struct inode *, struct file *);
static int stopwatch_close(struct inode *, struct file *);
static int stopwatch_read(struct file *, char __user *, size_t, loff_t *);
static int stopwatch_write(struct file *, const char __user *, size_t, loff_t *);
static struct file_operations stopwatch_fops = {
    .owner = THIS_MODULE,
    .open = stopwatch_open,
    .release = stopwatch_close,
    .read = stopwatch_read,
    .write = stopwatch_write,
};

/* HEADER: interface/Utility functions for managing stopwatch */
static void stopwatch_display();
static void stopwatch_add();
static void stopwatch_handler(unsigned long);

/* 1. replaced fop functions for device file -
 * When a file operation is executed on the timer device file,
 * the corresponding (replaced) fop function is executed.
 */
/* stopwatch_open - Replace 'open' fop on device file. */
static int stopwatch_open(struct inode *inode, struct file *file)
{
    /* Compare already_open flag with NOT_USED and set it to EXCLUSIVE_OPEN if equal;
     * Otherwise, meaning that file is already opened, return error(-EBUSY).
     */
    if (atomic_cmpxchg(&already_open, NOT_USED, EXCLUSIVE_OPEN))
        return -EBUSY;
    /* Increase module usage. */
    try_module_get(THIS_MODULE);
    /* Initialize inturrupts and workqueue. */
    intr_init();
    return 0;
}

/* stopwatch_close - Replace 'close' fop on device file. */
static int stopwatch_close(struct inode *inode, struct file *file)
{
    /* Set already_open flag to NOT_USED */
    atomic_set(&already_open, NOT_USED);
    /* Decrease module usage. */
    module_put(THIS_MODULE);
    /* Release interrupts and destroy workqueue. */
    intr_free();
    return 0;
}

/* stopwatch_read - Replace 'read' fop on device file. */
static int stopwatch_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    stopwatch.elapsed = 0;
    stopwatch.paused = 0;
    stopwatch.reset = 1;
    stopwatch.stopped = STOP_NOT_PRESSED;

    return 1;
}

/* stopwatch_write - Replace 'write' fop on device file. */
static int stopwatch_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
    del_timer_sync(&(stopwatch.timer));
    stopwatch_add();
    down_interruptible(&STOPWATCH_QUIT);

    return 1;
}

/* 2. interface/Utility functions for managing stopwatch */
/* stopwatch_display - Display stopwatch's elapsed time data on FPGA devices. */
static void stopwatch_display()
{
    fnd_write(stopwatch.elapsed);
    dot_write(stopwatch.elapsed);
}

/* stopwatch_add - Set timer field and register timer. */
static void stopwatch_add()
{
    stopwatch.timer.expires = get_jiffies_64() + (HZ / 10);
    stopwatch.timer.data = (unsigned long)&stopwatch;
    stopwatch.timer.function = stopwatch_handler;
    add_timer(&(stopwatch_data.timer));
}

/* stopwatch_handler - Timer handler called whenever the timer expires(timer.expires == get_jiffies_64()). */
static void stopwatch_handler(unsigned long timeout)
{
    if (!stopwatch.paused && !stopwatch.reset)
        stopwatch.elapsed++;

    if (stopwatch.elapsed - stopwatch.stopped >= 30)
    {
        stopwatch.elapsed = TIME_LIMIT;
        stopwatch_display();
        up(&STOPWATCH_QUIT);
        return;
    }

    stopwatch_display();
    stopwatch_add();
}

/* 3. functions for module init/exit -
 * These functions are executed when this module(timer device driver) is installed/removed.
 */
/* stopwatch_init - insmod */
static int __init stopwatch_init()
{
    /* Register device driver module. */
    int res = register_chrdev(MAJOR_NUM, DEV_NAME, &stopwatch_fops);
    if (res < 0)
    {
        printk("ERROR(timer.c) : register_chrdev failed\n");
        return -1;
    }
    /* Initialize stopwatch_END semaphore to 0. */
    sema_init(&stopwatch_END, 0);
    /* Initialize timer. */
    init_timer(&(stopwatch.timer));
    /* Map all FPGA devies. */
    map_device();

    return 0;
}

/* stopwatch_exit - rmmod */
static void __exit stopwatch_exit()
{
    /* Unmap all FPGA devies. */
    unmap_device();
    /* Remove timer. */
    del_stopwatch_sync(&(stopwatch_data.timer));
    /* Unregister device driver module. */
    unregister_chrdev(MAJOR_NUM, DEV_NAME);
}

module_init(stopwatch_init);
module_exit(stopwatch_exit);

/* Module license and author */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junyeong JANG");
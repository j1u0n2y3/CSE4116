/*
 * timer.c -
 *
 */

#include "core.h"

static TIMER timer_data;
enum
{
    NOT_USED = 0,
    EXCLUSIVE_OPEN = 1
};
static atomic_t already_open = ATOMIC_INIT(NOT_USED);

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
}

/**/

static int __init timer_init()
{
    int res = register_chrdev(MAJOR_NUM, DEV_NAME, &timer_fops);
    if (res < 0)
    {
        printk("ERROR(timer.c) : register_chrdev failed\n");
        return -1;
    }
    init_timer(&(timer_data.timer));
    map_device();

    return 0;
}

static void __exit timer_exit()
{
    unregister_chrdev(MAJOR_NUM, DEV_NAME);

    del_timer_sync(&(timer_data.timer));
    unmap_device();
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junyeong JANG");
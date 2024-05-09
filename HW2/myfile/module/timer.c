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
static int timer_read(struct file *, char __user *, size_t, loff_t *); // user ?
static long timer_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .release = timer_close,
    .read = timer_read,
    .unlocked_ioctl = timer_ioctl,
};

static int timer_atoi(const char *);
static void timer_metadata_init(const char *);
static void timer_display();
static void timer_add();
static void timer_handler(unsigned long);
static void timer_finish();

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
        /*sema down*/
    }
    default:
    {
        printk("ERROR(timer.c) : unknown ioctl command\n");
        return -1;
    }
    }
    return 0;
}

/**/

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
    snprintf(tmp.left_up, 8, STUDENT_ID);
    tmp.right_up = tmp.cnt - tmp.elapsed;
    snprintf(tmp.down, 3, STUDENT_NAME);
    /* FND fields */
    for (i = 0; i < 4; i++)
    {
        if (u_init[i] != '0')
        {
            tmp.init_fnd_idx = i;
            tmp.init_symbol = u_init[i] - '0';
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
    }
    else if (tmp.cnt <= tmp.elapsed &&
             tmp.elapsed < tmp.cnt + 3)
    {
    }
    else
    {
        /*END(sema up)*/
        return;
    }
}

static void timer_finish()
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
    unmap_device();
    del_timer_sync(&(timer_data.timer));
    unregister_chrdev(MAJOR_NUM, DEV_NAME);
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junyeong JANG");
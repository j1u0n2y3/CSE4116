/*
 * module.c -
 * This source code is for a Linux kernel module that implements a stopwatch device driver.
 * It includes (1)replaced fop functions for device file,
 *             (2)interface/utility functions for managing timer,
 *             (3)interrupt control functions,
 *             (4)functions for module init/exit.
 *
 * To install this module, the following steps need to be taken:
 *             $ insmod stopwatch.ko
 *             $ mknod /dev/stopwatch c 242 0
 *
 * To remove this module, the following steps need to be taken:
 * (optional)  $ rm /dev/stopwatch
 *             $ rmmod stopwatch
 *
 * Author : 20211584 Junyeong JANG
 */

#include "core.h"

/* HEADER 1: replaced fop functions for device file */
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

/* HEADER 2: interface/utility functions for managing stopwatch */
static void stopwatch_display();
static void stopwatch_add();
static void stopwatch_handler(unsigned long);

/* HEADER 3: interrupt control functions */
static void intr_init();
static void intr_free();
static void stop_timer_handler(unsigned long);
static void stop_timer_add();
static irqreturn_t btn_home_handler(int, void *);
static irqreturn_t btn_back_handler(int, void *);
static irqreturn_t btn_vol_up_handler(int, void *);
static irqreturn_t btn_vol_down_handler(int, void *);
static void wq_handler(struct work_struct *);

/* STOPWATCH */
static STOPWATCH stopwatch;
/* timer to check if the stop button is pressed for 3 seconds */
static struct timer_list stop_timer;
/* 'already_open' flag for I/O(open) blocking */
enum
{
    NOT_USED = 0,
    EXCLUSIVE_OPEN = 1
};
static atomic_t already_open = ATOMIC_INIT(NOT_USED);
/* mutex semaphore to prevent user process from exiting before the stopwatch ends */
struct semaphore STOPWATCH_QUIT;

/* BUTTON DEFINITIONS */
#define BTN_NUM 4
#define BTN_HOME 0
#define BTN_BACK 1
#define BTN_VOL_UP 2
#define BTN_VOL_DOWN 3
/* button informations for gpio */
static unsigned int btn_gpio[BTN_NUM] = {
    IMX_GPIO_NR(1, 11),
    IMX_GPIO_NR(1, 12),
    IMX_GPIO_NR(2, 15),
    IMX_GPIO_NR(5, 14),
};
static irqreturn_t (*btn_handler[BTN_NUM])(int, void *) = {
    btn_home_handler,
    btn_back_handler,
    btn_vol_up_handler,
    btn_vol_down_handler,
};
static const int btn_flag[BTN_NUM] = {
    IRQF_TRIGGER_RISING,
    IRQF_TRIGGER_RISING,
    IRQF_TRIGGER_RISING,
    IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
};
static const char *btn_name[BTN_NUM] = {
    "BTN#HOME",
    "BTN#BACK",
    "BTN#VOL+",
    "BTN#VOL-",
};
/* work queue for bottom half */
static struct workqueue_struct *wqueue = NULL;
typedef struct _BTN_WORK
{
    struct work_struct work;
    int type;
} BTN_WORK;

/* 1. replaced fop functions for device file -
 * When a file operation is executed on the stopwatch device file,
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
    /* Free IRQs and destroy workqueue. */
    intr_free();
    return 0;
}

/* stopwatch_read - Replace 'read' fop on device file. */
static int stopwatch_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    /* Initialize stopwatch fields. */
    stopwatch.elapsed = 0;
    stopwatch.reset = 1;
    stopwatch.paused = 0;
    return 1;
}

/* stopwatch_write - Replace 'write' fop on device file. */
static int stopwatch_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
    del_timer_sync(&(stopwatch.timer));
    /* Update the displays. */
    stopwatch_display();
    down_interruptible(&STOPWATCH_QUIT); /* blocked until the stopwatch ends */
    return 1;
}

/* 2. interface/utility functions for managing stopwatch */
/* stopwatch_display - Display stopwatch's elapsed time data on FPGA devices. */
static void stopwatch_display()
{
    fnd_write(stopwatch.elapsed);
    dot_write(stopwatch.elapsed);
}

/* stopwatch_add - Set stopwatch's timer fields and register timer. */
static void stopwatch_add()
{
    stopwatch.timer.expires = get_jiffies_64() + (HZ / 10);
    stopwatch.timer.data = (unsigned long)&stopwatch;
    stopwatch.timer.function = stopwatch_handler;
    add_timer(&(stopwatch.timer));
}

/* stopwatch_handler - Timer handler called whenever the stopwatch's timer expires(timer.expires == get_jiffies_64()). */
static void stopwatch_handler(unsigned long timeout)
{
    stopwatch.elapsed++;
    /* End the stopwatch when the time limit is reached. */
    if(stopwatch.elapsed >= TIME_LIMIT)
    {
        stopwatch.elapsed = TIME_LIMIT;
        stopwatch_display();
        up(&STOPWATCH_QUIT);
        return;
    }
    /* Update the displays and add timer. */
    stopwatch_display();
    stopwatch_add();
}

/* 3. interrupt control functions -
 * These functions manage interrupts and perform the appropriate actions accordingly.
 */
static unsigned int intr_cnt = 0; /* count how many times interrupt was called */
static int vol_down_pressed = 0; /* check if the vol down button is pressed or released */

/* intr_init - Initialize interrupts, workqueue, and timer. */
static void intr_init()
{
    int i;
    for (i = 0; i < BTN_NUM; i++)
    {
        /* Set GPIO direction to input. */
        gpio_direction_input(btn_gpio[i]);
        /* Request IRQ for each button. */
        request_irq(gpio_to_irq(btn_gpio[i]), btn_handler[i], btn_flag[i], btn_name[i], 0);
    }
    /* Create workqueue. */
    wqueue = create_workqueue("STOPWATCH_WQ");
    /* Initialize stop_timer. */
    init_timer(&stop_timer);
    /* Initialize variables. */
    intr_cnt = 0;
    vol_down_pressed = 0;
}

/* intr_free - Free interrupts, workqueue, and timer. */
static void intr_free()
{
    int i;
    for (i = 0; i < BTN_NUM; i++)
        /* Free IRQ for each button. */
        free_irq(gpio_to_irq(btn_gpio[i]), NULL);
    /* Flush and destroy workqueue. */
    flush_workqueue(wqueue);
    destroy_workqueue(wqueue);
    /* Delete stop_timer. */
    del_timer_sync(&stop_timer);
    /* Initialize variables. */
    intr_cnt = 0;
    vol_down_pressed = 0;
}

/* stop_timer_handler - Timer handler for stop_timer. */
static void stop_timer_handler(unsigned long timeout)
{
    /* Delete stop_timer. */
    del_timer_sync(&(stopwatch.timer));
    /* Set stopwatch's elapsed time to limit to notice the end of the program
     * and update the displays.
     */
    stopwatch.elapsed = TIME_LIMIT;
    stopwatch_display();
    /* Do semaphore-up-operation to wake up the process. */
    up(&STOPWATCH_QUIT);
}

/* stop_timer_add - Add stop_timer with a 3-second expiration. */
static void stop_timer_add()
{
    stop_timer.expires = get_jiffies_64() + (3 * HZ); /* 3 (sec) */
    stop_timer.data = NULL;
    stop_timer.function = stop_timer_handler;
    /* Add the timer. */
    add_timer(&stop_timer);
}

/* btn_home_handler - Interrupt handler for the home button. */
static irqreturn_t btn_home_handler(int irq, void *data)
{
    /* TOP HALF - Handle real-time critical behaviors. */
    if (!stopwatch.paused && stopwatch.reset)
    {
        /* Start or restart the stopwatch from reset state. */
        stopwatch_add();
        /* Set the reset flag to false. */
        stopwatch.reset = 0;
    }
    /* BOTTOM HALF - Use workqueue to handle operations that take too long to be processed in the interrupt handler
     * and for which is not very real-time critical.
     */
    /* Allocate memory for bottom half work. */
    BTN_WORK *home_work = (BTN_WORK *)kmalloc(sizeof(BTN_WORK), GFP_KERNEL);
    if (home_work)
    {
        /* Initialize work. */
        INIT_WORK((struct work_struct *)home_work, wq_handler);
        /* Set the button type. */
        home_work->type = BTN_HOME;
        /* Queue work to workqueue. */
        queue_work(wqueue, (struct work_struct *)home_work);
    }
    return IRQ_HANDLED;
    /* All other interrupt handlers follow the same process. */
}

/* btn_back_handler - Interrupt handler for the back button. */
static irqreturn_t btn_back_handler(int irq, void *data)
{
    /* TOP HALF */
    if (!stopwatch.reset)
    {
        if (stopwatch.paused)
            /* Add the timer for progress. */
            stopwatch_add();
        else
            /* Delete the timer for pausing. */
            del_timer_sync(&(stopwatch.timer));
        /* Toggle the paused/progress state. */
        stopwatch.paused = 1 - stopwatch.paused;
    }
    /* BOTTOM HALF */
    BTN_WORK *back_work = (BTN_WORK *)kmalloc(sizeof(BTN_WORK), GFP_KERNEL);
    if (back_work)
    {
        INIT_WORK((struct work_struct *)back_work, wq_handler);
        back_work->type = BTN_BACK;
        queue_work(wqueue, (struct work_struct *)back_work);
    }
    return IRQ_HANDLED;
}

/* btn_vol_up_handler - Interrupt handler for the volume up button. */
static irqreturn_t btn_vol_up_handler(int irq, void *data)
{
    /* TOP HALF */
    /* Delete the timer for reset. */
    del_timer_sync(&(stopwatch.timer));
    /* Initialize stopwatch fields. */
    stopwatch.elapsed = 0;
    stopwatch.reset = 1;
    stopwatch.paused = 0;
    /* Update the displays. */
    stopwatch_display();
    /* BOTTOM HALF */
    BTN_WORK *vol_up_work = (BTN_WORK *)kmalloc(sizeof(BTN_WORK), GFP_KERNEL);
    if (vol_up_work)
    {
        INIT_WORK((struct work_struct *)vol_up_work, wq_handler);
        vol_up_work->type = BTN_VOL_UP;
        queue_work(wqueue, (struct work_struct *)vol_up_work);
    }
    return IRQ_HANDLED;
}

/* btn_vol_down_handler - Interrupt handler for the volume down button. */
static irqreturn_t btn_vol_down_handler(int irq, void *data)
{
    /* TOP HALF */
    /* Toggle the pressed/released state. */
    vol_down_pressed = 1 - vol_down_pressed;
    if (vol_down_pressed)
        stop_timer_add(); /* Add stop_timer to time how long the vol down button is pressed. */
    else
        del_timer_sync(&stop_timer); /* Delete the stop_timer 
                                      * because the button was released before 3 seconds.
                                      */
    /* BOTTOM HALF */
    BTN_WORK *vol_down_work = (BTN_WORK *)kmalloc(sizeof(BTN_WORK), GFP_KERNEL);
    if (vol_down_work)
    {
        INIT_WORK((struct work_struct *)vol_down_work, wq_handler);
        vol_down_work->type = BTN_VOL_DOWN + vol_down_pressed;
        queue_work(wqueue, (struct work_struct *)vol_down_work);
    }
    return IRQ_HANDLED;
}

/* wq_handler - Handle the work in the workqueue. */
static void wq_handler(struct work_struct *work)
{
    /* Print interrupt count. */
    printk(KERN_INFO "[INTR %u] ", ++intr_cnt);
    /* Print which button was pressed. */
    switch (((BTN_WORK *)work)->type)
    {
    case BTN_HOME:
        printk(KERN_INFO "HOME pressed\n");
        break;
    case BTN_BACK:
        printk(KERN_INFO "BACK pressed\n");
        break;
    case BTN_VOL_UP:
        printk(KERN_INFO "VOL+ pressed\n");
        break;
    case BTN_VOL_DOWN + 0:
        printk(KERN_INFO "VOL- released\n");
        break;
    case BTN_VOL_DOWN + 1:
        printk(KERN_INFO "VOL- pressed\n");
        break;
    default:
        break;
    }
    /* Free allocated memory for work. */
    kfree((BTN_WORK *)work);
}

/* 4. functions for module init/exit -
 * These functions are executed when this module(stopwatch device driver) is installed/removed.
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
    sema_init(&STOPWATCH_QUIT, 0);
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
    del_timer_sync(&(stopwatch.timer));
    /* Unregister device driver module. */
    unregister_chrdev(MAJOR_NUM, DEV_NAME);
}

module_init(stopwatch_init);
module_exit(stopwatch_exit);

/* Module license and author */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Junyeong JANG");
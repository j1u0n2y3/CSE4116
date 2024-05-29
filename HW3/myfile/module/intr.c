#include "core.h"

static struct timer_list stop_timer;

#define BTN_NUM 4
#define BTN_HOME 0
#define BTN_BACK 1
#define BTN_VOL_UP 2
#define BTN_VOL_DOWN 3

irqreturn_t btn_home_handler(int, void *);
irqreturn_t btn_back_handler(int, void *);
irqreturn_t btn_vol_up_handler(int, void *);
irqreturn_t btn_vol_down_handler(int, void *);
void wq_handler(struct work_struct *);

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

static struct workqueue_struct *wqueue = NULL;
typedef struct _BTN_WORK
{
    struct work_struct work;
    int type;
} BTN_WORK;

void intr_init()
{
    int i;
    for (i = 0; i < BTN_NUM; i++)
    {
        gpio_direction_input(btn_gpio[i]);
        request_irq(gpio_to_irq(btn_gpio[i]), btn_handler[i], btn_flag[i], btn_name[i], 0);
    }
    wqueue = create_workqueue("STOPWATCH_WQ");
    init_timer(&stop_timer);
}

void intr_free()
{
    int i;
    for (i = 0; i < BTN_NUM; i++)
        free_irq(gpio_to_irq(btn_gpio[i]), NULL);
    flush_workqueue(wqueue);
    destroy_workqueue(wqueue);
    del_timer_sync(&stop_timer);
}

void stop_timer_handler(unsigned long timeout)
{
    del_timer(&stopwatch);
    stopwatch.elapsed = TIME_LIMIT;
    stopwatch_display();
    up(&STOPWATCH_QUIT);
    return;
}

void stop_timer_add()
{
    stop_timer.expires = get_jiffies_64() + (HZ * 3);
    stop_timer.data = NULL;
    stop_timer.function = stop_timer_handler;
    add_timer(&stop_timer);
}

irqreturn_t btn_home_handler(int irq, void *data)
{
    /* TOP HALF */
    stopwatch.reset = 0;
    /* BOTTOM HALF */
    BTN_WORK *home_work = (BTN_WORK *)kmalloc(sizeof(BTN_WORK), GFP_KERNEL);
    if (home_work)
    {
        INIT_WORK((struct work_struct *)home_work, wq_handler);
        home_work->type = BTN_HOME;
        queue_work(wqueue, (struct work_struct *)home_work);
    }
    return IRQ_HANDLED;
}

irqreturn_t btn_back_handler(int irq, void *data)
{
    /* TOP HALF */
    if (!stopwatch.reset)
        stopwatch.paused = 1 - stopwatch.paused;
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

irqreturn_t btn_vol_up_handler(int irq, void *data)
{
    /* TOP HALF */
    stopwatch.reset = 1;
    stopwatch.paused = 0;
    stopwatch.elapsed = 0;
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

static int vol_down_pressed = 0;
irqreturn_t btn_vol_down_handler(int irq, void *data)
{
    /* TOP HALF */
    vol_down_pressed = 1 - vol_down_pressed;
    if (vol_down_pressed)
        stopwatch.stopped = stopwatch.elapsed;
    // stopwatch.stopped = (stopwatch.stopped != STOP_NOT_PRESSED) ? stopwatch.stopped : stopwatch.elapsed;
    else
        stopwatch.stopped = STOP_NOT_PRESSED;
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

static unsigned int intr_cnt = 0;
void wq_handler(struct work_struct *work)
{
    printk(KERN_INFO "[INTR %u] ", ++intr_cnt);
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
    /*printk(KERN_INFO "*** elapsed : %d / paused : %d\n*** reset : %d / stopped : %d\n",
           stopwatch.elapsed, stopwatch.paused, stopwatch.reset, stopwatch.stopped);*/
    // kfree((void *)work);
    kfree((BTN_WORK *)work);
}
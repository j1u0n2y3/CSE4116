#ifndef _MODULE_H_
#define _MODULE_H_

/* TIMER DEVICE DEFINITIONS */
#define MAJOR_NUM 242
#define DEV_NAME "stopwatch"
#define DEV_FILE_LOC "/dev/stopwatch"

/* TIMER STRUCTURE */
typedef struct _STOPWATCH
{
    struct timer_list timer;
    int elapsed;
    int reset, paused;
} STOPWATCH;

/* STOPWATCH TIME LIMIT */
#define TIME_LIMIT (100 * 60 * 10)

#endif /* _MODULE_H_ */
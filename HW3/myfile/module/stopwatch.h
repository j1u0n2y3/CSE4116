#ifndef _STOPWATCH_H_
#define _STOPWATCH_H_

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

#endif /* _STOPWATCH_H_ */
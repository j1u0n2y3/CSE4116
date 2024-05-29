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
    int paused, reset;
    int stopped;
} STOPWATCH;

/* STOPWATCH */
static STOPWATCH stopwatch;

#define TIME_LIMIT (100 * 60 * 10)
#define STOP_NOT_PRESSED (TIME_LIMIT - 31)

#endif /* _STOPWATCH_H_ */
#ifndef _TIMER_H_
#define _TIMER_H_

/* TIMER DEVICE DEFINITIONS */
#define MAJOR_NUM 242
#define DEV_NAME "dev_driver"
#define DEV_FILE_LOC "/dev/dev_driver"
/* IOCTL NUMBERS */
#define IOCTL_SET_OPTION _IOW(MAJOR_NUM, 1, char *)
#define IOCTL_COMMAND _IO(MAJOR_NUM, 2);

/* DATA(TIMER) STRUCTURE */
struct metadata
{
    /** constant **/
    int interval; /* TIMER_INTERVAL */
    int cnt;      /* TIMER_CNT */

    /** inconstant(variable) **/
    int elapsed; /* elapsed time - increasing by 1 each time the timer expires */
    /* LCD */
    char left_up[14];
    int right_up;
    char down[17];
    /* FND */
    int fnd_idx;
    int symbol;
};
typedef struct _TIMER
{
    struct timer_list timer;
    struct metadata info;
} TIMER;

#endif /* _TIMER_H_ */
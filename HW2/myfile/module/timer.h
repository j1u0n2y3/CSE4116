#ifndef _TIMER_H_
#define _TIMER_H_

#define MAJOR_NUM 242
#define DEV_NAME "dev_driver"
#define DEV_FILE_LOC "/dev/dev_driver"

#define IOCTL_SET_OPTION _IOW(MAJOR_NUM, 1, char *)
#define IOCTL_COMMAND _IO(MAJOR_NUM, 2);

#define NAME "JJY"
#define STUDENT_ID "20211584"

struct metadata
{
    int interval;
    int cnt;
    int elapsed;
};

typedef struct
{
    struct timer_list timer;
    struct metadata info;
} TIMER;

#endif /* _TIMER_H_ */
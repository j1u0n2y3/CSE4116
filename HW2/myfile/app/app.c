/*
 * app.c -
 * This program receives the options to drive the timer device from user
 * and passes it to the timer device driver via ioctl.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
// #include <ctype.h>
// #include <sys/stat.h>
// #include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>

#define MAJOR_NUM 242
#define DEV_FILE_LOC "/dev/dev_driver"

#define IOCTL_SET_OPTION _IOW(MAJOR_NUM, 1, char *)
#define IOCTL_COMMAND _IO(MAJOR_NUM, 2);

int main(int argc, char **argv)
{
    /* ARGUMENT COUNT VALIDATION ROUTINE */
    if (argc != 4)
    {
        printf("usage: ./app TIMER_INTERVAL[1-100] TIMER_CNT[1-200] TIMER_INIT[0001-8000]\n");
        return -1;
    }

    /* ARGUMENT VARIABLES VALIDATION ROUTINE */
    int i;
    int interval = atoi(argv[1]),
        cnt = atoi(argv[2]),
        init = atoi(argv[3]);
    if (interval < 1 || interval > 100)
    {
        printf("ERROR(app.c) : TIMER_INTERVAL[1-100] out of range\n");
        return -1;
    }
    if (cnt < 1 || cnt > 200)
    {
        printf("ERROR(app.c) : TIMER_CNT[1-200] out of range\n");
        return -1;
    }
    char nonzero = 0;
    for (i = 0; i < 4; i++)
    {
        if (argv[3][i] == '\0')
        {
            printf("ERROR(app.c) : TIMER_INIT[0001-8000] must be four characters\n");
            return -1;
        }
        if (argv[3][i] < '0' || argv[3][i] > '8')
        {
            printf("ERROR(app.c) : TIMER_INIT[0001-8000] out of range\n");
            return -1;
        }
        if (argv[3][i] > '0')
        {
            if (nonzero)
            {
                printf("ERROR(app.c) : TIMER_INIT[0001-8000] multiple non-zero characters\n");
                return -1;
            }
            nonzero++;
        }
    }

    /* TIMER DEVICE ROUTINE */
    /* Open timer device file. */
    int timer_fd = open(DEV_FILE_LOC, O_WRONLY);
    if (timer_fd == -1)
    {
        printf("ERROR(app.c) : timer device file open failed\n");
        return -1;
    }
    /* Set ioctl parameter and send it to the timer device. */
    char option[11];
    snprintf(option, 11, "%03d%03d%04d", interval, cnt, init);
    ioctl(timer_fd, IOCTL_SET_OPTION, option);
    /* Start timer when the RESET signal comes in. */
    while (read(timer_fd, NULL, 1))
        /* In device driver, .read fop returns true when the RESET signal comes in. */
        usleep(200000);
    ioctl(timer_fd, IOCTL_COMMAND);

    /* EXIT ROUTINE */
    close(timer_fd);
    return 0;
}
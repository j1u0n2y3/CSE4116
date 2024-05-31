/*
 * app.c -
 * This program controls the stopwatch device
 * through file operations performed on the device file.
 *
 * Author : 20211584 Junyeong JANG
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define DEV_FILE_LOC "/dev/stopwatch"

int main()
{
    /* Open the stopwatch device file. */
    int stopwatch_fd = open(DEV_FILE_LOC, O_RDWR);
    if (stopwatch_fd == -1)
    {
        printf("ERROR(app.c) : stopwatch device file open failed\n");
        return -1;
    }
    /* Initialize the stopwatch. */
    read(stopwatch_fd, NULL, 0);
    /* Start the stopwatch. */
    write(stopwatch_fd, NULL, 0);
    /* Close the stopwatch device file. */
    close(stopwatch_fd);
    return 0;
}

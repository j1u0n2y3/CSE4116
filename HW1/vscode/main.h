#ifndef _MAIN_H_
#define _MAIN_H_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <termios.h>

#define INPUT_KEY (key_t)0xAA
#define OUTPUT_KEY (key_t)0xBB
#define MERGE_KEY (key_t)0xCC

/* key-value structure */
struct table_elem
{
    int order;
    int key;
    char val[6];
};

/* IPC */
struct input_msg // msgq
{
    long mtype;
    bool _BACK_;
    unsigned char switch_input[9];
    int readkey_input;
    bool reset_input;
};
#define INPUT_MSG_SIZE sizeof(struct input_msg) - sizeof(long)

struct output_msg // msgq
{
    long mtype;
    bool _BACK_;
};
#define OUTPUT_MSG_SIZE sizeof(struct output_msg) - sizeof(long)

struct merge_msg // shm
{
    bool _BACK_;
    // struct table_elem mem_table[3];
};
#define MERGE_MSG_SIZE sizeof(struct merge_msg)

/* input : readkey nums */
#define READKEY_BACK 158
#define READKEY_VOL_UP 115
#define READKEY_VOL_DOWN 114
#define READKEY_RELEASED 0
#define READKEY_PRESSED 1

void ipc_init();
void ipc_ctl();

#endif /* _MAIN_H_ */
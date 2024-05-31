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
#include <limits.h>

#define INPUT_KEY (key_t)0x1A1A
#define OUTPUT_KEY (key_t)0x2B2B
#define MERGE_KEY (key_t)0x3C3C
#define LED_KEY (key_t)0x4D4D

/* main : modes */
enum mode
{
    PUT_INIT = 0,
    PUT_KEY,
    PUT_VAL,
    PUT_REQ,
    GET_INIT,
    GET_KEY,
    GET_REQ,
    MERGE_INIT,
    MERGE_REQ
};

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
    bool _RESET_;

    enum mode cur_mode;
    int fnd;
    char lcd2[16];
    bool motor;
};
#define OUTPUT_MSG_SIZE sizeof(struct output_msg) - sizeof(long)

struct merge_msg // shm
{
    bool _BACK_;
    bool _CALL_;
    bool merge_end;
};
#define MERGE_MSG_SIZE sizeof(struct merge_msg)

struct led_msg // msgq
{
    long mtype;
    bool _BACK_;

    enum mode cur_mode;
};
#define LED_MSG_SIZE sizeof(struct led_msg) - sizeof(long)

/* input : readkey nums */
#define READKEY_BACK 158
#define READKEY_VOL_UP 115
#define READKEY_VOL_DOWN 114
#define READKEY_RELEASED 0
#define READKEY_PRESSED 1

/* output : blanks */
#define LED_BLANK (unsigned char)0x00
// unsigned char LCD_BLANK[16] = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };
#define MOTOR_OFF (unsigned char)0
#define MOTOR_ON (unsigned char)1

/* main : st files */
#define STORAGE_DIR "storage_files"

void ipc_init();
void ipc_ctl();

#endif /* _MAIN_H_ */
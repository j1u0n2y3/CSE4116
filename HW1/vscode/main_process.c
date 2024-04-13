#include "main_process.h"

void main_process()
{
    printf("BEGIN : main process\n");

    /* STORAGE FILE DIRECTORY */
    struct stat st = {0};
    if (stat(STORAGE_DIR, &st) == -1)
    {
        if (mkdir(STORAGE_DIR, 0755) == -1)
        {
            perror("ERROR(main_process.c) : mkdir failed.\n");
            _exit(-1);
        }
    }

    int input_q = msgget(INPUT_KEY, 0666 | IPC_CREAT);
    int output_q = msgget(OUTPUT_KEY, 0666 | IPC_CREAT);
    /**/ int merge_q = shmget(MERGE_KEY, MERGE_MSG_SIZE, 0666 | IPC_CREAT);
    if (input_q == -1 || output_q == -1 || merge_q == -1)
    {
        perror("ERROR(main_process.c) : msgget/shmget failed.\n");
        _exit(-1);
    }
    struct input_msg *input;
    /**/ struct merge_msg *merge = shmat(merge_q, NULL, 0);
    /**/ merge->_BACK_ = false;

    enum mode cur_mode = PUT_INIT;
    char key_buf[500], val_buf[500];

    struct table_elem mem_table[3];
    int put_order = 1;
    int mem_table_cnt = 0;
    while (1)
    {
        struct output_msg *output;
        output->mtype = 1;
        output->_BACK_ = false;
        output->_RESET_ = false;
        if (msgrcv(input_q, input, INPUT_MSG_SIZE, 1, 0) == -1)
        {
            perror("ERROR(main_process.c) : msgrcv failed.\n");
            _exit(-1);
        }

        /* DEBUG ROUTINE */
        printf("--------------------------------------------------------------\n");
        printf("* current mode : %d *\n", cur_mode);
        printf("*mtype : %d\n*_BACK_ : %d\n*readkey : %d\n*reset : %d\n",
               input->mtype, input->_BACK_, input->readkey_input, input->reset_input);
        printf("*switch(num) : ");
        for (int i = 0; i < 9; i++)
            printf("[%d]%d ", i + 1, input->switch_input[i]);
        printf("\n");
        printf("--------------------------------------------------------------\n");

        /* URGENT REQUESTS */
        if (input->_BACK_)
        {
            output->_BACK_ = true;
            if (msgsnd(output_q, output, OUTPUT_MSG_SIZE, 0) == -1)
            {
                perror("ERROR(main_process.c) : msgsnd failed\n");
                _exit(-1);
            }
            merge->_BACK_ = true;
            // usleep(1000);
            break;
        }
        if (input->readkey_input == READKEY_VOL_UP)
        {
            mode_up(&cur_mode);
            output->_RESET_ = true;
            if (msgsnd(output_q, output, OUTPUT_MSG_SIZE, 0) == -1)
            {
                perror("ERROR(main_process.c) : msgsnd failed\n");
                _exit(-1);
            }
            continue;
        }
        if (input->readkey_input == READKEY_VOL_DOWN)
        {
            mode_down(&cur_mode);
            output->_RESET_ = true;
            if (msgsnd(output_q, output, OUTPUT_MSG_SIZE, 0) == -1)
            {
                perror("ERROR(main_process.c) : msgsnd failed\n");
                _exit(-1);
            }
            continue;
        }

        switch (cur_mode)
        {
        case PUT_INIT:

            break;
        case PUT_KEY:

            break;
        case PUT_VAL:

            break;
        case PUT_REQ:

            break;
        case GET_INIT:

            break;
        case GET_KEY:

            break;
        case GET_REQ:

            break;
        case MERGE_INIT:

            break;
        case MERGE_REQ:

            break;
        default:
            break;
        }
    }

    if (shmdt(merge) < 0)
    {
        perror("ERROR(main_process.c) : shmdt failed.\n");
        _exit(-1);
    }

    printf("END : main process\n");
}

void mode_up(enum mode *_cur)
{
    enum mode cur = *_cur;
    if (cur == PUT_INIT || cur == PUT_KEY || cur == PUT_VAL || cur == PUT_REQ)
        cur = GET_INIT;
    else if (cur == GET_INIT || cur == GET_KEY || cur == GET_REQ)
        cur = MERGE_INIT;
    else
        cur = PUT_INIT;
    *_cur = cur;
}

void mode_down(enum mode *_cur)
{
    enum mode cur = *_cur;
    if (cur == PUT_INIT || cur == PUT_KEY || cur == PUT_VAL || cur == PUT_REQ)
        cur = MERGE_INIT;
    else if (cur == GET_INIT || cur == GET_KEY || cur == GET_REQ)
        cur = PUT_INIT;
    else
        cur = GET_INIT;
    *_cur = cur;
}
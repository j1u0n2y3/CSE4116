#include "main_process.h"

void main_process()
{
    printf("BEGIN : main process\n");

    int input_q = msgget(INPUT_KEY, 0666 | IPC_CREAT);
    int output_q = msgget(OUTPUT_KEY, 0666 | IPC_CREAT);
    if (input_q == -1 || output_q == -1)
    {
        perror("ERROR(main_process.c) : msgget failed.\n");
        _exit(-1);
    }

    int mode = PUT_MODE;

    struct input_msg *input;
    struct output_msg *output;
    struct merge_msg *merge;

    struct table_elem mem_table[3];
    int table_order = 1;
    int mem_table_cnt = 0;

    while (1)
    {
        if (msgrcv(input_q, input, INPUT_MSG_SIZE, 1, 0) == -1)
        {
            perror("ERROR(main_process.c) : msgrcv failed.\n");
            _exit(-1);
        }

        /* DEBUG ROUTINE */
        printf("--------------------------------------------------------------\n");
        printf("* current mode : %d *\n", mode);
        printf("*mtype : %d\n*_BACK_ : %d\n*readkey : %d\n*reset : %d\n",
               input->mtype, input->_BACK_, input->readkey_input, input->reset_input);
        printf("*switch(num) : ");
        for (int i = 0; i < 9; i++)
            printf("[%d]%d ", i + 1, input->switch_input[i]);
        printf("\n");
        printf("--------------------------------------------------------------\n");

        if (input->_BACK_)
        {
            output->_BACK_ = true;
            merge->_BACK_ = true;
            // break;
        }
        if (input->readkey_input == READKEY_VOL_UP)
            MODE_UP(mode);
        if (input->readkey_input == READKEY_VOL_DOWN)
            MODE_DOWN(mode);

        switch (mode)
        {
        case PUT_MODE:

            break;
        case GET_MODE:

            break;
        case MERGE_MODE:

            break;
        default:
            break;
        }
    }

    printf("END : main process\n");
}
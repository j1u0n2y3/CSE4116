#include "output_process.h"

void output_process()
{
    printf("BEGIN : output process\n");

    int output_q = msgget(OUTPUT_KEY, 0666 | IPC_CREAT);
    if (output_q == -1)
    {
        perror("ERROR(output_process.c) : msgget failed.\n");
        _exit(-1);
    }

    struct output_msg *output;
    while (1)
    {
        if (msgrcv(output_q, (void *)output, OUTPUT_MSG_SIZE, 1, 0))
        {
            perror("ERROR(output_process.c) : msgrcv failed.\n");
            _exit(-1);
        }
        if (output->_BACK_)
            break;
    }

    output_reset();

    printf("END : output process\n");
}

void output_reset()
{
}
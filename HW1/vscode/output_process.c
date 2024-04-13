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
    enum mode prev_mode = MERGE_REQ;
    while (1)
    {
        if (msgrcv(output_q, (void *)output, OUTPUT_MSG_SIZE, 1, 0))
        {
            perror("ERROR(output_process.c) : msgrcv failed.\n");
            _exit(-1);
        }
        if (output->_BACK_)
            break;
        if (output->_RESET_)
        {
            output_reset();
            continue;
        }

        if (prev_mode != output->cur_mode)
        {
            switch (output->cur_mode)
            {
            case PUT_INIT:
            {
                led_mm(0x01);
                break;
            }
            case PUT_KEY:
            {

                break;
            }
            case PUT_VAL:
            {

                break;
            }
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

        prev_mode = output->cur_mode;
    }

    output_reset();

    printf("END : output process\n");
}

void output_reset()
{
    fnd_dd(0000);
    led_mm(LED_BLANK);
    lcd_dd(" ", " ");
    motor_dd(MOTOR_OFF);
    // usleep(1000);
}
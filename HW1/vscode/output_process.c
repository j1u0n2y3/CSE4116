#include "output_process.h"

void output_process()
{
    printf("BEGIN : output process\n");

    int led_q = msgget(LED_KEY, 0666 | IPC_CREAT);
    int led_pid = fork();
    if (led_pid == 0) // led process
    {
        printf("BEGIN : led process\n");

        struct led_msg *led;
        led->cur_mode = MERGE_INIT;
        while (1)
        {
            msgrcv(led_q, led, LED_MSG_SIZE, 1, IPC_NOWAIT);
            if (led->_BACK_)
                break;

            switch (led->cur_mode)
            {
            case PUT_INIT:
            {
                lcd_dd("PUT MODE", NULL);
                led_mm(0x01);
                break;
            }
            case PUT_KEY:
            {
                lcd_dd("PUT MODE", NULL);
                led_mm(0x04);
                usleep(1000000);
                led_mm(0x08);
                usleep(1000000);
                break;
            }
            case PUT_VAL:
            {
                lcd_dd("PUT MODE", NULL);
                led_mm(0x40);
                usleep(1000000);
                led_mm(0x80);
                usleep(1000000);
                break;
            }
            case PUT_REQ:
            {
                lcd_dd("PUT MODE", NULL);
                led_mm(0xFF);
                usleep(1000000);
                led_mm(0x01); /**/
                break;
            }
            case GET_INIT:
            {
                lcd_dd("GET MODE", NULL);
                led_mm(0x10);
                break;
            }
            case GET_KEY:
            {
                lcd_dd("GET MODE", NULL);
                led_mm(0x04);
                usleep(1000000);
                led_mm(0x08);
                usleep(1000000);
                break;
            }
            case GET_REQ:
            {
                lcd_dd("GET MODE", NULL);
                led_mm(0xFF);
                usleep(1000000);
                led_mm(0x10); /**/
                break;
            }
            case MERGE_INIT:
            {
                lcd_dd("MERGE MODE", NULL);
                led_mm(0x00);
                break;
            }
            case MERGE_REQ:
            {
                lcd_dd("MERGE MODE", NULL);
                led_mm(0x00);
                break;
            }
            default:
                break;
            }
        }

        printf("END : led process\n");
        _exit(0);
    }
    //
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
        if (msgrcv(output_q, (void *)output, OUTPUT_MSG_SIZE, 1, 0) == -1)
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
            struct led_msg *led;
            led->mtype = 1;
            led->cur_mode = output->cur_mode;
            led->_BACK_ = false;
            if (msgsnd(led_q, led, LED_MSG_SIZE, 0) == -1)
            {
                perror("ERROR(output_process.c) : msgsnd failed\n");
                _exit(-1);
            }
        }

        prev_mode = output->cur_mode;
    }

    struct led_msg *led;
    led->_BACK_ = true;
    if (msgsnd(led_q, led, LED_MSG_SIZE, 0) == -1)
    {
        perror("ERROR(output_process.c) : msgsnd failed\n");
        _exit(-1);
    }
    waitpid(led_pid, NULL, 0);

    output_reset();
    if (msgctl(msgget(LED_KEY, 0666 | IPC_CREAT), IPC_RMID, NULL) == -1)
    {
        perror("ERROR(output_process.c) : msgctl failed\n");
        _exit(-1);
    }

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
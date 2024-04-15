#include "output_process.h"

void output_process()
{
    printf("BEGIN : output process\n");
    output_reset();
    led_mm(0xFF);

    int led_q = msgget(LED_KEY, 0666 | IPC_CREAT);
    int led_pid = fork();
    if (led_pid == 0) // led process
    {
        printf("BEGIN : led process\n");

        struct led_msg *led = (struct led_msg *)malloc(sizeof(struct led_msg));
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
                led_mm(0x80);
                break;
            }
            case PUT_KEY:
            {
                led_mm(0x20);
                usleep(1000000);
                led_mm(0x10);
                usleep(1000000);
                break;
            }
            case PUT_VAL:
            {
                led_mm(0x02);
                usleep(1000000);
                led_mm(0x01);
                usleep(1000000);
                break;
            }
            case PUT_REQ:
            {
                led_mm(0xFF);
                usleep(2500000);
                led->cur_mode = PUT_INIT;
                break;
            }
            case GET_INIT:
            {
                led_mm(0x08);
                break;
            }
            case GET_KEY:
            {
                led_mm(0x20);
                usleep(1000000);
                led_mm(0x10);
                usleep(1000000);
                break;
            }
            case GET_REQ:
            {
                led_mm(0xFF);
                usleep(2500000);
                led->cur_mode=GET_INIT;
                break;
            }
            case MERGE_INIT:
            {
                led_mm(0x00);
                break;
            }
            case MERGE_REQ:
            {
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

    struct output_msg *output = (struct output_msg *)malloc(sizeof(struct output_msg));
    output->_BACK_=false;
    output->_RESET_=true;
    output->mtype=1;


    struct led_msg *led = (struct led_msg *)malloc(sizeof(struct led_msg));
    led->mtype = 1;
    led->_BACK_ = false;

    enum mode prev_mode = MERGE_REQ;
    while (1)
    {
        if (msgrcv(output_q, output, OUTPUT_MSG_SIZE, 1, 0) == -1)
        {
            perror("ERROR(output_process.c) : msgrcv failed.\n");
            _exit(-1);
        }

        printf("**********************************************\n");
        printf("* cur mode : %d\n", output->cur_mode);
        printf("* fnd : %d\n", output->fnd);
        printf("* lcd2 : %s\n", output->lcd2);

        if (output->_BACK_){
            usleep(5000000);
            break;

        }
        if (output->_RESET_)
        {
            output_reset();
            continue;
        }

        if (prev_mode != output->cur_mode)
        {
            led->cur_mode = output->cur_mode;
            if (msgsnd(led_q, led, LED_MSG_SIZE, 0) == -1)
            {
                perror("ERROR(output_process.c) : msgsnd failed\n");
                _exit(-1);
            }
        }

        fnd_dd(output->fnd);
        switch (output->cur_mode)
            {
            case PUT_INIT:
            {
                lcd_dd("PUT MODE        ", output->lcd2);
                break;
            }
            case PUT_KEY:
            {
                lcd_dd("PUT MODE        ", output->lcd2);
                break;
            }
            case PUT_VAL:
            {
                lcd_dd("PUT MODE        ", output->lcd2);
                break;
            }
            case PUT_REQ:
            {
                lcd_dd("PUT MODE        ", output->lcd2);
                break;
            }
            case GET_INIT:
            {
                lcd_dd("GET MODE        ", output->lcd2);
                break;
            }
            case GET_KEY:
            {
                lcd_dd("GET MODE        ", output->lcd2);
                break;
            }
            case GET_REQ:
            {
                lcd_dd("GET MODE        ", output->lcd2);
                break;
            }
            case MERGE_INIT:
            {
                lcd_dd("MERGE MODE      ", output->lcd2);
                break;
            }
            case MERGE_REQ:
            {
                lcd_dd("MERGE MODE      ", output->lcd2);
                break;
            }
            default:
                break;
            }

        prev_mode = output->cur_mode;
    }

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

    printf("IM HERE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11\n");
    printf("END : output process\n");
}

void output_reset()
{
    fnd_dd(0000);
    led_mm(LED_BLANK);
    lcd_dd("                ", "                ");
    motor_dd(MOTOR_OFF);
    // usleep(1000);
}

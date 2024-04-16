#include "input_process.h"

void input_process()
{
    printf("BEGIN : input process\n");
    int input_q = msgget(INPUT_KEY, 0666 | IPC_CREAT);
    if (input_q == -1)
    {
        perror("ERROR(input_process.c) : msgget failed.\n");
        _exit(-1);
    }

    int readkey_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK),
        switch_fd = open("/dev/fpga_push_switch", O_RDWR),
        reset_fd = open("/dev/fpga_dip_switch", O_RDWR);
    if (readkey_fd == -1 || switch_fd == -1 || reset_fd == -1)
    {
        perror("ERROR(input_process.c) : open failed.\n");
        _exit(-1);
    }

    struct input_msg *input = (struct input_msg *)malloc(sizeof(struct input_msg));
    input->mtype = 1;
    input->_BACK_ = false;
    while (!input->_BACK_)
    {
        input_readkey(readkey_fd, input);
        input_reset(reset_fd, input);
        input_switch(switch_fd, input);
        if (msgsnd(input_q, (void *)input, INPUT_MSG_SIZE, 0) == -1)
        {
            perror("ERROR(input_process.c) : msgsnd failed.\n");
            _exit(-1);
        }
        usleep(50000);
    }

    close(readkey_fd);
    close(switch_fd);
    close(reset_fd);
    
    free(input);

    printf("END : input process\n");
}

void input_readkey(int fd, struct input_msg *msg)
{
    struct input_event ev;
    if (read(fd, (void *)&ev, sizeof(ev)) < sizeof(ev))
    {
        perror("ERROR(input_process.c) : readkey read failed.\n");
        _exit(-1);
    }

    if (ev.value == READKEY_PRESSED)
    {
        switch (ev.code)
        {
        case READKEY_BACK:
            msg->_BACK_ = true;
            msg->readkey_input = READKEY_BACK;
            break;
        case READKEY_VOL_UP:
            msg->readkey_input = READKEY_VOL_UP;
            break;
        case READKEY_VOL_DOWN:
            msg->readkey_input = READKEY_VOL_DOWN;
            break;
        default:
            break;
        }
    }
    else
        msg->readkey_input = -1;
}

void input_switch(int fd, struct input_msg *msg)
{
    unsigned char cur_buf[9], prev_buf[9], switch_val[9];
    memset(cur_buf, 0, sizeof(cur_buf));
    memset(switch_val, 0, sizeof(switch_val));

	int term, i;
    for (term = 5000; term >= 0; term--)
    {
        memcpy(prev_buf, cur_buf, sizeof(cur_buf));
        if (read(fd, (void *)cur_buf, sizeof(cur_buf)) < sizeof(cur_buf))
        {
            perror("ERROR(input_process.c) : switch read failed.\n");
            _exit(-1);
        }

        for (i = 0; i < 9; i++)
        {
            if (cur_buf[i] - prev_buf[i] == 1)
                switch_val[i] = 1;
        }
        // usleep(10);
    }
    memcpy(msg->switch_input, switch_val, sizeof(switch_val));
}

void input_reset(int fd, struct input_msg *msg)
{
    unsigned char buf = 0;
    if (read(fd, (void *)&buf, 1) < 1)
    {
        perror("ERROR(input_process.c) : reset read failed.\n");
        _exit(-1);
    }
    if (buf == 0 /* find and fill!! */)
        msg->reset_input = true;
    else
        msg->reset_input = false;
}

#include "main.h"

int main()
{
    ipc_init();

    int input_pid, output_pid, merge_pid, main_pid;
    /* INPUT PROCESS */
    input_pid = fork();
    if (input_pid == 0)
    {
        input_process();
        _exit(0);
    }
    /* OUTPUT PROCESS */
    output_pid = fork();
    if (output_pid == 0)
    {
        output_process();
        _exit(0);
    }
    /* MERGE PROCESS */
    merge_pid = fork();
    if (merge_pid == 0)
    {
        merge_process();
        _exit(0);
    }
    /* MAIN PROCESS */
    main_pid = fork();
    if (main_pid == 0)
    {
        main_process();
        _exit(0);
    }
    if (input_pid == -1 || output_pid == -1 || merge_pid == -1 || main_pid == -1)
    {
        perror("ERROR(main.c) : fork failed.\n");
        _exit(-1);
    }

    waitpid(input_pid, NULL, 0);
    waitpid(output_pid, NULL, 0);
    waitpid(merge_pid, NULL, 0);
    waitpid(main_pid, NULL, 0);
    ipc_ctl();

    printf("Thank you!\n");

    return 0;
}

void ipc_init()
{
    int in, out, merge;
    in = msgget(INPUT_KEY, 0666 | IPC_CREAT);
    out = msgget(OUTPUT_KEY, 0666 | IPC_CREAT);
    merge = shmget(MERGE_KEY, MERGE_MSG_SIZE, 0666 | IPC_CREAT);
    if (in == -1 || out == -1 || merge == -1)
    {
        perror("ERROR(main.c) : ipc_init failed.\n");
        _exit(-1);
    }
    return;
}

void ipc_ctl()
{
    int in, out, merge;
    in = msgctl(msgget(INPUT_KEY, 0666 | IPC_CREAT), IPC_RMID, NULL);
    out = msgctl(msgget(OUTPUT_KEY, 0666 | IPC_CREAT), IPC_RMID, NULL);
    merge = shmctl(shmget(MERGE_KEY, MERGE_MSG_SIZE, 0666 | IPC_CREAT), IPC_RMID, NULL);
    if (in == -1 || out == -1 || merge == -1)
    {
        perror("ERROR(main.c) : ipc_ctl failed.");
        _exit(-1);
    }
    return;
}
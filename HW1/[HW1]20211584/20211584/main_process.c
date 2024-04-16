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
    if (input_q == -1 || output_q == -1)
    {
        perror("ERROR(main_process.c) : msgget/shmget failed.\n");
        _exit(-1);
    }
    struct input_msg *input = (struct input_msg *)malloc(sizeof(struct input_msg));
    struct output_msg *output = (struct output_msg *)malloc(sizeof(struct output_msg));
    output->mtype = 1;
    output->_BACK_ = false;
    output->_RESET_ = false;

    enum mode cur_mode = PUT_INIT;
    char key_buf[4], val_buf[16], val_input_buf[1000], merge_lcd[17];
    int val_input_buf_top = 0;
    int prev_switch_input = 255, prev_reset_input = 0, term_counter = 0;
    init_buf(key_buf, val_buf, val_input_buf, &val_input_buf_top);

    struct table_elem mem_table[3];
    int put_order = 1;
    int mem_table_cnt = 0;
    while (1)
    {
        struct output_msg *output = (struct output_msg *)malloc(sizeof(struct output_msg));
        output->mtype = 1;
        output->_BACK_ = false;
        output->_RESET_ = false;
        if (msgrcv(input_q, input, INPUT_MSG_SIZE, 1, 0) == -1)
        {
            perror("ERROR(main_process.c) : msgrcv failed.\n");
            _exit(-1);
        }

        /* DEBUG ROUTINE *//*
        printf("--------------------------------------------------------------\n");
        printf("* current mode : %d *\n", cur_mode);
        printf("*mtype : %d\n*_BACK_ : %d\n*readkey : %d\n*reset : %d\n",
               input->mtype, input->_BACK_, input->readkey_input, input->reset_input);
        printf("*switch(num) : ");
        int i;
        for (i = 0; i < 9; i++)
            printf("[%d]%d ", i + 1, input->switch_input[i]);
        printf("\n");
        printf("*val_input_buf : %s\n", val_input_buf);
        printf("*prev_switch_input : %d\n", prev_switch_input);
        printf("*term_counter : %d\n", term_counter);
        printf("*mem table : \n");
        for (i = 0; i < 3; i++)
        {
            printf("[%d] %d / %d / %s\n", i, mem_table[i].order, mem_table[i].key, mem_table[i].val);
        }
        printf("*mem table cnt : %d\n", mem_table_cnt);
        printf("--------------------------------------------------------------\n");
        printf("@ key_buf : ");
        for (i = 0; i < 4; i++)
            printf("%d ", key_buf[i]);
        printf("\n");
        printf("@@ key : %d\n", my_atoi(key_buf));*/

        /* URGENT REQUESTS */
        if (input->_BACK_)
        {
            output->_BACK_ = true;
            if (msgsnd(output_q, output, OUTPUT_MSG_SIZE, 0) == -1)
            {
                perror("ERROR(main_process.c) : msgsnd failed\n");
                _exit(-1);
            }
            main_flush(mem_table, mem_table_cnt, true, false);
            usleep(5000000);
            break;
        }
        if (input->readkey_input == READKEY_VOL_UP)
        {
            init_buf(key_buf, val_buf, val_input_buf, &val_input_buf_top);
            memset(merge_lcd, ' ', sizeof(merge_lcd));
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
            init_buf(key_buf, val_buf, val_input_buf, &val_input_buf_top);
            memset(merge_lcd, ' ', sizeof(merge_lcd));
            mode_down(&cur_mode);
            output->_RESET_ = true;
            if (msgsnd(output_q, output, OUTPUT_MSG_SIZE, 0) == -1)
            {
                perror("ERROR(main_process.c) : msgsnd failed\n");
                _exit(-1);
            }
            continue;
        }

        /* REQ PROCESS ROUTINE */
        int switch_input = switch_check(input);
        int reset_input = input->reset_input;
        switch (cur_mode)
        {
        case PUT_INIT:
        {
            init_buf(key_buf, val_buf, val_input_buf, &val_input_buf_top);
            output->cur_mode = cur_mode;
            output->fnd = 0;
            memset(output->lcd2, ' ', sizeof(output->lcd2));
            if (1 <= switch_input && switch_input <= 89 && prev_switch_input == 255)
            {
                cur_mode++;
                switch_input = 255;
            }
            break;
        }
        case PUT_KEY:
        {
            if (term_counter > 10) /* 1~ */
                init_buf(key_buf, NULL, NULL, NULL);
            if (switch_input != 255 && prev_switch_input == 255) /* (#)123456789 */
            {
                int i;
                for (i = 0; i < 4 && key_buf[i] != 0; i++)
                    ;
                if (i != 4) /* if not full */
                    key_buf[i] = switch_input;
            }
            if (switch_input == 255 && prev_switch_input != 255)
                term_counter = 0;
            else if (switch_input == 1)
                term_counter++;

            output->cur_mode = cur_mode;
            output->fnd = my_atoi(key_buf);
            memcpy(output->lcd2, val_buf, 16);

            if (reset_input && prev_reset_input == 0) /* reset */
                cur_mode++;

            break;
        }
        case PUT_VAL:
        {
            if (switch_input == 46)
            {
                main_put(key_buf, val_buf, mem_table, &mem_table_cnt, &put_order);
                cur_mode = PUT_REQ;
            }
            if (term_counter > 10) /* 1~ */
                init_buf(NULL, val_buf, val_input_buf, &val_input_buf_top);
            if (switch_input != 255 && prev_switch_input == 255) /* (#)123456789 */
            {
                val_input_buf[val_input_buf_top++] = (char)switch_input;
            }
            if (switch_input == 255 && prev_switch_input != 255)
            {
                term_counter = 0;
                if (val_input_buf_top != 0)
                    val_input_buf[val_input_buf_top++] = '#';
            }
            else if (switch_input == 1)
                term_counter++;
            val_interpret(val_buf, val_input_buf, val_input_buf_top);

            output->cur_mode = cur_mode;
            output->fnd = my_atoi(key_buf);
            memcpy(output->lcd2, val_buf, 16);

            if (reset_input && prev_reset_input == 0) /* reset */
                cur_mode--;
            break;
        }
        case PUT_REQ:
        {
            int i, res_top = 0;
            char res[16];
            res[res_top++] = '(';

            if (mem_table[mem_table_cnt - 1].order >= 10)
            {
                res[res_top++] = mem_table[mem_table_cnt - 1].order / 10 + '0';
                res[res_top++] = mem_table[mem_table_cnt - 1].order % 10 + '0';
                res[res_top++] = ',';
                res[res_top++] = ' ';
            }
            else
            {
                res[res_top++] = mem_table[mem_table_cnt - 1].order + '0';
                res[res_top++] = ',';
                res[res_top++] = ' ';
            }
            res[res_top++] = key_buf[0] + '0';
            res[res_top++] = key_buf[1] + '0';
            res[res_top++] = key_buf[2] + '0';
            res[res_top++] = key_buf[3] + '0';
            res[res_top++] = ',';
            for (i = 0; i < 5; i++)
            {
                if (mem_table[mem_table_cnt - 1].val[i] != ' ')
                {
                    res[res_top++] = mem_table[mem_table_cnt - 1].val[i];
                }
            }
            res[res_top++] = ')';
            while (res_top != 16)
                res[res_top++] = ' ';

            output->cur_mode = cur_mode;
            output->fnd = my_atoi(key_buf);
            memcpy(output->lcd2, res, 16);
            if (1 <= switch_input && switch_input <= 89 && prev_switch_input == 255)
            {
                cur_mode = PUT_INIT;
                switch_input = 255;
            }

            break;
        }
        case GET_INIT:
        {
            init_buf(key_buf, val_buf, val_input_buf, &val_input_buf_top);
            output->cur_mode = cur_mode;
            output->fnd = 0;
            memset(output->lcd2, ' ', sizeof(output->lcd2));
            if (1 <= switch_input && switch_input <= 89 && prev_switch_input == 255)
            {
                cur_mode++;
                switch_input = 255;
            }
            break;
        }
        case GET_KEY:
        {
            if (term_counter > 10) /* 1~ */
                init_buf(key_buf, NULL, NULL, NULL);
            if (switch_input != 255 && prev_switch_input == 255) /* (#)123456789 */
            {
                int i;
                for (i = 0; i < 4 && key_buf[i] != 0; i++)
                    ;
                if (i != 4) /* if not full */
                    key_buf[i] = switch_input;
            }
            if (switch_input == 255 && prev_switch_input != 255)
                term_counter = 0;
            else if (switch_input == 1)
                term_counter++;

            output->cur_mode = cur_mode;
            output->fnd = my_atoi(key_buf);
            memcpy(output->lcd2, val_buf, 16);

            if (reset_input && prev_reset_input == 0) /* reset */
            {
                main_get(key_buf, val_buf, mem_table);
                cur_mode = GET_REQ;
            }

            break;
        }
        case GET_REQ:
        {
            output->cur_mode = cur_mode;
            output->fnd = my_atoi(key_buf);
            memset(output->lcd2, ' ', 16);
            if(output->fnd == 0)
                strncpy(val_buf, "ERROR ", 5);
            strncpy(output->lcd2, val_buf, 5);
            if (1 <= switch_input && switch_input <= 89 && prev_switch_input == 255)
            {
                cur_mode = GET_INIT;
                switch_input = 255;
            }
            break;
        }
        case MERGE_INIT:
            init_buf(key_buf, val_buf, val_input_buf, &val_input_buf_top);
            output->cur_mode = cur_mode;
            output->fnd = 0;
            memcpy(output->lcd2, merge_lcd, 16);
            if (reset_input && !prev_reset_input)
            {
                cur_mode++;
                reset_input = 0;
            }
            break;
        case MERGE_REQ:
        {
            output->cur_mode = cur_mode;
            output->fnd = 0;

            DIR *dir = opendir("storage_files");
            struct dirent *entry;

            if (dir == NULL)
            {
                _exit(-1);
            }

            int fileCount = 0;
            while ((entry = readdir(dir)) != NULL)
            {
                if (entry->d_name[0] != '.')
                {
                    fileCount++;
                }
            }
            closedir(dir);

            if (fileCount == 1)
            {
                memcpy(merge_lcd, "ONLY ONE FILE   ", 16);
            }
            else if (fileCount == 2)
            {
                main_flush(mem_table, mem_table_cnt, false, true);
                int merge_q = shmget(MERGE_KEY, MERGE_MSG_SIZE, 0666 | IPC_CREAT);
                struct merge_msg *merge = (struct merge_msg *)shmat(merge_q, NULL, 0);
                while (!merge->merge_end)
                    usleep(250000);
                merge->merge_end = false;
                shmdt(merge);
                char str[25];
                memset(str, ' ', 25);
                sprintf(str, "1 st %d                ", countLinesInFile("storage_files/1.st"));   
                memcpy(merge_lcd, str, 16);
            }

            cur_mode = MERGE_INIT;
            
            break;
        }
        default:
            break;
        }
        if (msgsnd(output_q, output, OUTPUT_MSG_SIZE, 0) == -1)
        {
            perror("ERROR(main_process) : msgsnd failed.\n");
            _exit(-1);
        }

        prev_switch_input = switch_input;
        prev_reset_input = reset_input;
        free(output);
    }

    free(input);
    free(output);

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

void init_buf(char *key_buf, char *val_buf, char *val_input_buf, int *val_input_buf_top)
{
    if (key_buf != NULL)
        memset(key_buf, 0, 4);
    if (val_buf != NULL)
        memset(val_buf, ' ', 16);
    if (val_input_buf != NULL)
    {
        memset(val_input_buf, 0, 1000);
        *val_input_buf_top = 0;
    }
}

char switch_check(struct input_msg *input)
{
    int ret = 0;
    bool flag = false;
    int i;
    for (i = 0; i < 9; i++)
    {
        if (input->switch_input[i])
        {
            flag = true;
            ret *= 10;
            ret += (i + 1);
        }
    }
    return (flag ? ret : -1);
}

void val_interpret(char *val_buf, char *val_input_buf, int val_input_buf_top)
{
    int i;
    if (val_input_buf_top == 0)
        return;
    if (val_input_buf[val_input_buf_top - 1] != '#')
        return;

    for (i = 0; i < val_input_buf_top && val_input_buf[i] != '#' && val_input_buf[i] != 1; i++)
        ;
    char prev = val_input_buf[i - 1];
    int prev_cnt = 0;
    int num_flag = 0;
    char val[500];
    int val_top = 0;
    memset(val, ' ', 500);
    for (i = 0; i < val_input_buf_top; i += 2)
    {
        if (val_input_buf[i] == 1)
        {
            num_flag = (num_flag + 1) % 2;
            continue;
        }

        if (num_flag)
        {
            val[val_top++] = (val_input_buf[i] + '0');
        }
        else
        {
            if (eng_to_num(val[val_top - 1]) == val_input_buf[i])
                val[val_top - 1] = eng_plus(val[val_top - 1]);
            else
                val[val_top++] = num_to_eng(val_input_buf[i]);
        }
    }

    for (i = 0; i < 5; i++)
    {
        val_buf[i] = val[i];
    }
}

char eng_to_num(char eng)
{
    if (eng == 'A' || eng == 'B' || eng == 'C')
        return 2;
    else if (eng == 'D' || eng == 'E' || eng == 'F')
        return 3;
    else if (eng == 'G' || eng == 'H' || eng == 'I')
        return 4;
    else if (eng == 'J' || eng == 'K' || eng == 'L')
        return 5;
    else if (eng == 'M' || eng == 'N' || eng == 'O')
        return 6;
    else if (eng == 'P' || eng == 'Q' || eng == 'R' || eng == 'S')
        return 7;
    else if (eng == 'T' || eng == 'U' || eng == 'V')
        return 8;
    else if (eng == 'W' || eng == 'X' || eng == 'Y' || eng == 'Z')
        return 9;
}

char num_to_eng(char num)
{
    if (num == 2)
        return 'A';
    else if (num == 3)
        return 'D';
    else if (num == 4)
        return 'G';
    else if (num == 5)
        return 'J';
    else if (num == 6)
        return 'M';
    else if (num == 7)
        return 'P';
    else if (num == 8)
        return 'T';
    else if (num == 9)
        return 'W';
}

char eng_plus(char eng)
{
    eng++;
    if (eng == 'D')
        eng = 'A';
    else if (eng == 'G')
        eng = 'D';
    else if (eng == 'J')
        eng = 'G';
    else if (eng == 'M')
        eng = 'J';
    else if (eng == 'P')
        eng = 'M';
    else if (eng == 'T')
        eng = 'P';
    else if (eng == 'W')
        eng = 'T';
    else if (eng == 'Z' + 1)
        eng = 'W';
    return eng;
}

void main_put(char *key_buf, char *val_buf, struct table_elem *mem_table, int *mem_table_cnt, int *put_order)
{
    if (*mem_table_cnt == 3)
    {
        main_flush(mem_table, *mem_table_cnt, false, false);
        *mem_table_cnt = 0;
    }

    struct table_elem tmp;
    tmp.order = (*put_order)++;
    tmp.key = my_atoi(key_buf);
    int i;
    for (i = 0; i < 5; i++)
    {
        tmp.val[i] = val_buf[i];
    }
    tmp.val[5] = '\0';

    mem_table[*mem_table_cnt] = tmp;

    *mem_table_cnt = *mem_table_cnt + 1;
}

void main_flush(struct table_elem *mem_table, int mem_table_cnt, bool _BACK_, bool _CALL_)
{

    int i;

    if (!_CALL_ && mem_table_cnt != 0)
    {
        const char *dirPath = "storage_files"; 
        char filePath[256];          
        FILE *file;
        DIR *dir;
        struct dirent *entry;
        int file1Exists = 0, file2Exists = 0, file3Exists = 0;

        dir = opendir(dirPath);
        if (dir == NULL)
        {
            perror("opendir");
            return;
        }

        while ((entry = readdir(dir)) != NULL)
        {
            if (!strcmp(entry->d_name, "1.st"))
            {
                file1Exists = 1;
            }
            else if (!strcmp(entry->d_name, "2.st"))
            {
                file2Exists = 1;
            }
            else if (!strcmp(entry->d_name, "3.st"))
            {
                file3Exists = 1;
            }
        }

        closedir(dir);

        if (!file1Exists)
        {
            snprintf(filePath, sizeof(filePath), "%s/1.st", dirPath);
        }
        else if (!file2Exists)
        {
            snprintf(filePath, sizeof(filePath), "%s/2.st", dirPath);
        }
        else if (!file3Exists)
        {
            snprintf(filePath, sizeof(filePath), "%s/3.st", dirPath);
        }
        else
        {
            return;
        }

        file = fopen(filePath, "w");
        if (file == NULL)
        {
            perror("fopen");
            return;
        }
        for (i = 0; i < mem_table_cnt; i++)
            fprintf(file, "%d %d %s\n", mem_table[i].order, mem_table[i].key, mem_table[i].val);
        fclose(file);
    }
    int merge_q = shmget(MERGE_KEY, MERGE_MSG_SIZE, 0666 | IPC_CREAT);
    struct merge_msg *merge = (struct merge_msg *)shmat(merge_q, NULL, 0);
    merge->_BACK_ = _BACK_;
    merge->_CALL_ = _CALL_;
    /*shmsnd*/
    usleep(100000);
    shmdt(merge);
}

void main_get(char *key_buf, char *val_buf, struct table_elem *mem_table)
{
    int key = 1000 * key_buf[0] + 100 * key_buf[1] + 10 * key_buf[2] + key_buf[3];
    int i;
    for (i = 2; i >= 0; i--)
    {
        if (mem_table[i].key == key)
        {
            memcpy(val_buf, mem_table[i].val, 16);
            return;
        }
    }

    if (findValueByKey("storage_files/2.st", key, val_buf))
        return;
    findValueByKey("storage_files/1.st", key, val_buf);
}

bool findValueByKey(const char *filename, int searchKey, char *result)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        strcpy(result, ""); 
        return false;
    }

    int maxOrder = -1;       
    char tempResult[5] = ""; 
    bool found = false;

    while (!feof(file))
    {
        int order, key;
        char value[5];

        if (fscanf(file, "%d %d %s", &order, &key, value) == 3)
        {
            if (key == searchKey && order > maxOrder)
            {
                strcpy(tempResult, value); 
                maxOrder = order;         
                found = true;
            }
        }
    }

    if (found)
    {
        strcpy(result, tempResult); 
    }
    else
    {
        strcpy(result, "ERROR"); 
    }

    int i;
    for (i = 0; result[i] != '\0'; i++)
        ;
    for (; i < 16; i++)
        result[i] = ' ';

    fclose(file);
    return found;
}

int countLinesInFile(const char *fileName)
{
    FILE *file = fopen(fileName, "r"); 
    if (file == NULL) {
        return -1; 
    }

    int lines = 0;
    char ch;

    while ((ch = fgetc(file)) != 255) {
        if (ch == '\n') {
            lines++;
        }
    }

    fclose(file);

    return lines;
}
#ifndef _MAIN_P_H_
#define _MAIN_P_H_

#include "main.h"

void main_process();
void mode_up(enum mode *);
void mode_down(enum mode *);
void init_buf(char *, char *, char *, int *);
char switch_check(struct input_msg *input);
void val_interpret(char *val_buf, char *val_input_buf, int val_input_buf_top);
char eng_to_num(char eng);
char num_to_eng(char num);
char eng_plus(char eng);
void main_put(char *key_buf, char *val_buf, struct table_elem *mem_table, int *mem_table_cnt, int *put_order);
void main_flush(struct table_elem *mem_table, int mem_table_cnt, bool _BACK_, bool _CALL_);
void main_get(char *key_buf, char *val_buf, struct table_elem *mem_table);
bool findValueByKey(const char *filename, int searchKey, char *result);
int countLinesInFile(const char *fileName);

inline int my_atoi(char *key_buf)
{
    int result = 0;
    int i;
    for (i = 0; i < 4; ++i)
        result = result * 10 + key_buf[i];
    return result;
}

#endif /* _MAIN_P_H_ */
#ifndef _INPUT_P_H_
#define _INPUT_P_H_

#include "main.h"

void input_process();
void input_readkey(int, struct input_msg *);
void input_switch(int, struct input_msg *);
void input_reset(int, struct input_msg *);

#endif /* _INPUT_P_H_ */
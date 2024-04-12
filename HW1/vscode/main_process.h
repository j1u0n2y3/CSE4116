#ifndef _MAIN_P_H_
#define _MAIN_P_H_

#include "main.h"

#define MODE_NUM 3
#define PUT_MODE 0
#define GET_MODE 1
#define MERGE_MODE 2
#define MODE_UP(mod) mod = (mod + 1) % MODE_NUM
#define MODE_DOWN(mod) mod = (mod + MODE_NUM - 1) % MODE_NUM

void main_process();

#endif /* _MAIN_P_H_ */
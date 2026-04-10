#ifndef __PIR_H
#define __PIR_H

#include "stm32f10x.h"

void PIR_Init(void);
uint8_t PIR_GetState(void);

#endif

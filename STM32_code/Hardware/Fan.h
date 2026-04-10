#ifndef __FAN_H
#define __FAN_H

#include "stm32f10x.h"

void Fan_Init(void);
void Fan_On(void);
void Fan_Off(void);
uint8_t Fan_GetState(void);

#endif
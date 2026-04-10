#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

void LED_Init(void);
void LED_Green_On(void);
void LED_Green_Off(void);
void LED_Red_On(void);
void LED_Red_Off(void);
void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);
uint8_t LED_GetState(void);

#endif

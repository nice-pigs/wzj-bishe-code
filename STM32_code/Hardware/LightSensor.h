#ifndef __LIGHTSENSOR_H
#define __LIGHTSENSOR_H

#include "stm32f10x.h"

void LightSensor_Init(void);
uint16_t LightSensor_ReadADC(void);
uint8_t LightSensor_GetPercent(void);
uint8_t LightSensor_GetDigital(void);

#endif
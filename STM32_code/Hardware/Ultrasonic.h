#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include "stm32f10x.h"

// 超声波测距模块 HC-SR04
// Trig: PB0 (触发引脚)
// Echo: PB3 (回响引脚)

void Ultrasonic_Init(void);
uint16_t Ultrasonic_GetDistance(void);  // 返回距离，单位：cm
uint8_t Ultrasonic_IsOccupied(void);    // 返回1=有人，0=无人

#endif


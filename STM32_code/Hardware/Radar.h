#ifndef __RADAR_H
#define __RADAR_H

#include "stm32f10x.h"

// 雷达传感器模块
// OUT: PA7 (检测输出引脚)

void Radar_Init(void);
uint8_t Radar_GetState(void);  // 返回1=检测到人体，0=未检测到

#endif


#ifndef __PRESSURE_H
#define __PRESSURE_H

#include "stm32f10x.h"

// 二路薄膜压力传感器模块
// 引脚：VCC, GND, OUT2, GND, OUT1, GND
// OUT1: PB4 (压力通道1) - 完全空闲
// OUT2: PB5 (压力通道2) - 完全空闲
// 有压力：高电平
// 无压力：低电平

void Pressure_Init(void);
uint8_t Pressure_GetState1(void);     // 返回通道1状态：1=有压力，0=无压力
uint8_t Pressure_GetState2(void);     // 返回通道2状态：1=有压力，0=无压力
uint8_t Pressure_IsOccupied(void);    // 综合判断：任一通道有压力即判定为有人
uint8_t Pressure_GetRawGPIO1(void);   // 调试用：获取通道1原始GPIO电平（1=高，0=低）
uint8_t Pressure_GetRawGPIO2(void);   // 调试用：获取通道2原始GPIO电平（1=高，0=低）

#endif


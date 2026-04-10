#ifndef __CURTAIN_H
#define __CURTAIN_H

#include "stm32f10x.h"

// 窗帘状态枚举
typedef enum
{
    CURTAIN_CLOSED = 0,     // 窗帘关闭
    CURTAIN_OPEN = 1,       // 窗帘打开
    CURTAIN_OPENING = 2,    // 窗帘正在打开
    CURTAIN_CLOSING = 3,    // 窗帘正在关闭
    CURTAIN_STOPPED = 4     // 窗帘停止（中间位置）
} CurtainState_t;

void Curtain_Init(void);
void Curtain_Open(void);
void Curtain_Close(void);
void Curtain_Stop(void);
CurtainState_t Curtain_GetState(void);
void Curtain_SetSpeed(uint16_t speed_us);  // 设置速度（微秒）
void Curtain_ManualTest(uint16_t steps, uint8_t direction);  // 手动测试

#endif
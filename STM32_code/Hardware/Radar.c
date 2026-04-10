#include "Radar.h"

/*=============================================================================
 * 函数：Radar_Init
 * 功能：初始化雷达传感器模块
 *============================================================================*/
void Radar_Init(void)
{
    // 使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // PA7配置为上拉输入（雷达输出引脚）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*=============================================================================
 * 函数：Radar_GetState
 * 功能：获取雷达传感器状态
 * 返回：1=检测到人体，0=未检测到
 *============================================================================*/
uint8_t Radar_GetState(void)
{
    // 读取PA7引脚状态
    // 高电平=检测到人体，低电平=未检测到
    if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == Bit_SET)
    {
        return 1;  // 检测到人体
    }
    else
    {
        return 0;  // 未检测到
    }
}


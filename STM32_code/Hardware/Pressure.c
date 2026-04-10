#include "Pressure.h"

/*=============================================================================
 * 函数：Pressure_Init
 * 功能：初始化二路压力传感器模块
 * 说明：OUT1接PB4，OUT2接PB5（完全空闲的引脚）
 *============================================================================*/
void Pressure_Init(void)
{
    // 使能GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // PB4配置为上拉输入（压力传感器通道1）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // PB5配置为上拉输入（压力传感器通道2）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/*=============================================================================
 * 函数：Pressure_GetState1
 * 功能：获取压力传感器通道1状态
 * 返回：1=有压力（有人坐），0=无压力（无人）
 *============================================================================*/
uint8_t Pressure_GetState1(void)
{
    // 读取PB4引脚状态（通道1）
    // 先按照原始逻辑：高电平=有压力，低电平=无压力
    // 通过调试输出来确定实际逻辑
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == Bit_SET)
    {
        return 1;  // 有压力（高电平）
    }
    else
    {
        return 0;  // 无压力（低电平）
    }
}

/*=============================================================================
 * 函数：Pressure_GetState2
 * 功能：获取压力传感器通道2状态
 * 返回：1=有压力（有人坐），0=无压力（无人）
 *============================================================================*/
uint8_t Pressure_GetState2(void)
{
    // 读取PB5引脚状态（通道2）
    // 先按照原始逻辑：高电平=有压力，低电平=无压力
    // 通过调试输出来确定实际逻辑
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5) == Bit_SET)
    {
        return 1;  // 有压力（高电平）
    }
    else
    {
        return 0;  // 无压力（低电平）
    }
}

/*=============================================================================
 * 函数：Pressure_GetRawGPIO1
 * 功能：获取压力传感器通道1的原始GPIO电平（调试用）
 * 返回：1=高电平，0=低电平
 *============================================================================*/
uint8_t Pressure_GetRawGPIO1(void)
{
    return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4);
}

/*=============================================================================
 * 函数：Pressure_GetRawGPIO2
 * 功能：获取压力传感器通道2的原始GPIO电平（调试用）
 * 返回：1=高电平，0=低电平
 *============================================================================*/
uint8_t Pressure_GetRawGPIO2(void)
{
    return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5);
}

/*=============================================================================
 * 函数：Pressure_IsOccupied
 * 功能：综合判断座位是否有人
 * 返回：1=有人（任一通道有压力），0=无人（两个通道都无压力）
 * 说明：只要有一个压力传感器检测到压力，就判定为有人
 *============================================================================*/
uint8_t Pressure_IsOccupied(void)
{
    uint8_t state1 = Pressure_GetState1();
    uint8_t state2 = Pressure_GetState2();
    
    // 任一通道有压力，即判定为有人
    if(state1 == 1 || state2 == 1)
    {
        return 1;  // 有人
    }
    else
    {
        return 0;  // 无人
    }
}


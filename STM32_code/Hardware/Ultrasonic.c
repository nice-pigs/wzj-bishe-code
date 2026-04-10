#include "Ultrasonic.h"
#include "Delay.h"

// 超声波测距阈值（单位：cm）
#define OCCUPIED_THRESHOLD 50  // 小于50cm判定为有人

/*=============================================================================
 * 函数：Ultrasonic_Init
 * 功能：初始化超声波测距模块
 *============================================================================*/
void Ultrasonic_Init(void)
{
    // 使能GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // PB0配置为推挽输出（Trig触发引脚）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // PB3配置为浮空输入（Echo回响引脚）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // Trig初始化为低电平
    GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
}

/*=============================================================================
 * 函数：Ultrasonic_GetDistance
 * 功能：获取超声波测距值
 * 返回：距离（单位：cm），0表示测量失败
 *============================================================================*/
uint16_t Ultrasonic_GetDistance(void)
{
    uint32_t timeout = 0;
    uint32_t high_time = 0;
    uint16_t distance = 0;
    
    // 1. 发送10us高电平触发信号
    GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_SET);
    Delay_us(10);
    GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
    
    // 2. 等待Echo变为高电平（超时时间：10ms）
    timeout = 10000;  // 10ms超时
    while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3) == Bit_RESET)
    {
        Delay_us(1);
        timeout--;
        if(timeout == 0)
        {
            return 0;  // 超时，返回0
        }
    }
    
    // 3. 测量Echo高电平持续时间（最大38ms，对应距离约6.5米）
    high_time = 0;
    timeout = 38000;  // 38ms超时
    while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3) == Bit_SET)
    {
        Delay_us(1);
        high_time++;
        timeout--;
        if(timeout == 0)
        {
            return 0;  // 超时，返回0
        }
    }
    
    // 4. 计算距离
    // 声速：340m/s = 0.034cm/us
    // 距离 = (高电平时间 * 0.034) / 2
    // 简化：距离 = 高电平时间 / 58
    distance = high_time / 58;
    
    // 5. 有效范围检查（2cm - 400cm）
    if(distance < 2 || distance > 400)
    {
        return 0;  // 超出有效范围
    }
    
    return distance;
}

/*=============================================================================
 * 函数：Ultrasonic_IsOccupied
 * 功能：判断座位是否有人
 * 返回：1=有人，0=无人
 *============================================================================*/
uint8_t Ultrasonic_IsOccupied(void)
{
    uint16_t distance = Ultrasonic_GetDistance();
    
    // 测量失败，返回0（无人）
    if(distance == 0)
    {
        return 0;
    }
    
    // 距离小于阈值，判定为有人
    if(distance < OCCUPIED_THRESHOLD)
    {
        return 1;
    }
    
    return 0;
}


#include "stm32f10x.h"
#include "LED.h"

/**
  * 函    数：LED初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：直接GPIO驱动，不使用继电器（继电器需要12V供电）
  */
void LED_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA5引脚初始化为推挽输出
	
	/*设置GPIO默认电平*/
	GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);				//设置PA5引脚为低电平，LED默认关闭（直接GPIO驱动）
}

/**
  * 函    数：LED开启
  * 参    数：无
  * 返 回 值：无
  */
void LED_On(void)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET);					//设置PA5引脚为高电平，LED开启（直接GPIO驱动，高电平=3.3V）
}

/**
  * 函    数：LED关闭
  * 参    数：无
  * 返 回 值：无
  */
void LED_Off(void)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);				//设置PA5引脚为低电平，LED关闭（直接GPIO驱动，低电平=0V）
}

/**
  * 函    数：LED状态翻转
  * 参    数：无
  * 返 回 值：无
  */
void LED_Toggle(void)
{
	// 直接GPIO驱动：高电平=开启，低电平=关闭
	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_5) == Bit_SET)	//如果当前为高电平（LED开启）
	{
		GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);			//则设置PA5引脚为低电平（LED关闭）
	}
	else														//否则，即当前为低电平（LED关闭）
	{
		GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET);				//则设置PA5引脚为高电平（LED开启）
	}
}

/**
  * 函    数：获取LED状态
  * 参    数：无
  * 返 回 值：LED状态，1表示开启，0表示关闭
  */
uint8_t LED_GetState(void)
{
	// 直接GPIO驱动：高电平=开启(1)，低电平=关闭(0)
	return (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_5) == Bit_SET) ? 1 : 0;
}
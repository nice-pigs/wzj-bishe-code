#include "stm32f10x.h"
#include "Fan.h"

/**
  * 函    数：风扇初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：使用5V继电器控制，高电平触发
  */
void Fan_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;					//使用PA6控制风扇继电器
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA6引脚初始化为推挽输出
	
	/*设置GPIO默认电平*/
	GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_RESET);				//设置PA6引脚为低电平，继电器断开，风扇默认关闭（高电平触发）
}

/**
  * 函    数：风扇开启
  * 参    数：无
  * 返 回 值：无
  */
void Fan_On(void)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_SET);					//设置PA6引脚为高电平，继电器闭合，风扇开启（高电平触发）
}

/**
  * 函    数：风扇关闭
  * 参    数：无
  * 返 回 值：无
  */
void Fan_Off(void)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_RESET);				//设置PA6引脚为低电平，继电器断开，风扇关闭（高电平触发）
}

/**
  * 函    数：获取风扇状态
  * 参    数：无
  * 返 回 值：风扇状态，1表示开启，0表示关闭
  */
uint8_t Fan_GetState(void)
{
	// 5V继电器高电平触发：高电平=开启(1)，低电平=关闭(0)
	return (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_6) == Bit_SET) ? 1 : 0;
}
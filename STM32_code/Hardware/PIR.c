#include "stm32f10x.h"
#include "PIR.h"

/**
  * 函    数：PIR人体红外传感器初始化
  * 参    数：无
  * 返 回 值：无
  * 注意事项：PIR传感器输出高电平表示检测到人体
  */
void PIR_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;				//下拉输入模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;					//使用PA3作为PIR传感器输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA3引脚初始化为下拉输入
}

/**
  * 函    数：读取PIR传感器状态
  * 参    数：无
  * 返 回 值：1表示检测到人体，0表示未检测到
  */
uint8_t PIR_GetState(void)
{
	return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3);			//读取PA3引脚电平
}

#include "stm32f10x.h"
#include "Buzzer.h"
#include "Delay.h"

/**
  * 函    数：蜂鸣器初始化
  * 参    数：无
  * 返 回 值：无
  */
void Buzzer_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA4引脚初始化为推挽输出
	
	/*设置GPIO默认电平*/
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_SET);				//设置PA4引脚为高电平，蜂鸣器默认关闭（低电平触发）
}

/**
  * 函    数：蜂鸣器开启
  * 参    数：无
  * 返 回 值：无
  */
void Buzzer_On(void)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_RESET);					//设置PA4引脚为低电平，蜂鸣器开启（低电平触发）
}

/**
  * 函    数：蜂鸣器关闭
  * 参    数：无
  * 返 回 值：无
  */
void Buzzer_Off(void)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_SET);				//设置PA4引脚为高电平，蜂鸣器关闭（低电平触发）
}

/**
  * 函    数：蜂鸣器定时蜂鸣
  * 参    数：duration_ms 蜂鸣持续时间（毫秒）
  * 返 回 值：无
  */
void Buzzer_Beep(uint16_t duration_ms)
{
	Buzzer_On();												//开启蜂鸣器
	Delay_ms(duration_ms);										//延时指定时间
	Buzzer_Off();												//关闭蜂鸣器
}

/**
  * 函    数：蜂鸣器报警模式
  * 参    数：无
  * 返 回 值：无
  * 注意事项：此函数会阻塞执行，产生3次短促蜂鸣
  */
void Buzzer_Alarm(void)
{
	uint8_t i;
	for (i = 0; i < 3; i++)
	{
		Buzzer_On();
		Delay_ms(200);											//蜂鸣200ms
		Buzzer_Off();
		Delay_ms(100);											//间隔100ms
	}
}
#include "stm32f10x.h"
#include "ESP32_Serial.h"
#include <stdio.h>
#include <stdarg.h>

uint8_t ESP32_Serial_RxData;
uint8_t ESP32_Serial_RxFlag;

// 环形缓冲区
#define RX_BUFFER_SIZE 128
static uint8_t rx_ring_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

/**
  * 函    数：ESP32串口初始化
  * 参    数：无
  * 返 回 值：无
  */
void ESP32_Serial_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//开启USART3的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//开启GPIOB的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;				//PB10 - TX
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);					//将PB10引脚初始化为复用推挽输出
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;				//PB11 - RX
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);					//将PB11引脚初始化为上拉输入
	
	/*USART初始化*/
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;  // 修改为115200，与ESP32匹配
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART3, &USART_InitStructure);				//将USART3初始化
	
	/*中断输出配置*/
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);			//开启串口接收数据的中断
	
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);			//配置NVIC为分组2
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	
	/*USART使能*/
	USART_Cmd(USART3, ENABLE);								//使能USART3，串口开始运行
}

/**
  * 函    数：ESP32串口发送一个字节
  * 参    数：Byte 要发送的一个字节
  * 返 回 值：无
  */
void ESP32_Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART3, Byte);
	while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

/**
  * 函    数：ESP32串口发送一个数组
  * 参    数：Array 要发送数组的首地址
  * 参    数：Length 要发送数组的长度
  * 返 回 值：无
  */
void ESP32_Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i ++)
	{
		ESP32_Serial_SendByte(Array[i]);
	}
}

/**
  * 函    数：ESP32串口发送一个字符串
  * 参    数：String 要发送字符串的首地址
  * 返 回 值：无
  */
void ESP32_Serial_SendString(char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i ++)
	{
		ESP32_Serial_SendByte(String[i]);
	}
}

/**
  * 函    数：ESP32串口获取接收标志位
  * 参    数：无
  * 返 回 值：串口接收标志位，范围：0~1，接收到数据后，标志位置1，读取后标志位自动清零
  */
uint8_t ESP32_Serial_GetRxFlag(void)
{
	// 检查环形缓冲区是否有数据
	if (rx_head != rx_tail)
	{
		return 1;
	}
	return 0;
}

/**
  * 函    数：ESP32串口获取接收到的数据
  * 参    数：无
  * 返 回 值：接收到的数据，范围：0~255
  */
uint8_t ESP32_Serial_GetRxData(void)
{
	uint8_t data = 0;
	if (rx_head != rx_tail)
	{
		data = rx_ring_buffer[rx_tail];
		rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
	}
	return data;
}

/**
  * 函    数：USART3中断函数
  * 参    数：无
  * 返 回 值：无
  * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
  *           函数名为预留的指定名称，可以从启动文件复制
  *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
  */
void USART3_IRQHandler(void)
{
	if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
	{
		uint8_t data = USART_ReceiveData(USART3);
		uint16_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
		
		// 检查缓冲区是否已满
		if (next_head != rx_tail)
		{
			rx_ring_buffer[rx_head] = data;
			rx_head = next_head;
		}
		// 缓冲区满则丢弃数据
		
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}
}
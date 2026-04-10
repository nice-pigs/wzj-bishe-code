#include "stm32f10x.h"
#include "Curtain.h"
#include "Delay.h"
#include <stdio.h>

// 窗帘状态变量
static CurtainState_t curtainState = CURTAIN_CLOSED;
static uint8_t currentStep = 0;  // 当前步进位置 (0-3)
static uint16_t stepCount = 0;   // 已执行步数
static uint16_t targetSteps = 0; // 目标步数
static uint8_t motorDirection = 0; // 0=正转(打开), 1=反转(关闭)
static volatile uint8_t isRunning = 0; // 电机运行标志

// 28BYJ-48 步进电机 4拍全步进时序表（商家标准版本）
// 引脚顺序: PB0(IN1), PB1(IN2), PB3(IN3), PB5(IN4)
static const uint8_t stepSequence[4] = {
	0x01,  // IN1 (0001)
	0x02,  // IN2 (0010)
	0x04,  // IN3 (0100)
	0x08   // IN4 (1000)
};

#define STEP_COUNT 4  // 4拍全步进模式

/**
  * 函    数：设置步进电机相位
  * 参    数：step - 步进序号 (0-3)
  * 返 回 值：无
  */
static void Stepper_SetPhase(uint8_t step)
{
	uint8_t phase = stepSequence[step];
	
	GPIO_WriteBit(GPIOB, GPIO_Pin_0, (phase & 0x01) ? Bit_SET : Bit_RESET);  // IN1
	GPIO_WriteBit(GPIOB, GPIO_Pin_1, (phase & 0x02) ? Bit_SET : Bit_RESET);  // IN2
	GPIO_WriteBit(GPIOB, GPIO_Pin_3, (phase & 0x04) ? Bit_SET : Bit_RESET);  // IN3
	GPIO_WriteBit(GPIOB, GPIO_Pin_5, (phase & 0x08) ? Bit_SET : Bit_RESET);  // IN4
	
	// 每 500 步输出一次 GPIO 状态
	static uint16_t debug_count = 0;
	if (++debug_count >= 500)
	{
		debug_count = 0;
		printf("[STEPPER] Step=%d, Phase=0x%02X, PB0=%d, PB1=%d, PB3=%d, PB5=%d\r\n",
		       step, phase,
		       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0),
		       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_1),
		       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_3),
		       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_5));
	}
}

/**
  * 函    数：步进电机停止（所有相位断电）
  * 参    数：无
  * 返 回 值：无
  */
static void Stepper_Stop(void)
{
	GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_3, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_RESET);
	isRunning = 0;
	TIM_Cmd(TIM2, DISABLE);  // 停止定时器
}

/**
  * 函    数：窗帘控制初始化
  * 参    数：无
  * 返 回 值：无
  * 注意事项：使用 28BYJ-48 步进电机 + ULN2003 驱动板，定时器中断驱动
  */
void Curtain_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);		//开启TIM2的时钟
	
	/*GPIO初始化 - 步进电机控制引脚 PB0, PB1, PB3, PB5*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/*设置GPIO默认电平 - 所有相位断电*/
	Stepper_Stop();
	
	/*TIM2定时器初始化 - 用于步进电机驱动（商家标准配置）*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;				// 72MHz/72 = 1MHz (1us分辨率)
	TIM_TimeBaseStructure.TIM_Period = 1000;					// 1000us = 1ms (商家标准速度)
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	
	/*TIM2中断配置*/
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	curtainState = CURTAIN_CLOSED;								//初始状态为关闭
	currentStep = 0;
	stepCount = 0;
}

/**
  * 函    数：TIM2中断服务函数 - 步进电机驱动
  * 参    数：无
  * 返 回 值：无
  */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		
		if (isRunning)
		{
			// 执行单步（8拍模式）
			if (motorDirection == 0)  // 正转
			{
				currentStep = (currentStep + 1) % STEP_COUNT;
			}
			else  // 反转
			{
				currentStep = (currentStep == 0) ? (STEP_COUNT - 1) : (currentStep - 1);
			}
			
			// 更新线圈状态
			Stepper_SetPhase(currentStep);
			
			// 步数控制
			if (targetSteps > 0)
			{
				stepCount++;
				
				// 每100步输出一次进度
				if (stepCount % 100 == 0)
				{
					printf("[CURTAIN] Progress: %d/%d steps\r\n", stepCount, targetSteps);
				}
				
				if (stepCount >= targetSteps)
				{
					printf("[CURTAIN] Target reached! Stopping motor.\r\n");
					Stepper_Stop();
					
					// 更新状态
					if (motorDirection == 0)
					{
						curtainState = CURTAIN_OPEN;
						printf("[CURTAIN] State changed to OPEN\r\n");
					}
					else
					{
						curtainState = CURTAIN_CLOSED;
						printf("[CURTAIN] State changed to CLOSED\r\n");
					}
				}
			}
		}
	}
}

/**
  * 函    数：窗帘打开
  * 参    数：无
  * 返 回 值：无
  * 注意事项：28BYJ-48 步进电机，512步/圈（减速比64:1），完整打开约需2048步
  */
void Curtain_Open(void)
{
	printf("[CURTAIN] Open command received, Current state: %d\r\n", curtainState);
	
	if (curtainState == CURTAIN_CLOSED || curtainState == CURTAIN_STOPPED)
	{
		printf("[CURTAIN] Starting to open... (2048 steps, 4-beat mode)\r\n");
		printf("[CURTAIN] Note: Requires 5V motor or 12V power supply\r\n");
		curtainState = CURTAIN_OPENING;
		motorDirection = 0;  // 正转
		stepCount = 0;
		targetSteps = 2048;  // 4拍模式：2048步 = 完整行程
		isRunning = 1;
		TIM_Cmd(TIM2, ENABLE);  // 启动定时器
		printf("[CURTAIN] Timer started, isRunning=%d\r\n", isRunning);
	}
	else
	{
		printf("[CURTAIN] Cannot open - wrong state (current: %d)\r\n", curtainState);
	}
}

/**
  * 函    数：窗帘关闭
  * 参    数：无
  * 返 回 值：无
  */
void Curtain_Close(void)
{
	printf("[CURTAIN] Close command received, Current state: %d\r\n", curtainState);
	
	if (curtainState == CURTAIN_OPEN || curtainState == CURTAIN_STOPPED)
	{
		printf("[CURTAIN] Starting to close... (2048 steps, 4-beat mode)\r\n");
		printf("[CURTAIN] Note: Requires 5V motor or 12V power supply\r\n");
		curtainState = CURTAIN_CLOSING;
		motorDirection = 1;  // 反转
		stepCount = 0;
		targetSteps = 2048;  // 4拍模式：2048步 = 完整行程
		isRunning = 1;
		TIM_Cmd(TIM2, ENABLE);  // 启动定时器
		printf("[CURTAIN] Timer started, isRunning=%d\r\n", isRunning);
	}
	else
	{
		printf("[CURTAIN] Cannot close - wrong state (current: %d)\r\n", curtainState);
	}
}

/**
  * 函    数：窗帘停止
  * 参    数：无
  * 返 回 值：无
  */
void Curtain_Stop(void)
{
	Stepper_Stop();
	
	if (curtainState == CURTAIN_OPENING || curtainState == CURTAIN_CLOSING)
	{
		curtainState = CURTAIN_STOPPED;
	}
}

/**
  * 函    数：获取窗帘状态
  * 参    数：无
  * 返 回 值：窗帘当前状态
  */
CurtainState_t Curtain_GetState(void)
{
	return curtainState;
}

/**
  * 函    数：设置步进速度
  * 参    数：speed_us - 速度（微秒），范围 500-5000，值越小越快
  * 返 回 值：无
  */
void Curtain_SetSpeed(uint16_t speed_us)
{
	if (speed_us < 500) speed_us = 500;
	if (speed_us > 5000) speed_us = 5000;
	
	TIM2->ARR = speed_us;  // 直接设置周期（参考商家代码，不减1）
	
	// 如果定时器已启用，需要手动触发更新
	if(TIM2->CR1 & TIM_CR1_CEN) {
		TIM2->EGR = TIM_PSCReloadMode_Immediate;
	}
}

/**
  * 函    数：手动测试步进电机（用于调试）
  * 参    数：steps - 步数，direction - 方向（0=正转，1=反转）
  * 返 回 值：无
  */
void Curtain_ManualTest(uint16_t steps, uint8_t direction)
{
	printf("[CURTAIN] Manual test: %d steps, direction=%d\r\n", steps, direction);
	printf("[CURTAIN] Testing GPIO output...\r\n");
	
	// 先测试 GPIO 是否能输出
	printf("[TEST] Setting all pins HIGH...\r\n");
	GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_SET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_3, Bit_SET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_SET);
	printf("[TEST] PB0=%d, PB1=%d, PB3=%d, PB5=%d\r\n",
	       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0),
	       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_1),
	       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_3),
	       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_5));
	
	Delay_ms(1000);
	
	printf("[TEST] Setting all pins LOW...\r\n");
	GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_3, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_RESET);
	printf("[TEST] PB0=%d, PB1=%d, PB3=%d, PB5=%d\r\n",
	       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0),
	       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_1),
	       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_3),
	       GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_5));
	
	Delay_ms(1000);
	
	// 开始步进
	printf("[CURTAIN] Starting stepper motor...\r\n");
	curtainState = CURTAIN_OPENING;
	motorDirection = direction;
	stepCount = 0;
	targetSteps = steps;
	isRunning = 1;
	TIM_Cmd(TIM2, ENABLE);
}
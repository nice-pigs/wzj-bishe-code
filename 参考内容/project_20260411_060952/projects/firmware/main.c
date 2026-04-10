/**
 ******************************************************************************
 * @file    main.c
 * @brief   座位状态监测系统主程序
 * @author  毕业设计项目组
 * @version V1.0
 * @date    2024
 ******************************************************************************
 * @attention
 * 本程序用于STM32F103C8T6单片机，实现座位占用状态检测和上报功能
 ******************************************************************************
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

/* ==================== 宏定义 ==================== */

// 设备配置
#define DEVICE_KEY      "STM32-001"              // 设备唯一密钥
#define DEVICE_NAME     "座位监测设备-001"        // 设备名称
#define SERVER_IP       "192.168.1.100"          // 服务器IP地址
#define SERVER_PORT     "5000"                   // 服务器端口

// 传感器阈值
#define PRESSURE_THRESHOLD  2000                 // 压力传感器阈值（ADC值）
#define OCCUPIED_DELAY      3000                 // 确认占用的延迟时间（ms）
#define IDLE_DELAY          5000                 // 确认空闲的延迟时间（ms）

// 定时器周期
#define SENSOR_READ_INTERVAL  100                // 传感器读取间隔（ms）
#define WIFI_HEARTBEAT_INTERVAL 30000            // Wi-Fi心跳间隔（ms）
#define STATUS_UPLOAD_INTERVAL 10000             // 状态上传间隔（ms）

// 引脚定义
#define LED_RED_PIN        GPIO_PIN_1            // 红色LED引脚
#define LED_GREEN_PIN      GPIO_PIN_2            // 绿色LED引脚
#define SENSOR_PIN         GPIO_PIN_0            // 传感器引脚（ADC）
#define WIFI_RST_PIN       GPIO_PIN_0            // Wi-Fi复位引脚

// 状态定义
#define SEAT_IDLE          0                     // 座位空闲
#define SEAT_OCCUPIED      1                     // 座位占用
#define SEAT_UNKNOWN       2                     // 状态未知

/* ==================== 全局变量 ==================== */

UART_HandleTypeDef huart1;                       // 串口1（用于Wi-Fi通信）
UART_HandleTypeDef huart2;                       // 串口2（用于调试输出）
ADC_HandleTypeDef hadc1;                         // ADC句柄（用于读取传感器）
I2C_HandleTypeDef hi2c1;                         // I2C句柄（用于OLED显示）

uint8_t currentSeatStatus = SEAT_UNKNOWN;        // 当前座位状态
uint8_t lastUploadedStatus = SEAT_UNKNOWN;       // 上次上传的状态
uint32_t statusChangeTime = 0;                   // 状态改变的时间戳
uint8_t statusConfirmed = 0;                     // 状态是否已确认

char wifiBuffer[512];                            // Wi-Fi接收缓冲区
uint16_t wifiBufferIndex = 0;                    // 缓冲区索引

/* ==================== 函数声明 ==================== */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);

void LED_Init(void);
void LED_SetRed(void);
void LED_SetGreen(void);
void LED_SetOff(void);

void OLED_Init(void);
void OLED_DisplayStatus(uint8_t status);
void OLED_DisplayString(uint8_t line, char *str);

uint16_t Sensor_Read(void);
uint8_t Sensor_CheckStatus(void);

void WiFi_Init(void);
void WiFi_Reset(void);
uint8_t WiFi_ConnectAP(char *ssid, char *password);
uint8_t WiFi_SendCommand(char *cmd, char *response, uint32_t timeout);
void WiFi_SendHTTPRequest(char *path, char *method, char *data);
uint8_t WiFi_RegisterDevice(void);
uint8_t WiFi_UploadStatus(uint8_t status);

/* ==================== 主函数 ==================== */

int main(void)
{
    uint32_t lastSensorReadTime = 0;
    uint32_t lastHeartbeatTime = 0;
    uint32_t lastUploadTime = 0;
    
    // HAL库初始化
    HAL_Init();
    
    // 系统时钟配置
    SystemClock_Config();
    
    // 外设初始化
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    
    // LED初始化
    LED_Init();
    
    // OLED初始化
    OLED_Init();
    OLED_DisplayString(0, "座位监测系统");
    OLED_DisplayString(1, "正在初始化...");
    
    // Wi-Fi初始化
    WiFi_Init();
    
    // 连接Wi-Fi热点（需要根据实际情况修改）
    if (WiFi_ConnectAP("YourWiFiSSID", "YourPassword") == 0) {
        OLED_DisplayString(2, "WiFi连接成功");
        LED_SetGreen();
    } else {
        OLED_DisplayString(2, "WiFi连接失败");
        LED_SetRed();
        while (1);  // 连接失败，停止运行
    }
    
    // 设备注册
    HAL_Delay(1000);
    if (WiFi_RegisterDevice() == 0) {
        OLED_DisplayString(3, "设备注册成功");
    } else {
        OLED_DisplayString(3, "设备注册失败");
    }
    
    HAL_Delay(2000);
    OLED_DisplayString(1, "运行中...");
    
    // 主循环
    while (1)
    {
        uint32_t currentTime = HAL_GetTick();
        
        // 定时读取传感器
        if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL)
        {
            lastSensorReadTime = currentTime;
            uint8_t newStatus = Sensor_CheckStatus();
            
            // 状态变化检测
            if (newStatus != currentSeatStatus)
            {
                if (!statusConfirmed)
                {
                    // 首次检测到状态变化
                    statusChangeTime = currentTime;
                    statusConfirmed = 0;
                }
                else
                {
                    // 状态再次变化，重置计时
                    statusChangeTime = currentTime;
                    currentSeatStatus = newStatus;
                    statusConfirmed = 0;
                }
            }
            else if (!statusConfirmed && newStatus == currentSeatStatus)
            {
                // 状态保持，检查确认时间
                uint32_t confirmDelay = (newStatus == SEAT_OCCUPIED) ? OCCUPIED_DELAY : IDLE_DELAY;
                
                if (currentTime - statusChangeTime >= confirmDelay)
                {
                    // 状态确认
                    statusConfirmed = 1;
                    
                    // 更新本地显示
                    if (currentSeatStatus == SEAT_OCCUPIED)
                    {
                        LED_SetRed();
                        OLED_DisplayStatus(SEAT_OCCUPIED);
                    }
                    else
                    {
                        LED_SetGreen();
                        OLED_DisplayStatus(SEAT_IDLE);
                    }
                }
            }
        }
        
        // 定时上传状态（状态变化时立即上传，否则定时上传心跳）
        if (currentTime - lastUploadTime >= STATUS_UPLOAD_INTERVAL)
        {
            lastUploadTime = currentTime;
            
            // 状态变化或定时上传
            if (currentSeatStatus != lastUploadedStatus || statusConfirmed)
            {
                if (WiFi_UploadStatus(currentSeatStatus) == 0)
                {
                    lastUploadedStatus = currentSeatStatus;
                    statusConfirmed = 0;  // 重置确认标志
                }
            }
        }
        
        // 定时发送心跳
        if (currentTime - lastHeartbeatTime >= WIFI_HEARTBEAT_INTERVAL)
        {
            lastHeartbeatTime = currentTime;
            // 可在此处添加心跳包发送逻辑
        }
        
        // 短暂延时，降低CPU占用
        HAL_Delay(10);
    }
}

/* ==================== LED控制函数 ==================== */

/**
 * @brief LED初始化
 */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 使能GPIOB时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // 配置LED引脚为推挽输出
    GPIO_InitStruct.Pin = LED_RED_PIN | LED_GREEN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // 初始状态：LED熄灭
    LED_SetOff();
}

/**
 * @brief 设置红色LED（座位占用）
 */
void LED_SetRed(void)
{
    HAL_GPIO_WritePin(GPIOB, LED_RED_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, LED_GREEN_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 设置绿色LED（座位空闲）
 */
void LED_SetGreen(void)
{
    HAL_GPIO_WritePin(GPIOB, LED_RED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_GREEN_PIN, GPIO_PIN_SET);
}

/**
 * @brief 关闭所有LED
 */
void LED_SetOff(void)
{
    HAL_GPIO_WritePin(GPIOB, LED_RED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LED_GREEN_PIN, GPIO_PIN_RESET);
}

/* ==================== 传感器函数 ==================== */

/**
 * @brief 读取传感器ADC值
 * @return ADC转换结果（0-4095）
 */
uint16_t Sensor_Read(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    
    // 配置ADC通道
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // 启动ADC转换
    HAL_ADC_Start(&hadc1);
    
    // 等待转换完成
    HAL_ADC_PollForConversion(&hadc1, 100);
    
    // 读取转换结果
    uint16_t adcValue = HAL_ADC_GetValue(&hadc1);
    
    // 停止ADC
    HAL_ADC_Stop(&hadc1);
    
    return adcValue;
}

/**
 * @brief 检测座位状态
 * @return 座位状态（SEAT_IDLE/SEAT_OCCUPIED）
 */
uint8_t Sensor_CheckStatus(void)
{
    // 多次采样取平均值，提高稳定性
    uint32_t sum = 0;
    for (int i = 0; i < 10; i++)
    {
        sum += Sensor_Read();
        HAL_Delay(5);
    }
    uint16_t avgValue = sum / 10;
    
    // 判断状态
    if (avgValue >= PRESSURE_THRESHOLD)
    {
        return SEAT_OCCUPIED;
    }
    else
    {
        return SEAT_IDLE;
    }
}

/* ==================== Wi-Fi通信函数 ==================== */

/**
 * @brief Wi-Fi模块初始化
 */
void WiFi_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 使能GPIOB时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // 配置Wi-Fi复位引脚
    GPIO_InitStruct.Pin = WIFI_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // 复位Wi-Fi模块
    WiFi_Reset();
}

/**
 * @brief Wi-Fi模块复位
 */
void WiFi_Reset(void)
{
    HAL_GPIO_WritePin(GPIOB, WIFI_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOB, WIFI_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(2000);  // 等待模块启动
}

/**
 * @brief 连接Wi-Fi热点
 * @param ssid WiFi名称
 * @param password WiFi密码
 * @return 0-成功，1-失败
 */
uint8_t WiFi_ConnectAP(char *ssid, char *password)
{
    char cmd[128];
    
    // 设置工作站模式
    if (WiFi_SendCommand("AT+CWMODE=1", "OK", 1000) != 0)
    {
        return 1;
    }
    
    // 连接热点
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
    if (WiFi_SendCommand(cmd, "WIFI GOT IP", 20000) != 0)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief 发送AT指令
 * @param cmd AT指令
 * @param response 期望的响应
 * @param timeout 超时时间（ms）
 * @return 0-成功，1-失败
 */
uint8_t WiFi_SendCommand(char *cmd, char *response, uint32_t timeout)
{
    uint32_t startTime = HAL_GetTick();
    uint16_t len = strlen(cmd);
    
    // 清空接收缓冲区
    memset(wifiBuffer, 0, sizeof(wifiBuffer));
    wifiBufferIndex = 0;
    
    // 发送指令
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, len, 1000);
    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 1000);
    
    // 等待响应
    while (HAL_GetTick() - startTime < timeout)
    {
        if (strstr(wifiBuffer, response) != NULL)
        {
            return 0;  // 成功
        }
        if (strstr(wifiBuffer, "ERROR") != NULL)
        {
            return 1;  // 失败
        }
        HAL_Delay(10);
    }
    
    return 1;  // 超时
}

/**
 * @brief 发送HTTP请求
 * @param path API路径
 * @param method HTTP方法（GET/POST）
 * @param data POST数据（仅POST请求使用）
 */
void WiFi_SendHTTPRequest(char *path, char *method, char *data)
{
    char cmd[256];
    char httpHeader[512];
    uint16_t dataLen = (data != NULL) ? strlen(data) : 0;
    
    // 建立TCP连接
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", SERVER_IP, SERVER_PORT);
    if (WiFi_SendCommand(cmd, "OK", 5000) != 0)
    {
        return;
    }
    
    // 构造HTTP请求头
    if (strcmp(method, "POST") == 0)
    {
        sprintf(httpHeader, 
            "POST %s HTTP/1.1\r\n"
            "Host: %s:%s\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n%s",
            path, SERVER_IP, SERVER_PORT, dataLen, data);
    }
    else
    {
        sprintf(httpHeader, 
            "GET %s HTTP/1.1\r\n"
            "Host: %s:%s\r\n"
            "Connection: close\r\n\r\n",
            path, SERVER_IP, SERVER_PORT);
    }
    
    // 发送数据长度
    sprintf(cmd, "AT+CIPSEND=%d", strlen(httpHeader));
    if (WiFi_SendCommand(cmd, ">", 2000) != 0)
    {
        WiFi_SendCommand("AT+CIPCLOSE", "OK", 1000);
        return;
    }
    
    // 发送HTTP请求
    HAL_UART_Transmit(&huart1, (uint8_t*)httpHeader, strlen(httpHeader), 5000);
    
    // 等待发送完成
    HAL_Delay(1000);
    
    // 关闭连接
    WiFi_SendCommand("AT+CIPCLOSE", "OK", 1000);
}

/**
 * @brief 设备注册
 * @return 0-成功，1-失败
 */
uint8_t WiFi_RegisterDevice(void)
{
    char jsonData[128];
    
    // 构造JSON数据
    sprintf(jsonData, "{\"deviceKey\":\"%s\",\"deviceName\":\"%s\"}", 
            DEVICE_KEY, DEVICE_NAME);
    
    // 发送注册请求
    WiFi_SendHTTPRequest("/api/devices/register", "POST", jsonData);
    
    // 等待响应（简化处理，实际应解析响应）
    HAL_Delay(2000);
    
    return 0;
}

/**
 * @brief 上传座位状态
 * @param status 座位状态（0-空闲，1-占用）
 * @return 0-成功，1-失败
 */
uint8_t WiFi_UploadStatus(uint8_t status)
{
    char jsonData[128];
    
    // 构造JSON数据
    sprintf(jsonData, "{\"deviceKey\":\"%s\",\"isOccupied\":%s}", 
            DEVICE_KEY, status ? "true" : "false");
    
    // 发送状态上报请求
    WiFi_SendHTTPRequest("/api/seats/status", "POST", jsonData);
    
    // 等待响应（简化处理，实际应解析响应）
    HAL_Delay(2000);
    
    return 0;
}

/* ==================== OLED显示函数（简化版） ==================== */

/**
 * @brief OLED初始化
 */
void OLED_Init(void)
{
    // 简化实现，实际需要发送初始化命令
    // 参考SSD1306数据手册
}

/**
 * @brief 显示座位状态
 * @param status 座位状态
 */
void OLED_DisplayStatus(uint8_t status)
{
    if (status == SEAT_OCCUPIED)
    {
        OLED_DisplayString(2, "状态: 占用");
    }
    else
    {
        OLED_DisplayString(2, "状态: 空闲");
    }
}

/**
 * @brief 显示字符串
 * @param line 行号（0-3）
 * @param str 字符串
 */
void OLED_DisplayString(uint8_t line, char *str)
{
    // 简化实现，实际需要通过I2C发送显示数据
    // 参考SSD1306驱动程序
}

/* ==================== 系统配置函数 ==================== */

/**
 * @brief 系统时钟配置
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // 配置主PLL
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    // 配置系统时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/**
 * @brief GPIO初始化
 */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

/**
 * @brief USART1初始化（Wi-Fi通信）
 */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/**
 * @brief USART2初始化（调试输出）
 */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

/**
 * @brief ADC1初始化（传感器读取）
 */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);
    
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/**
 * @brief I2C1初始化（OLED显示）
 */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

/* ==================== 中断回调函数 ==================== */

/**
 * @brief UART接收中断回调函数
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // 处理Wi-Fi接收数据
        // 简化实现，实际需要处理完整的接收逻辑
    }
}

/* ==================== 错误处理函数 ==================== */

/**
 * @brief 错误处理函数
 */
void Error_Handler(void)
{
    while (1)
    {
        // 错误指示：LED闪烁
        LED_SetRed();
        HAL_Delay(200);
        LED_SetOff();
        HAL_Delay(200);
    }
}

/********************** 文件结束 **********************/

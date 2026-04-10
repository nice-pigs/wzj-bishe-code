#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "DHT11.h"
#include "Key.h"
#include "LightSensor.h"
#include "Buzzer.h"
#include "LED.h"
#include "PIR.h"
#include "Ultrasonic.h"
#include "Pressure.h"
#include "ESP32_Comm.h"

/*=============================================================================
 * 座位占用检测系统 - 最终版
 * 功能：多传感器融合检测座位是否有人，并通过WiFi上报状态
 * 
 * 硬件清单：
 * - STM32F103C8T6 单片机
 * - ESP32-S3 WiFi模块
 * - 压力传感器（二路）- 主要检测
 * - HC-SR04 超声波测距 - 辅助检测
 * - PIR人体红外传感器 - 辅助检测
 * - DHT11 温湿度传感器 - 环境监测
 * - 光敏传感器 - 环境监测
 * - OLED显示屏（带4按键）
 * - LED指示灯（绿色+红色）
 * - 蜂鸣器
 * 
 * 传感器权重分配：
 * - 压力传感器（任一通道）：70分（主要依据）
 * - 超声波测距（<50cm）：25分（辅助判断）
 * - PIR红外（检测到运动）：5分（辅助判断）
 *============================================================================*/

// 座位状态定义
typedef enum {
    SEAT_VACANT = 0,    // 空闲
    SEAT_OCCUPIED = 1   // 占用
} SeatState_t;

// 全局变量
SeatState_t current_seat_state = SEAT_VACANT;
uint32_t occupied_duration = 0;
uint32_t vacant_duration = 0;

// 传感器数据
uint8_t temperature = 0;
uint8_t humidity = 0;
uint16_t distance = 0;
uint8_t pir_state = 0;
uint8_t pressure1 = 0;
uint8_t pressure2 = 0;
uint8_t light_percent = 0;

// 功能开关
#define DHT11_ENABLED 1  // 0=禁用, 1=启用

// 函数声明
void System_Init(void);
void Display_Welcome(void);
void Update_Seat_State_Final(void);
void Display_Seat_Status(void);
void Send_Data_To_ESP32(void);
void Handle_Key_Input(void);

int main(void)
{
    /*========== 系统初始化 ==========*/
    System_Init();
    
    /*========== 显示欢迎信息 ==========*/
    Display_Welcome();
    
    /*========== 定时器 ==========*/
    uint32_t sensor_timer = 0;      // 传感器读取（每1秒）
    uint32_t display_timer = 0;     // 显示更新（每1秒）
    uint32_t esp32_timer = 0;       // ESP32数据发送（每5秒）
    uint32_t state_timer = 0;       // 状态持续时间（每秒）
    
    printf("[SYSTEM] Seat detection system started!\r\n");
    printf("[SYSTEM] Sensors: Pressure(2ch) + Ultrasonic + PIR + DHT11 + Light\r\n\r\n");
    Buzzer_Beep(200);  // 启动提示音
    
    /*========== 主循环 ==========*/
    while (1)
    {
        /*---------- 1. 处理ESP32通信 ----------*/
        ESP32_ProcessReceivedData();
        
        /*---------- 2. 处理按键输入 ----------*/
        Handle_Key_Input();
        
        /*---------- 3. 读取传感器数据（每1秒） ----------*/
        if(++sensor_timer >= 100000)
        {
            sensor_timer = 0;
            
            // 读取压力传感器（最重要）
            pressure1 = Pressure_GetState1();
            pressure2 = Pressure_GetState2();
            
            // 读取超声波距离
            distance = Ultrasonic_GetDistance();
            
            // 读取PIR传感器
            pir_state = PIR_GetState();
            
#if DHT11_ENABLED
            // 读取温湿度（每2秒一次）
            static uint32_t dht11_timer = 0;
            if(++dht11_timer >= 2)
            {
                dht11_timer = 0;
                DHT11_Read(&temperature, &humidity);
            }
#endif
            
            // 读取光照强度
            light_percent = LightSensor_GetPercent();
            
            // 获取压力传感器原始GPIO电平（调试用）
            uint8_t p1_gpio = Pressure_GetRawGPIO1();
            uint8_t p2_gpio = Pressure_GetRawGPIO2();
            
            printf("[SENSOR] P1:%d P2:%d Dist:%dcm PIR:%d Light:%d%% T:%dC H:%d%%\r\n", 
                   pressure1, pressure2, distance, pir_state, 
                   light_percent, temperature, humidity);
            printf("[DEBUG] P1_GPIO:%d P2_GPIO:%d (1=高电平/无压力, 0=低电平/有压力)\r\n",
                   p1_gpio, p2_gpio);
        }
        
        /*---------- 4. 更新座位状态（每0.5秒） ----------*/
        static uint32_t state_check_timer = 0;
        if(++state_check_timer >= 50000)
        {
            state_check_timer = 0;
            Update_Seat_State_Final();
        }
        
        /*---------- 5. 更新显示（每1秒） ----------*/
        if(++display_timer >= 100000)
        {
            display_timer = 0;
            Display_Seat_Status();
        }
        
        /*---------- 6. 发送数据到ESP32（每5秒） ----------*/
        if(++esp32_timer >= 500000)
        {
            esp32_timer = 0;
            Send_Data_To_ESP32();
        }
        
        /*---------- 7. 更新状态持续时间（每秒） ----------*/
        if(++state_timer >= 100000)
        {
            state_timer = 0;
            
            if(current_seat_state == SEAT_OCCUPIED)
                occupied_duration++;
            else
                vacant_duration++;
        }
        
        Delay_us(100);
    }
}

/*=============================================================================
 * 函数：System_Init
 * 功能：系统初始化
 *============================================================================*/
void System_Init(void)
{
    Serial_Init();
    printf("\r\n");
    printf("========================================\r\n");
    printf("  Seat Detection System - Final\r\n");
    printf("  Multi-Sensor Fusion\r\n");
    printf("========================================\r\n");
    
    printf("[INIT] Initializing OLED...\r\n");
    OLED_Init();
    
    printf("[INIT] Initializing DHT11...\r\n");
    DHT11_Init();
    
    printf("[INIT] Initializing Pressure sensor (2ch)...\r\n");
    Pressure_Init();
    
    printf("[INIT] Initializing Ultrasonic...\r\n");
    Ultrasonic_Init();
    
    printf("[INIT] Initializing PIR sensor...\r\n");
    PIR_Init();
    
    printf("[INIT] Initializing Light sensor...\r\n");
    LightSensor_Init();
    
    printf("[INIT] Initializing Keys...\r\n");
    Key_Init();
    
    printf("[INIT] Initializing Buzzer...\r\n");
    Buzzer_Init();
    
    printf("[INIT] Initializing LEDs...\r\n");
    LED_Init();
    
    printf("[INIT] Initializing ESP32 communication...\r\n");
    ESP32_Comm_Init();
    
    printf("\r\n[INIT] Hardware Configuration:\r\n");
    printf("  === Primary Sensors ===\r\n");
    printf("  Pressure CH1  : PB4 (OUT1) - Main detection\r\n");
    printf("  Pressure CH2  : PB5 (OUT2) - Main detection\r\n");
    printf("  === Secondary Sensors ===\r\n");
    printf("  Ultrasonic    : PB0(Trig), PB3(Echo)\r\n");
    printf("  PIR           : PA3 (OUT)\r\n");
    printf("  === Environment Sensors ===\r\n");
    printf("  DHT11         : PA0 (Single Wire)\r\n");
    printf("  Light Sensor  : PA1 (ADC), PA2 (Digital)\r\n");
    printf("  === Actuators ===\r\n");
    printf("  Buzzer        : PA4\r\n");
    printf("  Green LED     : PA5 (Vacant)\r\n");
    printf("  Red LED       : PA6 (Occupied)\r\n");
    printf("  OLED Display  : PB6(SCL), PB7(SDA)\r\n");
    printf("  === Communication ===\r\n");
    printf("  ESP32 Comm    : PB10(TX), PB11(RX)\r\n");
    printf("  Keys          : PB12-PB15\r\n");
    printf("========================================\r\n\r\n");
}

/*=============================================================================
 * 函数：Display_Welcome
 * 功能：显示欢迎信息
 *============================================================================*/
void Display_Welcome(void)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "Seat Detection");
    OLED_ShowString(2, 1, "System Final");
    OLED_ShowString(3, 1, "Initializing");
    OLED_ShowString(4, 1, "Please wait..");
    
#if DHT11_ENABLED
    printf("[SYSTEM] Waiting for DHT11 to stabilize...\r\n");
    DHT11_Init();
    Delay_ms(2000);
    
    uint8_t temp = 0, humi = 0;
    if(DHT11_Read(&temp, &humi) == 0) {
        printf("[DHT11] OK - Temp:%dC, Humi:%d%%\r\n", temp, humi);
        temperature = temp;
        humidity = humi;
    } else {
        printf("[DHT11] Read failed, will continue without it\r\n");
    }
#endif
    
    printf("[SYSTEM] Testing pressure sensor...\r\n");
    pressure1 = Pressure_GetState1();
    pressure2 = Pressure_GetState2();
    printf("[PRESSURE] CH1:%d, CH2:%d\r\n", pressure1, pressure2);
}

/*=============================================================================
 * 函数：Update_Seat_State_Final
 * 功能：最终版多传感器融合判断座位状态
 * 算法：
 *   - 压力传感器（任一通道） → 权重70分（主要依据）
 *   - 超声波距离 < 50cm → 权重25分
 *   - PIR检测到运动 → 权重5分
 *   - 总分 >= 70分 → 判定为占用
 *============================================================================*/
void Update_Seat_State_Final(void)
{
    static uint8_t last_state = SEAT_VACANT;
    static uint8_t confirm_count = 0;
    
    uint8_t occupied_score = 0;
    
    // 1. 压力传感器判断（权重70%，最重要）
    if(pressure1 == 1 || pressure2 == 1)
    {
        occupied_score += 70;
        printf("[FUSION] Pressure: OCCUPIED (+70) [CH1:%d CH2:%d]\r\n", pressure1, pressure2);
    }
    
    // 2. 超声波测距判断（权重25%）
    if(distance > 0 && distance < 50)
    {
        occupied_score += 25;
        printf("[FUSION] Ultrasonic: OCCUPIED (+25) [%dcm]\r\n", distance);
    }
    
    // 3. PIR传感器判断（权重5%）
    if(pir_state == 1)
    {
        occupied_score += 5;
        printf("[FUSION] PIR: MOTION (+5)\r\n");
    }
    
    // 4. 综合判断
    printf("[FUSION] Total Score: %d/100 (Threshold: 70)\r\n", occupied_score);
    
    uint8_t new_state = (occupied_score >= 70) ? SEAT_OCCUPIED : SEAT_VACANT;
    
    // 5. 防抖动：连续3次确认才改变状态
    if(new_state == last_state)
    {
        confirm_count++;
        if(confirm_count >= 3 && new_state != current_seat_state)
        {
            // 状态确认改变
            current_seat_state = new_state;
            confirm_count = 0;
            
            if(current_seat_state == SEAT_OCCUPIED)
            {
                occupied_duration = 0;
                printf("\r\n[STATE CHANGE] VACANT -> OCCUPIED\r\n");
                printf("[REASON] Score: %d >= 70\r\n\r\n", occupied_score);
                Buzzer_Beep(200);
                Delay_ms(100);
                Buzzer_Beep(200);
            }
            else
            {
                vacant_duration = 0;
                printf("\r\n[STATE CHANGE] OCCUPIED -> VACANT\r\n");
                printf("[REASON] Score: %d < 70\r\n\r\n", occupied_score);
                Buzzer_Beep(200);
            }
        }
    }
    else
    {
        last_state = new_state;
        confirm_count = 0;
    }
    
    // 6. 更新LED指示灯
    if(current_seat_state == SEAT_OCCUPIED)
    {
        LED_Off();                                      // 关闭绿灯
        GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_SET);     // 打开红灯
    }
    else
    {
        LED_On();                                       // 打开绿灯
        GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_RESET);   // 关闭红灯
    }
}

/*=============================================================================
 * 函数：Display_Seat_Status
 * 功能：在OLED上显示座位状态
 *============================================================================*/
void Display_Seat_Status(void)
{
    OLED_Clear();
    
    // 第1行：座位状态
    if(current_seat_state == SEAT_OCCUPIED)
    {
        OLED_ShowString(1, 1, "Status:OCCUPIED");
    }
    else
    {
        OLED_ShowString(1, 1, "Status:VACANT  ");
    }
    
    // 第2行：压力传感器和距离
    OLED_ShowString(2, 1, "P:");
    OLED_ShowNum(2, 3, pressure1, 1);
    OLED_ShowString(2, 4, "/");
    OLED_ShowNum(2, 5, pressure2, 1);
    OLED_ShowString(2, 7, "D:");
    if(distance > 0)
    {
        OLED_ShowNum(2, 9, distance, 3);
    }
    else
    {
        OLED_ShowString(2, 9, "---");
    }
    
    // 第3行：温湿度
    OLED_ShowString(3, 1, "T:");
    OLED_ShowNum(3, 3, temperature, 2);
    OLED_ShowString(3, 5, "C");
    OLED_ShowString(3, 7, "H:");
    OLED_ShowNum(3, 9, humidity, 2);
    OLED_ShowString(3, 11, "%");
    
    // 第4行：持续时间
    if(current_seat_state == SEAT_OCCUPIED)
    {
        OLED_ShowString(4, 1, "Time:");
        if(occupied_duration < 60)
        {
            OLED_ShowNum(4, 6, occupied_duration, 2);
            OLED_ShowString(4, 8, "s ");
        }
        else
        {
            OLED_ShowNum(4, 6, occupied_duration / 60, 2);
            OLED_ShowString(4, 8, "m ");
        }
    }
    else
    {
        OLED_ShowString(4, 1, "Idle:");
        if(vacant_duration < 60)
        {
            OLED_ShowNum(4, 6, vacant_duration, 2);
            OLED_ShowString(4, 8, "s ");
        }
        else
        {
            OLED_ShowNum(4, 6, vacant_duration / 60, 2);
            OLED_ShowString(4, 8, "m ");
        }
    }
}

/*=============================================================================
 * 函数：Send_Data_To_ESP32
 * 功能：发送座位状态和传感器数据到ESP32
 *============================================================================*/
void Send_Data_To_ESP32(void)
{
    SensorData_t sensor_data;
    DeviceState_t device_state;
    
    // 准备传感器数据
    sensor_data.temperature = temperature;
    sensor_data.humidity = humidity;
    sensor_data.light_adc = LightSensor_ReadADC();
    sensor_data.light_percent = light_percent;
    sensor_data.light_digital = LightSensor_GetDigital();
    
    // 准备设备状态
    device_state.led_state = (current_seat_state == SEAT_VACANT) ? 1 : 0;
    device_state.fan_state = (current_seat_state == SEAT_OCCUPIED) ? 1 : 0;
    device_state.curtain_state = current_seat_state;
    device_state.buzzer_state = 0;
    
    // 发送数据
    ESP32_SendSensorData(&sensor_data);
    ESP32_SendDeviceState(&device_state);
    
    printf("[ESP32] Data sent - State:%s, P:%d/%d, Dist:%dcm, T:%dC, H:%d%%\r\n",
           current_seat_state == SEAT_OCCUPIED ? "OCCUPIED" : "VACANT",
           pressure1, pressure2, distance, temperature, humidity);
}

/*=============================================================================
 * 函数：Handle_Key_Input
 * 功能：处理按键输入
 *============================================================================*/
void Handle_Key_Input(void)
{
    uint8_t keyNum = Key_GetNum();
    
    if(keyNum == 1)  // K1：显示传感器详细信息
    {
        printf("[KEY] K1 Pressed - Show sensor details\r\n");
        Buzzer_Beep(50);
        
        OLED_Clear();
        OLED_ShowString(1, 1, "P1:");
        OLED_ShowNum(1, 4, pressure1, 1);
        OLED_ShowString(1, 6, "P2:");
        OLED_ShowNum(1, 9, pressure2, 1);
        
        OLED_ShowString(2, 1, "Dist:");
        OLED_ShowNum(2, 6, distance, 3);
        OLED_ShowString(2, 9, "cm");
        
        OLED_ShowString(3, 1, "PIR:");
        OLED_ShowNum(3, 5, pir_state, 1);
        OLED_ShowString(3, 7, "L:");
        OLED_ShowNum(3, 9, light_percent, 3);
        OLED_ShowString(3, 12, "%");
        
        OLED_ShowString(4, 1, "T:");
        OLED_ShowNum(4, 3, temperature, 2);
        OLED_ShowString(4, 5, "C");
        OLED_ShowString(4, 7, "H:");
        OLED_ShowNum(4, 9, humidity, 2);
        OLED_ShowString(4, 11, "%");
        
        Delay_ms(3000);
    }
    else if(keyNum == 2)  // K2：手动设置为占用
    {
        printf("[KEY] K2 Pressed - Manual set to OCCUPIED\r\n");
        Buzzer_Beep(100);
        
        current_seat_state = SEAT_OCCUPIED;
        occupied_duration = 0;
        
        OLED_Clear();
        OLED_ShowString(1, 1, "Manual Mode");
        OLED_ShowString(2, 1, "Set:OCCUPIED");
        Delay_ms(2000);
    }
    else if(keyNum == 3)  // K3：手动设置为空闲
    {
        printf("[KEY] K3 Pressed - Manual set to VACANT\r\n");
        Buzzer_Beep(100);
        
        current_seat_state = SEAT_VACANT;
        vacant_duration = 0;
        
        OLED_Clear();
        OLED_ShowString(1, 1, "Manual Mode");
        OLED_ShowString(2, 1, "Set:VACANT");
        Delay_ms(2000);
    }
    else if(keyNum == 4)  // K4：刷新所有传感器
    {
        printf("[KEY] K4 Pressed - Refresh all sensors\r\n");
        Buzzer_Beep(50);
        
        pressure1 = Pressure_GetState1();
        pressure2 = Pressure_GetState2();
        distance = Ultrasonic_GetDistance();
        pir_state = PIR_GetState();
        light_percent = LightSensor_GetPercent();
        
#if DHT11_ENABLED
        DHT11_Read(&temperature, &humidity);
#endif
        
        printf("[REFRESH] All sensors updated\r\n");
        printf("[SENSOR] P1:%d P2:%d Dist:%dcm PIR:%d Light:%d%%\r\n",
               pressure1, pressure2, distance, pir_state, light_percent);
    }
}

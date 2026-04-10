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
#include "ESP32_Comm.h"

/*=============================================================================
 * 座位占用检测系统 - STM32主程序
 * 功能：检测座位是否有人，并通过WiFi上报状态
 * 硬件：STM32F103C8T6 + ESP32-S3 + PIR + DHT11 + 光敏 + OLED + LED + 蜂鸣器
 *============================================================================*/

// 座位状态定义
typedef enum {
    SEAT_VACANT = 0,    // 空闲
    SEAT_OCCUPIED = 1   // 占用
} SeatState_t;

// 全局变量
SeatState_t current_seat_state = SEAT_VACANT;
SeatState_t last_seat_state = SEAT_VACANT;
uint32_t state_change_time = 0;
uint32_t occupied_duration = 0;
uint32_t vacant_duration = 0;

// DHT11温湿度数据（全局变量）
uint8_t temperature = 0;
uint8_t humidity = 0;

// DHT11功能开关（如果硬件未连接，设置为0禁用）
#define DHT11_ENABLED 1  // 0=禁用, 1=启用

// 函数声明
void System_Init(void);
void Display_Welcome(void);
void Update_Seat_State(void);
void Display_Seat_Status(void);
void Send_Data_To_ESP32(void);
void Handle_Key_Input(void);

int main(void)
{
    /*========== 系统初始化 ==========*/
    System_Init();
    
    /*========== 显示欢迎信息 ==========*/
    Display_Welcome();
    
    /*========== 变量定义 ==========*/
    uint16_t light_adc = 0;
    uint8_t light_percent = 0;
    uint8_t pir_state = 0;
    
    // 定时器
    uint32_t sensor_timer = 0;      // 传感器读取定时器（每2秒）
    uint32_t display_timer = 0;     // 显示更新定时器（每1秒）
    uint32_t esp32_timer = 0;       // ESP32数据发送定时器（每5秒）
    uint32_t state_timer = 0;       // 状态持续时间计数器（每秒）
    
    /*========== 等待传感器稳定 ==========*/
    printf("\r\n[SYSTEM] Waiting for sensors to stabilize...\r\n");
    OLED_ShowString(1, 1, "Initializing");
    OLED_ShowString(2, 1, "Please wait");
    OLED_ShowString(3, 1, "PIR warming");
    OLED_ShowString(4, 1, "up...");
    
    // PIR传感器需要预热30秒
    for(int i = 30; i > 0; i--)
    {
        OLED_ShowString(4, 1, "Wait:");
        OLED_ShowNum(4, 6, i, 2);
        OLED_ShowString(4, 8, "s ");
        Delay_ms(1000);
    }
    
    printf("[SYSTEM] Sensors ready! Starting seat detection...\r\n\r\n");
    
#if DHT11_ENABLED
    printf("[INFO] DHT11 temperature/humidity sensor: ENABLED\r\n");
#else
    printf("[INFO] DHT11 temperature/humidity sensor: DISABLED\r\n");
    printf("[INFO] Temperature and humidity will show as 0\r\n");
#endif
    
    Buzzer_Beep(200);  // 启动提示音
    
    /*========== 主循环 ==========*/
    while (1)
    {
        /*---------- 1. 处理ESP32通信 ----------*/
        ESP32_ProcessReceivedData();
        
        /*---------- 2. 处理按键输入 ----------*/
        Handle_Key_Input();
        
        /*---------- 3. 读取传感器数据（每2秒） ----------*/
        if(++sensor_timer >= 200000)
        {
            sensor_timer = 0;
            
            // 读取PIR传感器
            pir_state = PIR_GetState();
            
#if DHT11_ENABLED
            // 读取温湿度（仅在启用时）
            DHT11_Read(&temperature, &humidity);
#endif
            
            // 读取光照强度
            light_adc = LightSensor_ReadADC();
            light_percent = LightSensor_GetPercent();
            
            printf("[SENSOR] PIR:%d, Temp:%dC, Humi:%d%%, Light:%d%%\r\n", 
                   pir_state, temperature, humidity, light_percent);
        }
        
        /*---------- 4. 更新座位状态（每0.5秒检测一次） ----------*/
        static uint32_t state_check_timer = 0;
        if(++state_check_timer >= 50000)
        {
            state_check_timer = 0;
            Update_Seat_State();
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
        
        /*---------- 8. 简单延时控制循环频率 ----------*/
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
    printf("  Seat Occupancy Detection System\r\n");
    printf("  STM32F103C8T6 + ESP32-S3\r\n");
    printf("========================================\r\n");
    
    printf("[INIT] Initializing OLED...\r\n");
    OLED_Init();
    
    printf("[INIT] Initializing DHT11...\r\n");
    DHT11_Init();
    
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
    printf("  === PA Port: Sensors + Actuators ===\r\n");
    printf("  DHT11         : PA0 (Single Wire)\r\n");
    printf("  Light Sensor  : PA1 (ADC), PA2 (Digital)\r\n");
    printf("  PIR Sensor    : PA3 (GPIO Input)\r\n");
    printf("  Buzzer        : PA4 (Low Level Trigger)\r\n");
    printf("  Green LED     : PA5 (Vacant Indicator)\r\n");
    printf("  Red LED       : PA6 (Occupied Indicator)\r\n");
    printf("  === PB Port: Communication + Keys ===\r\n");
    printf("  OLED Display  : PB6(SCL), PB7(SDA)\r\n");
    printf("  ESP32 Comm    : PB10(TX), PB11(RX)\r\n");
    printf("  Keys          : PB12(K1), PB13(K2), PB14(K3), PB15(K4)\r\n");
    printf("========================================\r\n\r\n");
}

/*=============================================================================
 * 函数：Display_Welcome
 * 功能：显示欢迎信息并等待DHT11稳定
 *============================================================================*/
void Display_Welcome(void)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "Seat Detection");
    OLED_ShowString(2, 1, "System v1.0");
    OLED_ShowString(3, 1, "Initializing");
    OLED_ShowString(4, 1, "Please wait..");
    
#if DHT11_ENABLED
    printf("[SYSTEM] Waiting for DHT11 to stabilize...\r\n");
    
    // 重要：在所有GPIO初始化完成后，重新初始化DHT11
    // 因为其他模块的GPIO_Init可能影响了PA0的配置
    printf("[DHT11_DEBUG] Re-initializing DHT11 after all GPIO init...\r\n");
    DHT11_Init();
    
    // 测试PA0引脚状态
    printf("[DHT11_DEBUG] Testing PA0 pin state...\r\n");
    printf("[DHT11_DEBUG] PA0 initial state: %d (should be 1)\r\n", GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0));
    
    Delay_ms(2000);  // DHT11需要上电后等待1-2秒
    
    printf("[DHT11_DEBUG] PA0 after 2s delay: %d\r\n", GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0));
    
    // 尝试多次读取DHT11（最多3次）
    uint8_t temp = 0, humi = 0;
    uint8_t retry = 0;
    for(retry = 0; retry < 3; retry++)
    {
        printf("[DHT11_DEBUG] Attempt %d - Starting read...\r\n", retry+1);
        uint8_t result = DHT11_Read(&temp, &humi);
        
        if(result == 0) {
            printf("[DHT11] Read OK (attempt %d) - Temp:%dC, Humi:%d%%\r\n", retry+1, temp, humi);
            temperature = temp;
            humidity = humi;
            break;
        } else {
            printf("[DHT11] Read failed (attempt %d) - Error code: %d\r\n", retry+1, result);
            if(retry < 2) {
                printf("[DHT11_DEBUG] Waiting 2s before retry...\r\n");
                Delay_ms(2000);  // 等待2秒后重试
            }
        }
    }
    
    if(retry >= 3) {
        printf("[DHT11] All attempts failed. Possible causes:\r\n");
        printf("  1. DHT11 not connected to PA0\r\n");
        printf("  2. Missing pull-up resistor (4.7K-10K between DATA and VCC)\r\n");
        printf("  3. DHT11 power supply issue\r\n");
        printf("  4. DHT11 sensor damaged\r\n");
        printf("[DHT11] System will continue without temperature/humidity data\r\n");
    }
#else
    printf("[SYSTEM] DHT11 disabled in configuration\r\n");
    Delay_ms(1000);
#endif
}

/*=============================================================================
 * 函数：Update_Seat_State
 * 功能：更新座位状态（核心逻辑）
 * 算法：
 *   - PIR检测到人体 → 立即判定为占用
 *   - PIR持续无人体30秒 → 判定为空闲
 *   - 光照传感器辅助判断（人体遮挡会降低光照）
 *============================================================================*/
void Update_Seat_State(void)
{
    static uint8_t no_motion_count = 0;  // 无人体检测计数（每0.5秒一次）
    static uint8_t motion_count = 0;     // 有人体检测计数
    static uint8_t low_light_count = 0;  // 低光照计数
    
    uint8_t pir_state = PIR_GetState();
    uint8_t light_percent = LightSensor_GetPercent();
    
    // 检测到人体红外信号
    if(pir_state == 1)
    {
        motion_count++;
        no_motion_count = 0;
        
        // 连续检测到3次人体信号，判定为占用
        if(motion_count >= 3 && current_seat_state == SEAT_VACANT)
        {
            current_seat_state = SEAT_OCCUPIED;
            state_change_time = 0;
            occupied_duration = 0;
            
            printf("\r\n[STATE CHANGE] VACANT -> OCCUPIED\r\n");
            printf("[REASON] PIR detected human presence\r\n\r\n");
            
            // 状态变化提示
            Buzzer_Beep(200);
            Delay_ms(100);
            Buzzer_Beep(200);
        }
    }
    else
    {
        motion_count = 0;
        no_motion_count++;
    }
    
    // 光照传感器判断：光照低于50%认为有人遮挡
    if(light_percent < 50)
    {
        low_light_count++;
        no_motion_count = 0;  // 重置无人计数
        
        // 连续检测到低光照，判定为占用
        if(low_light_count >= 5 && current_seat_state == SEAT_VACANT)
        {
            current_seat_state = SEAT_OCCUPIED;
            state_change_time = 0;
            occupied_duration = 0;
            
            printf("\r\n[STATE CHANGE] VACANT -> OCCUPIED\r\n");
            printf("[REASON] Light sensor detected obstruction (Light: %d%%)\r\n\r\n", light_percent);
            
            Buzzer_Beep(200);
            Delay_ms(100);
            Buzzer_Beep(200);
        }
    }
    else
    {
        low_light_count = 0;
        
        // 光照恢复正常且持续60次（30秒）无人体信号，判定为空闲
        if(no_motion_count >= 60 && current_seat_state == SEAT_OCCUPIED)
        {
            current_seat_state = SEAT_VACANT;
            state_change_time = 0;
            vacant_duration = 0;
            
            printf("\r\n[STATE CHANGE] OCCUPIED -> VACANT\r\n");
            printf("[REASON] Light restored and no motion for 30s (Light: %d%%)\r\n\r\n", light_percent);
            
            // 状态变化提示
            Buzzer_Beep(200);
        }
    }
    
    // 辅助判断：光照突然降低可能有人坐下
    static uint8_t last_light_percent = 100;
    if(light_percent < last_light_percent - 20 && current_seat_state == SEAT_VACANT)
    {
        printf("[HINT] Light dropped significantly, possible occupancy\r\n");
    }
    last_light_percent = light_percent;
    
    // 更新LED指示灯
    if(current_seat_state == SEAT_OCCUPIED)
    {
        LED_Off();           // 关闭绿灯（PA5）
        GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_SET);  // 打开红灯（PA6）
    }
    else
    {
        LED_On();            // 打开绿灯（PA5）
        GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_RESET); // 关闭红灯（PA6）
    }
}

/*=============================================================================
 * 函数：Display_Seat_Status
 * 功能：在OLED上显示座位状态
 *============================================================================*/
void Display_Seat_Status(void)
{
    // 温湿度数据使用全局变量（每2秒在主循环中更新）
    
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
    
    // 第2行：温度
    OLED_ShowString(2, 1, "Temp:");
    OLED_ShowNum(2, 6, temperature, 2);
    OLED_ShowString(2, 8, "C");
    
    // 第3行：湿度
    OLED_ShowString(3, 1, "Humi:");
    OLED_ShowNum(3, 6, humidity, 2);
    OLED_ShowString(3, 8, "%");
    
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
    
    // 使用全局变量中的温湿度数据（每2秒在主循环中更新）
    
    // 准备传感器数据
    sensor_data.temperature = temperature;
    sensor_data.humidity = humidity;
    sensor_data.light_adc = LightSensor_ReadADC();
    sensor_data.light_percent = LightSensor_GetPercent();
    sensor_data.light_digital = LightSensor_GetDigital();
    
    // 准备设备状态（座位状态）
    device_state.led_state = (current_seat_state == SEAT_VACANT) ? 1 : 0;  // 绿灯=空闲
    device_state.fan_state = (current_seat_state == SEAT_OCCUPIED) ? 1 : 0; // 红灯=占用（复用fan_state）
    device_state.curtain_state = current_seat_state;  // 座位状态
    device_state.buzzer_state = 0;
    
    // 发送数据
    ESP32_SendSensorData(&sensor_data);
    ESP32_SendDeviceState(&device_state);
    
    printf("[ESP32] Data sent - State:%s, T:%dC, H:%d%%, L:%d%%\r\n",
           current_seat_state == SEAT_OCCUPIED ? "OCCUPIED" : "VACANT",
           sensor_data.temperature,
           sensor_data.humidity,
           sensor_data.light_percent);
}

/*=============================================================================
 * 函数：Handle_Key_Input
 * 功能：处理按键输入
 *============================================================================*/
void Handle_Key_Input(void)
{
    uint8_t keyNum = Key_GetNum();
    
    if(keyNum == 1)  // K1：手动刷新传感器
    {
        printf("[KEY] K1 Pressed - Manual sensor refresh\r\n");
        Buzzer_Beep(50);
        
#if DHT11_ENABLED
        // 强制读取DHT11
        if(DHT11_Read(&temperature, &humidity) == 0)
        {
            printf("[DHT11] T:%dC, H:%d%%\r\n", temperature, humidity);
        }
#else
        printf("[DHT11] Disabled\r\n");
#endif
        
        Display_Seat_Status();
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
        OLED_ShowString(3, 1, "Press K3 to");
        OLED_ShowString(4, 1, "set VACANT");
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
        OLED_ShowString(3, 1, "Press K2 to");
        OLED_ShowString(4, 1, "set OCCUPIED");
        Delay_ms(2000);
    }
    else if(keyNum == 4)  // K4：显示详细信息
    {
        printf("[KEY] K4 Pressed - Show detailed info\r\n");
        Buzzer_Beep(50);
        
#if DHT11_ENABLED
        // 强制读取DHT11
        DHT11_Read(&temperature, &humidity);
#endif
        
        OLED_Clear();
        OLED_ShowString(1, 1, "PIR:");
        OLED_ShowNum(1, 5, PIR_GetState(), 1);
        OLED_ShowString(2, 1, "Light:");
        OLED_ShowNum(2, 7, LightSensor_GetPercent(), 3);
        OLED_ShowString(2, 10, "%");
        OLED_ShowString(3, 1, "Temp:");
        OLED_ShowNum(3, 6, temperature, 2);
        OLED_ShowString(3, 8, "C");
        OLED_ShowString(4, 1, "Humi:");
        OLED_ShowNum(4, 6, humidity, 2);
        OLED_ShowString(4, 8, "%");
        Delay_ms(3000);
    }
}

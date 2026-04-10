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
 * 双座位检测系统
 * 功能：利用两个压力传感器通道，模拟两个独立座位的检测
 * 
 * 座位1：压力传感器CH1 (PB4)
 * 座位2：压力传感器CH2 (PB5)
 * 
 * 共享传感器：
 * - 超声波、PIR、DHT11、光敏传感器作为环境数据
 *============================================================================*/

// 座位状态定义
typedef enum {
    SEAT_VACANT = 0,
    SEAT_OCCUPIED = 1
} SeatState_t;

// 双座位数据结构
typedef struct {
    SeatState_t state;
    uint32_t occupied_duration;
    uint32_t vacant_duration;
    uint8_t pressure;
} SeatInfo_t;

// 全局变量
SeatInfo_t seat1 = {SEAT_VACANT, 0, 0, 0};
SeatInfo_t seat2 = {SEAT_VACANT, 0, 0, 0};

// 共享传感器数据
uint8_t temperature = 0;
uint8_t humidity = 0;
uint16_t distance = 0;
uint8_t pir_state = 0;
uint8_t light_percent = 0;

// 功能开关
#define DHT11_ENABLED 1

// 函数声明
void System_Init(void);
void Display_Welcome(void);
void Update_Dual_Seat_State(void);
void Display_Dual_Seat_Status(void);
void Send_Dual_Seat_Data_To_ESP32(void);
void Handle_Key_Input(void);

int main(void)
{
    System_Init();
    Display_Welcome();
    
    uint32_t sensor_timer = 0;
    uint32_t display_timer = 0;
    uint32_t esp32_timer = 0;
    uint32_t state_timer = 0;
    
    printf("[SYSTEM] Dual-Seat detection system started!\r\n");
    printf("[SYSTEM] Seat1: Pressure CH1 (PB4)\r\n");
    printf("[SYSTEM] Seat2: Pressure CH2 (PB5)\r\n\r\n");
    Buzzer_Beep(200);
    
    while (1)
    {
        ESP32_ProcessReceivedData();
        Handle_Key_Input();
        
        // 读取传感器（每1秒）
        if(++sensor_timer >= 100000)
        {
            sensor_timer = 0;
            
            seat1.pressure = Pressure_GetState1();
            seat2.pressure = Pressure_GetState2();
            distance = Ultrasonic_GetDistance();
            pir_state = PIR_GetState();
            
#if DHT11_ENABLED
            static uint32_t dht11_timer = 0;
            if(++dht11_timer >= 2)
            {
                dht11_timer = 0;
                DHT11_Read(&temperature, &humidity);
            }
#endif
            
            light_percent = LightSensor_GetPercent();
            
            printf("[SENSOR] Seat1:%d Seat2:%d Dist:%dcm PIR:%d T:%dC H:%d%% L:%d%%\r\n", 
                   seat1.pressure, seat2.pressure, distance, pir_state,
                   temperature, humidity, light_percent);
        }
        
        // 更新座位状态（每0.5秒）
        static uint32_t state_check_timer = 0;
        if(++state_check_timer >= 50000)
        {
            state_check_timer = 0;
            Update_Dual_Seat_State();
        }
        
        // 更新显示（每1秒）
        if(++display_timer >= 100000)
        {
            display_timer = 0;
            Display_Dual_Seat_Status();
        }
        
        // 发送数据到ESP32（每5秒）
        if(++esp32_timer >= 500000)
        {
            esp32_timer = 0;
            Send_Dual_Seat_Data_To_ESP32();
        }
        
        // 更新持续时间（每秒）
        if(++state_timer >= 100000)
        {
            state_timer = 0;
            
            if(seat1.state == SEAT_OCCUPIED)
                seat1.occupied_duration++;
            else
                seat1.vacant_duration++;
                
            if(seat2.state == SEAT_OCCUPIED)
                seat2.occupied_duration++;
            else
                seat2.vacant_duration++;
        }
        
        Delay_us(100);
    }
}

void System_Init(void)
{
    Serial_Init();
    printf("\r\n========================================\r\n");
    printf("  Dual-Seat Detection System\r\n");
    printf("  Two Independent Seats\r\n");
    printf("========================================\r\n");
    
    printf("[INIT] Initializing OLED...\r\n");
    OLED_Init();
    
    printf("[INIT] Initializing DHT11...\r\n");
    DHT11_Init();
    
    printf("[INIT] Initializing Pressure sensors...\r\n");
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
    
    printf("\r\n[CONFIG] Seat Configuration:\r\n");
    printf("  Seat 1: Pressure CH1 (PB4)\r\n");
    printf("  Seat 2: Pressure CH2 (PB5)\r\n");
    printf("  Shared: Ultrasonic, PIR, DHT11, Light\r\n");
    printf("========================================\r\n\r\n");
}

void Display_Welcome(void)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "Dual-Seat Sys");
    OLED_ShowString(2, 1, "Initializing");
    OLED_ShowString(3, 1, "Please wait..");
    
#if DHT11_ENABLED
    printf("[SYSTEM] Waiting for DHT11...\r\n");
    DHT11_Init();
    Delay_ms(2000);
    
    uint8_t temp = 0, humi = 0;
    if(DHT11_Read(&temp, &humi) == 0) {
        printf("[DHT11] OK - Temp:%dC, Humi:%d%%\r\n", temp, humi);
        temperature = temp;
        humidity = humi;
    }
#endif
    
    printf("[SYSTEM] Testing pressure sensors...\r\n");
    seat1.pressure = Pressure_GetState1();
    seat2.pressure = Pressure_GetState2();
    printf("[PRESSURE] Seat1:%d, Seat2:%d\r\n", seat1.pressure, seat2.pressure);
}

void Update_Dual_Seat_State(void)
{
    static uint8_t last_state1 = SEAT_VACANT;
    static uint8_t last_state2 = SEAT_VACANT;
    static uint8_t confirm_count1 = 0;
    static uint8_t confirm_count2 = 0;
    
    // 座位1状态判断
    uint8_t new_state1 = (seat1.pressure == 1) ? SEAT_OCCUPIED : SEAT_VACANT;
    
    if(new_state1 == last_state1)
    {
        confirm_count1++;
        if(confirm_count1 >= 3 && new_state1 != seat1.state)
        {
            seat1.state = new_state1;
            confirm_count1 = 0;
            
            if(seat1.state == SEAT_OCCUPIED)
            {
                seat1.occupied_duration = 0;
                printf("\r\n[SEAT1] VACANT -> OCCUPIED\r\n");
                Buzzer_Beep(100);
            }
            else
            {
                seat1.vacant_duration = 0;
                printf("\r\n[SEAT1] OCCUPIED -> VACANT\r\n");
                Buzzer_Beep(50);
            }
        }
    }
    else
    {
        last_state1 = new_state1;
        confirm_count1 = 0;
    }
    
    // 座位2状态判断
    uint8_t new_state2 = (seat2.pressure == 1) ? SEAT_OCCUPIED : SEAT_VACANT;
    
    if(new_state2 == last_state2)
    {
        confirm_count2++;
        if(confirm_count2 >= 3 && new_state2 != seat2.state)
        {
            seat2.state = new_state2;
            confirm_count2 = 0;
            
            if(seat2.state == SEAT_OCCUPIED)
            {
                seat2.occupied_duration = 0;
                printf("\r\n[SEAT2] VACANT -> OCCUPIED\r\n");
                Buzzer_Beep(100);
            }
            else
            {
                seat2.vacant_duration = 0;
                printf("\r\n[SEAT2] OCCUPIED -> VACANT\r\n");
                Buzzer_Beep(50);
            }
        }
    }
    else
    {
        last_state2 = new_state2;
        confirm_count2 = 0;
    }
    
    // 更新LED（任一座位占用则红灯亮）
    if(seat1.state == SEAT_OCCUPIED || seat2.state == SEAT_OCCUPIED)
    {
        LED_Off();
        GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_SET);
    }
    else
    {
        LED_On();
        GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_RESET);
    }
}

void Display_Dual_Seat_Status(void)
{
    OLED_Clear();
    
    // 第1行：座位1状态
    OLED_ShowString(1, 1, "S1:");
    if(seat1.state == SEAT_OCCUPIED)
        OLED_ShowString(1, 4, "OCC");
    else
        OLED_ShowString(1, 4, "VAC");
    
    // 座位1时间
    if(seat1.state == SEAT_OCCUPIED && seat1.occupied_duration < 60)
    {
        OLED_ShowNum(1, 8, seat1.occupied_duration, 2);
        OLED_ShowString(1, 10, "s");
    }
    else if(seat1.state == SEAT_OCCUPIED)
    {
        OLED_ShowNum(1, 8, seat1.occupied_duration / 60, 2);
        OLED_ShowString(1, 10, "m");
    }
    
    // 第2行：座位2状态
    OLED_ShowString(2, 1, "S2:");
    if(seat2.state == SEAT_OCCUPIED)
        OLED_ShowString(2, 4, "OCC");
    else
        OLED_ShowString(2, 4, "VAC");
    
    // 座位2时间
    if(seat2.state == SEAT_OCCUPIED && seat2.occupied_duration < 60)
    {
        OLED_ShowNum(2, 8, seat2.occupied_duration, 2);
        OLED_ShowString(2, 10, "s");
    }
    else if(seat2.state == SEAT_OCCUPIED)
    {
        OLED_ShowNum(2, 8, seat2.occupied_duration / 60, 2);
        OLED_ShowString(2, 10, "m");
    }
    
    // 第3行：温湿度
    OLED_ShowString(3, 1, "T:");
    OLED_ShowNum(3, 3, temperature, 2);
    OLED_ShowString(3, 5, "C");
    OLED_ShowString(3, 7, "H:");
    OLED_ShowNum(3, 9, humidity, 2);
    OLED_ShowString(3, 11, "%");
    
    // 第4行：距离和光照
    OLED_ShowString(4, 1, "D:");
    if(distance > 0)
        OLED_ShowNum(4, 3, distance, 3);
    else
        OLED_ShowString(4, 3, "---");
    OLED_ShowString(4, 7, "L:");
    OLED_ShowNum(4, 9, light_percent, 3);
}

void Send_Dual_Seat_Data_To_ESP32(void)
{
    // 发送座位1数据
    // 这里需要修改ESP32_Comm来支持多座位，暂时发送座位1的状态
    SensorData_t sensor_data;
    DeviceState_t device_state;
    
    sensor_data.temperature = temperature;
    sensor_data.humidity = humidity;
    sensor_data.light_adc = LightSensor_ReadADC();
    sensor_data.light_percent = light_percent;
    sensor_data.light_digital = LightSensor_GetDigital();
    
    device_state.led_state = (seat1.state == SEAT_VACANT) ? 1 : 0;
    device_state.fan_state = (seat1.state == SEAT_OCCUPIED) ? 1 : 0;
    device_state.curtain_state = seat1.state;
    device_state.buzzer_state = 0;
    
    ESP32_SendSensorData(&sensor_data);
    ESP32_SendDeviceState(&device_state);
    
    printf("[ESP32] Sent - Seat1:%s Seat2:%s T:%dC H:%d%%\r\n",
           seat1.state == SEAT_OCCUPIED ? "OCC" : "VAC",
           seat2.state == SEAT_OCCUPIED ? "OCC" : "VAC",
           temperature, humidity);
}

void Handle_Key_Input(void)
{
    uint8_t keyNum = Key_GetNum();
    
    if(keyNum == 1)  // K1：显示详细信息
    {
        printf("[KEY] K1 - Show details\r\n");
        Buzzer_Beep(50);
        
        OLED_Clear();
        OLED_ShowString(1, 1, "Seat1:");
        OLED_ShowNum(1, 7, seat1.pressure, 1);
        OLED_ShowString(1, 9, seat1.state ? "OCC" : "VAC");
        
        OLED_ShowString(2, 1, "Seat2:");
        OLED_ShowNum(2, 7, seat2.pressure, 1);
        OLED_ShowString(2, 9, seat2.state ? "OCC" : "VAC");
        
        OLED_ShowString(3, 1, "Dist:");
        OLED_ShowNum(3, 6, distance, 3);
        OLED_ShowString(3, 9, "cm");
        
        OLED_ShowString(4, 1, "PIR:");
        OLED_ShowNum(4, 5, pir_state, 1);
        OLED_ShowString(4, 7, "L:");
        OLED_ShowNum(4, 9, light_percent, 3);
        
        Delay_ms(3000);
    }
    else if(keyNum == 2)  // K2：座位1手动占用
    {
        printf("[KEY] K2 - Seat1 OCCUPIED\r\n");
        Buzzer_Beep(100);
        seat1.state = SEAT_OCCUPIED;
        seat1.occupied_duration = 0;
    }
    else if(keyNum == 3)  // K3：座位2手动占用
    {
        printf("[KEY] K3 - Seat2 OCCUPIED\r\n");
        Buzzer_Beep(100);
        seat2.state = SEAT_OCCUPIED;
        seat2.occupied_duration = 0;
    }
    else if(keyNum == 4)  // K4：全部重置为空闲
    {
        printf("[KEY] K4 - Reset all to VACANT\r\n");
        Buzzer_Beep(50);
        seat1.state = SEAT_VACANT;
        seat1.vacant_duration = 0;
        seat2.state = SEAT_VACANT;
        seat2.vacant_duration = 0;
    }
}

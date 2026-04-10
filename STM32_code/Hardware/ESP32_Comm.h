#ifndef __ESP32_COMM_H
#define __ESP32_COMM_H

#include "stm32f10x.h"

// 数据包类型定义
typedef enum
{
    DATA_TYPE_SENSOR = 0x01,    // 传感器数据
    DATA_TYPE_DEVICE = 0x02,    // 设备状态
    DATA_TYPE_CONTROL = 0x03,   // 控制命令
    DATA_TYPE_ALARM = 0x04      // 报警信息
} DataType_t;

// 传感器数据结构
typedef struct
{
    uint8_t temperature;        // 温度
    uint8_t humidity;          // 湿度
    uint16_t light_adc;        // 光照ADC值
    uint8_t light_percent;     // 光照百分比
    uint8_t light_digital;     // 光照数字值
} SensorData_t;

// 设备状态结构
typedef struct
{
    uint8_t led_state;         // LED状态
    uint8_t fan_state;         // 风扇状态
    uint8_t curtain_state;     // 窗帘状态
    uint8_t buzzer_state;      // 蜂鸣器状态
} DeviceState_t;

// 控制命令结构
typedef struct
{
    uint8_t device_id;         // 设备ID: 1-LED, 2-Fan, 3-Curtain, 4-Buzzer
    uint8_t command;           // 命令: 0-OFF, 1-ON, 2-Toggle, 3-Open, 4-Close
    uint8_t param;             // 参数（如蜂鸣器持续时间）
} ControlCmd_t;

// 数据包结构
typedef struct
{
    uint8_t header;            // 包头 0xAA
    uint8_t type;              // 数据类型
    uint8_t length;            // 数据长度
    uint8_t data[32];          // 数据内容
    uint8_t checksum;          // 校验和
    uint8_t tail;              // 包尾 0x55
} DataPacket_t;

// 函数声明
void ESP32_Comm_Init(void);
void ESP32_SendSensorData(SensorData_t *sensor_data);
void ESP32_SendDeviceState(DeviceState_t *device_state);
void ESP32_SendAlarm(uint8_t alarm_type, uint8_t alarm_level);
uint8_t ESP32_ReceiveCommand(ControlCmd_t *cmd);
void ESP32_ProcessReceivedData(void);

#endif
#ifndef __ESP32_COMM_H
#define __ESP32_COMM_H

#include "stm32f10x.h"

// 数据包类型定义（座位检测系统）
typedef enum
{
    DATA_TYPE_SENSOR = 0x01,    // 传感器数据（温湿度、光照、压力、超声波、PIR）
    DATA_TYPE_DEVICE = 0x02,    // 设备状态（LED、蜂鸣器、座位状态）
    DATA_TYPE_CONTROL = 0x03,   // 控制命令（远程控制LED、蜂鸣器）
    DATA_TYPE_SEAT = 0x04       // 座位状态数据（扩展）
} DataType_t;

// 传感器数据结构（环境数据）
typedef struct __attribute__((packed))
{
    uint8_t temperature;        // 温度
    uint8_t humidity;          // 湿度
    uint16_t light_adc;        // 光照ADC值
    uint8_t light_percent;     // 光照百分比
    uint8_t light_digital;     // 光照数字值
} SensorData_t;

// 座位检测数据结构（完整数据）
typedef struct __attribute__((packed))
{
    uint8_t seat_occupied;     // 座位占用状态（0=空闲，1=占用）
    uint8_t pressure1;         // 压力传感器1
    uint8_t pressure2;         // 压力传感器2
    uint16_t distance;         // 超声波距离（cm）
    uint8_t pir_state;         // PIR传感器状态
    uint8_t temperature;       // 温度
    uint8_t humidity;          // 湿度
    uint8_t light_percent;     // 光照百分比
    uint8_t detection_score;   // 检测分数（0-100）
    uint32_t duration;         // 持续时间（秒）
} SeatDetectionData_t;

// 设备状态结构（座位检测系统）
typedef struct
{
    uint8_t green_led;         // 绿色LED（空闲指示）
    uint8_t red_led;           // 红色LED（占用指示）
    uint8_t buzzer_state;      // 蜂鸣器状态
    uint8_t seat_occupied;     // 座位占用状态（0=空闲，1=占用）
} DeviceState_t;

// 控制命令结构（座位检测系统）
typedef struct
{
    uint8_t device_id;         // 设备ID: 1-绿色LED, 2-红色LED, 3-蜂鸣器
    uint8_t command;           // 命令: 0-OFF, 1-ON, 2-Toggle
    uint8_t param;             // 参数（如蜂鸣器持续时间，单位：秒）
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
void ESP32_SendSeatDetectionData(SeatDetectionData_t *seat_data);  // 新增
uint8_t ESP32_ReceiveCommand(ControlCmd_t *cmd);
void ESP32_ProcessReceivedData(void);

#endif
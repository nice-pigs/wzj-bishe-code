#include "ESP32_Comm.h"
#include "ESP32_Serial.h"
#include "string.h"
#include <stdio.h>

// 接收缓冲区
static uint8_t rx_buffer[64];
static uint8_t rx_index = 0;
static uint8_t packet_received = 0;
static DataPacket_t received_packet;  // 保存接收到的完整数据包

/**
  * 函    数：ESP32通信初始化
  * 参    数：无
  * 返 回 值：无
  */
void ESP32_Comm_Init(void)
{
    // 初始化ESP32专用串口
    ESP32_Serial_Init();
    rx_index = 0;
    packet_received = 0;
    memset(rx_buffer, 0, sizeof(rx_buffer));
}

/**
  * 函    数：计算校验和
  * 参    数：data - 数据指针, length - 数据长度
  * 返 回 值：校验和
  */
static uint8_t Calculate_Checksum(uint8_t *data, uint8_t length)
{
    uint8_t checksum = 0;
    for(uint8_t i = 0; i < length; i++)
    {
        checksum += data[i];
    }
    return checksum;
}

/**
  * 函    数：发送数据包
  * 参    数：packet - 数据包指针
  * 返 回 值：无
  */
static void ESP32_SendPacket(DataPacket_t *packet)
{
    // 计算校验和
    packet->checksum = Calculate_Checksum((uint8_t*)packet, sizeof(DataPacket_t) - 2);
    
    // 发送数据包
    ESP32_Serial_SendArray((uint8_t*)packet, sizeof(DataPacket_t));
}

/**
  * 函    数：发送传感器数据
  * 参    数：sensor_data - 传感器数据指针
  * 返 回 值：无
  */
void ESP32_SendSensorData(SensorData_t *sensor_data)
{
    DataPacket_t packet;
    
    packet.header = 0xAA;
    packet.type = DATA_TYPE_SENSOR;
    packet.length = sizeof(SensorData_t);
    memcpy(packet.data, sensor_data, sizeof(SensorData_t));
    packet.tail = 0x55;
    
    ESP32_SendPacket(&packet);
    
    printf("[ESP32] Sensor data sent - Temp:%dC, Humi:%d%%, Light:%d%%\r\n", 
           sensor_data->temperature, sensor_data->humidity, sensor_data->light_percent);
}

/**
  * 函    数：发送设备状态
  * 参    数：device_state - 设备状态指针
  * 返 回 值：无
  */
void ESP32_SendDeviceState(DeviceState_t *device_state)
{
    DataPacket_t packet;
    
    packet.header = 0xAA;
    packet.type = DATA_TYPE_DEVICE;
    packet.length = sizeof(DeviceState_t);
    memcpy(packet.data, device_state, sizeof(DeviceState_t));
    packet.tail = 0x55;
    
    ESP32_SendPacket(&packet);
    
    printf("[ESP32] Device state sent - Green:%d, Red:%d, Seat:%s\r\n", 
           device_state->green_led, device_state->red_led, 
           device_state->seat_occupied ? "OCCUPIED" : "VACANT");
}

/**
  * 函    数：发送座位检测完整数据
  * 参    数：seat_data - 座位检测数据指针
  * 返 回 值：无
  */
void ESP32_SendSeatDetectionData(SeatDetectionData_t *seat_data)
{
    DataPacket_t packet;
    
    packet.header = 0xAA;
    packet.type = DATA_TYPE_SEAT;
    packet.length = sizeof(SeatDetectionData_t);
    memcpy(packet.data, seat_data, sizeof(SeatDetectionData_t));
    packet.tail = 0x55;
    
    ESP32_SendPacket(&packet);
    
    printf("[ESP32] Seat data sent - State:%s, P:%d/%d, Dist:%dcm, PIR:%d, Score:%d\r\n", 
           seat_data->seat_occupied ? "OCCUPIED" : "VACANT",
           seat_data->pressure1, seat_data->pressure2,
           seat_data->distance, seat_data->pir_state,
           seat_data->detection_score);
    
    // 调试：打印数据结构大小和前几个字节
    printf("[DEBUG] SeatDetectionData_t size: %d bytes\r\n", sizeof(SeatDetectionData_t));
    printf("[DEBUG] Data bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
           packet.data[0], packet.data[1], packet.data[2], packet.data[3], packet.data[4],
           packet.data[5], packet.data[6], packet.data[7], packet.data[8], packet.data[9]);
}

/**
  * 函    数：处理接收到的数据
  * 参    数：无
  * 返 回 值：无
  */
void ESP32_ProcessReceivedData(void)
{
    uint8_t received_byte;
    
    // 检查是否有新数据
    while(ESP32_Serial_GetRxFlag())  // 改为while循环，处理所有待接收数据
    {
        received_byte = ESP32_Serial_GetRxData();
        
        // 寻找包头
        if(rx_index == 0 && received_byte == 0xAA)
        {
            rx_buffer[rx_index++] = received_byte;
        }
        // 接收数据
        else if(rx_index > 0 && rx_index < sizeof(rx_buffer))
        {
            rx_buffer[rx_index++] = received_byte;
            
            // 检查是否接收完整包 (固定包长度为sizeof(DataPacket_t))
            if(rx_index >= sizeof(DataPacket_t))
            {
                DataPacket_t *packet = (DataPacket_t*)rx_buffer;
                
                // 验证包的完整性
                if(packet->header == 0xAA && packet->tail == 0x55)
                {
                    // 验证校验和
                    uint8_t calc_checksum = Calculate_Checksum(rx_buffer, sizeof(DataPacket_t) - 2);
                    if(calc_checksum == packet->checksum)
                    {
                        // 保存数据包到received_packet
                        memcpy(&received_packet, packet, sizeof(DataPacket_t));
                        packet_received = 1;
                        printf("[ESP32] Valid packet received, Type: %d\r\n", packet->type);
                    }
                    else
                    {
                        printf("[ESP32] Checksum error (calc:%d, recv:%d)\r\n", calc_checksum, packet->checksum);
                    }
                }
                else
                {
                    printf("[ESP32] Invalid packet format\r\n");
                }
                
                // 重置接收缓冲区
                rx_index = 0;
                memset(rx_buffer, 0, sizeof(rx_buffer));
            }
        }
        else
        {
            // 缓冲区溢出或无效数据，重置
            rx_index = 0;
            memset(rx_buffer, 0, sizeof(rx_buffer));
        }
    }
}

/**
  * 函    数：接收控制命令
  * 参    数：cmd - 控制命令指针
  * 返 回 值：1-接收到命令, 0-无命令
  */
uint8_t ESP32_ReceiveCommand(ControlCmd_t *cmd)
{
    if(packet_received)
    {
        if(received_packet.type == DATA_TYPE_CONTROL)
        {
            memcpy(cmd, received_packet.data, sizeof(ControlCmd_t));
            packet_received = 0;
            
            printf("[ESP32] Control command received - Device:%d, Cmd:%d, Param:%d\r\n", 
                   cmd->device_id, cmd->command, cmd->param);
            
            return 1;
        }
        
        packet_received = 0;
    }
    
    return 0;
}
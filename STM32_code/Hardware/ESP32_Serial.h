#ifndef __ESP32_SERIAL_H
#define __ESP32_SERIAL_H

#include "stm32f10x.h"

void ESP32_Serial_Init(void);
void ESP32_Serial_SendByte(uint8_t Byte);
void ESP32_Serial_SendArray(uint8_t *Array, uint16_t Length);
void ESP32_Serial_SendString(char *String);
uint8_t ESP32_Serial_GetRxFlag(void);
uint8_t ESP32_Serial_GetRxData(void);

#endif
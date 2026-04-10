/*
 * 座位检测系统 - ESP32云端上传
 * 使用WiFiClient直接发送HTTP请求
 */

#include <WiFi.h>
#include <ArduinoJson.h>

// WiFi配置
const char* ssid = "cs";
const char* password = "88888888";

// 服务器配置
const char* serverIP = "10.150.235.41";
const int serverPort = 3000;

// STM32串口配置
#define STM32_SERIAL Serial2
#define STM32_RX_PIN 16
#define STM32_TX_PIN 17
#define STM32_BAUD 115200

// 座位状态数据
struct SeatData {
    bool occupied;
    int temperature;
    int humidity;
    int lightLevel;
    unsigned long duration;
    unsigned long lastUpdate;
    bool dataValid;
} seatData;

// 上传计时器
unsigned long lastUploadTime = 0;
unsigned long lastHeartbeatTime = 0;
const unsigned long UPLOAD_INTERVAL = 5000;
const unsigned long HEARTBEAT_INTERVAL = 10000;

WiFiClient client;

void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n=== ESP32 Seat Detection (WiFiClient) ===");
    
    // 初始化座位数据
    seatData.occupied = false;
    seatData.temperature = 0;
    seatData.humidity = 0;
    seatData.lightLevel = 0;
    seatData.duration = 0;
    seatData.lastUpdate = 0;
    seatData.dataValid = false;
    
    // STM32通信串口
    STM32_SERIAL.begin(STM32_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
    Serial.println("[INIT] STM32 Serial initialized");
    
    // 连接WiFi
    connectWiFi();
}

void loop() {
    processSTM32Data();
    
    if (millis() - lastUploadTime >= UPLOAD_INTERVAL) {
        lastUploadTime = millis();
        if (seatData.dataValid) {
            uploadDataToServer();
        }
    }
    
    if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
        lastHeartbeatTime = millis();
        sendHeartbeat();
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Reconnecting...");
        connectWiFi();
    }
    
    if (seatData.dataValid && (millis() - seatData.lastUpdate > 10000)) {
        seatData.dataValid = false;
        Serial.println("[WARNING] Data timeout");
    }
}

void connectWiFi() {
    Serial.printf("[WIFI] Connecting to %s", ssid);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Connected!");
        Serial.printf("[WIFI] IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WIFI] Signal: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("\n[WIFI] Failed!");
    }
}

void uploadDataToServer() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERROR] WiFi not connected");
        return;
    }
    
    Serial.printf("[UPLOAD] Connecting to %s:%d\n", serverIP, serverPort);
    
    if (!client.connect(serverIP, serverPort)) {
        Serial.println("[UPLOAD] ✗ Connection failed");
        return;
    }
    
    Serial.println("[UPLOAD] ✓ Connected to server");
    
    // 构建JSON
    StaticJsonDocument<256> doc;
    doc["occupied"] = seatData.occupied;
    doc["temperature"] = seatData.temperature;
    doc["humidity"] = seatData.humidity;
    doc["lightLevel"] = seatData.lightLevel;
    doc["duration"] = seatData.duration;
    
    String jsonData;
    serializeJson(doc, jsonData);
    
    // 构建HTTP POST请求
    String request = "POST /api/seat/update HTTP/1.1\r\n";
    request += "Host: " + String(serverIP) + "\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + String(jsonData.length()) + "\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
    request += jsonData;
    
    Serial.printf("[UPLOAD] Sending: %s\n", jsonData.c_str());
    
    // 发送请求
    client.print(request);
    
    // 等待响应
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println("[UPLOAD] ✗ Timeout");
            client.stop();
            return;
        }
    }
    
    // 读取响应
    String response = "";
    bool headerEnded = false;
    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (!headerEnded) {
            if (line.startsWith("HTTP/1.1")) {
                Serial.printf("[UPLOAD] Response: %s\n", line.c_str());
            }
            if (line == "\r") {
                headerEnded = true;
            }
        } else {
            response += line;
        }
    }
    
    if (response.length() > 0) {
        Serial.printf("[UPLOAD] ✓ SUCCESS - %s\n", 
            seatData.occupied ? "OCCUPIED" : "VACANT");
        Serial.printf("[UPLOAD] Body: %s\n", response.c_str());
    }
    
    client.stop();
}

void sendHeartbeat() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    if (!client.connect(serverIP, serverPort)) {
        return;
    }
    
    StaticJsonDocument<64> doc;
    doc["timestamp"] = millis();
    
    String jsonData;
    serializeJson(doc, jsonData);
    
    String request = "POST /api/device/heartbeat HTTP/1.1\r\n";
    request += "Host: " + String(serverIP) + "\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + String(jsonData.length()) + "\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
    request += jsonData;
    
    client.print(request);
    
    delay(100);
    
    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line.startsWith("HTTP/1.1 200")) {
            Serial.println("[HEARTBEAT] ✓ Sent");
            break;
        }
    }
    
    client.stop();
}

void processSTM32Data() {
    static uint8_t rxBuffer[64];
    static uint8_t rxIndex = 0;
    
    while (STM32_SERIAL.available()) {
        uint8_t byte = STM32_SERIAL.read();
        
        if (rxIndex == 0 && byte == 0xAA) {
            rxBuffer[rxIndex++] = byte;
        }
        else if (rxIndex > 0 && rxIndex < sizeof(rxBuffer)) {
            rxBuffer[rxIndex++] = byte;
            
            if (rxIndex >= 37) {
                if (rxBuffer[36] == 0x55) {
                    parseDataPacket(rxBuffer, rxIndex);
                } else {
                    Serial.println("[RX] ✗ Invalid packet");
                }
                rxIndex = 0;
            }
        }
        else {
            rxIndex = 0;
        }
    }
}

void parseDataPacket(uint8_t* buffer, uint8_t len) {
    if (len < 6) return;
    if (buffer[0] != 0xAA || buffer[36] != 0x55) return;
    
    uint8_t type = buffer[1];
    uint8_t length = buffer[2];
    uint8_t* data = &buffer[3];
    uint8_t checksum = buffer[35];
    
    uint8_t calc_checksum = 0;
    for (int i = 0; i < 35; i++) {
        calc_checksum += buffer[i];
    }
    
    if (calc_checksum != checksum) {
        Serial.printf("[RX] ✗ Checksum fail\n");
        return;
    }
    
    switch (type) {
        case 0x01: // 传感器数据
            if (length >= 5) {
                seatData.temperature = data[0];
                seatData.humidity = data[1];
                seatData.lightLevel = (data[2] << 8) | data[3];
                seatData.lastUpdate = millis();
                seatData.dataValid = true;
                Serial.printf("[DATA] ✓ Sensor - T:%d°C, H:%d%%, L:%d\n", 
                    seatData.temperature, seatData.humidity, seatData.lightLevel);
            }
            break;
            
        case 0x02: // 设备状态
            if (length >= 4) {
                // 新的DeviceState_t结构：green_led, red_led, buzzer_state, seat_occupied
                bool newOccupied = (data[3] == 1);  // seat_occupied是第4个字节
                if (newOccupied != seatData.occupied) {
                    Serial.printf("[DATA] ✓ State: %s -> %s\n",
                        seatData.occupied ? "OCCUPIED" : "VACANT",
                        newOccupied ? "OCCUPIED" : "VACANT");
                    seatData.occupied = newOccupied;
                    uploadDataToServer();
                } else {
                    seatData.occupied = newOccupied;
                }
                seatData.lastUpdate = millis();
                seatData.dataValid = true;
            }
            break;
    }
}

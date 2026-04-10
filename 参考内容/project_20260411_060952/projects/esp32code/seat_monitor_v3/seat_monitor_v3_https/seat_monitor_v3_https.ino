/*
 * 座位检测系统 - ESP32云端上传 (HTTPS版本)
 * 支持多传感器数据上传
 * 
 * 传感器数据：
 * - 温度/湿度 (DHT11)
 * - 光照强度 (光敏电阻)
 * - 压力传感器1/2
 * - 超声波距离
 * - PIR人体红外
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ============== WiFi配置 ==============
const char* ssid = "你的WiFi名称";
const char* password = "你的WiFi密码";

// ============== 服务器配置 (使用HTTPS) ==============
const char* serverHost = "smkcc339ny.coze.site";
const int serverPort = 443;  // HTTPS端口

// ============== 设备配置 ==============
const char* deviceKey = "esp32-seat-001";
const char* deviceName = "图书馆座位001";

// ============== STM32串口配置 ==============
#define STM32_SERIAL Serial2
#define STM32_RX_PIN 16
#define STM32_TX_PIN 17
#define STM32_BAUD 115200

// ============== 座位状态数据结构 ==============
struct SeatData {
    // 基础状态
    bool occupied;
    unsigned long duration;
    unsigned long lastUpdate;
    bool dataValid;
    
    // 传感器数据
    int temperature;      // 温度 (°C)
    int humidity;         // 湿度 (%)
    int lightLevel;       // 光照强度 (0-65535)
    
    // 检测传感器
    int pressure1;        // 压力传感器1 (0/1)
    int pressure2;        // 压力传感器2 (0/1)
    int distance;         // 超声波距离 (cm)
    int pirState;         // PIR红外状态 (0/1)
    
    // WiFi信息
    int wifiSignal;       // WiFi信号强度 (dBm)
} seatData;

// ============== 上传计时器 ==============
unsigned long lastUploadTime = 0;
unsigned long lastHeartbeatTime = 0;
const unsigned long UPLOAD_INTERVAL = 5000;    // 上传间隔 5秒
const unsigned long HEARTBEAT_INTERVAL = 10000; // 心跳间隔 10秒

WiFiClientSecure secureClient;

// ============== 初始化 ==============
void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n=== ESP32 Seat Detection - Coze Cloud (HTTPS) ===");
    Serial.println("Target: smkcc339ny.coze.site:443");
    
    // 初始化座位数据
    initSeatData();
    
    // STM32通信串口
    STM32_SERIAL.begin(STM32_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
    Serial.println("[INIT] STM32 Serial initialized");
    
    // HTTPS不验证证书 (简化处理)
    secureClient.setInsecure();
    
    // 连接WiFi
    connectWiFi();
}

void initSeatData() {
    seatData.occupied = false;
    seatData.duration = 0;
    seatData.lastUpdate = 0;
    seatData.dataValid = false;
    
    seatData.temperature = 0;
    seatData.humidity = 0;
    seatData.lightLevel = 0;
    seatData.pressure1 = 0;
    seatData.pressure2 = 0;
    seatData.distance = 0;
    seatData.pirState = 0;
    seatData.wifiSignal = 0;
}

// ============== 主循环 ==============
void loop() {
    processSTM32Data();
    
    // 定期上传数据
    if (millis() - lastUploadTime >= UPLOAD_INTERVAL) {
        lastUploadTime = millis();
        if (seatData.dataValid) {
            uploadDataToServer();
        }
    }
    
    // 心跳包
    if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
        lastHeartbeatTime = millis();
        sendHeartbeat();
    }
    
    // WiFi重连
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Reconnecting...");
        connectWiFi();
    }
    
    // 数据超时检测
    if (seatData.dataValid && (millis() - seatData.lastUpdate > 10000)) {
        seatData.dataValid = false;
        Serial.println("[WARNING] Data timeout");
    }
}

// ============== WiFi连接 ==============
void connectWiFi() {
    Serial.printf("[WIFI] Connecting to %s\n", ssid);
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
        seatData.wifiSignal = WiFi.RSSI();
        Serial.printf("[WIFI] Signal: %d dBm\n", seatData.wifiSignal);
    } else {
        Serial.println("\n[WIFI] Failed!");
    }
}

// ============== 上传数据到服务器 (HTTPS) ==============
void uploadDataToServer() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERROR] WiFi not connected");
        return;
    }
    
    // 更新WiFi信号
    seatData.wifiSignal = WiFi.RSSI();
    
    Serial.printf("[COZE] Connecting to %s:%d\n", serverHost, serverPort);
    
    if (!secureClient.connect(serverHost, serverPort)) {
        Serial.println("[COZE] Connection failed");
        return;
    }
    
    Serial.println("[COZE] Connected");
    
    // 构建JSON - 支持传感器数据
    StaticJsonDocument<512> doc;
    
    // 设备信息
    doc["deviceKey"] = deviceKey;
    doc["deviceName"] = deviceName;
    
    // 座位状态
    doc["isOccupied"] = seatData.occupied;
    
    // 传感器数据
    doc["temperature"] = seatData.temperature;
    doc["humidity"] = seatData.humidity;
    doc["lightLevel"] = seatData.lightLevel;
    doc["pressure1"] = seatData.pressure1;
    doc["pressure2"] = seatData.pressure2;
    doc["distance"] = seatData.distance;
    doc["pirState"] = seatData.pirState;
    doc["wifiSignal"] = seatData.wifiSignal;
    doc["duration"] = seatData.duration;
    
    String jsonData;
    serializeJson(doc, jsonData);
    
    // 构建HTTP POST请求
    String request = "POST /api/seats/status HTTP/1.1\r\n";
    request += "Host: " + String(serverHost) + "\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + String(jsonData.length()) + "\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
    request += jsonData;
    
    Serial.printf("[COZE] Sending: %s\n", jsonData.c_str());
    
    // 发送请求
    secureClient.print(request);
    
    // 等待响应
    unsigned long timeout = millis();
    while (secureClient.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println("[COZE] Timeout");
            secureClient.stop();
            return;
        }
    }
    
    // 读取响应
    String response = "";
    bool headerEnded = false;
    while (secureClient.available()) {
        String line = secureClient.readStringUntil('\n');
        if (!headerEnded) {
            if (line.startsWith("HTTP/1.1")) {
                Serial.printf("[COZE] Response: %s\n", line.c_str());
            }
            if (line == "\r") {
                headerEnded = true;
            }
        } else {
            response += line;
        }
    }
    
    if (response.length() > 0) {
        Serial.printf("[COZE] Status: %s\n", 
            seatData.occupied ? "OCCUPIED" : "VACANT");
        Serial.printf("[COZE] Data: T:%d°C H:%d%% L:%d P:%d/%d D:%dcm\n", 
            seatData.temperature, seatData.humidity, seatData.lightLevel,
            seatData.pressure1, seatData.pressure2, seatData.distance);
    }
    
    secureClient.stop();
}

// ============== 发送心跳包 ==============
void sendHeartbeat() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    if (!secureClient.connect(serverHost, serverPort)) {
        return;
    }
    
    StaticJsonDocument<128> doc;
    doc["deviceKey"] = deviceKey;
    doc["timestamp"] = millis();
    doc["wifiSignal"] = WiFi.RSSI();
    
    String jsonData;
    serializeJson(doc, jsonData);
    
    String request = "POST /api/devices/heartbeat HTTP/1.1\r\n";
    request += "Host: " + String(serverHost) + "\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + String(jsonData.length()) + "\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
    request += jsonData;
    
    secureClient.print(request);
    
    delay(100);
    
    while (secureClient.available()) {
        String line = secureClient.readStringUntil('\n');
        if (line.startsWith("HTTP/1.1 200")) {
            Serial.println("[HEARTBEAT] Sent");
            break;
        }
    }
    
    secureClient.stop();
}

// ============== 处理STM32数据 ==============
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
            
            // 数据包长度检查
            if (rxIndex >= 37 && rxBuffer[36] == 0x55) {
                parseDataPacket(rxBuffer, rxIndex);
                rxIndex = 0;
            }
        }
        else {
            rxIndex = 0;
        }
    }
}

// ============== 解析数据包 ==============
void parseDataPacket(uint8_t* buffer, uint8_t len) {
    if (len < 6) return;
    if (buffer[0] != 0xAA || buffer[36] != 0x55) return;
    
    uint8_t type = buffer[1];
    uint8_t length = buffer[2];
    uint8_t* data = &buffer[3];
    uint8_t checksum = buffer[35];
    
    // 校验和验证
    uint8_t calc_checksum = 0;
    for (int i = 0; i < 35; i++) {
        calc_checksum += buffer[i];
    }
    
    if (calc_checksum != checksum) {
        Serial.printf("[RX] Checksum fail\n");
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
                Serial.printf("[DATA] Sensor - T:%d°C, H:%d%%, L:%d\n", 
                    seatData.temperature, seatData.humidity, seatData.lightLevel);
            }
            break;
            
        case 0x02: // 座位状态
            if (length >= 4) {
                bool newOccupied = (data[2] == 1);
                if (newOccupied != seatData.occupied) {
                    Serial.printf("[DATA] State: %s -> %s\n",
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
            
        case 0x03: // 扩展传感器数据
            if (length >= 6) {
                seatData.pressure1 = data[0];
                seatData.pressure2 = data[1];
                seatData.distance = (data[2] << 8) | data[3];
                seatData.pirState = data[4];
                seatData.lastUpdate = millis();
                seatData.dataValid = true;
                Serial.printf("[DATA] Ext - P1:%d P2:%d Dist:%d PIR:%d\n", 
                    seatData.pressure1, seatData.pressure2, 
                    seatData.distance, seatData.pirState);
            }
            break;
            
        case 0xFF: // 完整数据包
            if (length >= 15) {
                seatData.temperature = data[0];
                seatData.humidity = data[1];
                seatData.lightLevel = (data[2] << 8) | data[3];
                seatData.pressure1 = data[4];
                seatData.pressure2 = data[5];
                seatData.distance = (data[6] << 8) | data[7];
                seatData.pirState = data[8];
                seatData.occupied = (data[9] == 1);
                seatData.duration = (data[10] << 24) | (data[11] << 16) | 
                                    (data[12] << 8) | data[13];
                seatData.lastUpdate = millis();
                seatData.dataValid = true;
                
                Serial.printf("[DATA] Full - T:%d H:%d L:%d P1:%d P2:%d D:%d PIR:%d\n", 
                    seatData.temperature, seatData.humidity, seatData.lightLevel,
                    seatData.pressure1, seatData.pressure2, seatData.distance,
                    seatData.pirState);
            }
            break;
    }
}

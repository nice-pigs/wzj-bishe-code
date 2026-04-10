/*
 * 座位检测系统 - ESP32扣子云平台上传
 * 适配STM32现有数据格式，上传到扣子平台
 * 
 * 域名：smkcc339ny.coze.site
 * API：POST /api/seats/status
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ============== WiFi配置 ==============
const char* ssid = "cs";                    // 修改为你的WiFi
const char* password = "88888888";          // 修改为你的WiFi密码

// ============== 扣子云平台配置 ==============
const char* serverHost = "rwzs4zf6nn.coze.site";
const int serverPort = 443;  // HTTPS 端口

// ============== 设备配置 ==============
const char* deviceKey = "esp32-seat-001";   // 设备唯一标识
const char* deviceName = "图书馆座位001";    // 设备名称

// ============== STM32串口配置 ==============
#define STM32_SERIAL Serial2
#define STM32_RX_PIN 16
#define STM32_TX_PIN 17
#define STM32_BAUD 115200

// ============== 座位状态数据 ==============
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
    int detectionScore;   // 检测置信度 (0-100)
} seatData;

// ============== 上传计时器 ==============
unsigned long lastUploadTime = 0;
unsigned long lastHeartbeatTime = 0;
const unsigned long UPLOAD_INTERVAL = 3000;      // 3秒上传一次（配合STM32的2秒发送）
const unsigned long HEARTBEAT_INTERVAL = 10000;  // 10秒心跳一次

WiFiClientSecure client;

void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n=== ESP32 Seat Detection - Coze Cloud (HTTPS) ===");
    Serial.println("Target: https://smkcc339ny.coze.site");
    
    // 初始化数据
    initSeatData();
    
    // STM32串口
    STM32_SERIAL.begin(STM32_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
    Serial.println("[INIT] STM32 Serial initialized");
    
    // 配置 HTTPS 客户端 - 不验证证书（用于测试）
    client.setInsecure();
    Serial.println("[INIT] HTTPS client configured (insecure mode)");
    
    // 连接WiFi
    connectWiFi();
    
    // 注册设备
    delay(2000);
    registerDevice();
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
    seatData.detectionScore = 0;
}

void loop() {
    processSTM32Data();
    
    // 定期上传
    if (millis() - lastUploadTime >= UPLOAD_INTERVAL) {
        lastUploadTime = millis();
        if (seatData.dataValid) {
            uploadToCozeCloud();
        }
    }
    
    // 心跳
    if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
        lastHeartbeatTime = millis();
        sendHeartbeat();
    }
    
    // WiFi重连
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Reconnecting...");
        connectWiFi();
    }
    
    // 数据超时检查（改为6秒，给STM32 2秒发送间隔留出余量）
    if (seatData.dataValid && (millis() - seatData.lastUpdate > 6000)) {
        seatData.dataValid = false;
        Serial.println("[WARNING] Data timeout - No data from STM32 for 6 seconds");
    }
}

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

void uploadToCozeCloud() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERROR] WiFi not connected");
        return;
    }
    
    // 更新WiFi信号
    seatData.wifiSignal = WiFi.RSSI();
    
    Serial.printf("[COZE] Connecting to %s:%d\n", serverHost, serverPort);
    
    if (!client.connect(serverHost, serverPort)) {
        Serial.println("[COZE] ✗ Connection failed");
        return;
    }
    
    Serial.println("[COZE] ✓ Connected");
    
    // 构建JSON - 扣子平台格式
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
    doc["detectionScore"] = seatData.detectionScore;
    
    String jsonData;
    serializeJson(doc, jsonData);
    
    // HTTP POST请求
    String request = "POST /api/seats/status HTTP/1.1\r\n";
    request += "Host: " + String(serverHost) + "\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + String(jsonData.length()) + "\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
    request += jsonData;
    
    Serial.printf("[COZE] Sending: %s\n", jsonData.c_str());
    
    client.print(request);
    
    // 等待响应
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println("[COZE] ✗ Timeout");
            client.stop();
            return;
        }
    }
    
    // 读取响应
    String response = "";
    String statusLine = "";
    bool headerEnded = false;
    int statusCode = 0;
    
    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (!headerEnded) {
            if (line.startsWith("HTTP/1.1")) {
                statusLine = line;
                // 提取状态码
                int spacePos = line.indexOf(' ');
                if (spacePos > 0) {
                    statusCode = line.substring(spacePos + 1, spacePos + 4).toInt();
                }
                Serial.printf("[COZE] Response: %s\n", line.c_str());
            }
            if (line == "\r") {
                headerEnded = true;
            }
        } else {
            response += line;
        }
    }
    
    // 显示响应体（如果有）
    if (response.length() > 0) {
        Serial.printf("[COZE] Body: %s\n", response.c_str());
    }
    
    // 根据状态码判断
    if (statusCode == 200 || statusCode == 201) {
        Serial.printf("[COZE] ✓ SUCCESS - %s\n", 
            seatData.occupied ? "OCCUPIED" : "VACANT");
        Serial.printf("[COZE] Data: T:%d°C H:%d%% L:%d P:%d/%d D:%dcm\n",
            seatData.temperature, seatData.humidity, seatData.lightLevel,
            seatData.pressure1, seatData.pressure2, seatData.distance);
    } else if (statusCode == 404) {
        Serial.println("[COZE] ✗ ERROR: API endpoint not found (404)");
        Serial.println("[COZE] Please check if the API is deployed on Coze platform");
    } else if (statusCode > 0) {
        Serial.printf("[COZE] ✗ ERROR: HTTP %d\n", statusCode);
    }
    
    client.stop();
}

void sendHeartbeat() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    if (!client.connect(serverHost, serverPort)) {
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

void registerDevice() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[REGISTER] WiFi not connected");
        return;
    }
    
    Serial.println("[REGISTER] Registering device...");
    
    if (!client.connect(serverHost, serverPort)) {
        Serial.println("[REGISTER] ✗ Connection failed");
        return;
    }
    
    StaticJsonDocument<128> doc;
    doc["deviceKey"] = deviceKey;
    doc["deviceName"] = deviceName;
    
    String jsonData;
    serializeJson(doc, jsonData);
    
    String request = "POST /api/devices/register HTTP/1.1\r\n";
    request += "Host: " + String(serverHost) + "\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + String(jsonData.length()) + "\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
    request += jsonData;
    
    Serial.printf("[REGISTER] Sending: %s\n", jsonData.c_str());
    client.print(request);
    
    // 等待响应
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println("[REGISTER] ✗ Timeout");
            client.stop();
            return;
        }
    }
    
    // 读取响应
    bool headerEnded = false;
    int statusCode = 0;
    String response = "";
    
    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (!headerEnded) {
            if (line.startsWith("HTTP/1.1")) {
                int spacePos = line.indexOf(' ');
                if (spacePos > 0) {
                    statusCode = line.substring(spacePos + 1, spacePos + 4).toInt();
                }
                Serial.printf("[REGISTER] Response: %s\n", line.c_str());
            }
            if (line == "\r") {
                headerEnded = true;
            }
        } else {
            response += line;
        }
    }
    
    if (statusCode == 200) {
        Serial.println("[REGISTER] ✓ Device registered successfully");
    } else if (statusCode == 404) {
        Serial.println("[REGISTER] ✗ API not found - Please deploy the backend first");
    } else if (statusCode > 0) {
        Serial.printf("[REGISTER] ✗ HTTP %d\n", statusCode);
    }
    
    client.stop();
}

void processSTM32Data() {
    static uint8_t rxBuffer[64];
    static uint8_t rxIndex = 0;
    static unsigned long lastByteTime = 0;
    static int totalBytesReceived = 0;
    
    while (STM32_SERIAL.available()) {
        uint8_t byte = STM32_SERIAL.read();
        totalBytesReceived++;
        lastByteTime = millis();
        
        // 调试：打印接收到的字节
        if (totalBytesReceived <= 10) {
            Serial.printf("[RX] Byte %d: 0x%02X\n", totalBytesReceived, byte);
        }
        
        if (rxIndex == 0 && byte == 0xAA) {
            rxBuffer[rxIndex++] = byte;
            Serial.println("[RX] Found packet header 0xAA");
        }
        else if (rxIndex > 0 && rxIndex < sizeof(rxBuffer)) {
            rxBuffer[rxIndex++] = byte;
            
            if (rxIndex >= 37 && rxBuffer[36] == 0x55) {
                Serial.printf("[RX] Complete packet received (%d bytes)\n", rxIndex);
                parseDataPacket(rxBuffer, rxIndex);
                rxIndex = 0;
            }
        }
        else {
            Serial.printf("[RX] Buffer overflow or invalid data, reset (rxIndex=%d)\n", rxIndex);
            rxIndex = 0;
        }
    }
    
    // 定期报告串口状态
    static unsigned long lastStatusReport = 0;
    if (millis() - lastStatusReport > 5000) {
        lastStatusReport = millis();
        Serial.printf("[STATUS] Total bytes received: %d, Last byte: %lu ms ago\n", 
            totalBytesReceived, 
            lastByteTime > 0 ? (millis() - lastByteTime) : 0);
    }
}

void parseDataPacket(uint8_t* buffer, uint8_t len) {
    if (len < 6) return;
    if (buffer[0] != 0xAA || buffer[36] != 0x55) return;
    
    uint8_t type = buffer[1];
    uint8_t length = buffer[2];
    uint8_t* data = &buffer[3];
    uint8_t checksum = buffer[35];
    
    // 校验和
    uint8_t calc_checksum = 0;
    for (int i = 0; i < 35; i++) {
        calc_checksum += buffer[i];
    }
    
    if (calc_checksum != checksum) {
        Serial.printf("[RX] ✗ Checksum fail\n");
        return;
    }
    
    switch (type) {
        case 0x01: // 传感器数据 (温湿度+光照)
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
            
        case 0x02: // 设备状态 (占用状态)
            if (length >= 4) {
                // 新的DeviceState_t结构：green_led, red_led, buzzer_state, seat_occupied
                bool newOccupied = (data[3] == 1);  // seat_occupied是第4个字节
                if (newOccupied != seatData.occupied) {
                    Serial.printf("[DATA] ✓ State: %s -> %s\n",
                        seatData.occupied ? "OCCUPIED" : "VACANT",
                        newOccupied ? "OCCUPIED" : "VACANT");
                    seatData.occupied = newOccupied;
                    // 状态变化立即上传
                    uploadToCozeCloud();
                } else {
                    seatData.occupied = newOccupied;
                }
                seatData.lastUpdate = millis();
                seatData.dataValid = true;
            }
            break;
            
        case 0x03: // 扩展传感器 (压力+超声波+PIR)
            if (length >= 6) {
                seatData.pressure1 = data[0];
                seatData.pressure2 = data[1];
                seatData.distance = (data[2] << 8) | data[3];
                seatData.pirState = data[4];
                
                // 计算检测置信度
                int score = 0;
                if (seatData.pressure1 || seatData.pressure2) score += 70;
                if (seatData.distance > 0 && seatData.distance < 50) score += 25;
                if (seatData.pirState) score += 5;
                seatData.detectionScore = score;
                
                seatData.lastUpdate = millis();
                seatData.dataValid = true;
                Serial.printf("[DATA] ✓ Extended - P1:%d P2:%d Dist:%dcm PIR:%d Score:%d\n", 
                    seatData.pressure1, seatData.pressure2, 
                    seatData.distance, seatData.pirState, seatData.detectionScore);
            }
            break;
            
        case 0x04: // 完整座位检测数据 (SeatDetectionData_t)
            if (length >= 16) {
                // 解析完整数据结构
                // seat_occupied(1) + pressure1(1) + pressure2(1) + distance(2) + pir_state(1) 
                // + temperature(1) + humidity(1) + light_percent(1) + detection_score(1) + duration(4)
                seatData.occupied = (data[0] == 1);
                seatData.pressure1 = data[1];
                seatData.pressure2 = data[2];
                seatData.distance = (data[3] << 8) | data[4];
                seatData.pirState = data[5];
                seatData.temperature = data[6];
                seatData.humidity = data[7];
                seatData.lightLevel = data[8] * 655;  // 百分比转换为0-65535
                seatData.detectionScore = data[9];
                seatData.duration = (data[10] << 24) | (data[11] << 16) | (data[12] << 8) | data[13];
                
                seatData.lastUpdate = millis();
                seatData.dataValid = true;
                
                Serial.printf("[DATA] ✓ Complete - State:%s P:%d/%d D:%dcm PIR:%d T:%d°C H:%d%% Score:%d\n", 
                    seatData.occupied ? "OCCUPIED" : "VACANT",
                    seatData.pressure1, seatData.pressure2, 
                    seatData.distance, seatData.pirState,
                    seatData.temperature, seatData.humidity,
                    seatData.detectionScore);
            }
            break;
    }
}

/**
 * ESP32 座位状态上报测试代码
 * 
 * 功能：
 * 1. 连接 Wi-Fi
 * 2. 注册设备
 * 3. 上报座位状态
 * 
 * 硬件：ESP32 开发板
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ========== 配置区域 ==========

// Wi-Fi 配置
const char* ssid = "你的WiFi名称";
const char* password = "你的WiFi密码";

// 服务器配置 - 使用 Coze 域名
const char* serverUrl = "https://smkcc339ny.coze.site";

// 设备配置
const char* deviceKey = "ESP32_SEAT_001";
const char* deviceName = "ESP32座位监测终端";

// 传感器引脚（根据实际连接修改）
const int sensorPin = 34;  // 红外传感器或压力传感器

// ========== 全局变量 ==========

bool lastOccupiedState = false;
unsigned long lastReportTime = 0;
const unsigned long reportInterval = 5000;  // 5秒上报一次

// ========== 初始化 ==========

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=================================");
  Serial.println("ESP32 座位监测系统启动中...");
  Serial.println("=================================\n");
  
  // 初始化传感器引脚
  pinMode(sensorPin, INPUT);
  
  // 连接 Wi-Fi
  connectWiFi();
  
  // 注册设备
  registerDevice();
  
  Serial.println("\n系统初始化完成！开始监测...\n");
}

// ========== 主循环 ==========

void loop() {
  // 检查 Wi-Fi 连接
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi 断开，重新连接...");
    connectWiFi();
    return;
  }
  
  // 读取传感器状态
  bool isOccupied = readSensorStatus();
  
  // 状态变化或定时上报
  unsigned long currentTime = millis();
  bool stateChanged = (isOccupied != lastOccupiedState);
  bool timeToReport = (currentTime - lastReportTime >= reportInterval);
  
  if (stateChanged || timeToReport) {
    // 上报状态
    if (reportStatus(isOccupied)) {
      lastOccupiedState = isOccupied;
      lastReportTime = currentTime;
    }
  }
  
  delay(100);  // 短暂延时
}

// ========== 功能函数 ==========

// 连接 Wi-Fi
void connectWiFi() {
  Serial.print("正在连接 Wi-Fi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ Wi-Fi 连接成功！");
    Serial.print("  IP 地址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ Wi-Fi 连接失败，请检查配置！");
  }
}

// 读取传感器状态
bool readSensorStatus() {
  int sensorValue = analogRead(sensorPin);
  
  // 根据传感器类型调整阈值
  // 红外传感器 HC-SR501: 检测到人时输出高电平
  // 压力传感器 FSR402: 受力时阻值变化，电压变化
  
  // 示例：假设传感器值 > 2000 表示有人
  bool isOccupied = (sensorValue > 2000);
  
  Serial.print("传感器值: ");
  Serial.print(sensorValue);
  Serial.print(" -> 状态: ");
  Serial.println(isOccupied ? "有人" : "无人");
  
  return isOccupied;
}

// 注册设备
bool registerDevice() {
  Serial.println("\n正在注册设备...");
  
  // 创建 JSON 请求体
  StaticJsonDocument<256> doc;
  doc["deviceKey"] = deviceKey;
  doc["deviceName"] = deviceName;
  
  String requestBody;
  serializeJson(doc, requestBody);
  
  // 发送 HTTPS 请求
  String url = String(serverUrl) + "/api/devices/register";
  String response = httpsPost(url, requestBody);
  
  // 解析响应
  if (response.length() > 0) {
    StaticJsonDocument<512> responseDoc;
    deserializeJson(responseDoc, response);
    
    if (responseDoc["success"] == true) {
      Serial.println("✓ 设备注册成功！");
      Serial.print("  设备ID: ");
      Serial.println(responseDoc["data"]["deviceId"].as<int>());
      return true;
    } else {
      Serial.print("✗ 设备注册失败: ");
      Serial.println(responseDoc["error"].as<String>());
      return false;
    }
  }
  
  return false;
}

// 上报座位状态
bool reportStatus(bool isOccupied) {
  Serial.println("\n正在上报座位状态...");
  
  // 创建 JSON 请求体
  StaticJsonDocument<256> doc;
  doc["deviceKey"] = deviceKey;
  doc["isOccupied"] = isOccupied;
  
  String requestBody;
  serializeJson(doc, requestBody);
  
  // 发送 HTTPS 请求
  String url = String(serverUrl) + "/api/seats/status";
  String response = httpsPost(url, requestBody);
  
  // 解析响应
  if (response.length() > 0) {
    StaticJsonDocument<512> responseDoc;
    deserializeJson(responseDoc, response);
    
    if (responseDoc["success"] == true) {
      Serial.println("✓ 状态上报成功！");
      Serial.print("  座位ID: ");
      Serial.print(responseDoc["data"]["seatId"].as<int>());
      Serial.print(", 座位名称: ");
      Serial.print(responseDoc["data"]["seatName"].as<String>());
      Serial.print(", 状态: ");
      Serial.println(responseDoc["data"]["isOccupied"].as<bool>() ? "有人" : "无人");
      return true;
    } else {
      Serial.print("✗ 状态上报失败: ");
      Serial.println(responseDoc["error"].as<String>());
      return false;
    }
  }
  
  return false;
}

// 发送 HTTPS POST 请求
String httpsPost(String url, String payload) {
  WiFiClientSecure client;
  HTTPClient https;
  
  // ⚠️ 关键：忽略 SSL 证书验证
  // 这是解决 HTTPS 通信问题的关键！
  client.setInsecure();
  
  String response = "";
  
  Serial.print("请求URL: ");
  Serial.println(url);
  Serial.print("请求体: ");
  Serial.println(payload);
  
  https.begin(client, url);
  https.addHeader("Content-Type", "application/json");
  
  int httpCode = https.POST(payload);
  
  if (httpCode > 0) {
    Serial.print("HTTP 状态码: ");
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
      response = https.getString();
      Serial.print("响应: ");
      Serial.println(response);
    }
  } else {
    Serial.print("✗ HTTPS 请求失败: ");
    Serial.println(https.errorToString(httpCode));
  }
  
  https.end();
  return response;
}

/**
 * 座位状态实时监测系统 - ESP32-S3 版本
 * 
 * 硬件配置：
 * - 主控：ESP32-S3 N6R8
 * - 传感器：HC-SR501 红外人体感应模块
 * - 显示：0.96寸 OLED (SSD1306 I2C)
 * 
 * 功能：
 * 1. 检测座位是否有人
 * 2. 状态变化时自动上报云端
 * 3. OLED 实时显示状态
 * 4. LED 指示灯
 * 
 * 作者：五子鸡大坏蛋
 * 日期：2024
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

// ==================== 配置区域（请修改这里）====================

// WiFi 配置
const char* WIFI_SSID = "你的WiFi名称";        // 修改为你的WiFi名称
const char* WIFI_PASSWORD = "你的WiFi密码";    // 修改为你的WiFi密码

// 云端服务器配置
const char* SERVER_URL = "https://smkcc339ny.coze.site";

// 设备配置（每个设备需要唯一ID）
const char* DEVICE_KEY = "ESP32_S3_001";       // 设备唯一标识
const char* DEVICE_NAME = "座位监测器-1号";     // 设备名称

// 引脚配置
#define PIR_PIN        7      // HC-SR501 信号引脚 (GPIO7)
#define LED_RED_PIN    8      // 红色LED (有人)
#define LED_GREEN_PIN  9      // 绿色LED (无人)
#define SDA_PIN        6      // OLED I2C SDA (ESP32-S3 默认)
#define SCL_PIN        5      // OLED I2C SCL (ESP32-S3 默认)

// 检测参数
#define DETECTION_INTERVAL  500   // 传感器检测间隔 (ms)
#define OCCUPIED_TIMEOUT    30000 // 无人确认超时时间 (ms) - 超过这个时间没检测到人则认为无人
#define REPORT_INTERVAL     5000  // 定时上报间隔 (ms)

// ==================== 全局变量 ====================

// OLED 显示屏
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// 状态变量
bool currentOccupied = false;      // 当前状态
bool lastReportedOccupied = false; // 上次上报的状态
unsigned long lastDetectionTime = 0;
unsigned long lastReportTime = 0;
unsigned long lastPirTriggerTime = 0;

// WiFi 连接状态
bool wifiConnected = false;

// ==================== 初始化 ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("座位状态监测系统 - ESP32-S3");
  Serial.println("========================================");
  
  // 初始化引脚
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  
  // 初始化 LED（全灭）
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  
  // 初始化 OLED
  initOLED();
  
  // 显示启动画面
  showStartupScreen();
  
  // 连接 WiFi
  connectWiFi();
  
  // 注册设备
  if (wifiConnected) {
    registerDevice();
  }
  
  Serial.println("\n系统初始化完成！开始监测...\n");
  displayStatus("系统就绪", "等待检测...");
}

// ==================== 主循环 ====================

void loop() {
  unsigned long currentTime = millis();
  
  // 检查 WiFi 连接
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    displayStatus("WiFi断开", "重连中...");
    connectWiFi();
    return;
  }
  wifiConnected = true;
  
  // 读取传感器状态
  bool pirDetected = readPIRSensor();
  
  // 更新座位状态逻辑
  updateOccupancyStatus(pirDetected, currentTime);
  
  // 状态变化时上报
  if (currentOccupied != lastReportedOccupied) {
    if (reportStatus(currentOccupied)) {
      lastReportedOccupied = currentOccupied;
      Serial.println("✓ 状态变化已上报");
    }
    lastReportTime = currentTime;
  }
  
  // 定时心跳上报
  if (currentTime - lastReportTime >= REPORT_INTERVAL) {
    reportStatus(currentOccupied);
    lastReportTime = currentTime;
  }
  
  // 更新 LED 指示灯
  updateLEDs();
  
  // 更新 OLED 显示
  updateDisplay();
  
  delay(DETECTION_INTERVAL);
}

// ==================== 功能函数 ====================

/**
 * 初始化 OLED 显示屏
 */
void initOLED() {
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED 初始化失败！");
    return;
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("OLED OK");
  display.display();
  
  Serial.println("✓ OLED 初始化成功");
}

/**
 * 显示启动画面
 */
void showStartupScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println("座位监测系统");
  display.println("----------------");
  display.print("设备: ");
  display.println(DEVICE_KEY);
  display.println("");
  display.println("正在启动...");
  
  display.display();
}

/**
 * 连接 WiFi
 */
void connectWiFi() {
  Serial.print("正在连接 WiFi: ");
  Serial.println(WIFI_SSID);
  
  displayStatus("连接WiFi", WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n✓ WiFi 连接成功！");
    Serial.print("  IP 地址: ");
    Serial.println(WiFi.localIP());
    
    displayStatus("WiFi已连接", WiFi.localIP().toString());
    delay(1000);
  } else {
    wifiConnected = false;
    Serial.println("\n❌ WiFi 连接失败！");
    displayStatus("WiFi失败", "请检查配置");
  }
}

/**
 * 读取 HC-SR501 红外传感器
 */
bool readPIRSensor() {
  int pirValue = digitalRead(PIR_PIN);
  return pirValue == HIGH;
}

/**
 * 更新座位占用状态
 * @param pirDetected PIR 传感器是否检测到人
 * @param currentTime 当前时间
 */
void updateOccupancyStatus(bool pirDetected, unsigned long currentTime) {
  if (pirDetected) {
    // 检测到人，更新最后触发时间
    lastPirTriggerTime = currentTime;
    
    if (!currentOccupied) {
      currentOccupied = true;
      Serial.println("📢 检测到人！状态: 有人");
    }
  } else {
    // 没检测到人，检查超时
    if (currentOccupied && (currentTime - lastPirTriggerTime >= OCCUPIED_TIMEOUT)) {
      currentOccupied = false;
      Serial.println("📢 超时无人。状态: 无人");
    }
  }
}

/**
 * 更新 LED 指示灯
 */
void updateLEDs() {
  if (currentOccupied) {
    // 有人：红灯亮
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, LOW);
  } else {
    // 无人：绿灯亮
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, HIGH);
  }
}

/**
 * 更新 OLED 显示
 */
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // 标题
  display.setCursor(0, 0);
  display.println("=== 座位状态 ===");
  
  // 状态
  display.setCursor(0, 16);
  display.print("状态: ");
  display.setTextSize(2);
  if (currentOccupied) {
    display.println("有人");
  } else {
    display.println("无人");
  }
  
  // WiFi 状态
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("WiFi: ");
  display.println(wifiConnected ? "已连接" : "断开");
  
  // 设备ID
  display.setCursor(0, 52);
  display.print("ID: ");
  display.println(DEVICE_KEY);
  
  display.display();
}

/**
 * 显示两行状态信息
 */
void displayStatus(String line1, String line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 24);
  display.println(line1);
  display.setCursor(0, 36);
  display.println(line2);
  
  display.display();
}

/**
 * 注册设备到云端
 */
bool registerDevice() {
  Serial.println("\n正在注册设备...");
  
  String url = String(SERVER_URL) + "/api/devices/register";
  String body = "{\"deviceKey\":\"" + String(DEVICE_KEY) + 
                "\",\"deviceName\":\"" + String(DEVICE_NAME) + "\"}";
  
  String response = httpsPost(url, body);
  
  if (response.length() > 0) {
    JsonDocument doc;
    deserializeJson(doc, response);
    
    if (doc["success"] == true) {
      Serial.println("✓ 设备注册成功！");
      Serial.print("  设备ID: ");
      Serial.println(doc["data"]["deviceId"].as<int>());
      return true;
    } else {
      Serial.print("❌ 设备注册失败: ");
      Serial.println(doc["error"].as<String>());
      return false;
    }
  }
  
  return false;
}

/**
 * 上报座位状态到云端
 */
bool reportStatus(bool isOccupied) {
  String url = String(SERVER_URL) + "/api/seats/status";
  String body = "{\"deviceKey\":\"" + String(DEVICE_KEY) + 
                "\",\"isOccupied\":" + (isOccupied ? "true" : "false") + "}";
  
  String response = httpsPost(url, body);
  
  if (response.length() > 0) {
    JsonDocument doc;
    deserializeJson(doc, response);
    
    if (doc["success"] == true) {
      Serial.print("✓ 状态上报成功: ");
      Serial.print(doc["data"]["seatName"].as<String>());
      Serial.print(" -> ");
      Serial.println(doc["data"]["isOccupied"].as<bool>() ? "有人" : "无人");
      return true;
    } else {
      Serial.print("❌ 状态上报失败: ");
      Serial.println(doc["error"].as<String>());
      return false;
    }
  }
  
  return false;
}

/**
 * 发送 HTTPS POST 请求
 */
String httpsPost(String url, String payload) {
  WiFiClientSecure client;
  HTTPClient https;
  
  // 关键：忽略 SSL 证书验证（ESP32 访问 HTTPS 必需）
  client.setInsecure();
  
  String response = "";
  
  Serial.print("请求: ");
  Serial.println(url);
  
  if (https.begin(client, url)) {
    https.addHeader("Content-Type", "application/json");
    
    int httpCode = https.POST(payload);
    
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        response = https.getString();
      } else {
        Serial.print("HTTP 错误: ");
        Serial.println(httpCode);
      }
    } else {
      Serial.print("请求失败: ");
      Serial.println(https.errorToString(httpCode));
    }
    
    https.end();
  } else {
    Serial.println("无法连接服务器");
  }
  
  return response;
}

/**
 * 座位状态实时监测系统 - ESP32-S3 版本（带压力传感器扩展）
 * 
 * 硬件配置：
 * - 主控：ESP32-S3 N6R8
 * - 传感器：HC-SR501 红外 + FSR402 压力传感器（可选）
 * - 显示：0.96寸 OLED (SSD1306 I2C)
 * 
 * 功能：
 * 1. 双传感器检测（红外+压力）提高准确性
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

// ==================== 引脚配置 ====================

// HC-SR501 红外传感器
#define PIR_PIN        7      // HC-SR501 信号引脚 (GPIO7)

// 压力传感器（可选，接 ADC 引脚）
#define PRESSURE_PIN   4      // FSR402 压力传感器 (GPIO4, ADC1_CH3)
#define PRESSURE_ENABLED false // 是否启用压力传感器

// LED 指示灯
#define LED_RED_PIN    8      // 红色LED (有人)
#define LED_GREEN_PIN  9      // 绿色LED (无人)

// OLED I2C
#define SDA_PIN        6      // OLED I2C SDA
#define SCL_PIN        5      // OLED I2C SCL

// ==================== 检测参数 ====================

#define DETECTION_INTERVAL  500    // 传感器检测间隔 (ms)
#define OCCUPIED_TIMEOUT    30000  // 无人确认超时时间 (ms)
#define REPORT_INTERVAL     5000   // 定时上报间隔 (ms)

// 压力传感器阈值（需要根据实际调整）
#define PRESSURE_THRESHOLD  1000   // ADC 值超过此值认为有人坐

// ==================== 全局变量 ====================

Adafruit_SSD1306 display(128, 64, &Wire, -1);

bool currentOccupied = false;
bool lastReportedOccupied = false;
unsigned long lastDetectionTime = 0;
unsigned long lastReportTime = 0;
unsigned long lastPirTriggerTime = 0;

// 传感器数值（用于调试）
int pressureValue = 0;
bool pirDetected = false;

bool wifiConnected = false;

// ==================== 初始化 ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("座位状态监测系统 - ESP32-S3");
  Serial.printf("压力传感器: %s\n", PRESSURE_ENABLED ? "启用" : "禁用");
  Serial.println("========================================");
  
  // 初始化引脚
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  
  if (PRESSURE_ENABLED) {
    pinMode(PRESSURE_PIN, INPUT);
  }
  
  // 初始化 LED（全灭）
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  
  // 初始化 OLED
  initOLED();
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
  
  // 读取传感器
  pirDetected = readPIRSensor();
  
  if (PRESSURE_ENABLED) {
    pressureValue = readPressureSensor();
  }
  
  // 更新状态逻辑
  updateOccupancyStatus(currentTime);
  
  // 状态变化时上报
  if (currentOccupied != lastReportedOccupied) {
    if (reportStatus(currentOccupied)) {
      lastReportedOccupied = currentOccupied;
    }
    lastReportTime = currentTime;
  }
  
  // 定时心跳上报
  if (currentTime - lastReportTime >= REPORT_INTERVAL) {
    reportStatus(currentOccupied);
    lastReportTime = currentTime;
  }
  
  // 更新显示和 LED
  updateLEDs();
  updateDisplay();
  
  delay(DETECTION_INTERVAL);
}

// ==================== 传感器函数 ====================

/**
 * 读取 HC-SR501 红外传感器
 */
bool readPIRSensor() {
  return digitalRead(PIR_PIN) == HIGH;
}

/**
 * 读取压力传感器（模拟值）
 */
int readPressureSensor() {
  return analogRead(PRESSURE_PIN);
}

/**
 * 判断压力传感器是否检测到人
 */
bool isPressureDetected() {
  if (!PRESSURE_ENABLED) return false;
  return pressureValue > PRESSURE_THRESHOLD;
}

/**
 * 更新座位占用状态
 */
void updateOccupancyStatus(unsigned long currentTime) {
  
  if (PRESSURE_ENABLED) {
    // 双传感器模式：压力 + 红外
    bool pressureOK = isPressureDetected();
    bool pirOK = pirDetected;
    
    // 策略：压力检测到 OR 红外检测到 = 有人
    if (pressureOK || pirOK) {
      lastPirTriggerTime = currentTime;
      if (!currentOccupied) {
        currentOccupied = true;
        Serial.printf("📢 双传感器检测: 压力=%d(%s) 红外=%s → 有人\n", 
          pressureValue, pressureOK?"✓":"✗", pirOK?"✓":"✗");
      }
    } else {
      if (currentOccupied && (currentTime - lastPirTriggerTime >= OCCUPIED_TIMEOUT)) {
        currentOccupied = false;
        Serial.println("📢 双传感器超时 → 无人");
      }
    }
    
  } else {
    // 单传感器模式：仅红外
    if (pirDetected) {
      lastPirTriggerTime = currentTime;
      if (!currentOccupied) {
        currentOccupied = true;
        Serial.println("📢 红外检测到人 → 有人");
      }
    } else {
      if (currentOccupied && (currentTime - lastPirTriggerTime >= OCCUPIED_TIMEOUT)) {
        currentOccupied = false;
        Serial.println("📢 超时无人 → 无人");
      }
    }
  }
}

// ==================== 显示和 LED ====================

void initOLED() {
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED 初始化失败！");
    return;
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
  
  Serial.println("✓ OLED 初始化成功");
}

void showStartupScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println("座位监测系统 v1.0");
  display.println("----------------");
  display.print("设备: ");
  display.println(DEVICE_KEY);
  display.print("传感器: ");
  display.println(PRESSURE_ENABLED ? "红外+压力" : "红外");
  display.println("");
  display.println("正在启动...");
  
  display.display();
}

void updateLEDs() {
  if (currentOccupied) {
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, LOW);
  } else {
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, HIGH);
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // 标题
  display.setCursor(0, 0);
  display.print("状态:");
  display.setTextSize(2);
  display.println(currentOccupied ? " 有人" : " 无人");
  
  // 传感器数据
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print("PIR: ");
  display.println(pirDetected ? "检测到" : "无");
  
  if (PRESSURE_ENABLED) {
    display.setCursor(0, 36);
    display.print("压力: ");
    display.print(pressureValue);
    display.println(isPressureDetected() ? " ✓" : " ✗");
  }
  
  // WiFi 状态
  display.setCursor(0, 52);
  display.print("WiFi:");
  display.println(wifiConnected ? "✓" : "✗");
  display.print(" ID:");
  display.println(DEVICE_KEY);
  
  display.display();
}

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

// ==================== WiFi 和网络 ====================

void connectWiFi() {
  Serial.printf("正在连接 WiFi: %s\n", WIFI_SSID);
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
    Serial.printf("\n✓ WiFi 连接成功！IP: %s\n", WiFi.localIP().toString().c_str());
    displayStatus("WiFi已连接", WiFi.localIP().toString());
    delay(1000);
  } else {
    wifiConnected = false;
    Serial.println("\n❌ WiFi 连接失败！");
    displayStatus("WiFi失败", "请检查配置");
  }
}

bool registerDevice() {
  Serial.println("正在注册设备...");
  
  String url = String(SERVER_URL) + "/api/devices/register";
  String body = "{\"deviceKey\":\"" + String(DEVICE_KEY) + 
                "\",\"deviceName\":\"" + String(DEVICE_NAME) + "\"}";
  
  String response = httpsPost(url, body);
  
  if (response.length() > 0) {
    JsonDocument doc;
    deserializeJson(doc, response);
    
    if (doc["success"] == true) {
      Serial.printf("✓ 设备注册成功！ID: %d\n", doc["data"]["deviceId"].as<int>());
      return true;
    } else {
      Serial.printf("❌ 设备注册失败: %s\n", doc["error"].as<String>().c_str());
    }
  }
  return false;
}

bool reportStatus(bool isOccupied) {
  String url = String(SERVER_URL) + "/api/seats/status";
  String body = "{\"deviceKey\":\"" + String(DEVICE_KEY) + 
                "\",\"isOccupied\":" + (isOccupied ? "true" : "false") + "}";
  
  String response = httpsPost(url, body);
  
  if (response.length() > 0) {
    JsonDocument doc;
    deserializeJson(doc, response);
    
    if (doc["success"] == true) {
      Serial.printf("✓ 上报成功: %s -> %s\n", 
        doc["data"]["seatName"].as<String>().c_str(),
        doc["data"]["isOccupied"].as<bool>() ? "有人" : "无人");
      return true;
    } else {
      Serial.printf("❌ 上报失败: %s\n", doc["error"].as<String>().c_str());
    }
  }
  return false;
}

String httpsPost(String url, String payload) {
  WiFiClientSecure client;
  HTTPClient https;
  
  client.setInsecure();  // 忽略证书验证
  
  String response = "";
  
  if (https.begin(client, url)) {
    https.addHeader("Content-Type", "application/json");
    
    int httpCode = https.POST(payload);
    
    if (httpCode == HTTP_CODE_OK) {
      response = https.getString();
    }
    
    https.end();
  }
  
  return response;
}

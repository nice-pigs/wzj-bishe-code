/**
 * ESP32 最简调用示例
 * 只需要修改 WiFi 配置即可运行
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ===== 修改这里 =====
const char* ssid = "你的WiFi名称";
const char* password = "你的WiFi密码";
const char* deviceKey = "ESP32_001";  // 设备唯一ID
// ===================

const char* serverUrl = "https://smkcc339ny.coze.site";

void setup() {
  Serial.begin(115200);
  
  // 连接WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi已连接");
  
  // 注册设备
  registerDevice();
  
  // 上报状态（测试：有人）
  reportStatus(true);
  
  // 上报状态（测试：无人）
  delay(2000);
  reportStatus(false);
}

void loop() {}

// 注册设备
void registerDevice() {
  String url = String(serverUrl) + "/api/devices/register";
  String body = "{\"deviceKey\":\"" + String(deviceKey) + "\",\"deviceName\":\"ESP32座位监测器\"}";
  
  WiFiClientSecure client;
  client.setInsecure();  // 忽略证书验证
  
  HTTPClient https;
  https.begin(client, url);
  https.addHeader("Content-Type", "application/json");
  
  int code = https.POST(body);
  Serial.println("注册结果: " + https.getString());
  https.end();
}

// 上报座位状态
void reportStatus(bool isOccupied) {
  String url = String(serverUrl) + "/api/seats/status";
  String body = "{\"deviceKey\":\"" + String(deviceKey) + "\",\"isOccupied\":" + (isOccupied ? "true" : "false") + "}";
  
  WiFiClientSecure client;
  client.setInsecure();
  
  HTTPClient https;
  https.begin(client, url);
  https.addHeader("Content-Type", "application/json");
  
  int code = https.POST(body);
  Serial.println("状态上报: " + https.getString());
  https.end();
}

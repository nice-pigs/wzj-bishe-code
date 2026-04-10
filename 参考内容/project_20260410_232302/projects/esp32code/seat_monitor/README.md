# ESP32-S3 座位监测系统

基于 ESP32-S3 的物联网座位状态实时监测终端。

## 功能特性

- ✅ 红外人体检测（HC-SR501）
- ✅ OLED 实时状态显示
- ✅ LED 指示灯（红/绿）
- ✅ WiFi 自动连接
- ✅ HTTPS 云端通信
- ✅ 状态变化自动上报
- ✅ 定时心跳上报

## 快速开始

### 1. 安装 Arduino IDE

下载地址：https://www.arduino.cc/en/software

### 2. 安装 ESP32 开发板支持

1. 打开 Arduino IDE
2. 菜单：**文件** → **首选项**
3. 在"附加开发板管理器网址"中添加：
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
4. 菜单：**工具** → **开发板** → **开发板管理器**
5. 搜索 "ESP32"，安装 **ESP32 by Espressif Systems**

### 3. 安装依赖库

菜单：**工具** → **管理库**，搜索并安装：

| 库名 | 版本 |
|------|------|
| Adafruit SSD1306 | 2.5.7 或更高 |
| Adafruit GFX Library | 1.11.5 或更高 |
| ArduinoJson | 7.0.4 或更高 |

### 4. 配置开发板

菜单：**工具**，设置以下选项：

```
开发板：ESP32S3 Dev Module
USB CDC On Boot：Enabled
CPU Frequency：240MHz
Flash Mode：QIO 80MHz
Flash Size：8MB
Partition Scheme：Default
PSRAM：OPI PSAM
Upload Mode：UART0 / Hardware CDC
USB Mode：USB-OTG (TinyUSB) 或 Hardware CDC
```

### 5. 修改配置

打开 `seat_monitor.ino`，修改以下配置：

```cpp
// WiFi 配置
const char* WIFI_SSID = "你的WiFi名称";
const char* WIFI_PASSWORD = "你的WiFi密码";

// 设备配置（每个设备需要唯一ID）
const char* DEVICE_KEY = "ESP32_S3_001";
const char* DEVICE_NAME = "座位监测器-1号";
```

### 6. 上传代码

1. 用 USB-C 数据线连接 ESP32-S3 到电脑
2. 选择端口：**工具** → **端口** → 选择 ESP32 端口
3. 点击 **上传** 按钮
4. 打开串口监视器查看输出（波特率 115200）

## 文件说明

```
seat_monitor/
├── seat_monitor.ino    # 主程序代码
└── 接线指南.md          # 硬件接线说明
```

## 接线说明

详细接线请参考 [接线指南.md](./接线指南.md)

快速参考：

| 组件 | ESP32 引脚 |
|------|-----------|
| HC-SR501 OUT | GPIO7 |
| LED 红色 | GPIO8 |
| LED 绿色 | GPIO9 |
| OLED SDA | GPIO6 |
| OLED SCL | GPIO5 |

## 云端 API

本设备连接的云端服务地址：
- Web 界面：https://smkcc339ny.coze.site
- API 地址：https://smkcc339ny.coze.site/api

### API 接口

**设备注册：**
```
POST /api/devices/register
Body: {"deviceKey":"ESP32_S3_001","deviceName":"座位监测器"}
```

**状态上报：**
```
POST /api/seats/status
Body: {"deviceKey":"ESP32_S3_001","isOccupied":true}
```

## 检测逻辑

1. HC-SR501 检测到人体移动 → 认为座位有人
2. 持续检测，如果超过 30 秒未检测到人 → 认为座位无人
3. 状态变化时立即上报云端
4. 每 5 秒心跳上报一次当前状态

## 参数调整

可在代码中修改以下参数：

```cpp
#define DETECTION_INTERVAL  500    // 检测间隔 500ms
#define OCCUPIED_TIMEOUT    30000  // 无人确认超时 30秒
#define REPORT_INTERVAL     5000   // 心跳上报间隔 5秒
```

## 毕设要求符合性

| 要求 | 实现 |
|------|------|
| 状态变化10秒内上报 | ✅ 状态变化立即上报 |
| 网页自动刷新≤10秒 | ✅ 前端 5 秒自动刷新 |
| 物理感知到网络显示闭环 | ✅ 传感器 → ESP32 → 云端 → 网页 |

## 故障排除

### OLED 不显示
- 检查 I2C 地址（0x3C 或 0x3D）
- 检查接线

### HC-SR501 检测异常
- 调节灵敏度和延时电位器
- 选择 H 模式（可重复触发）

### WiFi 连接失败
- 确认是 2.4GHz WiFi
- 检查密码是否正确

### 上传失败
- 按住 BOOT 键再上传
- 尝试不同的 USB 线
- 更换 USB 端口

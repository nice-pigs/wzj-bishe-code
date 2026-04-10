# ESP32 扣子云平台版本

## 概述

这是专门为扣子云平台（Coze）开发的 ESP32 代码版本，用于将座位检测数据上传到你部署在扣子平台的服务器。

## 配置信息

- **扣子域名**: `smkcc339ny.coze.site`
- **API 端点**: `POST /api/seats/status`
- **设备标识**: `esp32-seat-001`
- **设备名称**: `图书馆座位001`

## WiFi 配置

```cpp
const char* ssid = "cs";
const char* password = "88888888";
```

## 上传的数据格式

```json
{
  "deviceKey": "esp32-seat-001",
  "deviceName": "图书馆座位001",
  "isOccupied": true,
  "temperature": 25,
  "humidity": 45,
  "lightLevel": 30000,
  "pressure1": 1,
  "pressure2": 1,
  "distance": 15,
  "pirState": 1,
  "wifiSignal": -55,
  "duration": 0,
  "detectionScore": 100
}
```

## 数据说明

- `deviceKey`: 设备唯一标识符
- `deviceName`: 设备显示名称
- `isOccupied`: 座位是否被占用（true/false）
- `temperature`: 温度（°C）
- `humidity`: 湿度（%）
- `lightLevel`: 光照强度（0-65535）
- `pressure1`: 压力传感器1状态（0/1）
- `pressure2`: 压力传感器2状态（0/1）
- `distance`: 超声波距离（cm）
- `pirState`: PIR红外状态（0/1）
- `wifiSignal`: WiFi信号强度（dBm）
- `duration`: 占用时长（秒）
- `detectionScore`: 检测置信度（0-100）

## 使用方法

### 1. 硬件连接

ESP32 与 STM32 通过串口连接：
- ESP32 RX (GPIO16) ← STM32 TX (PB10)
- ESP32 TX (GPIO17) → STM32 RX (PB11)
- 波特率：115200

### 2. Arduino IDE 配置

1. 打开 Arduino IDE
2. 选择开发板：ESP32S3 Dev Module
3. 安装依赖库：
   - WiFi（ESP32 内置）
   - ArduinoJson（通过库管理器安装）

### 3. 编译上传

1. 打开 `ESP32_CozeCloud.ino`
2. 修改 WiFi 配置（如需要）
3. 点击"上传"按钮

### 4. 查看调试信息

打开串口监视器（波特率 115200），可以看到：
- WiFi 连接状态
- 接收到的 STM32 数据
- 上传到扣子云的结果

## 调试输出示例

```
=== ESP32 Seat Detection - Coze Cloud ===
Target: smkcc339ny.coze.site
[INIT] STM32 Serial initialized
[WIFI] Connecting to cs
[WIFI] Connected!
[WIFI] IP: 10.150.235.123
[WIFI] Signal: -55 dBm
[DATA] ✓ Sensor - T:25°C, H:45%, L:30000
[DATA] ✓ State: VACANT -> OCCUPIED
[COZE] Connecting to smkcc339ny.coze.site:80
[COZE] ✓ Connected
[COZE] Sending: {"deviceKey":"esp32-seat-001",...}
[COZE] Response: HTTP/1.1 200 OK
[COZE] ✓ SUCCESS - OCCUPIED
[COZE] Data: T:25°C H:45% L:30000 P:1/1 D:15cm
```

## 上传频率

- **定期上传**: 每 5 秒上传一次完整数据
- **状态变化**: 座位状态变化时立即上传
- **心跳包**: 每 10 秒发送一次心跳

## 与本地服务器版本的区别

| 特性 | 本地版本 (ESP32_CloudUpload) | 扣子云版本 (ESP32_CozeCloud) |
|------|------------------------------|------------------------------|
| 服务器地址 | 10.150.235.41:3000 | smkcc339ny.coze.site:80 |
| 数据格式 | 简化版 | 完整版（含扩展传感器） |
| 设备标识 | 无 | deviceKey + deviceName |
| 检测置信度 | 无 | detectionScore (0-100) |
| 心跳机制 | 无 | 每10秒发送 |

## 故障排查

### 1. WiFi 连接失败
- 检查 WiFi 名称和密码是否正确
- 确认 WiFi 信号强度

### 2. 无法连接扣子服务器
- 检查扣子平台服务是否正常运行
- 确认域名 `smkcc339ny.coze.site` 可访问
- 检查防火墙设置

### 3. 没有收到 STM32 数据
- 检查串口连接是否正确
- 确认 STM32 波特率为 115200
- 查看 STM32 是否正常发送数据

### 4. 上传失败
- 查看串口监视器的错误信息
- 检查 JSON 数据格式是否正确
- 确认扣子平台 API 端点正确

## 注意事项

1. **Arduino IDE 限制**: 同一文件夹只能有一个 .ino 文件，因此本版本独立于 `ESP32_CloudUpload` 文件夹
2. **内存使用**: JSON 文档大小为 512 字节，适合 ESP32-S3
3. **网络稳定性**: 建议使用稳定的 WiFi 网络，避免频繁断线
4. **数据同步**: 扣子云版本包含更多传感器数据，适合完整的监控系统

## 相关文档

- [扣子云平台对接说明](../../扣子云平台对接说明.md)
- [云平台数据格式说明](../../云平台数据格式说明.md)
- [STM32 代码](../../STM32_code/)

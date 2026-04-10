# 座位检测系统 - 毕业设计

基于STM32F103C8T6 + ESP32-S3的智能座位占用检测系统

## 项目简介

本项目是一个完整的物联网座位检测系统，能够实时检测座位的占用状态，并通过WiFi将数据上传到云端服务器，支持网页端实时查看。

### 主要功能

- ✅ 多传感器融合检测座位占用状态（压力+超声波+PIR）
- ✅ 实时环境监测（温湿度、光照强度）
- ✅ OLED本地显示
- ✅ LED指示灯状态提示
- ✅ WiFi数据上传
- ✅ Web端实时监控
- ✅ 历史数据记录

## 系统架构

```
┌─────────────┐      串口      ┌──────────┐      WiFi      ┌─────────────┐
│   STM32     │ ──────────────> │  ESP32   │ ──────────────> │  服务器     │
│  传感器采集  │   115200bps    │  数据转发 │   HTTP POST    │  数据存储   │
└─────────────┘                └──────────┘                └─────────────┘
                                                                    │
                                                                    │ HTTP
                                                                    ▼
                                                            ┌─────────────┐
                                                            │  Web前端    │
                                                            │  实时显示   │
                                                            └─────────────┘
```

## 硬件清单

### 主控芯片
- STM32F103C8T6 单片机
- ESP32-S3 WiFi模块

### 传感器
- 压力传感器（2通道）- 主要检测
- HC-SR04 超声波测距 - 辅助检测
- PIR人体红外传感器 - 辅助检测
- DHT11 温湿度传感器 - 环境监测
- 光敏传感器 - 环境监测

### 显示与交互
- OLED显示屏（0.96寸 I2C）
- 4个按键
- LED指示灯（绿色+红色）
- 蜂鸣器

## 引脚连接

详见 [最终接线方案.md](./最终接线方案.md)

### STM32引脚分配

**PA端口：传感器 + 执行器**
- PA0: DHT11 温湿度传感器
- PA1: 光敏传感器 ADC
- PA2: 光敏传感器 数字输出
- PA3: PIR 人体红外传感器
- PA4: 蜂鸣器（低电平触发）
- PA5: 绿色LED（空闲指示）
- PA6: 红色LED（占用指示）

**PB端口：通信 + 按键**
- PB0: 超声波 Trig
- PB3: 超声波 Echo
- PB4: 压力传感器 CH1
- PB5: 压力传感器 CH2
- PB6: OLED SCL
- PB7: OLED SDA
- PB10: ESP32 TX
- PB11: ESP32 RX
- PB12-PB15: 按键 K1-K4

### ESP32引脚分配
- GPIO16: STM32 RX
- GPIO17: STM32 TX

## 检测算法

采用多传感器融合算法，权重分配如下：

- **压力传感器**：70分（任一通道触发）
- **超声波测距**：25分（距离<50cm）
- **PIR红外**：5分（检测到运动）

**判定规则**：总分 ≥ 70分 → 座位占用

## 数据格式

### ESP32上传到服务器的数据格式

```json
{
  "occupied": true,           // 座位占用状态
  "temperature": 26,          // 温度（°C）
  "humidity": 44,             // 湿度（%）
  "lightLevel": 25605,        // 光照强度（ADC值 0-65535）
  "duration": 0               // 持续时间（秒）
}
```

### 服务器API接口

#### 1. 上传座位数据
```
POST /api/seat/update
Content-Type: application/json

{
  "occupied": true,
  "temperature": 26,
  "humidity": 44,
  "lightLevel": 25605,
  "duration": 0
}
```

#### 2. 获取当前状态
```
GET /api/seat/status
```

#### 3. 设备心跳
```
POST /api/device/heartbeat
Content-Type: application/json

{
  "timestamp": 123456789
}
```

#### 4. 获取历史数据
```
GET /api/seat/history?limit=100&offset=0
```

#### 5. 获取统计数据
```
GET /api/seat/statistics
```

## 项目结构

```
bishe/
├── STM32_code/              # STM32代码
│   ├── Hardware/            # 硬件驱动
│   │   ├── Pressure.c/h     # 压力传感器
│   │   ├── Ultrasonic.c/h   # 超声波
│   │   ├── PIR.c/h          # PIR传感器
│   │   ├── DHT11.c/h        # 温湿度
│   │   ├── LightSensor.c/h  # 光敏传感器
│   │   ├── OLED.c/h         # OLED显示
│   │   ├── LED.c/h          # LED
│   │   ├── Buzzer.c/h       # 蜂鸣器
│   │   ├── Key.c/h          # 按键
│   │   └── ESP32_Comm.c/h   # ESP32通信
│   ├── User/                # 用户代码
│   │   └── main.c           # 主程序
│   └── System/              # 系统文件
│
├── ESP32_SeatDetection/     # ESP32代码
│   └── ESP32_CloudUpload/
│       └── ESP32_CloudUpload.ino
│
├── server/                  # Node.js服务器
│   ├── server_v2.js         # 单设备服务器
│   ├── server_multi.js      # 多设备服务器
│   ├── simulator.js         # 数据模拟器
│   └── package.json
│
├── web/                     # Web前端
│   ├── index.html           # 单设备页面
│   ├── multi.html           # 多设备页面
│   ├── dashboard.html       # 仪表盘
│   ├── css/
│   └── js/
│
└── 参考内容/
    ├── 需求.md              # 项目需求
    └── 设备清单.md          # 硬件清单
```

## 快速开始

### 1. STM32开发环境

**工具**：Keil MDK-ARM

**步骤**：
1. 打开 `STM32_code/Project.uvprojx`
2. 编译项目（F7）
3. 下载到STM32（F8）

**注意事项**：
- 确保已添加 `Pressure.c` 和 `Ultrasonic.c` 到项目
- 波特率设置为 115200

### 2. ESP32开发环境

**工具**：Arduino IDE

**依赖库**：
- WiFi (内置)
- ArduinoJson (6.x)

**步骤**：
1. 安装ESP32开发板支持
2. 打开 `ESP32_CloudUpload.ino`
3. 修改WiFi配置：
   ```cpp
   const char* ssid = "你的WiFi名称";
   const char* password = "你的WiFi密码";
   const char* serverIP = "服务器IP地址";
   ```
4. 编译并上传

### 3. 服务器部署

**环境**：Node.js 14+

**步骤**：
```bash
cd server
npm install
node server_v2.js
```

服务器将运行在 `http://localhost:3000`

**防火墙设置**（Windows）：
```powershell
# 添加防火墙规则
netsh advfirewall firewall add rule name="Node.js Server Port 3000" dir=in action=allow protocol=TCP localport=3000

# 禁用阻止规则（如果存在）
Get-NetFirewallRule | Where-Object {$_.DisplayName -eq "node.exe" -and $_.Action -eq "Block"} | Disable-NetFirewallRule
```

### 4. 访问Web界面

- 单设备监控：http://localhost:3000
- 多设备监控：http://localhost:3000/multi.html
- 仪表盘：http://localhost:3000/dashboard.html

## 按键功能

- **K1**：显示传感器详细信息
- **K2**：手动设置为占用
- **K3**：手动设置为空闲
- **K4**：刷新所有传感器

## 云平台对接

如果要对接扣子平台或其他云平台，设备需要上传以下数据：

### 必需数据
```json
{
  "deviceId": "seat-001",        // 设备ID
  "occupied": true,              // 占用状态
  "temperature": 26,             // 温度
  "humidity": 44,                // 湿度
  "lightLevel": 25605,           // 光照
  "timestamp": "2026-04-10T14:00:00Z"  // 时间戳
}
```

### 可选数据（增强功能）
```json
{
  "pressure1": 1,                // 压力传感器1
  "pressure2": 1,                // 压力传感器2
  "distance": 15,                // 超声波距离(cm)
  "pir": 0,                      // PIR状态
  "duration": 120,               // 占用时长(秒)
  "location": "图书馆-A区-01号",  // 位置信息
  "wifiSignal": -55              // WiFi信号强度
}
```

详见 [云平台数据格式说明.md](./云平台数据格式说明.md)

## 常见问题

### 1. ESP32连接服务器失败

**症状**：`[UPLOAD] ✗ Connection failed`

**解决方案**：
- 检查WiFi连接是否正常
- 确认服务器IP地址正确
- 检查防火墙设置
- 确保服务器监听 `0.0.0.0:3000`

### 2. STM32与ESP32通信失败

**症状**：ESP32收到 `0x00` 字节

**解决方案**：
- 检查波特率是否一致（115200）
- 检查TX/RX接线是否正确
- 确认串口初始化顺序

### 3. 传感器读取异常

**症状**：数据一直为0或异常值

**解决方案**：
- 检查传感器供电
- 确认引脚连接
- 查看串口调试信息

## 性能指标

- **检测准确率**：≥95%（多传感器融合）
- **响应时间**：<2秒（状态变化到显示）
- **数据上传间隔**：5秒
- **心跳间隔**：10秒
- **功耗**：约1.5W（工作状态）

## 开发团队

- 开发者：[你的名字]
- 指导老师：[老师名字]
- 学校：[学校名称]
- 专业：物联网工程

## 许可证

本项目仅用于学习和研究目的。

## 更新日志

### v1.0.0 (2026-04-10)
- ✅ 完成STM32传感器驱动
- ✅ 完成ESP32 WiFi通信
- ✅ 完成服务器API
- ✅ 完成Web前端界面
- ✅ 多传感器融合算法
- ✅ 系统联调成功

## 致谢

感谢所有开源库和参考资料的作者。

---

**联系方式**：[你的邮箱]

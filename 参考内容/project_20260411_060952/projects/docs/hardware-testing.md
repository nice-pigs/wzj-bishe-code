# 硬件测试指南

## 测试总览

硬件测试分为三个阶段：

| 阶段 | 内容 | 所需时间 |
|------|------|---------|
| 阶段一 | 软件模拟测试（无需硬件） | 立即可做 |
| 阶段二 | 硬件基础测试（LED、传感器） | 硬件到货后 |
| 阶段三 | 系统联调测试（WiFi、云端） | 基础测试通过后 |

---

## 阶段一：软件模拟测试（无需硬件）

### 测试目的
验证云端 API 和 Web 界面功能正常

### 测试步骤

#### 1. 测试设备注册 API

打开浏览器开发者工具（F12），在 Console 中执行：

```javascript
// 模拟设备注册
fetch('https://smkcc339ny.coze.site/api/devices/register', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    deviceKey: 'STM32-DESK-001',
    deviceName: '书桌座位监测设备'
  })
})
.then(r => r.json())
.then(d => console.log('注册结果:', d));
```

**期望结果：**
```json
{
  "success": true,
  "data": {
    "deviceId": 1,
    "deviceKey": "STM32-DESK-001",
    "deviceName": "书桌座位监测设备"
  }
}
```

#### 2. 测试状态上报 API

```javascript
// 模拟座位被占用
fetch('https://smkcc339ny.coze.site/api/seats/status', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    deviceKey: 'STM32-DESK-001',
    isOccupied: true
  })
})
.then(r => r.json())
.then(d => console.log('上报结果:', d));
```

#### 3. 测试座位查询 API

```javascript
// 查询所有座位状态
fetch('https://smkcc339ny.coze.site/api/seats')
.then(r => r.json())
.then(d => console.log('座位列表:', d));
```

#### 4. 测试历史记录 API

```javascript
// 查询座位历史记录
fetch('https://smkcc339ny.coze.site/api/seats/1/history?limit=10')
.then(r => r.json())
.then(d => console.log('历史记录:', d));
```

#### 5. 查看 Web 界面

访问 https://smkcc339ny.coze.site

确认：
- ✅ 页面能正常加载
- ✅ 能看到刚才创建的座位
- ✅ 状态显示正确
- ✅ 10秒后自动刷新

---

## 阶段二：硬件基础测试

### 前置条件
- ✅ 硬件已到货
- ✅ Keil 已安装
- ✅ ST-Link 已连接

### 测试 1：LED 闪烁测试

**目的**：验证 STM32 能正常工作

**代码**：
```c
// 在 main 函数的 while(1) 中添加
while (1)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);    // 红灯亮
    HAL_Delay(500);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);  // 红灯灭
    HAL_Delay(500);
}
```

**预期结果**：红色 LED 每 500ms 闪烁一次

---

### 测试 2：传感器读取测试

**目的**：验证传感器能正确检测人体

**代码**：
```c
while (1)
{
    // 读取红外传感器
    GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
    
    if (state == GPIO_PIN_SET)
    {
        // 检测到人，红灯亮
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
    }
    else
    {
        // 未检测到人，绿灯亮
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
    }
    
    HAL_Delay(100);
}
```

**预期结果**：
- 有人靠近时，红灯亮
- 无人时，绿灯亮

---

### 测试 3：OLED 显示测试

**目的**：验证 OLED 能正常显示

**代码**（需要 OLED 驱动库）：
```c
// 初始化 OLED
OLED_Init();
OLED_Clear();

// 显示文字
OLED_ShowString(0, 0, "Seat Monitor", 16);
OLED_ShowString(0, 2, "Status: IDLE", 16);
```

**预期结果**：OLED 显示文字

---

### 测试 4：ESP8266 AT 指令测试

**目的**：验证 ESP8266 能正常响应

**代码**：
```c
// 发送 AT 测试指令
HAL_UART_Transmit(&huart1, (uint8_t*)"AT\r\n", 4, 1000);

// 等待响应 "OK"
// 需要在串口接收中断中处理响应
```

**预期结果**：ESP8266 返回 "OK"

---

## 阶段三：系统联调测试

### 测试 1：WiFi 连接测试

**目的**：验证 ESP8266 能连接 WiFi

**步骤**：
1. 修改代码中的 WiFi 名称和密码
2. 复位 STM32
3. 观察 OLED 或串口输出

**预期结果**：显示 "WiFi Connected"

---

### 测试 2：设备注册测试

**目的**：验证设备能在云端注册

**步骤**：
1. 上电后自动执行注册
2. 检查云端数据库是否有新设备

**预期结果**：
- OLED 显示 "Register OK"
- 云端数据库 devices 表有新记录

---

### 测试 3：状态上报测试

**目的**：验证状态能正确上报到云端

**步骤**：
1. 用手遮挡红外传感器（模拟有人）
2. 等待 3 秒确认状态
3. 查看云端数据

**预期结果**：
- LED 变红
- OLED 显示 "Occupied"
- Web 界面显示座位被占用

---

### 测试 4：完整功能测试

**目的**：验证整个系统工作正常

**测试用例**：

| 序号 | 操作 | 预期结果 |
|------|------|---------|
| 1 | 上电启动 | LED闪烁，OLED显示"Connecting..." |
| 2 | WiFi连接成功 | OLED显示"WiFi OK" |
| 3 | 设备注册成功 | OLED显示"Register OK" |
| 4 | 人靠近传感器 | LED变红，OLED显示"Occupied" |
| 5 | 等待10秒 | Web界面显示占用 |
| 6 | 人离开传感器 | LED变绿，OLED显示"Idle" |
| 7 | 等待10秒 | Web界面显示空闲 |

---

## 故障排查

### 问题 1：ESP8266 无法连接 WiFi

**排查步骤**：
1. 检查 WiFi 名称和密码是否正确
2. 确认 WiFi 是 2.4GHz（不支持 5GHz）
3. 检查 ESP8266 供电是否稳定
4. 用串口助手直接发送 AT 指令测试

### 问题 2：数据上报失败

**排查步骤**：
1. 检查服务器域名是否正确
2. 用浏览器测试 API 是否正常
3. 检查 HTTP 请求格式
4. 查看串口调试输出

### 问题 3：状态检测不准确

**排查步骤**：
1. 调节 HC-SR501 灵敏度旋钮
2. 调节延时旋钮
3. 增加软件滤波逻辑

---

## 测试记录表

| 测试项 | 测试时间 | 结果 | 备注 |
|--------|---------|------|------|
| LED闪烁 | | ⬜通过 / ⬜失败 | |
| 传感器检测 | | ⬜通过 / ⬜失败 | |
| OLED显示 | | ⬜通过 / ⬜失败 | |
| WiFi连接 | | ⬜通过 / ⬜失败 | |
| 设备注册 | | ⬜通过 / ⬜失败 | |
| 状态上报 | | ⬜通过 / ⬜失败 | |
| Web显示 | | ⬜通过 / ⬜失败 | |
